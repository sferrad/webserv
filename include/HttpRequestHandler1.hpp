#ifndef HTTPREQUESTHANDLER_HPP
#define HTTPREQUESTHANDLER_HPP

#include "webserv1.h"

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

	////////////////////////////////////////
	//////////////MODIFS BILAL//////////////
	////////////////////////////////////////
	
	std::map<std::string, std::string> parseUrlEncoded(const std::string &body);
	std::string extractContentType(const std::string &request);
	size_t	extractContentLength(const std::string &request);
	std::string fullRequest_;
	std::string extractBoundary(const std::string &contentType);
    std::vector<std::string> splitMultipart(const std::string &body, const std::string &boundary);
    bool extractFileFromPart(const std::string &part, std::string &filename, std::string &fileContent);
    std::string sanitizeFilename(const std::string &filename);
	bool isValidContentType(const std::string &contentType);
	bool _isChunked;
	std::string unchunkBody(const std::string& chunkedBody);
	bool isChunkedBodyComplete(const std::string& body);

	////////////////////////////////////////
	////////////////////////////////////////
	
	public:
	
	////////////////////////////////////////
	//////////////MODIFS BILAL//////////////
	////////////////////////////////////////
	
	HttpRequestHandler() : visit_count_(1), serverConfig_(NULL), clientMaxBodySize_(10485760) {}
	HttpRequestHandler(const ServerConf* config) : visit_count_(1), serverConfig_(config), clientMaxBodySize_(10485760) {}
	size_t	clientMaxBodySize_;
	
	////////////////////////////////////////
	////////////////////////////////////////

	std::string root;
	std::string index;
	std::map<int, std::string> errorPages;
    std::string parseRequest(const std::string &request);
	void handleError(int code);
	bool parseHeader(const std::string &request);
	bool isChunked() const;
};

#endif // HTTPREQUESTHANDLER_HPP
