#include "Server.hpp"
#include "Channel.hpp"
#include <sstream>

bool	is_in_channel(Client &cli, Channel &channel)
{
	std::map<Client*, bool> clis = channel.get_clients();
	
	for (std::map<Client*, bool>::iterator it = clis.begin(); it != clis.end(); it++)
	{
		if ((*it).first->getfd() == cli.getfd())
			return (true);
	}
	return (false);
}

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
		cli.set_has_pass(true);
		if (cli.get_has_nick() == false || cli.get_has_user() == false)
			return;
		cli.set_auth(true);
		Server::sendresponse(001, cli.get_nickname(), cli.getfd(), " :Welcome to the Internet Relay Network\n");
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
		if (!cli.get_auth())
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
		
		cli.set_has_user(true);
		cli.set_username(cmd[1]);
		cli.set_realname(cmd[4]);
		if (cli.get_has_nick() == false || cli.get_has_pass() == false)
			return;
		cli.set_auth(true);
		Server::sendresponse(001, cli.get_nickname(), cli.getfd(), " :Welcome to the Internet Relay Network\n");
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

void	Server::invite_only_join(int fd, std::string *cmd, int i, int j)
{
	if (cmd[2].empty())
	{
		Server::senderror(473, Clients[i].get_nickname(), fd, " :Invite only channel\n");
		return;
	}
	for (size_t k = 0; k < Channels[j].get_invite_list().size(); k++)
	{
		if (Channels[j].get_invite_list()[k] == Clients[i].get_nickname())
		{
			Clients[i].join_channel(&Channels[j]);
			Channels[j].remove_invite(Clients[i].get_nickname());
			return;
		}
	}
	Server::senderror(473, Clients[i].get_nickname(), fd, " :Invite only channel\n");
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
				//if invite only
				if (Channels[j].get_modes()['i'] == true)
				{
					invite_only_join(fd, cmd, i, j);
					return;
				}
				//if key is set
				if (Channels[j].get_key() != "")
				{
					if (cmd[2].empty() || cmd[2] != Channels[j].get_key())
					{
						Server::senderror(475, Clients[i].get_nickname(), fd, " :Bad channel key\n");
						return;
					}
				}
				Clients[i].join_channel(&Channels[j]);
				return;
			}
		}
		Channel new_channel(cmd[1]);
		Clients[i].join_channel(&new_channel);
		Channels.push_back(new_channel);
		return;
}

void	Server::part(int fd, std::string *cmd, int i)
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
				Channels[j].remove_client(&Clients[i]);
				if (Channels[j].get_clients().empty())
					Channels.erase(Channels.begin() + j);
				return;
			}
		}
		Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
}

void	Server::privmsg(int fd, std::string *cmd, int i)
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

void	Server::topic(int fd, std::string *cmd, int i)
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

void	kick(Client &cli, Channel &channel, std::string *cmd)
{
	std::map<Client*, bool> clis = channel.get_clients();
	if (cmd[1].empty())
	{
		Server::senderror(461, cli.get_nickname(), cli.getfd(), " :Not enough parameters\n");
		return;
	}
	for (std::map<Client *, bool>::iterator it = clis.begin(); it != clis.end(); it++)
	{
		if ((*it).first->get_nickname() == cmd[1])
		{
			if (channel.get_modes()['o'] == false)
			{
				Server::senderror(482, cli.get_nickname(), cli.getfd(), " :You're not channel operator\n");
				return;
			}
			channel.remove_client((*it).first);//complete later
			return;
		}
	}
	Server::senderror(441, cli.get_nickname(), cli.getfd(), " :They aren't on that channel\n");
}

void	Server::mode(int fd, std::string *cmd, int i)
{
	
}

void	Server::invite(int fd, std::string *cmd, int i)
{
	if (cmd[1].empty() || cmd[2].empty())
	{
		Server::senderror(461, Clients[i].get_nickname(), fd, " :Not enough parameters\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == cmd[2]) // if channel exists
		{
			if (!is_in_channel(Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i].get_nickname(), fd, " :You're not on that channel\n");
				return;
			}
			if (Channels[j].get_modes()['i'] == false)
			{
				Server::senderror(482, Clients[i].get_nickname(), fd, " :You're not channel operator\n");
				return;
			}
			for (size_t k = 0; k < Clients.size(); k++) // send message to specific client
			{
				if (Clients[k].get_nickname() == cmd[1]) // if client exists`
				{
					std::stringstream ss;
					ss << ":localhost INVITE " << cmd[1] << " :" << cmd[2] << "\n";
					std::string resp = ss.str();
					if(send(Clients[k].getfd(), resp.c_str(), resp.size(),0) == -1)
						std::cerr << "send() faild" << std::endl;
					return;
				}
			}
			Server::senderror(401, Clients[i].get_nickname(), fd, " :No such nick/channel\n");
			return;
		}
	}
	Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
}

void	Server::kick(int fd, std::string *cmd, int i)
{
}