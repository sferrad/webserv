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
	for (std::map<int, size_t>::iterator it = clientFdToConf_.begin(); it != clientFdToConf_.end(); ++it)
	{
		int clientFd = it->first;
		if (clientFd != -1)
		{
			epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
			close(clientFd);
		}
	}

	for (std::map<int, CgiState>::iterator it = cgiFdToState_.begin(); it != cgiFdToState_.end(); ++it)
	{
		int cgiFd = it->first;
		pid_t pid = it->second.pid;
		
		if (cgiFd != -1)
		{
			epoll_ctl(epollFd_, EPOLL_CTL_DEL, cgiFd, NULL);
			close(cgiFd);
		}
		
		if (pid > 0)
		{
			kill(pid, SIGTERM);
			usleep(1000);
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
		}
	}
	cgiFdToState_.clear();
	clientFdToCgiFd_.clear();

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

	try
	{
		makeSocketNonBlocking(clientSocket);
		addEpollEvent(clientSocket, EPOLLIN);
	}
	catch (const std::exception &e)
	{
		close(clientSocket);
		throw;
	}

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
		{
			close(s);
			throw std::runtime_error(std::string("Setsockopt failed: ") + strerror(errno));
		}

		struct sockaddr_in serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(port);

		if (bind(s, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
		{
			close(s);
			std::ostringstream oss;
			oss << "Bind failed on port " << port << ": " << strerror(errno);
			throw std::runtime_error(oss.str());
		}

		if (listen(s, 128) < 0)
		{
			close(s);
			throw std::runtime_error(std::string("Listen failed: ") + strerror(errno));
		}

		try
		{
			makeSocketNonBlocking(s);
			addEpollEvent(s, EPOLLIN);
		}
		catch (const std::exception &e)
		{
			close(s);
			throw;
		}
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
			time_t elapsed = now - lastActivity;
			
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
		if (sendBuffers_.count(fd) && (now - it->second) > clientTimeout_)
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

void Server::checkCgiTimeouts()
{
	time_t now = time(NULL);
	for (std::map<int, CgiState>::iterator it = cgiFdToState_.begin(); it != cgiFdToState_.end();)
	{
		if (now - it->second.startTime > CGI_TIMEOUT)
		{
			int cgiFd = it->first;
			int clientFd = it->second.clientFd;
			pid_t pid = it->second.pid;
			
			std::cout << "\033[31m[" << getCurrentTime() << "] "
					  << "⚠️ CGI timeout for client " << clientFd << ", killing PID " << pid << "\033[0m" << std::endl;
			
			kill(pid, SIGTERM);
			usleep(1000);
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			
			epoll_ctl(epollFd_, EPOLL_CTL_DEL, cgiFd, NULL);
			close(cgiFd);

			std::string body504;
			std::map<int, std::string> &errorPages = it->second.errorPages;
			std::string root = it->second.root;
			
			if (errorPages.count(504))
			{
				std::string page = errorPages[504];
				std::string errPath;
				if (page.rfind("./", 0) == 0)
				{
					std::string rel = page.substr(2);
					if (!rel.empty() && rel[0] == '/')
						errPath = root + rel;
					else
						errPath = root + "/" + rel;
				}
				else if (!page.empty() && page[0] == '/')
					errPath = root + page;
				else
					errPath = root + "/" + page;
				
				std::ifstream ferr(errPath.c_str());
				if (ferr)
				{
					std::ostringstream ss;
					ss << ferr.rdbuf();
					body504 = ss.str();
				}
			}
			
			if (body504.empty())
			{
				std::ostringstream fallback;
				fallback << "./www/error/504.html";
				std::ifstream ferr2(fallback.str().c_str());
				if (ferr2)
				{
					std::ostringstream ss;
					ss << ferr2.rdbuf();
					body504 = ss.str();
				}
			}

			if (body504.empty())
				body504 = "<html><body><h1>504 Gateway Timeout</h1></body></html>";

			std::ostringstream resp;
			resp << "HTTP/1.1 504 Gateway Timeout\r\n"
				 << "Content-Length: " << body504.size() << "\r\n"
				 << "Content-Type: text/html\r\n"
				 << "Connection: close\r\n\r\n"
				 << body504;
			
			sendBuffers_[clientFd] = resp.str();
			sendOffsets_[clientFd] = 0;
			clientSendStart_[clientFd] = time(NULL);
			
			struct epoll_event ev;
			ev.events = EPOLLOUT;
			ev.data.fd = clientFd;
			epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
			
			clientFdToCgiFd_.erase(clientFd);
			cgiFdToState_.erase(it++);
		}
		else
			++it;
	}
}

void Server::handleReadEvent(int clientFd)
{
    if (clientBytesToIgnore_.count(clientFd) > 0)
    {
        int bytesRead = read(clientFd, buffer_, sizeof(buffer_) - 1);
        if (bytesRead > 0)
        {
            clientBytesToIgnore_[clientFd] -= std::min((size_t)bytesRead, clientBytesToIgnore_[clientFd]);
            std::cout << "\033[93m[" << getCurrentTime() << "] "
                      << "🗑️ Ignoring " << bytesRead << " bytes (413 sent), "
                      << clientBytesToIgnore_[clientFd] << " remaining"
                      << "\033[0m" << std::endl;
            
            if (clientBytesToIgnore_[clientFd] == 0)
            {
                std::cout << "\033[92m[" << getCurrentTime() << "] "
                          << "✅ All excess data drained, sending 413 now"
                          << "\033[0m" << std::endl;
                clientBytesToIgnore_.erase(clientFd);
                
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

                sendBuffers_[clientFd] = resp413.str();
                sendOffsets_[clientFd] = 0;
                clientSendStart_[clientFd] = time(NULL);

                struct epoll_event ev;
                ev.events = EPOLLOUT;
                ev.data.fd = clientFd;
                epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
            }
        }
        return;
    }

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
        
        if (clientFdToCgiFd_.count(clientFd))
        {
            int cgiFd = clientFdToCgiFd_[clientFd];
            pid_t pid = cgiFdToState_[cgiFd].pid;
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            epoll_ctl(epollFd_, EPOLL_CTL_DEL, cgiFd, NULL);
            close(cgiFd);
            cgiFdToState_.erase(cgiFd);
            clientFdToCgiFd_.erase(clientFd);
        }

        close(clientFd);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, NULL);
        clientFdToConf_.erase(clientFd);
        clientFdToPort_.erase(clientFd);
        clientFdToIp_.erase(clientFd);
        clientLastActivity_.erase(clientFd);
        clientBuffers_.erase(clientFd);
        clientBytesToIgnore_.erase(clientFd);
        return;
    }
    
    if (bytesRead > 0)
        clientLastActivity_[clientFd] = currentTime;
    
    buffer_[bytesRead] = '\0';
    clientBuffers_[clientFd].append(buffer_, bytesRead);
    
    std::string& fullRequest = clientBuffers_[clientFd];
        if (fullRequest.size() > 1048576 && fullRequest.size() % 10485760 == 0) {
        std::cout << "\033[36m[" << getCurrentTime() << "] "
                  << "📥 Upload progress: " << (fullRequest.size() / 1048576) << " MB received"
                  << "\033[0m" << std::endl;
    }
    
    size_t headerEnd = fullRequest.find("\r\n\r\n");
    
    if (headerEnd == std::string::npos) {
        std::cout << "\033[93m" << "[" << getCurrentTime() << "] " 
                  << "Waiting for complete headers..." << "\033[0m" << std::endl;
        return;
    }
    
    std::istringstream preReq(fullRequest);
    std::string preMethod, preUri;
    preReq >> preMethod >> preUri;
    
    std::cout << "\033[35m[DEBUG] Server received request: " << preMethod << " " << preUri << "\033[0m" << std::endl;

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
                
                if (maxBodySize > 0 && announcedLength > maxBodySize)
                {
                    std::cout << "\033[91m" << "[" << getCurrentTime() << "] "
                              << "❌ Content-Length " << announcedLength 
                              << " exceeds limit " << maxBodySize 
                              << " → Draining body before 413" << "\033[0m" << std::endl;

                    size_t bodyStart = headerEnd + 4;
                    size_t bodyReceived = fullRequest.size() - bodyStart;
                    size_t remainingToIgnore = (announcedLength > bodyReceived) 
                                               ? (announcedLength - bodyReceived) 
                                               : 0;
                    
                    if (remainingToIgnore > 0)
                    {
                        clientBytesToIgnore_[clientFd] = remainingToIgnore;
                        clientBuffers_.erase(clientFd);
                        std::cout << "\033[93m[" << getCurrentTime() << "] "
                                  << "🗑️ Will ignore " << remainingToIgnore 
                                  << " more bytes before sending 413"
                                  << "\033[0m" << std::endl;
                        return;
                    }

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

                    sendBuffers_[clientFd] = resp413.str();
                    sendOffsets_[clientFd] = 0;
                    clientSendStart_[clientFd] = time(NULL);
                    clientBuffers_.erase(clientFd);

                    struct epoll_event ev;
                    ev.events = EPOLLOUT;
                    ev.data.fd = clientFd;
                    epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);

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
        tePos = (tePos == std::string::npos) ? fullRequest.find("transfer-encoding:") : tePos;
        
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
                      << "📄 Chunked request detected" << "\033[0m" << std::endl;
            
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
                
                if (maxBodySize > 0 && totalBodySize > maxBodySize) {
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
                    sendBuffers_[clientFd] = resp413.str();
                    sendOffsets_[clientFd] = 0;
                    clientSendStart_[clientFd] = time(NULL);
                    clientBuffers_.erase(clientFd);

                    struct epoll_event ev;
                    ev.events = EPOLLOUT;
                    ev.data.fd = clientFd;
                    epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);

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
		if (!loc->errorPages_.empty())
			httpRequestHandler_->errorPages = loc->errorPages_;
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
    
    if (response == "CGI_PENDING")
    {
        const CgiExecutionInfo& info = httpRequestHandler_->getCgiInfo();
        CgiState state;
        state.pid = info.pid;
        state.pipeFd = info.pipeFd;
        state.clientFd = clientFd;
        state.startTime = time(NULL);
        state.errorPages = httpRequestHandler_->errorPages;
        state.root = httpRequestHandler_->root;
        
        cgiFdToState_[info.pipeFd] = state;
        clientFdToCgiFd_[clientFd] = info.pipeFd;
        
        makeSocketNonBlocking(info.pipeFd);
        addEpollEvent(info.pipeFd, EPOLLIN);
        
        std::cout << "\033[36m[" << getCurrentTime() << "] "
                  << "Registered async CGI (PID: " << info.pid 
                  << ", Pipe: " << info.pipeFd << ") for client " << clientFd << "\033[0m" << std::endl;
        
        clientBuffers_.erase(clientFd);
        return;
    }

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
        bool is413Response = (buf.find("413 Payload Too Large") != std::string::npos);

        sendBuffers_.erase(clientFd);
        sendOffsets_.erase(clientFd);
        clientSendStart_.erase(clientFd);
        clientBuffers_.erase(clientFd);

        if (isTimeoutResponse || is413Response)
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

void Server::handleCgiReadEvent(int cgiFd)
{
	char buf[4096];
	ssize_t bytesRead = read(cgiFd, buf, sizeof(buf));
	
	if (bytesRead > 0)
	{
		cgiFdToState_[cgiFd].responseBuffer.append(buf, bytesRead);
	}
	else
	{
		CgiState state = cgiFdToState_[cgiFd];
		int clientFd = state.clientFd;
		pid_t pid = state.pid;
		
		epoll_ctl(epollFd_, EPOLL_CTL_DEL, cgiFd, NULL);
		close(cgiFd);
		
		int status;
		waitpid(pid, &status, 0);
		
		std::string cgiOutput = state.responseBuffer;
		
		std::string body = cgiOutput;
		std::string headers = "";
		std::string statusLine = "HTTP/1.1 200 OK";
		
		size_t headerEnd = cgiOutput.find("\r\n\r\n");
		size_t delimiterSize = 4;
		if (headerEnd == std::string::npos) {
			headerEnd = cgiOutput.find("\n\n");
			delimiterSize = 2;
		}
			
		if (headerEnd != std::string::npos) {
			std::string headerPart = cgiOutput.substr(0, headerEnd);
			body = cgiOutput.substr(headerEnd + delimiterSize);
			
			std::istringstream iss(headerPart);
			std::string line;
			while (std::getline(iss, line)) {
				if (!line.empty() && line[line.size()-1] == '\r')
					line.erase(line.size()-1);
				if (line.empty()) continue;
				
				if (line.compare(0, 7, "Status:") == 0) {
					std::string status = line.substr(7);
					size_t first = status.find_first_not_of(" \t");
					if (first != std::string::npos)
						status = status.substr(first);
					statusLine = "HTTP/1.1 " + status;
				}
				else {
					headers += line + "\r\n";
				}
			}
		}

		std::ostringstream resp;
		resp << statusLine << "\r\n"
			 << "Date: " << getCurrentTime() << "\r\n"
			 << "Server: WebServ\r\n"
			 << headers
			 << "Content-Length: " << body.size() << "\r\n"
			 << "Connection: close\r\n\r\n"
			 << body;
		
		sendBuffers_[clientFd] = resp.str();
		sendOffsets_[clientFd] = 0;
		clientSendStart_[clientFd] = time(NULL);
		
		struct epoll_event ev;
		ev.events = EPOLLOUT;
		ev.data.fd = clientFd;
		epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &ev);
		
		clientFdToCgiFd_.erase(clientFd);
		cgiFdToState_.erase(cgiFd);
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
			
			if (cgiFdToState_.count(fd))
			{
				if (eventsLocal[i].events & (EPOLLIN | EPOLLHUP))
					handleCgiReadEvent(fd);
				continue;
			}

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
		checkCgiTimeouts();
	}
}