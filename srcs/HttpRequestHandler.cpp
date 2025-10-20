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

    if (method != "GET" && method != "POST" && method != "DELETE") {
        return false;
    }
    this->method_ = method;
	std::cout << "\033[31m" << "Method: " << method << "\033[0m" << std::endl;
    return true;
}

void HttpRequestHandler::getUri(const std::string &request){
	size_t pos = request.find("GET ");
	if (pos != std::string::npos) {
		size_t endPos = request.find(" ", pos + 4);
		if (endPos != std::string::npos) {
			this->uri_ = request.substr(pos + 4, endPos - (pos + 4));
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

bool HttpRequestHandler::parseHeader(const std::string &request) {
	// this->resp_.clear();
	this->resp_.clear();
    this->resp_.str("");
	if (!isValidMethod(request))
	{
		std::cout << "Invalid Method Detected" << std::endl;
		handleError(405);
		std::string body405 = this->respBody_.str();
		this->resp_ << "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: " << body405.size() << "\r\n\r\n" << body405;
		return false;
	}

	// HTTP/1.1 -> Host obligatoire
	bool isHttp11 = (request.find("HTTP/1.1") != std::string::npos);
	bool hasHost = (request.find("\r\nHost:") != std::string::npos)
		|| (request.find("\nHost:") != std::string::npos)
		|| (request.find("Host:") == 0);
	if (isHttp11 && !hasHost)
	{
		std::cout << "Missing Host header for HTTP/1.1" << std::endl;
		handleError(400);
		std::string body400 = this->respBody_.str();
		this->resp_ << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << body400.size() << "\r\n\r\n" << body400;
		return false;
	}
	return true;
}

std::string HttpRequestHandler::parseRequest(const std::string &request) {
	std::cout << "Parsing request: \n\n" << request << std::endl;
	if (isEmpty(request))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
	if (!parseHeader(request))
		return this->resp_.str();
    extractBody(request);
	if (HttpRequestHandler::method_ == "POST" && isEmpty(this->body_))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

    this->respBody_.clear();
    this->respBody_.str("");
	
	getUri(request);
	bool found = true;
    if (method_ == "GET") {
		found = (getHtmlPage() != 0);
	}
	std::string body = this->respBody_.str();
	if (!found) {
		this->resp_ << "HTTP/1.1 404 Not Found\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
		return this->resp_.str();
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
    this->resp_ << "HTTP/1.1 200 OK\r\nContent-Length: " << body.size() << "\r\nSet-Cookie: visited=" << visitor << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/" << "\r\nSet-Cookie: visit_count=" << visit_count_ << "; Expires=Wed, 23 Oct 2025 07:28:00 GMT; Path=/" << "\r\n\r\n" << body;
    return this->resp_.str();
}