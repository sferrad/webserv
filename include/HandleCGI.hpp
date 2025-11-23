#ifndef HANDLE_CGI_HPP
#define HANDLE_CGI_HPP

class HandleCGI{
	private:
		std::string cgiPath_;
		std::string cgiExtension_;
		char **envp_;
		std::string root_;
		std::string serverName_;
		int serverPort_;
		int findCgiExtensionInUri(const std::string &uri);
		std::string extractScriptName(const std::string &uri);
	public:
		std::ostringstream respBody_;
		HandleCGI(const std::string &cgiPath, const std::string &cgiExtension, char **envp = NULL)
			: cgiPath_(cgiPath), cgiExtension_(cgiExtension), envp_(envp), root_("./www"), 
			  serverName_("webserv"), serverPort_(8080) {}
		
		std::string getCgiPath() const { return cgiPath_; }
		std::string getCgiExtension() const { return cgiExtension_; }
		void setCgiPath(const std::string &path) { cgiPath_ = path; }
		void setCgiExtension(const std::string &ext) { cgiExtension_ = ext; }
		void setEnvp(char **envp) { envp_ = envp; }
		char **getEnvp() const { return envp_; }
		void setRoot(const std::string &root) { root_ = root; }
		std::string getRoot() const { return root_; }
		void setServerName(const std::string &name) { serverName_ = name; }
		void setServerPort(int port) { serverPort_ = port; }
		int GetMethodCGI(const std::string &uri, const std::string &queryString = "", const std::string &method = "GET");
};

#endif