#include <iostream>
#include <queue>
#include <thread>
#include <SFML\Network.hpp>

void ReceptionThread(bool* end, std::queue<sf::Packet>* incomingInfo, sf::UdpSocket* socket);

int main() {
	std::cout << "CLIENTE INICIADO" << std::endl;
	sf::IpAddress serverIP = sf::IpAddress::getLocalAddress();
	int serverPort = 50000;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	sf::Packet infoToSend;
	std::string command;
	unsigned int myID;
	bool end = false; //finalizar programa
	bool connected = false; //controla cuando se ha conectado con el servidor
	std::queue<sf::Packet> incomingInfo; //cola de paquetes entrantes
	//std::queue<std::string> commands; //cola de comandos entrantes

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);

	infoToSend << "HELLO_";
	status = socket->send(infoToSend, serverIP, serverPort);
	if (status != sf::Socket::Done) {
		std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
	}
	else {
		std::cout << "Intentando conectar al servidor..." << std::endl;
	}

	sf::Clock clock;
	while (!end) {
		//CONTROL DE REENVIOS POR HASTA QUE EL SERVIDOR NOS CONFIRMA LA CONEXION
		if (clock.getElapsedTime().asMilliseconds() > 500 && !connected) {
			infoToSend.clear();
			infoToSend << "HELLO_";
			status = socket->send(infoToSend, serverIP, serverPort);
			if (status != sf::Socket::Done) {
				std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
			}
			else {
				std::cout << "Intentando conectar al servidor..." << std::endl;
			}
			clock.restart();
		}

		if (!incomingInfo.empty()) {
			std::string command;
			sf::Packet inc;
			inc = incomingInfo.front();
			inc >> command;
			if (command == "WELCOME_") {
				inc >> myID;
				std::cout << "Conectado al servidor" << std::endl;
				std::cout << "Mi ID es--> " << myID << std::endl;
				connected = true;
			}
			incomingInfo.pop();
		}

	}
	

	system("pause");
	t.join();
	delete socket;
	return 0;
}

void ReceptionThread(bool* end, std::queue<sf::Packet>* incomingInfo, sf::UdpSocket* socket) {
	sf::Socket::Status status;
	socket->setBlocking(true);
	while (!*end) {
		sf::Packet inc;
		sf::IpAddress incomingIP;
		unsigned short incomingPort;
		status = socket->receive(inc, incomingIP, incomingPort);
		if (status != sf::Socket::Done) {
			std::cout << "Error al recibir informacion" << std::endl;
		}
		else {
			std::cout << "Paquete recibido correctamente" << std::endl;
			incomingInfo->push(inc);
			//incomingInfo >> command;
			//if (command == "WELCOME_") {
			//	incomingInfo >> myID;
			//	std::cout << "Conectado al servidor" << std::endl;
			//	std::cout << "Mi ID es--> " << myID << std::endl;
			//}
		}
	}
}