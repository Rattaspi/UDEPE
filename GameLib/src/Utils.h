#pragma once
#include "Client.h"

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