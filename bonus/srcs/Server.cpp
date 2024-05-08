#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Bot.hpp"
#include <sstream>
#include <arpa/inet.h>

bool Server::Signal = false;
std::string Server::creationdate;
std::string Server::servername;

std::string	Server::get_servername()
{
	return Server::servername;
}

Client *Server::get_client_by_fd(std::deque<Client *> &clients, int fd)
{
	for (size_t i = 0; i < clients.size(); i++)
	{
		if (clients[i]->getfd() == fd)
			return clients[i];
	}
	throw std::runtime_error("CLIENT NOT FOUND");
}

Server::Server()
{
	std::time_t now = std::time(0);
    std::tm* localtm = std::localtime(&now);
    char buffer[80];
    std::strftime(buffer, 80, "%a %b %d %Y", localtm);
    Server::creationdate = buffer;
}

Server::~Server()
{
	Closefds();
	for (size_t i = 0; i < Clients.size(); i++)
	{
		delete Clients[i];
	}
}

void	Server::show_clients()
{
	std::cout << "Clients connected: " << Clients.size() << std::endl;
	for (size_t i = 0; i < Clients.size(); i++)
	{
		std::cout << "Client " << i + 1 << " fd: " << Clients[i]->getfd()  << " username : " << Clients[i]->get_username() << " pass: " << Clients[i]->get_has_pass() << " nickname: " << Clients[i]->get_nickname() << " auth: " << Clients[i]->get_auth() << std::endl;
		std::cout << "has_user: " << Clients[i]->get_has_user() << " has_nick: " << Clients[i]->get_has_nick() << std::endl;
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
	if (ser_pol_fd.fd < 0)
		throw (std::runtime_error("FAILED TO CREATE SOCKET"));
	int en = 1;
	if (setsockopt(ser_pol_fd.fd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)))
		throw(std::runtime_error("FAILED TO MAKE ADDRESS REUSABLE"));
	if (fcntl(ser_pol_fd.fd, F_SETFL, O_NONBLOCK) < 0)
		throw(std::runtime_error("FAILED TO MAKE SOCKET NON-BLOCKING"));
	if (bind(ser_pol_fd.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		throw(std::runtime_error("FAILED to bind socket"));
	if (listen(ser_pol_fd.fd, SOMAXCONN) < 0)
		throw(std::runtime_error("listen() FAILED"));
	ser_pol_fd.events = POLLIN;
	ser_pol_fd.revents = 0;
	this->pollfds.push_back(ser_pol_fd);
	this->servername = inet_ntoa(addr.sin_addr);
	std::cout << "Waiting to accept a connection...\n";
	Bot bot;
	if (bot.connect_to_server((struct sockaddr*)&addr, this->Port, this->password))
		bot.authenticate();
	this->pollfds.push_back(bot.get_fd());
	while (Server::Signal == false)
	{
		if((poll(pollfds.data(), pollfds.size(), -1) < 0) && Server::Signal == false)
			throw(std::runtime_error("poll() FAILED"));	
		for (size_t i = 0; i < pollfds.size(); i++)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (i == 0)
					AcceptNewClient();
				else if (i == 1)
					bot.receive_message();
				else
					ReceiveNewData(pollfds[i].fd);
			}
		}
		//show_clients();
	}
}

void	Server::AcceptNewClient()
{
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	struct pollfd new_pol_fd;

	new_pol_fd.fd = accept(pollfds[0].fd, (struct sockaddr *)&addr, &addr_len);
	if (new_pol_fd.fd < 0)
		throw(std::runtime_error("accept() FAILED"));
	if (fcntl(new_pol_fd.fd, F_SETFL, O_NONBLOCK) < 0)
		throw(std::runtime_error("FAILED TO MAKE SOCKET NON-BLOCKING"));
	new_pol_fd.events = POLLIN;
	new_pol_fd.revents = 0;
	this->pollfds.push_back(new_pol_fd);
	Client *new_client = new Client(new_pol_fd.fd, inet_ntoa(addr.sin_addr));
	this->Clients.push_back(new_client);
}

int	count_args(std::string str)
{
	int args = 0;
	std::string::size_type pos = 0;
	std::string::size_type last_arg = 0;
	if ((last_arg = str.find(":")) != std::string::npos)
	{
		args++;
		str.erase(last_arg);
	}
	while ((pos = str.find(" ")) != std::string::npos)
	{
		while (str[pos] == ' ')
			pos++;
		str.erase(0, pos);
		args++;
	}
	return (args + 1);
}

std::vector<std::string> parse_cmd(std::string str)
{
    std::vector<std::string> cmd;
    std::string::size_type pos = 0;

    if ((pos = str.find("\r\n")) != std::string::npos)
        str.erase(pos, 2);
	else if ((pos = str.find("\n")) != std::string::npos)
		str.erase(pos, 1);
    if (str[0] == ':')
    {
        pos = str.find(" ");
        cmd.push_back(str.substr(1, pos - 1));
        str.erase(0, pos + 1);
    }
    while ((pos = str.find(" ")) != std::string::npos)
    {
        if (str[0] == ':')
        {
            cmd.push_back(str.substr(1));
            break;
        }
        else
        {
            cmd.push_back(str.substr(0, pos));
            str.erase(0, pos + 1);
        }
    }
    if (!str.empty())
        cmd.push_back(str);
	cmd.push_back("");
    return cmd;
}

