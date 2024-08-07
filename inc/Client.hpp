#pragma once
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <deque>
#include <algorithm>

class Channel;

class Client
{
private:
	int						fd;
	std::string 			ip;
	bool					is_auth;
	bool					has_nick;
	bool					has_user;
	bool					has_pass;
	std::string				username;
	std::string				nickname;

public:
	Client();
	Client(int fd, std::string ip);
	Client(const Client &copy);
	Client &operator=(const Client &copy);
	int						getfd() const;
	bool					get_auth() const;
	bool					get_has_nick() const;
	bool					get_has_user() const;
	bool					get_has_pass() const;
	void					reset();
	void					set_auth(bool auth);
	void					set_has_pass(bool pass);
	void					set_has_nick(bool nick);
	void					set_has_user(bool user);
	std::string				get_username() const;
	std::string				get_nickname() const;
	std::string				get_ip() const;
	void					set_username(std::string username);
	void					set_nickname(std::string nickname);
	void					set_ip(std::string ip);
	~Client();
};
