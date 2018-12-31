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
#ifndef DBMySQLH
#define DBMySQLH
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ChatCommand;
struct User;
typedef struct st_mysql MYSQL;
typedef struct st_mysql_res MYSQL_RES;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class DBMySQL {
private:
	MYSQL * m_pDBHandle;

#ifdef _WIN32
	HANDLE m_hThreadHandle;
#else
	pthread_t m_threadId;
#endif

	bool m_bConnected, m_bTerminated;

    DBMySQL(const DBMySQL&);
    const DBMySQL& operator=(const DBMySQL&);

	bool SendQueryResults(ChatCommand * pChatCommand, MYSQL_RES * pResult, uint64_t &ui64Rows);
	void ReconnectDb();
public:
    static DBMySQL * m_Ptr;

	DBMySQL();
	~DBMySQL();

	void UpdateRecord(User * pUser);

	bool SearchNick(ChatCommand * pChatCommand);
	bool SearchIP(ChatCommand * pChatCommand);

	void RemoveOldRecords(const uint16_t ui16Days);
	void RunReconnect();
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
