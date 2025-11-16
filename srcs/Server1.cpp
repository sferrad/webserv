#include "../include/webserv1.h"

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

// ============================================
// GARDE SEULEMENT CETTE VERSION (la nouvelle)
// ============================================
void Server::handleReadEvent(int clientFd)
{
    int bytesRead = recv(clientFd, Server::buffer_, sizeof(Server::buffer_) - 1, 0);
    if (bytesRead <= 0)
    {
        std::cout << "Client disconnected" << std::endl;
        close(clientFd);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
        clientFdToConf_.erase(clientFd);
        clientBuffers_.erase(clientFd);  // ← Important !
        return;
    }

    Server::buffer_[bytesRead] = '\0';
    
    // Accumuler les données
    clientBuffers_[clientFd].append(Server::buffer_, bytesRead);
    
    std::string& fullRequest = clientBuffers_[clientFd];
    
    // Vérifier si on a tous les headers
    size_t headerEnd = fullRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        std::cout << "Waiting for complete headers..." << std::endl;
        return;
    }
    
    // Extraire Content-Length si POST/PUT
    size_t contentLength = 0;
    if (fullRequest.find("POST") == 0 || fullRequest.find("PUT") == 0) {
        size_t clPos = fullRequest.find("Content-Length:");
        if (clPos == std::string::npos) {
            clPos = fullRequest.find("content-length:");
        }
        
        if (clPos != std::string::npos) {
            size_t clStart = fullRequest.find(":", clPos) + 1;
            size_t clEnd = fullRequest.find("\r\n", clStart);
            if (clEnd == std::string::npos) clEnd = fullRequest.find("\n", clStart);
            
            std::string clStr = fullRequest.substr(clStart, clEnd - clStart);
            size_t first = clStr.find_first_not_of(" \t\r\n");
            size_t last = clStr.find_last_not_of(" \t\r\n");
            if (first != std::string::npos) {
                clStr = clStr.substr(first, last - first + 1);
                std::istringstream iss(clStr);
                iss >> contentLength;
            }
        }
    }
    
    // Vérifier si on a tout le body
    size_t bodyStart = headerEnd + 4;
	size_t bodyReceived = fullRequest.size() - bodyStart;
	
	// 🔍 Détecter si c'est une requête chunked
	bool isChunked = (fullRequest.find("Transfer-Encoding: chunked") != std::string::npos ||
	                  fullRequest.find("transfer-encoding: chunked") != std::string::npos);
	
	if (isChunked) {
	    // Pour chunked, vérifier si on a le dernier chunk "0\r\n\r\n"
	    std::string body = fullRequest.substr(bodyStart);
	
	    if (body.find("0\r\n\r\n") == std::string::npos) {
	        std::cout << "\033[93m" << "⏳ Chunked body incomplete (waiting for 0\\r\\n\\r\\n)..." << "\033[0m" << std::endl;
	        return;  // Attendre le prochain recv()
	    }
	
	    std::cout << "\033[92m" << "✅ Complete chunked body received!" << "\033[0m" << std::endl;
	} else if (contentLength > 0 && bodyReceived < contentLength) {
	    // Pour les requêtes normales avec Content-Length
	    std::cout << "Waiting for complete body: " << bodyReceived << "/" << contentLength << " bytes" << std::endl;
	    return;
	}
	
	std::cout << "✅ Received complete request (" << fullRequest.size() << " bytes)" << std::endl;

    
    // Passer en mode EPOLLOUT pour envoyer la réponse
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = clientFd;
    epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
}

// ============================================
// AJOUTE/MODIFIE CETTE FONCTION
// ============================================
void Server::handleSendEvent(int clientFd)
{
    // Récupérer la config du serveur
    size_t confIdx = 0;
    std::map<int, size_t>::iterator it = clientFdToConf_.find(clientFd);
    if (it != clientFdToConf_.end())
        confIdx = it->second;
    const ServerConf &conf = serverConfs_[confIdx];

    // Récupérer la requête complète depuis le buffer du client
    std::string fullRequest = clientBuffers_[clientFd];
    
    // Parser la première ligne
    std::istringstream req(fullRequest);
    std::string method, uri, version;
    req >> method >> uri >> version;

    // Trouver la location correspondante
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

    // Créer le handler avec la bonne config
    delete httpRequestHandler_;
    httpRequestHandler_ = new HttpRequestHandler(&conf);
    httpRequestHandler_->root = handlerRoot;
    httpRequestHandler_->index = handlerIndex;
    httpRequestHandler_->errorPages = conf.getErrorPages();
    httpRequestHandler_->clientMaxBodySize_ = conf.getClientMaxBodySize();

    std::cout << "Using root: " << httpRequestHandler_->root << ", index: " << httpRequestHandler_->index << std::endl;
    
    // Parser la requête et générer la réponse
    std::string response = httpRequestHandler_->parseRequest(fullRequest);

    // Envoyer la réponse
    int bytesSent = send(clientFd, response.c_str(), response.length(), 0);

    if (bytesSent <= 0)
    {
        std::cerr << "Error: Send failed or connection closed" << std::endl;
        close(clientFd);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
        clientFdToConf_.erase(clientFd);
        clientBuffers_.erase(clientFd);  // ← Nettoyer le buffer
        return;
    }

    std::cout << "Message sent to client." << std::endl;

    // Nettoyer le buffer après envoi
    clientBuffers_.erase(clientFd);

    // Repasser en mode lecture seulement
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
