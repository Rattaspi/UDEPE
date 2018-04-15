#pragma once
#include <iostream>
#include <vector>
#include <SFML\Network.hpp>
#include "OutputMemoryBitStream.h"
#include <mutex>

#define LOSSRATE 25;

class AccumMove {
public:
	int idMove;
	std::pair<short, short> delta;
	std::pair<short, short> absolute;
	AccumMove() {
		idMove = 0;
		delta = { 0,0 };
		absolute = { 0,0 };
	}

	AccumMove(int idMove, std::pair<short, short> delta, std::pair<short, short> absolute) {
		this->idMove = idMove;
		this->delta = delta;
		this->absolute = absolute;
	}

};

class AccumMoveServer : public AccumMove {
public:
	int playerID;
	AccumMoveServer(int idMove, std::pair<short, short> delta, std::pair<short, short> absolute, int playerID) {
		this->idMove = idMove;
		this->delta = delta;
		this->absolute = absolute;
		this->playerID = playerID;
	}

	AccumMoveServer() {
		this->idMove = 0;
		this->delta = { 0,0 };
		this->absolute = { 0,0 };
		this->playerID = 0;
	}

};

class CriticalMessage {
public:
	int criticalId;
	uint32_t messageSize;
	char* message;
	CriticalMessage(int criticalId, char* message, uint32_t messageSize) {
		this->criticalId = criticalId;
		this->message = message;
		this->messageSize = messageSize;
	}
};

class Client {
public:
	std::string userName;
	std::pair<short, short> position;
	int id;
	//unsigned int id;
	Client() {
		userName = "";
		position.first = 0;
		position.second = 0;
		id = 0;
	}

	Client(int id) {
		this->id = id;
		userName = "";
		position.first = 0;
		position.second = 0;
	}

	Client(int id, std::pair<short, short> position){
		this->id = id;
		this->position = position;
	}

};

class ServerClient:public Client{
private:
	std::mutex mut;
	unsigned short port;
	std::string ip;
	std::vector<CriticalMessage> criticalVector; //POR ALGUNA RAZON NO PUEDO ACCEDER A ESTE VECTOR
public:
	std::vector<AccumMoveServer> acumulatedMoves;
	sf::Clock moveClock;

	int criticalId; //para llevar track del id de los mensajes criticos
	sf::Clock pingCounter;
	ServerClient(std::string ip, unsigned short port, int id, std::pair<short, short> position) {
		this->ip = ip;
		this->port = port;
		this->id = id;
		this->position = position;
		pingCounter.restart();
		userName = "Player" + id;
	}


	unsigned short GetPort() {
		return port;
	}
	std::string GetIP() {
		return ip;
	}
	int GetID() {
		return id;
	}

	std::pair<short, short> GetPosition() {
		return position;
	}
	//Añade al vector de paquetes criticos un nuevo paquete
	void AddCriticalMessage(CriticalMessage* critical) {
		criticalVector.push_back(*critical);
		criticalId++;
	}

	void DebugCriticalPackets() {
		for (int i = 0; i < criticalVector.size(); i++) {
			std::cout << "CRITICAL PACKET ID-> " << criticalVector[i].criticalId << std::endl;
		}
	}

	bool HasCriticalPackets() {
		return criticalVector.size() > 0;
	}

	void SendAllCriticalPackets(sf::UdpSocket* socket) {
		DebugCriticalPackets();

		sf::Socket::Status status;

		for (int i = 0; i < criticalVector.size(); i++) {
			status = socket->send(criticalVector[i].message, criticalVector[i].messageSize, ip, port);

			if (status == sf::Socket::Status::Error) {
				std::cout << "Error enviando packet critico\n";
			}
			else if (status == sf::Socket::Status::Done) {
				//std::cout << "PAQUETE CRITICO ENVIADO CORRECTAMENTE\n";
			}

		}


	}

	//void SendAllCriticalPackets(sf::UdpSocket* socket) {
	//	//se itera todo el vector de paquetes y se envian todos a la ip y puerto de este cliente
	//	sf::Socket::Status status;
	//	int lossRate = LOSSRATE;

	//	//DebugCriticalPackets();

	//	for (int i = 0; i < criticalVector.size(); i++) {
	//		if ((int)(rand() % 100) < lossRate) {
	//			std::cout << "PAQUETE CRITICO PERDIDO\n";
	//		}
	//		else {
	//			//OutputMemoryBitStream ombs = criticalVector[i].GetPacket(); //hay que recoger el packet antes porque al poner la funcion GetPacket dentro del send da error
	//			//status = socket->send(criticalVector[i].message, criticalVector[i].messageSize, ip, port);
	//			
	//			status = socket->send(criticalVector[i].ombs->GetBufferPtr(), criticalVector[i].ombs->GetByteLength(), ip, port);

	//			//std::cout << "ombsPTR " << criticalVector[i].message << std::endl;

	//			if (status == sf::Socket::Status::Error) {
	//				std::cout << "Error enviando packet critico\n";
	//			}
	//			else if (status == sf::Socket::Status::Done) {
	//				//std::cout << "PAQUETE CRITICO ENVIADO CORRECTAMENTE\n";
	//			}
	//		}
	//	}
	//}
	void RemoveCriticalPacket(int id) {
		//se itera todo el vector de paquetes criticos y eliminamos el que tiene el id que le pasamos a esta funcion
		int actualIndex = -1;
		for (int i = 0; i < criticalVector.size(); i++) {
			if (criticalVector[i].criticalId == id) {
				actualIndex = i;
				break;
			}
		}

		if (actualIndex != -1){
			criticalVector.erase(criticalVector.begin() + actualIndex);
			std::cout << "CriticalPacket en posicion " << actualIndex << " borrado con exito\n";
		}

	}
};
