#include <iostream>
#include <queue>
#include <thread>
#include <SFML\Network.hpp>
#include <Client.h>
#include "Utils.h"
#include "Event.h"

void ReceptionThread(bool* end, std::queue<Event>* incomingInfo, sf::UdpSocket* socket);

bool ClientExists(std::vector<Client*>aClients,int id);

int main() {
	std::cout << "CLIENTE INICIADO" << std::endl;
	std::string serverIp = "localhost";
	int serverPort = 50000;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	sf::Packet infoToSend;
	std::string command;
	int myId = 0;
	std::vector<int>acks;
	std::pair<short, short> myCoordenates{ 0,0 };
	bool end = false; //finalizar programa
	bool connected = false; //controla cuando se ha conectado con el servidor
	std::queue<Event> incomingInfo; //cola de paquetes entrantes
	std::vector<Client*> aClients;
	sf::Clock clockForTheServer;

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);

	sf::Clock clock;
	while (!end) {
		//CONTROL DE REENVIOS POR HASTA QUE EL SERVIDOR NOS CONFIRMA LA CONEXION
		if (clock.getElapsedTime().asMilliseconds() > 500 && !connected) {
			OutputMemoryBitStream ombs;
			ombs.Write(PacketType::HELLO, commandBits);
			status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(),serverIp,serverPort);

			if (status != sf::Socket::Done) {
				std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
			}
			else if(status==sf::Socket::Done){
				std::cout << "Intentando conectar al servidor..." << std::endl;
			}
			clock.restart();
		}

		if (!incomingInfo.empty()) {
			Event inc;
			inc = incomingInfo.front();
			Event infoReceived;
			PacketType command = HELLO;
			infoReceived = incomingInfo.front();
			InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
			imbs.Read(&command, commandBits);
			int playerSize = 0;
			int aCriticalId = 0;
			OutputMemoryBitStream ombs;

			switch (command) {
			case PacketType::WELCOME:
				imbs.Read(&myId,playerSizeBits);
				imbs.Read(&playerSize, playerSizeBits);
				playerSize++;
				for (int i = 0; i < playerSize; i++) {
					Client* aClient = new Client();

					imbs.Read(&aClient->id, playerSizeBits);
					imbs.Read(&aClient->position.first, coordsbits);
					imbs.Read(&aClient->position.second, coordsbits);

					if (aClient->id != myId&&myId>0) {
						std::cout << "Recibiendo cliente preexistente con id " << aClient->id << " y coordenadas " << aClient->position.first << "," << aClient->position.second << std::endl;
						aClients.push_back(aClient);
					}
					else {
						myCoordenates = aClient->position;
						delete aClient;
					}
				}

				connected = true;
				std::cout << "MY ID IS " << myId << std::endl;

				break;
			case PacketType::PING:
				
				ombs.Write(PacketType::ACKPING, commandBits);
				socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
				clockForTheServer.restart();
				break;
			case PacketType::NEWPLAYER:
				for (int i = 0; i < 1; i++) {
					Client* newPlayer = new Client();
					imbs.Read(&aCriticalId, criticalBits);
					std::cout << "Reiciving Critical ID " << aCriticalId<<std::endl;
					imbs.Read(&newPlayer->id, playerSizeBits);
					imbs.Read(&newPlayer->position.first, coordsbits);
					imbs.Read(&newPlayer->position.second, coordsbits);
					acks.push_back(aCriticalId);

					if (GetClientWithId(newPlayer->id, aClients) == nullptr) {
						aClients.push_back(newPlayer);
						std::cout << "Adding player with id " << newPlayer->id << std::endl;
					}
					else {
						std::cout << "Already existing player received\n";
						delete newPlayer;
					}
				}
				break;
			case PacketType::DISCONNECT:
				for (int i = 0; i < 1; i++) {
					int leaverId=0;
					imbs.Read(&aCriticalId, criticalBits);
					imbs.Read(&leaverId, playerSizeBits);

					acks.push_back(aCriticalId);
					if (GetClientWithId(aCriticalId,aClients) != nullptr) {
						std::cout << "Player " << leaverId << " Disconnected\n";
					}
					else {
						std::cout << "Trying to disconnect non existing player with id " << leaverId << std::endl;
					}


				}
				break;
			}

			incomingInfo.pop();
		}

		if (acks.size() > 0) {
			for (int i = 0; i < acks.size(); i++) {
				OutputMemoryBitStream ombs;
				ombs.Write(PacketType::ACK, commandBits);
				ombs.Write(acks[i], criticalBits);
				std::cout << "Sending ack " << acks[i] << std::endl;
				socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
			}
			acks.clear();
		}

		if (clockForTheServer.getElapsedTime().asMilliseconds() > 30000) {
			end = true;
			std::cout << "SERVIDOR DESCONECTADOOOO\n";
			socket->unbind();
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

void ReceptionThread(bool* end, std::queue<Event>* incomingInfo, sf::UdpSocket* socket) {
	sf::Socket::Status status;
	while (!*end) {
		sf::IpAddress incomingIP;
		unsigned short incomingPort;
		char message[maxBufferSize];
		size_t sizeReceived;

		status = socket->receive(message, maxBufferSize, sizeReceived, incomingIP, incomingPort);

		if (status == sf::Socket::Error) {
			std::cout << "Error al recibir informacion" << std::endl;
		}
		else {
			//std::cout << "Paquete recibido correctamente" << std::endl;
			incomingInfo->push(Event(message, sizeReceived, incomingIP, incomingPort));

		}
	}
}