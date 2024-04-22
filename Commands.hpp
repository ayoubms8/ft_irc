#pragma once

#include "Client.hpp"

void	authenticate(std::deque<Client> &clients, int fd, std::string *cmd, std::string &password);
void	nick(std::deque<Client> &Clients, int fd, std::string *cmd);
void	user(std::deque<Client> &Clients, int fd, std::string *cmd);
void	quit(std::deque<Client> &Clients, int fd, std::string *cmd, std::vector<struct pollfd> &pollfds);
