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

int main() {
	std::cout << "CLIENTE INICIADO" << std::endl;
	std::string serverIp = "localhost";
	int serverPort = 50000;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	sf::Packet infoToSend;
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
	sf::Clock clockForTheServer;

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

				if (aClient != nullptr) {
					aClient->position = someCoords;
					if (aPlayerId == myId) {
						myCoordenates = someCoords;
						auxPosition = myCoordenates;
					}

				}
				else if (aPlayerId == myId) {
					myCoordenates = someCoords;
					auxPosition = myCoordenates;
					std::cout << "MY NEW COORDS: " << myCoordenates.first << ", " << myCoordenates.second << std::endl;
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
					if (event.key.code == sf::Keyboard::Left)
					{
						//socket->send(pckLeft, serverIp, serverPort);
						int deltaX = -1;
						int deltaY = 0;
						auxPosition.first+=deltaX;						
						currentDelta.first += deltaX;
					}
					else if (event.key.code == sf::Keyboard::Right)
					{
						int deltaX = 1;
						int deltaY = 0;
						auxPosition.first += deltaX;
						currentDelta.first += deltaX;
					}
					else if (event.key.code == sf::Keyboard::Up) {
						int deltaX = 0;
						int deltaY = -1;
						auxPosition.second+= deltaY;
						currentDelta.second += deltaY;
					}
					else if (event.key.code == sf::Keyboard::Down) {
						int deltaX = 0;
						int deltaY = 1;
						auxPosition.second += deltaY;
						currentDelta.second+=deltaY;
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