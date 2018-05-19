#include "Match.h"



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
	Match match;
	match.SetUp();
	//std::thread t(&ReceptionThread, &match);
	//std::thread s(&PingThread, &match);

	std::thread s(&ReceptionThread, &match);
	std::thread t(&PingThread, &match);

	match.Run();

	s.join();
	t.join();

	return 0;
}

