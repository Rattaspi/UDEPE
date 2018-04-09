#pragma once
#include <SFML\Network.hpp>
class Event {
	sf::IpAddress adress;
	unsigned short port;
public:
	//sf::Packet packet;
	char* message;
	size_t messageSize;

	Event(char* message, size_t messageSize, sf::IpAddress ip, unsigned short port) {
		//packet = p;
		this->message = message;
		this->messageSize = messageSize;
		adress = ip;
		this->port = port;
	}
	Event() {

	}
	sf::IpAddress GetAdress() {
		return adress;
	}
	unsigned short GetPort() {
		return port;
	}

};