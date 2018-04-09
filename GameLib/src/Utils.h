#pragma once
#include "Client.h"
#include "OutputMemoryBitStream.h"
#include "InputMemoryBitStream.h"

enum PacketType {HELLO, WELCOME, NEWPLAYER, DISCONNECT, ACK, PING, ACKPING, INPUT, SHOOT, GAMEOVER, GAMESTART, GOAL, NOTWELCOME, MOVE, MOVEBALL};
const int8_t commandBits = 4;
const int maxBufferSize = 300;
const int playerSizeBits = 2;
const int coordsbits = 100;
const int criticalBits=8;

ServerClient* GetClientWithId(int id, std::vector<ServerClient*>*aClients) {

	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->GetID() == id) {
			return aClients->at(i);
		}
	}
}

Client* GetClientWithId(int id, std::vector<Client*>*aClients) {

	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->id == id) {
			return aClients->at(i);
		}
	}
}

int GetIndexClientWithId(int id, std::vector<Client*>*aClients) {

	for (int i = 0; i < aClients->size(); i++) {
		if (aClients->at(i)->id == id) {
			return i;
		}
	}
	return -1;
}