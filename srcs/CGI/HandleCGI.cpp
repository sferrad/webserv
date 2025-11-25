#include "../../include/webserv.h"
#include <signal.h>
#include <errno.h>
#include <climits>

int HandleCGI::waitWithTimeout(pid_t pid, int timeout_seconds)
{
	time_t start_time = time(NULL);
	int status;
	int result;
	
	while ((result = waitpid(pid, &status, WNOHANG)) == 0)
	{
		if (time(NULL) - start_time >= timeout_seconds)
		{
			std::cout << "\033[31m[" << getCurrentTime() << "] "
					  << "⚠️  CGI timeout after " << timeout_seconds << " seconds, killing process " << pid << "\033[0m" << std::endl;
			killCgiProcess(pid);
			return -1;
		}
		usleep(100000);
	}
	
	if (result == -1)
	{
		std::cerr << "Error waiting for CGI process: " << strerror(errno) << std::endl;
		return -1;
	}
	
	return result;
}

void HandleCGI::killCgiProcess(pid_t pid)
{
	if (kill(pid, SIGTERM) == 0)
	{
		time_t start = time(NULL);
		int status;
		while (time(NULL) - start < 2 && waitpid(pid, &status, WNOHANG) == 0)
			usleep(100000);
		
		if (waitpid(pid, &status, WNOHANG) == 0)
		{
			std::cout << "\033[31m[" << getCurrentTime() << "] "
					  << "🔥 Process didn't terminate with SIGTERM, using SIGKILL" << "\033[0m" << std::endl;
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0);
		}
	}
	else
		std::cerr << "Failed to kill CGI process " << pid << ": " << strerror(errno) << std::endl;
}

void HandleCGI::generateErrorPage(int code)
{
	lastErrorCode_ = code;
	respBody_.clear();
	respBody_.str("");

	std::string base = webRoot_.empty() ? root_ : webRoot_;

	std::map<int, std::string>::const_iterator it = errorPages_.find(code);
	if (it != errorPages_.end())
	{
		std::string page = it->second;
		if (page.rfind("./", 0) == 0)
			page.erase(0, 2);

		std::string errPath;
		if (!page.empty() && page[0] == '/')
			errPath = base + page;
		else
			errPath = base + "/" + page;

		std::ifstream ferr(errPath.c_str());
		if (ferr)
		{
			respBody_ << ferr.rdbuf();
			return;
		}
	}

	std::ostringstream fallback;
	fallback << "./www/error/" << code << ".html";
	std::ifstream ferr2(fallback.str().c_str());
	if (ferr2)
	{
		respBody_ << ferr2.rdbuf();
		return;
	}

	const char *reason = "Error";
	switch (code)
	{
	case 408:
		reason = "Request Timeout";
		break;
	case 500:
		reason = "Internal Server Error";
		break;
	case 504:
		reason = "Gateway Timeout";
		break;
	default:
		reason = "Error";
		break;
	}
	respBody_ << "<html><head><title>" << code << " " << reason
			  << "</title></head><body><h1>" << code << " " << reason
			  << "</h1><p>CGI script timeout or execution error.</p></body></html>";
}

int HandleCGI::findCgiExtensionInUri(const std::string &uri)
{
	size_t pos = uri.rfind('.');
	if (pos == std::string::npos)
		return -1;
	std::string ext = uri.substr(pos);
	if (ext == cgiExtension_)
		return pos;
	return -1;
}

std::string HandleCGI::extractScriptName(const std::string &uri)
{
	int extPos = findCgiExtensionInUri(uri);
	if (extPos == -1)
		return "";

	size_t lastSlash = uri.rfind('/');
	if (lastSlash == std::string::npos)
		return uri;
	size_t scriptEnd = extPos + cgiExtension_.length();
	return uri.substr(lastSlash + 1, scriptEnd - lastSlash - 1);
}

