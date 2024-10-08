#pragma once

#include "Client.hpp"
#include <stdexcept>
#include <fcntl.h>
#include <poll.h>
#include <cstdlib>
#include <ctime>
#include <string>

class Server
{
private:
	int							Port;
	std::string					password;
	static std::string			servername;
	static int					Signal_detected;
	std::deque<Client *>		Clients;
	std::vector<Channel>		Channels;
	std::vector<struct pollfd>	pollfds;
	static std::string			creationdate;
public:
	void			Serverinit(int port, std::string password);
	void			accept_client();
	void			receive_message(int fd);
	void			execute(int fd, std::vector<std::string> cmd);
	static void		SignalHandler(int signum);
	void			Closefds();
	void			rmclient(int fd);
	static void		senderror(int code, std::string clientname, int fd, std::string msg);
	static void		sendWelcomeMessages(Client &cli);
	static void 	ch_join_message(Client &cli, Channel channel);
	static void 	broadcastmsg(std::string msg, Channel &channel);
	static void		sendmsg(int fd, std::string msg);
	static std::string get_servername();
	void			show_clients();

	bool			authentication(int fildD, std::vector<std::string> command);
	static bool		is_op_in(Client &cli, Channel &channel);
	void			ft_pass(int fd, std::vector<std::string> cmd);
	void			ft_nick(int fd, std::vector<std::string> cmd);
	void			ft_user(int fd, std::vector<std::string> cmd);
	void			ft_quit(int fd, std::vector<std::string> cmd);
	void			join(int fd, std::vector<std::string> cmd, int i);
	void			join_channel(std::string &chnl, std::vector<std::string> &keys, size_t &k, int i, int fd);
	void			invite_only_join(int fd, Client &cli, Channel &channel);
	void			part(int fd, std::vector<std::string> cmd, int i);
	void			part_channel(const std::string &chnl, int i, int fd);
	void			privmsg(int fd, std::vector<std::string> cmd, int i);
	void			send_priv(std::string &receiver, std::string &msg, int i, int fd);
	void			topic(int fd, std::vector<std::string> cmd, int i);
	void			mode(int fd, std::vector<std::string> cmd, int i);
	void			kick(int fd, std::vector<std::string> cmd, int i);
	void			invite(int fd, std::vector<std::string> cmd, int i);
	Client 			*get_client_by_fd(std::deque<Client *> &clients, int fd);
	bool 			is_in_channel(Client &cli, Channel &channel);
	Server();
	~Server();
};

bool			isCompared(std::string const &str1, std::string const &str2);