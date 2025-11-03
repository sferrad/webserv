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
	std::time_t now = std::time(NULL);
	char *timeStr = std::ctime(&now);
	timeStr[strlen(timeStr) - 1] = '\0';
	std::strftime(timeStr, 100, "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
	return timeStr;
}