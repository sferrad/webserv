#include "../include/webserv.h"

bool HttpRequestHandler::isValidMethod(const std::string &request)
{
	char tmp[1024];
	strncpy(tmp, request.c_str(), sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = 0;
	tmp[strcspn(tmp, "\r\n")] = 0;

	std::string tmpStr(tmp);
	size_t pos = tmpStr.find(' ');

	std::string method;
	if (pos == std::string::npos)
	{
		method = tmpStr;
	}
	else
	{
		method = tmpStr.substr(0, pos);
	}
	if (method != "GET" && method != "POST" && method != "DELETE" && method != "PUT")
	{
		return false;
	}
	this->method_ = method;
	std::cout << "\033[32m" << "[" << getCurrentTime() << "] " << "Method detected: " << method << "\033[0m" << std::endl;
	return true;
}

std::string HttpRequestHandler::normalizeUri(const std::string &uri)
{
	std::string normalized = uri;

	std::string result;
	bool lastWasSlash = false;
	for (size_t i = 0; i < normalized.size(); ++i) {
		char c = normalized[i];
		if (c == '/') {
			if (!lastWasSlash) {
				result += c;
				lastWasSlash = true;
			}
		} else {
			result += c;
			lastWasSlash = false;
		}
	}

	if (result.empty() || result[0] != '/')
		result = "/" + result;
	
	return result;
}

void HttpRequestHandler::getUri(const std::string &request)
{
	size_t pos = request.find(method_ + " ");
	if (pos != std::string::npos)
	{
		size_t endPos = request.find(" ", pos + method_.length() + 1);
		if (endPos != std::string::npos)
		{
			std::string fullUri = request.substr(pos + method_.length() + 1, endPos - (pos + method_.length() + 1));

			size_t queryPos = fullUri.find('?');
			if (queryPos != std::string::npos)
			{
				this->uri_ = normalizeUri(fullUri.substr(0, queryPos));
				this->queryString_ = fullUri.substr(queryPos + 1);
				std::cout << "\033[36m[" << getCurrentTime() << "] "
						  << "Query string detected: " << this->queryString_ << "\033[0m" << std::endl;
			}
			else
			{
				this->uri_ = normalizeUri(fullUri);
				this->queryString_.clear();
			}
			
			std::cout << "\033[36m[" << getCurrentTime() << "] "
					  << "URI normalized: " << fullUri << " -> " << this->uri_ << "\033[0m" << std::endl;
		}
	}
}

void HttpRequestHandler::parseHeaders(const std::string &request)
{
	this->headers_.clear();
	std::istringstream stream(request);
	std::string line;

	std::getline(stream, line);

	while (std::getline(stream, line) && line != "\r")
	{
		if (line.empty()) break;
		if (line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
			
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos)
		{
			std::string key = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);

			size_t firstNotSpace = value.find_first_not_of(" \t");
			if (firstNotSpace != std::string::npos)
				value = value.substr(firstNotSpace);
			else
				value = "";
				
			this->headers_[key] = value;
		}
	}
}

void HttpRequestHandler::extractBody(const std::string &request)
{
    if (isTransferEncodingChunked(request)) {
        std::string chunkedBody;
        size_t totalSize = 0;
        
        if (extractChunkedBody(request, chunkedBody, totalSize)) {
            this->body_ = chunkedBody;
            std::cout << "\033[92m[" << getCurrentTime() << "] "
                      << "✅ Chunked body extracted: " << totalSize << " bytes"
                      << "\033[0m" << std::endl;
        } else {
			this->body_.clear();
        }
    } else {
        size_t pos = request.find("\r\n\r\n");
        if (pos != std::string::npos) {
            this->body_ = request.substr(pos + 4);
        }
    }
}

