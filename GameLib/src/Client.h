#pragma once
#include <iostream>
#include <vector>
#include <SFML\Network.hpp>
#include "OutputMemoryBitStream.h"
#include <mutex>

#define LOSSRATE 50;

//CLASE PARA TENER EL PAQUETE RELACIONADO CON UN ID
class CriticalPacket {
private:
	//sf::Packet packet;
	//OutputMemoryBitStream ombs;


	int id;
public:
	char* message;
	uint32_t messageSize;
	CriticalPacket(/*OutputMemoryBitStream ombs*/char* message, uint32_t messageSize, int id) {
		//this->ombs = ombs;
		this->id = id;
		this->message = message;
		this->messageSize = messageSize;
	}
	int GetID() {
		return id;
	}
};

class Client {
public:
	std::pair<float, float> position;
	unsigned int id;
	Client() {
		id = 0;
	}

	Client(unsigned int id) {
		this->id = id;
	}

	Client(unsigned int id, std::pair<float, float> position){
		this->id = id;
		this->position = position;
	}
};

class ServerClient:Client{
private:
	std::mutex mut;
	unsigned short port;
	std::string ip;
	std::vector<CriticalPacket> criticalVector; //POR ALGUNA RAZON NO PUEDO ACCEDER A ESTE VECTOR
	int8_t criticalPacketID; //para llevar track del id de los mensajes criticos
public:
	sf::Clock pingCounter;
	ServerClient(std::string ip, unsigned short port, unsigned int id, std::pair<float,float> position) {
		this->ip = ip;
		this->port = port;
		this->id = id;
		this->position = position;
		criticalPacketID = 0;
		pingCounter.restart();
		
	}

	unsigned short GetPort() {
		return port;
	}
	std::string GetIP() {
		return ip;
	}
	unsigned int GetID() {
		return id;
	}

	int GetCriticalId() {
		return criticalPacketID;
	}

	std::pair<float, float> GetPosition() {
		return position;
	}
	//A�ade al vector de paquetes criticos un nuevo paquete
	void AddCriticalPacket(CriticalPacket aCritical) {
		//Aqui hay que crear un CriticalPacket y meterlo en el vector
		//Despues de crearlo tenemos que incrementar en 1 el ID de los paquetes criticos.


		criticalVector.push_back(aCritical);

		mut.lock();
		criticalPacketID+=1;
		mut.unlock();
		
		std::cout << "CriticalPacket incrementado a" << criticalPacketID<<"\n";


	}
	void SendAllCriticalPackets(sf::UdpSocket* socket) {
		//se itera todo el vector de paquetes y se envian todos a la ip y puerto de este cliente
		sf::Socket::Status status;
		int lossRate = LOSSRATE;
		for (int i = 0; i < criticalVector.size(); i++) {
			if ((int)(rand() % 100) < lossRate) {
				std::cout << "PAQUETE CRITICO PERDIDO\n";
			}
			else {
				//OutputMemoryBitStream ombs = criticalVector[i].GetPacket(); //hay que recoger el packet antes porque al poner la funcion GetPacket dentro del send da error
				status = socket->send(criticalVector[i].message, criticalVector[i].messageSize, ip, port);

				if (status == sf::Socket::Status::Error) {
					std::cout << "Error enviando packet critico\n";
				}
				else if (status == sf::Socket::Status::Done) {
					std::cout << "PAQUETE CRITICO ENVIADO CORRECTAMENTE\n";
				}
			}
		}
	}
	void RemoveCriticalPacket(int id) {
		//se itera todo el vector de paquetes criticos y eliminamos el que tiene el id que le pasamos a esta funcion

		int actualIndex = -1;
		for (int i = 0; i < criticalVector.size(); i++) {
			if (criticalVector[i].GetID() == id) {
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
