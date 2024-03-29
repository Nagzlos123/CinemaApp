#include "stdafx.h"
#include <occi.h>
#include <iostream>
#include <fstream>
using namespace oracle::occi;
using namespace std;

class  Project
{
private:
	Environment * env;
	Connection *conn;
	Statement *stmt;
public:
	Project(string user, string passwd, string db)
	{
		env = Environment::createEnvironment(Environment::DEFAULT);
		conn = env->createConnection(user, passwd, db);
	}

	~Project()
	{
		env->terminateConnection(conn);
		Environment::terminateEnvironment(env);
	}

	void wykonajZapytanie()
	{
		try {
			stmt->execute();
		}
		catch (SQLException ex)
		{
			cout << "Blad w zapytaniu do bazy danych" << endl;
			cout << ex.getMessage() << endl;
		}
	}

	void wykonajZapytania(const char * nazwa_pliku)
	{
		fstream plik;
		plik.open(nazwa_pliku, ios::in); // otwiera plik do odczytu
		while (!plik.eof())   
		{
			char zapytanie[5000];
			plik.getline(zapytanie,5000);  // odczytuje linijke pliku z zapytaniem
			zapytanie[strlen(zapytanie)-1] = 0;  // usuwa średnik na koncu zapytania
				
			stmt = conn->createStatement(zapytanie);
			try {
				stmt->executeUpdate();
			}
			catch (SQLException ex)
			{
				cout << "Blad w zapytaniu:" <<  endl << zapytanie << endl;
				cout << ex.getMessage() << endl;
			}
			conn->terminateStatement(stmt);

		}
		plik.close();
	}


	void wyswietlCennik()
	{
		system("cls");
		stmt = conn->createStatement("BEGIN drukuj_cennik(2019); END;");
		wykonajZapytanie();
		conn->terminateStatement(stmt);
		wyswietlWynik();
	}

	void wyswietlSeanse()
	{
		string data;
		system("cls");
		cout << "Podaj date w formacie RR/MM/DD, dla ktorej wyswietlic seanse: ";
		cin >> data;
		stmt = conn->createStatement("BEGIN seanse_w_dniu(:1); END;");
		stmt->setString(1, data);
		wykonajZapytanie();
		conn->terminateStatement(stmt);
		wyswietlWynik();
	}

	void kupBilet()
	{
		wyswietlSeanse();

		ResultSet * rset;
		int id_seansu;
		cout << "\n\nNa ktory seans chcesz kupic bilet?\nPodaj numer seansu (0 - wyjscie do menu): ";
		cin >> id_seansu;
		if (id_seansu == 0) return;
		cout << "\nWybierz rodzaj biletu:\n";

		stmt = conn->createStatement("SELECT * FROM rodzaj_biletu");
		rset = stmt->executeQuery();
		rset->setCharacterStreamMode(2, 20);
		Stream * s=NULL;
		try {
			while (rset->next())
			{
				char rodzaj[20];
				memset(rodzaj, 0, 20);
				int id = rset->getInt(1);
				//string rodzaj = rset->getString(2);
				 s= rset->getStream(2);
				s->readBuffer(rodzaj, 20);
				
				cout << "(" << id << ") " << rodzaj << endl;
			}
			stmt->closeStream(s);
		}
		catch (SQLException ex)
		{
			cout << "Blad w zapytaniu SELECT" << endl;
			cout << ex.getMessage() << endl;
		}
		stmt->closeResultSet(rset);
		conn->terminateStatement(stmt);

		int rodzaj_biletu;
		cout << "Twoj wybor: ";
		cin >> rodzaj_biletu;


		stmt = conn->createStatement("SELECT id FROM cennik WHERE id_dnia_tygodnia=1 AND id_rodzaju_biletu=:1 AND rok=2019");
		stmt->setInt(1, rodzaj_biletu);
		rset = stmt->executeQuery();
		rset->next();
		int id_cennika = rset->getInt(1);
		stmt->closeResultSet(rset);
		conn->terminateStatement(stmt);

		stmt = conn->createStatement("INSERT INTO bilet VALUES (sekwencja_bilet.nextval, :1, :2, NULL, NULL)");
		stmt->setInt(1, id_seansu);
		stmt->setInt(2, id_cennika);
		wykonajZapytanie();
		conn->terminateStatement(stmt);
		wyswietlWynik();

		stmt = conn->createStatement("SELECT sekwencja_bilet.currval FROM dual");
		rset = stmt->executeQuery();
		rset->next();
		int id_biletu = rset->getInt(1);
		stmt->closeResultSet(rset);
		conn->terminateStatement(stmt);


		char wybor;
		do {
			cout << "Czy kupic jeszcze jeden bilet na ten seans? (t/n)";
			cin >> wybor;
			if (wybor == 't')
			{
				stmt = conn->createStatement("BEGIN kopiuj_bilet(:1); END;");
				stmt->setInt(1, id_biletu);
				wykonajZapytanie();
				conn->terminateStatement(stmt);
				wyswietlWynik();
			}
		} while (wybor == 't');
	}

