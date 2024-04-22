#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"

Client::Client() : is_auth(false), is_nick(false), is_user(false), is_oper(false)
{
}

Client::~Client()
{
}

void	Client::reset()
{
	std::cout << "resetting client\n";
	this->is_auth = false;
	this->is_nick = false;
	this->is_user = false;
	this->is_oper = false;
	this->has_pass = false;
	this->username.clear();
	this->realname.clear();
	this->nickname.clear();
	this->channels.clear();
}

Client::Client(int fd, struct sockaddr_in addr, socklen_t addr_len) : is_auth(false), is_nick(false), is_user(false), is_oper(false)
{
	this->fd = fd;
	this->addr = addr;
	this->addr_len = addr_len;
}

int Client::getfd() const
{
	return this->fd;
}

bool Client::get_auth() const
{
	return this->is_auth;
}

bool Client::get_nick() const
{
	return this->is_nick;
}

bool Client::get_user() const
{
	return this->is_user;
}

bool Client::get_oper() const
{
	return this->is_oper;
}

bool Client::get_pass() const
{
	return this->has_pass;
}

void Client::set_pass(bool has_pass)
{
	this->has_pass = has_pass;
}

std::vector<Channel*> Client::get_channels() const
{
	return this->channels;
}

void Client::leave_all_channels()
{
	for (size_t i = 0; i < this->channels.size(); i++)
	{
		this->channels[i]->remove_client(this);
	}
	this->channels.clear();
}

void Client::join_channel(Channel *channel)
{
	for (size_t i = 0; i < this->channels.size(); i++)
	{
		if (this->channels[i]->get_name() == channel->get_name()) // if already in channel
		{
			Server::senderror(443, this->nickname, this->fd, " :is already in channel\n");
			return;
		}
	}
	channel->add_client(this);
	this->channels.push_back(channel);
	Server::sendresponse(331, this->nickname, this->fd, " :Channel joined\n");
}

void Client::leave_channel(std::string channel_name)
{
	for (std::vector<Channel*>::iterator it = this->channels.begin(); it != this->channels.end(); it++)
	{
		if ((*it)->get_name() == channel_name)
		{
			(*it)->remove_client(this);
			this->channels.erase(it);
			Server::sendresponse(331, this->nickname, this->fd, " :Channel left\n");
			return;
		}
	}
	Server::senderror(442, this->nickname, this->fd, " :You're not on that channel\n");
}

void Client::set_auth(bool auth)
{
	this->is_auth = auth;
}

void Client::set_nick(bool nick)
{
	this->is_nick = nick;
}

void Client::set_user(bool user)
{
	this->is_user = user;
}

void Client::set_oper(bool oper)
{
	this->is_oper = oper;
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

std::string Client::get_realname() const
{
	return this->realname;
}

void Client::set_realname(std::string realname)
{
	this->realname = realname;
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
