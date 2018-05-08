#include <Client.h>
#include <queue>
#include <thread>
#include <Event.h>
#include <random>
#include <time.h>
#include "Utils.h"
#include "Match.h"
int main() {
	Match match(50000);
	match.Run();


	return 0;
}