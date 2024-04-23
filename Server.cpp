#include "Server.hpp"
#include "Channel.hpp"
#include <sstream>

bool Server::Signal = false;

Server::Server()
{
}

Server::~Server()
{
}

void	Server::show_clients()
{
	std::cout << "Clients connected: " << Clients.size() << std::endl;
	for (size_t i = 0; i < Clients.size(); i++)
	{
		std::cout << "Client " << i + 1 << " fd: " << Clients[i].getfd()  << " username : " << Clients[i].get_username() << " pass: " << Clients[i].get_has_pass() << " nickname: " << Clients[i].get_nickname() << " auth: " << Clients[i].get_auth() << std::endl;
		std::cout << "has_user: " << Clients[i].get_has_user() << " has_nick: " << Clients[i].get_has_nick() << std::endl;
	}
}

void	Server::Serverinit(int port, std::string password)
{
	this->Port = port;
	this->password = password;
	struct sockaddr_in addr;
	struct pollfd ser_pol_fd;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(this->Port);
	addr.sin_addr.s_addr = INADDR_ANY;

	ser_pol_fd.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (ser_pol_fd.fd == -1)
		throw (std::runtime_error("failed to create socket"));
	
	int en = 1;
	if (setsockopt(ser_pol_fd.fd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)))
		throw(std::runtime_error("faild to set option (SO_REUSEADDR) on socket"));
	if (fcntl(ser_pol_fd.fd, F_SETFL, O_NONBLOCK) == -1)
		throw(std::runtime_error("faild to set option (O_NONBLOCK) on socket"));
	if (bind(ser_pol_fd.fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		throw(std::runtime_error("faild to bind socket"));
	if (listen(ser_pol_fd.fd, SOMAXCONN) == -1)
		throw(std::runtime_error("listen() faild"));
	ser_pol_fd.events = POLLIN;
	ser_pol_fd.revents = 0;
	this->pollfds.push_back(ser_pol_fd);
	std::cout << "Waiting to accept a connection...\n";

	while (Server::Signal == false)
	{
		if((poll(&pollfds[0], pollfds.size(), -1) == -1) && Server::Signal == false)
			throw(std::runtime_error("poll failed"));	
		for (size_t i = 0; i < pollfds.size(); i++)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (i == 0)
					AcceptNewClient();
				else
					ReceiveNewData(pollfds[i].fd);
			}
		}
		//show_clients();
	}
	Closefds();
}

Server::Server(const Server &copy)
{
	*this = copy;
}

Server &Server::operator=(const Server &copy)
{
	this->Port = copy.Port;
	this->pollfds = copy.pollfds;
	this->Clients = copy.Clients;
	return (*this);
}

void	Server::AcceptNewClient()
{
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	struct pollfd new_pol_fd;

	new_pol_fd.fd = accept(pollfds[0].fd, (struct sockaddr *)&addr, &addr_len);
	if (new_pol_fd.fd == -1)
		throw(std::runtime_error("accept() faild"));
	if (fcntl(new_pol_fd.fd, F_SETFL, O_NONBLOCK) == -1)
		throw(std::runtime_error("faild to set option (O_NONBLOCK) on socket"));
	new_pol_fd.events = POLLIN;
	new_pol_fd.revents = 0;
	this->pollfds.push_back(new_pol_fd);
	Client new_client(new_pol_fd.fd, addr, addr_len);
	this->Clients.push_back(new_client);
	std::cout << "New client connected\n";
}

int	count_args(std::string str)
{
	int args = 0;
	std::string::size_type pos = 0;
	while ((pos = str.find(" ")) != std::string::npos)
	{
		while (str[pos] == ' ')
			pos++;
		str.erase(0, pos);
		args++;
	}
	return (args + 1);
}

std::string	*parse_cmd(std::string str)
{
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end()); // complete later
	int args_num = count_args(str);
	std::string *cmd = new std::string[args_num + 1];
	std::string tmp;
	std::string::size_type pos = 0;
	int i = 0;

	while ((pos = str.find(" ")) != std::string::npos)
	{

		tmp = str.substr(0, pos);
		while (str[pos] == ' ')
			pos++;
		str.erase(0, pos);
		cmd[i] = tmp;
		i++;
	}
	cmd[i] = str;
	return (cmd);
}

bool	is_in_channel(Client &cli, Channel &channel)
{
	std::deque<Client*> clis = channel.get_clients();
	
	for (size_t i = 0; i < clis.size(); i++)
	{
		if (clis[i]->getfd() == cli.getfd())
			return (true);
	}
	return (false);
}

void Server::senderror(int code, std::string clientname, int fd, std::string msg)
{
	std::stringstream ss;
	ss << ":localhost " << code << " " << clientname << msg;
	std::string resp = ss.str();
	if(send(fd, resp.c_str(), resp.size(),0) == -1)
		std::cerr << "send() faild" << std::endl;
}

void Server::sendresponse(int code, std::string clientname, int fd, std::string msg)
{
	std::stringstream ss;
	ss << ":localhost " << code << " " << clientname << msg;
	std::string resp = ss.str();
	if(send(fd, resp.c_str(), resp.size(),0) == -1)
		std::cerr << "send() faild" << std::endl;
}


