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

void LogIn(sf::UdpSocket* socket, std::string user, std::string password, sf::IpAddress, unsigned short port);

void Register(sf::UdpSocket* socket, std::string user, std::string password, std::string confirmedPassword, sf::IpAddress ip, unsigned short port,std::string*, sf::Clock* );

void ResetValues(int* selectedOption);

sf::Font font;
sf::Texture bgTex;
sf::Sprite bgSprite;

class MatchInfo {
public:
	int matchId;
	std::string gameName;
	int connectedPlayers;
	int maxPlayers;
	MatchInfo(int id, std::string name, int mp, int connectedPlayers) {
		maxPlayers = mp;
		gameName = name;
		matchId = id;
		this->connectedPlayers = connectedPlayers;
	}

	MatchInfo(int id, std::string name, int mp) {
		maxPlayers = mp;
		gameName = name;
		matchId = id;
		connectedPlayers = 0;
	}


	//bool operator () (MatchInfo i, MatchInfo j, int sortMode) { 
	//	switch (sortMode)
	//	{
	//	case 0:
	//		return i.connectedPlayers < j.connectedPlayers;
	//		break;
	//	case 1: {
	//		for (int k = 0; k < i.gameName.size(); k++) {
	//			if (j.gameName.size() > k) {
	//				if (i.gameName[k] != j.gameName[k]) {
	//					std::string::size_type sz;   // alias of size_t
	//					return i.gameName[k] < j.gameName[k];
	//				}
	//			}
	//		}
	//		return (i.gameName.size() < j.gameName.size());
	//	
	//		break;
	//	}
	//	default:
	//		break;
	//	}
	//}

};

bool SortByNames(MatchInfo i, MatchInfo j) {
	for (int k = 0; k < i.gameName.size(); k++) {
		if (j.gameName.size() > k) {
			if (i.gameName[k] != j.gameName[k]) {
				std::string::size_type sz;   // alias of size_t
				return i.gameName[k] < j.gameName[k];
			}
		}
	}
	return (i.gameName.size() < j.gameName.size());
}

bool SortByPlayers(MatchInfo i, MatchInfo j) {
	return i.connectedPlayers < j.connectedPlayers;
}

class MatchDetailedInfo :public MatchInfo{
	std::vector<Client>clients;
};

