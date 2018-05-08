#pragma once
#include <Client.h>
#include <queue>
#include <thread>
#include <Event.h>
#include <random>
#include <time.h>
#include "Utils.h"

class Match {
	short gamePort;

	static void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket);

	static void PingThread(bool* end, std::vector<ServerClient*>* aClients, sf::UdpSocket* socket);

	void DisconnectPlayer(std::vector<ServerClient*>* aClients, ServerClient* aClient);

	void SendBallPos(std::vector<ServerClient*>*aClients, sf::UdpSocket* socket, std::pair<short, short> ballPos);

	void UpdateBall(std::pair<float, float>* coords, std::pair<float, float>*speed, float delta, int*, int*, std::vector<ServerClient*>*, sf::UdpSocket*, bool*);

	int GetAvailableId(std::vector<ServerClient*>aClients, int num);

	bool CheckGameOver(int leftScore, int rightScore, std::vector<ServerClient*>*aClients, sf::UdpSocket* socket);

public:

	Match(short gamePort) {
		this->gamePort = gamePort;
	}
	void Run() {
		sf::Clock criticalClock;
		srand(time(NULL));
		std::cout << "INICIANDO SERVIDOR..." << std::endl;
		int clientID = 0;
		std::vector<ServerClient*> aClients;
		sf::Socket::Status status;
		sf::UdpSocket* socket = new sf::UdpSocket();
		status = socket->bind(gamePort);
		bool end = false;
		bool sentEnd = false;
		bool gameHadStarted = false;
		std::queue<Event*> incomingInfo; //cola de paquetes entrantes
		std::vector<std::vector<AccumMoveServer>> acumulatedMoves;
		std::pair<short, short>initialPositions[4] = { { 200,200 },{ 200,400 },{ 800,200 },{ 800,400 } }; //formación inicial de los jugadores

		std::pair<float, float> ballCoords = ballStartPos;
		std::pair<float, float>* ballSpeed = new std::pair<float, float>(0, 0);
		std::pair<float, float> auxBallSpeed{ 0,0 };
		sf::Clock ballClock;
		sf::Clock gameClock;
		sf::Clock gameOverClock;
		bool gameOver = false;
		int leftScore = 0;
		int rightScore = 0;

		if (status != sf::Socket::Done) {
			std::cout << "No se ha podido vincular al puerto" << std::endl;
			system("pause");
			if (ballSpeed != nullptr)
				delete ballSpeed;

			exit(0);
		}

		std::thread t(&ReceptionThread, &end, &incomingInfo, socket);
		std::thread s(&PingThread, &end, &aClients, socket);
		ballClock.restart();
		while (!end) {
			if (aClients.size() == numPlayers) {
				if (gameClock.getElapsedTime().asSeconds() > 5) {
					if (!gameHadStarted) {


						for (int i = 0; i < aClients.size(); i++) {
							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::GAMESTART, commandBits);
							ombs.Write(aClients[i]->criticalId, criticalBits);
							aClients[i]->AddCriticalMessage(new CriticalMessage(aClients[i]->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
						}
						gameHadStarted = true;
					}
				}
			}
			else if (!gameHadStarted) {
				gameClock.restart();
			}

			if (gameHadStarted) {
				if (ballClock.getElapsedTime().asMilliseconds() > 100) {
					UpdateBall(&ballCoords, ballSpeed, ballClock.getElapsedTime().asSeconds(), &leftScore, &rightScore, &aClients, socket, &gameOver);
					SendBallPos(&aClients, socket, ballCoords);
					ballClock.restart();
				}
			}

			if (gameOver) {
				if (gameOverClock.getElapsedTime().asSeconds() > 5) {
					end = true;
				}
			}
			else {
				gameOverClock.restart();
			}

			if (!incomingInfo.empty()) {
				Event infoReceived;
				sf::IpAddress remoteIP;
				unsigned short remotePort;
				int command = HELLO;
				infoReceived = *incomingInfo.front();
				remoteIP = infoReceived.GetAdress();
				remotePort = infoReceived.GetPort();

				InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
				imbs.Read(&command, commandBits);
				OutputMemoryBitStream ombs;
				sf::Socket::Status status;
				bool exists = false;
				ServerClient* aClient;
				int aCriticalId = 0;
				int repeatingId = 0;
				std::pair<short, short> coords{ 0,0 };
				std::pair<short, short> auxCoords{ 0,0 };
				switch (command) {
				case HELLO:
					//coords.first = rand() % 740;
					//coords.second = rand() % 540;

					for (int i = 0; i < aClients.size(); i++) {
						if (aClients[i]->GetIP() == remoteIP&&aClients[i]->GetPort() == remotePort) {
							exists = true;
							repeatingId = aClients[i]->GetID();
							coords = initialPositions[aClients[i]->GetID()];

						}
					}

					if (aClients.size() < 4) {
						ombs.Write(PacketType::WELCOME, commandBits);

						if (!exists) {
							clientID = GetAvailableId(aClients, numPlayers);
							aClients.push_back(new ServerClient(remoteIP.toString(), remotePort, clientID, initialPositions[clientID]));
							coords = initialPositions[clientID];
							ombs.Write(clientID, playerSizeBits);
							repeatingId = clientID;
							clientID++;
						}
						else {
							ombs.Write(repeatingId, playerSizeBits);
						}

						int aClientSizeMinus = aClients.size();
						aClientSizeMinus--;
						ombs.Write(aClientSizeMinus, playerSizeBits);

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
								if (aClients[i]->id != repeatingId) {
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
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &aClients);
					if (aClient != nullptr) {

						imbs.Read(&aCriticalId, criticalBits);
						aClient->RemoveCriticalPacket(aCriticalId);
						//imbs.Read(&aCriticalId, criticalBits);
						//aClient->RemoveCriticalPacket(aCriticalId);
						std::cout << "Recibido ACK " << aCriticalId << "Del jugador con ID " << aClient->GetID() << std::endl;

					}
					break;
				case ACKPING:
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &aClients);
					if (aClient != nullptr) {
						aClient->pingCounter.restart();
						std::cout << "Recibido ACKPING de jugador " << aClient->GetID() << std::endl;
					}
					break;
				case SHOOT:
					if (gameHadStarted) {
						std::cout << "SHOOT RECIBIDO\n";
						aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &aClients);

						if (aClient != nullptr) {
							coords.first = 0;
							coords.second = 0;
							auxCoords.first = 0;
							auxCoords.second = 0;

							//coords ->Jugador
							imbs.Read(&coords.first, coordsbits);
							imbs.Read(&coords.second, coordsbits);
							coords.first += playerRadius;
							coords.second += playerRadius;

							//auxCoords ->Pelota en su simulacion
							imbs.Read(&auxCoords.first, coordsbits);
							imbs.Read(&auxCoords.second, coordsbits);

							auxCoords.first += ballRadius;
							auxCoords.second += ballRadius;

							//if (std::sqrt(std::pow(coords.first - auxCoords.first, 2) + (std::pow(coords.second - auxCoords.second, 2))) < 15) {
							//ballSpeed->first = auxCoords.first - coords.first;
							//ballSpeed->second = auxCoords.second - coords.second;

							auxBallSpeed.first = auxCoords.first - coords.first;
							auxBallSpeed.second = auxCoords.second - coords.second;


							if (aClient->shootClock.getElapsedTime().asMilliseconds() > shootCoolDown) {
								float magnitude = auxBallSpeed.first * auxBallSpeed.first + auxBallSpeed.second * auxBallSpeed.second;

								//std::cout << "Magnitude = " << magnitude << std::endl;
								std::cout << "resta = " << ballSpeed->first << ", " << ballSpeed->second << std::endl;

								magnitude = std::sqrt(magnitude);
								std::cout << "Magnitude = " << magnitude << std::endl;

								if (magnitude < ballRadius + playerRadius + 5) {

									aClient->shootClock.restart();

									std::cout << "Raiz Magnitude = " << magnitude << std::endl;
									*ballSpeed = auxBallSpeed;

									ballSpeed->first /= magnitude;
									ballSpeed->second /= magnitude;

									std::cout << "BallSpeed dividida magnitude " << ballSpeed->first << ", " << ballSpeed->second << std::endl;

									ballSpeed->first *= shootStrength;
									ballSpeed->second *= shootStrength;

									std::cout << "BallSpeed dividida magnitude por 10 " << ballSpeed->first << ", " << ballSpeed->second << std::endl;


									//ballSpeed.first = std::floor(ballSpeed.first);
									//ballSpeed.second = std::floor(ballSpeed.second);

									//std::cout << "Recibido Shoot, new BallSpeed = " << ballSpeed.first << ", " << ballSpeed.second << std::endl;
								}
							}

							//}

						}
					}

					break;
				case MOVE:
					if (aClients.size() > 0) {
						aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &aClients);
						if (aClient != nullptr) {
							AccumMoveServer accumMove;
							accumMove.playerID = aClient->id;
							imbs.Read(&accumMove.idMove, criticalBits);
							imbs.Read(&accumMove.absolute.first, coordsbits);
							imbs.Read(&accumMove.absolute.second, coordsbits);
							//std::cout << "Recibido accumMove de jugador ID " << aClient->id << " que tiene delta = " << accumMove.delta.first << ", " << accumMove.delta.second << " y absolute = " << accumMove.absolute.first << ", " <<  accumMove.absolute.second << std::endl;;
							//acumulatedMoves.push_back(accumMove);
							aClient->acumulatedMoves.push_back(accumMove);
						}
						else {
							std::cout << "Null player trying to send movemen\n";
						}
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
				if (aClients[i]->pingCounter.getElapsedTime().asMilliseconds() > 20000) {
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

						if ((aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first + playerRadius) < 1000 && (aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first) > 0 && (aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second + playerRadius) < 600 && (aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second) > 0) {
							ombs.Write(PacketType::ACKMOVE, commandBits);
							ombs.Write(aClients[i]->id, playerSizeBits);
							ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].idMove, criticalBits);
							//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.first,deltaMoveBits);
							//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.second, deltaMoveBits);


							ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first, coordsbits);
							ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second, coordsbits);
							aClients[i]->position = aClients[i]->acumulatedMoves[latestMessageIndex].absolute;
							aClients[i]->acumulatedMoves.clear();

							//std::cout << "Enviada posicion de jugador con ID " << aClients[i]->GetID() << ".Sus coordenadas son " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first << ", " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second << "\n";


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

							//std::cout << "Enviada posicion de jugador con ID " << aClients[i]->GetID() << ".Sus coordenadas son " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first << ", " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second << "\n";


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
		if (ballSpeed != nullptr) {
			delete ballSpeed;
		}
	}
};