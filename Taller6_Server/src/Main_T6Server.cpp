#include "Match.h"
//#define MAX_MATCHES 10

short GetFreePort(std::map<short,bool>map) {
	for (std::map<short, bool>::iterator it = map.begin(); it != map.end(); ++it) {
		if (!it->second) {
			return it->first;
		}
	}

}

int GetFreeId(std::map<int, bool>map) {
	for (std::map<int, bool>::iterator it = map.begin(); it != map.end(); ++it) {
		if (!it->second) {
			return it->first;
		}
	}

}

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
	bool once = false;
	std::vector<User> users;

	User devildra("devildrake","ab123");
	User rattaspi("rattaspi", "ab123");
	User ah97("ah97", "ab123");
	User urisel("urisel", "ab123");

	users.push_back(devildra);
	users.push_back(rattaspi);
	users.push_back(ah97);
	users.push_back(urisel);

	std::map<short, bool>portMap;
	std::map<int, bool> idMap;
	for (int i = 0; i < 16; i++) {
		portMap[50001 + i] = false;
		idMap[i] = false;
	}

	sf::UdpSocket* socket=new sf::UdpSocket();
	sf::Socket::Status status;
	std::queue<Event*> incomingInfo;
	std::vector<Match> aMatches;

	//std::vector<std::thread*>receptionThreads;
	std::vector<std::thread>receptionThreads;
	//Match Reception Threads
	//std::thread mrt[MAX_MATCHES];

	bool end = false;
	status = socket->bind(50000);
	if (status == sf::Socket::Error) {
		std::cout << "Error bindeando socket\n";
		end = true;
	}

	std::thread r(&ReceptionThreadServer, socket, &end, &incomingInfo);




	Match aMatch1;
	short aPort = GetFreePort(portMap);
	int anId = GetFreeId(idMap);
	aMatch1.gameName = "Match Port " + std::to_string(aPort);
	aMatch1.matchId = anId;
	aMatch1.SetUp(aPort);
	portMap[aPort] = true;
	idMap[anId] = true;
	receptionThreads.push_back(std::thread(&ReceptionThread, &aMatch1));

	aMatches.push_back(aMatch1);

	sf::Clock pingTimer;


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

				ombs.Write(ACKPING, commandBits);
				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

				if (status == sf::Socket::Error) {
					std::cout << "ERROR ENVIANDO\n";
				}
				else {
					std::cout << "Contestando al ping\n";
				}

				break;
			case PacketType::ACKPING: {
				std::cout << "Recibido ACKPING\n";

				ServerClient* aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &connectedClients);
				if (aClient != nullptr) {
					aClient->pingCounter.restart();
					std::cout << "Recibido ACKPING de jugador " + aClient->userName<<"\n";
				}

				break; 
			}
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
			case PacketType::REGISTER: {
				std::string aUserName="";
				std::string aPassWord="";
				imbs.ReadString(&aUserName);
				imbs.ReadString(&aPassWord);

				bool found = false;
				for (int i = 0; i < users.size(); i++) {
					if (users[i].userName == aUserName) {
						found = true;
					}
				}
				OutputMemoryBitStream ombs;

				ombs.Write(PacketType::REGISTER, commandBits);

				if (!found) {
					User user(aUserName, aPassWord);
					users.push_back(user);

					//users[index].connected = true;
					ServerClient* aClient = new ServerClient(remoteIP.toString(), remotePort, 0, std::pair<short, short>(0, 0));
					aClient->SetUserName(users[users.size()-1].userName);
					connectedClients.push_back(aClient);


					ombs.Write(true, boolBit);
				}
				else {
					ombs.Write(false, boolBit);
				}

				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

				if (status == sf::Socket::Error) {
					std::cout << "Error enviando respuesta a registro";
				}
				break;
			}			   
			case PacketType::LOGIN: {
				ombs.Write(PacketType::LOGIN, commandBits);
				std::string userName = "";
				std::string passWord = "";

				imbs.ReadString(&userName);
				imbs.ReadString(&passWord);
				int index = -1;


				for (int i = 0; i < users.size(); i++) {
					std::cout << "Recibido usuario " << userName << ", comparando con " << users[i].userName << std::endl;

					if (users[i].userName == userName) {
						std::cout << "Este usuario existe\n";

						index = i;
					}
				}

				if (index >= 0) {
					if (users[index].passWord == passWord) {
						//users[index].connected = true;
						ServerClient* aClient = new ServerClient(remoteIP.toString(), remotePort, 0, std::pair<short, short>(0, 0));
						aClient->SetUserName(users[index].userName);


						if (GetServerClientWithIpPort(remotePort, remoteIP.toString(), &connectedClients) == nullptr) {
							connectedClients.push_back(aClient);
						}
						else {
							std::cout << "Cliente ya constaba como conectado\n";
						}

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
			case PacketType::CREATEGAME:{

				Match aMatch;
				short aPort= GetFreePort(portMap);
				int anId = GetFreeId(idMap);
				aMatch.gameName = "Match Port " + std::to_string(aPort);
				aMatch.matchId = anId;
				aMatch.SetUp(aPort);
				portMap[aPort] = true;
				idMap[anId] = true;
				receptionThreads.push_back(std::thread(&ReceptionThread, &aMatch));

				aMatches.push_back(aMatch);
				//LUEGO HABRA QUE ACORDARSE DE LIBERAR IDS Y PUERTOS CUANDO SE BORREN PARTIDAS
				ombs.Write(PacketType::UPDATEGAMELIST, commandBits);
				ombs.Write((int)aMatches.size(), matchBits);
				for (int i = 0; i < aMatches.size(); i++) {
					ombs.Write((int)aMatches[i].aClients.size(), playerSizeBits);
					ombs.Write(aMatches[i].matchId, matchBits);
					ombs.WriteString(aMatches[i].gameName);
				}

				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

				if (status == sf::Socket::Error) {
					std::cout << "Error enviando info de matches\n";
				}



				break;
			}
			case PacketType::UPDATEGAMELIST:{
				ombs.Write(PacketType::UPDATEGAMELIST, commandBits);
				ombs.Write((int)aMatches.size(), matchBits);
				for (int i = 0; i < aMatches.size(); i++) {
					ombs.Write((int)aMatches[i].aClients.size(), playerSizeBits);
					ombs.Write(aMatches[i].matchId, matchBits);
					ombs.WriteString(aMatches[i].gameName);
				}
				
				status = socket->send(ombs.GetBufferPtr(),ombs.GetByteLength(),remoteIP, remotePort);

				if (status == sf::Socket::Error) {
					std::cout << "Error enviando info de matches\n";
				}
				else {
					std::cout << "Respondiendo info de matches\n";

				}


				break;
			}
			default:
				break;
			}
			incomingInfo.pop();
		}

		//std::cout << aMatches.size()<<"\n";

		//int anIndex = -1;
		//for (int i = 0; i < aMatches.size(); i++) {
		//	if (aMatches[i].aClients.size() == 0) {
		//		anIndex = i;
		//	}
		//}

		//if (!once) {
		//	once = true;
		//	Match aMatch;
		//	aMatch.SetUp(50001);
		//	aMatch.end = false;
		//	aMatches.push_back(aMatch);
		//	std::thread t(&ReceptionThread, &aMatch);
		//	receptionThreads.push_back(&t);
		//}

		//if (anIndex > 0) {
		//	aMatches[anIndex].end = true;
		//	aMatches[anIndex].socket->unbind();
		//	receptionThreads[anIndex]->join();
		//	//receptionThreads.erase(receptionThreads.begin() + anIndex);
		//	aMatches.erase(aMatches.begin() + anIndex);
		//}


		//Match match;
		//match.SetUp();
		////std::thread t(&ReceptionThread, &match);
		////std::thread s(&PingThread, &match);

		//std::thread s(&ReceptionThread, &match);
		//std::thread t(&PingThread, &match);

		//match.Run();

		//s.join();
		//t.join();

		std::cout << pingTimer.getElapsedTime().asSeconds()<<" segundos \n";

		if (pingTimer.getElapsedTime().asSeconds() >= 5) {
			OutputMemoryBitStream ombs;
			ombs.Write(PING, commandBits);
			for (int i = 0; i < connectedClients.size(); i++) {
				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), connectedClients[i]->GetIP(), connectedClients[i]->GetPort());
				if (status == sf::Socket::Error) {
					std::cout << "Error enviando Ping\n";
				}
				else {
					std::cout << "Enviando ping";
				}
			}
			pingTimer.restart();
		}

		std::queue<int>idsToDisconnect;
		for (int i = 0; i < connectedClients.size(); i++) {
			if (connectedClients[i]->pingCounter.getElapsedTime().asSeconds() > 40) {
				idsToDisconnect.push(connectedClients[i]->id);
			}
		}

		while (idsToDisconnect.size() > 0) {
			int id = idsToDisconnect.front();
			int index = -1;
			for (int i = 0; i < connectedClients.size(); i++) {
				if (connectedClients[i]->id == id) {
					index = i;
				}
			}

			if (index > -1) {
				delete connectedClients[index];
				connectedClients.erase(connectedClients.begin() + index);
			}
			std::cout << "Desconectando un jugador\n";
			idsToDisconnect.pop();
		}

	}

	//for (int i = 0; i < MAX_MATCHES; i++) {
	//	mrt[i].join();
	//}

	while (receptionThreads.size()>0) {
		receptionThreads[0].join();
		receptionThreads.erase(receptionThreads.begin());
	}

	socket->unbind();
	r.join();

	return 0;
}

