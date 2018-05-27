#pragma once
#include <queue>
#include <thread>
#include "Event.h""
#include <random>
#include <time.h>
#include "Utils.h"


class MatchRoom {
public:
	std::vector<ServerClient*>aClients;
};

class Match {

public:
	int playerLimit;
	int matchId;
	std::string gameName;
	sf::Clock criticalClock;
	int clientID = 0;
	std::vector<ServerClient*> aClients;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	unsigned short matchPort;
	bool end = false;
	bool sentEnd = false;
	bool gameHadStarted = false;
	bool startFlag = false;
	int prevPlayerSize=0;
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
	int winScore;
	//void Run();
	//void Run(Match* match);

	void SetUp(unsigned short);
	void ResetCounters();
	void DisconnectPlayer(std::vector<ServerClient*>* aClients, ServerClient* aClient);
	void SendBallPos(std::vector<ServerClient*>*aClients, sf::UdpSocket* socket, std::pair<short, short> ballPos);
	void UpdateBall(std::pair<float, float>* coords, std::pair<float, float>*speed, float delta, int*, int*, std::vector<ServerClient*>*, sf::UdpSocket*, bool*);
	int GetAvailableId(std::vector<ServerClient*>aClients, int num);
	bool CheckGameOver(int leftScore, int rightScore, std::vector<ServerClient*>*aClients, sf::UdpSocket* socket,int );
	bool Match::ContainsClient(ServerClient* aClient);
};