#include "../../include/webserv.h"

static std::string trim_token(const std::string &s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(" \t\r\n;"); // retire aussi le ';' final
	return s.substr(start, end - start + 1);
}

ServerConf::ServerConf(std::string configFile) {
	// Parsing logic to initialize port, root, index, host from configFile
	FindConfigValues(configFile);
}

void ServerConf::FindConfigValues(std::string configFile) {
	std::ifstream file(configFile.c_str());
	if (!file.is_open()) {
		std::cerr << "Error: Could not open config file " << configFile << std::endl;
		return;
	}

	std::string line;
	while (getline(file, line))
	{
		// retirer commentaires simples '#'
		size_t hash = line.find('#');
		if (hash != std::string::npos) line.erase(hash);

		// listen 8080; | root ./www; | index index.html; | host 127.0.0.1;
		if (line.find("listen") != std::string::npos) {
			std::string v = trim_token(line.substr(line.find("listen") + 6));
			v = trim_token(v);
			port = stoi(v);
		}
		else if (line.find("root") != std::string::npos) {
			root = trim_token(line.substr(line.find("root") + 4));
		}
		else if (line.find("index") != std::string::npos) {
			index = trim_token(line.substr(line.find("index") + 5));
		}
		else if (line.find("host") != std::string::npos) {
			host = trim_token(line.substr(line.find("host") + 4));
		}
	}

	file.close();
}

// Getter methods

std::string ServerConf::getHost() const {
	return host;
}

int ServerConf::getPort() const {
	return port;
}

std::string ServerConf::getRoot() const {
	return root;
}

std::string ServerConf::getIndex() const {
	return index;
}

// -----------------------------------------------------