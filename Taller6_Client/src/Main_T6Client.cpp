#include <iostream>
#include <SFML\Network.hpp>

int main() {
	std::cout << "CLIENTE INICIADO" << std::endl;
	sf::IpAddress serverIP = sf::IpAddress::getLocalAddress();
	int serverPort = 50000;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	sf::Packet infoToSend, incomingInfo;
	std::string command;
	unsigned int myID;

	infoToSend << "HELLO_";
	status = socket->send(infoToSend, serverIP, serverPort);
	if (status != sf::Socket::Done) {
		std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
	}
	else {
		std::cout << "Intentando conecar al servidor" << std::endl;
	}

	sf::IpAddress incomingIP;
	unsigned short incomingPort;
	status = socket->receive(incomingInfo, incomingIP, incomingPort);
	if (status != sf::Socket::Done) {
		std::cout << "Error al recibir informacion" << std::endl;
	}
	else {
		incomingInfo >> command;
		if (command == "WELCOME_") {
			incomingInfo >> myID;
			std::cout << "Conectado al servidor" << std::endl;
			std::cout << "Mi ID es--> " << myID << std::endl;
		}
	}

	system("pause");
	delete socket;
	return 0;
}