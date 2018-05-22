#include "DBManager.h";

int main() {
	DBManager* dbm = new DBManager();
	dbm->rs = dbm->stmt->executeQuery("select username, password from Accounts");
	//while (dbm->rs->next())
	//{
	//	std::cout << dbm->rs->getString("username").c_str() << " - " <<
	//		dbm->rs->getString("password").c_str() << std::endl;
	//}
	//delete dbm->rs;

	std::string user = "rattaspi";
	dbm->Register(user, "asdf");

	delete dbm;
	system("pause");
	return 0;
}