void HttpRequestHandler::handleError(int code)
{
	respBody_.clear();
	respBody_.str("");

	std::string base = this->root;

	std::map<int, std::string>::const_iterator it = this->errorPages.find(code);
	if (code == 403)
		is403Forbidden_ = true;
	if (it != this->errorPages.end())
	{
		std::string page = it->second;

		std::string errPath;
		std::string serverRoot = base;
		if (serverConfig_)
			serverRoot = serverConfig_->getRoot();
		if (page.rfind("./", 0) == 0)
		{
			std::string rel = page.substr(2);
			if (!rel.empty() && rel[0] == '/')
				errPath = serverRoot + rel;
			else
				errPath = serverRoot + "/" + rel;
		}
		else if (!page.empty() && page[0] == '/')
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
		fallback << "./www/error/" << code << ".html";
		std::ifstream ferr2(fallback.str().c_str());
		if (ferr2)
		{
			respBody_ << ferr2.rdbuf();
			return;
		}
	}

	const char *reason = "Error";
	switch (code)
	{
	case 400:
		reason = "Bad Request";
		break;
	case 403:
		reason = "Forbidden";
		is403Forbidden_ = true;
		break;
	case 404:
		reason = "Not Found";
		break;
	case 405:
		reason = "Method Not Allowed";
		break;
	case 413:
		reason = "Payload Too Large";
		break;
	case 500:
		reason = "Internal Server Error";
		break;
	default:
		reason = "Error";
		break;
	}
	respBody_ << "<html><head><title>" << code << " " << reason
			  << "</title></head><body><h1>" << code << " " << reason
			  << "</h1><p>The server encountered an error.</p></body></html>";
}

std::string HttpRequestHandler::generateAutoindex(const std::string &dirPath, const std::string &uri)
{
	DIR *dir = opendir(dirPath.c_str());
	if (!dir)
		return "<html><body><h1>403 Forbidden</h1></body></html>";

	std::ostringstream out;
	out << "<html><head><title>Index of " << uri << "</title></head><body>";
	out << "<h1>Index of " << uri << "</h1><ul>";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == ".")
			continue;
		out << "<li><a href=\"" << uri;
		if (uri.size() > 0 && *(uri.end() - 1) != '/')
			out << "/";
		out << name << "\">" << name << "</a></li>";
	}

	out << "</ul></body></html>";
	closedir(dir);
	return out.str();
}

int HttpRequestHandler::getHtmlPage()
{
	std::cout << "\033[95m[" << getCurrentTime() << "] "
			  << "🔍 getHtmlPage() called - URI: " << this->uri_ << "\033[0m" << std::endl;
	std::string base = this->root;
	std::string uri = this->uri_;

	if (uri.empty())
		uri = "/";
	if (!uri.empty() && uri[0] != '/')
		uri = "/" + uri;
	Location *matchedLoc = NULL;
	std::string effectiveIndex = this->index;
	bool indexWasExplicitlySet = false;
	if (serverConfig_)
	{
		ServerConf *nonConst = const_cast<ServerConf *>(serverConfig_);
		matchedLoc = nonConst->findLocation(uri);
		std::cout << "\033[96m[" << getCurrentTime() << "] "
				  << "📍 Location search for URI: " << uri 
				  << " -> Found: " << (matchedLoc ? matchedLoc->path : "NULL") << "\033[0m" << std::endl;
		if (uri == "/favicon.ico")
		{
			std::cout << "\033[93m[" << getCurrentTime() << "] "
					  << "🍪 Special case: favicon.ico request" << "\033[0m" << std::endl;
			if (matchedLoc)
			{
				if (!matchedLoc->root.empty())
					base = matchedLoc->root;
				if (!matchedLoc->index.empty())
				{
					effectiveIndex = matchedLoc->index;
					indexWasExplicitlySet = true;
				}
				else
				{
					effectiveIndex = "";
					indexWasExplicitlySet = true;
				}
			}
		}
		if (matchedLoc)
		{
			if (!matchedLoc->root.empty())
				base = matchedLoc->root;
			if (!matchedLoc->index.empty())
			{
				effectiveIndex = matchedLoc->index;
				indexWasExplicitlySet = true;
			}
			else
			{
				effectiveIndex = "";
				indexWasExplicitlySet = true;
			}
			if (!matchedLoc->errorPages_.empty())
				this->errorPages = matchedLoc->errorPages_;

			if (!matchedLoc->cgi_pass.empty())
			{
				for (size_t i = 0; i < matchedLoc->cgi_pass.size(); ++i)
				{
					const std::string& extension = matchedLoc->cgi_pass[i].first;
					const std::string& interpreter = matchedLoc->cgi_pass[i].second;

					if (uri_.find(extension) != std::string::npos)
					{
				HandleCGI cgiHandler(interpreter, extension, env_);
				cgiHandler.setRoot(base);
				cgiHandler.setHeaders(this->headers_);
				cgiHandler.setClientIp(this->clientIp_);
				
				cgiInfo_ = cgiHandler.executeCgi(uri_, this->queryString_, "", this->method_);
				if (cgiInfo_.pid != -1)
					return 1;
				
				if (cgiInfo_.exitCode != 0)
				{
					handleError(cgiInfo_.exitCode);
					return 0;
				}
				return 1;
					}
				}
			}
			this->autoindex_ = matchedLoc->autoindex;
		}
		else
		{
			this->autoindex_ = false;
		}
	}
	std::string rel = uri;
	if (matchedLoc && uri.find(matchedLoc->path) == 0)
	{
		rel = uri.substr(matchedLoc->path.size());
		if (rel.empty())
			rel = "/";
		else if (rel[0] != '/')
			rel = "/" + rel;
	}
	else
	{
		if (uri == "/")
			rel = "/";
	}

	std::string path = base + rel;

	if (isDirectory(path))
	{
		if (!effectiveIndex.empty())
		{
			std::string indexPath = path;
			if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
				indexPath += '/';
			indexPath += effectiveIndex;
			std::ifstream f(indexPath.c_str());
			if (f)
			{
				respBody_ << f.rdbuf();
				return 1;
			}
		}

		if (autoindex_)
		{
			respBody_ << generateAutoindex(path, uri);
			return 1;
		}
		else
		{
			handleError(404);
			return 0;
		}
	}
	std::ifstream file(path.c_str());
	if (!file)
	{
		handleError(404);
		return 0;
	}
	respBody_ << file.rdbuf();
	return 1;
}

