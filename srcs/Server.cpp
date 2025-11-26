#include "../include/webserv.h"

volatile bool Server::running_ = true;

void Server::handleSignal(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
	{
		Server::running_ = false;
	}
}

Server::Server(const std::vector<ServerConf> &serverConfs, char **envp) : epollFd_(-1), serverConfs_(serverConfs), envp_(envp), clientTimeout_(5)
{
	memset(this->buffer_, 0, sizeof(this->buffer_));
	memset(this->events_, 0, sizeof(this->events_));
	httpRequestHandler_ = new HttpRequestHandler();
	httpRequestHandler_->env_ = envp_;
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
	std::cout << "\033[33m" << "[" << getCurrentTime() << "] " << "Server stopped." << "\033[0m" << std::endl;
}

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
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
	if (clientSocket < 0)
		throw std::runtime_error(std::string("Accept failed: ") + strerror(errno));

	makeSocketNonBlocking(clientSocket);
	addEpollEvent(clientSocket, EPOLLIN);

	char ipStr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
	clientFdToIp_[clientSocket] = ipStr;

	std::map<int, size_t>::iterator it = listenFdToConf_.find(serverSocket);
	if (it != listenFdToConf_.end())
	{
		clientFdToConf_[clientSocket] = it->second;
	}
	std::map<int, int>::iterator pit = listenFdToPort_.find(serverSocket);
	if (pit != listenFdToPort_.end())
	{
		clientFdToPort_[clientSocket] = pit->second;
	}

	std::cout << "\033[32m" << "[" << getCurrentTime() << "] " << "Client connected" << "\033[0m" << std::endl;
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
		listenFdToPort_[s] = port;

		for (size_t idx = 0; idx < serverConfs_.size(); ++idx)
		{
			const std::vector<int> &portsVec = serverConfs_[idx].getPorts();
			if (std::find(portsVec.begin(), portsVec.end(), port) != portsVec.end())
			{
				listenFdToConf_[s] = idx;
				break;
			}
		}

		std::cout << "\033[34m" << "[" << getCurrentTime() << "] " << "Listening on port " << port << "\033[0m" << std::endl;
	}

	return 0;
}

void Server::checkRequestTimeouts()
{
	time_t now = time(NULL);
	
	for (std::map<int, time_t>::iterator it = clientLastActivity_.begin(); it != clientLastActivity_.end();)
	{
		int fd = it->first;
		time_t lastActivity = it->second;
		
		if (clientBuffers_.count(fd) > 0)
		{
			double elapsed = difftime(now, lastActivity);
			
			if (elapsed > clientTimeout_)
			{
				std::cout << "\033[91m" << "[" << getCurrentTime() << "] "
						  << "⚠️ Client " << fd << " timed out waiting for body (inactive for " 
						  << elapsed << "s)" << "\033[0m" << std::endl;
				
				std::string timeoutBody = "<html><body><h1>408 Request Timeout</h1>"
										  "<p>The server timed out waiting for the complete request.</p></body></html>";
				std::ostringstream timeoutResponse;
				timeoutResponse << "HTTP/1.1 408 Request Timeout\r\n"
							   << "Date: " << getCurrentTime() << "\r\n"
							   << "Server: WebServ\r\n"
							   << "Content-Length: " << timeoutBody.size() << "\r\n"
							   << "Content-Type: text/html\r\n"
							   << "Connection: close\r\n\r\n"
							   << timeoutBody;
				
				sendBuffers_[fd] = timeoutResponse.str();
				sendOffsets_[fd] = 0;
				clientBuffers_.erase(fd);
				
				struct epoll_event ev;
				ev.events = EPOLLOUT;
				ev.data.fd = fd;
				epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
				
				++it;
				continue;
			}
		}
		++it;
	}
}

