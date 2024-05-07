#include "../inc/Bot.hpp"
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <strings.h>
#include <sys/_endian.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <ctime>
#include <poll.h>
#include <sstream>

Bot::Bot()
{
	this->set_nickname("Botto");
	this->set_username("Botto");
	_jokes.push_back("Why did the tomato turn red? Because it saw the salad dressing!");
	_jokes.push_back("What do you call a fake noodle? An impasta!");
	_jokes.push_back("I told my wife she was drawing her eyebrows too high. She looked surprised.");
	_jokes.push_back("Why did the bicycle fall over? Because it was two-tired!");
	_jokes.push_back("What do you call a bear with no teeth? A gummy bear!");
	_jokes.push_back("Why did the math book look sad? Because it had too many problems.");
	_jokes.push_back("How do you catch a squirrel? Climb a tree and act like a nut!");
	_jokes.push_back("Why don't eggs tell jokes? Because they might crack up!");
}

struct pollfd	Bot::get_fd() const
{
	return _polfd;
}

bool Bot::connect_to_server(struct sockaddr* serv_addr, int port, std::string pass) {
	struct pollfd polfd;
    polfd.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (polfd.fd < 0) {
        std::cerr << "Failed to open the socket" << std::endl;
        return false;
    }
    if (connect(polfd.fd, serv_addr, sizeof(*serv_addr)) < 0) {
        std::cerr << "Failed to connect" << std::endl;
        return false;
    }
	polfd.events = POLLIN;
	polfd.revents = 0;
    this->_port = port;
    this->_pass = pass;
	this->_polfd = polfd;
    return true;
}

void Bot::receive_message()
{
	char buffer[1024];
	std::string::size_type pos;
	int ret = recv(_polfd.fd, buffer, 1024, 0);
	if (ret == -1)
		throw(std::runtime_error("recv() failure"));
	if (ret == 0)
	{
		std::cout << "Client disconnected\n";
	}
	else
	{
		buffer[ret] = '\0';
		std::string str(buffer);
		std::cout << "Received by bot: " << str << std::endl;
		pos = str.find("\r\n");
		while (pos != std::string::npos)
		{
			std::string temp = str.substr(0, pos);
			std::vector<std::string> cmd = parse_cmd(temp);
			this->execute(cmd);
			str.erase(0, pos + 2);
			pos = str.find("\r\n");
		}
		pos = str.find("\n");
		while (pos != std::string::npos)
		{
			std::string temp = str.substr(0, pos);
			std::vector<std::string> cmd = parse_cmd(temp);
			this->execute(cmd);
			str.erase(0, pos + 2);
			pos = str.find("\n");
		}
	}
}

void Bot::authenticate()
{
	send_message("PASS " + _pass);
	send_message("NICK " + this->get_nickname());
	send_message("USER " + this->get_username() + " O * :" + this->get_username());
}

void Bot::join_channel(std::string const &channel)
{
	send_message("JOIN " + channel);
}

void Bot::send_message(std::string const &message)
{
	std::string full_message = message + "\r\n";
	if (send(_polfd.fd, full_message.c_str(), full_message.length(), 0) < 0)
		std::cerr << "Failed to send message" << std::endl;
}

Bot::~Bot()
{
	close(_polfd.fd);
}

std::string Bot::get_random_joke()
{
	std::srand(std::time(NULL));
	int i = std::rand() % _jokes.size();
	return _jokes[i];
}

void Bot::execute(std::vector<std::string> cmd)
{
	std::vector<std::string> cmd2;
	if (cmd.size() == 0)
		return;
	else if (cmd[1] == "PRIVMSG")
	{
		std::stringstream ss(cmd[3]);
		std::string	token;
		while (std::getline(ss, token, ' '))
		{
			cmd2.push_back(token);
		}
		if (isCompared(cmd2[0], "!joke") || isCompared(cmd2[0], ":!joke"))
			send_message("PRIVMSG " + cmd[2] + " :" + get_random_joke());
		else if (isCompared(cmd2[0], "!join"))
		{
			std::string channel = cmd2[1];
			send_message("JOIN " + channel);
			send_message("PRIVMSG " + channel + " :Hello, I am Botto. I am here to entertain you. Type !joke to get a joke.");
		}
		else if (cmd2[0] == "!part")
		{
			std::string channel = cmd2[1];
			send_message("PRIVMSG " + channel + " :MY final message. Goodbye!");
			send_message("PART " + channel);
		}
		else if (cmd2[0] == "!info")
		{
			// if (cmd2.size() == 1)
			// 	info of cmd[2] from 42 api here;
			// else
			// {
			// 	info from 42 api here of cmd2[1];
			// }
		}
		else if (cmd2[0] == "!help")
		{
			send_message("PRIVMSG " + cmd[2] + " :Commands available: !joke, !join <channel>, !part, !info <42username>, !help");
		}
	}
}