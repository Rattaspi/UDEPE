#include <Client.h>
#include <SFML\Network.hpp>

int main() {
	std::cout << "INICIANDO SERVIDOR..." << std::endl;
	unsigned int clientID = 0;
	std::vector<Client> aClients;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	status = socket->bind(50000);
	if (status != sf::Socket::Done) {
		std::cout << "No se ha podido vincular al puerto" << std::endl;
		system("pause");
		exit(0);
	}

	while (true) {
		sf::Packet infoReceived;
		sf::Packet infoToSend;
		sf::IpAddress remoteIP;
		unsigned short remotePort;
		std::string command;
		
		status = socket->receive(infoReceived, remoteIP, remotePort);
		infoReceived >> command;
		if (status == sf::Socket::Done) {
			if (command == "HELLO_") {
				std::cout << "Conexion aceptada " << remoteIP << ":" << remotePort << std::endl;
				aClients.push_back(Client(remoteIP.toString(), remotePort, clientID));
				infoToSend << "WELCOME_" << clientID;
				socket->send(infoToSend, remoteIP, remotePort);
				clientID++;
			}
		}
	}

	socket->unbind();
	delete socket;
	return 0;
}