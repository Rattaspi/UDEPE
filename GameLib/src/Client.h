#pragma once
#include <iostream>
#include <vector>
#include <SFML\Network.hpp>
#include <mutex>


//CLASE PARA TENER EL PAQUETE RELACIONADO CON UN ID
class CriticalPacket {
private:
	sf::Packet packet;
	int id;
public:
	CriticalPacket(sf::Packet packet, int id) {
		this->packet = packet;
		this->id = id;
	}
	sf::Packet GetPacket() {
		return packet;
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
	int criticalPacketID; //para llevar track del id de los mensajes criticos
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
	//Añade al vector de paquetes criticos un nuevo paquete
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
		for (int i = 0; i < criticalVector.size(); i++) {
			status = socket->send(criticalVector[i].GetPacket(), ip, port);

			if (status == sf::Socket::Status::Error) {
				std::cout << "Error enviando packet critico\n";
			}
			else if (status == sf::Socket::Status::Done) {
				std::cout << "Packet critico enviado\n";
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
