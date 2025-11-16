#include "../include/webserv1.h"

bool HttpRequestHandler::isValidMethod(const std::string &request) {
    char tmp[1024];
    strncpy(tmp, request.c_str(), sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;
    tmp[strcspn(tmp, "\r\n")] = 0;

    std::string tmpStr(tmp);
    size_t pos = tmpStr.find(' ');

    std::string method;
    if (pos == std::string::npos) {
        method = tmpStr;
    } else {
        method = tmpStr.substr(0, pos);
    }

    if (method != "GET" && method != "POST" && method != "DELETE") {
        return false;
    }
    this->method_ = method;
	std::cout << "\033[31m" << "Method: " << method << "\033[0m" << std::endl;
    return true;
}

void HttpRequestHandler::getUri(const std::string &request){
	size_t pos = request.find(method_ + " ");
	if (pos != std::string::npos) {
		size_t endPos = request.find(" ", pos + method_.length() + 1);
		if (endPos != std::string::npos) {
			this->uri_ = request.substr(pos + method_.length() + 1, endPos - (pos + method_.length() + 1));
		}
	}
}

void HttpRequestHandler::extractBody(const std::string &request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos != std::string::npos) {
        this->body_ = request.substr(pos + 4);
    }
}

void HttpRequestHandler::handleError(int code)
{
    respBody_.clear();
    respBody_.str("");

    std::string base = this->root;

    std::map<int, std::string>::const_iterator it = this->errorPages.find(code);
    if (it != this->errorPages.end())
    {
        std::string page = it->second;
        if (page.rfind("./", 0) == 0)
            page.erase(0, 2);

        std::string errPath;
        if (!page.empty() && page[0] == '/')
            errPath = base + page;
        else
            errPath = base + "/" + page;

        std::ifstream ferr(errPath.c_str());
        if (ferr)
        {
            respBody_ << ferr.rdbuf();
            return;
        }
    }

    {
        std::ostringstream fallback;
        fallback << base << "/error/" << code << ".html";
        std::ifstream ferr2(fallback.str().c_str());
        if (ferr2)
        {
            respBody_ << ferr2.rdbuf();
            return;
        }
    }

    const char *reason = "Error";
    switch (code) {
        case 400: reason = "Bad Request"; break;
        case 403: reason = "Forbidden"; break;
        case 404: reason = "Not Found"; break;
        case 405: reason = "Method Not Allowed"; break;
        case 413: reason = "Payload Too Large"; break;
        case 500: reason = "Internal Server Error"; break;
        default:  reason = "Error"; break;
    }
    respBody_ << "<html><head><title>" << code << " " << reason
              << "</title></head><body><h1>" << code << " " << reason
              << "</h1><p>The server encountered an error.</p></body></html>";
}

int HttpRequestHandler::getHtmlPage() {
	std::string base = this->root;
	std::string uri  = this->uri_;

	if (base.empty()) {
		std::cerr << "\033[91m" << "ERROR: root is empty!" << "\033[0m" << std::endl;
		handleError(500);
		return 0;
	}

	if (uri.empty()) uri = "/";
	if (!uri.empty() && uri[0] != '/') uri = "/" + uri;

	Location* matchedLoc = NULL;
	std::string effectiveIndex = this->index;
	
	if (serverConfig_) {
		ServerConf* nonConst = const_cast<ServerConf*>(serverConfig_);
		matchedLoc = nonConst->findLocation(uri);
		if (matchedLoc && !matchedLoc->index.empty())
			effectiveIndex = matchedLoc->index;
	}

	if (effectiveIndex.empty()) {
		effectiveIndex = "index.html";
	}
	std::string rel = uri;
	
	if (matchedLoc && !matchedLoc->path.empty() && uri.find(matchedLoc->path) == 0) {
		size_t pathSize = matchedLoc->path.size();
		if (pathSize <= uri.size()) {
			rel = uri.substr(pathSize);
			if (rel.empty() || rel == "/")
				rel = "/" + effectiveIndex;
			else if (rel[0] != '/')
				rel = "/" + rel;
		}
	} else {
		if (uri == "/")
			rel = "/" + effectiveIndex;
	}

	std::string path = base + rel;
	
	std::cout << "\033[96m" << "GET - Trying to open: " << path << "\033[0m" << std::endl;

	std::ifstream file(path.c_str());
	if (!file) {
		struct stat st;
		if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
			std::string withIndex = path;
			if (withIndex.size() == 0 || withIndex[withIndex.size() - 1] != '/') 
				withIndex += "/";
			withIndex += effectiveIndex;
			
			std::cout << "\033[96m" << "Directory detected, trying: " << withIndex << "\033[0m" << std::endl;
			
			std::ifstream f2(withIndex.c_str());
			if (f2) {
				respBody_ << f2.rdbuf();
				return 1;
			}
		}
		std::cout << "\033[93m" << "File not found: " << path << "\033[0m" << std::endl;
		handleError(404);
		return 0;
	}
	
	std::cout << "\033[92m" << "✓ File found and opened: " << path << "\033[0m" << std::endl;
	respBody_ << file.rdbuf();
	return 1;
}