int main() {
	bool welcomed = false;
	bgTex.loadFromFile("bg.jpg");
	bgSprite.setTexture(bgTex);
	font.loadFromFile("arial_narrow_7.ttf");

	std::vector<std::string>aMsjs;

	std::cout << "CLIENTE INICIADO" << std::endl;
	std::string serverIp = "localhost";
	int serverPort = 50000;
	unsigned short matchPort=0;

	sf::Socket::Status status;
	sf::UdpSocket* socket = new sf::UdpSocket();
	std::string command;
	int myId = 0;
	int myMatchId = 0;
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
	int currentMatchPlayerSize = 0;
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
	std::vector<MatchInfo> matchesInfo;
	std::queue<std::pair<short, short>> ballSteps;
	
	SetUpMapLines(&mapLines);

	std::thread t(&ReceptionThread, &end, &incomingInfo, socket);

	sf::Clock shootCounter;
	sf::Clock clock, clockForTheBallMovement;

	sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Cliente con ID "+myId);
	std::vector<sf::CircleShape> playerRenders;
	enum ProgramState { SERVERCHECK, LOGIN, MAIN_MENU,JOIN_MATCH, MATCH_CREATION, MATCH_PASS,MATCH_ROOM,MATCH };
	enum FilterMode{NONE,ALPHABETICAL,NUMPLAYERS};
	ProgramState programState = LOGIN;
	FilterMode filterMode = ALPHABETICAL;

	std::string myUsername="";
	std::string myPassword="";
	std::string myRegisteredUsername = "";
	std::string myRegisteredPassword = "";
	std::string myConfirmedPassword = "";

	std::string roomName = "";
	std::string gamePlayersNum = "";
	std::string goalsNumber = "";
	std::vector<std::string>aRoomNames;
	std::string myMessage = "";

	int selectedOption=0;
	bool logInAnswer = true;
	bool joinAnswer = true;
	bool readyPressed = false;

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
					PacketType command = PacketType::LOGIN;
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
						PacketType command = PacketType::LOGIN;
						infoReceived = incomingInfo.front();
						InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
						imbs.Read(&command, commandBits);
						switch (command) {
						case PacketType::LOGIN_OK: {
							std::cout << "LoginOKRecibido\n";
							logInAnswer = true;
							programState = ProgramState::MAIN_MENU;
							ResetValues(&selectedOption);
							break; 
						}
						case PacketType::LOGIN_NO: {
							logInAnswer = true;
							serverMessageClock.restart();
							serverMessage = "Contraseña/Usuario incorrectos";
							break;
						}
						case PacketType::REGISTER_OK: {
							bool accepted = false;
							int aCriticalId=0;
							//imbs.Read(&accepted, boolBit);
							imbs.Read(&aCriticalId, criticalBits);
							acks.push_back(aCriticalId);
							//if (accepted) {
								programState = ProgramState::MAIN_MENU;
								ResetValues(&selectedOption);
							//}
							break; 
						}
						case PacketType::REGISTER_NO: {
							serverMessageClock.restart();
							serverMessage = "Error durante el registro";
							break;
						}
						default:
							break;
						}
						incomingInfo.pop();
					}

					

					if (!logInAnswer&&logInClock.getElapsedTime().asSeconds()>0.3f) {
						if (myUsername.size() > 0 && myPassword.size() > 0) {
							LogIn(socket, myUsername, myPassword, serverIp, serverPort);
							std::cout << logInAnswer << std::endl;
						}
						else {
							logInAnswer = true;
						}
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
										if (myUsername.size() > 0 && myPassword.size() > 0) {
											logInAnswer = false;
										}
										else {
											serverMessageClock.restart();
											serverMessage = "Hay que rellenar los espacios";
										}
									}
								}
								if (x >= 480 && x <= (480 + 300)) {
									if (y >= 600 && y <= (600 + 45)) {
										if (myRegisteredPassword.size() > 0 && myRegisteredUsername.size() > 0 && myConfirmedPassword.size() > 0) {
											Register(socket, myRegisteredUsername, myRegisteredPassword, myConfirmedPassword, serverIp, serverPort,&serverMessage,&serverMessageClock);
										}
										else {
											//std::cout << "Falta introducir agun dato\n";
											serverMessageClock.restart();
											serverMessage = "Hay que rellenar los espacios";
										}
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
									if (myUsername.size() > 0 && myPassword.size() > 0) {
										logInAnswer = false;
									}
									else {
										serverMessageClock.restart();
										serverMessage = "Hay que rellenar los espacios";
									}
								}
								else {
									if (myRegisteredPassword.size() > 0 && myRegisteredUsername.size() > 0 && myConfirmedPassword.size() > 0) {
										Register(socket, myRegisteredUsername, myRegisteredPassword, myConfirmedPassword, serverIp, serverPort,&serverMessage,&serverMessageClock);
									}
									else {
										//std::cout << "Falta introducir agun dato\n";
										serverMessageClock.restart();
										serverMessage = "Hay que rellenar los espacios";
									}
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
							if (evento.text.unicode >= 32 && evento.text.unicode <= 126) {
								switch (selectedOption) {
									case 0:
										if (myUsername.length() < 20) {
											myUsername += (char)evento.text.unicode;
										}
										break;
									case 1:
										if (myPassword.length() < 20) {
											myPassword += (char)evento.text.unicode;
										}
										break;
									case 2:
										if (myRegisteredUsername.length() < 20) {
											myRegisteredUsername += (char)evento.text.unicode;
										}
										break;
									case 3:
										if (myRegisteredPassword.length() < 20) {
											myRegisteredPassword += (char)evento.text.unicode;
										}
										break;
									case 4:
										if (myConfirmedPassword.length() < 20) {
											myConfirmedPassword += (char)evento.text.unicode;
										}
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
					std::string starsRegisterConfirmPassword = "";

					for (int i = 0; i < myPassword.size(); i++) {
						starsPassword += "*";
					}

					for (int i = 0; i < myRegisteredPassword.size(); i++) {
						starsRegisterPassword += "*";
					}

					for (int i = 0; i < myConfirmedPassword.size(); i++) {
						starsRegisterConfirmPassword += "*";
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


					sf::Text* emailText = new sf::Text("Confirm Password: " + starsRegisterConfirmPassword, font, 20);
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

					if (serverMessage.size() > 0) {
						sf::Text serverTextRender;
						serverTextRender.setFont(font);
						serverTextRender.setCharacterSize(50);
						serverTextRender.setString(serverMessage);
						serverTextRender.setPosition(window.getSize().x / 4, window.getSize().y / 24);
						serverTextRender.setFillColor(sf::Color::White);

						if (serverMessageClock.getElapsedTime().asMilliseconds() < 5000) {
							window.draw(serverTextRender);
						}
						else {
							serverMessage = "";
							serverMessageClock.restart();
						}
					}


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
				std::vector<sf::RectangleShape>rectangleShapes;
				std::vector<sf::Text>matchNames;
				std::vector<sf::Text>playersText;

				sf::Text headerText1("Nombre de partida",font,40);
				headerText1.setPosition(20, 15);
				headerText1.setFillColor(sf::Color::White);

				sf::Text headerText2("Jugadores", font, 40);
				headerText2.setPosition(windowWidth - 200, 15);
				headerText2.setFillColor(sf::Color::White);



				sf::RectangleShape rect5(sf::Vector2f( 200, 60));
				rect5.setPosition(200, 0 + rect5.getSize().y);
				rect5.setFillColor(sf::Color::Blue);

				sf::RectangleShape rect6(sf::Vector2f( 200, 60));
				rect6.setPosition(400, 0 + rect6.getSize().y);
				rect6.setFillColor(sf::Color::Magenta);

				sf::RectangleShape rect7(sf::Vector2f( 200, 60));
				rect7.setPosition(600, 0 + rect7.getSize().y);
				rect7.setFillColor(sf::Color::Blue);


				sf::Text headerText5("No filtrar", font, 20);
				headerText5.setPosition(220, rect5.getSize().y+20);
				headerText5.setFillColor(sf::Color::White);

				sf::Text headerText6("Nombre", font, 20);
				headerText6.setPosition(420, rect6.getSize().y+20);
				headerText6.setFillColor(sf::Color::White);

				sf::Text headerText7("N. Jugadores", font, 20);
				headerText7.setPosition(620, rect7.getSize().y+20);
				headerText7.setFillColor(sf::Color::White);

				if (selectedOption > matchesInfo.size()) {
					selectedOption = matchesInfo.size() - 1;
				}

				if (matchesInfo.size() > 0) {
					if (filterMode == NONE) {

					}
					else if(filterMode==ALPHABETICAL){
						std::sort(matchesInfo.begin(), matchesInfo.end(), SortByNames);

					}
					else if (filterMode == NUMPLAYERS) {
						std::sort(matchesInfo.begin(), matchesInfo.end(), SortByPlayers);

					}
				}

				for (int i = 0; i < matchesInfo.size(); i++) {
					sf::RectangleShape aRect(sf::Vector2f(windowWidth - 200, 50));
					aRect.setPosition(100, 0 + aRect.getSize().y*(i + 3));
					sf::Text aGameName(matchesInfo[i].gameName, font, 30);
					aGameName.setPosition(aRect.getPosition().x + 15, aRect.getPosition().y + 10);
					sf::Text aPlayerText(std::to_string(matchesInfo[i].connectedPlayers) + "/" + std::to_string(matchesInfo[i].maxPlayers), font, 30);
					aPlayerText.setPosition(windowWidth - 200, aRect.getPosition().y + 15);

					if (selectedOption == i) {
						aRect.setFillColor(sf::Color::Yellow);
						aGameName.setFillColor(sf::Color::Black);
						aPlayerText.setFillColor(sf::Color::Black);
					}
					else {
						aRect.setFillColor(sf::Color::Green);
						aGameName.setFillColor(sf::Color::White);
						aPlayerText.setFillColor(sf::Color::White);


					}
					rectangleShapes.push_back(aRect);
					matchNames.push_back(aGameName);
					playersText.push_back(aPlayerText);
				}

				if (window.isOpen()) {
					window.clear();

					if (shootCounter.getElapsedTime().asSeconds() > 4.0f) {
						shootCounter.restart();
						OutputMemoryBitStream ombs;
						ombs.Write(PacketType::UPDATEGAMELIST, commandBits);
						status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

						if (status == sf::Socket::Error) {
							std::cout << "Error pidiendo actualizacion de partidas\n";
						}
						else {
							std::cout << "Pidiendo actualizacion de paartidas\n";
						}

					}

					if (!incomingInfo.empty()) {
						Event infoReceived;
						PacketType command = PacketType::LOGIN;
						infoReceived = incomingInfo.front();
						InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
						imbs.Read(&command, commandBits);
						switch (command) {
						case PacketType::UPDATEGAMELIST: {
							matchesInfo.clear();
							int matchesSize = 0;

							imbs.Read(&matchesSize, matchBits);

							for (int i = 0; i < matchesSize; i++) {
								int someClientSize = 0;
								int anId = 0;
								std::string someName = "";
								int someMaxSize = 0;
								imbs.Read(&someClientSize, playerSizeBits);
								imbs.Read(&anId, matchBits);
								imbs.ReadString(&someName);
								imbs.Read(&someMaxSize, playerSizeBits);
								MatchInfo matchInfo(anId, someName, someMaxSize, someClientSize);
								
								std::cout << "Recibida sala con nombre" << someName << std::endl;

								matchesInfo.push_back(matchInfo);

							}
							break;

						}case PacketType::REGISTER_OK: {
							int aCriticalId = 0;
							imbs.Read(&aCriticalId, criticalBits);
							acks.push_back(aCriticalId);
							break;
						}
						case PacketType::JOINGAME_OK:{
							//int aCriticalID = 0;
							//imbs.Read(&aCriticalID, criticalBits);

							//acks.push_back(aCriticalID);

								unsigned short aMatchPort = 0;
								imbs.Read(&aMatchPort, portBits);
								matchPort = aMatchPort;

								int aSize = 0;
								imbs.Read(&aSize, playerSizeBits);
								aRoomNames.clear();
								for (int i = 0; i < aSize; i++){
									std::string aName = "";
									imbs.ReadString(&aName);
									aRoomNames.push_back(aName);
								}


								programState = MATCH_ROOM;

								OutputMemoryBitStream ombs;
								ombs.Write(PacketType::UPDATEROOM, commandBits);
								status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);

								if (status == sf::Socket::Error) {
									std::cout << "Error pidiendo actualizacion de partidas\n";
								}
								else {
									std::cout << "Pidiendo actualizacion de paartidas\n";
								}

							joinAnswer = true;
							break;
						}case PacketType::JOINGAME_NO: {
							//int aCriticalID = 0;
							//imbs.Read(&aCriticalID, criticalBits);

							//acks.push_back(aCriticalID);
							std::cout << "No puedes unirte a la partida\n";
							//serverMessage = "Error uniendose a partida\n";
							joinAnswer = true;
							break;
						}
						case PacketType::PING: {
							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::ACKPING, commandBits);
							status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

							if (status == sf::Socket::Error) {
								std::cout << "Error enviando ACKPING\n";
							}
							else {
								std::cout << "ENVIANDO ACKPING\n";
							}

							break;
						}
						default:
							break;
						}
						incomingInfo.pop();
					}


					float windowWidthf = windowWidth;
					float windowHeightf = windowHeight;


					//sf::RectangleShape rect1(sf::Vector2f(windowWidthf / 4, windowHeightf / 4));
					sf::RectangleShape rect1(sf::Vector2f(400.0f, 100.0f));
					rect1.setSize(sf::Vector2f(300, 100.0f));
					rect1.setFillColor(sf::Color::Green);
					rect1.setPosition(sf::Vector2f((windowWidthf / 4) - 150, windowHeightf - rect1.getSize().y));


					//sf::RectangleShape rect2(sf::Vector2f((windowWidthf / 4)*3, (windowHeightf / 4)*3 - 50));
					sf::RectangleShape rect2(sf::Vector2f(400.0f, 100.0f));
					rect2.setSize(sf::Vector2f(300, 100.0f));
					rect2.setPosition(sf::Vector2f((windowWidthf / 4) * 3 - 100, windowHeightf-rect2.getSize().y));
					rect2.setFillColor(sf::Color::Green);


					sf::Text JoinGameText("Join Game", font, 40);
					JoinGameText.setPosition(rect1.getPosition().x+20,rect1.getPosition().y+20);
					JoinGameText.setFillColor(sf::Color::White);

					sf::Text CreateGameText("Create Game", font, 40);
					CreateGameText.setPosition(rect2.getPosition().x+20, rect2.getPosition().y+20);
					CreateGameText.setFillColor(sf::Color::White);



					sf::RectangleShape rect3(sf::Vector2f(260.0f, 100.0f));
					rect3.setSize(sf::Vector2f(260.0f, 100.0f));
					rect3.setPosition(rect1.getPosition().x + rect1.getSize().x,rect1.getPosition().y);
					rect3.setFillColor(sf::Color::Red);

					sf::Text disconnectText("Disconnect", font, 40);
					disconnectText.setPosition(sf::Vector2f(rect3.getPosition().x+40, rect3.getPosition().y+20));
					disconnectText.setFillColor(sf::Color::White);



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

								for (int i = 0; i < matchesInfo.size(); i++) {
									if (rectangleShapes.size() != i) {
										if (x >= rectangleShapes[i].getPosition().x&&x <= rectangleShapes[i].getPosition().x + rectangleShapes[i].getSize().x) {
											if (y >= rectangleShapes[i].getPosition().y&&y <= rectangleShapes[i].getPosition().y + rectangleShapes[i].getSize().y) {
												if (selectedOption == i&&joinAnswer) {
													joinAnswer = false;
												}
												else {
													selectedOption = i;
												}
											}
										}
									}
								}

								if (y >= rect1.getPosition().y && y <= rect1.getPosition().y + rect1.getSize().y) {
									if (x >= rect1.getPosition().x && x <= rect1.getPosition().x + rect1.getSize().x) {
										//selectedOption = 0;
										myMatchId = matchesInfo[selectedOption].matchId;
										//programState = MATCH_ROOM;
										aMsjs.clear();
										ResetValues(&selectedOption);

									}
									else if (x >= rect2.getPosition().x && x <= rect2.getPosition().x + rect2.getSize().x) {

										//OutputMemoryBitStream ombs;
										//ombs.Write(PacketType::CREATEGAME,commandBits);
										//socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);


										programState = MATCH_CREATION;
										ResetValues(&selectedOption);
										//selectedOption = 1;
									}
								}

								if (x >= rect3.getPosition().x&&x <= rect3.getPosition().x + rect3.getSize().x) {
									if (y >= rect3.getPosition().y&&y <= rect3.getPosition().y + rect3.getSize().y) {

										OutputMemoryBitStream ombs;
										ombs.Write(PacketType::UNLOG, commandBits);

										status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

										if (status == sf::Socket::Error) {
											std::cout << "Error enviando desconexión\n";
										}
										else {
											programState = LOGIN;
											ResetValues(&selectedOption);

										}

										//std::cout << "DISCONNECT\n";
									}
								}
								if (y >= rect5.getPosition().y&&y <= rect5.getPosition().y + rect5.getSize().y) {
									if (x >= rect5.getPosition().x&&x <= rect5.getPosition().x + rect5.getSize().x) {
										filterMode = NONE;
									}else if (x >= rect6.getPosition().x&&x <= rect6.getPosition().x + rect6.getSize().x) {
										filterMode = ALPHABETICAL;
									}else if (x >= rect7.getPosition().x&&x <= rect7.getPosition().x + rect7.getSize().x) {
										filterMode = NUMPLAYERS;
									}
								}
	




							}
							break;
						}
					}
					
					if (!joinAnswer&&logInClock.getElapsedTime().asSeconds()>0.5f) {
						logInClock.restart();
						OutputMemoryBitStream ombs;
						ombs.Write(PacketType::JOINGAME, commandBits);
						ombs.Write(matchesInfo[selectedOption].matchId, matchBits);
						currentMatchPlayerSize = matchesInfo[selectedOption].maxPlayers;
						std::cout << "Intentando unirse a partida\n";
						status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
						if (status == sf::Socket::Error) {
							std::cout << "Error mandando petición de unión a partida\n";
						}
					}

					if (serverMessage.size() > 0) {
						sf::Text serverTextRender;
						serverTextRender.setFont(font);
						serverTextRender.setCharacterSize(50);
						serverTextRender.setString(serverMessage);
						serverTextRender.setPosition(window.getSize().x / 4, window.getSize().y / 24);
						serverTextRender.setFillColor(sf::Color::White);

						if (serverMessageClock.getElapsedTime().asMilliseconds() < 5000) {
							window.draw(serverTextRender);
						}
						else {
							serverMessage = "";
							serverMessageClock.restart();
						}
					}


					window.draw(rect3);
					window.draw(rect1);
					window.draw(rect2);
					window.draw(disconnectText);
					window.draw(JoinGameText);
					window.draw(CreateGameText);

					window.draw(headerText1);
					window.draw(headerText2);

					window.draw(rect5);
					window.draw(rect6);
					window.draw(rect7);

					window.draw(headerText5);
					window.draw(headerText6);
					window.draw(headerText7);
		


					if (rectangleShapes.size() > 0) {
						for (int i = 0; i < matchesInfo.size(); i++) {
							if (rectangleShapes.size() != i) {
								window.draw(rectangleShapes[i]);
							}
							else {
								std::cout << rectangleShapes.size() << std::endl;
							}

							if (matchNames.size() != i) {
								window.draw(matchNames[i]);
							}

							if (playersText.size() != i) {
								window.draw(playersText[i]);
							}
						}
					}


					window.display();
					break;
				}
			}
			case MATCH_CREATION: {
				if (window.isOpen()) {
					window.clear();

					if (!incomingInfo.empty()) {
						Event infoReceived;
						PacketType command = PacketType::LOGIN;
						infoReceived = incomingInfo.front();
						InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
						imbs.Read(&command, commandBits);
						switch (command) {
						case PacketType::PING: {
							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::ACKPING, commandBits);
							status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

							if (status == sf::Socket::Error) {
								std::cout << "Error enviando ACKPING\n";
							}

							break;
						}
						case PacketType::CREATEGAME:{
							bool anAnswer = false;

							imbs.Read(&anAnswer, boolBit);
							joinAnswer = true;

							if (anAnswer) {
								programState = ProgramState::MAIN_MENU;
								roomName = "";
								gamePlayersNum = "";
								goalsNumber = "";
								ResetValues(&selectedOption);
							}else{

							}

							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::UPDATEGAMELIST, commandBits);
							status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

							if (status == sf::Socket::Error) {
								std::cout << "Error pidiendo actualizacion de partidas\n";
							}
							else {
								std::cout << "Pidiendo actualizacion de paartidas\n";
							}

							break;
						}
						default:
							break;
						}
						incomingInfo.pop();
					}
					sf::RectangleShape roomNameRect(sf::Vector2f(400,60));
					roomNameRect.setPosition(windowWidth / 2 - roomNameRect.getSize().x/2, windowHeight / 8-roomNameRect.getSize().y/2);
					roomNameRect.setFillColor(sf::Color::Green);

					sf::Text roomNameTextHeader("Nombre de sala", font, 30);
					roomNameTextHeader.setFillColor(sf::Color::White);
					roomNameTextHeader.setPosition(sf::Vector2f(roomNameRect.getPosition().x + 20, roomNameRect.getPosition().y -60));

					sf::Text roomNameText(roomName,font,30);
					roomNameText.setFillColor(sf::Color::White);
					roomNameText.setPosition(sf::Vector2f(roomNameRect.getPosition().x+50,roomNameRect.getPosition().y));

					sf::RectangleShape passRect(sf::Vector2f(400, 60));
					passRect.setPosition(windowWidth / 2 - passRect.getSize().x / 2, 50+(windowHeight / 8)*2 - passRect.getSize().y / 2);
					passRect.setFillColor(sf::Color::Green);

					sf::Text passText("Número de jugadores",font,30);
					passText.setFillColor(sf::Color::White);
					passText.setPosition(sf::Vector2f(passRect.getPosition().x + 20, passRect.getPosition().y - 60));

					sf::Text passTextHeader(gamePlayersNum, font, 30);
					passTextHeader.setFillColor(sf::Color::White);
					passTextHeader.setPosition(sf::Vector2f(passRect.getPosition().x + 50, passRect.getPosition().y ));


					sf::RectangleShape goalsNumberRect(sf::Vector2f(400, 60));
					goalsNumberRect.setPosition(windowWidth / 2 - goalsNumberRect.getSize().x / 2, 100 + (windowHeight / 8) * 3 - goalsNumberRect.getSize().y / 2);
					goalsNumberRect.setFillColor(sf::Color::Green);

					sf::Text goalNumbersTextHeader("Numero de goles (max 30)", font, 30);
					goalNumbersTextHeader.setFillColor(sf::Color::White);
					goalNumbersTextHeader.setPosition(sf::Vector2f(goalsNumberRect.getPosition().x + 20, goalsNumberRect.getPosition().y - 60));


					sf::Text goalsNumberText(goalsNumber, font, 30);
					goalsNumberText.setFillColor(sf::Color::White);
					goalsNumberText.setPosition(sf::Vector2f(goalsNumberRect.getPosition().x + 50, goalsNumberRect.getPosition().y ));



					sf::RectangleShape cancelRect(sf::Vector2f(400, 100));
					cancelRect.setFillColor(sf::Color::Green);
					cancelRect.setPosition(windowWidth / 4 - cancelRect.getSize().x / 2, 150 + (windowHeight / 8) * 5 - cancelRect.getSize().y / 2);

					sf::Text cancelText("Cencelar", font, 30);
					cancelText.setFillColor(sf::Color::White);
					cancelText.setPosition(cancelRect.getPosition().x + 30, cancelRect.getPosition().y + cancelRect.getSize().y / 2);

					sf::RectangleShape createRect(sf::Vector2f(400, 100));
					createRect.setFillColor(sf::Color::Green);
					createRect.setPosition((windowWidth / 4)*3 - createRect.getSize().x / 2, 150 + (windowHeight / 8) * 5 - createRect.getSize().y / 2);


					sf::Text createText("Crear", font, 30);
					createText.setFillColor(sf::Color::White);
					createText.setPosition(createRect.getPosition().x + 30, createRect.getPosition().y + createRect.getSize().y / 2);


					sf::Event evento;
					while (window.pollEvent(evento)) {
						switch (evento.type) {
						case sf::Event::Closed:
							window.close();
							socket->unbind();
							end = true;
							break;
						case sf::Event::KeyPressed:
							 if (evento.key.code == sf::Keyboard::BackSpace) {
								switch (selectedOption) {
								case 0:
									roomName = roomName.substr(0, roomName.size() - 1);
									break;
								case 1:
									gamePlayersNum = gamePlayersNum.substr(0, gamePlayersNum.size() - 1);
									break;
								case 2:
									goalsNumber = goalsNumber.substr(0, goalsNumber.size() - 1);
									break;
								default:
									break;
								}
							}
							else if (evento.key.code == sf::Keyboard::Tab) {
								selectedOption++;
								selectedOption = selectedOption % 3;
							}
							else if (evento.key.code == sf::Keyboard::Return) {
								if (roomName.size() > 0 && goalsNumber.size()>0 && gamePlayersNum.size()>0) {
									//programState = MATCH_ROOM;
									joinAnswer = false;
									ResetValues(&selectedOption);
								}
								else {
									serverMessage = "Hay que rellenar todos los espacios";
								}
							}
							break;
						case sf::Event::TextEntered:
							if (evento.text.unicode >= 32 && evento.text.unicode <= 126) {
								switch (selectedOption) {
								case 0:
									if (roomName.size() < 20) {
										roomName += (char)evento.text.unicode;
									}
									break;
								case 1:
									if (gamePlayersNum.size() < 20) {
										if (evento.text.unicode >= 48 && evento.text.unicode <= 57) {
											if (gamePlayersNum.size() < 5) {
												gamePlayersNum += (char)evento.text.unicode;
											}
										}
									}
									break;
								case 2:
									if (evento.text.unicode >= 48 && evento.text.unicode <= 57) {
										if (goalsNumber.size() < 2) {
											goalsNumber += (char)evento.text.unicode;
										}
									}
									break;
								default:
									break;
								}
							}
							break;
						case sf::Event::MouseButtonPressed:
							if (evento.mouseButton.button == sf::Mouse::Left) {
								int x = evento.mouseButton.x;
								int y = evento.mouseButton.y;

								if (x >= roomNameRect.getPosition().x&&x <= roomNameRect.getPosition().x + roomNameRect.getSize().x) {
									if (y >= roomNameRect.getPosition().y&&y <= roomNameRect.getPosition().y + roomNameRect.getSize().y) {
										selectedOption = 0;
									}
									else if (y >= passRect.getPosition().y&&y <= passRect.getPosition().y + passRect.getSize().y) {
										selectedOption = 1;
									}
									else if (y >= goalsNumberRect.getPosition().y&&y <= goalsNumberRect.getPosition().y + goalsNumberRect.getSize().y) {
										selectedOption = 2;
									}

								}

								if (y >= cancelRect.getPosition().y&&y <= cancelRect.getPosition().y+cancelRect.getSize().y) {
									if (x >= cancelRect.getPosition().x&&x <= cancelRect.getPosition().x + cancelRect.getSize().x) {
										 roomName = "";
										 gamePlayersNum = "";
										 goalsNumber = "";
										 programState = MAIN_MENU;
										 ResetValues(&selectedOption);
									}
									else if (x >= createRect.getPosition().x&&x <= createRect.getPosition().x + createRect.getSize().x) {
										if (roomName.size() > 0&&goalsNumber.size()>0&&gamePlayersNum.size()>0) {
											//programState = MATCH_ROOM;
											joinAnswer = false;
											ResetValues(&selectedOption);
										}
										else {
											serverMessage = "Hay que rellenar todos los espacios";
											serverMessageClock.restart();

										}
									}
								}


							}
							break;
						}
					}

					if (!joinAnswer&&shootCounter.getElapsedTime().asSeconds()>0.5f) {
						shootCounter.restart();
						OutputMemoryBitStream ombs;
						ombs.Write(PacketType::CREATEGAME, commandBits);
						ombs.WriteString(roomName);
						//ombs.WriteString(gamePlayersNum);
						std::string::size_type sz;   // alias of size_t
						int players = std::stoi(gamePlayersNum, &sz);
						int goals = std::stoi(goalsNumber, &sz);

						if (players > 4) {
							players = 4;
						}
						else if (players < 2) {
							players = 2;
						}

						if (goals > 30) {
							goals = 30;
						}
						ombs.Write(players,playerSizeBits);
						ombs.Write(goals, commandBits);

						status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

						if (status == sf::Socket::Error) {
							std::cout << "Error enviando CreateGame\n";
						}

					}

					window.draw(roomNameRect);
					window.draw(roomNameText);
					window.draw(roomNameTextHeader);

					window.draw(passRect);
					window.draw(passTextHeader);
					window.draw(passText);

					window.draw(goalsNumberRect);
					window.draw(goalsNumberText);
					window.draw(goalNumbersTextHeader);

					window.draw(cancelRect);
					window.draw(cancelText);

					window.draw(createRect);
					window.draw(createText);

					if (serverMessage.size() > 0) {
						sf::Text serverTextRender;
						serverTextRender.setFont(font);
						serverTextRender.setCharacterSize(50);
						serverTextRender.setString(serverMessage);
						serverTextRender.setPosition(window.getSize().x / 6, window.getSize().y / 8);
						serverTextRender.setFillColor(sf::Color::White);

						if (serverMessageClock.getElapsedTime().asMilliseconds() < 5000) {
							window.draw(serverTextRender);
						}
						else {
							serverMessage = "";
							serverMessageClock.restart();
						}
					}


					window.display();
				}
				break;
			}
			case MATCH_ROOM: {
				if (window.isOpen()) {
					window.clear();

					if (!incomingInfo.empty()) {
						Event infoReceived;
						PacketType command = PacketType::LOGIN;
						infoReceived = incomingInfo.front();
						InputMemoryBitStream imbs(infoReceived.message, infoReceived.messageSize * 8);
						imbs.Read(&command, commandBits);
						switch (command) {
						case PacketType::PING: {
							OutputMemoryBitStream ombs;
							ombs.Write(PacketType::ACKPING, commandBits);
							status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);

							if (status == sf::Socket::Error) {
								std::cout << "Error enviando ACKPING\n";
							}

							break;
						}
						/*case PacketType::WELCOME: {
							clockForTheServer.restart();
							int aCriticalId = 0;
							int victoryScore = 0;
							imbs.Read(&aCriticalId, criticalBits);
							std::cout << "CRITICAL ID RECIBIDO " << aCriticalId << std::endl;


							imbs.Read(&victoryScore, criticalBits);
							if (!welcomed) {
								std::cout << "RECIBIDO WELCOME EN SALA\n";
								programState = ProgramState::MATCH;
								readyPressed = false;
								welcomed = true;
								serverMessageClock.restart();
								serverMessage = "WELCOME! \nSCORE " + std::to_string(victoryScore) + " GOALS TO WIN";
								int playerSize=0;
								imbs.Read(&myId, playerSizeBits);
								acks.push_back(aCriticalId);
								imbs.Read(&playerSize, playerSizeBits);
								//playerSize++;

								//std::cout << "aCriticalId = " << aCriticalId << std::endl;
								std::cout << "playerSize recibido = " << playerSize << std::endl;

								for (int i = 0; i < playerSize; i++) {
									Client* aClient = new Client();
									int anotherId = 0;
									//aClient->matchId = 0;
									aClient->position.first = aClient->position.second = 0;

									imbs.Read(&anotherId, playerSizeBits);

									aClient->matchId = anotherId;

									imbs.Read(&aClient->position.first, coordsbits);
									imbs.Read(&aClient->position.second, coordsbits);

									if (aClient->matchId != myId&&myId >= 0) {
										std::cout << "Recibiendo cliente preexistente con id " << aClient->matchId << " y coordenadas " << aClient->position.first << "," << aClient->position.second << std::endl;
										if (aClient->matchId < 2) {
											aClients.push_back(aClient);
											sf::CircleShape playerRender(playerRadius);
											playerRenders.push_back(playerRender);
										}

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
							}
							else {
								acks.push_back(aCriticalId);

							}

							break;
							}*/
						case GAMEINFO: {
							clockForTheServer.restart();
							myId = 0;
							int aCriticalId = 0;
							int aWinScore = 0;
							int aSize = 0;
							int myNewId = 0;
							std::pair<short, short>someCoords = { 0,0 };
							int anId = 0;


							imbs.Read(&aCriticalId, criticalBits);
							imbs.Read(&aWinScore, criticalBits);
							imbs.Read(&myNewId, playerSizeBits);
							imbs.Read(&aSize, playerSizeBits);
	
							myId = myNewId;

							serverMessageClock.restart();
							serverMessage = "WELCOME! \nSCORE " + std::to_string(aWinScore) + " GOALS TO WIN";

							std::cout << "MyId = " << myId << std::endl;
							std::cout << "size = " << aSize << std::endl;


							for (int k = 0; k < aSize; k++) {


								//std::cout << "Escribiendo idPlayer " << aMatches[i]->aClients[k]->GetMatchID() << std::endl;;
								imbs.Read(&anId, playerSizeBits);
								imbs.Read(&someCoords.first, coordsbits);
								imbs.Read(&someCoords.second, coordsbits);

								std::cout << "checking Player id = " << anId << std::endl;


								Client* aClient = new Client();
								aClient->matchId = anId;
								aClient->position.first = someCoords.first;
								aClient->position.second = someCoords.second;
								if (aClient->matchId != myId&&myId >= 0) {
									std::cout << "Recibiendo cliente preexistente con id " << aClient->matchId << " y coordenadas " << aClient->position.first << "," << aClient->position.second << std::endl;
									if (aClient->matchId < currentMatchPlayerSize) {
										aClients.push_back(aClient);
										sf::CircleShape playerRender(playerRadius);
										playerRenders.push_back(playerRender);
									}

								}
								else {
									myCoordenates = aClient->position;
									auxPosition = myCoordenates;
									originalCoordenates = myCoordenates;
									delete aClient;
								}

							}
							connected = true;
							acks.push_back(aCriticalId);

							programState = ProgramState::MATCH;
							break; 
						}
						case EXITROOM: {
							//int aCriticalId = 0;
							//imbs.Read(&aCriticalId, criticalBits);
							aMsjs.clear();
							aRoomNames.clear();
							ResetValues(&selectedOption);
							programState = MAIN_MENU;
							readyPressed = false;

							OutputMemoryBitStream auxOmbs;
							auxOmbs.Write(PacketType::UPDATEGAMELIST);
							status = socket->send(auxOmbs.GetBufferPtr(), auxOmbs.GetByteLength(), serverIp, serverPort);

							if (status == sf::Socket::Error) {
								std::cout << "Error enviando UpdateGameList\n";
							}
							//acks.push_back(aCriticalId);
							break; 
						}
						case MSG: {
							std::string aMsj = "";
							imbs.ReadString(&aMsj);
							aMsjs.push_back(aMsj);

							if (aMsjs.size() > 25) {
								aMsjs.erase(aMsjs.begin());
							}
							break;
						}
						case UPDATEROOM: {
							int aSize = 0;

							aRoomNames.clear();
							imbs.Read(&aSize, playerSizeBits);

							std::cout << "Recibido UPDATEROOM con " << aSize << " nombres\n";


							for (int i = 0; i < aSize; i++) {
								std::string aUserName = "";
								imbs.ReadString(&aUserName);
								aRoomNames.push_back(aUserName);
							}

							break;
						}
						default:
							break;
						}
						incomingInfo.pop();
					}

					std::vector<sf::Text>chat;
					std::vector<sf::Text>userNamesText;

					for (int i = 0; i < aMsjs.size(); i++) {
						sf::Text aText(aMsjs[i],font,15);
						aText.setPosition((float)windowWidth/2+70,50+i*18);
						aText.setFillColor(sf::Color::White);
						chat.push_back(aText);
					}

					for (int i = 0; i < aRoomNames.size(); i++) {
						sf::Text aText(aRoomNames[i], font, 40);
						aText.setPosition((float)70, windowHeight - 200 - i * 46);

						if (aRoomNames[i] == myUsername) {
							aText.setFillColor(sf::Color::Green);
						}
						else {
							aText.setFillColor(sf::Color::White);
						}

						userNamesText.push_back(aText);
					}

					sf::RectangleShape rect2(sf::Vector2f(50, windowHeight));
					rect2.setPosition((float)windowWidth/2,0);
					rect2.setFillColor(sf::Color::Cyan);

					sf::RectangleShape rect1(sf::Vector2f(200,100));
					rect1.setPosition(0, windowHeight - rect1.getSize().y - 40);
					if (readyPressed) {
						rect1.setFillColor(sf::Color::Blue);
					}
					else {
						rect1.setFillColor(sf::Color::Green);
					}

					sf::Text rect1Header("Ready",font,40);
					rect1Header.setPosition(rect1.getPosition().x+100-20, rect1.getPosition().y+50-20);
					rect1Header.setFillColor(sf::Color::Black);

					sf::RectangleShape rect3(sf::Vector2f(300.0f, 100.0f));
					rect3.setSize(sf::Vector2f(250, 100.0f));
					rect3.setPosition(rect1.getPosition().x+rect1.getSize().x,rect1.getPosition().y);
					rect3.setFillColor(sf::Color::Red);

					sf::RectangleShape rect4(sf::Vector2f(300.0f, 50));
					rect4.setSize(sf::Vector2f(windowWidth, 10.0f));
					rect4.setPosition(0, windowHeight - 100);
					rect4.setFillColor(sf::Color::Cyan);

					sf::Text disconnectText("Disconnect", font, 40);
					disconnectText.setPosition(sf::Vector2f(rect3.getPosition().x + 40, rect3.getPosition().y + 20));
					disconnectText.setFillColor(sf::Color::White);

					sf::Text myMessageText(myMessage, font, 20);
					myMessageText.setPosition(sf::Vector2f((float)windowWidth / 2 + 70, rect4.getPosition().y+rect4.getSize().y+20));
					myMessageText.setFillColor(sf::Color::White);

					

					sf::Event evento;

					while (window.pollEvent(evento)) {
						switch (evento.type) {
						case sf::Event::Closed:
							window.close();
							socket->unbind();
							end = true;
							break;
						case sf::Event::KeyPressed:
							if (evento.key.code == sf::Keyboard::Return) {
								OutputMemoryBitStream ombs;
								ombs.Write(PacketType::MSG, commandBits);
								ombs.WriteString(myMessage);

								socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);

								myMessage = "";
							}else if (evento.key.code == sf::Keyboard::BackSpace) {
								myMessage.erase(myMessage.size() - 1);
							}
							else if (evento.key.code == sf::Keyboard::Space) {
								myMessage += " ";
							}
							break;
						case sf::Event::TextEntered:
							if (evento.text.unicode >= 32 && evento.text.unicode <= 126) {
								myMessage += (char)evento.text.unicode;
							}
							break;
						case sf::Event::MouseButtonPressed:
							if (evento.mouseButton.button == sf::Mouse::Left) {
								int x = evento.mouseButton.x;
								int y = evento.mouseButton.y;

								if (y >= rect1.getPosition().y && y <= rect1.getPosition().y + rect1.getSize().y) {
									if (x >= rect1.getPosition().x && x <= rect1.getPosition().x + rect1.getSize().x) {
										//PULSADO READY
										readyPressed = true;
										OutputMemoryBitStream ombs;
										ombs.Write(PacketType::READY, commandBits);
										status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);

										std::cout << "ENVIANDO READY A PORT " << matchPort<<"\n";

										if (status == sf::Socket::Error) {
											std::cout << "Error enviando ready\n";
										}


									}
									else if (x >= rect3.getPosition().x&& x<= rect3.getPosition().x + rect3.getSize().x) {
										OutputMemoryBitStream ombs;
										ombs.Write(PacketType::EXITROOM, commandBits);
										status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);

										std::cout << "ENVIANDO EXITROOM " << matchPort << "\n";

										if (status == sf::Socket::Error) {
											std::cout << "Error enviando ready\n";
										}

									}
								}
							}
							break;
						}
					}


					window.draw(rect4);
					window.draw(rect3);
					window.draw(disconnectText);
					window.draw(rect2);
					window.draw(rect1);
					window.draw(rect1Header);
					for (int i = 0; i < chat.size(); i++) {
						window.draw(chat[i]);
					}



					window.draw(myMessageText);

					for (int i = 0; i < userNamesText.size(); i++) {
						window.draw(userNamesText[i]);
					}

					window.display();
				}
				break;
			}
			case MATCH: {
				//CONTROL DE REENVIOS POR HASTA QUE EL SERVIDOR NOS CONFIRMA LA CONEXION
				//if (clock.getElapsedTime().asMilliseconds() > 500 && !connected) {
				//	OutputMemoryBitStream ombs;
				//	ombs.Write(PacketType::HELLO, commandBits);
				//	status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);

				//	if (status != sf::Socket::Done) {
				//		std::cout << "El mensaje no se ha podido enviar correctamente" << std::endl;
				//	}
				//	else if (status == sf::Socket::Done) {
				//		std::cout << "Intentando conectar al servidor..." << std::endl;
				//	}
				//	clock.restart();
				//}

				if (!incomingInfo.empty()) {
					Event inc;
					inc = incomingInfo.front();
					Event infoReceived;
					PacketType command = PacketType::LOGIN;
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
					/*case PacketType::WELCOME: {
						aClients.clear();
						playerRenders.clear();
						//if (!welcomed) {
							int victoryScore = 0;
							std::cout << "RECIBIDO WELCOME EN PARTIDA\n";

							welcomed = true;
							serverMessageClock.restart();
							imbs.Read(&aCriticalId, criticalBits);

							imbs.Read(&victoryScore, criticalBits);

							serverMessage = "WELCOME! \nSCORE " + std::to_string(victoryScore) + " GOALS TO WIN";


							imbs.Read(&myId, playerSizeBits);

							acks.push_back(aCriticalId);


							imbs.Read(&playerSize, playerSizeBits);
							//playerSize++;

							std::cout << "aCriticalId = " << aCriticalId << std::endl;


							for (int i = 0; i < playerSize; i++) {
								Client* aClient = new Client();
								int anotherId = 0;
								//aClient->matchId = 0;
								aClient->position.first = aClient->position.second = 0;

								imbs.Read(&anotherId, playerSizeBits);

								aClient->matchId = anotherId;

								imbs.Read(&aClient->position.first, coordsbits);
								imbs.Read(&aClient->position.second, coordsbits);

								if (aClient->matchId != myId&&myId >= 0) {
									std::cout << "Recibiendo cliente preexistente con id " << aClient->matchId << " y coordenadas " << aClient->position.first << "," << aClient->position.second << std::endl;
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
						//}
						//else {

						//}
						break; 
					}*/
					case GAMEINFO: {
						clockForTheServer.restart();
						myId = 0;
						int aCriticalId = 0;
						int aWinScore = 0;
						int aSize = 0;
						int myNewId = 0;
						std::pair<short, short>someCoords = { 0,0 };
						int anId = 0;


						imbs.Read(&aCriticalId, criticalBits);
						imbs.Read(&aWinScore, criticalBits);
						imbs.Read(&myNewId, playerSizeBits);
						imbs.Read(&aSize, playerSizeBits);

						myId = myNewId;

						serverMessageClock.restart();
						serverMessage = "WELCOME! \nSCORE " + std::to_string(aWinScore) + " GOALS TO WIN";

						for (int k = 0; k < aSize; k++) {


							//std::cout << "Escribiendo idPlayer " << aMatches[i]->aClients[k]->GetMatchID() << std::endl;;
							imbs.Read(&anId, playerSizeBits);
							imbs.Read(&someCoords.first, coordsbits);
							imbs.Read(&someCoords.second, coordsbits);

							Client* aClient = new Client();
							aClient->matchId = anId;
							aClient->position.first = someCoords.first;
							aClient->position.second = someCoords.second;

							if (aClient->matchId != myId&&myId >= 0) {
								std::cout << "Recibiendo cliente preexistente con id " << aClient->matchId << " y coordenadas " << aClient->position.first << "," << aClient->position.second << std::endl;
								if (aClient->matchId < currentMatchPlayerSize) {
									aClients.push_back(aClient);
									sf::CircleShape playerRender(playerRadius);
									playerRenders.push_back(playerRender);
								}

							}
							else {
								myCoordenates = aClient->position;
								auxPosition = myCoordenates;
								originalCoordenates = myCoordenates;
								delete aClient;
							}

						}
						connected = true;
						acks.push_back(aCriticalId);

						programState = ProgramState::MATCH;
						break;
					}
					case PacketType::PING:
						ombs.Write(PacketType::ACKPING, commandBits);
						socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);
						socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
						clockForTheServer.restart();
						break;

					case PacketType::NEWPLAYER:
						for (int i = 0; i < 1; i++) {
							Client* newPlayer = new Client();
							imbs.Read(&aCriticalId, criticalBits);
							std::cout << "PUSHING ACK WITH ID" << aCriticalId << " NEW PLAYER " << std::endl;
							imbs.Read(&aPlayerId, playerSizeBits);
							newPlayer->matchId = aPlayerId;
							imbs.Read(&newPlayer->position.first, coordsbits);
							imbs.Read(&newPlayer->position.second, coordsbits);
							acks.push_back(aCriticalId);

							sf::CircleShape playerRender(playerRadius);

							playerRenders.push_back(playerRender);

							if (GetClientWithId(newPlayer->matchId, aClients) == nullptr) {
								aClients.push_back(newPlayer);
								std::cout << "Adding player with id " << newPlayer->matchId << std::endl;
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
							gameOver = true;
						}
						else {
							std::cout << "Trying to disconnect non existing player with id " << aPlayerId << std::endl;
							gameOver = true;

						}
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

							std::cout << "OTHERPLAYERMOIN\n";


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
							
							std::cout << "My Id is "<< myId<<"receiving id "<<aPlayerId<<"\n";
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
						}else{
							/////////////////////////////////////////////////////
							std::cout << "NEW CLIENT with id " << aPlayerId << " but my Id is "<<myId<<"\n";
							aClient = new Client();
							aClient->matchId = aPlayerId;
							aClient->position.first = someCoords.first;
							aClient->position.second = someCoords.first;

							aClients.push_back(aClient);

							sf::CircleShape playerRender(playerRadius);

							playerRenders.push_back(playerRender);

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

				//if (acks.size() > 0) {
				//	for (int i = 0; i < acks.size(); i++) {
				//		OutputMemoryBitStream ombs;
				//		ombs.Write(PacketType::ACK, commandBits);
				//		ombs.Write(acks[i], criticalBits);
				//		std::cout << "Sending ack " << acks[i] << std::endl;
				//		socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);
				//	}
				//	acks.clear();
				//}


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
									status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);
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

						int indexToDelete = -1;
						for (int i = 0; i < aClients.size(); i++) {
							if (aClients[i]->matchId > currentMatchPlayerSize -1) {
								if (indexToDelete == -1) {
									indexToDelete = i;
								}

							}
						}

						if (indexToDelete > -1) {
							aClients.erase(aClients.begin() + indexToDelete);
							playerRenders.erase(playerRenders.begin() + indexToDelete);
						}

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
									status = socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);
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
							if (aClients[i]->matchId == 2 || aClients[i]->matchId == 3) {
								playerRenders[i].setFillColor(sf::Color::Red);
							}
							else {
								playerRenders[i].setFillColor(sf::Color::Green);
							}
						}
						else {
							if (aClients[i]->matchId == 0 || aClients[i]->matchId == 1) {
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
							//end = true;
							joinAnswer = true;
							matchPort = 0;
							programState = MAIN_MENU;
							myId = 0;
							welcomed = false;
							ResetValues(&selectedOption);
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

		if (acks.size() > 0) {
			for (int i = 0; i < acks.size(); i++) {
				OutputMemoryBitStream ombs;
				ombs.Write(PacketType::ACK, commandBits);
				ombs.Write(acks[i], criticalBits);
				if (matchPort > 0) {
					socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, matchPort);
					std::cout << "Sending ack to Match " << acks[i] << std::endl;

				}
				else {
					socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), serverIp, serverPort);
					std::cout << "Sending ack to Server " << acks[i] << std::endl;

				}
			}
			acks.clear();
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
		if (aClients[i]->matchId == id){
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

void LogIn(sf::UdpSocket* socket, std::string user, std::string password,sf::IpAddress ip, unsigned short port) {
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

void Register(sf::UdpSocket* socket, std::string user, std::string password, std::string confirmedPassword,sf::IpAddress ip, unsigned short port, std::string* serverText,sf::Clock* relojServer ) {
	sf::Socket::Status status;
	if (confirmedPassword == password) {

		//TODO
		//std::cout << "Trying to Register\n";// with UserName " + user + " and password " + password + "\n";
		OutputMemoryBitStream ombs;
		ombs.Write(PacketType::REGISTER, commandBits);
		ombs.WriteString(user);
		ombs.WriteString(password);
		status=socket->send(ombs.GetBufferPtr(), ombs.GetByteLength(), ip, port);
		if (status == sf::Socket::Error) {
			std::cout << "Error enviando mensaje de registro\n";
		}
	}
	else {
		std::cout << "Passwords do not match";// with UserName " + user + " and password " + password + "\n";
		*serverText = "Las contraseñas deben coincidir";
		relojServer->restart();
	}
}

void ResetValues(int* selectedOption) {
	*selectedOption = 0;
}

