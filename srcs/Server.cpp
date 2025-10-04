#include "../include/webserv.h"

// --------- Constructor and Destructor -----------
Server::Server(int port)
{
	Server::port = port;
	Server::serverSocket = -1;
	Server::epollFd = -1;
}

Server::~Server()
{
	epoll_ctl(epollFd, EPOLL_CTL_DEL, serverSocket, NULL);
	close(serverSocket);
	close(epollFd);
	exit(0);
}
// -----------------------------------------------------

int Server::make_socket_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error(std::string("Fcntl get flags failed: ") + strerror(errno));
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::AddEpollEvent(int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
		throw std::runtime_error(std::string("Epoll_ctl add failed: ") + strerror(errno));
}

int Server::safeAccept(int serverSocket)
{
	int clientSocket = accept(serverSocket, NULL, NULL);
	if (clientSocket < 0)
		throw std::runtime_error(std::string("Accept failed: ") + strerror(errno));

	make_socket_non_blocking(clientSocket);
	AddEpollEvent(clientSocket, EPOLLIN);

	std::cout << "Client connected" << std::endl;
	return clientSocket;
}

int Server::serverSocket_init(int port)
{
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
		throw std::runtime_error(std::string("Socket creation failed: ") + strerror(errno));

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);
	int opt = 1;

	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error(std::string("Setsockopt failed: ") + strerror(errno));

	if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
		throw std::runtime_error(std::string("Bind failed: ") + strerror(errno));
	
	if (listen(serverSocket, 5) < 0)
		throw std::runtime_error(std::string("Listen failed: ") + strerror(errno));

	Server::epollFd = epoll_create(1);
	if (epollFd == -1)
		throw std::runtime_error(std::string("Epoll_create failed: ") + strerror(errno));
	AddEpollEvent(serverSocket, EPOLLIN);
	make_socket_non_blocking(serverSocket);

	return serverSocket;
}

void Server::HandleClient(int clientFd)
{
	char buffer[1024] = {0};
	int bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
	if (bytesRead <= 0)
	{
		std::cout << "Client disconnected" << std::endl;
		close(clientFd);
		epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
	}
	else
	{
		buffer[bytesRead] = '\0';
		std::cout << "Received: " << buffer << std::endl;
		buffer[strcspn(buffer, "\r\n")] = 0;

		const char *message = "Hello, client!\n";
		int bytesSent = send(clientFd, message, strlen(message), 0);

		if (bytesSent < 0)
			std::cerr << "Error send: " << strerror(errno) << std::endl;
		else
			std::cout << "Message sent to client." << std::endl;
		if (strcmp(buffer, "exit") == 0)
		{
			close(clientFd);
			epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
			std::cout << "Client disconnected" << std::endl;
		}
		if (strcmp(buffer, "shutdown") == 0)
		{
			std::cout << "Server shutting down..." << std::endl;
			close(clientFd);
			epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
			Server::~Server();
		}
	}
}


void Server::Server_run()
{
	Server::serverSocket = serverSocket_init(Server::port);
	struct epoll_event events[10];
	while (true)
	{
		int numEvents = epoll_wait(Server::epollFd, events, 10, -1);
		for (int i = 0; i < numEvents; i++)
		{
			if (events[i].events & EPOLLIN)
			{
				if (events[i].data.fd == Server::serverSocket)
				{
					int clientSocket = safeAccept(Server::serverSocket);
					std::cout << "New client connected: " << clientSocket << std::endl;
				}
				else
					HandleClient(events[i].data.fd);
			}
		}
	}
}
