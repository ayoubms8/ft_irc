#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"
#include <sstream>
#include <cstring>
#include <string.h>

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
		if (channel.get_invite_list()[k] == &cli)
		{
			channel.add_client(&cli);
			channel.remove_invite(&cli);
			return;
		}
	}
	Server::senderror(473, cli.get_nickname(), fd, " :Invite only channel\r\n");
}

bool is_invited(Client &cli, Channel &channel)
{
	for (size_t k = 0; k < channel.get_invite_list().size(); k++)
	{
		if (channel.get_invite_list()[k] == &cli)
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
	{
		if (std::find(result.begin(), result.end(), item) == result.end())
			result.push_back(item);
	}
	return result;
}

void Server::join_channel(std::string &chnl, std::vector<std::string> &keys, size_t &k, int i, int fd)
{
	if (chnl == "0")
	{
		for (int j = Channels.size() - 1; j >= 0; j--)
		{
			if (is_in_channel(*Clients[i], Channels[j]))
				part_channel(Channels[j].get_name(), i, fd);
		}
		return ;
	}
	else if ((chnl.empty() || chnl[0] != '#' || chnl.size() < 2))
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + chnl + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (isCompared(Channels[j].get_name(), chnl))
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
			if (Channels[j].get_modes()['l'] == true && Channels[j].get_clients()->size() >= Channels[j].get_limit())
			{
				Server::senderror(471, Clients[i]->get_nickname(), fd, " :Cannot join channel (+l)\r\n");
				return;
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
	else
		keys.push_back("");
	size_t k = 0;
	for (size_t it = 0; it < Chnls.size(); it++)
		join_channel(Chnls[it], keys, k, i, fd);
}

void Server::part_channel(const std::string &chnl, int i, int fd)
{
	if (chnl.empty() || chnl[0] != '#' || chnl.size() < 2)
	{
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + chnl + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		if (isCompared(Channels[j].get_name(), chnl))
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
		part_channel(Chnls[it], i, fd);
}

void Server::send_priv(std::string &receiver, std::string &msg, int i, int fd)
{
	if (msg.empty())
	{
		Server::senderror(412, Clients[i]->get_nickname(), fd, " :No text to send\r\n");
		return;
	}
	else if (receiver.empty())
	{
		Server::senderror(411, Clients[i]->get_nickname(), fd, " :No recipient given\r\n");
		return;
	}
	else if (receiver[0] == '#')
	{
		for (size_t j = 0; j < Channels.size(); j++)
		{
			if (isCompared(Channels[j].get_name(), receiver) && is_in_channel(*Clients[i], Channels[j]))
			{
				for (size_t k = 0; k < Clients.size(); k++)
					if (is_in_channel(*Clients[k], Channels[j]) && Clients[k]->getfd() != fd)
						Server::sendmsg(Clients[k]->getfd(), ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " PRIVMSG " + receiver + " :" + msg + "\r\n");
				return;
			}
		}
		Server::senderror(403, Clients[i]->get_nickname(), fd, " " + receiver + " :No such channel\r\n");
		return;
	}
	for (size_t j = 0; j < Clients.size(); j++)
	{
		if (isCompared(Clients[j]->get_nickname(), receiver))
		{
			Server::sendmsg(Clients[j]->getfd(), ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " PRIVMSG " + receiver + " :" + msg + "\r\n");
			return;
		}
	}
	Server::senderror(401, Clients[i]->get_nickname(), fd, " :No such nick/channel\r\n");
}

void Server::privmsg(int fd, std::vector<std::string> cmd, int i)
{
	std::vector<std::string> receivers = split(cmd[1], ',');

	for (size_t it = 0; it < receivers.size(); it++)
		send_priv(receivers[it], cmd[2], i, fd);
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
		if (isCompared(Channels[j].get_name(), cmd[1]))
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
		if (isCompared(Channels[j].get_name(), cmd[1]))
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
				if (isCompared((*it).first->get_nickname(), cmd[2]))
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
		if (isCompared(Channels[j].get_name(), cmd[2]))
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
				if (isCompared(Clients[k]->get_nickname(), cmd[1]))
				{
					if (is_in_channel(*Clients[k], Channels[j]))
						return;
					Channels[j].add_invite(Clients[k]);
					Server::sendmsg(Clients[k]->getfd(), ":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " INVITE " + cmd[1] + " " + cmd[2] + "\r\n");
					return;
				}
			}
			Server::senderror(401, Clients[i]->get_nickname(), fd, " :No such nick\r\n");
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
		if (isCompared(Channels[j].get_name(), cmd[1]))
		{
			if (!is_in_channel(*Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i]->get_nickname(), fd, " " + Channels[j].get_name() + " :You're not on that channel\r\n");
				return;
			}
			if (cmd[2].empty())
			{
				std::map<char, bool> modes = Channels[j].get_modes();
				std::stringstream resp;
				resp << ":" << Server::get_servername() << " 324 " << Clients[i]->get_nickname() << " " << cmd[1] << " :+";
				for (std::map<char, bool>::iterator it = modes.begin(); it != modes.end(); it++)
				{
					if ((*it).second == true)
						resp << (*it).first;
				}
				if (is_op_in(*Clients[i], Channels[j]))
					resp << " " << Channels[j].get_key();
				else if (Channels[j].get_modes()['k'])
					resp << " *";
				if (Channels[j].get_modes()['l'])
					resp << " " << Channels[j].get_limit();
				resp << "\r\n";
				Server::sendmsg(fd, resp.str());
				return;
			}
			if (!is_op_in(*Clients[i], Channels[j]))
			{
				Server::senderror(482, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :You're not channel operator\r\n");
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
						if (cmd[2][k] == 'o')
						{
							int done = 0;
							if (cmd[3 + params].empty())
							{
								Server::senderror(461, Clients[i]->get_nickname(), Clients[i]->getfd(), " MODE :Not enough parameters\r\n");
								continue;
							}
							for (size_t l = 0; l < Clients.size(); l++)
							{
								if (isCompared(Clients[l]->get_nickname(), cmd[3 + params]) && is_in_channel(*Clients[l], Channels[j]))
								{
									Channels[j].set_operator(Clients[l]);
									Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +o :" + cmd[3 + params] + "\r\n", Channels[j]);
									params++;
									done++;
									break;
								}
							}
							if (done)
								continue;
							Server::senderror(441, Clients[i]->get_nickname(), Clients[i]->getfd(), " :They aren't on that channel\r\n");
							params++;
							continue;
						}
						else if (cmd[2][k] == 't' && Channels[j].get_modes()['t'] == false)
						{
							Channels[j].set_mode('t', true);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +t\r\n", Channels[j]);
							continue;
						}
						else if (cmd[2][k] == 'i' && Channels[j].get_modes()['i'] == false)
						{
							Channels[j].set_mode('i', true);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +i\r\n", Channels[j]);
							continue;
						}
						else if (cmd[2][k] == 'l')
						{
							if (cmd[3 + params].empty())
							{
								Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
								continue;
							}
							else if (std::atoi(cmd[3 + params].c_str()) < 0)
							{
								Server::senderror(468, Clients[i]->get_nickname(), fd, " MODE :Channel limit cannot be less than 0\r\n");
								params++;
								continue;
							}
							Channels[j].set_mode('l', true);
							Channels[j].set_limit(std::atoi(cmd[3 + params].c_str()));
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +l " + cmd[3 + params] + "\r\n", Channels[j]);
							params++;
							continue;
						}
						else if (cmd[2][k] == 'k')
						{
							if (cmd[3 + params].empty())
							{
								Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
								continue;
							}
							else if (Channels[j].get_key() == cmd[3 + params])
							{
								Server::senderror(467, Clients[i]->get_nickname(), fd, " MODE :Channel key already set\r\n");
								params++;
								continue;
							}
							else if (strlen(cmd[3 + params].c_str()) < 1 || strpbrk(cmd[3 + params].c_str(), " \t\n\r\v\f"))
							{
								Server::senderror(467, Clients[i]->get_nickname(), fd, " MODE :Bad channel key\r\n");
								params++;
								continue;
							}
							Channels[j].set_mode('k', true);
							Channels[j].set_key(cmd[3 + params]);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " +k " + cmd[3 + params] + "\r\n", Channels[j]);
							params++;
							continue;
						}
					}
				} //
				if (cmd[2][k] == '-')
				{
					while (++k && k < cmd[2].size())
					{
						if (cmd[2][k] == '-')
							continue;
						if (cmd[2][k] == '+' && k--)
							break;
						if (cmd[2][k] == 'o')
						{
							int done = 0;
							if (cmd[3 + params].empty())
							{
								Server::senderror(461, Clients[i]->get_nickname(), Clients[i]->getfd(), " MODE :Not enough parameters\r\n");
								continue;
							}
							for (size_t l = 0; l < Clients.size(); l++)
							{
								if (isCompared(Clients[l]->get_nickname(), cmd[3 + params]) && is_in_channel(*Clients[l], Channels[j]))
								{
									Channels[j].remove_operator(Clients[l]);
									Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -o :" + cmd[3 + params] + "\r\n", Channels[j]);
									params++;
									done++;
									break;
								}
							}
							if (done)
								continue;
							Server::senderror(441, Clients[i]->get_nickname(), fd, " :They aren't on that channel\r\n");
							params++;
							continue;
						}
						else if (cmd[2][k] == 't' && Channels[j].get_modes()['t'] == true)
						{
							Channels[j].set_mode('t', false);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -t\r\n", Channels[j]);
							continue;
						}
						else if (cmd[2][k] == 'i' && Channels[j].get_modes()['i'] == true)
						{
							Channels[j].set_mode('i', false);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -i\r\n", Channels[j]);
							continue;
						}
						else if (cmd[2][k] == 'l' && Channels[j].get_modes()['l'] == true)
						{
							Channels[j].set_mode('l', false);
							Channels[j].set_limit(-1);
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -l\r\n", Channels[j]);
							continue;
						}
						else if (cmd[2][k] == 'k')
						{
							if (cmd[3 + params].empty())
							{
								Server::senderror(461, Clients[i]->get_nickname(), fd, " MODE :Not enough parameters\r\n");
								continue;
							}
							else if (Channels[j].get_modes()['k'] == false)
							{
								Server::senderror(467, Clients[i]->get_nickname(), fd, " MODE :Channel key not set\r\n");
								params++;
								continue;
							}
							else if (Channels[j].get_key() != cmd[3 + params])
							{
								Server::senderror(467, Clients[i]->get_nickname(), fd, " MODE :Bad Channel key\r\n");
								params++;
								continue;
							}
							Channels[j].set_mode('k', false);
							Channels[j].set_key("");
							Server::broadcastmsg(":" + Clients[i]->get_nickname() + "!~" + Clients[i]->get_username() + "@" + Clients[i]->get_ip() + " MODE " + cmd[1] + " -k\r\n", Channels[j]);
							params++;
							continue;
						}
					}
					if (cmd[2][k] == '+')
						k--;
				}
			}
			return;
		}
	}
	Server::senderror(403, Clients[i]->get_nickname(), fd, " " + cmd[1] + " :No such channel\r\n");
}