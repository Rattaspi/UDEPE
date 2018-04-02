#pragma once
#include <iostream>

class Client {
private:
	unsigned short port;
	std::string ip;
	unsigned int id;
public:
	Client(std::string ip, unsigned short port, unsigned int id) {
		this->ip = ip;
		this->port = port;
		this->id = id;
	}

	unsigned short GetPort() {
		return port;
	}
	std::string GetIP() {
		return ip;
	}
	unsigned int GetID() {
		return id;
	}
};