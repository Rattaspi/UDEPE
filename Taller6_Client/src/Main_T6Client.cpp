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

bool CheckValidMoveId(std::vector<AccumMove>*nonAckMoves, int aCriticalId, std::pair<short,short>someCoords,int* index);

void EraseAccums(std::vector<AccumMove>*nonAckMoves, int until);

void SetUpMapLines(std::vector<sf::RectangleShape>*);

void DrawMap(std::vector<sf::RectangleShape>*, sf::RenderWindow*);

void InterpolateBallMovement(std::queue<std::pair<short, short>>*, std::pair<short, short>,std::pair<short,short>);

void DrawScores(std::string localScoreLeft, std::string localScoreRight, std::string* serverMessage, sf::RenderWindow* window, sf::Clock*serverMessageClock);

void LogIn(sf::UdpSocket* socket, std::string user, std::string password, sf::IpAddress, short port);

void Register(sf::UdpSocket* socket, std::string user, std::string password, std::string email);

sf::Font font;
sf::Texture bgTex;
sf::Sprite bgSprite;

int main() {
	bgTex.loadFromFile("bg.jpg");
	bgSprite.setTexture(bgTex);
	font.loadFromFile("arial_narrow_7.ttf");


	std::cout << "CLIENTE INICIADO" << std::endl;
	std::string serverIp = "localhost";
	int serverPort = 50000;
	int matchPort=0;
	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	std::string command;
	int myId = 0;
	std::vector<int>acks;
	std::pair<short, short> myCoordenates{ 0,0 };
	std::pair<short, short> originalCoordenates{ 0,0 };

	int localScoreLeftNum = 0;
	int localScoreRightNum = 0;
	std::string localScoreLeft = std::to_string(localScoreLeftNum);
	std::string localScoreRight = std::to_string(localScoreRightNum);
	std::string serverMessage = "";
	sf::Clock serverMessageClock;

	std::pair<short, short> auxPosition{ 0,0 };
	std::pair<short, short> currentDelta{ 0,0 };
	std::pair<short, short>localBallCoords{ windowWidth/2,windowHeight/2 };
	std::vector<AccumMove> nonAckMoves;
	int currentMoveId=0;
	bool gameStart=false;
	bool end = false; //finalizar programa
	bool serverOnline = false;
	bool connected = false; //controla cuando se ha conectado con el servidor
	bool gameOver = false;
	bool winner = false;
	std::queue<Event> incomingInfo; //cola de paquetes entrantes
	std::vector<Client*> aClients;
	sf::Clock clockForTheServer, clockForTheStep, clockForMyMovement, clockGameOver;
	sf::Clock logInClock;
	int timeBetweenSteps = 2; //tiempo que tardan en actualizarse las posiciones interpoladas de los otros clientes.
	int playerSpeed = 1;
	bool down_key = false, up_key = false, right_key = false, left_key = false;
	std::vector<sf::RectangleShape> mapLines; //guarda todas las lineas que forman el mapa.
	std::queue<std::pair<short, short>> ballSteps;
	
	SetUpMapLines(&mapLines);

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);

	sf::Clock shootCounter;
	sf::Clock clock, clockForTheBallMovement;

	sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Cliente con ID "+myId);
	std::vector<sf::CircleShape> playerRenders;
	enum ProgramState { SERVERCHECK, LOGIN, MAIN_MENU,JOIN_MATCH, MATCH_CREATION, MATCH_ROOM,MATCH };
	ProgramState programState = MAIN_MENU;

	std::string myUsername="";
	std::string myPassword="";
	std::string myRegisteredUsername = "";
	std::string myRegisteredPassword = "";
	std::string myConfirmedPassword = "";


	int selectedOption=0;
	bool logInAnswer = true;
	while (!end){
			switch (programState) {
			case SERVERCHECK: {
				if (serverMessageClock.getElapsedTime().asSeconds() > 0.5f) {
					OutputMemoryBitStream ombs;
					ombs.Write(PacketType::PING,commandBits);
					serverMessageClock.restart();
					status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(),serverIp,serverPort);

					if (status == sf::Socket::Status::Error) {
						std::cout << "Error enviando Ping\n";
					}
					else {
						std::cout << "Enviando Ping\n";
					}

				}

				if (!incomingInfo.empty()) {
					Event infoReceived;
					PacketType command = HELLO;
					infoReceived = incomingInfo.front();
					InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
					imbs.Read(&command, commandBits);
					switch (command) {
					case PacketType::ACKPING:
						std::cout << "ACKPING Recibido\n";

						serverOnline = true;
						break;
					default:
						break;
					}
					incomingInfo.pop();
				}

				window.clear();

				sf::Text connectText;

				connectText.setPosition((windowWidth / 2) -300 , (windowHeight / 2));
				connectText.setFont(font);
				connectText.setCharacterSize(60);
				connectText.setFillColor(sf::Color::White);
				connectText.setString("Trying to connect to server...");

				window.draw(connectText);
				window.display();



				if (serverOnline) {
					programState = LOGIN;
					window.clear();
					serverMessageClock.restart();
				}

				break;
			}

			case LOGIN: {

				if (window.isOpen()) {


					if (!incomingInfo.empty()) {
						Event infoReceived;
						PacketType command = HELLO;
						infoReceived = incomingInfo.front();
						InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
						imbs.Read(&command, commandBits);
						switch (command) {
						case PacketType::LOGIN: {
							bool accepted=false;
							logInAnswer = true;
							imbs.Read(&accepted, boolBit);
							if (accepted) {
								programState = ProgramState::MAIN_MENU;
							}
							else {
								std::cout << "Nombre de usuario y/o contraseña erroneos\n";
							}
							break; 
						}
						case PacketType::REGISTER: {
							bool accepted = false;

							if (accepted) {
								programState = ProgramState::MAIN_MENU;
							}
							else {
								std::cout << "Error durante el registro\n";
							}
							break; 
						}
						default:
							break;
						}
						incomingInfo.pop();
					}

					if (!logInAnswer&&logInClock.getElapsedTime().asSeconds()>0.3f) {
						LogIn(socket, myUsername, myPassword, serverIp, serverPort);
						logInClock.restart();
					}


					sf::Event evento;

					while (window.pollEvent(evento)) {
						switch (evento.type) {
						case sf::Event::Closed:
							window.close();
							socket->unbind();
							end = true;
							break;
						case sf::Event::MouseButtonPressed:
							if (evento.mouseButton.button == sf::Mouse::Left) {
								int x = evento.mouseButton.x;
								int y = evento.mouseButton.y;

								if (x >= 50&&x<=350) {
									if (y >= 210 && y <= 255) {
										selectedOption = 0;
									}
									else if (y>=310&&y<=355) {
										selectedOption = 1;
									}
								}
								else if (x >= 460 && x <= 460 + 360) {
									if (y >= 210 && y <= 210+45) {
										selectedOption = 2;
									}
									else if (y >= 310 && y <= 310+45) {
										selectedOption = 3;
									}
									else if (y >= 390 && y <= 390+45) {
										selectedOption = 4;
									}
								}
								if (x >= 60 && x <= (60 + 300)) {
									if (y >= 600 && y <= (600 + 45)) {
										logInAnswer = false;
									}
								}
								if (x >= 480 && x <= (480 + 300)) {
									if (y >= 600 && y <= (600 + 45)) {
										Register(socket, myRegisteredUsername, myRegisteredPassword, myConfirmedPassword);
									}
								}

								//std::cout << x << "," <<y<<std::endl;


							}
							break;
						case sf::Event::KeyPressed:
							if (evento.key.code == sf::Keyboard::Escape) {
								socket->unbind();
								window.close();
								end = true;
							}
							else if (evento.key.code == sf::Keyboard::Return) {
								if (selectedOption == 0 || selectedOption == 1) {
									logInAnswer = false;
								}
								else {
									Register(socket, myRegisteredUsername, myRegisteredPassword, myConfirmedPassword);
								}
							}
							else if (evento.key.code == sf::Keyboard::BackSpace) {
								switch (selectedOption) {
								case 0:
									myUsername=myUsername.substr(0, myUsername.size() - 1);
									break;
								case 1:
									myPassword=myPassword.substr(0, myPassword.size() - 1);
									break;
								case 2:
									myRegisteredUsername=myRegisteredUsername.substr(0, myRegisteredUsername.size() - 1);
									break;
								case 3:
									myRegisteredPassword=myRegisteredPassword.substr(0, myRegisteredPassword.size() - 1);
									break;
								case 4:
									myConfirmedPassword=myConfirmedPassword.substr(0, myConfirmedPassword.size() - 1);
									break;
								default:
									break;
								}
							}
							else if (evento.key.code == sf::Keyboard::Tab) {
								selectedOption++;
								selectedOption = selectedOption%5;
							}
							break;											
						case sf::Event::TextEntered:
							if (evento.text.unicode >= 32 && evento.text.unicode <= 126 && myUsername.length() < 20) {
								switch (selectedOption) {
									case 0:
										myUsername += (char)evento.text.unicode;
										break;
									case 1:
										myPassword += (char)evento.text.unicode;
										break;
									case 2:
										myRegisteredUsername += (char)evento.text.unicode;
										break;
									case 3:
										myRegisteredPassword += (char)evento.text.unicode;
										break;
									case 4:
										myConfirmedPassword += (char)evento.text.unicode;
										break;
									default:
										break;
								}
							}								
							break;
						}
					}

					window.clear();

					sf::RectangleShape rectangles[9];
					rectangles[0].setPosition(20, 90);
					rectangles[0].setFillColor(sf::Color::Green);
					rectangles[0].setSize(sf::Vector2f(400, 600));

					rectangles[1].setPosition(440, 90);
					rectangles[1].setFillColor(sf::Color::Blue);
					rectangles[1].setSize(sf::Vector2f(400, 600));

					rectangles[2].setPosition(50, 210);
					rectangles[2].setFillColor(sf::Color::Red);
					rectangles[2].setSize(sf::Vector2f(300, 45));

					rectangles[3].setPosition(50, 310);
					rectangles[3].setFillColor(sf::Color::Red);
					rectangles[3].setSize(sf::Vector2f(300, 45));

					rectangles[4].setPosition(460, 210);
					rectangles[4].setFillColor(sf::Color::Red);
					rectangles[4].setSize(sf::Vector2f(360, 45));

					rectangles[5].setPosition(460, 310);
					rectangles[5].setFillColor(sf::Color::Red);
					rectangles[5].setSize(sf::Vector2f(360, 45));

					rectangles[6].setPosition(460, 390);
					rectangles[6].setFillColor(sf::Color::Red);
					rectangles[6].setSize(sf::Vector2f(360, 45));

					rectangles[7].setPosition(60, 600);
					rectangles[7].setFillColor(sf::Color::Red);
					rectangles[7].setSize(sf::Vector2f(300, 45));

					rectangles[8].setPosition(480, 600);
					rectangles[8].setFillColor(sf::Color::Red);
					rectangles[8].setSize(sf::Vector2f(300, 45));

					for (int i = 0; i < 9; i++) {
						window.draw(rectangles[i]);
					}

					sf::Text* loginText = new sf::Text("Login", font, 60);
					loginText->setPosition(60, 100);
					loginText->setFillColor(sf::Color::White);

					sf::Text* usernameText = new sf::Text("Username: " + myUsername, font, 20);
					usernameText->setPosition(60, 220);
					usernameText->setFillColor(sf::Color::White);

					std::string starsPassword = "";
					std::string starsRegisterPassword = "";

					for (int i = 0; i < myPassword.size(); i++) {
						starsPassword += "*";
					}

					for (int i = 0; i < myRegisteredPassword.size(); i++) {
						starsRegisterPassword += "*";
					}



					sf::Text* passwordText = new sf::Text("Password: " + starsPassword, font, 20);
					passwordText->setPosition(60, 320);
					passwordText->setFillColor(sf::Color::White);

					sf::Text* registerText = new sf::Text("Register", font, 60);
					registerText->setPosition(500, 100);
					registerText->setFillColor(sf::Color::White);

					sf::Text* registerUserNameText = new sf::Text("Username: " + myRegisteredUsername, font, 20);;
					registerUserNameText->setPosition(500, 220);
					registerUserNameText->setFillColor(sf::Color::White);

					sf::Text* registerPasswordText = new sf::Text("Password: " + starsRegisterPassword, font, 20);
					registerPasswordText->setPosition(500, 320);
					registerPasswordText->setFillColor(sf::Color::White);


					sf::Text* emailText = new sf::Text("Confirm Password: " + myConfirmedPassword, font, 20);
					emailText->setPosition(500, 400);
					emailText->setFillColor(sf::Color::White);

					sf::Text* LogInButton = new sf::Text("LOG IN", font, 40);
					LogInButton->setPosition(130, 600);
					LogInButton->setFillColor(sf::Color::White);

					sf::Text* RegisterButton = new sf::Text("REGISTER", font, 40);
					RegisterButton->setPosition(550, 600);
					RegisterButton->setFillColor(sf::Color::White);

					window.draw(*loginText);
					window.draw(*passwordText);
					window.draw(*usernameText);

					window.draw(*registerText);
					window.draw(*registerPasswordText);
					window.draw(*registerUserNameText);
					window.draw(*emailText);

					window.draw(*LogInButton);
					window.draw(*RegisterButton);


					window.display();

					delete loginText;
					delete passwordText;
					delete usernameText;
					delete registerUserNameText;
					delete registerText;
					delete registerPasswordText;
					delete emailText;
					delete LogInButton;
					delete RegisterButton;

				}
				break;

			}
			case MAIN_MENU: {
				window.clear();


				sf::Text JoinGameText("Join Game", font, 40);
				float windowWidthf = windowWidth;
				float windowHeightf = windowHeight;

				JoinGameText.setPosition(windowWidthf / 4-50, windowHeightf / 4);
				JoinGameText.setFillColor(sf::Color::White);

				sf::Text CreateGameText("Create Game", font, 40);
				CreateGameText.setPosition((windowWidthf / 4) * 3-50, windowHeightf / 4);
				CreateGameText.setFillColor(sf::Color::White);


				

				//sf::RectangleShape rect1(sf::Vector2f(windowWidthf / 4, windowHeightf / 4));
				sf::RectangleShape rect1(sf::Vector2f(400.0f, 100.0f));
				rect1.setSize(sf::Vector2f(300, 100.0f));
				rect1.setFillColor(sf::Color::Green);
				rect1.setPosition(sf::Vector2f((windowWidthf / 4) - 100, (windowHeightf / 4) - 25));


				//sf::RectangleShape rect2(sf::Vector2f((windowWidthf / 4)*3, (windowHeightf / 4)*3 - 50));
				sf::RectangleShape rect2(sf::Vector2f(400.0f, 100.0f));
				rect2.setSize(sf::Vector2f(300, 100.0f));
				rect2.setPosition(sf::Vector2f((windowWidthf / 4) * 3 - 100, (windowHeightf / 4) - 25));
				rect2.setFillColor(sf::Color::Green);


				sf::RectangleShape rect3(sf::Vector2f(400.0f, 100.0f));
				rect3.setSize(sf::Vector2f(400, 100.0f));
				rect3.setPosition(sf::Vector2f((windowWidthf / 2)-rect3.getSize().x/2, ((windowHeightf / 4)*3)));
				rect3.setFillColor(sf::Color::Red);

				sf::Text disconnectText("Disconnect", font, 40);
				disconnectText.setPosition(sf::Vector2f((windowWidthf / 2) - rect3.getSize().x / 2 + 100, ((windowHeightf / 4) * 3)+20));
				CreateGameText.setFillColor(sf::Color::White);


				sf::Event evento;

				while (window.pollEvent(evento)) {
					switch (evento.type) {
					case sf::Event::Closed:
						window.close();
						socket->unbind();
						end = true;
						break;
					case sf::Event::MouseButtonPressed:
						if (evento.mouseButton.button == sf::Mouse::Left) {
							int x = evento.mouseButton.x;
							int y = evento.mouseButton.y;

							if (y >= rect1.getPosition().y && y <= rect1.getPosition().y+rect1.getSize().y) {
								if (x >= rect1.getPosition().x && x <= rect1.getPosition().x+rect1.getSize().x) {
									//selectedOption = 0;
								}
								else if (x >= rect2.getPosition().x && x <= rect2.getPosition().x + rect2.getSize().x) {
									programState = MATCH_CREATION;
									//selectedOption = 1;
								}
							}

							if (x >= rect3.getPosition().x&&x <= rect3.getPosition().x + rect3.getSize().x) {
								if (y >= rect3.getPosition().y&&y <= rect3.getPosition().y + rect3.getSize().y) {

									OutputMemoryBitStream ombs;
									ombs.Write(PacketType::DISCONNECT,commandBits);

									status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
									
									if (status == sf::Socket::Error) {
										std::cout << "Error enviando desconexión\n";
									}else{
										programState = LOGIN;
									}
									
									//std::cout << "DISCONNECT\n";
								}
							}




						}
					}
				}

				window.draw(rect3);
				window.draw(rect1);
				window.draw(rect2);
				window.draw(disconnectText);
				window.draw(JoinGameText);
				window.draw(CreateGameText);
				window.display();
				break; 
			}
			case MATCH_CREATION: {
				window.clear();




				window.display();
				break;
			}
			case MATCH: {
				//CONTROL DE REENVIOS POR HASTA QUE EL SERVIDOR NOS CONFIRMA LA CONEXION
				if (clock.getElapsedTime().asMilliseconds() > 500 && !connected) {
					OutputMemoryBitStream ombs;
					ombs.Write(PacketType::HELLO, commandBits);
					status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

					if (status != sf::Socket::Done) {
						std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
					}
					else if (status == sf::Socket::Done) {
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
					int aLeftScore = 0;
					int aRightScore = 0;
					bool aWinner = false;
					std::pair<short, short>someCoords = { 0,0 };
					OutputMemoryBitStream ombs;
					//Client* aClient = nullptr;
					int anIndex = 0;
					switch (command) {
					case PacketType::WELCOME:

						serverMessageClock.restart();
						serverMessage = "WELCOME! \nSCORE " + std::to_string(victoryScore) + " GOALS TO WIN";

						imbs.Read(&myId, playerSizeBits);
						imbs.Read(&playerSize, playerSizeBits);
						playerSize++;
						for (int i = 0; i < playerSize; i++) {
							Client* aClient = new Client();
							aClient->position.first = aClient->position.second = 0;

							imbs.Read(&aClient->id, playerSizeBits);
							imbs.Read(&aClient->position.first, coordsbits);
							imbs.Read(&aClient->position.second, coordsbits);

							if (aClient->id != myId&&myId > 0) {
								std::cout << "Recibiendo cliente preexistente con id " << aClient->id << " y coordenadas " << aClient->position.first << "," << aClient->position.second << std::endl;
								aClients.push_back(aClient);
								sf::CircleShape playerRender(playerRadius);
								playerRenders.push_back(playerRender);

							}
							else {
								myCoordenates = aClient->position;
								auxPosition = myCoordenates;
								originalCoordenates = myCoordenates;
								delete aClient;
							}
						}

						connected = true;
						std::cout << "MY ID IS " << myId << std::endl;
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

							sf::CircleShape playerRender(playerRadius);

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
						anIndex = GetIndexClientWithId(aPlayerId, &aClients);
						//aClient = GetClientWithId(leaverId, aClients);
						//if (aClient!=nullptr) {
						//	std::cout << "Player " << leaverId << " Disconnected\n";
						//	
						//}
						if (anIndex != -1) {
							aClients.erase(aClients.begin() + anIndex);
							playerRenders.erase(playerRenders.begin() + anIndex);
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
					case PacketType::MOVEBALL:
						imbs.Read(&someCoords.first, coordsbits);
						imbs.Read(&someCoords.second, coordsbits);
						//localBallCoords = someCoords;
						//ballSteps.push(someCoords);
						gameStart = true;

						InterpolateBallMovement(&ballSteps, localBallCoords, someCoords);
						break;
					case PacketType::GOAL:
						imbs.Read(&aCriticalId, criticalBits);
						acks.push_back(aCriticalId);

						imbs.Read(&aRightScore, scoreBits);
						imbs.Read(&aLeftScore, scoreBits);

						localScoreRightNum = aRightScore - 1;
						localScoreLeftNum = aLeftScore - 1;

						std::cout << "SCORE " << localScoreLeftNum << " - " << localScoreRightNum;
						localScoreLeft = std::to_string(localScoreLeftNum);
						localScoreRight = std::to_string(localScoreRightNum);
						break;
					case PacketType::GAMEOVER:
						imbs.Read(&aCriticalId, criticalBits);
						imbs.Read(&aWinner, boolBit);
						winner = aWinner;
						gameOver = true;
						break;
					case PacketType::GAMESTART:

						serverMessageClock.restart();
						serverMessage = "THE GAME HAS STARTED!";
						gameStart = true;
						imbs.Read(&aCriticalId, criticalBits);
						acks.push_back(aCriticalId);

						std::cout << "GameStarted\n";
						//myCoordenates = originalCoordenates;
						//auxPosition = myCoordenates;
						for (int i = 0; i < aClients.size(); i++) {
							aClients[i]->EmptyStepQueue();
						}
						break;
					case PacketType::ACKMOVE: {

						imbs.Read(&aPlayerId, playerSizeBits);
						imbs.Read(&aCriticalId, criticalBits); //Se lee aCriticalId pero en realidad es el ID del MOVIMIENTO
						imbs.Read(&someCoords.first, coordsbits);
						imbs.Read(&someCoords.second, coordsbits);
						AccumMove accumMove;
						Client* aClient = GetClientWithId(aPlayerId, aClients);

						if (aClient != nullptr&&aPlayerId != myId) { //Si no es nullptr no eres tu mismo.
																	 //aClient->position = someCoords;

																	 //std::cout << "Recibido ACKMOVE de jugador con ID " << aPlayerId << "Sus coordenadas son " << someCoords.first <<", "<<someCoords.second<< std::endl;

							std::pair<float, float>distance;
							std::pair <short, short>lastPosition;
							if (aClient->steps.size() > 0) {
								lastPosition = aClient->steps.back();
							}
							else {
								lastPosition = aClient->position;
							}



							distance.first = (someCoords.first - lastPosition.first);
							distance.second = someCoords.second - lastPosition.second;
							distance.first /= subdividedSteps;
							distance.second /= subdividedSteps;
							//std::cout << "someCoords -> " << someCoords.first << ", " << someCoords.second << " - lastPosition "<<lastPosition.first << ", " << lastPosition.second <<"\n";


							for (int i = 0; i < subdividedSteps; i++) {
								std::pair<short, short>aStep;
								aStep.first = lastPosition.first + (short)std::floor(distance.first*i);
								aStep.second = lastPosition.second + (short)std::floor(distance.second*i);

								if (aStep.first != lastPosition.first || aStep.second != lastPosition.second) {

									//std::cout << std::floor(distance.first*i)<<"\n";
									aClient->steps.push(aStep);
									std::cout << "Pushing step with coords-> " << aStep.first << ", " << aStep.second << "\n";

								}
							}
							/*	if (aPlayerId == myId) {
							myCoordenates = someCoords;
							auxPosition = myCoordenates;
							}*/

						}
						else if (aPlayerId == myId) {
							int ackIndex = 0;
							if (!CheckValidMoveId(&nonAckMoves, aCriticalId, someCoords, &ackIndex)) {
								myCoordenates = someCoords;
								auxPosition = myCoordenates;
								//std::cout << "MY NEW COORDS: " << myCoordenates.first << ", " << myCoordenates.second << std::endl;
								EraseAccums(&nonAckMoves, (int)(nonAckMoves.size() - 1));
								//std::cout << "NonAckMoves-> " << nonAckMoves.size()<<std::endl;
							}
							else {
								EraseAccums(&nonAckMoves, ackIndex);
								//std::cout << "NonAckMoves-> " << nonAckMoves.size() << std::endl;
							}
						}

						//std::cout << "Recibida posicion de jugador con ID " <<aPlayerId<< ".Sus coordenadas son "<<someCoords.first<<", "<<someCoords.second<<"\n";

						/*


						ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.first, coordsbits);
						ombs.Write(aClients[i]->acumulatedMoves[latestMessageIndex].absolute.second, coordsbits);*/
						break;
					}
					default:
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
							if (event.key.code == sf::Keyboard::Space) {
								if (shootCounter.getElapsedTime().asMilliseconds() > shootCoolDown) {
									shootCounter.restart();
									std::cout << "SHOOT ENVIADO\n";
									OutputMemoryBitStream ombs;
									ombs.Write(PacketType::SHOOT, commandBits);
									ombs.Write(myCoordenates.first, coordsbits);
									ombs.Write(myCoordenates.second, coordsbits);
									ombs.Write(localBallCoords.first, coordsbits);
									ombs.Write(localBallCoords.second, coordsbits);
									status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
									if (status == sf::Socket::Error) {
										std::cout << "Error enviando Shoot\n";
									}
									else {
										std::cout << "Enviando mi posicion: " << myCoordenates.first << ", " << myCoordenates.second << " y posicion bola " << localBallCoords.first << ", " << localBallCoords.second << std::endl;
									}
								}
							}
							else if (event.key.code == sf::Keyboard::Left) {
								left_key = true;
							}
							else if (event.key.code == sf::Keyboard::Right) {
								right_key = true;
							}
							else if (event.key.code == sf::Keyboard::Up) {
								up_key = true;
							}
							else if (event.key.code == sf::Keyboard::Down) {
								down_key = true;
							}

							else if (event.key.code == sf::Keyboard::Escape) {
								std::cout << "Application Closed\n";
								window.close();
								connected = false;
								end = true;
							}
							break;

						case sf::Event::KeyReleased:
							if (event.key.code == sf::Keyboard::Left) {
								left_key = false;
							}
							else if (event.key.code == sf::Keyboard::Right) {
								right_key = false;
							}
							else if (event.key.code == sf::Keyboard::Up) {
								up_key = false;
							}
							else if (event.key.code == sf::Keyboard::Down) {
								down_key = false;
							}
							break;
						default:
							break;
						}
					}

					//He sacado el control de movimiento fuera del switch de los eventos para poder realizar movimiento lateral.



					if (clockForMyMovement.getElapsedTime().asMilliseconds() > timeBetweenSteps) {
						if (up_key&&myCoordenates.second > 1) {
							int deltaX = 0;
							int deltaY = -playerSpeed;
							auxPosition.second += deltaY;
							currentDelta.second += deltaY;
							myCoordenates = auxPosition;
						}
						else if (down_key&&myCoordenates.second + playerRadius * 2 < 599) {
							int deltaX = 0;
							int deltaY = playerSpeed;
							auxPosition.second += deltaY;
							currentDelta.second += deltaY;
							myCoordenates = auxPosition;
						}
						if (right_key&&myCoordenates.first + playerRadius * 2 < 999) {
							int deltaX = playerSpeed;
							int deltaY = 0;
							auxPosition.first += deltaX;
							currentDelta.first += deltaX;
							myCoordenates = auxPosition;
						}
						else if (left_key&&myCoordenates.first > 1) {
							int deltaX = -playerSpeed;
							int deltaY = 0;
							auxPosition.first += deltaX;
							currentDelta.first += deltaX;
							myCoordenates = auxPosition;
						}
						clockForMyMovement.restart();
					}

					//MOVIMIENTO DE LA PELOTA
					if (clockForTheBallMovement.getElapsedTime().asMilliseconds() > timeBetweenSteps / 2) {
						if (ballSteps.size() > 100) {
							for (int i = 0; i < 25; i++) {
								ballSteps.pop();
							}
						}

						if (ballSteps.size() != 0) {
							localBallCoords = ballSteps.front();
							ballSteps.pop();
						}
						clockForTheBallMovement.restart();
					}

					window.clear();
					DrawMap(&mapLines, &window);

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


					if (clock.getElapsedTime().asMilliseconds() > 120 && connected) {
						if (currentDelta.first != 0 || currentDelta.second != 0) {
							AccumMove move;
							move.absolute = auxPosition;
							//std::cout << "ENVIANDO AUX " << auxPosition.first << ", " << auxPosition.second << "\n";

							if (currentMoveId > 255) {
								currentMoveId = 0;
							}

							move.idMove = currentMoveId;
							currentMoveId++;

							if (currentMoveId >= 255) {
								currentMoveId = 0;
							}

							nonAckMoves.push_back(move);
							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::MOVE, commandBits);
							ombs.Write(move.idMove, criticalBits);
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
					if (clockForTheStep.getElapsedTime().asMilliseconds() > timeBetweenSteps) {
						for (int i = 0; i < aClients.size(); i++) {
							if (aClients[i]->steps.size() > 0) {
								aClients[i]->position = aClients[i]->steps.front();
								aClients[i]->steps.pop();
							}
						}
						clockForTheStep.restart();
					}

					//DRAW DE PERSONAJES (dependiendo de quien sea se pintará de un color o otro)
					for (int i = 0; i < aClients.size(); i++) {
						if (myId == 0 || myId == 1) {
							if (aClients[i]->id == 2 || aClients[i]->id == 3) {
								playerRenders[i].setFillColor(sf::Color::Red);
							}
							else {
								playerRenders[i].setFillColor(sf::Color::Green);
							}
						}
						else {
							if (aClients[i]->id == 0 || aClients[i]->id == 1) {
								playerRenders[i].setFillColor(sf::Color::Red);
							}
							else {
								playerRenders[i].setFillColor(sf::Color::Green);
							}
						}

						playerRenders[i].setPosition(sf::Vector2f(aClients[i]->position.first, aClients[i]->position.second));
						window.draw(playerRenders[i]);
					}



					//Mi jugador
					sf::CircleShape myRender(playerRadius);
					myRender.setFillColor(sf::Color::Blue);
					myRender.setPosition(myCoordenates.first, myCoordenates.second);
					window.draw(myRender);


					//Pelota
					if (gameStart && !gameOver) {
						sf::CircleShape ballRender(ballRadius);
						ballRender.setFillColor(sf::Color::Yellow);
						ballRender.setPosition(localBallCoords.first, localBallCoords.second);
						window.draw(ballRender);
					}

					//Render puntuación
					DrawScores(localScoreLeft, localScoreRight, &serverMessage, &window, &serverMessageClock);

					if (gameOver) {
						if (clockGameOver.getElapsedTime().asSeconds() > 5) {
							end = true;
						}
						if ((myId == 0 || myId == 1) && !winner) {
							serverMessage = "YOU WIN!";
						}
						else if ((myId == 2 || myId == 3) && winner) {
							serverMessage = "YOU WIN!";
						}
						else {
							serverMessage = "YOU LOSE!";
						}
					}
					else {
						clockGameOver.restart();
					}

					window.display();

				}

				if (clockForTheServer.getElapsedTime().asMilliseconds() > 30000) {
					end = true;
					std::cout << "SERVIDOR DESCONECTADOOOO\n";
					socket->unbind();
				}

				break;
			}
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

void EraseAccums(std::vector<AccumMove>*nonAckMoves, int until) {
	if (nonAckMoves->size() > until) {
		nonAckMoves->erase(nonAckMoves->begin(), nonAckMoves->begin() + until);
	}
}

bool CheckValidMoveId(std::vector<AccumMove>*nonAckMoves, int aCriticalId, std::pair<short, short>someCoords, int* index) {
	for (int i = 0; i < nonAckMoves->size(); i++) {
		if (nonAckMoves->at(i).idMove == aCriticalId) {
			*index = i;
			if (someCoords == nonAckMoves->at(i).absolute) {
				return true;
			}
			else {
				return false;
			}
		}
	}
	*index = -1;
	return true;
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
			//std::cout << "Error al recibir informacion" << std::endl;
		}
		else {
			//std::cout << "Paquete recibido correctamente" << std::endl;
			incomingInfo->push(Event(message, sizeReceived, incomingIP, incomingPort));

		}
	}
}

void SetUpMapLines(std::vector<sf::RectangleShape>* mapLines) {
	//sf::RectangleShape rectangle(sf::Vector2f(1000, 7));
	//rectangle.setFillColor(sf::Color(100, 100, 100, 255));
	//mapLines->push_back(rectangle);//superior
	

	sf::RectangleShape rectangle(sf::Vector2f(1000,205));
	rectangle.setPosition(sf::Vector2f(0, 593));
	rectangle.setFillColor(sf::Color(100, 100, 100, 255));
	//rectangle.setSize(sf::Vector2f(1000, 205));
	mapLines->push_back(rectangle);//inferior
	
	//rectangle.setSize(sf::Vector2f(200, 7));
	//rectangle.setPosition(sf::Vector2f(7, 0));
	//rectangle.setRotation(90.0f);
	//mapLines->push_back(rectangle); //derecha superior
	//
	//rectangle.setPosition(sf::Vector2f(7, 400));
	//mapLines->push_back(rectangle); //derecha inferior
	//
	//rectangle.setPosition(sf::Vector2f(1000, 0));
	//mapLines->push_back(rectangle); //izquierda superior
	//
	//rectangle.setPosition(sf::Vector2f(1000, 400));
	//mapLines->push_back(rectangle); //izquierda inferior
}

void DrawMap(std::vector<sf::RectangleShape>* mapLines, sf::RenderWindow* window) {
	window->draw(bgSprite);
	for (int i = 0; i < mapLines->size(); i++) {
		window->draw(mapLines->at(i));
	}
}

void DrawScores(std::string localScoreLeft, std::string localScoreRight, std::string* serverMessage, sf::RenderWindow* window, sf::Clock* serverMessageClock) {
	sf::Text scoreLeftRender;
	scoreLeftRender.setFont(font);
	scoreLeftRender.setCharacterSize(80);
	scoreLeftRender.setString(localScoreLeft);
	scoreLeftRender.setPosition(100,650);
	scoreLeftRender.setFillColor(sf::Color::White);
	window->draw(scoreLeftRender);

	sf::Text scoreRightRender;
	scoreRightRender.setFont(font);
	scoreRightRender.setCharacterSize(80);
	scoreRightRender.setString(localScoreRight);
	scoreRightRender.setPosition(800,650);
	scoreRightRender.setFillColor(sf::Color::White);
	window->draw(scoreRightRender);



	if (serverMessage->size() > 0) {
		sf::Text serverTextRender;
		serverTextRender.setFont(font);
		serverTextRender.setCharacterSize(50);
		serverTextRender.setString(*serverMessage);
		serverTextRender.setPosition(window->getSize().x / 4, window->getSize().y / 8);
		serverTextRender.setFillColor(sf::Color::Black);

		if (serverMessageClock->getElapsedTime().asMilliseconds() < 5000) {
			window->draw(serverTextRender);
		}
		else {
			*serverMessage = "";
			serverMessageClock->restart();
		}
	}
	
}

void InterpolateBallMovement(std::queue<std::pair<short, short>>* ballSteps, std::pair<short, short> localBallPos,std::pair<short,short>newCoords) {
	std::pair<float, float>distance;
	std::pair <short, short>lastPosition;
	if (ballSteps->size() > 0) {
		lastPosition = ballSteps->back();
	}
	else {
		lastPosition = localBallPos;
	}
	distance.first = (newCoords.first - lastPosition.first);
	distance.second = newCoords.second - lastPosition.second;
	distance.first /= subdividedSteps*2;
	distance.second /= subdividedSteps*2;

	for (int i = 0; i < subdividedSteps*2; i++) {
		std::pair<short, short>aStep;
		aStep.first = lastPosition.first + (short)std::floor(distance.first*i);
		aStep.second = lastPosition.second + (short)std::floor(distance.second*i);

		if (aStep.first != lastPosition.first || aStep.second != lastPosition.second) {
			ballSteps->push(aStep);
			//std::cout << "Pushing step with coords-> " << aStep.first << ", " << aStep.second << "\n";
		}
	}
}

void LogIn(sf::UdpSocket* socket, std::string user, std::string password,sf::IpAddress ip, short port) {
	//TODO
	std::cout << "Trying to Log In with UserName\n";// " + user + " and password " + password + "\n";

	if (user.size() >= 0 && password.size() > 0) {
		OutputMemoryBitStream ombs;
		sf::Socket::Status status;
		ombs.Write(PacketType::LOGIN,commandBits);
		ombs.WriteString(user);
		ombs.WriteString(password);
		status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), ip, port);

		if (status == sf::Socket::Error) {
			std::cout << "Error intentando enviar LOGIN\n";
		}

	}
}

void Register(sf::UdpSocket* socket, std::string user, std::string password, std::string confirmedPassword) {

	if (confirmedPassword == password) {

		//TODO
		std::cout << "Trying to Register\n";// with UserName " + user + " and password " + password + "\n";
	}
}