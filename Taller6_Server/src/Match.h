#pragma once
#include "Client.h"
#include <queue>
#include <thread>
#include <Event.h>
#include <random>
#include "Utils.h"

class Match {
protected:
	short gamePort;
	std::queue<Event*> incomingInfo; //cola de paquetes entrantes
	bool end;
	sf::UdpSocket* socket;
	std::vector<ServerClient*> aClients;

	void ReceptionThread(bool* end, std::queue<Event*>* incomingInfo, sf::UdpSocket* socket);
	void PingThread(bool* end, std::vector<ServerClient*>* aClients, sf::UdpSocket* socket);
	void DisconnectPlayer(std::vector<ServerClient*>* aClients, ServerClient* aClient);
	void SendBallPos(std::vector<ServerClient*>*aClients, sf::UdpSocket* socket, std::pair<short, short> ballPos);
	void UpdateBall(std::pair<float, float>* coords, std::pair<float, float>*speed, float delta, int*, int*, std::vector<ServerClient*>*, sf::UdpSocket*, bool*);
	int GetAvailableId(std::vector<ServerClient*>aClients, int num);
	bool CheckGameOver(int leftScore, int rightScore, std::vector<ServerClient*>*aClients, sf::UdpSocket* socket);

public:
	Match(short gamePort);
	void Run();	
};