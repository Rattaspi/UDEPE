#include <iostream>
#include <queue>
#include <thread>
#include <SFML\Network.hpp>
#include <Client.h>
#include "Utils.h"
#include "Event.h"
#include <SFML\Graphics.hpp>

void ReceptionThread(bool* end, std::queue<Event>* incomingInfo, sf::UdpSocket* socket);

bool ClientExists(std::vector<Client*>aClients,int id);

bool CheckValidMoveId(std::vector<AccumMove>*nonAckMoves, int aCriticalId, std::pair<short,short>someCoords,int* index);

void EraseAccums(std::vector<AccumMove>*nonAckMoves, int until);

int main() {
	std::cout << "CLIENTE INICIADO" << std::endl;
	std::string serverIp = "localhost";
	int serverPort = 50000;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	std::string command;
	int myId = 0;
	std::vector<int>acks;
	std::pair<short, short> myCoordenates{ 0,0 };
	std::pair<short, short> auxPosition{ 0,0 };
	std::pair<short, short> currentDelta{ 0,0 };
	std::vector<AccumMove>nonAckMoves;
	int currentMoveId=0;
	bool end = false; //finalizar programa
	bool connected = false; //controla cuando se ha conectado con el servidor
	std::queue<Event> incomingInfo; //cola de paquetes entrantes
	std::vector<Client*> aClients;
	sf::Clock clockForTheServer, clockForTheStep, clockForMyMovement;
	int timeBetweenSteps = 3; //tiempo que tardan en actualizarse las posiciones interpoladas de los otros clientes.

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);

	sf::Clock clock;

	sf::RenderWindow window(sf::VideoMode(800, 600), "Sin acumulación en cliente");
	std::vector<sf::RectangleShape> playerRenders;

	while (!end) {
		//CONTROL DE REENVIOS POR HASTA QUE EL SERVIDOR NOS CONFIRMA LA CONEXION
		if (clock.getElapsedTime().asMilliseconds() > 500 && !connected) {
			OutputMemoryBitStream ombs;
			ombs.Write(PacketType::HELLO, commandBits);
			status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(),serverIp,serverPort);

			if (status != sf::Socket::Done) {
				std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
			}
			else if(status==sf::Socket::Done){
				std::cout << "Intentando conectar al servidor..." << std::endl;
			}
			clock.restart();
		}

		if (!incomingInfo.empty()) {
			Event inc;
			inc = incomingInfo.front();
			Event infoReceived;
			PacketType command = HELLO;
			infoReceived = incomingInfo.front();
			InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
			imbs.Read(&command, commandBits);
			int playerSize = 0;
			int aPlayerId = 0;
			int aCriticalId = 0;
			std::pair<short, short>someCoords = { 0,0 };
			OutputMemoryBitStream ombs;
			//Client* aClient = nullptr;
			int anIndex=0;
			switch (command) {
			case PacketType::WELCOME:
				imbs.Read(&myId,playerSizeBits);
				imbs.Read(&playerSize, playerSizeBits);
				playerSize++;
				for (int i = 0; i < playerSize; i++) {
					Client* aClient = new Client();
					aClient->position.first =aClient->position.second=0;

					imbs.Read(&aClient->id, playerSizeBits);
					imbs.Read(&aClient->position.first, coordsbits);
					imbs.Read(&aClient->position.second, coordsbits);

					if (aClient->id != myId&&myId>0) {
						std::cout << "Recibiendo cliente preexistente con id " << aClient->id << " y coordenadas " << aClient->position.first << "," << aClient->position.second << std::endl;
						aClients.push_back(aClient);
						sf::RectangleShape playerRender(sf::Vector2f(60, 60));
						playerRenders.push_back(playerRender);

					}
					else {
						myCoordenates = aClient->position;
						auxPosition = myCoordenates;
						delete aClient;
					}
				}

				connected = true;
				std::cout << "MY ID IS " << myId << std::endl;
				window.setTitle(std::to_string(socket->getLocalPort()));

				break;
			case PacketType::PING:
				
				ombs.Write(PacketType::ACKPING, commandBits);
				socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
				clockForTheServer.restart();
				break;
			case PacketType::NEWPLAYER:
				for (int i = 0; i < 1; i++) {
					Client* newPlayer = new Client();
					imbs.Read(&aCriticalId, criticalBits);
					std::cout << "PUSHING ACK WITH ID" << aCriticalId << " NEW PLAYER " << std::endl;
					imbs.Read(&aPlayerId, playerSizeBits);
					newPlayer->id = aPlayerId;
					imbs.Read(&newPlayer->position.first, coordsbits);
					imbs.Read(&newPlayer->position.second, coordsbits);
					acks.push_back(aCriticalId);

					sf::RectangleShape playerRender(sf::Vector2f(60, 60));

					playerRenders.push_back(playerRender);

					if (GetClientWithId(newPlayer->id, aClients) == nullptr) {
						aClients.push_back(newPlayer);
						std::cout << "Adding player with id " << newPlayer->id << std::endl;
					}
					else {
						std::cout << "Already existing player received\n";
						delete newPlayer;
					}
				}
				break;
			case PacketType::DISCONNECT:
				std::cout << "DISCONNEEEEEEEEEEEEEEEEEEEEEEEEEEEEEECT\n";
					imbs.Read(&aCriticalId, criticalBits);
					imbs.Read(&aPlayerId, playerSizeBits);

					acks.push_back(aCriticalId);
					std::cout << "PUSHING ACK WITH ID" << aCriticalId << " DISCONNECTION " << std::endl;
					anIndex = GetIndexClientWithId(aPlayerId,&aClients);
					//aClient = GetClientWithId(leaverId, aClients);
					//if (aClient!=nullptr) {
					//	std::cout << "Player " << leaverId << " Disconnected\n";
					//	
					//}
					if (anIndex != -1) {
						aClients.erase(aClients.begin() + anIndex);
						playerRenders.erase(playerRenders.begin()+anIndex);
						std::cout << "Player " << aPlayerId << " Disconnected\n";

					}
					else {
						std::cout << "Trying to disconnect non existing player with id " << aPlayerId << std::endl;
					}
				break;
			case PacketType::NOTWELCOME:
				std::cout << "SERVER FULL\n";
				end = true;
				break;
			case PacketType::ACKMOVE:

				imbs.Read(&aPlayerId, playerSizeBits);
				imbs.Read(&aCriticalId, criticalBits); //Se lee aCriticalId pero en realidad es el ID del MOVIMIENTO
				imbs.Read(&someCoords.first, coordsbits);
				imbs.Read(&someCoords.second, coordsbits);
				AccumMove accumMove;
				Client* aClient = GetClientWithId(aPlayerId,aClients);

				if (aClient != nullptr) { //Si no es nullptr no eres tu mismo.
					//aClient->position = someCoords;

					std::cout << "Recibido ACKMOVE de jugador con ID " << aPlayerId << "Sus coordenadas son " << someCoords.first <<", "<<someCoords.second<< std::endl;

					std::pair<float, float>distance;
					std::pair <short, short>lastPosition;
					if (aClient->steps.size() > 0) {
						 lastPosition = aClient->steps.back();
					}
					else {
						lastPosition = aClient->position;
					}

					

					distance.first = (someCoords.first - lastPosition.first);
					distance.second = someCoords.second - lastPosition.second;
					distance.first /= subdividedSteps;
					distance.second /= subdividedSteps;
					//std::cout << "someCoords -> " << someCoords.first << ", " << someCoords.second << " - lastPosition "<<lastPosition.first << ", " << lastPosition.second <<"\n";
					

					for (int i = 0; i < subdividedSteps; i++) {
						std::pair<short, short>aStep;
						aStep.first = lastPosition.first + (short)std::floor(distance.first*i);
						aStep.second = lastPosition.second + (short)std::floor(distance.second*i);

						if (aStep.first != lastPosition.first || aStep.second != lastPosition.second) {

							//std::cout << std::floor(distance.first*i)<<"\n";
							aClient->steps.push(aStep);
							std::cout << "Pushing step with coords-> " << aStep.first << ", " << aStep.second << "\n";

						}
					}
					/*	if (aPlayerId == myId) {
						myCoordenates = someCoords;
						auxPosition = myCoordenates;
					}*/

				}else if (aPlayerId == myId) {
					int ackIndex = 0;
					if (!CheckValidMoveId(&nonAckMoves, aCriticalId, someCoords, &ackIndex)) {
						myCoordenates = someCoords;
						auxPosition = myCoordenates;
						//std::cout << "MY NEW COORDS: " << myCoordenates.first << ", " << myCoordenates.second << std::endl;
						EraseAccums(&nonAckMoves, (int)(nonAckMoves.size() - 1));
						//std::cout << "NonAckMoves-> " << nonAckMoves.size()<<std::endl;
					}
					else {
						EraseAccums(&nonAckMoves, ackIndex);
						//std::cout << "NonAckMoves-> " << nonAckMoves.size() << std::endl;
					}
				}

				//std::cout << "Recibida posicion de jugador con ID " <<aPlayerId<< ".Sus coordenadas son "<<someCoords.first<<", "<<someCoords.second<<"\n";

		/*		
				

				ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first, coordsbits);
				ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second, coordsbits);*/
				break;
			}

			incomingInfo.pop();
		}

		if (acks.size() > 0) {
			for (int i = 0; i < acks.size(); i++) {
				OutputMemoryBitStream ombs;
				ombs.Write(PacketType::ACK, commandBits);
				ombs.Write(acks[i], criticalBits);
				std::cout << "Sending ack " << acks[i] << std::endl;
				socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
			}
			acks.clear();
		}


		if (window.isOpen()) {
			sf::Event event;

			while (window.pollEvent(event))
			{
				switch (event.type)
				{
				case sf::Event::Closed:
					window.close();
					break;
				case sf::Event::KeyPressed:
					if (clockForMyMovement.getElapsedTime().asMilliseconds() > timeBetweenSteps) {
						if (event.key.code == sf::Keyboard::Left)
						{
							//socket->send(pckLeft, serverIp, serverPort);
							int deltaX = -1;
							int deltaY = 0;
							auxPosition.first += deltaX;
							currentDelta.first += deltaX;
							myCoordenates = auxPosition;
						}
						else if (event.key.code == sf::Keyboard::Right)
						{
							int deltaX = 1;
							int deltaY = 0;
							auxPosition.first += deltaX;
							currentDelta.first += deltaX;
							myCoordenates = auxPosition;
						}
						else if (event.key.code == sf::Keyboard::Up) {
							int deltaX = 0;
							int deltaY = -1;
							auxPosition.second += deltaY;
							currentDelta.second += deltaY;
							myCoordenates = auxPosition;
						}
						else if (event.key.code == sf::Keyboard::Down) {
							int deltaX = 0;
							int deltaY = 1;
							auxPosition.second += deltaY;
							currentDelta.second += deltaY;
							myCoordenates = auxPosition;
						}
						clockForMyMovement.restart();
					}
					else if (event.key.code == sf::Keyboard::Escape) {
						std::cout << "Application Closed\n";
						window.close();
						connected = false;
						end = true;
					}
					break;
				default:
					break;
				}
			}


			window.clear();

			//sf::RectangleShape rectBlanco(sf::Vector2f(1, 600));
			//rectBlanco.setFillColor(sf::Color::White);
			//rectBlanco.setPosition(sf::Vector2f(200, 0));
			//window.draw(rectBlanco);
			//rectBlanco.setPosition(sf::Vector2f(600, 0));
			//window.draw(rectBlanco);

			//sf::RectangleShape rectAvatar(sf::Vector2f(60, 60));
			//rectAvatar.setFillColor(sf::Color::Green);
			//rectAvatar.setPosition(sf::Vector2f(myCoordenates.first, myCoordenates.second));
			//window.draw(rectAvatar);


			if (clock.getElapsedTime().asMilliseconds() > 120&&connected) {
				if (currentDelta.first != 0 || currentDelta.second != 0) {
					AccumMove move;
					move.absolute = auxPosition;
					//std::cout << "ENVIANDO AUX " << auxPosition.first << ", " << auxPosition.second << "\n";

					if (currentMoveId > 255) {
						currentMoveId = 0;
					}

					move.idMove = currentMoveId;
					currentMoveId++;
					nonAckMoves.push_back(move);
					OutputMemoryBitStream ombs;
					ombs.Write(PacketType::MOVE, commandBits);
					ombs.Write(move.idMove, criticalBits);
					ombs.Write(currentDelta.first, deltaMoveBits);
					ombs.Write(currentDelta.second, deltaMoveBits);
					ombs.Write(auxPosition.first, coordsbits);
					ombs.Write(auxPosition.second, coordsbits);

					int lossRate = LOSSRATE
					if ((int)(rand() % 100) > lossRate) {
					status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
					}
					else {
						std::cout << "Move perdido a posta\n";
					}
					
					if (status == sf::Socket::Error) {
						std::cout << "ERROR ENVIANDO MOVE\n";
					}
					currentDelta = { 0,0 };
				}
				clock.restart();
			}
			if (clockForTheStep.getElapsedTime().asMilliseconds() > timeBetweenSteps) {
				for (int i = 0; i < aClients.size(); i++) {
						if (aClients[i]->steps.size() > 0) {
							aClients[i]->position = aClients[i]->steps.front();
							aClients[i]->steps.pop();
						}
					}
				clockForTheStep.restart();
			}

			//DRAW DE PERSONAJES
			for (int i = 0; i < aClients.size(); i++) {
				//sf::RectangleShape rectAvatar(sf::Vector2f(60, 60));
				playerRenders[i].setFillColor(sf::Color::Green);
				playerRenders[i].setPosition(sf::Vector2f(aClients[i]->position.first, aClients[i]->position.second));
				window.draw(playerRenders[i]);
			}

			sf::RectangleShape myRender(sf::Vector2f(60, 60));
			myRender.setFillColor(sf::Color::Blue);
			myRender.setPosition(myCoordenates.first, myCoordenates.second);
			window.draw(myRender);

			window.display();

		}

		if (clockForTheServer.getElapsedTime().asMilliseconds() > 30000) {
			end = true;
			std::cout << "SERVIDOR DESCONECTADOOOO\n";
			socket->unbind();
		}

	}
	
	if (window.isOpen()) {
		window.close();
	}
	socket->unbind();
	t.join();
	system("pause");
	delete socket;
	exit(0);
	return 0;
}

void EraseAccums(std::vector<AccumMove>*nonAckMoves, int until) {
	nonAckMoves->erase(nonAckMoves->begin(), nonAckMoves->begin() + until);
}

bool CheckValidMoveId(std::vector<AccumMove>*nonAckMoves, int aCriticalId, std::pair<short, short>someCoords, int* index) {
	for (int i = 0; i < nonAckMoves->size(); i++) {
		if (nonAckMoves->at(i).idMove == aCriticalId) {
			*index = i;
			if (someCoords == nonAckMoves->at(i).absolute) {
				return true;
			}
			else {
				return false;
			}
		}
	}
	*index = -1;
	return true;
}

bool ClientExists(std::vector<Client*>aClients,int id) {
	for (int i = 0; i < aClients.size(); i++) {
		if (aClients[i]->id == id){
			return true;
		}
	}
	return false;
}

void ReceptionThread(bool* end, std::queue<Event>* incomingInfo, sf::UdpSocket* socket) {
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
			//std::cout << "Paquete recibido correctamente" << std::endl;
			incomingInfo->push(Event(message, sizeReceived, incomingIP, incomingPort));

		}
	}
}