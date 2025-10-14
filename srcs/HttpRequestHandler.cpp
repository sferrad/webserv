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
    this->method = method;
	std::cout << "\033[31m" << "Method: " << method << "\033[0m" << std::endl;
    return true;
}

void HttpRequestHandler::GetUri(std::string &request){
	size_t pos = request.find("GET ");
	if (pos != std::string::npos) {
		size_t endPos = request.find(" ", pos + 4);
		if (endPos != std::string::npos) {
			this->uri = request.substr(pos + 4, endPos - (pos + 4));
		}
	}
	
}

void HttpRequestHandler::RecupBody(const std::string &request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos != std::string::npos) {
        this->body = request.substr(pos + 4);
    }
}

int HttpRequestHandler::getHtmlPage() {
	std::string base = this->root;
	std::string uri  = this->uri;

	std::cout << "Root: " << base << ", URI: " << uri << std::endl;

	if (uri.empty() || uri == "/")
		uri = "/" + this->index;
	else if (uri[0] != '/')
		uri = "/" + uri;

	std::string path = base + uri;

	std::ifstream file(path.c_str());
	if (!file) {
		std::map<int, std::string>::const_iterator it = this->error_page.find(404);
		if (it == this->error_page.end())
			return 0;
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
			return 0;
		resp_body << ferr.rdbuf();
		return 0;
	}
	resp_body << file.rdbuf();
	return 1;
}


std::string HttpRequestHandler::parse_request(const std::string &request) {
    std::cout << "Parsing request: " << request << std::endl;
	if (isEmpty(request))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    if (!isValidMethod(request))
        return "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
    RecupBody(request);
	if (HttpRequestHandler::method == "POST" && isEmpty(this->body))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

	this->resp_body.str("");
	this->resp_body.clear();

	std::ostringstream resp;
	GetUri(const_cast<std::string&>(request));
	bool found = true;
    if (method == "GET") {
		found = (getHtmlPage() != 0);
	}
	std::string body = this->resp_body.str();
	if (!found) {
		resp << "HTTP/1.1 404 Not Found\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
		return resp.str();
	}
    resp << "HTTP/1.1 200 OK\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
    return resp.str();
}