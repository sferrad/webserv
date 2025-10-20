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
#include <string.h>
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

bool isEmpty(std::string str);
int stoi( std::string & s );

#endif // WEBSERV_H