std::string HttpRequestHandler::handleRedirect()
{

	for (std::map<int, std::string>::iterator it = redirects.begin(); it != redirects.end(); ++it)
	{
		int code = it->first;
		std::string target = it->second;
		std::cout << "\033[94m" << "[" << getCurrentTime() << "] " << "Applying redirect " << code << " to " << target << "\033[0m" << std::endl;

		std::ostringstream redirect;
		redirect << "HTTP/1.1 " << code;
		if (code == 301)
			redirect << " Moved Permanently";
		else if (code == 302)
			redirect << " Found";
		else
			redirect << " Redirect";

		redirect << "\r\n"
				 << "Date: " << getCurrentTime() << "\r\n"
				 << "Server: WebServ\r\n"
				 << "Location: " << target << "\r\n"
				 << "Content-Length: 0\r\n"
				 << "Connection: close\r\n\r\n";

		return redirect.str();
	}
	return "";
}

bool HttpRequestHandler::parseHeader(const std::string &request)
{
	this->resp_.clear();
	this->resp_.str("");
	if (!isValidMethod(request))
	{
		std::cout << "\033[91m" << "[" << getCurrentTime() << "] " << "Invalid Method Detected" << "\033[0m" << std::endl;
		getUri(request);
		handleError(405);
		std::string body405 = this->respBody_.str();
		std::string allowHeader = getAllowedMethodsHeader(uri_);
		this->resp_ << "HTTP/1.1 405 Method Not Allowed\r\n"
					<< "Date: " << getCurrentTime() << "\r\n"
					<< "Server: WebServ\r\n"
					<< allowHeader
					<< "Content-Length: " << body405.size() << "\r\n"
					<< "Content-Type: text/html\r\n"
					<< "Connection: close\r\n\r\n"
					<< body405;
		return false;
	}

	// HTTP/1.1 -> Host obligatoire
	bool isHttp11 = (request.find("HTTP/1.1") != std::string::npos);
	bool hasHost = (request.find("\r\nHost:") != std::string::npos) || (request.find("\nHost:") != std::string::npos) || (request.find("Host:") == 0);
	if (isHttp11 && !hasHost)
	{
		std::cout << "\033[91m" << "[" << getCurrentTime() << "] " << "Missing Host header for HTTP/1.1" << "\033[0m" << std::endl;
		handleError(400);
		std::string body400 = this->respBody_.str();
		this->resp_ << "HTTP/1.1 400 Bad Request\r\n"
					<< "Date: " << getCurrentTime() << "\r\n"
					<< "Server: WebServ\r\n"
					<< "Content-Length: " << body400.size() << "\r\n"
					<< "Content-Type: text/html\r\n"
					<< "Connection: close\r\n\r\n"
					<< body400;
		return false;
	}
	return true;
}

