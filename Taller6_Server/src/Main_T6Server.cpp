#include <Client.h>
#include <queue>
#include <thread>
#include <Event.h>
#include <random>

void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket);


int main() {
	std::cout << "INICIANDO SERVIDOR..." << std::endl;
	unsigned int clientID = 0;
	std::vector<Client> aClients;
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

	while (!end) {
		if (!incomingInfo.empty()) {
			Event infoReceived;
			sf::Packet infoToSend;
			sf::IpAddress remoteIP;
			unsigned short remotePort;
			std::string command;

			infoReceived = *incomingInfo.front();

			infoReceived.packet >> command;
			if (status == sf::Socket::Done) {
				if (command == "HELLO_") {
					remoteIP = infoReceived.GetAdress();
					remotePort = infoReceived.GetPort();
					std::cout << "Conexion aceptada " << remoteIP << ":" << remotePort << std::endl;
					//EL CLIENTE NECESITA UN PAIR QUE SERA LA POSICION X Y EN EL MAPA
					//ESTA POSICIÓN TAMBIÉN SE LA TENEMOS QUE PASAR AL CLIENTE
					//ANTES DE CREAR EL CLIENTE TENEMOS QUE CONTROLAR QUE ESTE YA NO EXISTA COMPARANDO IP:PUERTO DE ACLIENTS
					//std::pair<float, float> position(sf::);
					aClients.push_back(Client(remoteIP.toString(), remotePort, clientID));
					infoToSend << "WELCOME_" << clientID;
					socket->send(infoToSend, remoteIP, remotePort);
					clientID++;
				}
			}


			incomingInfo.pop();
		}

	}

	socket->unbind();
	delete socket;
	return 0;
}

void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket) {
	sf::Socket::Status status;
	socket->setBlocking(true);
	while (!*end) {
		sf::Packet inc;
		sf::IpAddress incomingIP;
		unsigned short incomingPort;
		status = socket->receive(inc, incomingIP, incomingPort);
		if (status == sf::Socket::Error) {
			std::cout << "Error al recibir informacion" << std::endl;
		}
		else {
			std::cout << "Paquete recibido correctamente" << std::endl;
			incomingInfo->push(new Event(inc,incomingIP, incomingPort));
			//incomingInfo >> command;
			//if (command == "WELCOME_") {
			//	incomingInfo >> myID;
			//	std::cout << "Conectado al servidor" << std::endl;
			//	std::cout << "Mi ID es--> " << myID << std::endl;
			//}
		}
	}
}