#ifndef SERVER_CONF_HPP
#define SERVER_CONF_HPP

#include "webserv.h"

struct Location {
	std::string path;
	std::string root;
	std::string index;
	std::vector<std::string> allowed_methods;
	std::string default_index;
	std::map<int, std::string> redirects;
	bool autoindex;
	size_t client_max_body_size;
	std::vector<std::pair<std::string, std::string> > cgi_pass;
	std::string upload_store;
	
	Location() : autoindex(false), client_max_body_size(0) {}

	Location(const Location &other) 
		: path(other.path), root(other.root), index(other.index),
		  allowed_methods(other.allowed_methods), default_index(other.default_index),
		  redirects(other.redirects), autoindex(other.autoindex),
		  client_max_body_size(other.client_max_body_size),
		  cgi_pass(other.cgi_pass), upload_store(other.upload_store) {}  // ✅ ADDED!

	Location &operator=(const Location &other) {
		if (this != &other) {
			path = other.path;
			root = other.root;
			index = other.index;
			allowed_methods = other.allowed_methods;
			default_index = other.default_index;
			redirects = other.redirects;
			autoindex = other.autoindex;
			client_max_body_size = other.client_max_body_size;
			cgi_pass = other.cgi_pass;
			upload_store = other.upload_store;
		}
		return *this;
	}
};

class ServerConf {
private:
	std::vector<int> ports_;
	std::string root_;
	std::string index_;
	std::string host_;
	std::vector<std::pair<std::string, std::string> > cgi_pass_;
	std::map<int, std::string> errorPages_;
	std::vector<Location> locations_;
	size_t client_max_body_size_;

	ServerConf(const std::vector<int> &ports, const std::string &root, const std::string &index, const std::string &host)
		: ports_(ports), root_(root), index_(index), host_(host), client_max_body_size_(0) {}
public:

	size_t getClientMaxBodySize() const { return client_max_body_size_; }
	ServerConf(std::string configFile);
	void setErrorPages(const std::map<int, std::string> &m) { errorPages_ = m; }
	size_t getPortsCount() const { return ports_.size(); }
	int getPort(size_t i) const { return ports_[i]; }
	std::vector<int> getPorts() const { return ports_; }

 	std::string getRoot() const;
	std::string getIndex() const;
	std::string getHost() const;
	std::map<int, std::string> getErrorPages() const;
	std::vector<Location> getLocations() const;
	Location* findLocation(const std::string &uri) const;

	std::vector<std::pair<std::string, std::string> > getCgiPass() const { return cgi_pass_; }
	bool isAutoindexEnabled(const std::string &uri) const;
	static std::vector<ServerConf> parseConfigFile(const std::string &configFile);
};

#endif