std::string HttpRequestHandler::parseRequest(const std::string &request)
{
    std::cout << "\033[36m" << "[" << getCurrentTime() << "] " 
              << "Parsing request..." << "\033[0m" << std::endl;

    currentRequest_ = request;
    is403Forbidden_ = false;
    
    if (isEmpty(request)) {
        std::ostringstream oss;
        oss << "HTTP/1.1 400 Bad Request\r\n"
            << "Date: " << getCurrentTime() << "\r\n"
            << "Server: WebServ\r\n"
            << "Content-Length: 0\r\n"
            << "Content-Type: text/html\r\n"
            << "Connection: close\r\n\r\n";
        return oss.str();
    }
    
    if (!parseHeader(request))
        return this->resp_.str();
    
    parseHeaders(request);
    extractBody(request);
    getUri(request);
    
    if (!redirects.empty())
        return handleRedirect();

    if (!isMethodAllowed(method_, uri_)) {
        handleError(405);
        std::string body405 = this->respBody_.str();
        std::string allowHeader = getAllowedMethodsHeader(uri_);
        std::ostringstream resp;
        resp << "HTTP/1.1 405 Method Not Allowed\r\n"
             << "Date: " << getCurrentTime() << "\r\n"
             << "Server: WebServ\r\n"
             << allowHeader
             << "Content-Length: " << body405.size() << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Connection: close\r\n\r\n"
             << body405;
        return resp.str();
    }

    bool hasContentLength = (request.find("Content-Length:") != std::string::npos) 
                         || (request.find("content-length:") != std::string::npos);
    bool isChunked = isTransferEncodingChunked(request);
    
    if (method_ == "POST" && !hasContentLength && !isChunked && isEmpty(this->body_))
    {
        std::ostringstream oss;
        oss << "HTTP/1.1 400 Bad Request\r\n"
            << "Date: " << getCurrentTime() << "\r\n"
            << "Server: WebServ\r\n"
            << "Content-Length: 0\r\n"
            << "Content-Type: text/html\r\n"
            << "Connection: close\r\n\r\n";
        return oss.str();
    }

    this->respBody_.clear();
    this->respBody_.str("");

    bool found = true;
    if (method_ == "GET")
        found = (getHtmlPage() != 0);
    else if (method_ == "DELETE")
        found = handleDeleteRequest();
    else if (method_ == "POST")
        found = handlePostRequest();
    else if (method_ == "PUT")
        found = handlePutRequest();
    
    if (cgiInfo_.pid != -1)
        return "CGI_PENDING";

    if (is403Forbidden_) {
        std::string body403 = this->respBody_.str();
        this->resp_ << "HTTP/1.1 403 Forbidden\r\n"
                    << "Date: " << getCurrentTime() << "\r\n"
                    << "Server: WebServ\r\n"
                    << "Content-Length: " << body403.size() << "\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n\r\n"
                    << body403;
        return this->resp_.str();
    }

	if (error_code_ != 0) {
		std::string bodyError = this->respBody_.str();
		this->resp_ << "HTTP/1.1 " << error_code_ << "\r\n"
					<< "Date: " << getCurrentTime() << "\r\n"
					<< "Server: WebServ\r\n"
					<< "Content-Length: " << bodyError.size() << "\r\n"
					<< "Content-Type: text/html\r\n"
					<< "Connection: close\r\n\r\n"
					<< bodyError;
		return this->resp_.str();
    }
    
    std::string body = this->respBody_.str();
    if (!found) {
        this->resp_ << "HTTP/1.1 404 Not Found\r\n"
                    << "Date: " << getCurrentTime() << "\r\n"
                    << "Server: WebServ\r\n"
                    << "Content-Length: " << body.size() << "\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n\r\n"
                    << body;
        return this->resp_.str();
    }

    bool isNewVisitor = (request.find("Cookie:") == std::string::npos || 
                        request.find("visited=") == std::string::npos);
    std::string visitor = isNewVisitor ? "Bienvenue nouveau visiteur!" : "bon retour, visiteur!";
    if (isNewVisitor) visit_count_++;
    
    resp_ << "HTTP/1.1 200 OK\r\n"
          << "Date: " << getCurrentTime() << "\r\n"
          << "Server: WebServ\r\n"
          << "Content-Length: " << body.size() << "\r\n"
          << "Content-Type: text/html\r\n"
          << "Set-Cookie: visited=" << visitor << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/\r\n"
          << "Set-Cookie: visit_count=" << visit_count_ << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/\r\n"
          << "Connection: close\r\n\r\n"
          << body;
    return resp_.str();
}

