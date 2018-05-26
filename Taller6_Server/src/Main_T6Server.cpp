#include "Match.h"
#include "DBManager.h"
//#define MAX_MATCHES 10

void PingThread(Match* match);
void MatchThread(Match* match);
unsigned short GetFreePort(std::map<unsigned short, bool>map);
int GetFreeId(std::map<int, bool>map);
void ReceptionThreadServer(sf::UdpSocket* socket, bool*end, std::queue<Event*>*incomingInfo);
void ReceptionThread(Match* match);
Match*GetMatchWithId(int id, std::vector<Match*>*matches);
static void AddPlayer(ServerClient* client, Match* match);

void MatchRoomThread(Match* match);


int main() {
	bool once = false;
	std::vector<User> users;
	DBManager* dbm = new DBManager();

	std::map<unsigned short, bool>portMap;
	std::map<int, bool> idMap;
	for (int i = 0; i < 16; i++) {
		portMap[50001 + i] = false;
		idMap[i] = false;
	}

	sf::UdpSocket* socket=new sf::UdpSocket();
	sf::Socket::Status status;
	std::queue<Event*> incomingInfo;
	std::vector<Match*> aMatches;

	//std::vector<std::thread*>receptionThreads;
	std::vector<std::thread>receptionThreads;
	std::vector<std::thread>pingThreads;
	std::vector<std::thread>matchThreads;
	std::vector<std::thread>matchRoomThreads;
	//Match Reception Threads
	//std::thread mrt[MAX_MATCHES];

	bool end = false;
	status = socket->bind(50000);
	if (status == sf::Socket::Error) {
		std::cout << "Error bindeando socket\n";
		end = true;
	}

	std::thread r(&ReceptionThreadServer, socket, &end, &incomingInfo);

	//Match* aMatch1 = new Match();

	//unsigned short aPort = GetFreePort(portMap);
	//int anId = GetFreeId(idMap);
	//aMatch1->gameName = "Match Port " + std::to_string(aPort);
	//aMatch1->matchId = anId;
	//aMatch1->SetUp(aPort);
	//portMap[aPort] = true;
	//idMap[anId] = true;

	//receptionThreads.push_back(std::thread(&ReceptionThread, aMatch1));
	//pingThreads.push_back(std::thread(&PingThread, aMatch1));
	//matchRoomThreads.push_back(std::thread(&MatchRoomThread, aMatch1));

	//aMatches.push_back(aMatch1);

	sf::Clock pingTimer;

	std::vector<ServerClient*>connectedClients;

	while (!end) {
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
				//std::cout << "Recibido ACKPING\n";

				ServerClient* aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &connectedClients);
				if (aClient != nullptr) {
					aClient->pingCounter.restart();
					//std::cout << "Recibido ACKPING de jugador " + aClient->userName<<"\n";
				}

				break; 
			}
			case PacketType::JOINGAME: {
				std::cout << "RecibidoJoinGame\n";
				int aMatchId = 0;
				imbs.Read(&aMatchId,matchBits);

				ServerClient* aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &connectedClients);
				Match* aMatch = GetMatchWithId(aMatchId, &aMatches);
				
				if (aClient != nullptr) {
					if (aMatch != nullptr) {
						if (!aMatch->ContainsClient(aClient)) {
							if (aClient->matchId > -1) {
								if (aMatch->aClients.size() < 4) {
									//////////////////////////////////////////////////////////////AQUI IRIA LA MAX CAPACIDAD DEL MATCH
									ombs.Write(PacketType::JOINGAME, commandBits);

									//ombs.Write(1, boolBit);
									ombs.Write(aMatch->matchPort, portBits);
									AddPlayer(aClient, aMatch);

									int clientSizeInt = aMatch->aClients.size();
									ombs.Write(clientSizeInt, playerSizeBits);

									for (int j = 0; j<aMatch->aClients.size(); j++) {
										ombs.WriteString(aMatch->aClients[j]->userName);
									}

									std::cout << "Enviando puerto" << aMatch->matchPort<<" \n";

								}else {
									//ombs.Write(0, boolBit);
									ombs.Write(PacketType::NOJOINGAME, commandBits);
									std::cout << "B\n";

								}
							}else {
								ombs.Write(PacketType::NOJOINGAME, commandBits);

								//ombs.Write(0, boolBit);
								std::cout << "C\n";

							}
						}else {
							//ombs.Write(1, boolBit);
							ombs.Write(PacketType::JOINGAME, commandBits);

							ombs.Write(aMatch->matchPort, portBits);
							std::cout << "Enviando puerto" << aMatch->matchPort << " \n";
							//AddPlayer(aClient, aMatch);
							std::cout << "D\n";

							int clientSizeInt = aMatch->aClients.size();
							ombs.Write(clientSizeInt, playerSizeBits);

							for (int j = 0; j<aMatch->aClients.size(); j++) {
								ombs.WriteString(aMatch->aClients[j]->userName);
							}

						}
					}else {
						ombs.Write(PacketType::NOJOINGAME, commandBits);
						//ombs.Write(0, boolBit);
						std::cout << "E\n";

					}
					status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), aClient->GetIP(), aClient->GetPort());

					if (status == sf::Socket::Error) {
						std::cout << "Error enviando respuesta de petición de unirse a partida\n";
					}
				}else {
					//NULLCLIENT
					std::cout << "NULLCLIENT\n";
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
				if (index > -1) {
					dbm->EndSession(connectedClients[index]->sessionID, connectedClients[index]->games);
					std::cout << "DISCONNECTING PLAYER" << std::endl;
					delete connectedClients[index];
					connectedClients.erase(connectedClients.begin() + index);
				}

				break;
			}
			case PacketType::REGISTER: {
				//TODO implementar el registro contra la base de datos 
				//bool done = dbm->Register(user, pass);
				std::string aUserName="";
				std::string aPassWord="";
				imbs.ReadString(&aUserName);
				imbs.ReadString(&aPassWord);

				bool ok = dbm->Register(aUserName, aPassWord);
				//for (int i = 0; i < users.size(); i++) {
				//	if (users[i].userName == aUserName) {
				//		found = true;
				//	}
				//}
				OutputMemoryBitStream ombs;

				ombs.Write(PacketType::REGISTER, commandBits);

				if (ok) {
					int sessionID = dbm->StartSession(dbm->GetUserID(aUserName));
					ServerClient* aClient = new ServerClient(remoteIP.toString(), remotePort, 0, std::pair<short, short>(0, 0), sessionID, aUserName);
					connectedClients.push_back(aClient);
				}
				
				ombs.Write(ok, boolBit);

				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

				if (status == sf::Socket::Error) {
					std::cout << "Error enviando respuesta a registro";
				}
				break;
			}			   
			case PacketType::LOGIN: {
				//TODO comprovar el usuario y contraseña contra la base de datos
				ombs.Write(PacketType::LOGIN, commandBits);
				std::string userName = "";
				std::string passWord = "";

				imbs.ReadString(&userName);
				imbs.ReadString(&passWord);
				
				bool ok = dbm->Login(userName, passWord);
				bool connected = false;

				for (int i = 0; i < connectedClients.size(); i++) {
					if (connectedClients[i]->userName == userName) {
						connected = true;
					}
				}

				if (ok&&!connected) {
					int sessionID = dbm->StartSession(dbm->GetUserID(userName));
					ServerClient* aClient = new ServerClient(remoteIP.toString(), remotePort, 0, std::pair<short, short>(0, 0), sessionID, userName);
					connectedClients.push_back(aClient);
				}
				else {
					ok = false;
				}
				ombs.Write(ok, boolBit);

				socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

				break;
			}
			case PacketType::CREATEGAME:{

				std::string gameName = "";
				std::string gamePass = "";
				int goals = 0;

				imbs.ReadString(&gameName);
				imbs.ReadString(&gamePass);
				imbs.Read(&goals, commandBits);

				Match* aMatch= new Match();
				unsigned short aPort= GetFreePort(portMap);
				int anId = GetFreeId(idMap);
				aMatch -> gameName = gameName;//"Match Port " + std::to_string(aPort);
				aMatch->winScore = goals;
				aMatch->matchId = anId;
				aMatch->SetUp(aPort);
				portMap[aPort] = true;
				idMap[anId] = true;
				receptionThreads.push_back(std::thread(&ReceptionThread, aMatch));
				matchRoomThreads.push_back(std::thread(&MatchRoomThread, aMatch));
				aMatches.push_back(aMatch);
				aMatches[aMatches.size() - 1]->gameName = gameName;
				std::cout << "NAME " << aMatches[aMatches.size()-1]->gameName + "\n";

				//LUEGO HABRA QUE ACORDARSE DE LIBERAR IDS Y PUERTOS CUANDO SE BORREN PARTIDAS

				//ombs.Write(PacketType::UPDATEGAMELIST, commandBits);
				//ombs.Write((int)aMatches.size(), matchBits);
				//for (int i = 0; i < aMatches.size(); i++) {
				//	ombs.Write((int)aMatches[i]->aClients.size(), playerSizeBits);
				//	ombs.Write(aMatches[i]->matchId, matchBits);
				//	ombs.WriteString(aMatches[i]->gameName);
				//}

				ombs.Write(PacketType::CREATEGAME, commandBits);
				ombs.Write(true, boolBit);

				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

				if (status == sf::Socket::Error) {
					std::cout << "Error enviando info de matches\n";
				}



				break;
			}
			case PacketType::UPDATEGAMELIST:{
				std::cout << "EventosSize" << incomingInfo.size() << std::endl;
				ombs.Write(PacketType::UPDATEGAMELIST, commandBits);
				ombs.Write((int)aMatches.size(), matchBits);
				for (int i = 0; i < aMatches.size(); i++) {
					ombs.Write((int)aMatches[i]->aClients.size(), playerSizeBits);
					ombs.Write(aMatches[i]->matchId, matchBits);
					std::cout << "Enviando nombre de nivel " <<aMatches[i]->gameName<<std::endl;
					ombs.WriteString(aMatches[i]->gameName);
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
				std::cout << "Comando no aceptado: " << command << std::endl;
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

		//std::cout << pingTimer.getElapsedTime().asSeconds()<<" segundos \n";

		for (int i = 0; i < aMatches.size(); i++) {

			if (!aMatches[i]->startFlag&&aMatches[i]->gameHadStarted) {
				int aClientSizeMinus = aMatches[i]->aClients.size();
				matchRoomThreads[i].join();
				matchRoomThreads.erase(matchRoomThreads.begin() + i);
				std::cout << "Iniciado thread de Partida, terminando thread de room\n";
				matchThreads.push_back(std::thread(&MatchThread, aMatches[i]));
				aMatches[i]->startFlag = true;
				for (int j = 0; j < aMatches[i]->aClients.size(); j++) {
					OutputMemoryBitStream ombs;

					ombs.Write(PacketType::WELCOME, commandBits);

					aMatches[i]->aClients[j]->matchId = j;
					ombs.Write(aMatches[i]->aClients[j]->criticalId, criticalBits);
					ombs.Write(aMatches[i]->winScore, criticalBits);

					ombs.Write(aMatches[i]->aClients[j]->GetMatchID(), playerSizeBits);


					ombs.Write(aClientSizeMinus, playerSizeBits);

					//std::cout << "Escribiendo aClientSizeMinus " << aClientSizeMinus << std::endl;;


					for (int k = 0; k < aMatches[i]->aClients.size(); k++) {

						//std::cout << "Escribiendo idPlayer " << aMatches[i]->aClients[k]->GetMatchID() << std::endl;;


						ombs.Write(k, playerSizeBits);
						//std::cout << "Adding player with id " << aMatches[i]->aClients[k]->GetMatchID() <<"and coords 0,0"<< std::endl;
						ombs.Write((short)600, coordsbits);
						ombs.Write((short)600, coordsbits);

						//ombs.Write(aMatches[i]->aClients[k]->GetMatchID(), playerSizeBits);
						//ombs.Write(aMatches[i]->aClients[k]->GetPosition().first, coordsbits);
						//ombs.Write(aMatches[i]->aClients[k]->GetPosition().second, coordsbits);
					}


					//std::cout << "Creado un Welcome con informacion de " << aMatches[i]->aClients.size() << "\n";


					//std::cout << "WelcomeAdded\n";
					aMatches[i]->aClients[j]->AddCriticalMessage(new CriticalMessage(aMatches[i]->aClients[j]->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));

				}


			}
		}

		if (pingTimer.getElapsedTime().asSeconds() >= 5) {
			OutputMemoryBitStream ombs;
			ombs.Write(PING, commandBits);
			for (int i = 0; i < connectedClients.size(); i++) {
				status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), connectedClients[i]->GetIP(), connectedClients[i]->GetPort());
				if (status == sf::Socket::Error) {
					std::cout << "Error enviando Ping\n";
				}
				else {
					//std::cout << "Enviando ping\n";
				}
			}
			//std::cout << "Se supone que ha mandado "<< connectedClients.size() <<" pings\n";
			pingTimer.restart();
		}

		std::queue<int>idsToDisconnect;
		for (int i = 0; i < connectedClients.size(); i++) {
			if (connectedClients[i]->pingCounter.getElapsedTime().asSeconds() > 40) {
				idsToDisconnect.push(connectedClients[i]->matchId);
			}
		}

		while (idsToDisconnect.size() > 0) {
			int id = idsToDisconnect.front();
			int index = -1;
			for (int i = 0; i < connectedClients.size(); i++) {
				if (connectedClients[i]->matchId == id) {
					index = i;
				}
			}

			if (index > -1) {
				dbm->EndSession(connectedClients[index]->sessionID, connectedClients[index]->games);
				std::cout << "DISCONNECTING PLAYER" << std::endl;
				delete connectedClients[index];
				connectedClients.erase(connectedClients.begin() + index);
			}
			std::cout << "Desconectando un jugador\n";
			idsToDisconnect.pop();
		}
		int matchToDelete = -1;
		for (int i = 0; i < aMatches.size(); i++) {
			if (matchToDelete == -1) {
				if (aMatches[i]->end) {

					std::cout << "Tamaño de receptionThreads "<<receptionThreads.size()<<std::endl;

					//receptionThreads[i].join();
					//matchThreads[i].join();
					//pingThreads[i].join();

					matchToDelete = i;



				}
			}
		}

		//std::cout << "Tamaño de receptionThreads " << receptionThreads.size() << std::endl;

		if (matchToDelete > -1) {
			std::cout << "Tamaño de receptionThreads " << receptionThreads.size() << std::endl;

			if (receptionThreads[matchToDelete].joinable()) {
				receptionThreads[matchToDelete].join();
			}			
			if (matchThreads[matchToDelete].joinable()) {
				matchThreads[matchToDelete].join();
			}
			if (pingThreads[matchToDelete].joinable()) {
				pingThreads[matchToDelete].join();
			}

			std::cout << "MATCHTHREADSIZE " << matchThreads.size() << std::endl;


			receptionThreads.erase(receptionThreads.begin() + matchToDelete);
			matchThreads.erase(matchThreads.begin() + matchToDelete);
			pingThreads.erase(pingThreads.begin() + matchToDelete);
			
			Match* aMatchToDelete = aMatches[matchToDelete];
			if (aMatchToDelete != nullptr) {
				delete aMatchToDelete;
			}

			aMatches.erase(aMatches.begin()+matchToDelete);
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
	//delete dbm;

	return 0;
}

