#pragma once
#include <iostream>
#include <vector>
#include <SFML\Network.hpp>
#include "OutputMemoryBitStream.h"
#include <mutex>
#include <queue>

#define LOSSRATE 0;

class AccumMove {
public:
	int idMove;
	std::pair<short, short> delta;
	std::pair<short, short> absolute;
	AccumMove() {
		idMove = 0;
		absolute = { 0,0 };
	}

	AccumMove(int idMove, std::pair<short, short> absolute) {
		this->idMove = idMove;
		this->absolute = absolute;
	}

};

class AccumMoveServer : public AccumMove {
public:
	int playerID;
	AccumMoveServer(int idMove, std::pair<short, short> absolute, int playerID) {
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
	//	char* message = new char();
	char* message;
	CriticalMessage(int criticalId, char* message, uint32_t messageSize) {
		this->message = new char[messageSize];
		this->criticalId = criticalId;
		//this->message = message;
		strcpy(this->message, message);
		this->messageSize = messageSize;
	}
	//~CriticalMessage() {
	//	if(message!=nullptr)
	//	delete message;
	//}
};

class Client {
public:
	std::string userName;
	std::pair<short, short> position;
	std::queue<std::pair<short, short>>steps; //Esto solo lo usan clientes
	int userId;
	int matchId;
	//unsigned int id;
	Client() {
		userName = "";
		position.first = 0;
		position.second = 0;
		matchId = -1;
	}

	Client(int id) {
		this->matchId = id;
		userName = "";
		position.first = 0;
		position.second = 0;
	}

	Client(int id, std::pair<short, short> position){
		this->matchId = id;
		this->position = position;
	}

	void EmptyStepQueue() {
		while (steps.size() > 0) {
			steps.pop();
		}
	}

};

class User {
public:
	std::string userName;
	std::string passWord;
	User(std::string u , std::string p) {
		userName = u;
		passWord = p;
	}
};

class ServerClient : public Client{
private:
	std::mutex mut;
	unsigned short port;
	std::string ip;
	std::vector<CriticalMessage> criticalVector; //POR ALGUNA RAZON NO PUEDO ACCEDER A ESTE VECTOR
public:
	//Match* inGame;
	bool inGame;
	std::vector<AccumMoveServer> acumulatedMoves;
	sf::Clock moveClock;
	sf::Clock shootClock;
	int criticalId; //para llevar track del id de los mensajes criticos
	int sessionID; //id de la entrada de sesion que se ha creado en la base de datos
	int games; //contador de cuantas partidas se han jugado en una sesion
	sf::Clock pingCounter;
	bool isReady;
	ServerClient(std::string ip, unsigned short port, int id, std::pair<short, short> position, int sessionID, std::string username) {
		this->ip = ip;
		this->port = port;
		this->matchId = id;
		this->position = position;
		pingCounter.restart();
		userName = "Player" + id;
		criticalId = 0;
		this->sessionID = sessionID;
		userName = username;
		games = 0;
		isReady = false;
	}

	ServerClient() {
		matchId = 0;
		criticalId = 0;
		userName = "";
	}
	
	void SetIp(std::string ip) {
		this->ip = ip;
	}

	void SetPort(unsigned short port) {
		this->port = port;
	}

	void SetUserName(std::string name) {
		userName = name;
	}

	unsigned short GetPort() {
		return port;
	}
	std::string GetIP() {
		return ip;
	}
	int GetMatchID() {
		return matchId;
	}

	int GetUserID() {
		return userId;
	}

	std::pair<short, short> GetPosition() {
		return position;
	}
	//Añade al vector de paquetes criticos un nuevo paquete
	void AddCriticalMessage(CriticalMessage* critical) {
		criticalVector.push_back(*critical);
		criticalId++;
		std::cout << "Client with id " << matchId << " raised critialId to " << criticalId << std::endl;
	}

	void DebugCriticalPackets() {
		for (int i = 0; i < criticalVector.size(); i++) {
			std::cout << "CRITICAL PACKET ID-> " << criticalVector[i].criticalId << "MY PORT IS " << port <<  std::endl;
		}
	}

	bool HasCriticalPackets() {
		return criticalVector.size() > 0;
	}

	void SendAllCriticalPackets(sf::UdpSocket* socket) {
		DebugCriticalPackets();

		sf::Socket::Status status;

		for (int i = 0; i < criticalVector.size(); i++) {
			std::cout << "Criticals sent to player " << matchId << "\n";
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
