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
	std::vector<int>acks;
	std::string command;
	unsigned int myID = 0;
	std::pair<float, float> myCoordenates{ 0,0 };
	bool end = false; //finalizar programa
	bool connected = false; //controla cuando se ha conectado con el servidor
	std::queue<Event> incomingInfo; //cola de paquetes entrantes
	std::vector<Client*> aClients;
	sf::Clock clockForTheServer;
	//std::queue<std::string> commands; //cola de comandos entrantes

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);


	//infoToSend << "HELLO_";
	//status = socket->send(infoToSend, serverIp, serverPort);
	//if (status != sf::Socket::Done) {
	//	std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
	//}
	//else {
	//	std::cout << "Intentando conectar al servidor..." << std::endl;
	//}

	sf::Clock clock;
	while (!end) {
		//CONTROL DE REENVIOS POR HASTA QUE EL SERVIDOR NOS CONFIRMA LA CONEXION
		if (clock.getElapsedTime().asMilliseconds() > 500 && !connected) {
			//infoToSend.clear();
			//infoToSend << "HELLO_";
			//status = socket->send(infoToSend, serverIp, serverPort);
			OutputMemoryBitStream ombs;
			ombs.Write(PacketType::HELLO, commandBits);
			status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(),serverIp,serverPort);

			if (status != sf::Socket::Done) {
				std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
			}
			else {
				std::cout << "Intentando conectar al servidor..." << std::endl;
			}
			clock.restart();
		}

		if (!incomingInfo.empty()) {
			//std::string command;
			//sf::Packet inc;
			Event inc;
			inc = incomingInfo.front();
			//inc >> command;
			Event infoReceived;
			PacketType command = HELLO;
			infoReceived = incomingInfo.front();
			InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
			imbs.Read(&command, commandBits);

			if (command == PacketType::WELCOME) {
				size_t otherPlayers=0;

				imbs.Read(&myID, playerSizeBits);
				//std::cout <<"ID: "<< myID << "\n";
				imbs.Read(&otherPlayers, playerSizeBits);
				otherPlayers++;
				//inc >> myID;
				//inc >> otherPlayers;

				for (int j = 0; j < otherPlayers; j++) {
					int anId=0;
					std::pair<float, float>aPos = { 0,0 };
					//inc >> anId;
					//inc >> aPos.first;
					//inc >> aPos.second;

					imbs.Read(&anId, playerSizeBits);
					imbs.Read(&aPos.first, coordsbits);
					imbs.Read(&aPos.second, coordsbits);


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
			else if (command == PacketType::PING) {

				clockForTheServer.restart();
				int clientSize=0;
				InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
				imbs.Read(&clientSize, playerSizeBits);
				clientSize++;

				//inc >> clientSize;

				for (int i = 0; i < clientSize; i++) {
					int aPlayerId;
					//inc >> aPlayerId;

					imbs.Read(&aPlayerId, playerSizeBits);


					if (aPlayerId == myID) {
						//inc >> myCoordenates.first;
						//inc >> myCoordenates.second;
						imbs.Read(&myCoordenates.first, coordsbits);
						imbs.Read(&myCoordenates.second, coordsbits);

					}
					else {
						Client* temp = nullptr;
						temp = GetClientWithId(aPlayerId, &aClients);
						if (temp != nullptr) {
							//inc >> temp->position.first;
							//inc >> temp->position.second;

							imbs.Read(&temp->position.first, coordsbits);
							imbs.Read(&temp->position.second, coordsbits);
						}
						temp = nullptr;
					}
				}


				OutputMemoryBitStream ombs;
				ombs.Write(PacketType::ACKPING, commandBits);
				ombs.Write(myID, playerSizeBits);
				//ackPacket << "ACKPING_";
				//ackPacket << myID;

				//status = socket->send(ackPacket, serverIp, serverPort);

				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

				if (status == sf::Socket::Error) {
					std::cout << "Error enviando ACKPING\n";
				}
				else if (status == sf::Socket::Done) {
					std::cout << "Ping recibido y respondido\n";
				}

			}
			//else if (command == "CRITICAL_") {
			//	int commandId;
			//	inc >> commandId;
			//	inc >> command;

				else if (command == PacketType::NEWPLAYER) {
					int criticalId=0;
					int aPlayerId=0;
					std::pair<float, float> aPosition{ 0,0 };
					imbs.Read(&criticalId, criticalBits);
					imbs.Read(&aPlayerId, playerSizeBits);
					imbs.Read(&aPosition.first, coordsbits);
					imbs.Read(&aPosition.second, coordsbits);

					acks.push_back(criticalId);
					
					if (!ClientExists(aClients,aPlayerId)) {
						Client* newClient = new Client(aPlayerId,aPosition);
						std::cout << "Recibido NewPlayer con id " << aPlayerId << " en coordenadas " << aPosition.first << "," << aPosition.second << "\n";
					}
					else {
						std::cout << "Recepcion de NewPlayer ya existente\n";
					}

				}

				else if (command == PacketType::DISCONNECT) {
					int leaverId=0;
					int criticalId = 0;

					imbs.Read(&criticalId, criticalBits);
					imbs.Read(&leaverId, playerSizeBits);

					int index = GetIndexClientWithId(leaverId, &aClients);

					if (index != -1) {
						delete aClients.at(index);
						aClients.erase(aClients.begin() + index);
						std::cout << "Cliente con ID " << leaverId << "desconectado\n";
					}

				}

			//	else if (command == "DC_") {
			//		int leaverId;
			//		inc >> leaverId;

			//		int index = GetIndexClientWithId(leaverId, &aClients);

			//		if (index != -1) {
			//			delete aClients.at(index);
			//			aClients.erase(aClients.begin() + index);
			//			std::cout << "Cliente con ID " << leaverId << "desconectado\n";
			//		}

			//	}

			//	acks.push_back(commandId);

			//}
			incomingInfo.pop();
		}

		for (int i = 0; i < acks.size(); i++) {

			OutputMemoryBitStream ombs;
			ombs.Write(PacketType::ACK, commandBits);
			ombs.Write(myID, playerSizeBits);
			ombs.Write(acks[i], criticalBits);

			status = socket->send(ombs.GetBufferPtr(),ombs.GetByteLength(),serverIp,serverPort);
			//sf::Packet ackPacket;
			//ackPacket << "ACK_";
			//ackPacket << myID;
			//ackPacket << acks[i];

			//status = socket->send(ackPacket, serverIp, serverPort);

			if (status == sf::Socket::Status::Error) {
				std::cout << "Error enviando ACK\n";
			}
			else if (status == sf::Socket::Status::Done) {
				std::cout << "ACK " << acks[i] << "enviado\n";
			}

		}
		acks.clear();


		//if (clockForTheServer.getElapsedTime().asMilliseconds() > 5000) {
		//	end = true;
		//	std::cout << "SERVIDOR DESCONECTADOOOO\n";
		//}

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
		//sf::Packet inc;
		sf::IpAddress incomingIP;
		unsigned short incomingPort;
		//status = socket->receive(inc, incomingIP, incomingPort);
		char message[maxBufferSize];
		size_t sizeReceived;

		status = socket->receive(message, maxBufferSize, sizeReceived, incomingIP, incomingPort);

		if (status == sf::Socket::Error) {
			std::cout << "Error al recibir informacion" << std::endl;
		}
		else {
			std::cout << "Paquete recibido correctamente" << std::endl;
			incomingInfo->push(Event(message, sizeReceived, incomingIP, incomingPort));

		}
	}
}