bool HttpRequestHandler::parseHeader(const std::string &request) {
	this->resp_.clear();
    this->resp_.str("");
	if (!isValidMethod(request))
	{
		std::cout << "Invalid Method Detected" << std::endl;
		handleError(405);
		std::string body405 = this->respBody_.str();
		this->resp_ << "HTTP/1.1 405 Method Not Allowed\r\nServer: WebServ\r\nContent-Length: " << body405.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" << body405;
		return false;
	}

	bool isHttp11 = (request.find("HTTP/1.1") != std::string::npos);
	bool hasHost = (request.find("\r\nHost:") != std::string::npos)
		|| (request.find("\nHost:") != std::string::npos)
		|| (request.find("Host:") == 0);
	if (isHttp11 && !hasHost)
	{
		std::cout << "Missing Host header for HTTP/1.1" << std::endl;
		handleError(400);
		std::string body400 = this->respBody_.str();
		this->resp_ << "HTTP/1.1 400 Bad Request\r\nServer: WebServ\r\nContent-Length: " << body400.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" << body400;
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////// MODIFS BILAL ////////////////////////////
//////////////////////////////////////////////////////////////////////

std::string HttpRequestHandler::parseRequest(const std::string &request) {
	std::cout << "Parsing request: \n\n" << request << std::endl;

	fullRequest_ = request;

	_isChunked = isChunked();
	if (_isChunked) {
    	std::cout << "\033[96m" << "🔍 Transfer-Encoding: chunked detected!" << "\033[0m" << std::endl;
	} else {
    	std::cout << "\033[96m" << "🔍 Transfer-Encoding: chunked NOT detected" << "\033[0m" << std::endl;
	}
	
	// Check 1: Request vide
	if (isEmpty(request))
		return "HTTP/1.1 400 Bad Request\r\nServer: WebServ\r\nContent-Length: 0\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
	
	// Check 2: Headers valides
	if (!parseHeader(request))
		return this->resp_.str();
	
	getUri(request);

	// 🔥 Check 3: Route existe ? (pour les méthodes autres que GET)
	Location* matchedLocation = NULL;
	if (serverConfig_) {
		ServerConf* nonConst = const_cast<ServerConf*>(serverConfig_);
		matchedLocation = nonConst->findLocation(uri_);
		
		// 🔍 DEBUG : Afficher ce que findLocation() retourne
		if (matchedLocation) {
			std::cout << "\033[96m" << "🔍 findLocation(\"" << uri_ << "\") → Found: " << matchedLocation->path << "\033[0m" << std::endl;
		} else {
			std::cout << "\033[93m" << "🔍 findLocation(\"" << uri_ << "\") → NULL (no match)" << "\033[0m" << std::endl;
		}
		
		// Pour POST/DELETE/PUT : vérifier que la location matchée n'est pas juste "/" par défaut
		// Si c'est "/" mais que l'URI n'est pas exactement "/", c'est un 404
		if (method_ == "POST" || method_ == "DELETE" || method_ == "PUT") {
			if (!matchedLocation) {
				// Aucune location → 404
				std::cout << "\033[91m" << "❌ Route NOT FOUND for " << method_ << ": " << uri_ << " (no location)" << "\033[0m" << std::endl;
				handleError(404);
				std::string body404 = this->respBody_.str();
				std::ostringstream resp;
				resp << "HTTP/1.1 404 Not Found\r\nServer: WebServ\r\nContent-Length: " 
				     << body404.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" 
				     << body404;
				return resp.str();
			} else if (matchedLocation->path == "/" && uri_ != "/") {
				// Location "/" par défaut mais URI différent → 404
				std::cout << "\033[91m" << "❌ Route NOT FOUND for " << method_ << ": " << uri_ << " (fallback to / but uri != /)" << "\033[0m" << std::endl;
				handleError(404);
				std::string body404 = this->respBody_.str();
				std::ostringstream resp;
				resp << "HTTP/1.1 404 Not Found\r\nServer: WebServ\r\nContent-Length: " 
				     << body404.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" 
				     << body404;
				return resp.str();
			}
		}
	}

	// 🔥 Check 4: Méthode autorisée ?
	std::cout << "\033[33m" << "Checking if method " << method_ << " is allowed for URI: " << uri_ << "\033[0m" << std::endl;
	if (!isMethodAllowed(method_, uri_)) {
		std::cout << "\033[91m" << "❌ Method " << method_ << " NOT ALLOWED for URI: " << uri_ << "\033[0m" << std::endl;
		handleError(405);
		std::string body405 = this->respBody_.str();
		std::ostringstream resp;
		resp << "HTTP/1.1 405 Method Not Allowed\r\nServer: WebServ\r\nContent-Length: " << body405.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" << body405;
		return resp.str();
	}
	std::cout << "\033[92m" << "✅ Method " << method_ << " ALLOWED for URI: " << uri_ << "\033[0m" << std::endl;
	
	// Check 5: POST doit avoir Content-Length
	if (method_ == "POST") {
    	bool hasContentLength = (request.find("Content-Length:") != std::string::npos || 
    	                     request.find("content-length:") != std::string::npos);
		
    	// ✅ Si chunked, pas besoin de Content-Length
    	if (!hasContentLength && !_isChunked) {
    	    std::cout << "\033[91m" << "ERROR: POST request without Content-Length or Transfer-Encoding: chunked" << "\033[0m" << std::endl;
    	    handleError(411);  // 411 Length Required
    	    std::string body411 = this->respBody_.str();
    	    std::ostringstream resp;
    	    resp << "HTTP/1.1 411 Length Required\r\nServer: WebServ\r\nContent-Length: " << body411.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" << body411;
    	    return resp.str();
    }
		
		size_t contentLength = extractContentLength(request);
		
		// Check 6: POST avec body vide (optionnel selon ton implémentation)
		if (contentLength == 0) {
			std::cout << "\033[93m" << "⚠️  POST request with Content-Length: 0 (empty body)" << "\033[0m" << std::endl;
			// Tu peux choisir de retourner 400 ou accepter les POST vides
			// Pour l'instant, on accepte
		}
		
		// ⚠️ NOTE: Le check 413 est maintenant fait AVANT dans Server.cpp handleReadEvent()
		// On n'a plus besoin de le faire ici car la connexion est déjà fermée si trop gros
	}

	extractBody(request);

	// 🔄 SI chunked, décoder le body
	if (_isChunked && !body_.empty()) {
    	std::cout << "\033[94m" << "🔄 Body is chunked, decoding..." << "\033[0m" << std::endl;
    	std::string decodedBody = unchunkBody(body_);
    	body_ = decodedBody;
    
    	// Mettre à jour Content-Length avec la vraie taille
    	std::stringstream ss;
    	ss << body_.size();
    
    	// Mettre à jour dans fullRequest_ si besoin (pour extractContentLength)
    	size_t clPos = fullRequest_.find("Content-Length:");
    	if (clPos == std::string::npos) {
	        clPos = fullRequest_.find("content-length:");
    	}
    
    	std::cout << "\033[92m" << "✅ Body decoded. New size: " << body_.size() << " bytes" << "\033[0m" << std::endl;
	}

	this->respBody_.clear();
	this->respBody_.str("");

	// Traiter la requête selon la méthode
	bool found = true;
	if (method_ == "GET") {
		found = (getHtmlPage() != 0);
	}
	else if (method_ == "DELETE") {
		found = handleDeleteRequest();
	}
	else if (method_ == "POST") {
		found = handlePostRequest();
	}
	else if (method_ == "PUT") {
		found = handlePutRequest();
	}
	
	std::string body = this->respBody_.str();
	if (!found) {
		handleError(404);
		body = this->respBody_.str();
		this->resp_ << "HTTP/1.1 404 Not Found\r\n" << "Server: WebServ" << "\r\nContent-Length: " << body.size() << "\r\nContent-Type: text/html\r\n" << "Connection: close" << "\r\n\r\n" << body;
		return this->resp_.str();
	}
	
	// Gestion des cookies (visiteurs)
	bool isNewVisitor = true;
	if (request.find("Cookie:") != std::string::npos && request.find("visited=") != std::string::npos)
	{
		isNewVisitor = false;
	}
	std::string visitor;
	if (isNewVisitor) {
		visitor = "Bienvenue nouveau visiteur!";
		visit_count_++;
	}
	else
		visitor = "bon retour, visiteur!";

	resp_ << "HTTP/1.1 200 OK" << "\r\nServer: WebServ" << "\r\nContent-Length: " << body.size()<< "\r\nContent-Type: text/html\r\nSet-Cookie: visited=" << visitor << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/\r\nSet-Cookie: visit_count=" << visit_count_ << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/\r\nConnection: close\r\n\r\n" << body;
	return resp_.str();
}

bool HttpRequestHandler::isMethodAllowed(const std::string &method, const std::string &uri) {
	if (!serverConfig_) {
		return true;
	}
	
	ServerConf* nonConstConfig = const_cast<ServerConf*>(serverConfig_);
	Location* location = nonConstConfig->findLocation(uri);
	
	if (!location) {
		return true;
	}

	const std::vector<std::string>& allowedMethods = location->allowed_methods;
	for (size_t i = 0; i < allowedMethods.size(); i++) {
		if (allowedMethods[i] == method) {
			return true;
		}
	}
	
	return false;
}

bool HttpRequestHandler::handleDeleteRequest() {
	std::string base = this->root;
	std::string uri = this->uri_;

	if (uri.empty()) uri = "/";
	if (!uri.empty() && uri[0] != '/') uri = "/" + uri;

	if (serverConfig_) {
		ServerConf* nonConst = const_cast<ServerConf*>(serverConfig_);
		Location* loc = nonConst->findLocation(uri);
		if (loc && uri.find(loc->path) == 0) {
			std::string rel = uri.substr(loc->path.size());
			if (rel.empty() || rel == "/") rel = "/";
			else if (rel[0] != '/') rel = "/" + rel;
			uri = rel;
		}
	}

	std::string filePath = base + uri;

	std::cout << "\033[96m" << "DELETE request - Root: " << base << ", URI: " << this->uri_ << ", FilePath: " << filePath << "\033[0m" << std::endl;

	std::ifstream file(filePath.c_str());
	if (!file) {
		std::cout << "\033[93m" << "File not found for DELETE: " << filePath << "\033[0m" << std::endl;
		handleError(404);
		return false;
	}
	file.close();

	if (remove(filePath.c_str()) == 0) {
		std::cout << "\033[92m" << "File deleted successfully: " << filePath << "\033[0m" << std::endl;
		respBody_.clear();
		respBody_.str("");
		return true;
	} else {
		std::cout << "\033[91m" << "Failed to delete file: " << filePath << "\033[0m" << std::endl;
		handleError(500);
		return false;
	}
}

bool HttpRequestHandler::handlePutRequest() {
	std::cout << "\033[95m" << "PUT request handled" << "\033[0m" << std::endl;
	respBody_.clear();
	respBody_.str("");
	return true;
}

size_t HttpRequestHandler::extractContentLength(const std::string &request) {
	if (_isChunked) {
        std::cout << "\033[96m" << "Content-Length: N/A (chunked transfer)" << "\033[0m" << std::endl;
        return 0;
    }
	size_t pos = request.find("Content-Length:");
	if (pos == std::string::npos) {
		pos = request.find("content-length:");
	}
	if (pos == std::string::npos) {
		return 0;
	}

	size_t startValue = request.find(":", pos) + 1;
	size_t endLine = request.find("\r\n", startValue);
	if (endLine == std::string::npos) {
		endLine = request.find("\n", startValue);
	}

	std::string lengthStr = request.substr(startValue, endLine - startValue);

	size_t first = lengthStr.find_first_not_of(" \t\r\n");
	size_t last = lengthStr.find_last_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return 0;
	}
	lengthStr = lengthStr.substr(first, last - first + 1);
	
	std::istringstream iss(lengthStr);
	size_t length = 0;
	iss >> length;

	std::cout << "\033[96m" << "Content-Length extracted: " << length << " bytes" << "\033[0m" << std::endl;
	return length;
}

std::string HttpRequestHandler::extractContentType(const std::string &request) {
    size_t pos = request.find("Content-Type:");
    if (pos == std::string::npos) {
        pos = request.find("content-type:");
    }
    if (pos == std::string::npos) {
        std::cout << "\033[93m" << "⚠️  No Content-Type header, using default: text/plain" << "\033[0m" << std::endl;
        return "text/plain";  // Comportement par défaut HTTP
    }

    size_t startValue = request.find(":", pos) + 1;
    size_t endLine = request.find("\r\n", startValue);
    if (endLine == std::string::npos) {
        endLine = request.find("\n", startValue);
    }

    std::string contentType = request.substr(startValue, endLine - startValue);

    size_t first = contentType.find_first_not_of(" \t\r\n");
    size_t last = contentType.find_last_not_of(" \t\r\n");
    if (first == std::string::npos) {
        std::cout << "\033[93m" << "⚠️  Empty Content-Type header, using default: text/plain" << "\033[0m" << std::endl;
        return "text/plain";
    }
    contentType = contentType.substr(first, last - first + 1);

    std::cout << "\033[96m" << "Content-Type extracted: " << contentType << "\033[0m" << std::endl;
    return contentType;
}

// 🔥 MODIFIER isValidContentType() pour accepter explicitement "text/plain" par défaut

bool HttpRequestHandler::isValidContentType(const std::string &contentType) {
    // Si vide ou "text/plain" (défaut), toujours accepter
    if (contentType.empty() || contentType == "text/plain") {
        return true;
    }
    
    std::vector<std::string> validTypes;
    validTypes.push_back("text/html");
    validTypes.push_back("application/x-www-form-urlencoded");
    validTypes.push_back("application/json");
    validTypes.push_back("application/xml");
    validTypes.push_back("application/octet-stream");
    
    if (contentType.find("multipart/form-data") != std::string::npos) {
        return true;
    }
    
    std::string mimeType = contentType;
    size_t semicolon = mimeType.find(';');
    if (semicolon != std::string::npos) {
        mimeType = mimeType.substr(0, semicolon);
    }
    
    size_t first = mimeType.find_first_not_of(" \t");
    size_t last = mimeType.find_last_not_of(" \t");
    if (first != std::string::npos) {
        mimeType = mimeType.substr(first, last - first + 1);
    }
    
    for (size_t i = 0; i < validTypes.size(); i++) {
        if (mimeType == validTypes[i]) {
            return true;
        }
    }
    
    std::cout << "\033[91m" << "❌ Invalid Content-Type: " << contentType << "\033[0m" << std::endl;
    return false;
}

std::map<std::string, std::string> HttpRequestHandler::parseUrlEncoded(const std::string &body) {
    std::map<std::string, std::string> params;
    
    std::string key, value;
    bool inKey = true;
    
    for (size_t i = 0; i < body.size(); i++) {
        char c = body[i];
        
        if (c == '=') {
            inKey = false;
        } else if (c == '&') {
            if (!key.empty()) {
                params[key] = value;
            }
            key.clear();
            value.clear();
            inKey = true;
        } else if (c == '+') {
            // '+' = espace en URL encoding
            if (inKey) key += ' ';
            else value += ' ';
        } else if (c == '%' && i + 2 < body.size()) {
            // Décodage hexadécimal : %20 = espace, %2B = +, etc.
            std::string hex = body.substr(i + 1, 2);
            char decoded = 0;
            
            for (int j = 0; j < 2; j++) {
                decoded *= 16;
                char h = hex[j];
                if (h >= '0' && h <= '9') decoded += h - '0';
                else if (h >= 'A' && h <= 'F') decoded += h - 'A' + 10;
                else if (h >= 'a' && h <= 'f') decoded += h - 'a' + 10;
            }
            
            if (inKey) key += decoded;
            else value += decoded;
            i += 2;
        } else {
            if (inKey) key += c;
            else value += c;
        }
    }
    
    // Dernier paramètre
    if (!key.empty()) {
        params[key] = value;
    }
    
    std::cout << "\033[96m" << "Parsed " << params.size() << " URL-encoded parameters:" << "\033[0m" << std::endl;
    for (std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it) {
        std::cout << "  " << it->first << " = " << it->second << std::endl;
    }
    
    return params;
}

bool HttpRequestHandler::handlePostRequest() {
	std::cout << "\033[94m" << "=== POST REQUEST HANDLER ===" << "\033[0m" << std::endl;
	
	size_t contentLength;
	if (_isChunked) {
		contentLength = body_.size();
		std::cout << "\033[96m" << "✓ Chunked transfer - Decoded body size: " << contentLength << " bytes" << "\033[0m" << std::endl;
	} else {
		contentLength = extractContentLength(fullRequest_);
		std::cout << "\033[92m" << "✓ Content-Length OK: " << contentLength << " bytes" << "\033[0m" << std::endl;
	}
	std::string contentType = extractContentType(fullRequest_);
	

	if (!contentType.empty() && !isValidContentType(contentType)) {
        std::cout << "\033[91m" << "ERROR: Invalid or unsupported Content-Type" << "\033[0m" << std::endl;
        handleError(415);  // 415 Unsupported Media Type
        return false;
    }


	std::cout << "\033[92m" << "✓ Body received: " << body_.size() << " bytes" << "\033[0m" << std::endl;

	// ========== NOUVEAU : Détecter multipart/form-data ==========
	if (contentType.find("multipart/form-data") != std::string::npos) {
		std::cout << "\033[96m" << "📦 Multipart upload detected!" << "\033[0m" << std::endl;
		
		// Extraire le boundary
		std::string boundary = extractBoundary(contentType);
		if (boundary.empty()) {
			std::cout << "\033[91m" << "ERROR: No boundary found!" << "\033[0m" << std::endl;
			handleError(400);
			return false;
		}
		
		// Séparer les parts
		std::vector<std::string> parts = splitMultipart(body_, boundary);
		if (parts.empty()) {
			std::cout << "\033[91m" << "ERROR: No parts found in multipart!" << "\033[0m" << std::endl;
			handleError(400);
			return false;
		}
		
		// Trouver le dossier d'upload
		std::string uploadDir = "./www/uploads";  // Valeur par défaut
		if (serverConfig_) {
			ServerConf* nonConst = const_cast<ServerConf*>(serverConfig_);
			Location* loc = nonConst->findLocation(uri_);
			if (loc && !loc->upload_store.empty()) {
				uploadDir = loc->upload_store;
			}
		}
		
		std::cout << "\033[96m" << "Upload directory: " << uploadDir << "\033[0m" << std::endl;
		
		// Traiter chaque part
		int filesUploaded = 0;
		for (size_t i = 0; i < parts.size(); i++) {
			std::string filename;
			std::string fileContent;
			
			if (extractFileFromPart(parts[i], filename, fileContent)) {
				// Sécuriser le nom de fichier
				filename = sanitizeFilename(filename);
				
				// Créer le chemin complet
				std::string fullPath = uploadDir;
				if (fullPath[fullPath.size() - 1] != '/') {
					fullPath += "/";
				}
				fullPath += filename;
				
				// Sauvegarder le fichier
				std::ofstream outFile(fullPath.c_str(), std::ios::binary);
				if (outFile) {
					outFile.write(fileContent.c_str(), fileContent.size());
					outFile.close();
					std::cout << "\033[92m" << "✓ File saved: " << fullPath << " (" << fileContent.size() << " bytes)" << "\033[0m" << std::endl;
					filesUploaded++;
				} else {
					std::cout << "\033[91m" << "ERROR: Failed to save file: " << fullPath << "\033[0m" << std::endl;
				}
			}
		}
		
		// Générer la réponse de succès
		respBody_.clear();
		respBody_.str("");
		respBody_ << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Upload Success</title>";
		respBody_ << "<style>body{font-family:system-ui;max-width:800px;margin:40px auto;padding:20px;background:#0f172a;color:#e2e8f0;}";
		respBody_ << "pre{background:#1e293b;padding:12px;border-radius:8px;overflow-x:auto;}</style></head><body>";
		respBody_ << "<h1>🎉 Upload successful!</h1>";
		respBody_ << "<p><strong>Files uploaded:</strong> " << filesUploaded << "</p>";
		respBody_ << "<p><strong>Upload directory:</strong> <code>" << uploadDir << "</code></p>";
		respBody_ << "<p><a href='/uploads'>📂 View uploads folder</a> | <a href='/'>🏠 Back to home</a></p>";
		respBody_ << "</body></html>";
		
		return true;
	}
	
	// ========== Code existant pour les autres types ==========
	respBody_.clear();
	respBody_.str("");
	respBody_ << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>POST Success</title>";
	respBody_ << "<style>body{font-family:system-ui;max-width:800px;margin:40px auto;padding:20px;background:#0f172a;color:#e2e8f0;}";
	respBody_ << "pre{background:#1e293b;padding:12px;border-radius:8px;overflow-x:auto;}</style></head><body>";
	respBody_ << "<h1>&#x2713; POST request received successfully!</h1>";
	respBody_ << "<p><strong>Content-Length:</strong> " << contentLength << " bytes</p>";
	respBody_ << "<p><strong>Content-Type:</strong> " << contentType << "</p>";
	respBody_ << "<p><strong>Body size received:</strong> " << body_.size() << " bytes</p>";
	respBody_ << "<h2>Body content:</h2><pre>" << body_ << "</pre>";
	respBody_ << "<p><a href='/'>&#x2190; Back to home</a></p>";
	respBody_ << "</body></html>";

	return true;
}

//////////////////////////////////////////////////////////////////////
///////////////// FONCTIONS UTILITAIRES MULTIPART ///////////////////
//////////////////////////////////////////////////////////////////////

std::string HttpRequestHandler::extractBoundary(const std::string &contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos) {
        return "";
    }
    
    std::string boundary = contentType.substr(pos + 9);
    
    if (!boundary.empty() && boundary[0] == '"') {
        boundary.erase(0, 1);
    }
    if (!boundary.empty() && boundary[boundary.size() - 1] == '"') {
        boundary.erase(boundary.size() - 1);
    }
    
    std::cout << "\033[96m" << "Boundary extracted: --" << boundary << "\033[0m" << std::endl;
    return boundary;
}

