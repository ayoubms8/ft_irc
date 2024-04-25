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
	std::map<Client*, bool> clients;
	std::map<char, bool> modes;
	std::vector<std::string> invite_list;
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
	void add_invite(std::string invite);
	void remove_invite(std::string invite);
	bool get_invite_only() const;
	bool get_has_key() const;
	bool get_has_topic() const;
	bool get_moderated() const;
	int get_limit() const;
	std::string get_key() const;
	std::string get_topic() const;
	std::string get_name() const;
	std::vector<std::string> get_invite_list() const;
	std::map<Client*, bool> get_clients() const;
	std::map<char, bool> get_modes() const;
	~Channel();
};
