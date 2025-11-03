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