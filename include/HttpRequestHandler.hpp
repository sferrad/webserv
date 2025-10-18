#ifndef HTTPREQUESTHANDLER_HPP
#define HTTPREQUESTHANDLER_HPP

#include "webserv.h"

class ServerConf;

class HttpRequestHandler {
private:
	std::string method_;
	std::string body_;
	std::string uri_;
	
	
	std::ostringstream respBody_;
	
	
	void extractBody(const std::string &request);
	bool isValidMethod(const std::string &request);
	int getHtmlPage();
	void getUri(std::string &request);
public:
	std::string root;
	std::string index;
	std::map<int, std::string> errorPages;
    std::string parseRequest(const std::string &request);
	void handleError(int code);
};

#endif // HTTPREQUESTHANDLER_HPP
