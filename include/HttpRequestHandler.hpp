#ifndef HTTPREQUESTHANDLER_HPP
#define HTTPREQUESTHANDLER_HPP

#include "webserv.h"

class ServerConf;

class HttpRequestHandler {
private:  
	std::string contentType_;
	std::string method_;
	std::string body_;
	std::string uri_;
	std::string queryString_;
	std::map<std::string, std::string> headers_;
	std::string clientIp_;

	long int visit_count_;
	int error_code_;
	std::ostringstream respBody_;
	std::ostringstream resp_;
	const ServerConf* serverConfig_;
	bool is403Forbidden_;
	std::string currentRequest_;

	void extractBody(const std::string &request);
	bool isValidMethod(const std::string &request);
	bool isMethodAllowed(const std::string &method, const std::string &uri);
	int getHtmlPage();
	bool handleDeleteRequest();
	bool handlePostRequest();
	bool handlePutRequest();
	void getUri(const std::string &request);
	std::string generateAutoindex(const std::string &dirPath, const std::string &uri);
	std::string handleRedirect();
	bool isTransferEncodingChunked(const std::string &request) const;
    bool extractChunkedBody(const std::string &request, std::string &outBody, size_t &outSize) const;
    int hexStringToInt(const std::string &hexStr) const;
	std::string extractBoundaryFromContentType(const std::string &contentType);
	std::string getAllowedMethodsHeader(const std::string &uri);
	bool parseMultipartBody(const std::string &body, 
                           const std::string &boundary,
                           std::string &fileContent,
                           std::string &filename);
	void parseHeaders(const std::string &request);

public:
		char **env_;
	HttpRequestHandler() : visit_count_(1), error_code_(0), serverConfig_(NULL), is403Forbidden_(false), env_(NULL), autoindex_(false) {}
	HttpRequestHandler(const ServerConf* config) : visit_count_(1), error_code_(0), serverConfig_(config), is403Forbidden_(false), env_(NULL), autoindex_(false) {}
	
	bool autoindex_;
	std::string server_name_;
	std::string root;
	std::string index;
	std::vector<std::pair<std::string, std::string> > cgi_pass;
	std::map<int, std::string> errorPages;
	std::map<int, std::string> redirects;
	std::string parseRequest(const std::string &request);
	void handleError(int code);
	bool parseHeader(const std::string &request);
	std::string getQueryString() const { return queryString_; }
	void setClientIp(const std::string &ip) { clientIp_ = ip; }
};

#endif

