#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.h"

class HttpRequestHandler;
class ServerConf;

class Server
{
private:

	std::vector<int> serverSockets;
	int epollFd;
	// Manage multiple server blocks in one event loop
	std::vector<ServerConf> serverConfs;
	std::map<int, size_t> listenFdToConf; // listening fd -> default server index for that port
	std::map<int, size_t> clientFdToConf; // client fd -> selected server index

	char buffer[1024];
	struct epoll_event events[10];
	HttpRequestHandler *httpRequestHandler;

	int make_socket_non_blocking(int fd);
	int safeAccept(int serverSocket);
	void AddEpollEvent(int fd, uint32_t events);
	int serverSocket_init();
	void HandleClient(int clientFd);
	void Handle_read_event(int clientFd);
	void Handle_send_event(int clientFd);

	static void handle_signal(int signum);
	static volatile bool running;

public:
	Server(const std::vector<ServerConf> &serverConfs);
	~Server();
	bool getRunning();
	void Server_run();
};

#endif // SERVER_HPP