bool HttpRequestHandler::isMethodAllowed(const std::string &method, const std::string &uri)
{
	if (!serverConfig_)
	{
		return true;
	}

	ServerConf *nonConstConfig = const_cast<ServerConf *>(serverConfig_);
	Location *location = nonConstConfig->findLocation(uri);

	if (!location)
	{
		return true;
	}

	const std::vector<std::string> &allowedMethods = location->allowed_methods;
	for (size_t i = 0; i < allowedMethods.size(); i++)
	{
		if (allowedMethods[i] == method)
		{
			return true;
		}
	}

	return false;
}

bool HttpRequestHandler::handleDeleteRequest()
{
	std::string base = this->root;
	std::string uri = this->uri_;

	if (uri.empty())
		uri = "/";
	if (!uri.empty() && uri[0] != '/')
		uri = "/" + uri;

	if (serverConfig_)
	{
		ServerConf *nonConst = const_cast<ServerConf *>(serverConfig_);
		Location *loc = nonConst->findLocation(uri);
		if (loc && uri.find(loc->path) == 0)
		{
			std::string rel = uri.substr(loc->path.size());
			if (rel.empty() || rel == "/")
				rel = "/";
			else if (rel[0] != '/')
				rel = "/" + rel;
			uri = rel;
		}
	}

	std::string filePath = base + uri;

	std::cout << "\033[96m" << "[" << getCurrentTime() << "] " << "DELETE request - Root: " << base << ", URI: " << this->uri_ << ", FilePath: " << filePath << "\033[0m" << std::endl;

	std::ifstream file(filePath.c_str());
	if (!file)
	{
		std::cout << "\033[93m" << "[" << getCurrentTime() << "] " << "File not found for DELETE: " << filePath << "\033[0m" << std::endl;
		handleError(404);
		return false;
	}
	file.close();

	if (remove(filePath.c_str()) == 0)
	{
		std::cout << "\033[92m" << "[" << getCurrentTime() << "] " << "File deleted successfully: " << filePath << "\033[0m" << std::endl;
		respBody_.clear();
		respBody_.str("");
		return true;
	}
	else
	{
		std::cout << "\033[91m" << "[" << getCurrentTime() << "] " << "Failed to delete file: " << filePath << "\033[0m" << std::endl;
		handleError(500);
		return false;
	}
}

