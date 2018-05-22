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

	bool Register(std::string user, std::string pass) {
		rs = stmt->executeQuery("select count(*) from Accounts where username='" + user.c_str + "'");
		rs->next();
		int i = rs->getInt(1);
		if (i == 0) {
			std::cout << "USUARIO NO REPETIDO" << std::endl;
		}
		else {
			std::cout << "USUARIO REPETIDO" << std::endl;
		}

		return false;
	}
};