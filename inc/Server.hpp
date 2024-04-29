#pragma once

#include "Client.hpp"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <deque>

class Server
{
private:
	int							Port;
	std::string					password;
	static bool					Signal;
	std::deque<Client>			Clients;
	std::vector<Channel>		Channels;
	std::vector<struct pollfd>	pollfds;
	static std::string			creationdate;
public:
	void	Serverinit(int port, std::string password);
	void	AcceptNewClient();
	void	ReceiveNewData(int fd);
	void	execute(int fd, std::vector<std::string> cmd);
	static void	SignalHandler(int signum);
	void	Closefds();
	void	rmclient(int fd);
	static void	senderror(int code, std::string clientname, int fd, std::string msg);
	static void sendresponse(int code, std::string clientname, int fd, std::string msg);
	static void	sendWelcomeMessages(Client &cli);
	static void ch_join_message(Client *cli, Channel *channel);
	static void	sendmsg(int fd, std::string msg);
	void	show_clients();
	Server();
	Server(const Server &copy);
	Server &operator=(const Server &copy);

	void	authenticate(int fd, std::vector<std::string> cmd);
	void	nick(int fd, std::vector<std::string> cmd);
	void	user(int fd, std::vector<std::string> cmd);
	void	quit(int fd, std::vector<std::string> cmd);
	void	join(int fd, std::vector<std::string> cmd, int i);
	void	invite_only_join(int fd, std::vector<std::string> cmd, Client &cli, Channel &channel);
	void	part(int fd, std::vector<std::string> cmd, int i);
	void	privmsg(int fd, std::vector<std::string> cmd, int i);
	void	topic(int fd, std::vector<std::string> cmd, int i);
	void	mode(int fd, std::vector<std::string> cmd, int i);
	void	kick(int fd, std::vector<std::string> cmd, int i);
	void	invite(int fd, std::vector<std::string> cmd, int i);
	~Server();
};
