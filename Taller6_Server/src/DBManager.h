#pragma once

#define HOST "tcp://192.168.1.110:3306"
#define USER "root"
#define PASSWORD "Pentakill123"

#include <mysql_connection.h>
#include <cppconn\driver.h>
#include <cppconn\resultset.h>
#include <cppconn\statement.h>
#include <cppconn\exception.h>

class DBManager {
public:
	sql::Driver* driver;
	sql::Connection* con;
	sql::Statement* stmt;
	sql::ResultSet* rs;

	DBManager() {
		driver = get_driver_instance();
		con = driver->connect(HOST, USER, PASSWORD);
		stmt = con->createStatement();
		stmt->execute("USE  PracticaDB");
	}
	
	~DBManager() {
		driver = nullptr;
		delete con;
		delete stmt;
		//delete rs;
	}

	/*
	 * Crea una nueva entrada en la base de datos si el usuario no está repetido.
	 * 
	 * Devuelve true si el registro se ha realizado con exito y false si el usuario está repetido.
	 */
	bool Register(std::string user, std::string pass) {
		rs = stmt->executeQuery(("select count(*) from Accounts where username='" + user + "'").c_str());
		rs->next();
		int i = rs->getInt(1);
		delete rs;
		if (i == 0) {
			stmt->execute(("insert into Accounts(username,password) values ('" + user + "','" + pass + "')").c_str());
			
			return true;
		}
		else {
			std::cout << "USUARIO REPETIDO" << std::endl;
		}
		return false;
	}

	/*
	 * Devuelve true si la informacion de usuario y contraseña es correcta, de lo contrario
	 * devuelve false.
	 */
	bool Login(std::string user, std::string pass) {
		rs = stmt->executeQuery(("select count(*) from Accounts where username='" + user +"' and password='"+pass+"'").c_str());
		rs->next();
		int i = rs->getInt(1);
		delete rs;
		if (i == 0) {
			std::cout << "Usuario o contraseña incorrectos" << std::endl;
			return false;
		}

		std::cout << "Login correcto!"<<std::endl;
		return true;
	}

	/*
	 * Dado un nombre de usuario te devuelve el id que tiene en la base de datos
	 */
	int GetUserID(std::string user) {
		rs = stmt->executeQuery(("select * from Accounts where username='" + user + "'").c_str());
		rs->next();
		int id = rs->getInt(1);
		delete rs;
		return id;
	}

	/*
	 * Este metodo crea una nueva sesion de un usuario que tu le pases.
	 * Devuelve el id de esta sesion nueva que se ha creado para accederla facilmente más adelante.
	 */
	int StartSession(int idPlayer) {
		stmt->execute(("insert into Sessions(idAcc) values (" + std::to_string(idPlayer) + ")").c_str());

		rs = stmt->executeQuery("select max(idSession) from Sessions");

		rs->next();
		int idSession = rs->getInt(1);
		delete rs;
		return idSession;
	}

	/*
	 * Finaliza la sesion de juego actualizando las partidas que se han jugado y la hora de desconexión
	 */
	void EndSession( int idSession, int games) {
		stmt->execute(("update Sessions set games=" + std::to_string(games) + ", end=CURRENT_TIMESTAMP where idSession=" + std::to_string(idSession)).c_str());
	}
};