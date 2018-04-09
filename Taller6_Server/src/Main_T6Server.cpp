#include <Client.h>
#include <queue>
#include <thread>
#include <Event.h>
#include <random>
#include <time.h>
#include "Utils.h"


void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket);

void PingThread(bool* end, std::vector<ServerClient*>* aClients, sf::UdpSocket* socket);

void DisconnectPlayer(std::vector<ServerClient*>* aClients, ServerClient* aClient);

int main() {
	sf::Clock criticalClock;
	srand(time(NULL));
	std::cout << "INICIANDO SERVIDOR..." << std::endl;
	unsigned int clientID = 0;
	std::vector<ServerClient*> aClients;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	status = socket->bind(50000);
	bool end = false;
	std::queue<Event*> incomingInfo; //cola de paquetes entrantes


	if (status != sf::Socket::Done) {
		std::cout << "No se ha podido vincular al puerto" << std::endl;
		system("pause");
		exit(0);
	}

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);
	std::thread s(&PingThread, &end, &aClients, socket);

	while (!end) {
		if (!incomingInfo.empty()) {
			Event infoReceived;
			//sf::Packet infoToSend;
			sf::IpAddress remoteIP;
			unsigned short remotePort;
			//std::string command;
			PacketType command = HELLO;
			infoReceived = *incomingInfo.front();

			InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize*8);
			imbs.Read(&command, commandBits);

			//infoReceived.packet >> command;


			if (status == sf::Socket::Done) {
				if (command == PacketType::HELLO) {
					remoteIP = infoReceived.GetAdress();
					remotePort = infoReceived.GetPort();
					bool exists = false;
					for (int i = 0; i < aClients.size(); i++) {
						if (aClients.at(i)->GetIP() == remoteIP&&aClients.at(i)->GetPort() == remotePort) {
							exists = true;

							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::WELCOME, commandBits);
							ombs.Write(aClients.at(i)->GetID(), playerSizeBits);
							ombs.Write((int)aClients.size()-1, playerSizeBits);

							for (int j = 0; j < aClients.size(); j++) {

								ombs.Write(aClients.at(j)->GetID(), playerSizeBits);

								ombs.Write(aClients.at(j)->GetPosition().first, coordsbits);
								ombs.Write(aClients.at(j)->GetPosition().second, coordsbits);

							}

							//socket->send(infoToSend, remoteIP, remotePort);
							socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

							std::cout << "Se reenvia un welcome a " << remoteIP << ":" << remotePort << std::endl;

						}
					}
					if (!exists) {
						std::cout << "Conexion aceptada " << remoteIP << ":" << remotePort << std::endl;
						//EL CLIENTE NECESITA UN PAIR QUE SERA LA POSICION X Y EN EL MAPA
						//ESTA POSICIÓN TAMBIÉN SE LA TENEMOS QUE PASAR AL CLIENTE
						//ANTES DE CREAR EL CLIENTE TENEMOS QUE CONTROLAR QUE ESTE YA NO EXISTA COMPARANDO IP:PUERTO DE ACLIENTS
						//std::pair<float, float> position(sf::);
						float x, y;
						x = rand() % 600;
						y = rand() % 400;
						std::pair<float, float> coordenadasPlayer{ x,y };



						for (int i = 0; i < aClients.size(); i++) {
							//sf::Packet criticalPacket;
							//criticalPacket << "CRITICAL_";
							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::NEWPLAYER, commandBits);
							ombs.Write(aClients[i]->GetCriticalId(), criticalBits);
							ombs.Write(clientID, playerSizeBits);
							ombs.Write(coordenadasPlayer.first, coordsbits);
							ombs.Write(coordenadasPlayer.second, coordsbits);

							CriticalPacket aCritical(ombs.GetBufferPtr(),ombs.GetByteLength(), aClients[i]->GetCriticalId());
							aClients[i]->AddCriticalPacket(aCritical);
							//std::cout << aClients[i]->GetCriticalId();
						}


						aClients.push_back(new ServerClient(remoteIP.toString(), remotePort, clientID, coordenadasPlayer));
						

						OutputMemoryBitStream ombs;
						ombs.Write(PacketType::WELCOME, commandBits);
						ombs.Write(clientID, playerSizeBits);
						ombs.Write(aClients.size()-1, playerSizeBits);

						for (int j = 0; j < aClients.size(); j++) {

							ombs.Write(aClients.at(j)->GetID(), playerSizeBits);

							ombs.Write(aClients.at(j)->GetPosition().first, coordsbits);
							ombs.Write(aClients.at(j)->GetPosition().second, coordsbits);

						}

						//socket->send(infoToSend, remoteIP, remotePort);
						socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

						//CriticalPacket aCritical(infoToSend, aClients[aClients.size() - 1]->GetCriticalId());

						//aClients[aClients.size() - 1]->AddCriticalPacket(aCritical);
						clientID++;
					}




				}
				else if (command == PacketType::ACK) {
					int receivedCriticalPlayerID = 0;
					int receivedCriticalID = 0;
					imbs.Read(&receivedCriticalPlayerID,playerSizeBits);
					imbs.Read(&receivedCriticalID, criticalBits);

					//int receivedCriticalID, receivedCriticalPlayerID;
					//infoReceived.packet >> receivedCriticalPlayerID;
					// infoReceived.packet >> receivedCriticalID;
					// 
					 ServerClient* aClient = nullptr;

					  aClient = GetClientWithId(receivedCriticalPlayerID, &aClients);

					  if (aClient != nullptr) {
						  aClient->RemoveCriticalPacket(receivedCriticalID);
					  }

				}
				else if (command == PacketType::ACKPING) {
					int playerID=0;

					//infoReceived.packet >> playerID;
					imbs.Read(&playerID, playerSizeBits);

					GetClientWithId(playerID, &aClients)->pingCounter.restart();

					std::cout << "se recibe un Ackping del jugador con id " << playerID << "\n";

				}


			}


			incomingInfo.pop();
		}
		if (criticalClock.getElapsedTime().asMilliseconds() > 200) {
			for (int i = 0; i < aClients.size(); i++) {
				aClients[i]->SendAllCriticalPackets(socket);
			}
			criticalClock.restart();
		}

		for (int i = 0; i < aClients.size(); i++) {
			if (aClients[i]->pingCounter.getElapsedTime().asMilliseconds() > 5001) {
				//DisconnectPlayer(&aClients, aClients[i]);
			}
		}


	}

	socket->unbind();
	end = true;
	s.join();
	t.join();
	delete socket;
	return 0;
}

