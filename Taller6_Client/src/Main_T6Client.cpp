#include <iostream>
#include <queue>
#include <thread>
#include <SFML\Network.hpp>
#include <Client.h>
#include "Utils.h"

void ReceptionThread(bool* end, std::queue<sf::Packet>* incomingInfo, sf::UdpSocket* socket);

bool ClientExists(std::vector<Client*>aClients,int id);

int main() {
	std::cout << "CLIENTE INICIADO" << std::endl;
	std::string serverIp = "localhost";
	int serverPort = 50000;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	sf::Packet infoToSend;
	std::vector<int>acks;
	std::string command;
	unsigned int myID;
	std::pair<float, float> myCoordenates;
	bool end = false; //finalizar programa
	bool connected = false; //controla cuando se ha conectado con el servidor
	std::queue<sf::Packet> incomingInfo; //cola de paquetes entrantes
	std::vector<Client*> aClients;
	sf::Clock clockForTheServer;
	//std::queue<std::string> commands; //cola de comandos entrantes

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);


	infoToSend << "HELLO_";
	do {
		status = socket->send(infoToSend, serverIp, serverPort);
	} while (status == sf::Socket::Partial);
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
			do {
				status = socket->send(infoToSend, serverIp, serverPort);
			} while (status == sf::Socket::Partial);
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
				int otherPlayers;
				inc >> myID;
				inc >> otherPlayers;

				for (int j = 0; j < otherPlayers; j++) {
					int anId;
					std::pair<float, float>aPos;
					inc >> anId;
					inc >> aPos.first;
					inc >> aPos.second;

					if (myID == anId) {
						myCoordenates = aPos;
					}
					else {
						Client* aClient = new Client(anId,aPos);
						std::cout << "Recibido jugador preexistente con ID " << anId << " y Coordenadas " << aPos.first << "," << aPos.second<<"\n";
						aClients.push_back(aClient);
					}

				}

				std::cout << "Conectado al servidor" << std::endl;
				std::cout << "Mi ID es--> " << myID << std::endl;
				std::cout << "Mi posicion es--> " << myCoordenates.first << ", " << myCoordenates.second << std::endl;

				connected = true;
			}
			else if (command == "PING_") {
				clockForTheServer.restart();
				int clientSize;
				inc >> clientSize;

				for (int i = 0; i < clientSize; i++) {
					int aPlayerId;
					inc >> aPlayerId;

					if (aPlayerId == myID) {
						inc >> myCoordenates.first;
						inc >> myCoordenates.second;
					}
					else {
						Client* temp = nullptr;
						temp = GetClientWithId(aPlayerId, &aClients);
						if (temp != nullptr) {
							inc >> temp->position.first;
							inc >> temp->position.second;
						}
						temp = nullptr;
					}
				}

				sf::Packet ackPacket;
				ackPacket << "ACKPING_";
				ackPacket << myID;
				do {
					status = socket->send(ackPacket, serverIp, serverPort);
				} while (status == sf::Socket::Partial);
				if (status == sf::Socket::Error) {
					//std::cout << "Error enviando ACKPING\n";
				}
				else if (status == sf::Socket::Done) {
					std::cout << "Ping recibido y respondido\n";
				}

			}
			else if (command == "CRITICAL_") {
				int commandId;
				inc >> commandId;
				inc >> command;

				if (command == "NEWPLAYER_") {
					int aPlayerId;
					std::pair<float, float> aPosition;
					inc >> aPlayerId;
					inc >> aPosition.first;
					inc >> aPosition.second;
					
					if (!ClientExists(aClients,aPlayerId)) {
						Client* newClient = new Client(aPlayerId,aPosition);
						std::cout << "Recibido NewPlayer con id " << aPlayerId << " en coordenadas " << aPosition.first << "," << aPosition.second << "\n";
					}
					else {
						std::cout << "Recepcion de NewPlayer ya existente\n";
					}

				}
				else if (command == "DC_") {
					int leaverId;
					inc >> leaverId;

					int index = GetIndexClientWithId(leaverId, &aClients);

					if (index != -1) {
						delete aClients.at(index);
						aClients.erase(aClients.begin() + index);
						std::cout << "Cliente con ID " << leaverId << "desconectado\n";
					}

				}

				acks.push_back(commandId);

			}
			incomingInfo.pop();
		}

		for (int i = 0; i < acks.size(); i++) {
			sf::Packet ackPacket;
			ackPacket << "ACK_";
			ackPacket << myID;
			ackPacket << acks[i];
			
			do {
				status = socket->send(ackPacket, serverIp, serverPort);
			} while (status == sf::Socket::Partial);
			if (status == sf::Socket::Status::Error) {
				std::cout << "Error enviando ACK\n";
			}
			else if (status == sf::Socket::Status::Done) {
				std::cout << "ACK " << acks[i] << "enviado\n";
			}

		}
		acks.clear();


		if (clockForTheServer.getElapsedTime().asMilliseconds() > 5000) {
			end = true;
			system("cls");
			std::cout << "SERVIDOR DESCONECTADOOOO\n";
		}

	}
	
	
	system("pause");
	t.join();
	delete socket;
	exit(0);
	return 0;
}


bool ClientExists(std::vector<Client*>aClients,int id) {
	for (int i = 0; i < aClients.size(); i++) {
		if (aClients[i]->id == id){
			return true;
		}
	}
	return false;
}

void ReceptionThread(bool* end, std::queue<sf::Packet>* incomingInfo, sf::UdpSocket* socket) {
	sf::Socket::Status status;
	socket->setBlocking(false);
	while (!*end) {
		sf::Packet inc;
		sf::IpAddress incomingIP;
		unsigned short incomingPort;
		status = socket->receive(inc, incomingIP, incomingPort);
		if (status != sf::Socket::NotReady) {
			if (status == sf::Socket::Error) {
				std::cout << "Error al recibir informacion" << std::endl;
			}
			else if (status == sf::Socket::Done) {
				std::cout << "Paquete recibido correctamente" << std::endl;
				incomingInfo->push(inc);
			}
		}
	}
}