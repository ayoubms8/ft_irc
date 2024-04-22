#pragma once
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <deque>

class Channel;

class Client
{
private:
	int fd;
	bool is_auth;
	bool is_nick;
	bool is_user;
	bool has_pass;
	bool is_oper;
	std::string username;
	std::string realname;
	std::string nickname;
	struct sockaddr_in addr;
	socklen_t addr_len;
	std::vector<Channel*> channels;

public:
	Client();
	Client(int fd, struct sockaddr_in addr, socklen_t addr_len);
	Client(const Client &copy);
	Client &operator=(const Client &copy);
	int getfd() const;
	bool get_auth() const;
	bool get_nick() const;
	bool get_user() const;
	bool get_oper() const;
	bool get_pass() const;
	std::vector<Channel*> get_channels() const;
	void join_channel(Channel *channel);
	void leave_channel(std::string channel_name);
	void leave_all_channels();
	void reset();
	void set_auth(bool auth);
	void set_pass(bool pass);
	void set_nick(bool nick);
	void set_user(bool user);
	void set_oper(bool oper);
	std::string get_username() const;
	std::string get_nickname() const;
	std::string get_realname() const;
	void set_realname(std::string realname);
	void set_username(std::string username);
	void set_nickname(std::string nickname);
	~Client();
};
