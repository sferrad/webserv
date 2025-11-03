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

	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		return false;
	}
	this->method_ = method;
	std::cout << "\033[31m" << "[" << getCurrentTime() << "]" << " Method: " << method << "\033[0m" << std::endl;
	return true;
}

void HttpRequestHandler::getUri(const std::string &request)
{
	size_t pos = request.find(method_ + " ");
	if (pos != std::string::npos)
	{
		size_t endPos = request.find(" ", pos + method_.length() + 1);
		if (endPos != std::string::npos)
		{
			this->uri_ = request.substr(pos + method_.length() + 1, endPos - (pos + method_.length() + 1));
		}
	}
}

void HttpRequestHandler::extractBody(const std::string &request)
{
	size_t pos = request.find("\r\n\r\n");
	if (pos != std::string::npos)
	{
		this->body_ = request.substr(pos + 4);
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
	std::string base = this->root;
	std::string uri = this->uri_;

	if (uri.empty())
		uri = "/";
	if (!uri.empty() && uri[0] != '/')
		uri = "/" + uri;
	Location *matchedLoc = NULL;
	std::string effectiveIndex = this->index;
		if (serverConfig_)
		{
			ServerConf *nonConst = const_cast<ServerConf *>(serverConfig_);
			matchedLoc = nonConst->findLocation(uri);
			if (matchedLoc)
			{
				if (!matchedLoc->root.empty())
					base = matchedLoc->root;
				if (!matchedLoc->index.empty())
					effectiveIndex = matchedLoc->index;
				this->autoindex_ = matchedLoc->autoindex;
			}
			else
			{
				this->autoindex_ = false;
			}
		}	std::string rel = uri;
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
		else if (autoindex_)
		{
			respBody_ << generateAutoindex(path, uri);
			return 1;
		}
		else
		{
			handleError(403);
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

bool HttpRequestHandler::parseHeader(const std::string &request)
{
	this->resp_.clear();
	this->resp_.str("");
	if (!isValidMethod(request))
	{
		std::cout << "Invalid Method Detected" << std::endl;
		handleError(405);
		std::string body405 = this->respBody_.str();
		this->resp_ << "HTTP/1.1 405 Method Not Allowed\r\n" << "Date: " << getCurrentTime() << "\r\nServer: WebServ\r\nContent-Length: " << body405.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
					<< body405;
		return false;
	}

	// HTTP/1.1 -> Host obligatoire
	bool isHttp11 = (request.find("HTTP/1.1") != std::string::npos);
	bool hasHost = (request.find("\r\nHost:") != std::string::npos) || (request.find("\nHost:") != std::string::npos) || (request.find("Host:") == 0);
	if (isHttp11 && !hasHost)
	{
		std::cout << "Missing Host header for HTTP/1.1" << std::endl;
		handleError(400);
		std::string body400 = this->respBody_.str();
		this->resp_ << "HTTP/1.1 400 Bad Request\r\n" << "Date: " << getCurrentTime() << "\r\nServer: WebServ\r\nContent-Length: " << body400.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
					<< body400;
		return false;
	}
	return true;
}

std::string HttpRequestHandler::parseRequest(const std::string &request)
{
	is403Forbidden_ = false;
	std::cout << "Parsing request: \n\n"
			  << request << std::endl;
	if (isEmpty(request))
	{
		std::ostringstream oss;
		oss << "HTTP/1.1 400 Bad Request\r\n" << "Date: " << getCurrentTime() << "\r\nServer: WebServ\r\nContent-Length: 0\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
		return oss.str();
	}
	if (!parseHeader(request))
		return this->resp_.str();
	extractBody(request);
	getUri(request);

	std::cout << "\033[33m" << "[" << getCurrentTime() << "]" << " Checking if method " << method_ << " is allowed for URI: " << uri_ << "\033[0m" << std::endl;
	if (!isMethodAllowed(method_, uri_))
	{
		std::cout << "\033[91m" << "[" << getCurrentTime() << "]" << " Method " << method_ << " NOT ALLOWED for URI: " << uri_ << "\033[0m" << std::endl;
		handleError(405);
		std::string body405 = this->respBody_.str();
		std::ostringstream resp;
		resp << "HTTP/1.1 405 Method Not Allowed\r\n" << "Date: " << getCurrentTime() << "\r\nServer: WebServ\r\nContent-Length: " << body405.size() << "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
			 << body405;
		return resp.str();
	}
	std::cout << "\033[92m" << "[" << getCurrentTime() << "]" << " Method " << method_ << " ALLOWED for URI: " << uri_ << "\033[0m" << std::endl;

	if (HttpRequestHandler::method_ == "POST" && isEmpty(this->body_))
	{
		std::ostringstream oss;
		oss << "HTTP/1.1 400 Bad Request\r\n" << "Date: " << getCurrentTime() << "\r\nServer: WebServ\r\nContent-Length: 0\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
		return oss.str();
	}

	this->respBody_.clear();
	this->respBody_.str("");

	getUri(request);
	bool found = true;
	if (method_ == "GET")
	{
		found = (getHtmlPage() != 0);
	}
	else if (method_ == "DELETE")
	{
		found = handleDeleteRequest();
	}
	else if (method_ == "POST")
	{
		found = handlePostRequest();
	}
	else if (method_ == "PUT")
	{
		found = handlePutRequest();
	}
	std::string body = this->respBody_.str();
	if (is403Forbidden_)
	{
		this->resp_ << "HTTP/1.1 403 Forbidden\r\n"
					<< "Date: " << getCurrentTime() << "\r\n"
					<< "Server: WebServ" << "\r\nContent-Length: " << body.size() << "\r\nContent-Type: text/html\r\n"
					<< "Connection: close" << "\r\n\r\n"
					<< body;
		return this->resp_.str();
	}
	if (!found)
	{
		this->resp_ << "HTTP/1.1 404 Not Found\r\n"
					<< "Date: " << getCurrentTime() << "\r\n"
					<< "Server: WebServ" << "\r\nContent-Length: " << body.size() << "\r\nContent-Type: text/html\r\n"
					<< "Connection: close" << "\r\n\r\n"
					<< body;
		return this->resp_.str();
	}
	bool isNewVisitor = true;
	//---------------- test des cookies ----------------
	if (request.find("Cookie:") != std::string::npos && request.find("visited=") != std::string::npos)
	{
		isNewVisitor = false;
	}
	std::string visitor;
	if (isNewVisitor)
	{
		visitor = "Bienvenue nouveau visiteur!";
		visit_count_++;
	}
	else
		visitor = "bon retour, visiteur!";
	// ----------------------------------------------------
	resp_ << "HTTP/1.1 200 OK" << "\r\n" << "Date: " << getCurrentTime() << "\r\nServer: WebServ" << "\r\nContent-Length: " << body.size() << "\r\nContent-Type: text/html\r\nSet-Cookie: visited=" << visitor << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/\r\nSet-Cookie: visit_count=" << visit_count_ << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/\r\nConnection: close\r\n\r\n"
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

	std::cout << "\033[96m" << "DELETE request - Root: " << base << ", URI: " << this->uri_ << ", FilePath: " << filePath << "\033[0m" << std::endl;

	std::ifstream file(filePath.c_str());
	if (!file)
	{
		std::cout << "\033[93m" << "File not found for DELETE: " << filePath << "\033[0m" << std::endl;
		handleError(404);
		return false;
	}
	file.close();

	if (remove(filePath.c_str()) == 0)
	{
		std::cout << "\033[92m" << "File deleted successfully: " << filePath << "\033[0m" << std::endl;
		respBody_.clear();
		respBody_.str("");
		return true;
	}
	else
	{
		std::cout << "\033[91m" << "Failed to delete file: " << filePath << "\033[0m" << std::endl;
		handleError(500);
		return false;
	}
}

bool HttpRequestHandler::handlePostRequest()
{
	std::cout << "\033[94m" << "POST request handled" << "\033[0m" << std::endl;
	respBody_.clear();
	respBody_.str("");
	return true;
}

bool HttpRequestHandler::handlePutRequest()
{
	std::cout << "\033[95m" << "PUT request handled" << "\033[0m" << std::endl;
	respBody_.clear();
	respBody_.str("");
	return true;
}