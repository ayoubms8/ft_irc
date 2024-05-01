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
	void			Serverinit(int port, std::string password);
	void			AcceptNewClient();
	void			ReceiveNewData(int fd);
	void			execute(int fd, std::vector<std::string> cmd);
	static void		SignalHandler(int signum);
	void			Closefds();
	void			rmclient(int fd);
	static void		senderror(int code, std::string clientname, int fd, std::string msg);
	static void 	sendresponse(int code, std::string clientname, int fd, std::string msg);
	static void		sendWelcomeMessages(Client &cli);
	static void 	ch_join_message(Client &cli, Channel channel);
	static void 	broadcastmsg(std::string msg, Channel &channel);
	static void		sendmsg(int fd, std::string msg);
	void			show_clients();

	bool			authentication(int fildD, std::vector<std::string> command);
	bool			isCompared(std::string const &str1, std::string const &str2);
	static bool		is_op_in(Client &cli, Channel &channel);
	void			ft_pass(int fd, std::vector<std::string> cmd);
	void			ft_nick(int fd, std::vector<std::string> cmd);
	void			ft_user(int fd, std::vector<std::string> cmd);
	void			ft_quit(int fd, std::vector<std::string> cmd);
	void			join(int fd, std::vector<std::string> cmd, int i);
	void			invite_only_join(int fd, std::vector<std::string> cmd, Client &cli, Channel &channel);
	void			part(int fd, std::vector<std::string> cmd, int i);
	void			privmsg(int fd, std::vector<std::string> cmd, int i);
	void			topic(int fd, std::vector<std::string> cmd, int i);
	void			mode(int fd, std::vector<std::string> cmd, int i);
	void			kick(int fd, std::vector<std::string> cmd, int i);
	void			invite(int fd, std::vector<std::string> cmd, int i);
	Client			&get_client_by_fd(std::deque<Client> &clients, int fd);
	Server();
	Server(const Server &copy);
	Server &operator=(const Server &copy);	
	~Server();
};
