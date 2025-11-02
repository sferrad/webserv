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
	//////////MODIF BILAL//////////
	std::map<std::string, std::string> headers;
	size_t content_length;
	std::string content_type;
	//////////MODIF BILAL//////////
	
	
	void extractBody(const std::string &request);
	bool isValidMethod(const std::string &request);
	int getHtmlPage();
	void getUri(std::string &request);

	//////////MODIF BILAL//////////
	void ParseHeaders(const std::string &request);
	size_t GetContentlength();
	std::string GetContentType();
	bool isBodyComplete(const std::string &request);
	//////////MODIF BILAL//////////

public:
	std::string root;
	std::string index;
	std::map<int, std::string> errorPages;
	
	HttpRequestHandler() : content_length(0) {}
	
	void handleError(int code);
    	std::string parseRequest(const std::string &request);
	

};

#endif
