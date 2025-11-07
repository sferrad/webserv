#ifndef WEBSERV_H
#define WEBSERV_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include "Server.hpp"
#include "HttpRequestHandler.hpp"
#include "ServerConf.hpp"
#include <ctime>
#include <iostream>

bool isEmpty(std::string str);
int stoi( std::string & s );
bool isDirectory(const std::string &path);
char *getCurrentTime();
std::string extractHost(const std::string &request);
ServerConf* selectServer(const std::string& hostHeader, int port, std::vector<ServerConf>& servers);

#endif // WEBSERV_H