void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket) {
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
			incomingInfo->push(new Event(message,sizeReceived,incomingIP, incomingPort));

		}
	}
}

void PingThread(bool* end, std::vector<ServerClient*>* aClients, sf::UdpSocket* socket) {
	while (!*end) {
		//sf::Packet sendPacket;
		//sendPacket << "PING_";
		//sendPacket << (int)aClients->size();

		//for (int i = 0; i < aClients->size(); i++) {
		//	sendPacket << aClients->at(i)->GetID();
		//	sendPacket << aClients->at(i)->GetPosition().first;
		//	sendPacket << aClients->at(i)->GetPosition().second;
		//}
		//for (int i = 0; i < aClients->size(); i++) {
		//	sf::Socket::Status status;
		//	status = socket->send(sendPacket, aClients->at(i)->GetIP(), aClients->at(i)->GetPort());
		//	if (status == sf::Socket::Status::Error) {
		//		std::cout << "Error enviando packet de Ping\n";
		//	}
		//	else if (status == sf::Socket::Status::Done) {
		//		std::cout << "Ping enviado\n";
		//	}
		//}
		int8_t size = aClients->size();
		OutputMemoryBitStream ombs;
		ombs.Write(PacketType::PING, commandBits);
		ombs.Write(size-1, playerSizeBits);

		for (int i = 0; i < aClients->size(); i++) {

			ombs.Write(aClients->at(i)->GetID(), playerSizeBits);
			ombs.Write(aClients->at(i)->GetPosition().first, coordsbits);
			ombs.Write(aClients->at(i)->GetPosition().second, coordsbits);

		}
		for (int i = 0; i < aClients->size(); i++) {
			sf::Socket::Status status;
			status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), aClients->at(i)->GetIP(), aClients->at(i)->GetPort());
			if (status == sf::Socket::Status::Error) {
				std::cout << "Error enviando packet de Ping\n";
			}
			else if (status == sf::Socket::Status::Done) {
				std::cout << "Ping enviado\n";
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void DisconnectPlayer(std::vector<ServerClient*>* aClients, ServerClient* aClient) {
	int whoLeaves = aClient->GetID();
	bool deleted = false;
	int index = 0;
	do {
		if (aClients->at(index)->GetID() == whoLeaves) {
			deleted = true;
			delete aClients->at(index);
			aClients->erase(aClients->begin() + index);
		}

	} while (!deleted);

	for (int i = 0; i < aClients->size(); i++) {
		OutputMemoryBitStream ombs;
		ombs.Write(PacketType::DISCONNECT);
		ombs.Write(aClients->at(i)->GetCriticalId(),criticalBits);
		ombs.Write(whoLeaves, playerSizeBits);
		//sf::Packet leaverPacket;
		//leaverPacket << "CRITICAL_";
		//leaverPacket << aClients->at(i)->GetCriticalId();
		//leaverPacket << "DC_";
		//leaverPacket << whoLeaves;

		//CriticalPacket aCriticalPacket(leaverPacket, aClients->at(i)->GetCriticalId());
		CriticalPacket aCriticalPacket(ombs.GetBufferPtr(),ombs.GetByteLength(), aClients->at(i)->GetCriticalId());
		aClients->at(i)->AddCriticalPacket(aCriticalPacket);
	}
}