std::vector<std::string> HttpRequestHandler::splitMultipart(const std::string &body, const std::string &boundary) {
    std::vector<std::string> parts;
    std::string delimiter = "--" + boundary;
    std::string endDelimiter = "--" + boundary + "--";
    
    size_t pos = 0;
    size_t nextPos = 0;
    
    pos = body.find(delimiter);
    if (pos == std::string::npos) {
        return parts;
    }
    pos += delimiter.length();
    
    while ((nextPos = body.find(delimiter, pos)) != std::string::npos) {
        std::string part = body.substr(pos, nextPos - pos);
        
        size_t start = part.find_first_not_of("\r\n");
        if (start != std::string::npos) {
            part = part.substr(start);
        }
        
        if (!part.empty() && part.find(endDelimiter) == std::string::npos) {
            parts.push_back(part);
        }
        
        pos = nextPos + delimiter.length();
    }
    
    std::cout << "\033[92m" << "Found " << parts.size() << " parts in multipart body" << "\033[0m" << std::endl;
    return parts;
}

bool HttpRequestHandler::extractFileFromPart(const std::string &part, std::string &filename, std::string &fileContent) {
    size_t dispositionPos = part.find("Content-Disposition:");
    if (dispositionPos == std::string::npos) {
        return false;
    }
    
    size_t filenamePos = part.find("filename=", dispositionPos);
    if (filenamePos == std::string::npos) {
        return false;
    }
    
    size_t filenameStart = part.find('"', filenamePos) + 1;
    size_t filenameEnd = part.find('"', filenameStart);
    filename = part.substr(filenameStart, filenameEnd - filenameStart);
    
    size_t bodyStart = part.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        bodyStart = part.find("\n\n");
        if (bodyStart == std::string::npos) {
            return false;
        }
        bodyStart += 2;
    } else {
        bodyStart += 4;
    }
    
    fileContent = part.substr(bodyStart);
    
    while (!fileContent.empty() && (fileContent[fileContent.size() - 1] == '\r' || 
                                     fileContent[fileContent.size() - 1] == '\n')) {
        fileContent.erase(fileContent.size() - 1);
    }
    
    std::cout << "\033[96m" << "Extracted file: " << filename << " (" << fileContent.size() << " bytes)" << "\033[0m" << std::endl;
    return true;
}