int HandleCGI::GetMethodCGI(const std::string &uri, const std::string &queryString, const std::string &method)
{
	lastErrorCode_ = 0;
	
	std::cout << "\033[32m[" << getCurrentTime() << "] "
			  << "CGI Method called for URI: " << uri 
			  << ", Query: " << queryString
			  << ", Method: " << method << "\033[0m" << std::endl;
	int extPos = findCgiExtensionInUri(uri);
	if (extPos == -1)
	{
		std::cout << "\033[33m[" << getCurrentTime() << "] "
				  << "No matching CGI extension found in URI: " << uri << "\033[0m" << std::endl;
		generateErrorPage(404);
		return 1;
	}
	std::cout << "\033[32m[" << getCurrentTime() << "] "
			  << "CGI extension matched at position " << extPos 
			  << " for URI: " << uri << "\033[0m" << std::endl;
	std::string scriptName = extractScriptName(uri);
	std::cout << "\033[32m[" << getCurrentTime() << "] "
			  << "Extracted script name: " << scriptName << "\033[0m" << std::endl;

	std::string scriptPath = root_;
	if (!scriptPath.empty() && scriptPath[scriptPath.length() - 1] != '/')
		scriptPath += "/";
	scriptPath += scriptName;
	
	char resolvedPath[PATH_MAX];
	char *absPath = realpath(scriptPath.c_str(), resolvedPath);
	std::string scriptAbsPath = absPath ? absPath : scriptPath;
	
	std::cout << "\033[36m[" << getCurrentTime() << "] "
			  << "🔧 Script path: " << scriptPath << "\033[0m" << std::endl;
	std::cout << "\033[36m[" << getCurrentTime() << "] "
			  << "🔧 Absolute path: " << scriptAbsPath << "\033[0m" << std::endl;

	int pipfd[2];
	if (pipe(pipfd) == -1)
	{
		std::cerr << "Error: Failed to create pipe for CGI." << std::endl;
		generateErrorPage(500);
		return 1;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		std::cerr << "Error: Failed to fork process for CGI." << std::endl;
		close(pipfd[0]);
		close(pipfd[1]);
		generateErrorPage(500);
		return 1;
	}
	if (pid == 0)
	{
		close(pipfd[0]);
		dup2(pipfd[1], STDOUT_FILENO);
		close(pipfd[1]);

		std::string scriptDir = scriptPath;
		size_t lastSlash = scriptDir.rfind('/');
		if (lastSlash != std::string::npos)
			scriptDir = scriptDir.substr(0, lastSlash);
		else
			scriptDir = ".";

		if (chdir(scriptDir.c_str()) == -1) {
			std::cerr << "Error: chdir failed: " << strerror(errno) << std::endl;
			exit(1);
		}

		std::vector<std::string> envStrings;
		envStrings.push_back("REQUEST_METHOD=" + method);
		envStrings.push_back("SCRIPT_FILENAME=" + scriptAbsPath);
		envStrings.push_back("REDIRECT_STATUS=200");
		envStrings.push_back("REMOTE_HOST=" + serverName_);
		envStrings.push_back("REMOTE_ADDR=");
		envStrings.push_back("PATH_INFO=");
		envStrings.push_back("SCRIPT_NAME=/" + scriptName);
		envStrings.push_back("QUERY_STRING=" + queryString);
		envStrings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		envStrings.push_back("CONTENT_LENGTH=0");
		envStrings.push_back("SERVER_SOFTWARE= webserv/1.0");
		
		std::ostringstream portStr;
		portStr << serverPort_;
		envStrings.push_back("SERVER_PORT=" + portStr.str());
		
		envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
		envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");

		std::vector<char*> envVec;
		for (size_t i = 0; i < envStrings.size(); i++)
			envVec.push_back(const_cast<char*>(envStrings[i].c_str()));
		envVec.push_back(NULL);

		std::string scriptFilename = scriptName;
		size_t lastSlashName = scriptName.rfind('/');
		if (lastSlashName != std::string::npos)
			scriptFilename = scriptName.substr(lastSlashName + 1);

		char *args[] = {const_cast<char *>(cgiPath_.c_str()), const_cast<char *>(scriptFilename.c_str()), NULL};
		if (envp_)
			execve(cgiPath_.c_str(), args, &envVec[0]);
		else
			execve(cgiPath_.c_str(), args, &envVec[0]);

		std::cerr << "Error: execve failed for CGI." << std::endl;
		exit(1);
	}
	else
	{
		close(pipfd[1]);
		char buffer[1024];
		ssize_t n;

		time_t readStartTime = time(NULL);
		
		while (true)
		{
			if (time(NULL) - readStartTime >= CGI_TIMEOUT)
			{
				std::cout << "\033[31m[" << getCurrentTime() << "] "
						  << "⚠️  CGI read timeout after " << CGI_TIMEOUT << " seconds" << "\033[0m" << std::endl;
				killCgiProcess(pid);
				break;
			}
			
			fd_set readSet;
			struct timeval timeout;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			
			FD_ZERO(&readSet);
			FD_SET(pipfd[0], &readSet);
			
			int selectResult = select(pipfd[0] + 1, &readSet, NULL, NULL, &timeout);
			
			if (selectResult > 0 && FD_ISSET(pipfd[0], &readSet))
			{
				n = read(pipfd[0], buffer, sizeof(buffer) - 1);
				if (n > 0)
				{
					buffer[n] = '\0';
					respBody_ << buffer;
				}
				else if (n == 0)
					break;
				else
					break;
			}
			else if (selectResult == 0)
			{
				int status;
				int waitResult = waitpid(pid, &status, WNOHANG);
				if (waitResult != 0)
					break;
			}
			else
				break;
		}
		
		close(pipfd[0]);
		
		if (time(NULL) - readStartTime >= CGI_TIMEOUT)
		{
			std::cerr << "\033[31m[" << getCurrentTime() << "] "
					  << "❌ CGI script timed out during execution" << "\033[0m" << std::endl;
			generateErrorPage(504);
			return 1;
		}
		
		if (waitWithTimeout(pid, 2) == -1)
		{
			std::cerr << "\033[31m[" << getCurrentTime() << "] "
					  << "❌ CGI script failed to terminate cleanly" << "\033[0m" << std::endl;
			generateErrorPage(504);
			return 1;
		}
		
		std::cout << "\033[32m[" << getCurrentTime() << "] "
				  << "✅ CGI script completed successfully" << "\033[0m" << std::endl;
	}
	return 1;
}

