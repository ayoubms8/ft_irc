#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"
#include <sstream>

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

void Server::invite_only_join(int fd, Client &cli, Channel &channel)
{
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

bool is_invited(Client &cli, Channel &channel)
{
	for (size_t k = 0; k < channel.get_invite_list().size(); k++)
	{
		if (channel.get_invite_list()[k] == cli.get_nickname())
			return (true);
	}
	return (false);
}

std::vector<std::string> split(std::string str, char delim)
{
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;

	while (std::getline(ss, item, delim))
		result.push_back(item);
	return result;
}

void Server::join_1(std::string &chnl, std::vector<std::string> &keys, size_t &k, int i, int fd)
{
	if (chnl.empty() || chnl[0] != '#' || chnl.size() < 2)
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + chnl + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == chnl)
		{
			if (is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(443, Clients[i]->get_nickname(), fd, " :is already on channel\r\n");
				return;
			}
			if (Channels[j].get_modes()['i'] == true || is_invited(*Clients[i], Channels[j]) == true)
			{
				invite_only_join(fd, *Clients[i], Channels[j]);
				return;
			}
			if (Channels[j].get_modes()['k'] == true)
			{
				if (keys.size() < k || keys[k].empty() || keys[k] != Channels[j].get_key())
				{
					Server::senderror(475, Clients[i]->get_nickname(), fd, " :Bad channel key\r\n");
					k++;
					return;
				}
				k++;
			}
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
	if (Clients[i]->get_nickname() == "Botto")
		return;
	Channel new_channel(chnl);
	new_channel.add_client(Clients[i]);
	Channels.push_back(new_channel);
}

void Server::join(int fd, std::vector<std::string> cmd, int i)
{
	std::vector<std::string> Chnls = split(cmd[1], ',');
	std::vector<std::string> keys;
	if (!cmd[2].empty())
		keys = split(cmd[2], ',');
	size_t k = 0;
	for (size_t it = 0; it < Chnls.size(); it++)
		join_1(Chnls[it], keys, k, i, fd);
	return;
}

void Server::part_1(std::string &chnl, int i, int fd)
{
	if (chnl.empty() || chnl[0] != '#' || chnl.size() < 2)
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + chnl + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == chnl)
		{
			Channels[j].remove_client(Clients[i]);
			if (Channels[j].get_clients()->empty() || (Channels[j].get_clients()->size() == 1 && Channels[j].get_clients()->begin()->first->get_nickname() == "Botto"))
				Channels.erase(Channels.begin() + j);
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + chnl + " :No such channel\r\n");
}

