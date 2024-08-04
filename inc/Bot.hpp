#pragma once

#include "Client.hpp"
#include <poll.h>
#include <sstream>
#include <fcntl.h>
#include <cstdio>
#include <stdexcept>


std::vector<std::string> parse_cmd(std::string str);
bool	isCompared(std::string const &str1, std::string const &str2);
std::string dad_jokes(const char* cmd);

class Bot : public Client
{
	private:
		int 						_port;
		std::string					_pass;
		struct pollfd				_polfd;
		void	send_message(std::string const &message);
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