int HandleCGI::PostMethodCGI(const std::string &uri,
                             const std::string &queryString,
                             const std::string &body,
                             const std::string &method)
{
	lastErrorCode_ = 0;
	(void)queryString;

	std::cout << "\033[32m[" << getCurrentTime() << "] "
			  << "CGI POST called for URI: " << uri
			  << ", Body Length: " << body.length()
			  << ", Method: " << method << "\033[0m" << std::endl;

	int extPos = findCgiExtensionInUri(uri);
	if (extPos == -1)
	{
		std::cout << "\033[33m[" << getCurrentTime() << "] "
				  << "No CGI extension found in URI: " << uri << "\033[0m" << std::endl;
		generateErrorPage(404);
		return 1;
	}

	std::string scriptName = extractScriptName(uri);
	std::string scriptPath = root_;
	if (!scriptPath.empty() && scriptPath[scriptPath.length() - 1] != '/')
		scriptPath += "/";
	scriptPath += scriptName;


	char resolvedPath[PATH_MAX];
	char *absPath = realpath(scriptPath.c_str(), resolvedPath);
	std::string scriptAbsPath = absPath ? absPath : scriptPath;

	std::cout << "\033[36m[" << getCurrentTime() << "] "
			  << "🔧 Script path: " << scriptPath << "\033[0m" << std::endl;
	std::cout << "\033[36m[" << getCurrentTime() << "] "
			  << "🔧 Absolute path: " << scriptAbsPath << "\033[0m" << std::endl;

	int in_fd[2];
	int out_fd[2];

	if (pipe(in_fd) == -1 || pipe(out_fd) == -1)
	{
		generateErrorPage(500);
		return 1;
	}

	pid_t pid = fork();

	if (pid == 0)
	{
		dup2(in_fd[0], STDIN_FILENO);
		dup2(out_fd[1], STDOUT_FILENO);

		close(in_fd[1]);
		close(out_fd[0]);

		std::string scriptDir = scriptPath;
		size_t lastSlash = scriptDir.rfind('/');
		if (lastSlash != std::string::npos)
			scriptDir = scriptDir.substr(0, lastSlash);
		else
			scriptDir = ".";

		if (chdir(scriptDir.c_str()) == -1) {
			std::cerr << "Error: chdir failed: " << strerror(errno) << std::endl;
			exit(1);
		}

		std::ostringstream len;
		len << body.length();

		std::vector<std::string> envStrings;
		envStrings.push_back("REQUEST_METHOD=POST");
		envStrings.push_back("PATH_INFO=");
		envStrings.push_back("REMOTE_HOST=" + serverName_);
		envStrings.push_back("SCRIPT_FILENAME=" + scriptAbsPath);
		envStrings.push_back("REDIRECT_STATUS=200");
		envStrings.push_back("REMOTE_ADDR=");
		envStrings.push_back("SERVER_SOFTWARE= webserv/1.0");
		envStrings.push_back("CONTENT_LENGTH=" + len.str());
		envStrings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		envStrings.push_back("QUERY_STRING=");
		envStrings.push_back("SCRIPT_NAME=/" + scriptName);
		envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");

		std::ostringstream portStr;
		portStr << serverPort_;
		envStrings.push_back("SERVER_PORT=" + portStr.str());
		envStrings.push_back("SERVER_NAME=" + serverName_);
		envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");

		std::vector<char*> envVec;
		for (size_t i = 0; i < envStrings.size(); i++)
			envVec.push_back(const_cast<char*>(envStrings[i].c_str()));
		envVec.push_back(NULL);

		std::string scriptFilename = scriptName;
		size_t lastSlashName = scriptName.rfind('/');
		if (lastSlashName != std::string::npos)
			scriptFilename = scriptName.substr(lastSlashName + 1);

		char *args[] = {
			const_cast<char *>(cgiPath_.c_str()),
			const_cast<char *>(scriptFilename.c_str()),
			NULL
		};

		execve(cgiPath_.c_str(), args, &envVec[0]);

		std::cerr << "Error: execve failed for CGI." << std::endl;
		exit(1);
	}
	else
	{
		close(in_fd[0]);
		write(in_fd[1], body.c_str(), body.length());
		close(in_fd[1]);

		close(out_fd[1]);

		char buffer[1024];
		ssize_t n;

		time_t readStartTime = time(NULL);
		
		while (true)
		{
			if (time(NULL) - readStartTime >= CGI_TIMEOUT)
			{
				std::cout << "\033[31m[" << getCurrentTime() << "] "
						  << "⚠️  CGI read timeout after " << CGI_TIMEOUT << " seconds" << "\033[0m" << std::endl;
				killCgiProcess(pid);
				break;
			}
			
			fd_set readSet;
			struct timeval timeout;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			
			FD_ZERO(&readSet);
			FD_SET(out_fd[0], &readSet);
			
			int selectResult = select(out_fd[0] + 1, &readSet, NULL, NULL, &timeout);
			
			if (selectResult > 0 && FD_ISSET(out_fd[0], &readSet))
			{
				n = read(out_fd[0], buffer, sizeof(buffer) - 1);
				if (n > 0)
				{
					buffer[n] = '\0';
					respBody_ << buffer;
				}
				else if (n == 0)
					break;
				else
					break;
			}
			else if (selectResult == 0)
			{
				int status;
				int waitResult = waitpid(pid, &status, WNOHANG);
				if (waitResult != 0)
					break;
			}
			else
				break;
		}

		close(out_fd[0]);

		if (waitWithTimeout(pid, CGI_TIMEOUT) == -1)
		{
			std::cerr << "\033[31m[" << getCurrentTime() << "] "
					  << "❌ CGI POST script timed out or failed" << "\033[0m" << std::endl;
			generateErrorPage(504);
			return 1;
		}
		
		std::cout << "\033[32m[" << getCurrentTime() << "] "
				  << "✅ CGI POST script completed successfully" << "\033[0m" << std::endl;
	}
	return 1;
}