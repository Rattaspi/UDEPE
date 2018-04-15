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

void RemoveNonAckMovesUntilId(std::vector<AccumMove>*aMoves, int id) {
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

ServerClient* GetClientWithIpPort(unsigned short port, std::string ip, std::vector<ServerClient*>*aClients) {
	int playerSize = 0;
	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->GetIP() == ip&&aClients->at(i)->GetPort() == port) {
			return aClients->at(i);
			playerSize++;
		}
	}
	std::cout << "JUGADORES PROCESADOS-> " << playerSize << std::endl;
	return nullptr;
}

Client* GetClientWithId(int id, std::vector<Client*>aClients) {
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

int GetIndexClientWithId(int id, std::vector<ServerClient*>*aClients) {

	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->id == id) {
			return i;
		}
	}
	return -1;
}

int GetIndexClientWithId(int id, std::vector<Client*>*aClients) {

	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->id == id) {
			return i;
		}
	}
	return -1;
}