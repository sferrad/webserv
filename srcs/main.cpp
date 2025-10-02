#include "../include/webserv.h"

int make_socket_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int serverSocket_init()
{
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		perror("socket");
		exit(1);
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(8080);

	if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	if (listen(serverSocket, 5) < 0)
	{
		perror("listen");
		exit(1);
	}

	make_socket_non_blocking(serverSocket);

	return serverSocket;
}

int safeAccept(int serverSocket)
{
	int clientSocket = accept(serverSocket, NULL, NULL);
	if (clientSocket < 0)
	{
		throw std::runtime_error(std::string("Accept failed: ") + strerror(errno));
	}
	make_socket_non_blocking(clientSocket);
	return clientSocket;
}

int main()
{
	int serverSocket = serverSocket_init();
	int clientSocket = -1;

	int epollFd = epoll_create(1);
	if (epollFd == -1)
	{
		perror("Epoll_create failed");
		exit(1);
	}

	struct epoll_event ev;
	struct epoll_event events[10];
	memset(&ev, 0, sizeof(ev));
	memset(events, 0, sizeof(events));

	ev.events = EPOLLIN;
	ev.data.fd = serverSocket;

	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &ev) == -1)
	{
		perror("Epoll_ctl failed");
		exit(1);
	}

	while (true)
	{
		int n = epoll_wait(epollFd, events, 10, -1);
		if (n == -1)
		{
			perror("Epoll_wait failed");
			exit(1);
		}
		for (int i = 0; i < n; i++)
		{
			if (events[i].data.fd == serverSocket)
			{
				// New incoming connection
				try
				{
					clientSocket = safeAccept(serverSocket);
				}
				catch (const std::runtime_error &e)
				{
					std::cerr << e.what() << std::endl;
					clientSocket = -1;
					continue;
				}
				std::cout << "Client connected" << std::endl;
				make_socket_non_blocking(clientSocket);

				ev.events = EPOLLIN;
				ev.data.fd = clientSocket;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &ev) == -1)
				{
					perror("Epoll_ctl add client failed");
					close(clientSocket);
					continue;
				}
				std::cout << "Client connected" << std::endl;
				std::cout << "test" << std::endl;
			}
			else
			{
				int clientFd = events[i].data.fd;
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
				}
			}
		}
	}
}