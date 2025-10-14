#ifndef HTTPREQUESTHANDLER_HPP
#define HTTPREQUESTHANDLER_HPP

#include "webserv.h"

class ServerConf;

class HttpRequestHandler {
private:
	std::string method;
	std::string body;
	std::string uri;
	
	
	std::ostringstream resp_body;
	
	
	void RecupBody(const std::string &request);
	bool isValidMethod(const std::string &request);
	int getHtmlPage();
	void GetUri(std::string &request);
public:
	std::string root;
	std::string index;
    std::string parse_request(const std::string &request);
};

#endif // HTTPREQUESTHANDLER_HPP
