#ifndef SERVER_CONF_HPP
#define SERVER_CONF_HPP

#include "webserv.h"

class ServerConf {
private:
	int port;
	std::string root;
	std::string index;
	std::string host;

	void FindConfigValues(std::string configFile);
public:
	ServerConf(std::string configFile);
	int getPort() const;
	std::string getRoot() const;
	std::string getIndex() const;
	std::string getHost() const;
};

#endif // SERVER_CONF_HPP