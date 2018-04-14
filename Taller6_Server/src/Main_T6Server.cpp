#include <Client.h>
#include <queue>
#include <thread>
#include <Event.h>
#include <random>
#include <time.h>
#include "Utils.h"


void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket);

void PingThread(bool* end, std::vector<ServerClient*>* aClients, sf::UdpSocket* socket);

void DisconnectPlayer(std::vector<ServerClient*>* aClients, ServerClient* aClient);

int main() {
	sf::Clock criticalClock;
	srand(time(NULL));
	std::cout << "INICIANDO SERVIDOR..." << std::endl;
	int clientID = 0;
	std::vector<ServerClient*> aClients;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	status = socket->bind(50000);
	bool end = false;
	std::queue<Event*> incomingInfo; //cola de paquetes entrantes
	std::vector<std::vector<AccumMoveServer>> acumulatedMoves;

	if (status != sf::Socket::Done) {
		std::cout << "No se ha podido vincular al puerto" << std::endl;
		system("pause");
		exit(0);
	}

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);
	std::thread s(&PingThread, &end, &aClients, socket);

	while (!end) {
		if (!incomingInfo.empty()) {
			Event infoReceived;
			sf::IpAddress remoteIP;
			unsigned short remotePort;
			int command = HELLO;
			infoReceived = *incomingInfo.front();
			remoteIP = infoReceived.GetAdress();
			remotePort = infoReceived.GetPort();

			InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize*8);
			imbs.Read(&command, commandBits);
			OutputMemoryBitStream ombs;
			sf::Socket::Status status;
			bool exists = false;
			ServerClient* aClient;
			int aCriticalId = 0 ;
			int repeatingId = 0;
			std::pair<short, short> coords;

			switch (command) {
			case HELLO:
				coords.first = rand() % 740;
				coords.second = rand() % 540;

				for (int i = 0; i < aClients.size(); i++) {
					if (aClients[i]->GetIP() == remoteIP&&aClients[i]->GetPort() == remotePort) {
						exists = true;
						repeatingId = aClients[i]->GetID();
					}
				}

				if (aClients.size() < 4) {
					ombs.Write(PacketType::WELCOME, commandBits);

					if (!exists) {
						aClients.push_back(new ServerClient(remoteIP.toString(), remotePort, clientID, coords));
						ombs.Write(clientID, playerSizeBits);
						repeatingId = clientID;
						clientID++;

					}
					else {
						ombs.Write(repeatingId, playerSizeBits);
					}
					for (int i = 0; i < 1; i++) {
						int aClientSizeMinus = aClients.size();
						aClientSizeMinus--;
						ombs.Write(aClientSizeMinus, playerSizeBits);
					}
					for (int i = 0; i < aClients.size(); i++) {
						ombs.Write(aClients[i]->GetID(), playerSizeBits);
						ombs.Write(aClients[i]->GetPosition().first, coordsbits);
						ombs.Write(aClients[i]->GetPosition().second, coordsbits);
					}

					status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

					if (status == sf::Socket::Error) {
						std::cout << "ERROR nen\n";
					}

					if (!exists) {

						std::cout << coords.second << "--AAAAAAAAAAAAAAA\n";
						for (int i = 0; i < aClients.size(); i++) {

							OutputMemoryBitStream* auxOmbs = new OutputMemoryBitStream();

							auxOmbs->Write(PacketType::NEWPLAYER, commandBits);
							auxOmbs->Write(aClients[i]->criticalId, criticalBits);
							auxOmbs->Write(repeatingId, playerSizeBits);
							auxOmbs->Write(coords.first, coordsbits);
							auxOmbs->Write(coords.second, coordsbits);

							//socket->send(auxOmbs->GetBufferPtr(), auxOmbs->GetByteLength(), aClients[i]->GetIP(), aClients[i]->GetPort());
							aClients[i]->AddCriticalMessage(new CriticalMessage(aClients[i]->criticalId, auxOmbs->GetBufferPtr(), auxOmbs->GetByteLength()));

						}


					}
				}
				else {
					if (exists) {
						ombs.Write(PacketType::WELCOME, commandBits);
						ombs.Write(repeatingId, playerSizeBits);
						
						for (int i = 0; i < 1; i++) {
							int aClientSizeMinus = aClients.size();
							aClientSizeMinus--;
							ombs.Write(aClientSizeMinus, playerSizeBits);
						}
						for (int i = 0; i < aClients.size(); i++) {
							ombs.Write(aClients[i]->GetID(), playerSizeBits);
							ombs.Write(aClients[i]->GetPosition().first, coordsbits);
							ombs.Write(aClients[i]->GetPosition().second, coordsbits);
						}

						status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

						if (status == sf::Socket::Error) {
							std::cout << "ERROR nen\n";
						}
					}
					else {
						ombs.Write(PacketType::NOTWELCOME, commandBits);
						status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

						if (status == sf::Socket::Error) {
							std::cout << "ERROR nen\n";
						}
					}
				}


				break;
			case ACK:
				aClient = GetClientWithIpPort(remotePort, remoteIP.toString(), &aClients);
				if (aClient != nullptr) {

					imbs.Read(&aCriticalId, criticalBits);
					aClient->RemoveCriticalPacket(aCriticalId);
					//imbs.Read(&aCriticalId, criticalBits);
					//aClient->RemoveCriticalPacket(aCriticalId);
					std::cout << "Recibido ACK " << aCriticalId << "Del jugador con ID " << aClient->GetID() << std::endl;
				
				}
				break;
			case ACKPING:
				aClient = GetClientWithIpPort(remotePort, remoteIP.toString(), &aClients);
				if (aClient != nullptr) {
					aClient->pingCounter.restart();
					std::cout << "Recibido ACKPING de jugador " << aClient->GetID() << std::endl;
				}
				break;
			case MOVE:
				aClient = GetClientWithIpPort(remotePort, remoteIP.toString(), &aClients);
				if (aClient != nullptr) {
					AccumMoveServer accumMove;
					accumMove.playerID = aClient->id;
					imbs.Read(&accumMove.idMove, criticalBits);
					imbs.Read(&accumMove.delta.first, deltaMoveBits);
					imbs.Read(&accumMove.delta.second, deltaMoveBits);
					imbs.Read(&accumMove.absolute.first, coordsbits);
					imbs.Read(&accumMove.absolute.second, coordsbits);
					//std::cout << "Recibido accumMove de jugador ID " << aClient->id << " que tiene delta = " << accumMove.delta.first << ", " << accumMove.delta.second << " y absolute = " << accumMove.absolute.first << ", " <<  accumMove.absolute.second << std::endl;;
					//acumulatedMoves.push_back(accumMove);
					aClient->acumulatedMoves.push_back(accumMove);
				}
				else {
					std::cout << "Null player trying to send movemen\n";
				}

				break;
			}

			incomingInfo.pop();
		}
		if (criticalClock.getElapsedTime().asMilliseconds() > 200) {
			for (int i = 0; i < aClients.size(); i++) {
				if (aClients[i]->HasCriticalPackets()) {
					aClients[i]->SendAllCriticalPackets(socket);
				}
			}
			criticalClock.restart();
		}


		for (int i = 0; i < aClients.size(); i++) {
			if (aClients[i]->pingCounter.getElapsedTime().asMilliseconds() > 10000) {
				std::cout << "Player with id " << aClients[i]->GetID() << " timeOut " << aClients[i]->pingCounter.getElapsedTime().asMilliseconds() << std::endl;;
				DisconnectPlayer(&aClients, aClients[i]);
			}
			else if (aClients[i]->moveClock.getElapsedTime().asMilliseconds() > 100) {

				if (aClients[i]->acumulatedMoves.size() > 0) {

					OutputMemoryBitStream ombs;
					int latestMessageIndex = 0;
					for (int j = 0; j < aClients[i]->acumulatedMoves.size(); j++) {
						if (aClients[i]->acumulatedMoves[j].idMove > aClients[i]->acumulatedMoves[latestMessageIndex].idMove) {
							latestMessageIndex = j;
						}
					}

					if ((aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first + 60) < 800 && (aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first)>0&&(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second+60)<600 && (aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second)>0) {
						ombs.Write(PacketType::ACKMOVE, commandBits);
						ombs.Write(aClients[i]->id, playerSizeBits);
						ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].idMove, criticalBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.first,deltaMoveBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.second, deltaMoveBits);


						ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first, coordsbits);
						ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second, coordsbits);
						aClients[i]->position = aClients[i]->acumulatedMoves[latestMessageIndex].absolute;
						aClients[i]->acumulatedMoves.clear();

						std::cout << "Enviada posicion de jugador con ID " << aClients[i]->GetID() << ".Sus coordenadas son " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first << ", " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second << "\n";


						for (int j = 0; j < aClients.size(); j++) {
							status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), aClients[j]->GetIP(), aClients[j]->GetPort());
							if (status == sf::Socket::Error) {
								std::cout << "ERROR ENVIANDO ACTUALIZACION DE POSICION\n";
							}
						}
					}
					else {
						ombs.Write(PacketType::ACKMOVE, commandBits);
						ombs.Write(aClients[i]->id, playerSizeBits);
						ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].idMove, criticalBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.first,deltaMoveBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.second, deltaMoveBits);


						ombs.Write(aClients[i]->position.first, coordsbits);
						ombs.Write(aClients[i]->position.second, coordsbits);
						//aClients[i]->position = aClients[i]->acumulatedMoves[latestMessageIndex].absolute;
						aClients[i]->acumulatedMoves.clear();

						std::cout << "Enviada posicion de jugador con ID " << aClients[i]->GetID() << ".Sus coordenadas son " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first << ", " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second << "\n";


						for (int j = 0; j < aClients.size(); j++) {
							status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), aClients[j]->GetIP(), aClients[j]->GetPort());
							if (status == sf::Socket::Error) {
								std::cout << "ERROR ENVIANDO ACTUALIZACION DE POSICION\n";
							}
						}
					}


				}
				aClients[i]->moveClock.restart();
			}
		}
	}

	socket->unbind();
	end = true;
	s.join();
	t.join();
	delete socket;
	return 0;
}

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
			incomingInfo->push(new Event(message,sizeReceived,incomingIP, incomingPort));

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
	int disconnectedIndex=GetIndexClientWithId(disconnectedId,aClients);

	if (disconnectedIndex > 0) {
		std::cout << "Erasing client with Id" << disconnectedId << std::endl;
		aClients->erase(aClients->begin() + disconnectedIndex);

		for (int i = 0; i < aClients->size(); i++) {
			OutputMemoryBitStream ombs;
			ombs.Write(PacketType::DISCONNECT, commandBits);
			ombs.Write(aClients->at(i)->criticalId, criticalBits);
			ombs.Write(disconnectedId, playerSizeBits);
			aClients->at(i)->AddCriticalMessage(new CriticalMessage(aClients->at(i)->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
		}
	}
	else {
		std::cout << "Trying to disconnect someone who does not exist\n";
	}
}

/*				if (command == PacketType::HELLO) {
					remoteIP = infoReceived.GetAdress();
					remotePort = infoReceived.GetPort();
					bool exists = false;

					std::cout << "IP: " << remoteIP.toString() << std::endl;

					for (int i = 0; i < aClients.size(); i++) {
						if (aClients.at(i)->GetIP() == remoteIP.toString()&&aClients.at(i)->GetPort() == remotePort) {
							exists = true;

							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::WELCOME, commandBits);
							ombs.Write(aClients.at(i)->GetID(), playerSizeBits);
							ombs.Write(aClients.size()-1, playerSizeBits);

							for (int j = 0; j < aClients.size(); j++) {

								ombs.Write(aClients.at(j)->GetID(), playerSizeBits);

								ombs.Write(aClients.at(j)->GetPosition().first, coordsbits);
								ombs.Write(aClients.at(j)->GetPosition().second, coordsbits);

							}
							socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

							std::cout << "Se reenvia un welcome a " << remoteIP << ":" << remotePort << std::endl;

						}
					}
					if (!exists&&clientID<4) {
						std::cout << "Conexion aceptada " << remoteIP << ":" << remotePort << std::endl;

						short x, y;
						x = rand() % 600;
						y = rand() % 400;
						std::pair<short, short> coordenadasPlayer{ x,y };



						for (int i = 0; i < aClients.size(); i++) {
							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::NEWPLAYER, commandBits);
							ombs.Write(aClients[i]->GetCriticalId(), criticalBits);
							ombs.Write(clientID, playerSizeBits);
							ombs.Write(coordenadasPlayer.first, coordsbits);
							ombs.Write(coordenadasPlayer.second, coordsbits);

							CriticalPacket aCritical(ombs.GetBufferPtr(),ombs.GetByteLength(), aClients[i]->GetCriticalId());
							aClients[i]->AddCriticalPacket(aCritical);
						}


						aClients.push_back(new ServerClient(remoteIP.toString(), remotePort, clientID, coordenadasPlayer));
						

						OutputMemoryBitStream ombs;
						ombs.Write(PacketType::WELCOME, commandBits);
						ombs.Write(clientID, playerSizeBits);
						ombs.Write(aClients.size()-1, playerSizeBits);


						for (int j = 0; j < aClients.size(); j++) {

							ombs.Write(aClients.at(j)->GetID(), playerSizeBits);
							ombs.Write(aClients.at(j)->GetPosition().first, coordsbits);
							ombs.Write(aClients.at(j)->GetPosition().second, coordsbits);

						}
						socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

						std::cout << "Se reenvia un welcome a " << remoteIP << ":" << remotePort << std::endl;
						std::cout << "CLIENT ID " << clientID << "\n";

						clientID++;
					}


				}
				else if (command == PacketType::ACK) {
					//int receivedCriticalPlayerID = 0;
					int receivedCriticalID = 0;
					imbs.Read(&receivedCriticalID, criticalBits);

					 ServerClient* aClient = nullptr;

					 // aClient = GetClientWithId(receivedCriticalPlayerID, &aClients);
					  aClient = GetClientWithIpPort(infoReceived.GetPort(), infoReceived.GetAdress().toString(), &aClients);

					  std::cout << "Recibido ACK de client con id" << aClient->GetID() << " y critical con Id " << receivedCriticalID << "\n";


					  if (aClient != nullptr) {
						  aClient->RemoveCriticalPacket(receivedCriticalID);
					  }

				}
				else if (command == PacketType::ACKPING) {
					//unsigned int playerID=1;
					//imbs.Read(&playerID, playerSizeBits);

					//std::cout << "se recibe un Ackping del jugador con id " << playerID << "\n";
					//GetClientWithId(playerID, &aClients)->pingCounter.restart();
					ServerClient* tempClient;
					tempClient = GetClientWithIpPort(infoReceived.GetPort(), infoReceived.GetAdress().toString(), &aClients);
					if (tempClient != nullptr){
						tempClient->pingCounter.restart();
						std::cout << "se recibe un Ackping del jugador con id " << tempClient->GetID() << "\n";
					}else{
						std::cout << "NULL POINTER \n";
					}
					}*/