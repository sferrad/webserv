#ifndef HANDLE_CGI_HPP
#define HANDLE_CGI_HPP

class HandleCGI{
	private:
		std::string cgiPath_;
		std::string cgiExtension_;
		char **envp_;
		std::string root_;
		std::string serverName_;
		std::string clientIp_;
		static const int CGI_TIMEOUT = 15;
		
		std::string webRoot_;
		std::map<int, std::string> errorPages_;
		std::map<std::string, std::string> headers_;
		int lastErrorCode_;
		
		int serverPort_;
		int findCgiExtensionInUri(const std::string &uri);
		std::string extractScriptName(const std::string &uri);
		int waitWithTimeout(pid_t pid, int timeout_seconds);
		void killCgiProcess(pid_t pid);
		void generateErrorPage(int code);
		std::vector<std::string> createEnv(const std::string &method, const std::string &scriptName, const std::string &scriptAbsPath, const std::string &queryString, const std::string &body);
	public:
		std::ostringstream respBody_;
		HandleCGI(const std::string &cgiPath, const std::string &cgiExtension, char **envp = NULL)
			: cgiPath_(cgiPath), cgiExtension_(cgiExtension), envp_(envp), root_("./www"), 
			  serverName_("webserv"), lastErrorCode_(0), serverPort_(8080) {}
		
		std::string getCgiPath() const { return cgiPath_; }
		std::string getCgiExtension() const { return cgiExtension_; }
		void setCgiPath(const std::string &path) { cgiPath_ = path; }
		void setCgiExtension(const std::string &ext) { cgiExtension_ = ext; }
		void setEnvp(char **envp) { envp_ = envp; }
		char **getEnvp() const { return envp_; }
		void setRoot(const std::string &root) { root_ = root; }
		std::string getRoot() const { return root_; }
		void setClientIp(const std::string &ip) { clientIp_ = ip; }
		void setServerName(const std::string &name) { serverName_ = name; }
		void setServerPort(int port) { serverPort_ = port; }
		
		void setWebRoot(const std::string &webRoot) { webRoot_ = webRoot; }
		void setErrorPages(const std::map<int, std::string> &errorPages) { errorPages_ = errorPages; }
		void setHeaders(const std::map<std::string, std::string> &headers) { headers_ = headers; }
		int getLastErrorCode() const { return lastErrorCode_; }
		
		int GetMethodCGI(const std::string &uri, const std::string &queryString = "", const std::string &method = "GET");
		int PostMethodCGI(const std::string &uri, const std::string &queryString = "", const std::string &body = "", const std::string &method = "POST");
};

#endif