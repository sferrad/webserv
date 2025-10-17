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
}

std::map<int, std::string> MapErrorPage(const std::string &line) {
    std::istringstream iss(line);
    int code;
    std::string page;

    iss >> code >> page;

    std::cout << "code : " << code << " page : " << page << std::endl;

    std::map<int, std::string> error_page;
    error_page[code] = page;

    return error_page;
}

// --- static: parse vector of ServerConf from config  -----------------
std::vector<ServerConf> ServerConf::parseConfigFile(const std::string &configFile)
{
	std::vector<ServerConf> result;
	std::ifstream file(configFile.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open config file " << configFile << std::endl;
		return result;
	}

	std::string line;
	bool inServerBlock = false;
	std::vector<int> ports;
	std::string root;
	std::string index;
	std::string host;
	std::map<int, std::string> error_page;
	while (getline(file, line))
	{
		size_t hash = line.find('#');
		if (hash != std::string::npos) line.erase(hash);
		line = trim_token(line);
		if (line.empty()) continue;

		if (starts_with(line, "server"))
		{
			if (inServerBlock)
			{
				if (host.empty()) host = "127.0.0.1";
				if (ports.empty()) ports.push_back(8080);
				if (root.empty()) root = "./www";
				if (index.empty()) index = "index.html";
				ServerConf sc(ports, root, index, host);
				if (!error_page.empty()) sc.setErrorPages(error_page);
				result.push_back(sc);
				ports.clear(); root.clear(); index.clear(); host.clear(); error_page.clear();
			}
			if (line.find("{") != std::string::npos)
				inServerBlock = true;
			continue;
		}
		if (line == "}")
		{
			if (inServerBlock)
			{
				if (host.empty()) host = "127.0.0.1";
				if (ports.empty()) ports.push_back(8080);
				if (root.empty()) root = "./www";
				if (index.empty()) index = "index.html";
				ServerConf sc(ports, root, index, host);
				if (!error_page.empty()) sc.setErrorPages(error_page);
				result.push_back(sc);
				ports.clear(); root.clear(); index.clear(); host.clear(); error_page.clear();
			}
			inServerBlock = false;
			continue;
		}
		if (!inServerBlock)
			continue;

		if (starts_with(line, "listen"))
		{
			std::string v = trim_token(line.substr(6));
			if (!v.empty() && v[v.size() - 1] == ';') v.erase(v.size() - 1);
			try { ports.push_back(stoi(v)); }
			catch (...) { std::cerr << "Error: invalid port value '" << v << "'\n"; }
		}
		else if (starts_with(line, "host"))
			host = trim_token(line.substr(4));
		else if (starts_with(line, "root"))
			root = trim_token(line.substr(4));
		else if (starts_with(line, "index"))
			index = trim_token(line.substr(5));
		else if (starts_with(line, "error_page")){
			std::map<int, std::string> one = MapErrorPage(trim_token(line.substr(10)));
			error_page.insert(one.begin(), one.end());
		}
	}
	if (inServerBlock)
	{
		if (host.empty()) host = "127.0.0.1";
		if (ports.empty()) ports.push_back(8080);
		if (root.empty()) root = "./www";
		if (index.empty()) index = "index.html";
		ServerConf sc(ports, root, index, host);
		if (!error_page.empty()) sc.setErrorPages(error_page);
		result.push_back(sc);
	}

	file.close();
	return result;
}

// --- getters ---------------------------------------------------------

std::string ServerConf::getHost() const { return host_; }
std::string ServerConf::getRoot() const { return root_; }
std::string ServerConf::getIndex() const { return index_; }
std::map<int, std::string> ServerConf::getErrorPages() const { return errorPages_; }