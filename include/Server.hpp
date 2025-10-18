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
	// Manage multiple server blocks in one event loop
	std::vector<ServerConf> serverConfs_;
	std::map<int, size_t> listenFdToConf_; // listening fd -> default server index for that port
	std::map<int, size_t> clientFdToConf_; // client fd -> selected server index

	char buffer_[1024];
	struct epoll_event events_[10];
	HttpRequestHandler *httpRequestHandler_;

	int makeSocketNonBlocking(int fd);
	int acceptClient(int serverSocket);
	void addEpollEvent(int fd, uint32_t events);
	int initServerSockets();
	void HandleClient(int clientFd);
	void handleReadEvent(int clientFd);
	void handleSendEvent(int clientFd);

	static void handleSignal(int signum);
	static volatile bool running_;

public:
	Server(const std::vector<ServerConf> &serverConfs);
	~Server();
	bool isRunning();
	void run();
};

#endif // SERVER_HPP