// Dans handleReadEvent(), remplace la section Content-Length check (~ligne 340)

void Server::handleReadEvent(int clientFd)
{
    // ✅ AJOUT : Si on doit ignorer des données (413 déjà envoyé)
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
                          << "✅ All excess data drained, ready to send 413"
                          << "\033[0m" << std::endl;
                clientBytesToIgnore_.erase(clientFd);
            }
        }
        return;
    }

    time_t currentTime = time(NULL);
    int bytesRead = read(clientFd, buffer_, sizeof(buffer_) - 1);
    
    if (bytesRead <= 0)
    {
        // ... ton code existant de gestion de déconnexion ...
        std::cout << "\033[33m" << "[" << getCurrentTime() << "] " 
                  << "Client disconnected" << "\033[0m" << std::endl;
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
                
                // ✅ CHECK AMÉLIORÉ : Détecte le dépassement AVANT de recevoir le body
                if (maxBodySize > 0 && announcedLength > maxBodySize)
                {
                    std::cout << "\033[91m" << "[" << getCurrentTime() << "] "
                              << "❌ Content-Length " << announcedLength 
                              << " exceeds limit " << maxBodySize 
                              << " → Sending 413 and draining body" << "\033[0m" << std::endl;

                    // ✅ Calcule combien de bytes restent à ignorer
                    size_t bodyStart = headerEnd + 4;
                    size_t bodyReceived = fullRequest.size() - bodyStart;
                    size_t remainingToIgnore = (announcedLength > bodyReceived) 
                                               ? (announcedLength - bodyReceived) 
                                               : 0;
                    
                    if (remainingToIgnore > 0)
                    {
                        clientBytesToIgnore_[clientFd] = remainingToIgnore;
                        std::cout << "\033[93m[" << getCurrentTime() << "] "
                                  << "🗑️ Will ignore " << remainingToIgnore 
                                  << " more bytes before sending 413"
                                  << "\033[0m" << std::endl;
                        return; // Continue à lire pour vider
                    }

                    // ✅ Si tout le body est déjà reçu, envoie 413 immédiatement
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
                
                // ... reste du code existant pour attendre le body complet ...
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

    // ... reste de ton code existant pour chunked, parsing, etc. ...
}