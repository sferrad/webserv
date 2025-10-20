#include "../include/webserv.h"

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

    if (method != "GET" && method != "POST" && method != "DELETE" && method != "PUT") {
        return false;
    }
    this->method_ = method;
	std::cout << "\033[31m" << "Method: " << method << "\033[0m" << std::endl;
    return true;
}

void HttpRequestHandler::getUri(const std::string &request){
	// Chercher le début de la méthode
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
	std::map<int, std::string>::const_iterator it = this->errorPages.find(code);

		std::string base = this->root;
		if (it == this->errorPages.end())
			return ;
		std::string page = it->second;

		if (page.rfind("./", 0) == 0)
			page.erase(0, 2);
		std::string errPath;
		if (!page.empty() && page[0] == '/')
			errPath = base + page;
		else
			errPath = base + "/" + page;
		std::ifstream ferr(errPath.c_str());
		if (!ferr)
			return ;
		respBody_ << ferr.rdbuf();
}

int HttpRequestHandler::getHtmlPage() {
	std::string base = this->root;
	std::string uri  = this->uri_;

	std::cout << "Root: " << base << ", URI: " << uri << std::endl;

	if (uri.empty() || uri == "/")
		uri = "/" + this->index;
	else if (uri[0] != '/')
		uri = "/" + uri;

	std::string path = base + uri;

	std::ifstream file(path.c_str());
	if (!file) {
		handleError(404);
		return 0;
	}
	respBody_ << file.rdbuf();
	return 1;
}

std::string HttpRequestHandler::parseRequest(const std::string &request) {
	std::cout << "Parsing request: " << request << std::endl;
	if (isEmpty(request))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    if (!isValidMethod(request))
	{
		handleError(405);
		std::string body405 = this->respBody_.str();
		std::ostringstream resp;
		resp << "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: " << body405.size() << "\r\n\r\n" << body405;
        return resp.str();
	}

    extractBody(request);
    getUri(request);
    
    // Vérifier si la méthode est autorisée pour cette URI
    if (!isMethodAllowed(method_, uri_)) {
        handleError(405);
        std::string body405 = this->respBody_.str();
        std::ostringstream resp;
        resp << "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: " << body405.size() << "\r\n\r\n" << body405;
        return resp.str();
    }

	if (HttpRequestHandler::method_ == "POST" && isEmpty(this->body_))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

    this->respBody_.clear();
    this->respBody_.str("");
	
	getUri(request);
	std::ostringstream resp;
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
		resp << "HTTP/1.1 404 Not Found\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
		return resp.str();
	}
	bool isNewVisitor = true;
//---------------- test des cookies ----------------
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
// ----------------------------------------------------
    resp << "HTTP/1.1 200 OK\r\nContent-Length: " << body.size() << "\r\nSet-Cookie: visited=" << visitor << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/" << "\r\nSet-Cookie: visit_count=" << visit_count_ << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/" << "\r\n\r\n" << body;
    return resp.str();
}

bool HttpRequestHandler::isMethodAllowed(const std::string &method, const std::string &uri) {
	if (!serverConfig_) {
		// Pas de configuration disponible, autoriser toutes les méthodes valides
		return true;
	}
	
	// Utiliser la méthode non-const pour trouver la location
	ServerConf* nonConstConfig = const_cast<ServerConf*>(serverConfig_);
	Location* location = nonConstConfig->findLocation(uri);
	
	if (!location) {
		// Aucune location trouvée, autoriser toutes les méthodes
		return true;
	}
	
	// Vérifier si la méthode est dans la liste des méthodes autorisées
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
	
	std::cout << "DELETE request - Root: " << base << ", URI: " << uri << std::endl;
	
	// Construire le chemin complet du fichier
	if (uri[0] != '/')
		uri = "/" + uri;
	
	std::string filePath = base + uri;
	
	// Vérifier si le fichier existe
	std::ifstream file(filePath.c_str());
	if (!file) {
		std::cout << "File not found for DELETE: " << filePath << std::endl;
		handleError(404);
		return false;
	}
	file.close();
	
	// Tenter de supprimer le fichier
	if (remove(filePath.c_str()) == 0) {
		std::cout << "File deleted successfully: " << filePath << std::endl;
		// Succès - pas de contenu à retourner pour DELETE
		respBody_.clear();
		respBody_.str("");
		return true;
	} else {
		std::cout << "Failed to delete file: " << filePath << std::endl;
		handleError(500);
		return false;
	}
}

bool HttpRequestHandler::handlePostRequest() {
	// Pour l'instant, juste un succès basique
	// Plus tard on peut implémenter l'upload de fichiers
	std::cout << "POST request handled" << std::endl;
	respBody_.clear();
	respBody_.str("");
	return true;
}

bool HttpRequestHandler::handlePutRequest() {
	// Pour l'instant, juste un succès basique  
	// Plus tard on peut implémenter la création/mise à jour de fichiers
	std::cout << "PUT request handled" << std::endl;
	respBody_.clear();
	respBody_.str("");
	return true;
}