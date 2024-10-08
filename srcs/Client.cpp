#include "../inc/Client.hpp"
#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"

Client::Client() : is_auth(false), has_nick(false), has_user(false), has_pass(false)
{
}

Client::~Client()
{
}

void	Client::reset()
{
	this->is_auth = false;
	this->has_nick = false;
	this->has_user = false;
	this->has_pass = false;
	this->username.clear();
	this->nickname.clear();
	this->ip.clear();
}

Client::Client(int fd, std::string ip) :fd(fd), ip(ip), is_auth(false), has_nick(false), has_user(false), has_pass(false)
{
}

int Client::getfd() const
{
	return this->fd;
}

bool Client::get_auth() const
{
	return this->is_auth;
}

bool Client::get_has_nick() const
{
	return this->has_nick;
}

bool Client::get_has_user() const
{
	return this->has_user;
}

bool Client::get_has_pass() const
{
	return this->has_pass;
}

void Client::set_has_pass(bool has_pass)
{
	this->has_pass = has_pass;
}

void Client::set_auth(bool auth)
{
	this->is_auth = auth;
}

void Client::set_has_nick(bool nick)
{
	this->has_nick = nick;
}

void Client::set_has_user(bool user)
{
	this->has_user = user;
}

std::string Client::get_ip() const
{
	return this->ip;
}

std::string Client::get_username() const
{
	return this->username;
}

std::string Client::get_nickname() const
{
	return this->nickname;
}

void Client::set_username(std::string username)
{
	this->username = username;
}

void Client::set_nickname(std::string nickname)
{
	this->nickname = nickname;
}

void Client::set_ip(std::string ip)
{
	this->ip = ip;
}

Client::Client(const Client &copy)
{
	this->fd = copy.fd;
}

Client &Client::operator=(const Client &copy)
{
	this->fd = copy.fd;
	return *this;
}
