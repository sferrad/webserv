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
