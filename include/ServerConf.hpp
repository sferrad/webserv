#ifndef SERVER_CONF_HPP
#define SERVER_CONF_HPP

#include "webserv.h"

struct Location {
	std::string path;
	std::string root;
	std::string index;
	std::vector<std::string> allowed_methods;
	std::string default_index;
	bool autoindex;
	std::string upload_store;
	
	Location() : autoindex(false) {}
};

class ServerConf {
private:
	std::vector<int> ports_;
	std::string root_;
	std::string index_;
	std::string host_;
	std::map<int, std::string> errorPages_;
	std::vector<Location> locations_;
	size_t clientMaxBodySize_;

	ServerConf(const std::vector<int> &ports, const std::string &root, const std::string &index, const std::string &host)
		: ports_(ports), root_(root), index_(index), host_(host), clientMaxBodySize_(10485760) {}
		
public:
	ServerConf(std::string configFile);
	void setErrorPages(const std::map<int, std::string> &m) { errorPages_ = m; }
	void setClientMaxBodySize(size_t size) { clientMaxBodySize_ = size; }
	
	size_t getPortsCount() const { return ports_.size(); }
	int getPort(size_t i) const { return ports_[i]; }
	std::vector<int> getPorts() const { return ports_; }

 	std::string getRoot() const;
	std::string getIndex() const;
	std::string getHost() const;
	std::map<int, std::string> getErrorPages() const;
	std::vector<Location> getLocations() const;
	Location* findLocation(const std::string &uri) const;
	size_t getClientMaxBodySize() const { return clientMaxBodySize_; }

	static std::vector<ServerConf> parseConfigFile(const std::string &configFile);
};

#endif