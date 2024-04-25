#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel()
{
}

Channel::~Channel()
{
}

Channel::Channel(std::string name) : name(name)
{
	this->limit = -1;
	this->modes.insert(std::pair<char, bool>('o', false));
    this->modes.insert(std::pair<char, bool>('i', false));
    this->modes.insert(std::pair<char, bool>('t', false));
    this->modes.insert(std::pair<char, bool>('l', false));
    this->modes.insert(std::pair<char, bool>('k', false));
}

Channel::Channel(const Channel &copy)
{
	*this = copy;
}

Channel &Channel::operator=(const Channel &copy)
{
	this->limit = copy.limit;
	this->topic = copy.topic;
	this->name = copy.name;
	this->key = copy.key;
	this->clients = copy.clients;
	this->modes = copy.modes;
	this->invite_list = copy.invite_list;
	return *this;
}

void Channel::set_key(std::string key)
{
	this->key = key;
}

void Channel::set_operator(Client *cli)
{
	for (std::map<Client*, bool>::iterator it = this->clients.begin(); it != this->clients.end(); it++)
	{
		if ((*it).first->getfd() == cli->getfd())
		{
			(*it).second = true;
			break;
		}
	}
}

void Channel::set_topic(std::string topic)
{
	this->topic = topic;
}

void Channel::set_modes(std::map<char, bool> modes)
{
	this->modes = modes;
}

void Channel::set_limit(int limit)
{
	this->limit = limit;
}

void Channel::add_client(Client *cli)
{
	this->clients[cli] = false;
}

void Channel::remove_client(Client *cli)
{
	std::map<Client*, bool >::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it).first->getfd() == cli->getfd())
		{
			this->clients.erase(it);
			std::cout << "Client removed from channel\n";
			break;
		}
		it++;
	}
}

void Channel::remove_operator(Client *cli)
{
	std::map<Client*, bool>::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it).first->getfd() == cli->getfd())
		{
			if ((*it).second == false)
			{
				std::cout << "Client is not an operator\n";
				return;
			}
			(*it).second = false;
			break;
		}
		it++;
	}
}

std::string Channel::get_key() const
{
	return this->key;
}

std::string Channel::get_topic() const
{
	return this->topic;
}

std::string Channel::get_name() const
{
	return this->name;
}

int Channel::get_limit() const
{
	return this->limit;
}

std::map<Client*, bool> Channel::get_clients() const
{
	return this->clients;
}

std::map<char, bool> Channel::get_modes() const
{
	return this->modes;
}

std::vector<std::string> Channel::get_invite_list() const
{
	return this->invite_list;
}

void Channel::add_invite(std::string invite)
{
	this->invite_list.push_back(invite);
}

void Channel::remove_invite(std::string invite)
{
	std::vector<std::string>::iterator it = this->invite_list.begin();
	while (it != this->invite_list.end())
	{
		if (*it == invite)
		{
			this->invite_list.erase(it);
			break;
		}
		it++;
	}
}
