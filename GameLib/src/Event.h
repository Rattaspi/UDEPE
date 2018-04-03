#pragma once
#include <SFML\Network.hpp>
class Event {
	sf::IpAddress adress;
	unsigned short port;
public:
	sf::Packet packet;

	Event(sf::Packet p, sf::IpAddress ip, unsigned short port) {
		packet = p;
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