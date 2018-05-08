#pragma once
#include "Client.h"
#include "OutputMemoryBitStream.h"
#include "InputMemoryBitStream.h"

enum PacketType {HELLO, WELCOME, NEWPLAYER, DISCONNECT, ACK, PING, ACKPING, ACKMOVE, SHOOT, GAMEOVER, GAMESTART, GOAL, NOTWELCOME, MOVE, MOVEBALL};
const int commandBits = 4;
const int maxBufferSize = 1300;
const int playerSizeBits = 3;
const int coordsbits = 10;
const int criticalBits=8;
const int deltaMoveBits = 8;
const int scoreBits = 4;
const int boolBit = 1;
const int subdividedSteps = 9;
const int playerRadius = 20;
const int ballRadius = 13;
const int shootStrength = 200;
const int numPlayers = 4;
const std::pair<short, short> leftGoalTop;
const std::pair<short, short> leftGoalBot;
const std::pair<short, short> rightGoalTop;
const std::pair<short, short> rightGoalBot;
const int windowWidth = 1000;
const int windowHeight = 800;
const int shootCoolDown = 500;
const int victoryScore = 10;
const std::pair<short, short> ballStartPos{windowWidth/2,300};

static void RemoveNonAckMovesUntilId(std::vector<AccumMove>*aMoves, int id) {
	int index = -1;
	for (int i = 0; i < aMoves->size(); i++) {
		if (aMoves->at(i).idMove==id) {
			index = i;
		}
	}

	if (index > -1) {
		aMoves->erase(aMoves->begin(), aMoves->begin() + index);
	}
}

static ServerClient* GetServerClientWithIpPort(unsigned short port, std::string ip, std::vector<ServerClient*>*aClients) {
	if (aClients->size() > 0) {
		for (int i = 0; i < aClients->size(); i++) {
			if (aClients->at(i)->GetIP() == ip&&aClients->at(i)->GetPort() == port) {
				return aClients->at(i);
			}
		}
	}
	else {
		//NO HAY JUGADORES
	}
	return nullptr;
}

static Client* GetClientWithId(int id, std::vector<Client*>aClients) {
	for (int i = 0; i < aClients.size(); i++) {
		if (aClients[i]->id == id) {
			return aClients[i];
		}
	}
	return nullptr;
}

//ServerClient* GetClientWithId(int id, std::vector<ServerClient*>*aClients) {
//
//	for (int i = 0; i < aClients->size(); i++) {
//		//if (aClients->at(i)->GetID() == id) {
//		//	return aClients->at(i);
//		//}
//	}
//}

//Client* GetClientWithId(int id, std::vector<Client*>*aClients) {
//
//	for (int i = 0; i < aClients->size(); i++) {
//		if (aClients->at(i)->id == id) {
//			return aClients->at(i);
//		}
//	}
//}

static int GetIndexServerClientWithId(int id, std::vector<ServerClient*>*aClients) {

	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->id == id) {
			return i;
		}
	}
	return -1;
}

static int GetIndexClientWithId(int id, std::vector<Client*>*aClients) {

	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->id == id) {
			return i;
		}
	}
	return -1;
}