std::string HttpRequestHandler::sanitizeFilename(const std::string &filename) {
    std::string safe = filename;
    
    for (size_t i = 0; i < safe.size(); i++) {
        char c = safe[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || 
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            safe[i] = '_';
        }
    }
    
    if (safe.empty()) {
        safe = "unnamed_file";
    }
    
    std::cout << "\033[93m" << "Sanitized filename: " << filename << " -> " << safe << "\033[0m" << std::endl;
    return safe;
}

////////////////////////////////////////
//////// CHUNKED ENCODING //////////////
////////////////////////////////////////

bool HttpRequestHandler::isChunked() const {
    // Chercher le header Transfer-Encoding dans fullRequest_
    size_t pos = fullRequest_.find("Transfer-Encoding:");
    if (pos == std::string::npos) {
        pos = fullRequest_.find("transfer-encoding:");
    }
    if (pos == std::string::npos) {
        return false;
    }
    
    // Extraire la valeur
    size_t startValue = fullRequest_.find(":", pos) + 1;
    size_t endLine = fullRequest_.find("\r\n", startValue);
    if (endLine == std::string::npos) {
        endLine = fullRequest_.find("\n", startValue);
    }
    
    std::string value = fullRequest_.substr(startValue, endLine - startValue);
    
    // Trim et chercher "chunked"
    size_t first = value.find_first_not_of(" \t\r\n");
    size_t last = value.find_last_not_of(" \t\r\n");
    if (first != std::string::npos) {
        value = value.substr(first, last - first + 1);
    }
    
    // Convertir en minuscules pour comparaison
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    return (value.find("chunked") != std::string::npos);
}

