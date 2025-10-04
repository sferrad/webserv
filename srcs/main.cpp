#include "../include/webserv.h"

int main()
{
	try {
		Server server(8080);
		server.Server_run();
	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}