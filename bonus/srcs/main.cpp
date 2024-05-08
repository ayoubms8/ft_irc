#include "../inc/Server.hpp"

int main(int ac, char **av)
{
	Server ser;
	if (ac != 3 || !isdigit(*av[1]) || atoi(av[1]) < 1024 || atoi(av[1]) > 49151){
		std::cerr << "Usage: ./ircserv <port[1024, 49151]> <password>" << std::endl;
		return 1;
	}


	std::cout << "---- SERVER ----" << std::endl;
	try{
		signal(SIGINT, Server::SignalHandler);
		signal(SIGQUIT, Server::SignalHandler);
		ser.Serverinit(atoi(av[1]), av[2]);
	}
	catch(const std::exception& e){
		ser.Closefds();//
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
}
