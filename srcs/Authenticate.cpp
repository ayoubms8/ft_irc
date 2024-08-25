#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"
#include <arpa/inet.h>

void Server::sendWelcomeMessages(Client &cli)
{
	std::string nick = cli.get_nickname();
	std::string user = cli.get_username();
	std::string host = cli.get_ip();
	std::string version = "1.0.0";
	std::string userModes = "";
	std::string channelModes = "iklot";
	std::string featureList = "CHANTYPES=# PREFIX=(o)@ CHANMODES=o,k,l,t,i";

	Server::sendmsg(cli.getfd(), "001 " + nick + " :Welcome to the Internet Relay Network " + nick + "!" + user + "@" + host + "\r\n");
	Server::sendmsg(cli.getfd(), "002 " + nick + " :Your host is " + Server::servername + ", running version " + version + "\r\n");
	Server::sendmsg(cli.getfd(), "003 " + nick + " :This server was created " + Server::creationdate + "\r\n");
	Server::sendmsg(cli.getfd(), "004 " + nick + " : " + Server::servername + " " + version + " " + userModes + " " + channelModes + "\r\n");
}

std::string get_users_in_channel(Channel channel)
{
	std::string users;
	std::map<Client *, bool> *clients = channel.get_clients();
	for (std::map<Client *, bool>::iterator it = clients->begin(); it != clients->end(); it++)
	{
		if ((*it).second == true)
			users += "@" + (*it).first->get_nickname() + " ";
		else
			users += (*it).first->get_nickname() + " ";
	}
	return users;
}

void Server::ch_join_message(Client &cli, Channel channel)
{
	std::string nickname = cli.get_nickname();
	std::string servername = Server::servername;
	std::string channelname = channel.get_name();
	std::string users = get_users_in_channel(channel);
	std::string username = cli.get_username();
	std::string topic = channel.get_topic();
	
	std::string msg = ":" + servername + " 353 " + nickname + " = " + channelname + " :" + users + "\r\n";
	msg += ":" + servername + " 366 " + nickname + " " + channelname + " :End of /NAMES list.\r\n";
	Server::sendmsg(cli.getfd(), msg);
}

void Server::ft_pass(int fd, std::vector<std::string> cmd)
{
	Client *cli = get_client_by_fd(Clients, fd);
	if (cli->get_has_pass())
	{
		Server::sendmsg(fd, ":"+Server::servername + " 462 " + cli->get_nickname() + " :You may not reregister\r\n");
		return;
	}
	if (cmd[1].empty() || cmd[1] != this->password)
	{
		Server::sendmsg(fd , ":"+Server::servername + " 464 :Password incorrect\r\n");
		return;
	}
	else
	{
		cli->set_has_pass(true);
	}
}

void Server::ft_nick(int fd, std::vector<std::string> cmd)
{
	Client *cli = get_client_by_fd(Clients, fd);
	if (!cli->get_has_pass())
	{
		Server::sendmsg(fd, ":"+Server::servername + " 451 " + cli->get_nickname() + " :Enter password first\r\n");
		return;
	}
	size_t j;
	if (cmd[1].empty())
	{
		Server::sendmsg(fd, ":"+Server::servername + " 461 " + cli->get_nickname() + " NICK :Not enough parameters\r\n");
		return;
	}
	size_t it = cmd[1].find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]\\`_^{|}-");
	if (it != std::string::npos)
	{
		if (it == 0)
		{
			Server::sendmsg(fd, ":"+Server::servername + " 432 " + cli->get_nickname() + " :Erroneous nickname\r\n");
			return;
		}
		cmd[1] = cmd[1].substr(0, it);
	}
	if (cmd[1].size() > 15)
		cmd[1] = cmd[1].substr(0, 15);
	for (j = 0; j < Clients.size(); j++)
	{
		if (isCompared(Clients[j]->get_nickname(), cmd[1]) && Clients[j]->getfd() != cli->getfd())
		{
			Server::sendmsg(fd, ":"+Server::servername + " 433 " + cli->get_nickname() + " " + cmd[1] + " :Nickname is already in use\r\n");
			return;
		}
	}
	if (cli->get_nickname() == cmd[1])
		cli->set_nickname(cmd[1]);
	else
	{
		bool has_ch = false;
		cli->set_has_nick(true);
		for (size_t i = 0; i < Channels.size(); i++)
		{
			if (is_in_channel(*cli, Channels[i]))
			{
				has_ch = true;
				Server::broadcastmsg(":" + cli->get_nickname() + "!" + cli->get_username() + "@" + cli->get_ip() + " NICK :" + cmd[1] + "\r\n", Channels[i]);
			}
		}
		if (!has_ch && cli->get_auth())
			Server::sendmsg(fd, ":" + cli->get_nickname() + "!" + cli->get_username() + "@" + cli->get_ip() + " NICK :" + cmd[1] + "\r\n");
		cli->set_nickname(cmd[1]);
		if (!cli->get_has_user())
			return;
		if (!cli->get_auth())
		{
			cli->set_auth(true);
			Server::sendWelcomeMessages(*cli);
		}
	}
}

void Server::ft_user(int fd, std::vector<std::string> cmd)
{
	Client *cli = get_client_by_fd(Clients, fd);
	if (!cli->get_has_pass())
	{
		Server::sendmsg(fd, ":"+Server::servername + " 451 " + cli->get_nickname() + " :Enter password first\r\n");
		return;
	}
	if (cli->get_has_user())
	{
		Server::sendmsg(fd, ":"+Server::servername + " 462 " + cli->get_nickname() + " :You may not reregister\r\n");
		return;
	}
	else if (cmd[1].empty() || cmd[2].empty() || cmd[3].empty() || cmd[4].empty())
	{
		Server::sendmsg(fd, ":"+Server::servername + " 461 " + cli->get_nickname() + " USER :Not enough parameters\r\n");
		return;
	}
	else
	{
		cli->set_has_user(true);
		cli->set_username(cmd[1]);
		if (!cli->get_has_nick())
			return;
		cli->set_auth(true);
		Server::sendWelcomeMessages(*cli);
	}
}

void Server::ft_quit(int fd, std::vector<std::string> cmd)
{
	(void)cmd;
	Client *cli = get_client_by_fd(Clients, fd);
	Server::sendmsg(cli->getfd(), ":" + cli->get_nickname() +"!" +cli->get_username() + "@" + cli->get_ip() + " QUIT :Client disconnected\r\n");
	for (size_t i = 0; i < Clients.size(); i++)
	{
		if (Clients[i]->getfd() == fd)
		{
			for (size_t j = 0; j < Channels.size(); j++)
			{
				if (is_in_channel(*Clients[i], Channels[j]))
					Channels[j].remove_client(Clients[i]);
				if (Channels[j].get_clients()->empty() || (Channels[j].get_clients()->size() == 1 && Channels[j].get_clients()->begin()->first->get_nickname() == "Botto"))
				{
					std::cout << "remove channel" << std::endl;
					Channels.erase(Channels.begin() + j);
				}
			}
			Clients[i]->reset();
			delete Clients[i];
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
