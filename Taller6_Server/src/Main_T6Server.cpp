#include "Match.h"

void ReceptionThreadServer(sf::UdpSocket* socket, bool*end, std::queue<Event*>*incomingInfo) {
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
			std::cout << "Recibido comando" << std::endl;
			incomingInfo->push(new Event(message, sizeReceived, incomingIP, incomingPort));
		}
	}
}

void ReceptionThread(Match* match) {
	sf::Socket::Status status;
	while (!match->end) {
		sf::IpAddress incomingIP;
		unsigned short incomingPort;
		char message[maxBufferSize];
		size_t sizeReceived;

		status = match->socket->receive(message, maxBufferSize, sizeReceived, incomingIP, incomingPort);

		if (status == sf::Socket::Error) {
			std::cout << "Error al recibir informacion" << std::endl;
		}
		else {
			match->incomingInfo.push(new Event(message, sizeReceived, incomingIP, incomingPort));
		}
	}
}

void PingThread(Match* match) {
	while (!match->end) {
		OutputMemoryBitStream ombs;
		ombs.Write(PacketType::PING, commandBits);
		//ombs.Write(size-1, playerSizeBits);

		//for (int i = 0; i < aClients->size(); i++) {

		//	ombs.Write(aClients->at(i)->GetID(), playerSizeBits);
		//	ombs.Write(aClients->at(i)->GetPosition().first, coordsbits);
		//	ombs.Write(aClients->at(i)->GetPosition().second, coordsbits);
		//}
		for (int i = 0; i < match->aClients.size(); i++) {
			if (match->aClients.at(i) != nullptr) {
				sf::Socket::Status status;
				status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), match->aClients.at(i)->GetIP(), match->aClients.at(i)->GetPort());
				if (status == sf::Socket::Status::Error) {
					std::cout << "Error enviando packet de Ping\n";
				}
				else if (status == sf::Socket::Status::Done) {
					std::cout << "Ping enviado\n";
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		//std::cout << "SLeep\n";
	}
}


int main() {

	std::vector<User> users;

	User devildra("devildrake","ab123");
	User rattaspi("rattaspi", "ab123");
	User ah97("ah97", "ab123");
	User urisel("urisel", "ab123");

	users.push_back(devildra);
	users.push_back(rattaspi);
	users.push_back(ah97);
	users.push_back(urisel);


	sf::UdpSocket* socket=new sf::UdpSocket();
	sf::Socket::Status status;
	std::queue<Event*> incomingInfo;
	bool end = false;
	status = socket->bind(50000);
	if (status == sf::Socket::Error) {
		std::cout << "Error bindeando socket\n";
		end = true;
	}

	std::thread r(&ReceptionThreadServer, socket, &end, &incomingInfo);

	while (!end) {
		std::vector<ServerClient*>connectedClients;

		if (!incomingInfo.empty()) {
			Event infoReceived;
			PacketType command = HELLO;
			infoReceived = *incomingInfo.front();
			InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
			imbs.Read(&command, commandBits);

			sf::IpAddress remoteIP;
			unsigned short remotePort;
			remoteIP = infoReceived.GetAdress();
			remotePort = infoReceived.GetPort();
			OutputMemoryBitStream ombs;


			switch (command) {
			case PacketType::PING:
				
				ombs.Write(ACKPING,commandBits);
				status=socket->send(ombs.GetBufferPtr(),ombs.GetByteLength(),remoteIP, remotePort);

				if (status == sf::Socket::Error) {
					std::cout << "ERROR ENVIANDO\n";
				}
				else {
					std::cout << "Contestando al ping\n";
				}

				break;
			case PacketType::DISCONNECT: {

				int index = -1;
				for (int i = 0; i < connectedClients.size(); i++) {
					if (connectedClients[i]->GetIP() == remoteIP&&connectedClients[i]->GetPort() == remotePort) {
						index = i;
					}
				}
				if (index > 0) {
					connectedClients.erase(connectedClients.begin() + index);
				}

				break;
			}
			case PacketType::LOGIN: {
				ombs.Write(PacketType::LOGIN, commandBits);
				std::string userName = "";
				std::string passWord= "";

				imbs.ReadString(&userName);
				imbs.ReadString(&passWord);
				int index = -1;


				for (int i = 0; i < users.size(); i++) {
					std::cout << "Recibido usuario " << userName <<", comparando con " << users[i].userName << std::endl;

					if (users[i].userName == userName) {
						std::cout << "Este usuario existe\n";

						index = i;
					}
				}

				if (index >= 0) {
					if (users[index].passWord == passWord) {
						//users[index].connected = true;
						ServerClient* aClient = new ServerClient(remoteIP.toString(),remotePort,0,std::pair<short,short>(0,0));
						aClient->SetUserName(users[index].userName);

						connectedClients.push_back(aClient);

						//ServerClient aClient;
						//aClient.SetUserName(users[index].userName);
						//aClient.SetIp(remoteIP.toString());
						//aClient.SetPort(remotePort);
						//connectedClients.push_back(aClient);


						ombs.Write(true, boolBit);
					}
					else {
						ombs.Write(false, boolBit);
					}
				}
				else {
					ombs.Write(false, boolBit);
				}

				socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

				break;
			}
			default:
				break;
			}
			incomingInfo.pop();
		}


		//Match match;
		//match.SetUp();
		////std::thread t(&ReceptionThread, &match);
		////std::thread s(&PingThread, &match);

		//std::thread s(&ReceptionThread, &match);
		//std::thread t(&PingThread, &match);

		//match.Run();

		//s.join();
		//t.join();
	}

	socket->unbind();
	r.join();

	return 0;
}

