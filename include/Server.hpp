#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.h"

class HttpRequestHandler;
class ServerConf;

struct CgiState {
	pid_t pid;
	int clientFd;
	int pipeFd;
	time_t startTime;
	std::string responseBuffer;
	std::map<int, std::string> errorPages;
	std::string root;
};

class Server
{
private:
	std::vector<int> listenSockets_;
	int epollFd_;
	
	std::map<int, time_t> clientLastActivity_;
	std::map<int, time_t> clientSendStart_;

	std::string host_;
	std::vector<ServerConf> serverConfs_;
	std::map<int, size_t> listenFdToConf_;
	std::map<int, size_t> clientFdToConf_;
	std::map<int, std::string> clientBuffers_;
	std::map<int, std::string> sendBuffers_;
	std::map<int, size_t> sendOffsets_;

	static const int CGI_TIMEOUT = 15;

	std::map<int, int> listenFdToPort_;
	std::map<int, int> clientFdToPort_;
	std::map<int, std::string> clientFdToIp_;

	std::map<int, bool> clientsToClose_;
	std::map<int, size_t> clientBytesToIgnore_;

	std::map<int, CgiState> cgiFdToState_;
	std::map<int, int> clientFdToCgiFd_;

	char **envp_;
	char buffer_[1048576]; // 1MB buffer for faster uploads
	struct epoll_event events_[10];
	HttpRequestHandler *httpRequestHandler_;

	void checkRequestTimeouts();
	int makeSocketNonBlocking(int fd);
	int acceptClient(int serverSocket);
	void addEpollEvent(int fd, uint32_t events);
	void checkTimeouts();
	void checkCgiTimeouts();
	int initServerSockets();
	void HandleClient(int clientFd);
	void handleReadEvent(int clientFd);
	void handleSendEvent(int clientFd);
	void handleCgiReadEvent(int cgiFd);

	// Fonctions auxiliaires pour handleReadEvent
	bool handleIgnoredBytes(int clientFd);
	void handleClientDisconnect(int clientFd);
	void cleanupClient(int clientFd);
	void sendErrorResponse(int clientFd, int statusCode, const std::string &statusText, const std::string &body);
	void scheduleResponse(int clientFd, const std::string &response);
	size_t parseContentLength(const std::string &request, size_t headerEnd);
	bool isChunkedTransfer(const std::string &request, size_t headerEnd);
	int parseChunkedBody(const std::string &request, size_t headerEnd, size_t &totalBodySize, bool &isComplete);
	bool validateContentLength(int clientFd, const std::string &request, size_t headerEnd, 
							   const std::string &method, const std::string &uri, ServerConf *selectedConf);
	bool validateChunkedBody(int clientFd, const std::string &request, size_t headerEnd,
							 const std::string &uri, ServerConf *selectedConf);
	void processCompleteRequest(int clientFd, const std::string &request, ServerConf *selectedConf);

	static void handleSignal(int signum);
	static volatile bool running_;
	int clientTimeout_;
public:
	Server(const std::vector<ServerConf> &serverConfs, char **envp = NULL);
	~Server();
	bool isRunning();
	void run();
};

#endif