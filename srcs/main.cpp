#include "../include/webserv.h"

int severSocket_init()
{
	int serverSocket;
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(8080);

	if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		std::cerr << "Bind failed: " << strerror(errno) << std::endl;
		return 1;
	}

	return serverSocket;
}

int safeAccept(int serverSocket)
{
	int clientSocket = accept(serverSocket, NULL, NULL);

	int flags = fcntl(serverSocket, F_GETFL, 0);
	fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);
	if (clientSocket < 0)
	{
		throw std::runtime_error(std::string("Accept failed: ") + strerror(errno));
	}
	return clientSocket;
}

int main()
{
	int serverSocket = severSocket_init();

	listen(serverSocket, 5);
	int clientSocket = -1;
	while (true)
	{
		try
		{
			clientSocket = safeAccept(serverSocket);
		}
		catch (const std::runtime_error &e)
		{
			std::cerr << e.what() << std::endl;
			clientSocket = -1;
			continue;
		}
		std::cout << "Client connected" << std::endl;
		std::cout << "test" << std::endl;

		while (clientSocket)
		{
			char buffer[1024] = {0};
			int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
			if (bytesRead < 0)
				std::cerr << "Read failed" << std::endl;
			else
			{
				buffer[bytesRead] = '\0';
				std::cout << "Received: " << buffer << std::endl;
			}
			buffer[strcspn(buffer, "\r\n")] = 0;
			const char *message = "Hello, client!\n";
			int bytesSent = send(clientSocket, message, strlen(message), 0);

			if (bytesSent < 0)
				std::cerr << "Error send: " << strerror(errno) << std::endl;
			else
				std::cout << "Message sent to client." << std::endl;
			if (strcmp(buffer, "exit") == 0)
			{
				close(clientSocket);
				clientSocket = -1; // Exit the loop
			}
		}
		std::cout << "Client disconnected" << std::endl;
	}

	return 0;
}
