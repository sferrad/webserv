#ifndef SERVER_CONF_HPP
#define SERVER_CONF_HPP

#include "webserv.h"

class ServerConf {
private:
	std::vector<int> ports_;
	std::string root_;
	std::string index_;
	std::string host_;
	std::map<int, std::string> errorPages_;

	ServerConf(const std::vector<int> &ports, const std::string &root, const std::string &index, const std::string &host)
		: ports_(ports), root_(root), index_(index), host_(host) {}
public:
	ServerConf(std::string configFile);
	void setErrorPages(const std::map<int, std::string> &m) { errorPages_ = m; }
	size_t getPortsCount() const { return ports_.size(); }
	int getPort(size_t i) const { return ports_[i]; }
	std::vector<int> getPorts() const { return ports_; }

 	std::string getRoot() const;
	std::string getIndex() const;
	std::string getHost() const;
	std::map<int, std::string> getErrorPages() const;

	static std::vector<ServerConf> parseConfigFile(const std::string &configFile);
};

#endif // SERVER_CONF_HPP