	void wyswietlWynik()
	{
		char line[32767];
		int status = 0;
		while (!status)
		{
			memset(line, 0, 32767);
			stmt = conn->createStatement("declare line varchar2(32767); status int; BEGIN DBMS_OUTPUT.GET_LINE(line=>:1, status=>:2); END;");
			stmt->registerOutParam(1, OCCISTRING, 32767);
			stmt->registerOutParam(2, OCCIINT, 100);
			stmt->setCharacterStreamMode(1, 32767);
			wykonajZapytanie();
			Stream * s = stmt->getStream(1);
			s->readBuffer(line, 32767);
			cout << line << endl;
			stmt->closeStream(s);
			status = stmt->getInt(2);
			conn->terminateStatement(stmt);

		}

	}

};



int main(int argc, char * argv[])
{
	cout << "argc = " << argc << " argv = [";
	for (int i = 0; i < argc; i++)
		cout << argv[i] << ", ";
	cout << "]" << endl;

	if (argc < 3)
	{
		cout << "Usage: " << argv[0] << " user password [database]";
		return 1;
	}
	string user = argv[1];
	string passwd = argv[2];
	string db;
	if (argc < 4)
		db = "";
	else
		db = argv[3];
	try {
		Project *project = new Project(user, passwd, db);
		char command;
		do {
			system("cls");
			cout << "1. Utworz baze danych" << endl;
			cout << "2. Usun baze danych" << endl;
			cout << "3. Cennik biletow" << endl;
			cout << "4. Wyswietl seanse w danym dniu" << endl;
			cout << "5. Kup bilet na film" << endl;
			cout << "0. Zakoncz program" << endl;
			cout << "Twoj wybor: " << endl;
			cin >> command;
			switch (command) {
			case '0': break;
			case '1': 
				project->wykonajZapytania("tabele.sql"); 
				project->wykonajZapytania("sekwencje.sql");
				project->wykonajZapytania("inserty.sql");
				project->wykonajZapytania("procedury.sql");
				project->wykonajZapytania("wyzwalacze.sql");
				break;
			case '2': 
				project->wykonajZapytania("sekwencje_drop.sql"); 
				project->wykonajZapytania("tabele_drop.sql");
				break;
			case '3': project->wyswietlCennik(); break;
			case '4': project->wyswietlSeanse(); break;
			case '5': project->kupBilet(); break;
			}
			cout << "# Wcisnij ENTER #" << endl;
			cin.get();
			cin.get();
		} while (command != '0');
		delete (project);
		cout << "Project executed correctly" << endl;
	}
	catch (SQLException ex)
	{
		cout << "Project executed not correctly" << endl;
		cout << ex.getMessage() << endl;
	}
}