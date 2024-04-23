#include "Server.hpp"
#include "Channel.hpp"

Client	&get_client_by_fd(std::deque<Client> &clients, int fd)
{
	for (size_t i = 0; i < clients.size(); i++)
	{
		if (clients[i].getfd() == fd)
			return clients[i];
	}
	throw std::runtime_error("Client not found");
}

void	Server::authenticate(int fd, std::string *cmd)
{
	Client &cli = get_client_by_fd(Clients, fd);
	if (cmd[1].empty() || cmd[1] != this->password)
	{
		Server::senderror(464, cli.get_nickname(), cli.getfd(), " :Password incorrect\n");
		return;
	}
	else
	{
		if (!cli.get_has_pass())
			Server::sendresponse(001, cli.get_nickname(), cli.getfd(), " :Welcome to the Internet Relay Network\n");
		cli.set_has_pass(true);
		if (cli.get_has_nick() == false || cli.get_has_user() == false)
			return;
		cli.set_auth(true);
	}
}

void	Server::nick(int fd, std::string *cmd)
{
	Client &cli = get_client_by_fd(Clients, fd);
	size_t j;
	if (cmd[1].empty())
	{
		Server::senderror(431, cli.get_nickname(), cli.getfd(), " :No nickname given\n");
		return;
	}
	for (j = 0; j < Clients.size(); j++)
	{
		if (Clients[j].get_nickname() == cmd[1] && Clients[j].getfd() != cli.getfd())
		{
			Server::senderror(433, cli.get_nickname(), cli.getfd(), " :Nickname is already in use\n");
			return;
		}
	}
	if (cli.get_nickname() == cmd[1])
		cli.set_nickname(cmd[1]);
	else
	{
		if (!cli.get_has_nick())
			Server::sendresponse(001, cli.get_nickname(), cli.getfd(), " :Welcome to the Internet Relay Network\n");
		cli.set_has_nick(true);
		cli.set_nickname(cmd[1]);
		if (cli.get_has_pass() == false || cli.get_has_user() == false)
			return;
		cli.set_auth(true);
	}
}

void	Server::user(int fd, std::string *cmd)
{
	Client &cli = get_client_by_fd(Clients, fd);
	if (cli.get_has_user() == true)
	{
		Server::senderror(462, cli.get_nickname(), cli.getfd(), " :You may not reregister\n");
		return;
	}
	else if (cmd[1].empty() || cmd[2].empty() || cmd[3].empty() || cmd[4].empty())
	{
		Server::senderror(461, cli.get_nickname(), cli.getfd(), " :Not enough parameters\n");
		return;
	}
	else
	{
		if (!cli.get_has_user())
			Server::sendresponse(001, cli.get_nickname(), cli.getfd(), " :Welcome to the Internet Relay Network\n");
		cli.set_has_user(true);
		cli.set_username(cmd[1]);
		cli.set_realname(cmd[4]);
		if (cli.get_has_nick() == false || cli.get_has_pass() == false)
			return;
		cli.set_auth(true);
	}
}

void	Server::quit(int fd, std::string *cmd)
{
	Client &cli = get_client_by_fd(Clients, fd);
	Server::sendresponse(001, cli.get_nickname(), fd, " :Goodbye\n");
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

void	Server::join(int fd, std::string *cmd, int i)
{
	if (cmd[1].empty() || cmd[1][0] != '#' || cmd[1].size() < 2)//complete parsing
		{
			Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
			return;
		}
		for (size_t j = 0; j < Channels.size(); j++)
		{
			if (Channels[j].get_name() == cmd[1]) // if channel exists
			{
				Clients[i].join_channel(&Channels[j]);
				return;
			}
		}
		Channel new_channel(cmd[1]);
		Clients[i].join_channel(&new_channel);
		Channels.push_back(new_channel);
		return;
}