bool HttpRequestHandler::handlePostRequest()
{
    std::string base = this->root;
    std::string uri = this->uri_;
    
    if (uri.empty())
        uri = "/";
    if (!uri.empty() && uri[0] != '/')
        uri = "/" + uri;

    Location *loc = NULL;
	size_t maxBodySize = 0;
    if (serverConfig_)
    {
        ServerConf *nonConst = const_cast<ServerConf *>(serverConfig_);
        loc = nonConst->findLocation(uri);
        if (!loc)
        {
            std::cout << "\033[93m" << "[" << getCurrentTime() << "] " 
                      << "POST to non-existent location: " << uri << "\033[0m" << std::endl;
            handleError(404);
            return false;
        }
		if (loc->client_max_body_size > 0)
			maxBodySize = loc->client_max_body_size;
		else
			maxBodySize = serverConfig_->getClientMaxBodySize();
		if (maxBodySize > 0 && body_.size() > maxBodySize)
		{
			std::cout << "\033[91m[" << getCurrentTime() << "] "
                      << "❌ 413 Payload Too Large: body size " << body_.size() 
                      << " exceeds limit " << maxBodySize << " bytes" 
                      << "\033[0m" << std::endl;
            handleError(413);
            return false;
		}
        if (!loc->cgi_pass.empty())
		{

			for (size_t i = 0; i < loc->cgi_pass.size(); ++i)
			{
				const std::string& extension = loc->cgi_pass[i].first;
				const std::string& interpreter = loc->cgi_pass[i].second;

				if (uri_.find(extension) != std::string::npos)
				{
				HandleCGI cgiHandler(interpreter, extension, env_);
				cgiHandler.setRoot(base);
				cgiHandler.setHeaders(this->headers_);
				cgiHandler.setClientIp(this->clientIp_);
				
				cgiInfo_ = cgiHandler.executeCgi(uri_, this->queryString_, this->body_, this->method_);
				if (cgiInfo_.pid != -1)
					return true;
				
				if (cgiInfo_.exitCode != 0)
				{
					handleError(cgiInfo_.exitCode);
					return false;
				}
				return true;
				}
			}
		}
        if (!loc->upload_store.empty())
            base = loc->upload_store;
        else if (!loc->root.empty())
            base = loc->root;
        
        std::cout << "\033[96m[" << getCurrentTime() << "] "
                  << "📂 Upload directory: " << base 
                  << (loc->upload_store.empty() ? " (from root)" : " (from upload_store)")
                  << "\033[0m" << std::endl;
    }
    
    std::cout << "\033[94m" << "[" << getCurrentTime() << "] " 
              << "POST request - URI: " << uri_ 
              << ", Body size: " << body_.size() 
              << ", Root: " << base << "\033[0m" << std::endl;
    
    if (body_.empty())
    {
        respBody_ << "<html><body><h1>Upload successful</h1>"
                  << "<p>Empty POST received (Content-Length: 0)</p></body></html>";
        return true;
    }
    std::string contentType;
    std::string boundary;
    if (body_.size() > 2 && body_[0] == '-' && body_[1] == '-')
    {
        size_t boundaryEnd = body_.find("\r\n");
        if (boundaryEnd != std::string::npos)
        {
            boundary = body_.substr(2, boundaryEnd - 2);
            
            std::string fileContent;
            std::string filename;
            
            if (parseMultipartBody(body_, boundary, fileContent, filename))
            {
                std::cout << "\033[96m" << "[" << getCurrentTime() << "] " 
                          << "Parsed multipart file: " << filename 
                          << " (" << fileContent.size() << " bytes)" << "\033[0m" << std::endl;
                std::string uploadFilename;
                if (!filename.empty())
                    uploadFilename = filename;
                else
                {
                    std::ostringstream oss;
                    oss << "upload_" << time(NULL) << "_" << (rand() % 10000);
                    uploadFilename = oss.str();
                }
                
                std::string uploadDir = base;
                if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
                    uploadDir += '/';
                
                std::string filepath = uploadDir + uploadFilename;
                
                std::ofstream outfile(filepath.c_str(), std::ios::binary);
                if (!outfile)
                {
                    std::cout << "\033[91m" << "[" << getCurrentTime() << "] " 
                              << "❌ Failed to create file: " << filepath 
                              << " (errno: " << strerror(errno) << ")" << "\033[0m" << std::endl;
                    handleError(500);
                    return false;
                }
                
                outfile.write(fileContent.c_str(), fileContent.size());
                outfile.close();
                
                std::cout << "\033[92m" << "[" << getCurrentTime() << "] " 
                          << "✅ File saved: " << filepath 
                          << " (" << fileContent.size() << " bytes)" << "\033[0m" << std::endl;
                
                respBody_ << "<html><head><title>Upload Success</title></head><body>"
                          << "<h1>Upload Successful</h1>"
                          << "<p><strong>File:</strong> " << uploadFilename << "</p>"
                          << "<p><strong>Size:</strong> " << fileContent.size() << " bytes</p>"
                          << "<p><strong>Path:</strong> " << filepath << "</p>"
                          << "<a href=\"/uploads\">View uploads directory</a>"
                          << "</body></html>";
                
                return true;
            }
        }
    }
	std::ostringstream filename;
    filename << "upload_" << time(NULL) << "_" << (rand() % 10000);

    std::string uploadDir = base;
    if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
        uploadDir += '/';
    
    std::string filepath = uploadDir + filename.str();

    std::ofstream outfile(filepath.c_str(), std::ios::binary);
    if (!outfile)
    {
        std::cout << "\033[91m" << "[" << getCurrentTime() << "] " 
                  << "❌ Failed to create file: " << filepath 
                  << " (errno: " << strerror(errno) << ")" << "\033[0m" << std::endl;
        handleError(500);
        return false;
    }

    outfile.write(body_.c_str(), body_.size());
    outfile.close();
    
    std::cout << "\033[92m" << "[" << getCurrentTime() << "] " 
              << "✅ File saved: " << filepath 
              << " (" << body_.size() << " bytes)" << "\033[0m" << std::endl;

    respBody_ << "<html><head><title>Upload Success</title></head><body>"
              << "<h1>Upload Successful</h1>"
              << "<p><strong>File:</strong> " << filename.str() << "</p>"
              << "<p><strong>Size:</strong> " << body_.size() << " bytes</p>"
              << "<p><strong>Path:</strong> " << filepath << "</p>"
              << "<a href=\"/uploads\">View uploads directory</a>"
              << "</body></html>";
    
    return true;
}

