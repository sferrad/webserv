#include "../../include/webserv.h"

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
	
	std::cout << "\033[32m[" << getCurrentTime() << "] "
			  << "CGI Method called for URI: " << uri 
			  << ", Query: " << queryString
			  << ", Method: " << method << "\033[0m" << std::endl;
	int extPos = findCgiExtensionInUri(uri);
	if (extPos == -1)
	{
		std::cout << "\033[33m[" << getCurrentTime() << "] "
				  << "No matching CGI extension found in URI: " << uri << "\033[0m" << std::endl;
		return -1;
	}
	std::cout << "\033[32m[" << getCurrentTime() << "] "
			  << "CGI extension matched at position " << extPos 
			  << " for URI: " << uri << "\033[0m" << std::endl;
	std::string scriptName = extractScriptName(uri);
	std::cout << "\033[32m[" << getCurrentTime() << "] "
			  << "Extracted script name: " << scriptName << "\033[0m" << std::endl;

	std::string scriptPath = root_;
	if (!scriptPath.empty() && scriptPath[scriptPath.length() - 1] != '/') {
		scriptPath += "/";
	}
	scriptPath += scriptName;
	std::cout << "\033[36m[" << getCurrentTime() << "] "
			  << "🔧 Script path: " << scriptPath << "\033[0m" << std::endl;

	int pipfd[2];
	if (pipe(pipfd) == -1)
	{
		std::cerr << "Error: Failed to create pipe for CGI." << std::endl;
		return	-1;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		std::cerr << "Error: Failed to fork process for CGI." << std::endl;
		close(pipfd[0]);
		close(pipfd[1]);
		return -1;
	}
	if (pid == 0)
	{
		close(pipfd[0]);
		dup2(pipfd[1], STDOUT_FILENO);
		close(pipfd[1]);

		std::vector<std::string> envStrings;
		envStrings.push_back("REQUEST_METHOD=" + method);
		envStrings.push_back("SCRIPT_NAME=/" + scriptName);
		envStrings.push_back("QUERY_STRING=" + queryString);
		envStrings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		envStrings.push_back("CONTENT_LENGTH=0");
		envStrings.push_back("SERVER_NAME=" + serverName_);
		
		std::ostringstream portStr;
		portStr << serverPort_;
		envStrings.push_back("SERVER_PORT=" + portStr.str());
		
		envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
		envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
		

		std::vector<char*> envVec;
		for (size_t i = 0; i < envStrings.size(); i++)
		{
			envVec.push_back(const_cast<char*>(envStrings[i].c_str()));
		}
		envVec.push_back(NULL);

		char *args[] = {const_cast<char *>(cgiPath_.c_str()), const_cast<char *>(scriptPath.c_str()), NULL};
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

		while ((n = read(pipfd[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[n] = '\0';
			respBody_ << buffer;
		}
		close(pipfd[0]);
		int status;
		waitpid(pid, &status, 0);
	}
	return 1;
}