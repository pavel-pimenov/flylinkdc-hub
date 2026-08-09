/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2022  Petr Kozelka, PPK at PtokaX dot org

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
#ifndef DBSQLiteH
#define DBSQLiteH

#include <sqlite3.h>

#ifdef FLYLINKDC_USE_DB

#include <memory>

#include <sqlite3.h>

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct ChatCommand;
struct User;
using sqlite3 = struct sqlite3;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class DBSQLite
{
private:
    sqlite3* m_pSqliteDB;

    bool m_bConnected = false;

    [[nodiscard]] auto SqlExec(const char* sql, const char* label, int (*callback)(void*, int, char**, char**) = nullptr) const -> bool;

public:
    static std::unique_ptr<DBSQLite> m_Ptr;

    DBSQLite();
    ~DBSQLite();

    void UpdateRecord(User* pUser);
    void IncMessageCount(User* pUser);

    [[nodiscard]] auto SearchNick(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] auto SearchIP(ChatCommand* pChatCommand) -> bool;

#ifdef FLYLINKDC_USE_SQLITE_REMOVE_OLD_RECORD
    void RemoveOldRecords(const uint16_t ui16Days);
#endif
    DBSQLite(const DBSQLite&) = delete;
    auto operator=(const DBSQLite&) -> DBSQLite& = delete;
};
#endif // FLYLINKDC_USE_DB
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
