
#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"

bool Server::is_in_channel(Client &cli, Channel &channel)
{
	std::map<Client *, bool> *clis = channel.get_clients();

	for (std::map<Client *, bool>::iterator it = clis->begin(); it != clis->end(); it++)
	{
		if ((*it).first->getfd() == cli.getfd())
			return (true);
	}
	return (false);
}

void Server::invite_only_join(int fd, std::vector<std::string> cmd, Client &cli, Channel &channel)
{
	(void)cmd;
	for (size_t k = 0; k < channel.get_invite_list().size(); k++)
	{
		if (channel.get_invite_list()[k] == cli.get_nickname())
		{
			channel.add_client(&cli);
			channel.remove_invite(cli.get_nickname());
			return;
		}
	}
	Server::senderror(473, cli.get_nickname(), fd, " :Invite only channel\r\n");
}

void Server::join(int fd, std::vector<std::string> cmd, int i)
{
	if (cmd[1].empty() || cmd[1][0] != '#' || cmd[1].size() < 2) // complete parsing
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == cmd[1]) // if channel exists
		{
			// if client is already in channel
			if (is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(443, Clients[i]->get_nickname(), fd, " :is already on channel\r\n");
				return;
			}
			// if invite only
			if (Channels[j].get_modes()['i'] == true)
			{
				invite_only_join(fd, cmd, *Clients[i], Channels[j]);
				return;
			}
			// if key is set
			if (Channels[j].get_modes()['k'] == true)
			{
				if (cmd[2].empty() || cmd[2] != Channels[j].get_key())
				{
					Server::senderror(475, Clients[i]->get_nickname(), fd, " :Bad channel key\r\n");
					return;
				}
			}
			// if limit is set
			if (Channels[j].get_modes()['l'] == true)
			{
				if (Channels[j].get_clients()->size() >= Channels[j].get_limit())
				{
					Server::senderror(471, Clients[i]->get_nickname(), fd, " :Cannot join channel (+l)\r\n");
					return;
				}
			}
			Channels[j].add_client(Clients[i]);
			return;
		}
	}
	Channel new_channel(cmd[1]);
	new_channel.add_client(Clients[i]);
	Channels.push_back(new_channel);
	return;
}

void Server::part(int fd, std::vector<std::string> cmd, int i)
{
	if (cmd[1].empty() || cmd[1][0] != '#' || cmd[1].size() < 2)
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
		// complete parsing
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == cmd[1]) // if channel exists
		{
			// Clients[i]->leave_channel(Channels[j].get_name());
			Channels[j].remove_client(Clients[i]);
			if (Channels[j].get_clients()->empty())
				Channels.erase(Channels.begin() + j);
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
}

void Server::privmsg(int fd, std::vector<std::string> cmd, int i)
{
	if (cmd[2].empty())
	{
		Server::senderror(412, Clients[i]->get_nickname(), fd, " :No text to send\r\n");
		return;
	}
	else if (cmd[1].empty())
	{
		Server::senderror(411, Clients[i]->get_nickname(), fd, " :No recipient given\r\n");
		return;
	}
	else if (cmd[1][0] == '#')
	{
		for (size_t j = 0; j < Channels.size(); j++)
		{
			if (Channels[j].get_name() == cmd[1] && is_in_channel(*Clients[i], Channels[j])) // if channel exists
			{
				for (size_t k = 0; k < Clients.size(); k++) // send message to all clients in channel
					if (is_in_channel(*Clients[k], Channels[j]) && Clients[k]->getfd() != fd)
						Server::sendmsg(Clients[k]->getfd(), ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " PRIVMSG " + cmd[1] + " :" + cmd[2] + "\r\n");
				return;
			}
		}
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
	}
	for (size_t j = 0; j < Clients.size(); j++) // send message to specific client
	{
		if (Clients[j]->get_nickname() == cmd[1]) // if client exists
		{
			if (Clients[j]->getfd() != fd)
				Server::sendmsg(Clients[j]->getfd(), ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " PRIVMSG " + cmd[1] + " :" + cmd[2] + "\r\n");
			return;
		}
	}
	Server::senderror(401, Clients[i]->get_nickname(), fd, " :No such nick/channel\r\n");
}

