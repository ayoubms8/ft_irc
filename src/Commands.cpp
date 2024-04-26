#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"
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


void	Server::invite_only_join(int fd, std::string *cmd, Client &cli, Channel &channel)
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
	Server::senderror(473, cli.get_nickname(), fd, " :Invite only channel\n");
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
			//if client is already in channel
			if (is_in_channel(Clients[i], Channels[j]))
			{
				Server::senderror(443, Clients[i].get_nickname(), fd, " :is already on channel\n");
				return;
			}
			//if invite only
			if (Channels[j].get_modes()['i'] == true)
			{
				invite_only_join(fd, cmd, Clients[i], Channels[j]);
				return;
			}
			//if key is set
			if (Channels[j].get_modes()['k'] == true)
			{
				if (cmd[2].empty() || cmd[2] != Channels[j].get_key())
				{
					Server::senderror(475, Clients[i].get_nickname(), fd, " :Bad channel key\n");
					return;
				}
			}
			Channels[j].add_client(&Clients[i]);
			return;
		}
	}
	Channel new_channel(cmd[1]);
	new_channel.add_client(&Clients[i]);
	new_channel.set_operator(&Clients[i]);
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
			//Clients[i].leave_channel(Channels[j].get_name());
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
		Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
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

void	Server::kick(int fd, std::string *cmd, int i)
{
	if (cmd[1].empty() || cmd[2].empty())
	{
		Server::senderror(461, Clients[i].get_nickname(), fd, " :Not enough parameters\n");
		return;
	}
	for (size_t j = 0; j < Channels.size(); j++)
	{
		std::map<Client*, bool> clis = Channels[j].get_clients();
		if (Channels[j].get_name() == cmd[1]) // if channel exists
		{
			if (!is_in_channel(Clients[i], Channels[j]))
			{
				Server::senderror(442, Clients[i].get_nickname(), fd, " :You're not on that channel\n");
				return;
			}
			if (Channels[j].get_clients()[&Clients[i]] == false)
			{
				Server::senderror(482, Clients[i].get_nickname(), fd, " :You're not channel operator\n");
				return;
			}
			for (std::map<Client *, bool>::iterator it = clis.begin(); it != clis.end(); it++) // send message to specific client
			{
				if ((*it).first->get_nickname() == cmd[2]) // if client exists
				{
					std::stringstream ss;
					ss << ":localhost KICK " << cmd[1] << " " << cmd[2];
					if (cmd[3].empty())
						ss << " :" << Clients[i].get_nickname() << "\n";
					else
						ss << " :" << cmd[3] << "\n";
					std::string resp = ss.str();
					if(send((*it).first->getfd(), resp.c_str(), resp.size(),0) == -1)
						std::cerr << "send() faild" << std::endl;
					Channels[j].remove_client((*it).first);//remove client from channel
					return;
				}
			}
			Server::senderror(441, Clients[i].get_nickname(), fd, " :They aren't on that channel\n");
			return;
		}
	}
	Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
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
			if (Channels[j].get_modes()['i'] == true && Channels[j].get_clients()[&Clients[i]] == false)
			{
				Server::senderror(482, Clients[i].get_nickname(), fd, " :You're not channel operator\n");
				return;
			}
			for (size_t k = 0; k < Clients.size(); k++) // send message to specific client
			{
				if (Clients[k].get_nickname() == cmd[1]) // if client exists`
				{
					Channels[j].add_invite(Clients[k].get_nickname());
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

void	Server::mode(int fd, std::string *cmd, int i)
{
	std::map<char, bool> modes = Channels[i].get_modes();
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
			if (cmd[2].empty())
			{
				std::stringstream ss;
				ss << ":localhost MODE " << cmd[1] << " ";
				for (std::map<char, bool>::iterator it = modes.begin(); it != modes.end(); it++)
				{
					if ((*it).second == true)
						ss << (*it).first;
				}
				ss << "\n";
				std::string resp = ss.str();
				if(send(fd, resp.c_str(), resp.size(),0) == -1)
					std::cerr << "send() faild" << std::endl;
				return;
			}
			if (cmd[2][0] == '+')
			{
				for (size_t k = 1; k < cmd[2].size(); k++)
				{
					if (Channels[j].get_clients()[&Clients[i]] == false && (cmd[2][k] == 'o' || cmd[2][k] == 'l' || cmd[2][k] == 'k'))
					{
						Server::senderror(482, Clients[i].get_nickname(), fd, " :You're not channel operator\n");
						return;
					}
					if (cmd[2][k] == 'o')
					{
						if (cmd[3].empty())
						{
							Server::senderror(461, Clients[i].get_nickname(), fd, " :Not enough parameters\n");
							return;
						}
						for (size_t l = 0; l < Clients.size(); l++)
						{
							if (Clients[l].get_nickname() == cmd[3])
							{
								Channels[j].set_operator(&Clients[l]);
								Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " +o " + cmd[3] + "\n");
								return;
							}
						}
						Server::senderror(441, Clients[i].get_nickname(), fd, " :They aren't on that channel\n");
						return;
					}
					else if (cmd[2][k] == 't')
					{
						Channels[j].set_mode('t', true);
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " +t\n");
					}
					else if (cmd[2][k] == 'i')
					{
						Channels[j].set_mode('i', true);
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " +i\n");
					}
					else if (cmd[2][k] == 'l')
					{
						if (cmd[3].empty())
						{
							Server::senderror(461, Clients[i].get_nickname(), fd, " :Not enough parameters\n");
							return;
						}
						Channels[j].set_mode('l', true);
						Channels[j].set_limit(std::stoi(cmd[3]));
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " +l " + cmd[3] + "\n");
					}
					else if (cmd[2][k] == 'k')
					{
						if (cmd[3].empty())
						{
							Server::senderror(461, Clients[i].get_nickname(), fd, " :Not enough parameters\n");
							return;
						}
						Channels[j].set_mode('k', true);
						Channels[j].set_key(cmd[3]);
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " +k " + cmd[3] + "\n");
					}
				}
			}
			else if (cmd[2][0] == '-')
			{
				for (size_t k = 1; k < cmd[2].size(); k++)
				{
					if (cmd[2][k] == 'o')
					{
						for (size_t l = 0; l < Clients.size(); l++)
						{
							if (Clients[l].get_nickname() == cmd[3])
							{
								Channels[j].remove_operator(&Clients[l]);
								Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " -o " + cmd[3] + "\n");
								return;
							}
						}
						Server::senderror(441, Clients[i].get_nickname(), fd, " :They aren't on that channel\n");
						return;
					}
					else if (cmd[2][k] == 't')
					{
						Channels[j].set_mode('t', false);
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " -t\n");
					}
					else if (cmd[2][k] == 'i')
					{
						Channels[j].set_mode('i', false);
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " -i\n");
					}
					else if (cmd[2][k] == 'l')
					{
						Channels[j].set_mode('l', false);
						Channels[j].set_limit(-1);
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " -l\n");
					}
					else if (cmd[2][k] == 'k')
					{
						Channels[j].set_mode('k', false);
						Channels[j].set_key("");
						Server::sendresponse(324, Clients[i].get_nickname(), fd, " :" + Channels[j].get_name() + " -k\n");
					}//to complete later
				}
			}
			return;
		}
	}
	Server::senderror(403, Clients[i].get_nickname(), fd, " :No such channel\n");
}