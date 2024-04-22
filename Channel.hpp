#pragma once
#include <iostream>
#include <vector>
#include <deque>
#include <map>

class Client;
class Channel
{
private:
	bool invite_only;
	bool has_key;
	bool moderated;
	bool has_topic;
	int limit;
	std::string topic;
	std::string name;
	std::string key;
	std::deque<Client*> clients;
	std::map<char, bool> modes;
public:
	Channel();
	Channel(std::string name);
	Channel(const Channel &copy);
	Channel &operator=(const Channel &copy);
	void set_invite_only(bool invite_only);
	void set_key(std::string key);
	void set_operator(Client *cli);
	void set_topic(std::string topic);
	void set_modes(std::map<char, bool> modes);
	void set_limit(int limit);
	void add_client(Client *cli);
	void remove_client(Client *cli);
	void remove_operator(Client *cli);
	bool get_invite_only() const;
	bool get_has_key() const;
	bool get_has_topic() const;
	std::string get_key() const;
	std::string get_topic() const;
	std::string get_name() const;
	int get_limit() const;
	bool get_moderated() const;
	std::deque<Client*> get_clients() const;
	std::deque<Client*> get_operators() const;
	std::map<char, bool> get_modes() const;
	~Channel();
};