void Server::topic(int fd, std::vector<std::string> cmd, int i)
{
	if (cmd[1].empty() || cmd[1][0] != '#')
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == cmd[1]) // if channel exists
		{
			if (!is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not on that channel\r\n");
				return;
			}
			if (cmd[2].empty()) // if no topic is given
			{
				if (Channels[j].get_topic().empty())
					Server::sendresponse(331, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No topic is set\r\n");
				else
					Server::sendresponse(332, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :" + Channels[j].get_topic() + "\r\n");
				return;
			}
			if (Channels[j].get_modes()['t'] == true && !is_op_in(*Clients[i], Channels[j]))
			{
				Server::senderror(482, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not channel operator\r\n");
				return;
			}
			Channels[j].set_topic(cmd[2]);
			std::string topic_msg = ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " TOPIC " + cmd[1] + " :" + cmd[2] + "\r\n";
			std::map<Client *, bool>::iterator it;
			for (size_t k = 0; k < Clients.size(); k++)
				if (is_in_channel(*Clients[k], Channels[j]))
					Server::sendmsg(Clients[k]->getfd(), topic_msg);
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
}

void Server::broadcastmsg(std::string msg, Channel &channel)
{
	std::map<Client *, bool> *clis = channel.get_clients();
	for (std::map<Client *, bool>::iterator it = clis->begin(); it != clis->end(); it++)
	{
		if (send((*it).first->getfd(), msg.c_str(), msg.size(), 0) == -1)
			std::cerr << "send() failure" << std::endl;
	}
}

void Server::kick(int fd, std::vector<std::string> cmd, int i)
{
	if (cmd[1].empty() || cmd[2].empty())
	{
		Server::senderror(461, Clients[i]->get_nickname(), fd, " KICK :Not enough parameters\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		std::map<Client *, bool> *clis = Channels[j].get_clients();
		if (Channels[j].get_name() == cmd[1]) // if channel exists
		{
			if (!is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not on that channel\r\n");
				return;
			}
			if (!is_op_in(*Clients[i], Channels[j]))
			{
				Server::senderror(482, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not channel operator\r\n");
				return;
			}
			for (std::map<Client *, bool>::iterator it = clis->begin(); it != clis->end(); it++) // send message to specific client
			{
				if ((*it).first->get_nickname() == cmd[2]) // if client exists
				{
					std::string ss;
					if (cmd[3].empty())
						ss = ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " KICK " + cmd[1] + " " + cmd[2] + " :No reason given\r\n";
					else
						ss = ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " KICK " + cmd[1] + " " + cmd[2] + " :" + cmd[3] + "\r\n";
					Server::broadcastmsg(ss, Channels[j]);
					clis->erase(it); // remove client from channel
					return;
				}
			}
			Server::senderror(401, Clients[i]->get_nickname(), fd, " :no such nick\r\n");
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
}

void Server::invite(int fd, std::vector<std::string> cmd, int i)
{
	if (cmd[1].empty() || cmd[2].empty())
	{
		Server::senderror(461, Clients[i]->get_nickname(), fd, " INVITE :Not enough parameters\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == cmd[2]) // if channel exists
		{
			if (!is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not on that channel\r\n");
				return;
			}
			if (Channels[j].get_modes()['i'] == true && !is_op_in(*Clients[i], Channels[j]))
			{
				Server::senderror(482, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not channel operator\r\n");
				return;
			}
			for (size_t k = 0; k < Clients.size(); k++) // send message to specific client
			{
				if (Clients[k]->get_nickname() == cmd[1]) // if client exists
				{
					if (is_in_channel(*Clients[k], Channels[j]))
						return;
					Channels[j].add_invite(Clients[k]->get_nickname());
					Server::sendmsg(Clients[k]->getfd(), ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " INVITE " + cmd[1] + " " + cmd[2] + "\r\n");
					return;
				}
			}
			Server::senderror(401, Clients[i]->get_nickname(), fd, " :No such nick/channel\r\n");
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[2] + ":No such channel\r\n");
}

void Server::mode(int fd, std::vector<std::string> cmd, int i)
{
	int params = 0;
	if (cmd[1].empty() || cmd[1][0] != '#')
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == cmd[1]) // if channel exists
		{
			std::map<char, bool> modes = Channels[j].get_modes();
			if (!is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not on that channel\r\n");
				return;
			}
			if (cmd[2].empty())
			{
				std::string resp;
				resp = ":" + Server::get_servername() + " 324 " + Clients[i]->get_nickname() + " " + cmd[1] + " :+";
				for (std::map<char, bool>::iterator it = modes.begin(); it != modes.end(); it++)
				{
					if ((*it).second == true)
						resp += (*it).first;
				}
				resp += "\r\n";
				Server::sendmsg(fd, resp);
				return;
			}
			for (size_t k = 0; k < cmd[2].size(); k = k + 2)
			{
				if (cmd[2][k] == '+')
				{
					if (!is_op_in(*Clients[i], Channels[j]) && (cmd[2][k + 1] == 'o' || cmd[2][k + 1] == 'l' || cmd[2][k + 1] == 'k'))
					{
						Server::senderror(482, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :You're not channel operator\r\n");
						continue;
					}
					if (cmd[2][k + 1] == 'o')
					{
						if (cmd[3 + params].empty())
						{
							Server::senderror(461, Clients[i]->get_nickname(), Clients[i]->getfd(), " MODE :Not enough parameters\r\n");
							continue;
						}
						for (size_t l = 0; l < Clients.size(); l++)
						{
							if (Clients[l]->get_nickname() == cmd[3 + params])
							{
								Channels[j].set_operator(Clients[l]);
								Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +o :" + cmd[3+params] + "\r\n", Channels[j]);
								params++;
								return;
							}
						}
						Server::senderror(441, Clients[i]->get_nickname(), Clients[i]->getfd(), " :They aren't on that channel\r\n");
						params++;
						continue;
					}
					else if (cmd[2][k + 1] == 't')
					{
						Channels[j].set_mode('t', true);
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " :+t\r\n", Channels[j]);
					}
					else if (cmd[2][k + 1] == 'i')
					{
						Channels[j].set_mode('i', true);
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " :+i\r\n", Channels[j]);
					}
					else if (cmd[2][k + 1] == 'l')
					{
						if (cmd[3+params].empty())
						{
							Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
							continue;
						}
						Channels[j].set_mode('l', true);
						Channels[j].set_limit(std::atoi(cmd[3+params].c_str()));
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +l " + cmd[3+params] + "\r\n", Channels[j]);
						params++;
					}
					else if (cmd[2][k + 1] == 'k')
					{
						if (cmd[3+params].empty())
						{
							Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
							continue;
						}
						Channels[j].set_mode('k', true);
						Channels[j].set_key(cmd[3+params]);
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +k " + cmd[3+params] + "\r\n", Channels[j]);
						params++;
					}
				}
				else if (cmd[2][k] == '-')
				{
					if (cmd[2][k + 1] == 'o')
					{
						if (cmd[3+params].empty())
						{
							Server::senderror(461, Clients[i]->get_nickname(), Clients[i]->getfd(), " MODE :Not enough parameters\r\n");
							continue;
						}
						for (size_t l = 0; l < Clients.size(); l++)
						{
							if (Clients[l]->get_nickname() == cmd[3+params])
							{
								Channels[j].remove_operator(Clients[l]);
								Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -o :" + cmd[3+params] + "\r\n", Channels[j]);
								params++;
								return;
							}
						}
						Server::senderror(441, Clients[i]->get_nickname(), fd, " :They aren't on that channel\r\n");
						params++;
						continue;
					}
					else if (cmd[2][k + 1] == 't')
					{
						Channels[j].set_mode('t', false);
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -t\r\n", Channels[j]);
					}
					else if (cmd[2][k + 1] == 'i')
					{
						Channels[j].set_mode('i', false);
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -i\r\n", Channels[j]);
					}
					else if (cmd[2][k + 1] == 'l')
					{
						Channels[j].set_mode('l', false);
						Channels[j].set_limit(-1);
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -l\r\n", Channels[j]);
					}
					else if (cmd[2][k + 1] == 'k')
					{
						Channels[j].set_mode('k', false);
						Channels[j].set_key("");
						Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -k\r\n", Channels[j]);
					} // to complete later
				}
			}
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
}