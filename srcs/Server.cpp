#include "../include/webserv.h"

volatile bool Server::running = true;

void Server::handle_signal(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		Server::running = false;
	}
}

// --------- Constructor and Destructor -----------

Server::Server(const ServerConf &serverConf) {
	this->root = serverConf.getRoot();
	this->index = serverConf.getIndex();
	this->host = serverConf.getHost();
	this->port = serverConf.getPorts();
	this->epollFd = -1;
	memset(this->buffer, 0, sizeof(this->buffer));
	memset(this->events, 0, sizeof(this->events));
	httpRequestHandler = new HttpRequestHandler();
	httpRequestHandler->root = this->root;
	httpRequestHandler->index = this->index; 
}


Server::~Server()
{
	for (size_t i = 0; i < serverSockets.size(); ++i) {
		int s = serverSockets[i];
		if (s != -1)
			epoll_ctl(epollFd, EPOLL_CTL_DEL, s, NULL);
		if (s != -1)
			close(s);
	}
	if (epollFd != -1) close(epollFd);
	delete httpRequestHandler;
	std::cout << "\nServer stopped." << std::endl;
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

int Server::serverSocket_init()
{
	Server::epollFd = epoll_create(1);
	if (epollFd == -1)
		throw std::runtime_error(std::string("Epoll_create failed: ") + strerror(errno));

	for (size_t i = 0; i < this->port.size(); ++i) {
		int s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == -1)
			throw std::runtime_error(std::string("Socket creation failed: ") + strerror(errno));

		int opt = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
			throw std::runtime_error(std::string("Setsockopt failed: ") + strerror(errno));

		struct sockaddr_in serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(this->port[i]);

		if (bind(s, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
			std::ostringstream oss;
			oss << "Bind failed on port " << this->port[i] << ": " << strerror(errno);
			throw std::runtime_error(oss.str());
		}

		if (listen(s, 128) < 0)
			throw std::runtime_error(std::string("Listen failed: ") + strerror(errno));

		make_socket_non_blocking(s);
		AddEpollEvent(s, EPOLLIN);
		serverSockets.push_back(s);
		std::cout << "Listening on port " << this->port[i] << std::endl;
	}

	return 0;
}

// ---------------- Handle Client Events ---------------------------

void Server::Handle_read_event(int clientFd)
{
    int bytesRead = read(clientFd, Server::buffer, sizeof(Server::buffer) - 1);
    if (bytesRead <= 0)
    {
        std::cout << "Client disconnected" << std::endl;
        close(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        return;
    }

    Server::buffer[bytesRead] = '\0';
	std::cout << "Received: " << Server::buffer << std::endl;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = clientFd;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
}


void Server::Handle_send_event(int clientFd)
{
    std::string response = httpRequestHandler->parse_request(std::string(buffer));
    int bytesSent = send(clientFd, response.c_str(), response.length(), 0);

    if (bytesSent <= 0)
    {
        std::cerr << "Error: Send failed or connection closed" << std::endl;
        close(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        return;
    }

    std::cout << "Message sent to client." << std::endl;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = clientFd;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
}


bool Server::getRunning()
{
	return this->running;
}

void Server::Server_run()
{
	serverSocket_init();
	signal(SIGINT, Server::handle_signal);
	signal(SIGTERM, Server::handle_signal);
	epoll_event events[10];
	while (this->running)
	{
		int numEvents = epoll_wait(epollFd, events, 10, 1000);
		for (int i = 0; i < numEvents; i++)
		{
			int fd = events[i].data.fd;
			if (events[i].events & EPOLLIN)
			{
				bool isListening = false;
				for (size_t k = 0; k < serverSockets.size(); ++k) {
					if (fd == serverSockets[k]) { isListening = true; break; }
				}
				if (isListening)
				{
					int clientSocket = safeAccept(fd);
					std::cout << "New client connected: " << clientSocket << std::endl;
				}
				else
					Handle_read_event(fd);
			}
			if (events[i].events & EPOLLOUT)
				Handle_send_event(fd);
		}
	}
}
