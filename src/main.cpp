#include "../inc/Server.hpp"

int main(int ac, char **av)
{
	Server ser;
	if (ac != 3 || !isdigit(*av[1]) || atoi(av[1]) < 0 || atoi(av[1]) > 65535 ){
		std::cerr << "Usage: ./server [port] [password]" << std::endl;
		return 1;
	}


	std::cout << "---- SERVER ----" << std::endl;
	try{
		signal(SIGINT, Server::SignalHandler);
		signal(SIGQUIT, Server::SignalHandler);
		ser.Serverinit(atoi(av[1]), av[2]);
	}
	catch(const std::exception& e){
		ser.Closefds();
		std::cerr << e.what() << std::endl;
	}
	std::cout << "The Server Closed!" << std::endl;
}
