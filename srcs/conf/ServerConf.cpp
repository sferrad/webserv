#include "../../include/webserv.h"
#include <sstream>

// --- utils ----------------------------------------------------------

static std::string trim_token(const std::string &s)
{
	size_t start = s.find_first_not_of(" \t\r\n{");
	if (start == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n;}");
	return s.substr(start, end - start + 1);
}

static bool starts_with(const std::string &str, const std::string &prefix)
{
	return str.compare(0, prefix.size(), prefix) == 0;
}

// --- ServerConf class -----------------------------------------------

ServerConf::ServerConf(std::string configFile)
{
	if (configFile.empty())
	{
		std::cerr << "Error: Config file name is empty." << std::endl;
		return;
	}
	FindConfigValues(configFile);
}

void ServerConf::FindConfigValues(std::string configFile)
{
	std::ifstream file(configFile.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open config file " << configFile << std::endl;
		return;
	}

	std::string line;
	bool inServerBlock = false;

	while (getline(file, line))
	{
		// Remove comments (#)
		size_t hash = line.find('#');
		if (hash != std::string::npos)
			line.erase(hash);

		line = trim_token(line);
		if (line.empty())
			continue;

		// Detect start and end of "server" block
		if (starts_with(line, "server"))
		{
			if (line.find("{") != std::string::npos)
				inServerBlock = true;
			continue;
		}
		if (line == "}")
		{
			inServerBlock = false;
			continue;
		}
		if (!inServerBlock)
			continue;

		// Parse directives inside the server block
		if (starts_with(line, "listen"))
		{
			std::string v = trim_token(line.substr(6));
			if (!v.empty() && v[v.size() - 1] == ';')
				v.erase(v.size() - 1);
			try
			{
				port = stoi(v);
			}
			catch (std::exception &e)
			{
				std::cerr << "Error: invalid port value '" << v << "'\n";
				port = 8080;
			}
		}
		else if (starts_with(line, "host"))
			host = trim_token(line.substr(4));
		else if (starts_with(line, "root"))
			root = trim_token(line.substr(4));
		else if (starts_with(line, "index"))
			index = trim_token(line.substr(5));
		else if (starts_with(line, "error_page"))
		{
			// optional future use — ignore for now
		}
	}

	file.close();

	// Default values if missing
	if (host.empty())
		host = "127.0.0.1";
	if (port == 0)
		port = 8080;
	if (root.empty())
		root = "./www";
	if (index.empty())
		index = "index.html";

	std::cout << "✅ Config loaded: host=" << host
			  << " | port=" << port
			  << " | root=" << root
			  << " | index=" << index << std::endl;
}

// --- getters ---------------------------------------------------------

std::string ServerConf::getHost() const { return host; }
int ServerConf::getPort() const { return port; }
std::string ServerConf::getRoot() const { return root; }
std::string ServerConf::getIndex() const { return index; }
