#ifndef SERVER_CONF_HPP
#define SERVER_CONF_HPP

#include "webserv.h"

class ServerConf {
private:
	std::vector<int> ports;
	std::string root;
	std::string index;
	std::string host;

	void FindConfigValues(std::string configFile);
	ServerConf(const std::vector<int> &ports, const std::string &root, const std::string &index, const std::string &host)
		: ports(ports), root(root), index(index), host(host) {}
public:
	ServerConf(std::string configFile);
	size_t getPortsCount() const { return ports.size(); }
	int getPort(size_t i) const { return ports[i]; }
	std::vector<int> getPorts() const { return ports; }

 	std::string getRoot() const;
	std::string getIndex() const;
	std::string getHost() const;

	static std::vector<ServerConf> parseConfigFile(const std::string &configFile);
};

#endif // SERVER_CONF_HPP