std::string HttpRequestHandler::unchunkBody(const std::string& chunkedBody) {
    std::string result;
    size_t pos = 0;
    
    std::cout << "\033[94m" << "🔄 Starting unchunking process..." << "\033[0m" << std::endl;
    std::cout << "\033[96m" << "📦 Chunked body size: " << chunkedBody.size() << " bytes" << "\033[0m" << std::endl;
    
    while (pos < chunkedBody.size()) {
        // 1. Trouver la fin de la ligne de taille (jusqu'à \r\n)
        size_t crlfPos = chunkedBody.find("\r\n", pos);
        if (crlfPos == std::string::npos) {
            std::cerr << "\033[91m" << "❌ ERROR: Missing \\r\\n after chunk size at pos " << pos << "\033[0m" << std::endl;
            break;
        }
        
        // 2. Extraire la taille en hexa
        std::string sizeStr = chunkedBody.substr(pos, crlfPos - pos);
        
        // Nettoyer les espaces
        size_t first = sizeStr.find_first_not_of(" \t\r\n");
        size_t last = sizeStr.find_last_not_of(" \t\r\n");
        if (first != std::string::npos) {
            sizeStr = sizeStr.substr(first, last - first + 1);
        }
        
        std::cout << "\033[96m" << "📏 Chunk size (hex): '" << sizeStr << "'" << "\033[0m" << std::endl;
        
        // 3. Convertir hexa -> decimal
        unsigned long chunkSize;
        std::stringstream ss;
        ss << std::hex << sizeStr;
        ss >> chunkSize;
        
        // Vérifier si la conversion a échoué
        if (ss.fail()) {
            std::cerr << "\033[91m" << "❌ ERROR: Invalid hex chunk size: " << sizeStr << "\033[0m" << std::endl;
            break;
        }
        
        std::cout << "\033[96m" << "📏 Chunk size (dec): " << chunkSize << " bytes" << "\033[0m" << std::endl;
        
        // 4. Si taille = 0, c'est le dernier chunk
        if (chunkSize == 0) {
            std::cout << "\033[92m" << "✅ Found last chunk (size 0)" << "\033[0m" << std::endl;
            break;
        }
        
        // 5. Se positionner après le \r\n de la ligne de taille
        pos = crlfPos + 2;
        
        // 6. Vérifier qu'on a assez de données
        if (pos + chunkSize > chunkedBody.size()) {
            std::cerr << "\033[91m" << "❌ ERROR: Chunk size " << chunkSize 
                      << " exceeds remaining data (" << (chunkedBody.size() - pos) << " bytes)" << "\033[0m" << std::endl;
            break;
        }
        
        // 7. Extraire les données du chunk
        std::string chunkData = chunkedBody.substr(pos, chunkSize);
        result += chunkData;
        std::cout << "\033[92m" << "✅ Extracted " << chunkSize << " bytes" << "\033[0m" << std::endl;
        
        // 8. Avancer après les données du chunk
        pos += chunkSize;
        
        // 9. Vérifier et sauter le \r\n après les données
        if (pos + 2 <= chunkedBody.size() && 
            chunkedBody.substr(pos, 2) == "\r\n") {
            pos += 2;
        } else {
            std::cerr << "\033[93m" << "⚠️  WARNING: Missing \\r\\n after chunk data at pos " << pos << "\033[0m" << std::endl;
            // Essayer de continuer quand même
        }
    }
    
    std::cout << "\033[92m" << "✅ Unchunking complete! Final size: " 
              << result.size() << " bytes" << "\033[0m" << std::endl;
    
    return result;
}

bool HttpRequestHandler::isChunkedBodyComplete(const std::string& body) {
    // Le body chunked est complet s'il se termine par "0\r\n\r\n"
    if (body.size() < 5) {
        return false;
    }
    
    // Chercher la séquence de fin
    size_t pos = body.rfind("0\r\n\r\n");
    if (pos != std::string::npos) {
        std::cout << "\033[92m" << "✅ Chunked body is complete (found 0\\r\\n\\r\\n)" << "\033[0m" << std::endl;
        return true;
    }
    
    std::cout << "\033[93m" << "⏳ Chunked body incomplete (waiting for 0\\r\\n\\r\\n)" << "\033[0m" << std::endl;
    return false;
}

