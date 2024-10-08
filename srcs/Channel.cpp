#include "../inc/Channel.hpp"
#include "../inc/Client.hpp"
#include "../inc/Server.hpp"

Channel::Channel()
{
}

Channel::~Channel()
{
}

Channel::Channel(std::string name) : name(name)
{
	this->limit = -1;
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

void Channel::set_mode(char mode, bool value)
{
	this->modes[mode] = value;
}

void Channel::set_limit(size_t limit)
{
	this->limit = limit;
}

void Channel::add_client(Client *cli)
{
    this->clients[cli] = false;
	if (this->clients.size() == 1)
		this->set_operator(cli);
	std::string msg = ":" + cli->get_nickname() + "!~" + cli->get_username() + "@" + cli->get_ip() + " JOIN :" + this->name + "\r\n";
    Server::broadcastmsg(msg, *this);
	Server::ch_join_message(*cli, *this);
}

void Channel::remove_client(Client *cli)
{
	std::map<Client*, bool >::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it).first->getfd() == cli->getfd())
		{
			Server::broadcastmsg(":" + cli->get_nickname() + "!~" + cli->get_username() + "@" + cli->get_ip() + " PART " + this->name + "\r\n", *this);
			this->clients.erase(it);
			return;
		}
		it++;
	}
	Server::sendmsg(cli->getfd(), ":" + Server::get_servername() + " 442 " + cli->get_nickname() + " " + this->name + " :You're not on that channel\r\n");
}

void Channel::remove_operator(Client *cli)
{
	std::map<Client*, bool>::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it).first->getfd() == cli->getfd())
		{
			if ((*it).second == false)
				return;
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

size_t Channel::get_limit() const
{
	return this->limit;
}

std::map<Client*, bool> *Channel::get_clients() const
{
	return (std::map<Client*, bool>*)&this->clients;
}

std::map<char, bool> Channel::get_modes() const
{
	return this->modes;
}

std::deque<Client*> Channel::get_invite_list() const
{
	return this->invite_list;
}

void Channel::add_invite(Client *cli)
{
	for(std::deque<Client*>::iterator it = this->invite_list.begin(); it != this->invite_list.end(); it++)
		if ((*it)->get_nickname() == cli->get_nickname())
			return;
	this->invite_list.push_back(cli);
}

void Channel::remove_invite(Client *cli)
{
	this->invite_list.erase(std::remove(this->invite_list.begin(), this->invite_list.end(), cli), this->invite_list.end());
}