void	Server::execute(int fd, std::string *cmd)
{
	// kick join part privmsg topic mode user nick pass quit
	int i = -1;
	while (++i < Clients.size())
		if (Clients[i].getfd() == fd)
			break ;
	if (cmd[0] == "pass" || cmd[0] == "PASS")
		authenticate(fd, cmd);
	else if (cmd[0] == "nick" || cmd[0] == "NICK")
		nick(fd, cmd);
	else if (cmd[0] == "user" || cmd[0] == "USER")
		user(fd, cmd);
	else if (cmd[0] == "quit" || cmd[0] == "QUIT")
		quit(fd, cmd);
	else if (Clients[i].get_auth() == false)
	{
		std::cout << "Client " << i + 1 << " needs to authenticate first\n";
		return;
	}
	else if (cmd[0] == "join" || cmd[0] == "JOIN")
	{
		join(fd, cmd, i);
	}
	else if (cmd[0] == "part" || cmd[0] == "PART")
	{
		if (cmd[1].empty() || cmd[1][0] != '#' || cmd[1].size() < 2)
		{
			Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
			//complete parsing
			return;
		}
		for (size_t j = 0; j < Channels.size(); j++)
		{
			if (Channels[j].get_name() == cmd[1]) // if channel exists
			{
				Clients[i].leave_channel(Channels[j].get_name());
				return;
			}
		}
		Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
	}
	else if (cmd[0] == "privmsg" || cmd[0] == "PRIVMSG")
	{
		if (cmd[2].empty())
		{
			Server::senderror(412, Clients[i].get_nickname(), fd, " :No text to send\n");
			return ;
		}
		else if (cmd[1].empty())
		{
			Server::senderror(411, Clients[i].get_nickname(), fd, " :No recipient given\n");
			return ;
		}
		else if (cmd[1][0] == '#')
		{
			for (size_t j = 0; j < Channels.size(); j++)
			{
				if (Channels[j].get_name() == cmd[1] && is_in_channel(Clients[i], Channels[j])) // if channel exists
				{
					for (size_t k = 0; k < Clients.size(); k++) // send message to all clients in channel
					{
						if (is_in_channel(Clients[k], Channels[j]))
						{
							std::stringstream ss;
							ss << ":localhost PRIVMSG " << cmd[1] << " :" << cmd[2] << "\n";
							std::string resp = ss.str();
							if(send(Clients[k].getfd(), resp.c_str(), resp.size(),0) == -1)
								std::cerr << "send() faild" << std::endl;
						}
					}
					return;
				}
			}
		}
		for (size_t j = 0; j < Clients.size(); j++) // send message to specific client
		{
			if (Clients[j].get_nickname() == cmd[1]) // if client exists
			{
				std::stringstream ss;
				ss << ":localhost PRIVMSG " << cmd[1] << " :" << cmd[2] << "\n";
				std::string resp = ss.str();
				if(send(Clients[j].getfd(), resp.c_str(), resp.size(),0) == -1)
					std::cerr << "send() faild" << std::endl;
				return;
			}
		}
		Server::senderror(401, Clients[i].get_nickname(), fd, " :No such nick/channel\n");
	}
	else if (cmd[0] == "topic" || cmd[0] == "TOPIC")
	{
		if (cmd[1].empty() || cmd[1][0] != '#')
		{
			Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
			return;
		}
		for (size_t j = 0; j < Channels.size(); j++)
		{
			if (Channels[j].get_name() == cmd[1]) // if channel exists
			{
				if (!is_in_channel(Clients[i], Channels[j]))
				{
					Server::senderror(442, Clients[i].get_nickname(), fd, " :You're not on that channel\n");
					return;
				}
				if (cmd[2].empty()) // if no topic is given
				{
					if (Channels[j].get_topic().empty())
						Server::senderror(331, Clients[i].get_nickname(), fd, " :No topic is set\n");
					else
						Server::sendresponse(332, Clients[i].get_nickname(), fd, " :" + Channels[j].get_topic() + "\n");
				}
				if (Channels[j].get_modes()['t'] == false)
				{
					Server::senderror(482, Clients[i].get_nickname(), fd, " :You're not channel operator\n");
					return;
				}
				Channels[j].set_topic(cmd[2]);
				return;
			}
		}
		Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
	}
	else if (cmd[0] == "mode" || cmd[0] == "MODE")
	{
	}
	else if (cmd[0] == "kick" || cmd[0] == "KICK")
	{
	}
	else
	{
		std::cout << "Client " << i + 1 << " sent an invalid command\n";
	}
}

void	Server::ReceiveNewData(int fd)
{
	char buffer[1024];
	int ret = recv(fd, buffer, 1024, 0);
	if (ret == -1)
		throw(std::runtime_error("recv() faild"));
	if (ret == 0)
	{
		std::cout << "Client disconnected\n";
		rmclient(fd);
	}
	else
	{
		buffer[ret] = '\0';
		std::string str(buffer);
		std::string *cmd = parse_cmd(str);
		this->execute(fd, cmd);
		delete [] cmd;
	}
}

void	Server::SignalHandler(int signum)
{
	(void)signum;
	Signal = true;
}

void	Server::Closefds()
{
	for (size_t i = 0; i < pollfds.size(); i++)
	{
		close(pollfds[i].fd);
	}
}

void	Server::rmclient(int fd)
{
	for (size_t i = 0; i < Clients.size(); i++)
	{
		if (Clients[i].getfd() == fd)
		{
			Clients[i].leave_all_channels();
			Clients[i].reset();
			Clients.erase(Clients.begin() + i);
			break;
		}
	}
	for (size_t i = 0; i < pollfds.size(); i++)
	{
		if (pollfds[i].fd == fd)
		{
			close(pollfds[i].fd);
			pollfds.erase(pollfds.begin() + i);
			break;
		}
	}
}