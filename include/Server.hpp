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
	std::vector<int> serverSockets;
	int epollFd;
	std::vector<int> port;
	std::string root;
	std::string index;
	std::string host;
	std::map<int, std::string> error_page;

	//////////MODIF BILAL//////////
	// char buffer[1024];
	std::map<int, std::string> clientBuffers;
	//////////MODIF BILAL//////////

	struct epoll_event events[10];
	HttpRequestHandler *httpRequestHandler;

	int make_socket_non_blocking(int fd);
	int safeAccept(int serverSocket);
	void AddEpollEvent(int fd, uint32_t events);
	int serverSocket_init();
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