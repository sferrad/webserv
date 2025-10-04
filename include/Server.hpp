#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.h"

class Server
{
private:
	int serverSocket;
	int epollFd;
	int port;

	int make_socket_non_blocking(int fd);
	int safeAccept(int serverSocket);
	void AddEpollEvent(int fd, uint32_t events);
	int serverSocket_init(int port);
	void HandleClient(int clientFd);
public:
	Server(int port);
	void Server_run();
	~Server();
};

#endif // SERVER_HPP