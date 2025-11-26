#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.h"

class HttpRequestHandler;
class ServerConf;

class Server
{
private:
	std::vector<int> listenSockets_;
	int epollFd_;
	
	std::map<int, time_t> clientLastActivity_;
	std::map<int, time_t> clientSendStart_;

	std::string host_;
	std::vector<ServerConf> serverConfs_;
	std::map<int, size_t> listenFdToConf_;
	std::map<int, size_t> clientFdToConf_;
	std::map<int, std::string> clientBuffers_;
	std::map<int, std::string> sendBuffers_;
	std::map<int, size_t> sendOffsets_;

	std::map<int, int> listenFdToPort_;
	std::map<int, int> clientFdToPort_;
	std::map<int, std::string> clientFdToIp_;

	
	char **envp_;
	char buffer_[1024];
	struct epoll_event events_[10];
	HttpRequestHandler *httpRequestHandler_;

	void checkRequestTimeouts();
	int makeSocketNonBlocking(int fd);
	int acceptClient(int serverSocket);
	void addEpollEvent(int fd, uint32_t events);
	void checkTimeouts();
	int initServerSockets();
	void HandleClient(int clientFd);
	void handleReadEvent(int clientFd);
	void handleSendEvent(int clientFd);

	static void handleSignal(int signum);
	static volatile bool running_;
	int clientTimeout_;
public:
	Server(const std::vector<ServerConf> &serverConfs, char **envp = NULL);
	~Server();
	bool isRunning();
	void run();
};

#endif