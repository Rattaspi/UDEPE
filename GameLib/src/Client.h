#pragma once
#include <iostream>

class Client {
private:
	unsigned short port;
	std::string ip;
	unsigned int id;
	std::pair<float, float> position;
	std::vector<CriticalPacket> criticalVector; //POR ALGUNA RAZON NO PUEDO ACCEDER A ESTE VECTOR
	int criticalPacketID; //para llevar track del id de los mensajes criticos
public:
	Client(std::string ip, unsigned short port, unsigned int id, std::pair<float,float> position) {
		this->ip = ip;
		this->port = port;
		this->id = id;
		this->position = position;
		criticalPacketID = 0;
		
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
	std::pair<float, float> GetPosition() {
		return position;
	}
	//Añade al vector de paquetes criticos un nuevo paquete
	void AddCriticalPacket(sf::Packet packet) {
		//Aqui hay que crear un CriticalPacket y meterlo en el vector
		//Despues de crearlo tenemos que incrementar en 1 el ID de los paquetes criticos.
	}
	void SendAllCriticalPackets() {
		//se itera todo el vector de paquetes y se envian todos a la ip y puerto de este cliente
	}
	void RemoveCriticalPacket(int id) {
		//se itera todo el vector de paquetes criticos y eliminamos el que tiene el id que le pasamos a esta funcion
	}
};

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