#include "../include/webserv.h"

bool HttpRequestHandler::isValidMethod(const std::string &request) {
    char tmp[1024];
    strncpy(tmp, request.c_str(), sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;  // sécurité
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
    return true;
}


void HttpRequestHandler::RecupBody(const std::string &request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos != std::string::npos) {
        this->body = request.substr(pos + 4);
    }
}



std::string HttpRequestHandler::parse_request(const std::string &request) {
    std::cout << "Parsing request: " << request << std::endl;
	if (isEmpty(request))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    if (!isValidMethod(request))
        return "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
    RecupBody(request);
    std::string body = "Hello, World! + " + this->method + " + " + this->body;
    std::ostringstream resp;
    resp << "HTTP/1.1 200 OK\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
    return resp.str();
}