#include "../../include/webserv.h"
#include <sstream>

// --- utils ----------------------------------------------------------

static std::string trim_token(const std::string &s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n;");
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

    std::map<int, std::string> error_page;
    error_page[code] = page;

    return error_page;
}

std::map<int, std::string> mapRedirects(const std::string &line) {
	std::istringstream iss(line);
	int code;
	std::string url;

	iss >> code >> url;

	std::map<int, std::string> redirects;
	redirects[code] = url;

	return redirects;
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
	bool inLocationBlock = false;
	bool inRootLocation = false;
	int braceDepth = 0;
	std::vector<int> ports;
	std::string root;
	std::string index;
	std::string host;
	std::map<int, std::string> error_page;
	std::vector<Location> locations;
	Location currentLocation;
	while (getline(file, line))
	{
		size_t hash = line.find('#');
		if (hash != std::string::npos) line.erase(hash);
		line = trim_token(line);
		if (line.empty()) continue;

		if (starts_with(line, "server") && !inServerBlock)
		{
			if (!ports.empty() || !root.empty() || !index.empty() || !host.empty())
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
			{
				inServerBlock = true;
				braceDepth = 1;
			}
			continue;
		}

		if (inServerBlock)
		{
			if (line.find("{") != std::string::npos)
			{
				braceDepth++;
				if (!inLocationBlock && starts_with(line, "location"))
				{
					inLocationBlock = true;
					std::istringstream iss(line);
					std::string keyword, path;
					iss >> keyword >> path;
					// Remove any trailing braces or semicolons from path
					while (!path.empty() && (path[path.size() - 1] == '{' || path[path.size() - 1] == ';'))
						path.erase(path.size() - 1);
					currentLocation = Location();
					currentLocation.path = path;

					inRootLocation = path == "/";
				}
				continue;
			}
			if (line.find("}") != std::string::npos)
			{
				braceDepth--;
				if (braceDepth == 0)
				{
					if (host.empty()) host = "127.0.0.1";
					if (ports.empty()) ports.push_back(8080);
					if (root.empty()) root = "./www";
					if (index.empty()) index = "index.html";
					ServerConf sc(ports, root, index, host);
					if (!error_page.empty()) sc.setErrorPages(error_page);
					sc.locations_ = locations;
					result.push_back(sc);
					ports.clear(); root.clear(); index.clear(); host.clear(); error_page.clear(); locations.clear();
					inServerBlock = false;
				}
				else if (inLocationBlock)
				{
					if (braceDepth == 1)
					{
						locations.push_back(currentLocation);
						inLocationBlock = false;
						inRootLocation = false;
					}
				}
				continue;
			}
		}

		if (!inServerBlock)
			continue;

		if (starts_with(line, "listen") && !inLocationBlock)
		{
			std::string v = trim_token(line.substr(6));
			if (!v.empty() && v[v.size() - 1] == ';') v.erase(v.size() - 1);
			try { ports.push_back(stoi(v)); }
			catch (...) { std::cerr << "Error: invalid port value '" << v << "'\n"; }
		}
		else if (starts_with(line, "server_name") && !inLocationBlock)
			host = trim_token(line.substr(12));
		else if (starts_with(line, "root"))
		{
			std::string rootValue = trim_token(line.substr(4));
			if (inLocationBlock)
				currentLocation.root = rootValue;
			else
				root = rootValue;
		}
		else if (starts_with(line, "index"))
		{
			std::string indexValue = trim_token(line.substr(5));
			if (inLocationBlock)
				currentLocation.index = indexValue;
			else
				index = indexValue;
		}
		else if (starts_with(line, "allowed_methods") && inLocationBlock)
		{
			std::string methodsStr = trim_token(line.substr(15));
			std::istringstream iss(methodsStr);
			std::string method;
			while (iss >> method)
			{
				if (method[method.size() - 1] == ';')
					method.erase(method.size() - 1);
				currentLocation.allowed_methods.push_back(method);
			}
		}
		else if (starts_with(line, "autoindex") && inLocationBlock)
		{
			std::string value = trim_token(line.substr(9));
			if (value[value.size() - 1] == ';')
				value.erase(value.size() - 1);
			currentLocation.autoindex = (value == "on");
		}
		else if (starts_with(line, "error_page") && !inLocationBlock)
		{
			std::map<int, std::string> one = MapErrorPage(trim_token(line.substr(10)));
			error_page.insert(one.begin(), one.end());
		}
		else if (starts_with(line, "return") && inLocationBlock)
		{
			std::map<int, std::string> redirects = mapRedirects(trim_token(line.substr(6)));
			currentLocation.redirects.insert(redirects.begin(), redirects.end());
			std::cout << "DEBUG: Parsed return redirect for location " << currentLocation.path << std::endl;
		}
		else if (starts_with(line, "redirect") && inLocationBlock)
		{
			std::map<int, std::string> redirects = mapRedirects(trim_token(line.substr(8)));
			currentLocation.redirects.insert(redirects.begin(), redirects.end());
			std::cout << "DEBUG: Parsed redirect for location " << currentLocation.path << std::endl;
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
		sc.locations_ = locations;
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
std::vector<Location> ServerConf::getLocations() const { return locations_; }
bool ServerConf::isAutoindexEnabled(const std::string &uri) const {
	Location* loc = findLocation(uri);
	if (loc) 
		return loc->autoindex;
	return false;
}

Location* ServerConf::findLocation(const std::string &uri) const {
    Location* bestMatch = NULL;
    size_t bestMatchLength = 0;
    
    for (size_t i = 0; i < locations_.size(); i++) {
        const std::string& locPath = locations_[i].path;
        if (uri.find(locPath) == 0 && (uri.size() == locPath.size() || uri[locPath.size()] == '/')) {
            if (locPath.length() > bestMatchLength) {
                bestMatch = const_cast<Location*>(&locations_[i]);
                bestMatchLength = locPath.length();
            }
        }
    }
    return bestMatch;
}

