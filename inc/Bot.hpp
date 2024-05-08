#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <strings.h>
#include <sys/_endian.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <poll.h>
#include "Client.hpp"

std::vector<std::string> parse_cmd(std::string str);
bool	isCompared(std::string const &str1, std::string const &str2);
std::string dad_jokes(const char* cmd);

class Bot : public Client
{
	private:
		int 						_port;
		std::string					_pass;
		struct pollfd				_polfd;
		std::vector<std::string>	_jokes;
		void	send_message(std::string const &message);
		std::string	get_random_joke();

	public:
		Bot();
		bool				connect_to_server(struct sockaddr* serv_addr, int port, std::string pass);
		void				join_channel(std::string const &channel);
		void				authenticate();
		void				receive_message();
		void				execute(std::vector<std::string> cmd);
		struct pollfd		get_fd() const;
		~Bot();
};