bool HttpRequestHandler::handlePutRequest()
{
	std::string base = this->root;
	std::string uri = this->uri_;
	
	if (uri.empty())
		uri = "/";
	if (!uri.empty() && uri[0] != '/')
		uri = "/" + uri;

	Location *loc = NULL;
	size_t maxBodySize = 0;
	if (serverConfig_)
	{
		ServerConf *nonConst = const_cast<ServerConf *>(serverConfig_);
        loc = nonConst->findLocation(uri);
        if (!loc)
        {
            std::cout << "\033[93m" << "[" << getCurrentTime() << "] " 
                      << "PUT to non-existent location: " << uri << "\033[0m" << std::endl;
            handleError(404);
            return false;
        }
        if (loc->client_max_body_size > 0)
            maxBodySize = loc->client_max_body_size;
        else
            maxBodySize = serverConfig_->getClientMaxBodySize();
        
        if (maxBodySize > 0 && body_.size() > maxBodySize)
        {
            std::cout << "\033[91m[" << getCurrentTime() << "] "
                      << "❌ 413 Payload Too Large: body size " << body_.size() 
                      << " exceeds limit " << maxBodySize << " bytes" 
                      << "\033[0m" << std::endl;
            handleError(413);
            return false;
        }
        if (!loc->upload_store.empty())
            base = loc->upload_store;
        else if (!loc->root.empty())
            base = loc->root;
	}

	std::cout << "\033[94m" << "[" << getCurrentTime() << "] " 
			  << "PUT request - URI: " << uri_ 
			  << ", Body size: " << body_.size() 
			  << ", Root: " << base << "\033[0m" << std::endl;

	if (body_.empty())
	{
		respBody_ << "<html><body><h1>Upload successful</h1>"
				  << "<p>Empty PUT received (Content-Length: 0)</p></body></html>";
		return true;
	}

	std::ostringstream filename;
	filename << "upload_" << time(NULL) << "_" << (rand() % 10000);

	std::string uploadDir = base;
	if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += '/';
	
	std::string filepath = uploadDir + filename.str();

	std::ofstream outfile(filepath.c_str(), std::ios::binary);
	if (!outfile)
	{
		std::cout << "\033[91m" << "[" << getCurrentTime() << "] " 
				  << "❌ Failed to create file: " << filepath 
				  << " (errno: " << strerror(errno) << ")" << "\033[0m" << std::endl;
		handleError(500);
		return false;
	}

	outfile.write(body_.c_str(), body_.size());
	outfile.close();
	
	std::cout << "\033[92m" << "[" << getCurrentTime() << "] " 
			  << "✅ File saved via PUT: " << filepath 
			  << " (" << body_.size() << " bytes)" << "\033[0m" << std::endl;

	respBody_ << "<html><head><title>Upload Success</title></head><body>"
			  << "<h1>Upload Successful (PUT)</h1>"
			  << "<p><strong>File:</strong> " << filename.str() << "</p>"
			  << "<p><strong>Size:</strong> " << body_.size() << " bytes</p>"
			  << "<p><strong>Path:</strong> " << filepath << "</p>"
			  << "</body></html>";
	
	return true;
}

