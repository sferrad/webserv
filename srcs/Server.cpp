#include "../include/webserv.h"

volatile bool Server::running_ = true;

void Server::handleSignal(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
	{
		Server::running_ = false;
	}
}

// --------- Constructor and Destructor -----------

Server::Server(const std::vector<ServerConf> &serverConfs) : epollFd_(-1), serverConfs_(serverConfs)
{
	memset(this->buffer_, 0, sizeof(this->buffer_));
	memset(this->events_, 0, sizeof(this->events_));
	httpRequestHandler_ = new HttpRequestHandler();
}

Server::~Server()
{
	for (size_t i = 0; i < listenSockets_.size(); ++i)
	{
		int s = listenSockets_[i];
		if (s != -1)
			epoll_ctl(epollFd_, EPOLL_CTL_DEL, s, NULL);
		if (s != -1)
			close(s);
	}
	if (epollFd_ != -1)
		close(epollFd_);
	delete httpRequestHandler_;
	std::cout << "\nServer stopped." << std::endl;
}

// -----------------------------------------------------

int Server::makeSocketNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error(std::string("Fcntl get flags failed: ") + strerror(errno));
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::addEpollEvent(int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == -1)
		throw std::runtime_error(std::string("Epoll_ctl add failed: ") + strerror(errno));
}

int Server::acceptClient(int serverSocket)
{
	int clientSocket = accept(serverSocket, NULL, NULL);
	if (clientSocket < 0)
		throw std::runtime_error(std::string("Accept failed: ") + strerror(errno));

	makeSocketNonBlocking(clientSocket);
	addEpollEvent(clientSocket, EPOLLIN);

	std::map<int, size_t>::iterator it = listenFdToConf_.find(serverSocket);
	if (it != listenFdToConf_.end())
	{
		clientFdToConf_[clientSocket] = it->second;
	}

	std::cout << "Client connected" << std::endl;
	return clientSocket;
}

int Server::initServerSockets()
{
	Server::epollFd_ = epoll_create(1);
	if (epollFd_ == -1)
		throw std::runtime_error(std::string("Epoll_create failed: ") + strerror(errno));

	std::set<int> uniquePorts;
	for (size_t i = 0; i < serverConfs_.size(); ++i)
	{
		const std::vector<int> &ports = serverConfs_[i].getPorts();
		for (size_t j = 0; j < ports.size(); ++j)
			uniquePorts.insert(ports[j]);
	}

	for (std::set<int>::iterator pit = uniquePorts.begin(); pit != uniquePorts.end(); ++pit)
	{
		int port = *pit;
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
		serverAddr.sin_port = htons(port);

		if (bind(s, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
		{
			std::ostringstream oss;
			oss << "Bind failed on port " << port << ": " << strerror(errno);
			throw std::runtime_error(oss.str());
		}

		if (listen(s, 128) < 0)
			throw std::runtime_error(std::string("Listen failed: ") + strerror(errno));

		makeSocketNonBlocking(s);
		addEpollEvent(s, EPOLLIN);
		listenSockets_.push_back(s);

		for (size_t idx = 0; idx < serverConfs_.size(); ++idx)
		{
			const std::vector<int> &ports = serverConfs_[idx].getPorts();
			if (std::find(ports.begin(), ports.end(), port) != ports.end())
			{
				listenFdToConf_[s] = idx;
				break;
			}
		}

		std::cout << "Listening on port " << port << std::endl;
	}

	return 0;
}

// ---------------- Handle Client Events ---------------------------

void Server::handleReadEvent(int clientFd)
{
	int bytesRead = read(clientFd, Server::buffer_, sizeof(Server::buffer_) - 1);
	if (bytesRead <= 0)
	{
		std::cout << "Client disconnected" << std::endl;
		close(clientFd);
		epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
		clientFdToConf_.erase(clientFd);
		return;
	}

	Server::buffer_[bytesRead] = '\0';
	std::cout << "Received: " << Server::buffer_ << std::endl;

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = clientFd;
	epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
}

void Server::handleSendEvent(int clientFd)
{
	size_t confIdx = 0;
	std::map<int, size_t>::iterator it = clientFdToConf_.find(clientFd);
	if (it != clientFdToConf_.end())
		confIdx = it->second;
	const ServerConf &conf = serverConfs_[confIdx];


	std::istringstream req(Server::buffer_);
	std::string method, uri, version;
	req >> method >> uri >> version;

	Location *loc = conf.findLocation(uri);

	std::string handlerRoot = conf.getRoot();
	std::string handlerIndex = conf.getIndex();

	if (loc)
	{
		if (!loc->root.empty())
			handlerRoot = loc->root;
		if (!loc->index.empty())
			handlerIndex = loc->index;
	}

	delete httpRequestHandler_;
	httpRequestHandler_ = new HttpRequestHandler(&conf);
	httpRequestHandler_->root = handlerRoot;
	httpRequestHandler_->index = handlerIndex;
	httpRequestHandler_->autoindex_ = conf.isAutoindexEnabled(uri);
	httpRequestHandler_->errorPages = conf.getErrorPages();

	std::cout << "Using root: " << httpRequestHandler_->root << ", index: " << httpRequestHandler_->index << std::endl;
	std::string response = httpRequestHandler_->parseRequest(std::string(buffer_));

	int bytesSent = send(clientFd, response.c_str(), response.length(), 0);

	if (bytesSent <= 0)
	{
		std::cerr << "Error: Send failed or connection closed" << std::endl;
		close(clientFd);
		epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
		clientFdToConf_.erase(clientFd);
		return;
	}

	std::cout << "Message sent to client." << std::endl;

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = clientFd;
	epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
}

bool Server::isRunning()
{
	return this->running_;
}

void Server::run()
{
	initServerSockets();
	signal(SIGINT, Server::handleSignal);
	signal(SIGTERM, Server::handleSignal);
	epoll_event eventsLocal[10];
	while (this->running_)
	{
		int numEvents = epoll_wait(epollFd_, eventsLocal, 10, 1000);
		for (int i = 0; i < numEvents; i++)
		{
			int fd = eventsLocal[i].data.fd;
			if (eventsLocal[i].events & EPOLLIN)
			{
				bool isListening = false;
				for (size_t k = 0; k < listenSockets_.size(); ++k)
				{
					if (fd == listenSockets_[k])
					{
						isListening = true;
						break;
					}
				}
				if (isListening)
				{
					int clientSocket = acceptClient(fd);
					std::cout << "New client connected: " << clientSocket << std::endl;
				}
				else
					handleReadEvent(fd);
			}
			if (eventsLocal[i].events & EPOLLOUT)
				handleSendEvent(fd);
		}
	}
}
