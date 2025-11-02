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

void HttpRequestHandler::getUri(std::string &request){
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


std::string HttpRequestHandler::parseRequest(const std::string &request) {
	std::cout << "Parsing request: " << request << std::endl;
	if (isEmpty(request))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    if (!isValidMethod(request))
	{
		handleError(405);
		std::string body405 = this->respBody_.str();
		std::ostringstream resp;
		resp << "HTTP/1.1 405 Forbidden\r\nContent-Length: " << body405.size() << "\r\n\r\n" << body405;
        return resp.str();
	}
    extractBody(request);
	if (HttpRequestHandler::method_ == "POST" && isEmpty(this->body_))
		return "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

	this->respBody_.str("");
	this->respBody_.clear();
	
	getUri(const_cast<std::string&>(request));
	std::ostringstream resp;
	bool found = true;
    if (method_ == "GET") {
		found = (getHtmlPage() != 0);
	}
	std::string body = this->respBody_.str();
	if (!found) {
		resp << "HTTP/1.1 404 Not Found\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
		return resp.str();
	}
    resp << "HTTP/1.1 200 OK\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
    return resp.str();
}

//////////MODIF BILAL//////////
void HttpRequestHandler::ParseHeaders(const std::string &request) {
	this->headers.clear();
	this->content_length = 0;
	this->content_type = "";

	size_t header_end = request.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		return ;
	}

	std::string header_section = request.substr(0, header_end);

	std::istringstream stream(header_section);
	std::string line;

	std::getline(stream, line);

	while (std::getline(stream, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r') {
			line.erase(line.size() - 1);
		}

		size_t colon = line.find(':');
		if (colon == std::string::npos) {
			continue;
		}

		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		size_t start = value.find_first_not_of(" \t");
		if (start != std::string::npos) {
			value = value.substr(start);
		}

		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		this->headers[key] = value;
		std::cout << "Header trouvé: " << key << " = " << value << std::endl;
	}

	this->content_length = GetContentlength();
	this->content_type = GetContentType();
}

size_t HttpRequestHandler::GetContentlength() {
	std::map<std::string, std::string>::iterator it = this->headers.find("content-length");

	if (it == this->headers.end()) {
		return (0);
	}

	std::string value = it->second;
	size_t length = 0;

	std::istringstream iss(value);
	iss >> length;

	std::cout << "Content-Length détecté: " << length << " bytes" << std::endl;
	return length;
}

std::string HttpRequestHandler::GetContentType() {
	std::map<std::string, std::string>::iterator it = this->headers.find("content-type");

	if (it == this->headers.end()) {
		return ("");
	}

	std::cout << "Content-Type détecté: " << it->second << std::endl;
	return (it->second);
}

bool HttpRequestHandler::isBodyComplete(const std::string &request) {
	size_t header_end = request.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		std::cout << "Headers incompletes" << std::endl;
		return (false);
	}

	size_t body_start = header_end + 4;
	size_t body_received = 0;

	if (body_start < request.size()) {
		body_received = request.size() - body_start;
	}

	std::cout << "Body reçu: " << body_received << " / " << this->content_length << " bytes" << std::endl;
	if (this->content_length == 0) {
		return (true);
	}

	return (body_received >= this->content_length);
}
//////////MODIF BILAL//////////