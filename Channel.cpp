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
	this->invite_only = false;
	this->has_key = false;
	this->moderated = false;
	this->has_topic = false;
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
	this->invite_only = copy.invite_only;
	this->has_key = copy.has_key;
	this->moderated = copy.moderated;
	this->has_topic = copy.has_topic;
	this->limit = copy.limit;
	this->topic = copy.topic;
	this->name = copy.name;
	this->key = copy.key;
	this->clients = copy.clients;
	this->modes = copy.modes;
	return *this;
}

void Channel::set_invite_only(bool invite_only)
{
	this->invite_only = invite_only;
}

void Channel::set_key(std::string key)
{
	this->key = key;
	this->has_key = true;
}

void Channel::set_operator(Client *cli)
{
	this->moderated = true;
	for (std::deque<Client*>::iterator it = this->clients.begin(); it != this->clients.end(); it++)
	{
		if ((*it)->getfd() == cli->getfd())
		{
			(*it)->set_oper(true);
			break;
		}
	}
}

void Channel::set_topic(std::string topic)
{
	this->topic = topic;
	this->has_topic = true;
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
	this->clients.push_back(cli);
}

void Channel::remove_client(Client *cli)
{
	std::deque<Client*>::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it)->getfd() == cli->getfd())
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
	std::deque<Client*>::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it)->getfd() == cli->getfd())
		{
			if ((*it)->get_oper() == false)
			{
				std::cout << "Client is not an operator\n";
				return;
			}
			(*it)->set_oper(false);
			break;
		}
		it++;
	}
}

bool Channel::get_invite_only() const
{
	return this->invite_only;
}

bool Channel::get_has_key() const
{
	return this->has_key;
}

bool Channel::get_has_topic() const
{
	return this->has_topic;
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

bool Channel::get_moderated() const
{
	return this->moderated;
}

std::deque<Client*> Channel::get_clients() const
{
	return this->clients;
}

std::map<char, bool> Channel::get_modes() const
{
	return this->modes;
}
