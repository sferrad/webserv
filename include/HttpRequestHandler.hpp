#ifndef HTTPREQUESTHANDLER_HPP
#define HTTPREQUESTHANDLER_HPP

#include "webserv.h"

class HttpRequestHandler {
private:
	std::string method;
	std::string body;

	void RecupBody(const std::string &request);
	bool isValidMethod(const std::string &request);
public:
    std::string parse_request(const std::string &request);
};

#endif // HTTPREQUESTHANDLER_HPP