void Server::checkTimeouts()
{
	time_t now = time(NULL);
	for (std::map<int, time_t>::iterator it = clientSendStart_.begin(); it != clientSendStart_.end();)
	{
		int fd = it->first;
		if (sendBuffers_.count(fd) && difftime(now, it->second) > clientTimeout_)
		{
			std::cout << "\033[33m" << "[" << getCurrentTime() << "] "
					  << "Client " << fd << " timed out on send" << "\033[0m" << std::endl;

			std::string timeoutBody = "<html><body><h1>408 Request Timeout</h1><p>The server closed this connection due to inactivity.</p></body></html>";
			std::ostringstream timeoutResponse;
			timeoutResponse << "HTTP/1.1 408 Request Timeout\r\n"
						   << "Date: " << getCurrentTime() << "\r\n"
						   << "Server: WebServ\r\n"
						   << "Content-Length: " << timeoutBody.size() << "\r\n"
						   << "Content-Type: text/html\r\n"
						   << "Connection: close\r\n"
						   << "\r\n"
						   << timeoutBody;

			sendBuffers_[fd] = timeoutResponse.str();
			sendOffsets_[fd] = 0;

			struct epoll_event ev;
			ev.events = EPOLLOUT;
			ev.data.fd = fd;
			epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);

			clientSendStart_.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

void Server::handleReadEvent(int clientFd)
{
    time_t currentTime = time(NULL);
    int bytesRead = read(clientFd, buffer_, sizeof(buffer_) - 1);
    
    if (bytesRead <= 0)
    {
        if (clientBuffers_.count(clientFd) > 0)
        {
            std::string& partial = clientBuffers_[clientFd];
            
            size_t headerEnd = partial.find("\r\n\r\n");
            if (headerEnd != std::string::npos)
            {
                size_t clPos = partial.find("Content-Length:");
                if (clPos == std::string::npos)
                    clPos = partial.find("content-length:");
                
                if (clPos != std::string::npos)
                {
                    size_t clStart = partial.find(":", clPos) + 1;
                    size_t clEnd = partial.find("\r\n", clStart);
                    std::string clStr = partial.substr(clStart, clEnd - clStart);
                    
                    size_t expectedLength = 0;
                    std::istringstream iss(clStr);
                    iss >> expectedLength;
                    
                    size_t bodyStart = headerEnd + 4;
                    size_t bodyReceived = partial.size() - bodyStart;
                    
                    if (bodyReceived < expectedLength)
                    {
                        std::cout << "\033[91m" << "[" << getCurrentTime() << "] "
                                  << "⚠️ Client disconnected with incomplete body: "
                                  << bodyReceived << "/" << expectedLength << " bytes"
                                  << "\033[0m" << std::endl;
                    }
                }
            }
        }
        
        std::cout << "\033[33m" << "[" << getCurrentTime() << "] " 
                  << "Client disconnected" << "\033[0m" << std::endl;
        close(clientFd);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
        clientFdToConf_.erase(clientFd);
        clientFdToPort_.erase(clientFd);
        clientFdToIp_.erase(clientFd);
        clientLastActivity_.erase(clientFd);
        clientBuffers_.erase(clientFd);
        return;
    }
    
    if (bytesRead > 0)
    {
        clientLastActivity_[clientFd] = currentTime;
    }
    
    buffer_[bytesRead] = '\0';
    clientBuffers_[clientFd].append(buffer_, bytesRead);
    
    std::string& fullRequest = clientBuffers_[clientFd];
	std::cout << "full request so far:\n" << fullRequest << std::endl;
    size_t headerEnd = fullRequest.find("\r\n\r\n");
    
    if (headerEnd == std::string::npos) {
        std::cout << "\033[93m" << "[" << getCurrentTime() << "] " 
                  << "Waiting for complete headers..." << "\033[0m" << std::endl;
        return;
    }
    
    std::istringstream preReq(fullRequest);
    std::string preMethod, preUri;
    preReq >> preMethod >> preUri;
    std::string hostHeader = extractHost(fullRequest);
    int localPort = clientFdToPort_[clientFd];
    size_t confIdx = clientFdToConf_[clientFd];
    ServerConf &defaultConf = serverConfs_[confIdx];
    
    ServerConf *selectedConf = selectServer(hostHeader, localPort, serverConfs_);
    if (!selectedConf)
        selectedConf = &defaultConf;
    
    if (preMethod == "POST" || preMethod == "PUT")
    {
        size_t clPos = fullRequest.find("Content-Length:");
        if (clPos == std::string::npos)
            clPos = fullRequest.find("content-length:");
        
        if (clPos != std::string::npos && clPos < headerEnd)
        {
            size_t clStart = fullRequest.find(":", clPos) + 1;
            size_t clEnd = fullRequest.find("\r\n", clStart);
            if (clEnd == std::string::npos) 
                clEnd = fullRequest.find("\n", clStart);
            
            std::string clStr = fullRequest.substr(clStart, clEnd - clStart);
            size_t first = clStr.find_first_not_of(" \t\r\n");
            size_t last = clStr.find_last_not_of(" \t\r\n");
            
            if (first != std::string::npos)
            {
                clStr = clStr.substr(first, last - first + 1);
                size_t announcedLength = 0;
                std::istringstream iss(clStr);
                iss >> announcedLength;
                
                size_t maxBodySize = selectedConf->getClientMaxBodySize();
                Location *loc = selectedConf->findLocation(preUri);
                if (loc && loc->client_max_body_size > 0)
                    maxBodySize = loc->client_max_body_size;
                
                if (maxBodySize > 0 && announcedLength >= maxBodySize)
                {
                    std::cout << "\033[91m" << "[" << getCurrentTime() << "] "
                              << "❌ Content-Length " << announcedLength 
                              << " exceeds limit " << maxBodySize 
                              << " (REJECTED BEFORE BODY RECEPTION)" << "\033[0m" << std::endl;
                    
                    std::string body413 = "<html><body><h1>413 Payload Too Large</h1>"
                                          "<p>Request body exceeds maximum allowed size.</p></body></html>";
                    std::ostringstream resp413;
                    resp413 << "HTTP/1.1 413 Payload Too Large\r\n"
                            << "Date: " << getCurrentTime() << "\r\n"
                            << "Server: WebServ\r\n"
                            << "Content-Length: " << body413.size() << "\r\n"
                            << "Content-Type: text/html\r\n"
                            << "Connection: close\r\n\r\n"
                            << body413;
                    std::string response = resp413.str();
                    send(clientFd, response.c_str(), response.size(), 0);
                    close(clientFd);
                    epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
                    clientFdToConf_.erase(clientFd);
                    clientFdToPort_.erase(clientFd);
                    clientFdToIp_.erase(clientFd);
                    clientLastActivity_.erase(clientFd);
                    clientBuffers_.erase(clientFd);
                    return;
                }
                
                size_t bodyStart = headerEnd + 4;
                size_t bodyReceived = fullRequest.size() - bodyStart;
                
                if (announcedLength > 0 && bodyReceived < announcedLength)
                {
                    std::cout << "\033[93m" << "[" << getCurrentTime() << "] " 
                              << "⏳ Waiting for complete body: " << bodyReceived << "/" 
                              << announcedLength << " bytes" << "\033[0m" << std::endl;
                    return;
                }
            }
        }
    }

    bool hasContentLength = (fullRequest.find("Content-Length:") != std::string::npos) 
                         || (fullRequest.find("content-length:") != std::string::npos);
    
    if (!hasContentLength && (preMethod == "POST" || preMethod == "PUT"))
    {
        size_t tePos = fullRequest.find("Transfer-Encoding:");
        if (tePos == std::string::npos)
            tePos = fullRequest.find("transfer-encoding:");
        
        bool isChunked = false;
        if (tePos != std::string::npos && tePos < headerEnd) {
            size_t valueStart = fullRequest.find(":", tePos) + 1;
            size_t valueEnd = fullRequest.find("\r\n", valueStart);
            std::string teValue = fullRequest.substr(valueStart, valueEnd - valueStart);
            
            size_t first = teValue.find_first_not_of(" \t\r\n");
            size_t last = teValue.find_last_not_of(" \t\r\n");
            if (first != std::string::npos)
                teValue = teValue.substr(first, last - first + 1);
            
            isChunked = (teValue == "chunked");
        }
        
        if (isChunked) {
            std::cout << "\033[95m[" << getCurrentTime() << "] "
                      << "🔄 Chunked request detected" << "\033[0m" << std::endl;
            
            size_t pos = headerEnd + 4;
            size_t totalBodySize = 0;
            bool isComplete = false;
            
            while (pos < fullRequest.size()) {
                size_t lineEnd = fullRequest.find("\r\n", pos);
                if (lineEnd == std::string::npos)
                    break;
                
                std::string sizeLine = fullRequest.substr(pos, lineEnd - pos);
                size_t semiColon = sizeLine.find(';');
                if (semiColon != std::string::npos)
                    sizeLine = sizeLine.substr(0, semiColon);
                
                size_t first = sizeLine.find_first_not_of(" \t\r\n");
                size_t last = sizeLine.find_last_not_of(" \t\r\n");
                if (first != std::string::npos)
                    sizeLine = sizeLine.substr(first, last - first + 1);
                
                int chunkSize = 0;
                for (size_t i = 0; i < sizeLine.length(); i++) {
                    char c = sizeLine[i];
                    if (c >= '0' && c <= '9')
                        chunkSize = chunkSize * 16 + (c - '0');
                    else if (c >= 'a' && c <= 'f')
                        chunkSize = chunkSize * 16 + (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F')
                        chunkSize = chunkSize * 16 + (c - 'A' + 10);
                    else
                        break;
                }
                
                pos = lineEnd + 2;
                
                if (chunkSize == 0) {
                    if (pos + 2 <= fullRequest.size() && 
                        fullRequest.substr(pos, 2) == "\r\n") {
                        isComplete = true;
                        std::cout << "\033[92m[" << getCurrentTime() << "] "
                                  << "✅ Chunked complete: " << totalBodySize << " bytes"
                                  << "\033[0m" << std::endl;
                    }
                    break;
                }
                
                totalBodySize += chunkSize;
                
                size_t maxBodySize = selectedConf->getClientMaxBodySize();
                Location *loc = selectedConf->findLocation(preUri);
                if (loc && loc->client_max_body_size > 0)
                    maxBodySize = loc->client_max_body_size;
                
                if (maxBodySize > 0 && totalBodySize >= maxBodySize) {
                    std::cout << "\033[91m[" << getCurrentTime() << "] "
                              << "❌ Chunked body exceeds limit: " << totalBodySize 
                              << " > " << maxBodySize << "\033[0m" << std::endl;
                    
                    std::string body413 = "<html><body><h1>413 Payload Too Large</h1>"
                                          "<p>Chunked request exceeds limit.</p></body></html>";
                    std::ostringstream resp413;
                    resp413 << "HTTP/1.1 413 Payload Too Large\r\n"
                            << "Date: " << getCurrentTime() << "\r\n"
                            << "Server: WebServ\r\n"
                            << "Content-Length: " << body413.size() << "\r\n"
                            << "Content-Type: text/html\r\n"
                            << "Connection: close\r\n\r\n"
                            << body413;
                    
                    std::string response = resp413.str();
                    send(clientFd, response.c_str(), response.size(), 0);
                    close(clientFd);
                    epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
                    clientFdToConf_.erase(clientFd);
                    clientFdToPort_.erase(clientFd);
                    clientFdToIp_.erase(clientFd);
                    clientLastActivity_.erase(clientFd);
                    clientBuffers_.erase(clientFd);
                    return;
                }
                
                if (pos + chunkSize + 2 > fullRequest.size())
                    break;
                
                pos += chunkSize + 2;
            }
            
            if (!isComplete) {
                std::cout << "\033[93m[" << getCurrentTime() << "] "
                          << "⏳ Waiting for complete chunked body (current: " 
                          << totalBodySize << " bytes)..." << "\033[0m" << std::endl;
                return;
            }
        }
    }
    
    std::cout << "\033[92m[" << getCurrentTime() << "] " 
              << "✅ Complete request received (" << fullRequest.size() 
              << " bytes)" << "\033[0m" << std::endl;
    
    const ServerConf &conf = *selectedConf;
    
    std::istringstream req(fullRequest);
    std::string method, uri, version;
    req >> method >> uri >> version;
    Location *loc = conf.findLocation(uri);
    
    std::string handlerRoot = conf.getRoot();
    std::string handlerIndex = conf.getIndex();
    std::map<int, std::string> handlerRedirects;
    if (loc)
    {
        if (!loc->root.empty())
            handlerRoot = loc->root;
        if (!loc->index.empty())
            handlerIndex = loc->index;
        if (!loc->redirects.empty())
            handlerRedirects = loc->redirects;
    }
    
    delete httpRequestHandler_;
    httpRequestHandler_ = new HttpRequestHandler(&conf);
    httpRequestHandler_->setClientIp(clientFdToIp_[clientFd]);
    httpRequestHandler_->env_ = envp_;
    httpRequestHandler_->root = handlerRoot;
    httpRequestHandler_->index = handlerIndex;
    httpRequestHandler_->redirects = handlerRedirects;
    httpRequestHandler_->autoindex_ = conf.isAutoindexEnabled(uri);
    httpRequestHandler_->errorPages = conf.getErrorPages();
    httpRequestHandler_->server_name_ = conf.getHost();
    std::string response = httpRequestHandler_->parseRequest(fullRequest);
    
    sendBuffers_[clientFd] = response;
    sendOffsets_[clientFd] = 0;
    clientSendStart_[clientFd] = time(NULL);
    clientBuffers_.erase(clientFd);
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = clientFd;
    epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
}

void Server::handleSendEvent(int clientFd)
{
	std::string &buf = sendBuffers_[clientFd];
	size_t &offset = sendOffsets_[clientFd];

	ssize_t bytesSent = send(clientFd, buf.c_str() + offset, buf.size() - offset, 0);
	if (bytesSent < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "\033[91m" << "[" << getCurrentTime() << "] "
					  << "Error: Send failed" << "\033[0m" << std::endl;
			close(clientFd);
			epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
			clientFdToConf_.erase(clientFd);
			clientFdToPort_.erase(clientFd);
			clientFdToIp_.erase(clientFd);
			clientLastActivity_.erase(clientFd);
			sendBuffers_.erase(clientFd);
			sendOffsets_.erase(clientFd);
			clientSendStart_.erase(clientFd);
		}
		return;
	}
	if (bytesSent > 0)
		clientLastActivity_[clientFd] = time(NULL);
	offset += bytesSent;

	if (offset >= buf.size())
	{
		bool isTimeoutResponse = (buf.find("408 Request Timeout") != std::string::npos);

		sendBuffers_.erase(clientFd);
		sendOffsets_.erase(clientFd);
		clientSendStart_.erase(clientFd);
		clientBuffers_.erase(clientFd);

		if (isTimeoutResponse)
		{
			close(clientFd);
			epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
			clientFdToConf_.erase(clientFd);
			clientFdToPort_.erase(clientFd);
			clientFdToIp_.erase(clientFd);
			clientLastActivity_.erase(clientFd);
		}
		else
		{
			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = clientFd;
			epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
		}
	}
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
					clientLastActivity_[clientSocket] = time(NULL);
					std::cout << "\033[32m" << "[" << getCurrentTime() << "] "
							  << "New client connected: " << clientSocket << "\033[0m" << std::endl;
				}
				else
					handleReadEvent(fd);
			}
			if (eventsLocal[i].events & EPOLLOUT)
				handleSendEvent(fd);
		}

		checkTimeouts();
		checkRequestTimeouts();
	}
}