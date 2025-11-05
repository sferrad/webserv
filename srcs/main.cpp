#include "../include/webserv.h"

bool checkFileConfig(std::string filename) {

	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		std::cerr << "Error: Could not open config file " << filename << std::endl;
		return false;
	}
	if (filename.find(".conf") == std::string::npos) {
		std::cerr << "Error: Config file must have a .conf extension" << std::endl;
		return false;
	}
	file.close();
	return true;
}

int main(int ac, char **av)
{
	std::string files;
	if (ac == 2)
		files = av[1];
	if (ac != 2) {
		if (ac == 1)
			files = "conf_file/default.conf";
		else{
			std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
			return 1;
		}
	}
	if (!checkFileConfig(files))
		return 1;
	const std::string cfgPath = static_cast<std::string>(files);

	std::vector<ServerConf> servers = ServerConf::parseConfigFile(cfgPath);
	if (servers.empty()) {
		ServerConf fallback(cfgPath);
		servers.push_back(fallback);
	}
	std::cout << "Loaded " << servers.size() << " server block(s) from config." << std::endl;
	std::cout << "\033[33m" << "[" << getCurrentTime() << "] " << "Starting server..." << "\033[0m" << std::endl;
	try {
		Server server(servers);
		server.run();
	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