void Server::senderror(int code, std::string clientname, int fd, std::string msg)
{
	std::stringstream ss;
	ss << ":" + Server::get_servername() + " " << code << " " << clientname << msg;
	std::string resp = ss.str();
	if(send(fd, resp.c_str(), resp.size(),0) < 0)
		std::cerr << "send() FAILED" << std::endl;
}

void Server::sendresponse(int code, std::string clientname, int fd, std::string msg)
{
	std::stringstream ss;
	ss << ":" + Server::get_servername() + " " << code << " " << clientname << msg;
	std::string resp = ss.str();
	if(send(fd, resp.c_str(), resp.size(),0) < 0)
		std::cerr << "send() FAILED" << std::endl;
}

void Server::sendmsg(int fd, std::string msg)
{
	if(send(fd, msg.c_str(), msg.size(),0) < 0)
		std::cerr << "send() FAILED" << std::endl;
}
bool	isCompared(std::string const &str1, std::string const &str2)
{
	if (str1.length() != str2.length())
		return false;
	for (size_t i = 0; i < str1.length(); i++)
	{
		if (std::toupper(str1[i]) != std::toupper(str2[i]))
			return false;
	}
	return true;
}

void	Server::execute(int fildD, std::vector<std::string> command)
{
	// kick join part privmsg topic mode user nick pass quit invite
	size_t i = -1;
	while (++i < Clients.size())
		if (Clients[i]->getfd() == fildD)
			break ;
	if (isCompared(command[0], "pong"))
		return ;
	else if (isCompared(command[0], "pass"))
		ft_pass(fildD, command);
	else if (isCompared(command[0], "nick"))
		ft_nick(fildD, command);
	else if (isCompared(command[0], "user"))
		ft_user(fildD, command);
	else if (isCompared(command[0], "quit"))
		ft_quit(fildD, command);
	else if (!Clients[i]->get_auth())
	{
		Server::senderror(451, Clients[i]->get_nickname(), fildD, " :You have not registered\n");
		return;
	}
	else if (isCompared(command[0], "join"))
		join(fildD, command, i);
	else if (isCompared(command[0], "part"))
		part(fildD, command, i);
	else if (isCompared(command[0], "privmsg"))
		privmsg(fildD, command, i);
	else if (isCompared(command[0], "topic"))
		topic(fildD, command, i);
	else if (isCompared(command[0], "mode"))
		mode(fildD, command, i);
	else if (isCompared(command[0], "kick"))
		kick(fildD, command, i);
	else if (isCompared(command[0], "invite"))
		invite(fildD, command, i);
	else
		Server::senderror(421, Clients[i]->get_nickname(), fildD, " " + command[0] + " :Unknown command\n");
}

void	Server::ReceiveNewData(int fd)
{
	char buffer[1024];
	std::string::size_type pos;
	int ret = recv(fd, buffer, 1024, 0);
	if (ret < 0)
		throw(std::runtime_error("recv() FAILED"));
	if (ret == 0)
	{
		rmclient(fd);
	}
	else
	{
		buffer[ret] = '\0';
		std::string str(buffer);
		pos = str.find("\r\n");
		while (pos != std::string::npos)
		{
			std::string temp = str.substr(0, pos);
			std::vector<std::string> cmd = parse_cmd(temp);
			this->execute(fd, cmd);
			str.erase(0, pos + 2);
			pos = str.find("\r\n");
		}
		pos = str.find("\n");
		while (pos != std::string::npos)
		{
			std::string temp = str.substr(0, pos);
			std::vector<std::string> cmd = parse_cmd(temp);
			this->execute(fd, cmd);
			str.erase(0, pos + 2);
			pos = str.find("\n");
		}
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
		if (Clients[i]->getfd() == fd)
		{
			for (size_t j = 0; j < Channels.size(); j++)
			{
				if (is_in_channel(*Clients[i], Channels[j]))
					Channels[j].remove_client(Clients[i]);
				if (Channels[j].get_clients()->empty() || (Channels[j].get_clients()->size() == 1 && Channels[j].get_clients()->begin()->first->get_nickname() == "Botto"))
					Channels.erase(Channels.begin() + j);
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

bool	Server::is_op_in(Client &cli, Channel &channel)
{
	std::map<Client *, bool> *clis = channel.get_clients();
	for (std::map<Client *, bool>::iterator it = clis->begin(); it != clis->end(); it++)
	{
		if ((*it).first->getfd() == cli.getfd() && (*it).second == true)
			return (true);
	}
	return (false);
}