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

	//dbm->Register("prueba1", "passwordPrueba");
	//dbm->Login("Rattaspi", "123");
	//std::cout << dbm->GetUserID("devildrake") << std::endl;
	//dbm->StartSession(1);
	//dbm->EndSession(1, 10);

	delete dbm;
	system("pause");
	return 0;
}