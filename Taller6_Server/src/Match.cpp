#include "Match.h"


void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket) {
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
			incomingInfo->push(new Event(message, sizeReceived, incomingIP, incomingPort));

		}
	}
}

void PingThread(bool* end, std::vector<ServerClient*>* aClients, sf::UdpSocket* socket) {
	while (!*end) {
		OutputMemoryBitStream ombs;
		ombs.Write(PacketType::PING, commandBits);
		//ombs.Write(size-1, playerSizeBits);

		//for (int i = 0; i < aClients->size(); i++) {

		//	ombs.Write(aClients->at(i)->GetID(), playerSizeBits);
		//	ombs.Write(aClients->at(i)->GetPosition().first, coordsbits);
		//	ombs.Write(aClients->at(i)->GetPosition().second, coordsbits);
		//}
		for (int i = 0; i < aClients->size(); i++) {
			if (aClients->at(i) != nullptr) {
				sf::Socket::Status status;
				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), aClients->at(i)->GetIP(), aClients->at(i)->GetPort());
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

void DisconnectPlayer(std::vector<ServerClient*>* aClients, ServerClient* aClient) {
	int disconnectedId = aClient->GetID();
	int disconnectedIndex = GetIndexServerClientWithId(disconnectedId, aClients);

	if (disconnectedIndex > -1) {
		for (int i = 0; i < aClients->size(); i++) {
			if (i != disconnectedIndex) {
				OutputMemoryBitStream ombs;
				ombs.Write(PacketType::DISCONNECT, commandBits);
				ombs.Write(aClients->at(i)->criticalId, criticalBits);
				ombs.Write(disconnectedId, playerSizeBits);
				std::cout << "ADDING CRITICAL TO PLAYER " << aClients->at(i)->GetID() << " WITH ID " << aClients->at(i)->criticalId << " THAT DISCONNECTS PLAYER WITH ID " << aClient->GetID() << std::endl;;
				aClients->at(i)->AddCriticalMessage(new CriticalMessage(aClients->at(i)->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
			}
		}

		std::cout << "Erasing client with Id" << disconnectedId << std::endl;
		aClients->erase(aClients->begin() + disconnectedIndex);
	}
	else {
		std::cout << "Trying to disconnect someone who does not exist\n";
	}
}

void SendBallPos(std::vector<ServerClient*>*aClients, sf::UdpSocket* socket, std::pair<short, short> ballPos) {
	OutputMemoryBitStream ombs;
	ombs.Write(PacketType::MOVEBALL, commandBits);

	std::pair<short, short>tempCoords;
	tempCoords.first = (short)ballPos.first;
	tempCoords.second = (short)ballPos.second;

	ombs.Write(tempCoords.first, coordsbits);
	ombs.Write(tempCoords.second, coordsbits);
	sf::Socket::Status status;
	for (int i = 0; i < aClients->size(); i++) {
		status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), aClients->at(i)->GetIP(), aClients->at(i)->GetPort());

		//if (status == sf::Socket::Done) {
		//	
		//}
		if (status == sf::Socket::Error) {
			std::cout << "Error de envío de pos de ball\n";
		}

	}
}

void UpdateBall(std::pair<float, float>* coords, std::pair<float, float>*speed, float delta, int*leftScore, int*rightScore, std::vector<ServerClient*>*aClients, sf::UdpSocket* socket, bool* gameOver) {
	coords->first += speed->first*delta;
	coords->second += speed->second*delta;

	if (coords->first + ballRadius * 2 > 1000 && speed->first>0) {
		coords->first = 999 - ballRadius;
		speed->first *= -1;
	}
	else if (coords->first < ballRadius&&speed->first<0) {
		coords->first = ballRadius;
		speed->first *= -1;
	}if (coords->second + ballRadius * 2 > 600 && speed->second>0) {
		coords->second = 599 - ballRadius;
		speed->second *= -1;
	}
	else if (coords->second < ballRadius&&speed->second<0) {
		coords->second = ballRadius;
		speed->second *= -1;
	}

	if (coords->second > 200 - ballRadius && coords->second < 400 + ballRadius) {
		std::cout << "Score: " << *leftScore << " - " << *rightScore << std::endl;
		if (coords->first <= 0 + ballRadius) {
			std::cout << "GOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOAL\n";
			*rightScore += 1;
			*gameOver = CheckGameOver(*leftScore, *rightScore, aClients, socket);
			for (int i = 0; i < aClients->size(); i++) {
				OutputMemoryBitStream ombs;
				ombs.Write(PacketType::GOAL, commandBits);
				ombs.Write(aClients->at(i)->criticalId, criticalBits);
				ombs.Write(*rightScore + 1, scoreBits);
				ombs.Write(*leftScore + 1, scoreBits);
				aClients->at(i)->AddCriticalMessage(new CriticalMessage(aClients->at(i)->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
				coords->first = (float)ballStartPos.first;
				coords->second = (float)ballStartPos.second;
				speed->first = 0;
				speed->second = 0;
			}
		}
		else if (coords->first >= windowWidth - ballRadius * 2) {
			std::cout << "GOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOAL\n";
			*leftScore += 1;
			*gameOver = CheckGameOver(*leftScore, *rightScore, aClients, socket);

			for (int i = 0; i < aClients->size(); i++) {
				OutputMemoryBitStream ombs;
				ombs.Write(PacketType::GOAL, commandBits);
				ombs.Write(aClients->at(i)->criticalId, criticalBits);
				ombs.Write(*rightScore + 1, scoreBits);
				ombs.Write(*leftScore + 1, scoreBits);
				aClients->at(i)->AddCriticalMessage(new CriticalMessage(aClients->at(i)->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
				coords->first = (float)ballStartPos.first;
				coords->second = (float)ballStartPos.second;
				speed->first = 0;
				speed->second = 0;
			}
		}
	}

	//std::cout << "New ball pos: " << coords->first <<", " << coords->second<<std::endl;
}

bool CheckGameOver(int leftScore, int rightScore, std::vector<ServerClient*>*aClients, sf::UdpSocket* socket) {
	//Si gana left se pasa un 0, si gana right se pasa un 1
	if (leftScore == victoryScore) {
		for (int i = 0; i < aClients->size(); i++) {
			OutputMemoryBitStream ombs;
			ombs.Write(PacketType::GAMEOVER, commandBits);
			ombs.Write(aClients->at(i)->criticalId, criticalBits);
			ombs.Write(0, boolBit);
			aClients->at(i)->AddCriticalMessage(new CriticalMessage(aClients->at(i)->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
		}
		return true;
	}
	else if (rightScore == victoryScore) {
		for (int i = 0; i < aClients->size(); i++) {
			OutputMemoryBitStream ombs;
			ombs.Write(PacketType::GAMEOVER, commandBits);
			ombs.Write(aClients->at(i)->criticalId, criticalBits);
			ombs.Write(1, boolBit);
			aClients->at(i)->AddCriticalMessage(new CriticalMessage(aClients->at(i)->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
		}
		return true;
	}
	return false;
}

int GetAvailableId(std::vector<ServerClient*>aClients, int num) {
	//Inicialización ids
	std::map<int, bool> ids;
	for (int i = 0; i < num; i++) {
		ids[i] = false;
	}

	//Comprovación de tamaño
	if (aClients.size() == num) {
		return -1;
	}
	else { //Si el tamaño es menor a num, se busca el primer id disponible
		for (int i = 0; i < aClients.size(); i++) {
			ids[aClients[i]->GetID()] = true;
		}

		for (std::map<int, bool>::iterator it = ids.begin(); it != ids.end(); ++it) {
			if (!it->second) {
				return it->first;
			}
		}
	}

	//Por si acaso
	return -1;
}
