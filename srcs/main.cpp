#include "../inc/Server.hpp"
#include <csignal>
#include <algorithm>
#include <string.h>

int HasChar(char *str)
{
	for (size_t i = 0; i < strlen(str); i++)
	{
		if (!isdigit(str[i]))
			return 1;
	}
	return 0;
}

int main(int ac, char **av)
{
	Server ser;
	if (ac != 3 || HasChar(av[1]) || atoi(av[1]) < 1024 || atoi(av[1]) > 49151 || strlen(av[2]) < 1 || strpbrk(av[2], " \t\n\r\v\f")){
		std::cerr << "Usage: ./ircserv <port[1024, 49151]> <password(no spaces allowed)>" << std::endl;
		return 1;
	}
	try{
		signal(SIGINT, Server::SignalHandler);
		signal(SIGQUIT, Server::SignalHandler);
		ser.Serverinit(atoi(av[1]), av[2]);
	}
	catch(std::exception& e){
		ser.Closefds();
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
}
