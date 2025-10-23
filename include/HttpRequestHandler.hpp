#ifndef HTTPREQUESTHANDLER_HPP
#define HTTPREQUESTHANDLER_HPP

#include "webserv.h"

class ServerConf;

class HttpRequestHandler {
private:
	std::string method_;
	std::string body_;
	std::string uri_;
	long int visit_count_;
	std::ostringstream respBody_;
	std::ostringstream resp_;

	const ServerConf* serverConfig_;
	
	void extractBody(const std::string &request);
	bool isValidMethod(const std::string &request);
	bool isMethodAllowed(const std::string &method, const std::string &uri);
	int getHtmlPage();
	bool handleDeleteRequest();
	bool handlePostRequest();
	bool handlePutRequest();
	void getUri(const std::string &request);
public:
 
	HttpRequestHandler() : visit_count_(1), serverConfig_(NULL) {}
	HttpRequestHandler(const ServerConf* config) : visit_count_(1), serverConfig_(config) {}
	
	std::string root;
	std::string index;
	std::map<int, std::string> errorPages;
    std::string parseRequest(const std::string &request);
	void handleError(int code);
	bool parseHeader(const std::string &request);
};

#endif // HTTPREQUESTHANDLER_HPP