void Server::part(int fd, std::vector<std::string> cmd, int i)
{
	std::vector<std::string> Chnls = split(cmd[1], ',');
	for (size_t it = 0; it < Chnls.size(); it++)
		part_1(Chnls[it], i, fd);
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
			if (Channels[j].get_name() == cmd[1] && is_in_channel(*Clients[i], Channels[j]))
			{
				for (size_t k = 0; k < Clients.size(); k++)
					if (is_in_channel(*Clients[k], Channels[j]) && Clients[k]->getfd() != fd)
						Server::sendmsg(Clients[k]->getfd(), ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " PRIVMSG " + cmd[1] + " :" + cmd[2] + "\r\n");
				return;
			}
		}
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
	}
	for (size_t j = 0; j < Clients.size(); j++)
	{
		if (Clients[j]->get_nickname() == cmd[1])
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
		if (Channels[j].get_name() == cmd[1])
		{
			if (!is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not on that channel\r\n");
				return;
			}
			if (cmd[2].empty())
			{
				if (Channels[j].get_topic().empty())
					Server::sendmsg(fd, "331 " + Clients[i]->get_nickname() + " " + cmd[1] + " :No topic is set\r\n");
				else
					Server::sendmsg(fd, "332 " + Clients[i]->get_nickname() + " " + cmd[1] + " :" + Channels[j].get_topic() + "\r\n");
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
			std::cerr << "send() FAILED" << std::endl;
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
		if (Channels[j].get_name() == cmd[1])
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
			for (std::map<Client *, bool>::iterator it = clis->begin(); it != clis->end(); it++)
			{
				if ((*it).first->get_nickname() == cmd[2])
				{
					std::string ss;
					if (cmd[3].empty())
						ss = ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " KICK " + cmd[1] + " " + cmd[2] + " :No reason given\r\n";
					else
						ss = ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " KICK " + cmd[1] + " " + cmd[2] + " :" + cmd[3] + "\r\n";
					Server::broadcastmsg(ss, Channels[j]);
					clis->erase(it);
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
		if (Channels[j].get_name() == cmd[2])
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
			for (size_t k = 0; k < Clients.size(); k++)
			{
				if (Clients[k]->get_nickname() == cmd[1])
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
	if (cmd[1].empty())
	{
		Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
		return;
	}
	if (cmd[1][0] != '#')
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (Channels[j].get_name() == cmd[1])
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
			for (size_t k = 0; k < cmd[2].size(); k++)
			{
				if (cmd[2][k] == '+')
				{
					while (++k && k < cmd[2].size() && cmd[2][k] != '-')
					{
						if (cmd[2][k] == '+')
							continue;
						if (!is_op_in(*Clients[i], Channels[j]) && (cmd[2][k] == 'o' || cmd[2][k] == 'l' || cmd[2][k] == 'k'))
						{
							Server::senderror(482, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :You're not channel operator\r\n");
							continue;
						}
						if (cmd[2][k] == 'o')
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
									Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +o :" + cmd[3 + params] + "\r\n", Channels[j]);
									params++;
									return;
								}
							}
							Server::senderror(441, Clients[i]->get_nickname(), Clients[i]->getfd(), " :They aren't on that channel\r\n");
							params++;
							continue;
						}
						else if (cmd[2][k] == 't')
						{
							Channels[j].set_mode('t', true);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " :+t\r\n", Channels[j]);
						}
						else if (cmd[2][k] == 'i')
						{
							Channels[j].set_mode('i', true);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " :+i\r\n", Channels[j]);
						}
						else if (cmd[2][k] == 'l')
						{
							if (cmd[3 + params].empty())
							{
								Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
								continue;
							}
							Channels[j].set_mode('l', true);
							Channels[j].set_limit(std::atoi(cmd[3 + params].c_str()));
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +l " + cmd[3 + params] + "\r\n", Channels[j]);
							params++;
						}
						else if (cmd[2][k] == 'k')
						{
							if (cmd[3 + params].empty())
							{
								Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
								continue;
							}
							Channels[j].set_mode('k', true);
							Channels[j].set_key(cmd[3 + params]);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +k " + cmd[3 + params] + "\r\n", Channels[j]);
							params++;
						}
					}
				} //
				else if (cmd[2][k] == '-')
				{
					while (++k && k < cmd[2].size() && cmd[2][k] != '+')
					{
						if (cmd[2][k] == '-')
							continue;
						if (cmd[2][k] == 'o')
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
									Channels[j].remove_operator(Clients[l]);
									Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -o :" + cmd[3 + params] + "\r\n", Channels[j]);
									params++;
									return;
								}
							}
							Server::senderror(441, Clients[i]->get_nickname(), fd, " :They aren't on that channel\r\n");
							params++;
							continue;
						}
						else if (cmd[2][k] == 't')
						{
							Channels[j].set_mode('t', false);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -t\r\n", Channels[j]);
						}
						else if (cmd[2][k] == 'i')
						{
							Channels[j].set_mode('i', false);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -i\r\n", Channels[j]);
						}
						else if (cmd[2][k] == 'l')
						{
							Channels[j].set_mode('l', false);
							Channels[j].set_limit(-1);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -l\r\n", Channels[j]);
						}
						else if (cmd[2][k] == 'k')
						{
							Channels[j].set_mode('k', false);
							Channels[j].set_key("");
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -k\r\n", Channels[j]);
						}
					}
				}
			}
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
}