#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.h"

class HttpRequestHandler;

class Server
{
private:
	bool running;

	int serverSocket;
	int epollFd;
	int port;
	char buffer[1024];
	struct epoll_event events[10];
	HttpRequestHandler* httpRequestHandler;

	int make_socket_non_blocking(int fd);
	int safeAccept(int serverSocket);
	void AddEpollEvent(int fd, uint32_t events);
	int serverSocket_init(int port);
	void HandleClient(int clientFd);
	void Handle_read_event(int clientFd);
	void Handle_send_event(int clientFd);
public:
	Server(int port);
	void Server_run();
	~Server();
};

#endif // SERVER_HPP