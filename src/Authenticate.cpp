#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"

void Server::sendWelcomeMessages(Client &cli)
{
	std::string nick = cli.get_nickname();
	std::string user = cli.get_username();
	std::string host = "127.0.0.1";
	std::string servername = "127.0.0.1";
	std::string version = "1.0.0";
	std::string userModes = "";
	std::string channelModes = "iklot";
	std::string featureList = "CHANTYPES=#& PREFIX=(ov)@+ CHANMODES=o,k,l,t,i";

	Server::sendresponse(001, nick, cli.getfd(), " :Welcome to the Internet Relay Network " + nick + "!" + user + "@" + host + "\n");
	Server::sendresponse(002, nick, cli.getfd(), " :Your host is " + servername + ", running version " + version + "\n");
	Server::sendresponse(003, nick, cli.getfd(), " :This server was created " + creationdate + "\n");
	Server::sendresponse(004, nick, cli.getfd(), " :" + servername + " " + version + " " + userModes + " " + channelModes + "\n");
	Server::sendresponse(005, nick, cli.getfd(), " :" + servername + " " + featureList + " :are supported by this server" + "\r\n");
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
	if (!users.empty())
        users = users.substr(0, users.size() - 1);
	return users;
}

void Server::ch_join_message(Client &cli, Channel channel)
{
	std::string nickname = cli.get_nickname();
	std::string servername = "127.0.0.1";
	std::string channelname = channel.get_name();
	std::string users = get_users_in_channel(channel);
	std::string username = cli.get_username();
	std::string topic = channel.get_topic();
	std::string mode = "+t";
	
	std::string msg = ":" + servername + " 353 " + nickname + " = " + channelname + " :" + users + "\r\n";
	msg += ":" + servername + " 366 " + nickname + " " + channelname + " :End of /NAMES list.\r\n";
	Server::sendmsg(cli.getfd(), msg);
}

void Server::ft_pass(int fd, std::vector<std::string> cmd)
{
	Client &cli = get_client_by_fd(Clients, fd);
	if (cmd[1].empty() || cmd[1] != this->password)
	{
		Server::senderror(464, cli.get_nickname(), cli.getfd(), " :Password incorrect\n");
		return;
	}
	else
		cli.set_has_pass(true);
}

void Server::ft_nick(int fd, std::vector<std::string> cmd)
{
	Client &cli = get_client_by_fd(Clients, fd);
	if (!cli.get_has_pass())
	{
		Server::senderror(451, cli.get_nickname(), cli.getfd(), " :Enter password first\n");
		return;
	}
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
			Server::senderror(433, cmd[1], cli.getfd(), " :Nickname is already in use\n");
			return;
		}
	}
	if (cli.get_nickname() == cmd[1])
		cli.set_nickname(cmd[1]);
	else
	{
		cli.set_has_nick(true);
		cli.set_nickname(cmd[1]);
		if (!cli.get_has_user())
			return;
		if (!cli.get_auth())
		{
			cli.set_auth(true);
			Server::sendWelcomeMessages(cli);
		}
	}
}

void Server::ft_user(int fd, std::vector<std::string> cmd)
{
	Client &cli = get_client_by_fd(Clients, fd);
	if (!cli.get_has_pass())
	{
		Server::senderror(451, cli.get_nickname(), cli.getfd(), " :Enter password first\n");
		return;
	}
	if (cli.get_has_user())
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
		if (!cli.get_has_nick())
			return;
		cli.set_auth(true);
		Server::sendWelcomeMessages(cli);
	}
}

void Server::ft_quit(int fd, std::vector<std::string> cmd)
{
	(void)cmd;
	Client &cli = get_client_by_fd(Clients, fd);
	Server::sendmsg(cli.getfd(), ":" + cli.get_nickname() +"!" +cli.get_username() + "@127.0.0.1 QUIT :Client disconnected\r\n");
	for (size_t i = 0; i < Clients.size(); i++)
	{
		if (Clients[i].getfd() == fd)
		{
			// Clients[i].leave_all_channels();
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