Match*GetMatchWithId(int id, std::vector<Match*>*matches) {
	for (int i = 0; i < matches->size(); i++) {
		if (matches->at(i)->matchId == id) {
			return matches->at(i);
		}
	}
	return nullptr;
}

unsigned short GetFreePort(std::map<unsigned short, bool>map) {
	for (std::map<unsigned short, bool>::iterator it = map.begin(); it != map.end(); ++it) {
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
			//std::cout << "Recibido comando" << std::endl;
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
			std::cout<<"Recibido un comando desde Match\n";
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

void MatchThread(Match* match) {

	while (!match->end) {

		if (match->aClients.size() == numPlayers) {
			if (match->gameClock.getElapsedTime().asSeconds() > 5) {
				if (!match->gameHadStarted) {


					for (int i = 0; i < match->aClients.size(); i++) {
						OutputMemoryBitStream ombs;
						ombs.Write(PacketType::GAMESTART, commandBits);
						ombs.Write(match->aClients[i]->criticalId, criticalBits);
						match->aClients[i]->AddCriticalMessage(new CriticalMessage(match->aClients[i]->criticalId, ombs.GetBufferPtr(), ombs.GetByteLength()));
					}
					match->gameHadStarted = true;
				}
			}
		}
		else if (!match->gameHadStarted) {
			match->gameClock.restart();
		}

		if (match->gameHadStarted) {
			if (match->ballClock.getElapsedTime().asMilliseconds() > 100) {
				match->UpdateBall(&match->ballCoords, match->ballSpeed, match->ballClock.getElapsedTime().asSeconds(), &match->leftScore, &match->rightScore, &match->aClients, match->socket, &match->gameOver);
				match->SendBallPos(&match->aClients, match->socket, match->ballCoords);
				match->ballClock.restart();
			}
		}

		if (match->gameOver) {
			if (match->gameOverClock.getElapsedTime().asSeconds() > 5) {
				match->end = true;
			}
		}
		else {
			match->gameOverClock.restart();
		}

		if (!match->incomingInfo.empty()) {
			Event infoReceived;
			sf::IpAddress remoteIP;
			unsigned short remotePort;
			int command = HELLO;
			infoReceived = *match->incomingInfo.front();
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
			//case HELLO:
			//	//coords.first = rand() % 740;
			//	//coords.second = rand() % 540;

			//	for (int i = 0; i < match->aClients.size(); i++) {
			//		if (match->aClients[i]->GetIP() == remoteIP&&match->aClients[i]->GetPort() == remotePort) {
			//			exists = true;
			//			repeatingId = match->aClients[i]->GetMatchID();
			//			coords = match->initialPositions[match->aClients[i]->GetMatchID()];

			//		}
			//	}

			//	if (match->aClients.size() < 4) {
			//		ombs.Write(PacketType::WELCOME, commandBits);

			//		if (!exists) {
			//			match->clientID = match->GetAvailableId(match->aClients, numPlayers);
			//			match->aClients.push_back(new ServerClient(remoteIP.toString(), remotePort, match->clientID, match->initialPositions[match->clientID]));
			//			coords = match->initialPositions[match->clientID];
			//			ombs.Write(match->clientID, playerSizeBits);
			//			repeatingId = match->clientID;
			//			match->clientID++;
			//		}
			//		else {
			//			ombs.Write(repeatingId, playerSizeBits);
			//		}

			//		int aClientSizeMinus = match->aClients.size();
			//		aClientSizeMinus--;
			//		ombs.Write(aClientSizeMinus, playerSizeBits);

			//		for (int i = 0; i <match->aClients.size(); i++) {
			//			ombs.Write(match->aClients[i]->GetMatchID(), playerSizeBits);
			//			ombs.Write(match->aClients[i]->GetPosition().first, coordsbits);
			//			ombs.Write(match->aClients[i]->GetPosition().second, coordsbits);
			//		}

			//		status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

			//		if (status == sf::Socket::Error) {
			//			std::cout << "ERROR nen\n";
			//		}

			//		if (!exists) {

			//			std::cout << coords.second << "--AAAAAAAAAAAAAAA\n";
			//			for (int i = 0; i < match->aClients.size(); i++) {
			//				if (match->aClients[i]->matchId != repeatingId) {
			//					OutputMemoryBitStream* auxOmbs = new OutputMemoryBitStream();

			//					auxOmbs->Write(PacketType::NEWPLAYER, commandBits);
			//					auxOmbs->Write(match->aClients[i]->criticalId, criticalBits);
			//					auxOmbs->Write(repeatingId, playerSizeBits);
			//					auxOmbs->Write(coords.first, coordsbits);
			//					auxOmbs->Write(coords.second, coordsbits);

			//					//socket->send(auxOmbs->GetBufferPtr(), auxOmbs->GetByteLength(), aClients[i]->GetIP(), aClients[i]->GetPort());
			//					match->aClients[i]->AddCriticalMessage(new CriticalMessage(match->aClients[i]->criticalId, auxOmbs->GetBufferPtr(), auxOmbs->GetByteLength()));
			//				}
			//			}
			//		}
			//	}
			//	else {
			//		if (exists) {
			//			ombs.Write(PacketType::WELCOME, commandBits);
			//			ombs.Write(repeatingId, playerSizeBits);

			//			for (int i = 0; i < 1; i++) {
			//				int aClientSizeMinus = match->aClients.size();
			//				aClientSizeMinus--;
			//				ombs.Write(aClientSizeMinus, playerSizeBits);
			//			}
			//			for (int i = 0; i < match->aClients.size(); i++) {
			//				ombs.Write(match->aClients[i]->GetMatchID(), playerSizeBits);
			//				ombs.Write(match->aClients[i]->GetPosition().first, coordsbits);
			//				ombs.Write(match->aClients[i]->GetPosition().second, coordsbits);
			//			}

			//			status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

			//			if (status == sf::Socket::Error) {
			//				std::cout << "ERROR nen\n";
			//			}
			//		}
			//		else {
			//			ombs.Write(PacketType::NOTWELCOME, commandBits);
			//			status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);

			//			if (status == sf::Socket::Error) {
			//				std::cout << "ERROR nen\n";
			//			}
			//		}
			//	}


			//	break;
			case ACK:
				aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
				if (aClient != nullptr) {

					imbs.Read(&aCriticalId, criticalBits);
					aClient->RemoveCriticalPacket(aCriticalId);
					//imbs.Read(&aCriticalId, criticalBits);
					//aClient->RemoveCriticalPacket(aCriticalId);
					std::cout << "Recibido ACK " << aCriticalId << "Del jugador con ID " << aClient->GetMatchID() << std::endl;

				}
				break;
			case ACKPING:
				aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
				if (aClient != nullptr) {
					aClient->pingCounter.restart();
					//std::cout << "Recibido ACKPING de jugador " << aClient->GetMatchID() << std::endl;
				}
				break;
			case SHOOT:
				if (match->gameHadStarted) {
					std::cout << "SHOOT RECIBIDO\n";
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);

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

						match->auxBallSpeed.first = auxCoords.first - coords.first;
						match->auxBallSpeed.second = auxCoords.second - coords.second;


						if (aClient->shootClock.getElapsedTime().asMilliseconds() > shootCoolDown) {
							float magnitude = match->auxBallSpeed.first * match->auxBallSpeed.first + match->auxBallSpeed.second * match->auxBallSpeed.second;

							//std::cout << "Magnitude = " << magnitude << std::endl;
							std::cout << "resta = " << match->ballSpeed->first << ", " << match->ballSpeed->second << std::endl;

							magnitude = std::sqrt(magnitude);
							std::cout << "Magnitude = " << magnitude << std::endl;

							if (magnitude < ballRadius + playerRadius + 5) {

								aClient->shootClock.restart();

								std::cout << "Raiz Magnitude = " << magnitude << std::endl;
								*match->ballSpeed = match->auxBallSpeed;

								match->ballSpeed->first /= magnitude;
								match->ballSpeed->second /= magnitude;

								std::cout << "BallSpeed dividida magnitude " << match->ballSpeed->first << ", " << match->ballSpeed->second << std::endl;

								match->ballSpeed->first *= shootStrength;
								match->ballSpeed->second *= shootStrength;

								std::cout << "BallSpeed dividida magnitude por 10 " << match->ballSpeed->first << ", " << match->ballSpeed->second << std::endl;


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
				if (match->aClients.size() > 0) {
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
					if (aClient != nullptr) {
						AccumMoveServer accumMove;
						accumMove.playerID = aClient->matchId;
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

			match->incomingInfo.pop();
		}
		if (match->criticalClock.getElapsedTime().asMilliseconds() > 200) {
			for (int i = 0; i < match->aClients.size(); i++) {
				if (match->aClients[i]->HasCriticalPackets()) {
					match->aClients[i]->SendAllCriticalPackets(match->socket);
				}
			}
			match->criticalClock.restart();
		}


		for (int i = 0; i < match->aClients.size(); i++) {
			if (match->aClients[i]->pingCounter.getElapsedTime().asMilliseconds() > 20000) {
				std::cout << "Player with id " << match->aClients[i]->GetMatchID() << " timeOut " << match->aClients[i]->pingCounter.getElapsedTime().asMilliseconds() << std::endl;;
				match->DisconnectPlayer(&match->aClients, match->aClients[i]);
			}
			else if (match->aClients[i]->moveClock.getElapsedTime().asMilliseconds() > 100) {

				if (match->aClients[i]->acumulatedMoves.size() > 0) {

					OutputMemoryBitStream ombs;
					int latestMessageIndex = 0;
					for (int j = 0; j < match->aClients[i]->acumulatedMoves.size(); j++) {
						if (match->aClients[i]->acumulatedMoves[j].idMove > match->aClients[i]->acumulatedMoves[latestMessageIndex].idMove) {
							latestMessageIndex = j;
						}
					}

					if ((match->aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first + playerRadius) < 1000 && (match->aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first)>0 && (match->aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second + playerRadius)<600 && (match->aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second)>0) {
						ombs.Write(PacketType::ACKMOVE, commandBits);
						ombs.Write(match->aClients[i]->matchId, playerSizeBits);
						ombs.Write(match->aClients[i]->acumulatedMoves[latestMessageIndex].idMove, criticalBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.first,deltaMoveBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.second, deltaMoveBits);


						ombs.Write(match->aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first, coordsbits);
						ombs.Write(match->aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second, coordsbits);
						match->aClients[i]->position = match->aClients[i]->acumulatedMoves[latestMessageIndex].absolute;
						match->aClients[i]->acumulatedMoves.clear();

						//std::cout << "Enviada posicion de jugador con ID " << aClients[i]->GetID() << ".Sus coordenadas son " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first << ", " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second << "\n";


						for (int j = 0; j < match->aClients.size(); j++) {
							match->status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), match->aClients[j]->GetIP(), match->aClients[j]->GetPort());
							if (match->status == sf::Socket::Error) {
								std::cout << "ERROR ENVIANDO ACTUALIZACION DE POSICION\n";
							}
						}
					}
					else {
						ombs.Write(PacketType::ACKMOVE, commandBits);
						ombs.Write(match->aClients[i]->matchId, playerSizeBits);
						ombs.Write(match->aClients[i]->acumulatedMoves[latestMessageIndex].idMove, criticalBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.first,deltaMoveBits);
						//ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].delta.second, deltaMoveBits);


						ombs.Write(match->aClients[i]->position.first, coordsbits);
						ombs.Write(match->aClients[i]->position.second, coordsbits);
						//aClients[i]->position = aClients[i]->acumulatedMoves[latestMessageIndex].absolute;
						match->aClients[i]->acumulatedMoves.clear();

						//std::cout << "Enviada posicion de jugador con ID " << aClients[i]->GetID() << ".Sus coordenadas son " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first << ", " << aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second << "\n";


						for (int j = 0; j < match->aClients.size(); j++) {
							match->status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), match->aClients[j]->GetIP(), match->aClients[j]->GetPort());
							if (match->status == sf::Socket::Error) {
								std::cout << "ERROR ENVIANDO ACTUALIZACION DE POSICION\n";
							}
						}
					}


				}
				match->aClients[i]->moveClock.restart();
			}
		}
	}

	match->socket->unbind();
	match->end = true;
	delete match->socket;
	if (match->ballSpeed != nullptr) {
		delete match->ballSpeed;
	}
}

static void AddPlayer(ServerClient* client, Match* match) {
	//match->aClients.push_back(client);
	//OutputMemoryBitStream ombs;
	bool exists = false;
	int repeatingId = -1;
	std::pair<short, short>coords;

	for (int i = 0; i < match->aClients.size(); i++) {
		if (match->aClients[i]->GetIP() == client->GetIP() && match->aClients[i]->GetPort() == client->GetPort()) {
			exists = true;
			repeatingId = match->aClients[i]->GetMatchID();
			coords = match->initialPositions[match->aClients[i]->GetMatchID()];

		}
	}

	if (match->aClients.size() < 4) {
		//ombs.Write(PacketType::WELCOME, commandBits);

		if (!exists) {
			match->matchId = match->GetAvailableId(match->aClients, numPlayers);
			match->aClients.push_back(client);
			coords = match->initialPositions[match->matchId];
			//ombs.Write(match->clientID, playerSizeBits);
			repeatingId = match->matchId;
			match->matchId++;
		}
		else {
			//ombs.Write(repeatingId, playerSizeBits);
		}

		int aClientSizeMinus = match->aClients.size();
		aClientSizeMinus--;
		//ombs.Write(aClientSizeMinus, playerSizeBits);

		for (int i = 0; i < match->aClients.size(); i++) {
			//ombs.Write(match->aClients[i]->GetMatchID(), playerSizeBits);
			//ombs.Write(match->aClients[i]->GetPosition().first, coordsbits);
			//ombs.Write(match->aClients[i]->GetPosition().second, coordsbits);
		}

		//match->status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), client->GetIP(), client->GetPort());

		if (match->status == sf::Socket::Error) {
			std::cout << "ERROR nen\n";
		}

		if (!exists) {

			std::cout << coords.second << "--AAAAAAAAAAAAAAA\n";
			for (int i = 0; i < match->aClients.size(); i++) {
				if (match->aClients[i]->matchId != repeatingId) {
					//OutputMemoryBitStream* auxOmbs = new OutputMemoryBitStream();

					//auxOmbs->Write(PacketType::NEWPLAYER, commandBits);
					//auxOmbs->Write(match->aClients[i]->criticalId, criticalBits);
					//auxOmbs->Write(repeatingId, playerSizeBits);
					//auxOmbs->Write(coords.first, coordsbits);
					//auxOmbs->Write(coords.second, coordsbits);

					//socket->send(auxOmbs->GetBufferPtr(), auxOmbs->GetByteLength(), aClients[i]->GetIP(), aClients[i]->GetPort());
					//match->aClients[i]->AddCriticalMessage(new CriticalMessage(match->aClients[i]->criticalId, auxOmbs->GetBufferPtr(), auxOmbs->GetByteLength()));
				}
			}
		}
	}
	else {
		if (exists) {
			//ombs.Write(PacketType::WELCOME, commandBits);
			//ombs.Write(repeatingId, playerSizeBits);

			for (int i = 0; i < 1; i++) {
				int aClientSizeMinus = match->aClients.size();
				aClientSizeMinus--;
				//ombs.Write(aClientSizeMinus, playerSizeBits);
			}
			for (int i = 0; i < match->aClients.size(); i++) {
				//ombs.Write(match->aClients[i]->GetMatchID(), playerSizeBits);
				//ombs.Write(match->aClients[i]->GetPosition().first, coordsbits);
				//ombs.Write(match->aClients[i]->GetPosition().second, coordsbits);
			}

			//match->status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), client->GetIP(), client->GetPort());

			//if (match->status == sf::Socket::Error) {
				//std::cout << "ERROR nen\n";
			//}
		}
		else {
			//ombs.Write(PacketType::NOTWELCOME, commandBits);
			//match->status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), client->GetIP(), client->GetPort());

			//if (match->status == sf::Socket::Error) {
				//std::cout << "ERROR nen\n";
			//}
		}
	}


}

void MatchRoomThread(Match* match) {
	while (!match->gameHadStarted) {
		if (!match->incomingInfo.empty()) {
			Event infoReceived;
			sf::IpAddress remoteIP;
			unsigned short remotePort;
			int command = HELLO;
			infoReceived = *match->incomingInfo.front();
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
				case ACK:
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
					if (aClient != nullptr) {

						imbs.Read(&aCriticalId, criticalBits);
						aClient->RemoveCriticalPacket(aCriticalId);
						//imbs.Read(&aCriticalId, criticalBits);
						//aClient->RemoveCriticalPacket(aCriticalId);

						//std::cout << "Recibido ACK " << aCriticalId << "Del jugador con ID " << aClient->GetMatchID() << std::endl;
					}
					break;
				case ACKPING:
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
					if (aClient != nullptr) {
						aClient->pingCounter.restart();
						//std::cout << "Recibido ACKPING de jugador " << aClient->GetMatchID() << std::endl;
					}
					break;
				case PacketType::READY: {
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
					if (aClient != nullptr) {
						aClient->isReady = true;
						std::cout << "Recibido READY\n";
					}

					break;
				}case PacketType::UPDATEROOM: {
					OutputMemoryBitStream ombs;
					sf::Socket::Status status;
					ombs.Write(PacketType::UPDATEROOM, commandBits);
					int clientSizeInt = match->aClients.size();
					ombs.Write(clientSizeInt, playerSizeBits);

					for (int j = 0; j<match->aClients.size(); j++) {
						ombs.WriteString(match->aClients[j]->userName);
					}

					status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), remoteIP, remotePort);
					if (status == sf::Socket::Error) {
						std::cout << "Error enviando UPDATEROOM\n";
					}
					break;
				}
				case PacketType::EXITROOM: {
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
					if (aClient != nullptr) {

						OutputMemoryBitStream auxOmbs;
						auxOmbs.Write(PacketType::EXITROOM, commandBits);
						//auxOmbs.Write(aClient->criticalId, criticalBits);

						//aClient->AddCriticalMessage(new CriticalMessage(aClient->criticalId, auxOmbs.GetBufferPtr(), auxOmbs.GetByteLength()));
						status = match->socket->send(auxOmbs.GetBufferPtr(), auxOmbs.GetByteLength(), remoteIP, remotePort);

						if (status == sf::Socket::Error) {
							std::cout << "Error enviando EXITROOM\n";
						}

						int index = GetIndexServerClientWithId(aClient->matchId, &match->aClients);

						std::cout << "El cliente con id " << aClient->matchId << " tiene un indice " << index << std::endl;

						if (index > -1) {
							match->aClients.erase(match->aClients.begin() + index);
							std::cout << "Despues de borrar un cliente el tamaño es " << match->aClients.size() << std::endl;
						}

						for (int i = 0; i<match->aClients.size(); i++) {
							OutputMemoryBitStream ombs;
							sf::Socket::Status status;
							ombs.Write(PacketType::UPDATEROOM, commandBits);
							int clientSizeInt = match->aClients.size();
							ombs.Write(clientSizeInt, playerSizeBits);

							for (int j = 0; j<match->aClients.size(); j++) {
								ombs.WriteString(match->aClients[j]->userName);
							}

							status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), match->aClients[i]->GetIP(), match->aClients[i]->GetPort());

							if (status == sf::Socket::Error) {
								std::cout << "Error enviando UPDATEROOM\n";
							}
						}
					}
					else {
						OutputMemoryBitStream auxOmbs;
						auxOmbs.Write(PacketType::EXITROOM, commandBits);
						status = match->socket->send(auxOmbs.GetBufferPtr(), auxOmbs.GetByteLength(), remoteIP, remotePort);

						if (status == sf::Socket::Error) {
							std::cout << "Error enviando EXITROOM\n";
						}
					}
					break;
				}
				case PacketType::MSG: {
					aClient = GetServerClientWithIpPort(remotePort, remoteIP.toString(), &match->aClients);
					if (aClient != nullptr) {
						std::string message = "";
						std::string finalMessage = "";
						finalMessage = aClient->userName + ": ";
						imbs.ReadString(&message);
						finalMessage += message;
						ombs.Write(PacketType::MSG, commandBits);
						ombs.WriteString(finalMessage);
						for (int i = 0; i<match->aClients.size(); i++) {
							sf::Socket::Status status;
							status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), match->aClients[i]->GetIP(), match->aClients[i]->GetPort());

							if (status == sf::Socket::Error) {
								std::cout << "Error enviando MSG\n";
							}
						}

					}

					break; 
				}
				default:
					std::cout << "Match con puerto " << match->matchPort << " Recibe command " << command << std::endl;
					break;
			}

			match->incomingInfo.pop();
		}

		if (match->prevPlayerSize!=match->aClients.size()) {
			match->prevPlayerSize = match->aClients.size();
			for (int i = 0; i<match->aClients.size(); i++) {
				OutputMemoryBitStream ombs;
				sf::Socket::Status status;
				ombs.Write(PacketType::UPDATEROOM, commandBits);
				int clientSizeInt = match->aClients.size();
				ombs.Write(clientSizeInt, playerSizeBits);

				for (int j = 0; j<match->aClients.size(); j++) {

					std::cout << j << std:: endl;


					ombs.WriteString(match->aClients[j]->userName);
				}
				
				status = match->socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), match->aClients[i]->GetIP(), match->aClients[i]->GetPort());

				if (status == sf::Socket::Error) {
					std::cout << "Error enviando UPDATEROOM\n";
				}
				else {
					std::cout << "ENVIADO UPDATEROOM\n";
				}
			}
		}

		bool allReady=true;
		if (match->aClients.size() ==2 ) {
			for (int i = 0; i < match->aClients.size(); i++) {
				if (!match->aClients[i]->isReady) {
					allReady = false;
					//std::cout << "playerNotReady\n";
				}
				else {
					//std::cout << "playerReady\n";
				}
			}
		}
		else {
			allReady = false;
		}

		if (allReady) {
			std::cout << "allReady\n";
			match->gameHadStarted = true;
		}

		if (match->criticalClock.getElapsedTime().asMilliseconds() > 200) {
			for (int i = 0; i < match->aClients.size(); i++) {
				if (match->aClients[i]->HasCriticalPackets()) {
					match->aClients[i]->SendAllCriticalPackets(match->socket);
				}
			}
			match->criticalClock.restart();
		}

	}
}