int HttpRequestHandler::hexStringToInt(const std::string &hexStr) const {
    int result = 0;
    for (size_t i = 0; i < hexStr.length(); i++) {
        char c = hexStr[i];
        if (c >= '0' && c <= '9')
            result = result * 16 + (c - '0');
        else if (c >= 'a' && c <= 'f')
            result = result * 16 + (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            result = result * 16 + (c - 'A' + 10);
        else
            break;
    }
    return result;
}

bool HttpRequestHandler::isTransferEncodingChunked(const std::string &request) const {
    size_t pos = request.find("Transfer-Encoding:");
    if (pos == std::string::npos)
        pos = request.find("transfer-encoding:");
    
    if (pos == std::string::npos)
        return false;

    size_t valueStart = pos + 18;
    while (valueStart < request.size() && std::isspace(request[valueStart]))
        valueStart++;
    
    size_t valueEnd = request.find("\r\n", valueStart);
    if (valueEnd == std::string::npos)
        valueEnd = request.size();
    
    std::string value = request.substr(valueStart, valueEnd - valueStart);

    size_t first = value.find_first_not_of(" \t\r\n");
    size_t last = value.find_last_not_of(" \t\r\n");
    if (first != std::string::npos)
        value = value.substr(first, last - first + 1);
    
    return (value == "chunked");
}

bool HttpRequestHandler::extractChunkedBody(const std::string &request, 
                                            std::string &outBody, 
                                            size_t &outSize) const {
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;
    
    size_t pos = headerEnd + 4;
    outBody.clear();
    outSize = 0;
    
    while (pos < request.size()) {
        size_t lineEnd = request.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            return false;
        
        std::string sizeLine = request.substr(pos, lineEnd - pos);
        
        size_t semiColon = sizeLine.find(';');
        if (semiColon != std::string::npos)
            sizeLine = sizeLine.substr(0, semiColon);

        size_t first = sizeLine.find_first_not_of(" \t\r\n");
        size_t last = sizeLine.find_last_not_of(" \t\r\n");
        if (first != std::string::npos)
            sizeLine = sizeLine.substr(first, last - first + 1);
        
        int chunkSize = hexStringToInt(sizeLine);
        pos = lineEnd + 2;
        if (chunkSize == 0) {
            if (pos + 2 <= request.size() && request.substr(pos, 2) == "\r\n")
                return true;
            return false;
        }

        if (pos + chunkSize + 2 > request.size())
            return false;
        
        outBody.append(request.substr(pos, chunkSize));
        outSize += chunkSize;
        pos += chunkSize + 2;
    }
    
    return false;
}

bool HttpRequestHandler::parseMultipartBody(const std::string &body, const std::string &boundary, std::string &fileContent, std::string &filename)
{
    std::string fullBoundary = "--" + boundary;

    size_t partStart = body.find(fullBoundary);
    if (partStart == std::string::npos)
        return false;
    
    partStart += fullBoundary.length();

    if (partStart + 2 > body.size() || body.substr(partStart, 2) != "\r\n")
        return false;
    partStart += 2;
    size_t headersEnd = body.find("\r\n\r\n", partStart);
    if (headersEnd == std::string::npos)
        return false;
    std::string headers = body.substr(partStart, headersEnd - partStart);

    size_t filenamePos = headers.find("filename=\"");
    if (filenamePos != std::string::npos) {
        filenamePos += 10;
        size_t filenameEnd = headers.find("\"", filenamePos);
        if (filenameEnd != std::string::npos) {
            filename = headers.substr(filenamePos, filenameEnd - filenamePos);
        }
    }

    size_t contentStart = headersEnd + 4;
    std::string endBoundary = "\r\n" + fullBoundary;
    size_t contentEnd = body.find(endBoundary, contentStart);
    
    if (contentEnd == std::string::npos)
        return false;
    fileContent = body.substr(contentStart, contentEnd - contentStart);
    
    return true;
}

std::string HttpRequestHandler::getAllowedMethodsHeader(const std::string &uri)
{
    if (!serverConfig_)
        return "";

    ServerConf *nonConstConfig = const_cast<ServerConf *>(serverConfig_);
    Location *location = nonConstConfig->findLocation(uri);

    if (!location || location->allowed_methods.empty())
        return "";

    std::string allowHeader = "Allow: ";
    for (size_t i = 0; i < location->allowed_methods.size(); i++)
    {
        if (i > 0)
            allowHeader += ", ";
        allowHeader += location->allowed_methods[i];
    }
    allowHeader += "\r\n";
    
    return allowHeader;
}
