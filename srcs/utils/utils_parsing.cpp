#include "../../include/webserv.h"

bool isEmpty(std::string str)
{
    str.erase(0, str.find_first_not_of(" \t\n\r"));
    return str.empty();
}

int stoi(std::string &s)
{
    int i;
    std::istringstream(s) >> i;
    return i;
}

bool isDirectory(const std::string &path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == 0)
        return S_ISDIR(s.st_mode);
    return false;
}

char *getCurrentTime()
{
	static char timeStr[100];
	std::time_t now = std::time(NULL);
	std::strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
	return timeStr;
}

std::string extractHost(const std::string &request) {
    size_t pos = request.find("Host:");
    if (pos == std::string::npos)
        return "";

    pos += 5;
    while (pos < request.size() && std::isspace(request[pos]))
        pos++;

    size_t end = request.find_first_of("\r\n", pos);
    if (end == std::string::npos)
        end = request.size();

    size_t colon = request.find(":", pos);
    if (colon != std::string::npos && colon < end)
        end = colon;

    return request.substr(pos, end - pos);
}


ServerConf* selectServer(const std::string& hostHeader, int port, std::vector<ServerConf>& servers)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const std::vector<int>& ports = servers[i].getPorts();
        if (std::find(ports.begin(), ports.end(), port) != ports.end())
        {
            if (servers[i].getHost() == hostHeader)
                return &servers[i];
        }
    }

    for (size_t i = 0; i < servers.size(); ++i)
    {
        const std::vector<int>& ports = servers[i].getPorts();
        if (std::find(ports.begin(), ports.end(), port) != ports.end())
            return &servers[i];
    }

    return NULL;
}

