#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.h"

class HttpRequestHandler;
class ServerConf;

class Server
{
private:

	int serverSocket;
	int epollFd;
	int port;
	std::string root;
	std::string index;
	std::string host;

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
	Server(const ServerConf &serverConf);
	~Server();
	bool getRunning();
	void Server_run();
};

#endif // SERVER_HPP