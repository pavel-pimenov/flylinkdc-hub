/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2017  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef DBPostgreSQLH
#define DBPostgreSQLH
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct User;
typedef struct pg_conn PGconn;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class DBPostgreSQL
{
private:
	PGconn * pDBConn;
	
	bool bConnected;
	
	DBPostgreSQL(const DBPostgreSQL&);
	const DBPostgreSQL& operator=(const DBPostgreSQL&);
	
public:
	static DBPostgreSQL * mPtr;
	
	DBPostgreSQL();
	~DBPostgreSQL();
	
	void UpdateRecord(User * pUser);
	
	bool SearchNick(char * sNick, const uint8_t &ui8NickLen, User * pUser, const bool &bFromPM);
	bool SearchIP(char * sIP, User * pUser, const bool &bFromPM);
	
	void RemoveOldRecords(const uint16_t &ui16Days);
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
