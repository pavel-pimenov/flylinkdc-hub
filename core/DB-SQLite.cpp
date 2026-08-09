/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2022 Petr Kozelka, PPK at PtokaX dot org

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
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "DB-SQLite.h"
#ifdef FLYLINKDC_USE_DB
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "HubCommands.h"
#include "IP2Country.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "TextConverter.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::unique_ptr<DBSQLite> DBSQLite::m_Ptr;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBSQLite::DBSQLite()
{
    m_pSqliteDB = nullptr;
    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_DATABASE)])
    {
        return;
    }

    int iRet = sqlite3_open((ServerManager::m_sPath + "/cfg/users.sqlite").c_str(), &m_pSqliteDB);
    if (iRet != SQLITE_OK)
    {
        m_bConnected = false;
        LogInfo("DBSQLite connection failed: {}", sqlite3_errmsg(m_pSqliteDB));
        sqlite3_close(m_pSqliteDB);

        return;
    }

    char* sErrMsg = nullptr;

    iRet = sqlite3_exec(m_pSqliteDB,
                        "PRAGMA synchronous = NORMAL;\r\n"
                        "PRAGMA journal_mode = WAL;",
                        nullptr,
                        nullptr,
                        &sErrMsg);

    if (iRet != SQLITE_OK)
    {
        m_bConnected = false;
        LogInfo("DBSQLite PRAGMA set failed: {}", sErrMsg);
        sqlite3_free(sErrMsg);
        sqlite3_close(m_pSqliteDB);

        return;
    }

    iRet = sqlite3_exec(m_pSqliteDB,
                        "CREATE TABLE IF NOT EXISTS userinfo ("
                        "nick VARCHAR(64) NOT NULL,"
                        "nick_lower VARCHAR(64) NOT NULL,"
                        "last_updated DATETIME NOT NULL,"
                        "ip_address VARCHAR(39) NOT NULL,"
                        "share VARCHAR(24) NOT NULL,"
                        "description VARCHAR(192),"
                        "tag VARCHAR(192),"
                        "connection VARCHAR(32),"
                        "email VARCHAR(96),"
                        "message_count INTEGER default 0"
                        ");",
                        nullptr,
                        nullptr,
                        &sErrMsg);

    if (iRet != SQLITE_OK)
    {
        m_bConnected = false;
        LogInfo("DBSQLite check/create table failed: {}", sErrMsg);
        sqlite3_free(sErrMsg);
        sqlite3_close(m_pSqliteDB);
        return;
    }
    //
    iRet = sqlite3_exec(m_pSqliteDB, "ALTER TABLE userinfo ADD COLUMN message_count INTEGER default 0;", nullptr, nullptr, &sErrMsg);
    sqlite3_free(sErrMsg);
    sErrMsg = nullptr;
    iRet = sqlite3_exec(m_pSqliteDB, "CREATE INDEX IF NOT EXISTS i_userinfo_message_count ON userinfo(message_count);", nullptr, nullptr, &sErrMsg);
    if (iRet != SQLITE_OK)
    {
        m_bConnected = false;
        LogInfo("DBSQLite CREATE UNIQUE INDEX message_count failed: {}", sErrMsg);
        sqlite3_free(sErrMsg);
        sqlite3_close(m_pSqliteDB);
        return;
    }
    sqlite3_free(sErrMsg);
    sErrMsg = nullptr;

    //
    iRet = sqlite3_exec(m_pSqliteDB, "ALTER TABLE userinfo ADD COLUMN nick_lower VARCHAR(64);", nullptr, nullptr, &sErrMsg);
    if (iRet == SQLITE_OK)
    {
        sqlite3_free(sErrMsg);
        sErrMsg = nullptr;
        iRet = sqlite3_exec(m_pSqliteDB, "update userinfo set nick_lower = LOWER(nick);", nullptr, nullptr, &sErrMsg);
        if (iRet != SQLITE_OK)
        {
            m_bConnected = false;
            LogInfo("DBSQLite update userinfo nick_lower failed: {}", sErrMsg);
            sqlite3_free(sErrMsg);
            sqlite3_close(m_pSqliteDB);
            return;
        }
        sqlite3_free(sErrMsg);
        sErrMsg = nullptr;
    }
    else
    {
        sqlite3_free(sErrMsg);
        sErrMsg = nullptr;
    }
    iRet = sqlite3_exec(m_pSqliteDB, "CREATE UNIQUE INDEX IF NOT EXISTS iu_userinfo_nick ON userinfo(nick_lower);", nullptr, nullptr, &sErrMsg);
    if (iRet != SQLITE_OK)
    {
        m_bConnected = false;
        LogInfo("DBSQLite CREATE UNIQUE INDEX nick_lower failed: {}", sErrMsg);
        sqlite3_free(sErrMsg);
        sqlite3_close(m_pSqliteDB);
        return;
    }
    sqlite3_free(sErrMsg);
    sErrMsg = nullptr;

    m_bConnected = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBSQLite::~DBSQLite()
{
#ifdef FLYLINKDC_USE_SQLITE_REMOVE_OLD_RECORD
    // When user don't want to save data in database forever then he can set to remove records older than X days.
    if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DB_REMOVE_OLD_RECORDS)] != 0)
    {
        RemoveOldRecords(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DB_REMOVE_OLD_RECORDS)]);
    }
#endif // FLYLINKDC_USE_SQLITE_REMOVE_OLD_RECORD
    if (m_bConnected)
    {
        sqlite3_close(m_pSqliteDB);
    }

    sqlite3_shutdown();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool DBSQLite::SqlExec(const char* sql, const char* label, int (*callback)(void*, int, char**, char**)) const
{
    char* sErrMsg = nullptr;
    const int iRet = sqlite3_exec(m_pSqliteDB, sql, callback, nullptr, &sErrMsg);
    if (iRet != SQLITE_OK)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite %s failed: %s", label, sErrMsg);
        sqlite3_free(sErrMsg);
        return false;
    }
    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void DBSQLite::IncMessageCount(User* pUser)
{
    if (!m_bConnected)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[WARN] DBSQLite::IncMessageCount skipped — database not connected.");
        return;
    }

    std::string sNick(65, '\0');
    const size_t szNickLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sNick.c_str(), pUser->m_sNick.size(), sNick.data(), 65);
    if (szNickLen == 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[WARN] DBSQLite::IncMessageCount skipped — nick conversion failed for %s.", pUser->m_sNick.c_str());
        return;
    }
    sNick.resize(szNickLen);
    std::string sSQLCommand;
    sSQLCommand.resize(1024);
    sqlite3_snprintf(static_cast<int>(sSQLCommand.size()),
                     sSQLCommand.data(),
                     "UPDATE userinfo SET message_count = message_count+1 WHERE nick_lower = LOWER(%Q);",
                     sNick.c_str());
    sSQLCommand.resize(strlen(sSQLCommand.data()));

    if (!SqlExec(sSQLCommand.c_str(), "update record [IncMessageCount]"))
    {
        LogDbgErr("[DB] Failed to increment message count for {}", sNick);
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Now that important part. Function to update or insert user to database.
void DBSQLite::UpdateRecord(User* pUser)
{
    if (!m_bConnected)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[WARN] DBSQLite::UpdateRecord skipped — database not connected.");
        return;
    }

    std::string sNick(65, '\0');
    const size_t szNickLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sNick.c_str(), pUser->m_sNick.size(), sNick.data(), 65);
    if (szNickLen == 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[WARN] DBSQLite::UpdateRecord skipped — nick conversion failed for %s.", pUser->m_sNick.c_str());
        return;
    }
    sNick.resize(szNickLen);

    std::array<char, 24> sShare = {};
    if (snprintf(sShare.data(), sShare.size(), "%0.02f GB", static_cast<double>(pUser->m_ui64SharedSize) / 1073741824) <= 0)
    {
        LogDbgWarn("[WARN] DBSQLite::UpdateRecord skipped — snprintf share failed for user {}.", pUser->m_sNick.c_str());
        return;
    }

    std::string sDescription(193, '\0');

    if (!pUser->m_sDescription.empty())
    {
        const size_t szDescLen = TextConverter::m_Ptr->CheckUtf8AndConvert(
            pUser->m_sDescription.data(), static_cast<uint8_t>(pUser->m_sDescription.size()), sDescription.data(), 193);
        sDescription.resize(szDescLen);
    }
    else
    {
        sDescription.clear();
    }

    std::string sTag(193, '\0');

    if (!pUser->m_sTag.empty())
    {
        const size_t szTagLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sTag.data(), static_cast<uint8_t>(pUser->m_sTag.size()), sTag.data(), 193);
        sTag.resize(szTagLen);
    }
    else
    {
        sTag.clear();
    }

    std::array<char, 33> sConnection = {};

    if (!pUser->m_sConnection.empty())
    {
        static_cast<void>(TextConverter::m_Ptr->CheckUtf8AndConvert(
            pUser->m_sConnection.data(), static_cast<uint8_t>(pUser->m_sConnection.size()), sConnection.data(), sConnection.size()));
    }

    std::string sEmail(97, '\0');

    if (!pUser->m_sEmail.empty())
    {
        const size_t szEmailLen =
            TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sEmail.data(), static_cast<uint8_t>(pUser->m_sEmail.size()), sEmail.data(), 97);
        sEmail.resize(szEmailLen);
    }
    else
    {
        sEmail.clear();
    }

    std::string sSQLCommand;
    sSQLCommand.resize(1024);
    sqlite3_snprintf(static_cast<int>(sSQLCommand.size()),
                     sSQLCommand.data(),
                     "UPDATE userinfo SET "
                     "nick = %Q,"
                     "last_updated = DATETIME('now')," // last_updated
                     "ip_address = %Q,"                // ip
                     "share = %Q,"                     // share
                     "description = %Q,"               // description
                     "tag = %Q,"                       // tag
                     "connection = %Q,"                // connection
                     "email = %Q"                      // email
                     "WHERE nick_lower = LOWER(%Q);",  // nick
                     sNick.c_str(),
                     pUser->m_sIP.data(),
                     sShare.data(),
                     sDescription.c_str(),
                     sTag.c_str(),
                     sConnection.data(),
                     sEmail.c_str(),
                     sNick.c_str());
    sSQLCommand.resize(strlen(sSQLCommand.data()));

    if (!SqlExec(sSQLCommand.c_str(), "update record"))
    {
        LogDbgErr("[DB] Failed to update record for {}", sNick);
    }

    const int iRet = sqlite3_changes(m_pSqliteDB);
    if (iRet != 0)
    {
        return;
    }

    sqlite3_snprintf(static_cast<int>(sSQLCommand.size()),
                     sSQLCommand.data(),
                     "INSERT INTO userinfo (nick, nick_lower, last_updated, ip_address, share, description, tag, connection, email) VALUES ("
                     "%Q,"              // nick
                     "LOWER(%Q),"       // nick
                     "DATETIME('now')," // last_updated
                     "%Q,"              // ip
                     "%Q,"              // share
                     "%Q,"              // description
                     "%Q,"              // tag
                     "%Q,"              // connection
                     "%Q"               // email
                     ");",
                     sNick.c_str(),
                     sNick.c_str(),
                     pUser->m_sIP.data(),
                     sShare.data(),
                     sDescription.c_str(),
                     sTag.c_str(),
                     sConnection.data(),
                     sEmail.c_str());
    sSQLCommand.resize(strlen(sSQLCommand.data()));

    if (!SqlExec(sSQLCommand.c_str(), "insert record"))
    {
        LogDbgErr("[DB] Failed to insert record for {}", sNick);
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace {
bool g_bFirst = true;
bool g_bSecond = false;
std::array<char, 65> g_sFirstNick{};
std::array<char, 40> g_sFirstIP{};
int g_iMsgLen = 0;
int g_iAfterHubSecMsgLen = 0;

int SelectCallBack(void*, int iArgCount, char** ppArgSTrings, char**)
{
    if (iArgCount != 8)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite SelectCallBack wrong iArgCount: %d", iArgCount);
        return 0;
    }

    if (g_bFirst)
    {
        g_bFirst = false;
        g_bSecond = true;

        size_t szLength = strlen(ppArgSTrings[0]);
        if (szLength == 0 || szLength > 64)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: %zu", szLength);
            return 0;
        }

        std::copy_n(ppArgSTrings[0], szLength, g_sFirstNick.begin());
        g_sFirstNick[szLength] = '\0';

        szLength = strlen(ppArgSTrings[2]);
        if (szLength > 39)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid first IP length: %zu", szLength);
            return 0;
        }

        std::copy_n(ppArgSTrings[2], szLength, g_sFirstIP.begin());
        g_sFirstIP[szLength] = '\0';

        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            g_iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                            ppArgSTrings[0]))
        {
            return 0;
        }

        RegUser* pReg = RegManager::m_Ptr->Find(std::string_view(ppArgSTrings[0], szLength));
        if (pReg)
        {
            if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                g_iMsgLen,
                                ServerManager::m_szGlobalBufferSize,
                                "\n%s: %s",
                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILE)].c_str(),
                                ProfileManager::m_Ptr->m_vpProfilesTable[pReg->m_ui16Profile]->m_sName.c_str()))
            {
                return 0;
            }
        }

        // In case when SQL wildcards were used is possible that user is online. Then we don't use data from database, but data that are in server memory.
        User* pOnlineUser = HashManager::m_Ptr->FindUser(std::string_view(ppArgSTrings[0], szLength));
        if (pOnlineUser)
        {
            if (!BuildUserOnlineInfo(g_iMsgLen, pOnlineUser))
            {
                return 0;
            }

            return 0;
        }
        // User is offline, then we use data from database.
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            g_iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s ",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STATUS)].c_str(),
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_OFFLINE_FROM)].c_str()))
        {
            return 0;
        }
        const auto tTime = static_cast<time_t>(strtoull(ppArgSTrings[1], nullptr, 10));
        const struct tm* tm = localtime(&tTime);
        const int iStrftimeRet2 = static_cast<int>(strftime(ServerManager::m_pGlobalBuffer + g_iMsgLen, ServerManager::m_szGlobalBufferSize - g_iMsgLen, "%c", tm));
        if (iStrftimeRet2 <= 0)
        {
            return 0;
        }
        g_iMsgLen += iStrftimeRet2;

        szLength = strlen(ppArgSTrings[2]);
        if (szLength == 0 || szLength > 39)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: %zu", szLength);
            return 0;
        }

        szLength = strlen(ppArgSTrings[3]);
        if (szLength == 0 || szLength > 24)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid share length: %zu", szLength);
            return 0;
        }

        char* sIP = ppArgSTrings[2];

        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            g_iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s\n%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                            sIP,
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SHARE_SIZE)].c_str(),
                            ppArgSTrings[3]))
        {
            return 0;
        }

        szLength = strlen(ppArgSTrings[4]);
        if (szLength != 0)
        {
            if (szLength > 192)
            {
                UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid description length: %zu", szLength);
                return 0;
            }

            if (!AppendLabeledField(g_iMsgLen, std::to_underlying(LangIds::LAN_DESCRIPTION), ppArgSTrings[4], szLength))
            {
                return 0;
            }
        }

        szLength = strlen(ppArgSTrings[5]);
        if (szLength != 0)
        {
            if (szLength > 192)
            {
                UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid tag length: %zu", szLength);
                return 0;
            }

            if (!AppendLabeledField(g_iMsgLen, std::to_underlying(LangIds::LAN_TAG), ppArgSTrings[5], szLength))
            {
                return 0;
            }
        }

        szLength = strlen(ppArgSTrings[6]);
        if (szLength != 0)
        {
            if (szLength > 32)
            {
                UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid connection length: %zu", szLength);
                return 0;
            }

            if (!AppendLabeledField(g_iMsgLen, std::to_underlying(LangIds::LAN_CONNECTION), ppArgSTrings[6], szLength))
            {
                return 0;
            }
        }

        szLength = strlen(ppArgSTrings[7]);
        if (szLength != 0)
        {
            if (szLength > 96)
            {
                UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid email length: %zu", szLength);
                return 0;
            }

            if (!AppendLabeledField(g_iMsgLen, std::to_underlying(LangIds::LAN_EMAIL), ppArgSTrings[7], szLength))
            {
                return 0;
            }
        }

        std::array<uint8_t, 16> ui128IPHash = {};

        if (IpP2Country::m_Ptr->m_ui32Count != 0 && HashIP(sIP, ui128IPHash.data()))
        {
            if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                g_iMsgLen,
                                ServerManager::m_szGlobalBufferSize,
                                "\n%s: ",
                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_COUNTRY)].c_str()))
            {
                return 0;
            }

            memcpy(ServerManager::m_pGlobalBuffer + g_iMsgLen, IpP2Country::m_Ptr->Find(ui128IPHash.data(), false), 2);
            g_iMsgLen += 2;
        }

        return 0;
    }
    else if (g_bSecond)
    {
        g_bSecond = false;

        size_t szLength = strlen(g_sFirstNick.data());
        if (szLength == 0)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: %zu", szLength);
            return 0;
        }

        szLength = strlen(g_sFirstIP.data());
        if (szLength == 0)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: %zu", szLength);
            return 0;
        }

        int iAfterHubSecMsgLen = g_iAfterHubSecMsgLen;
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iAfterHubSecMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s\t\t%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                            g_sFirstNick.data(),
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                            g_sFirstIP.data()))
        {
            return 0;
        }
        g_iMsgLen = iAfterHubSecMsgLen;
    }

    size_t szLength = strlen(ppArgSTrings[0]);
    if (szLength == 0 || szLength > 64)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: %zu", szLength);
        return 0;
    }

    szLength = strlen(ppArgSTrings[2]);
    if (szLength == 0 || szLength > 39)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: %zu", szLength);
        return 0;
    }

    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                        g_iMsgLen,
                        ServerManager::m_szGlobalBufferSize,
                        "\n%s: %s\t\t%s: %s",
                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                        ppArgSTrings[0],
                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                        ppArgSTrings[2]))
    {
        return 0;
    }

    return 0;
}
} // namespace
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// First of two functions to search data in database. Nick will be probably most used.
bool DBSQLite::SearchNick(ChatCommand* pChatCommand)
{
    if (!m_bConnected)
    {
        return false;
    }

    std::string sUtfNick(65, '\0');
    const size_t szUtfNickLen =
        TextConverter::m_Ptr->CheckUtf8AndConvert(pChatCommand->m_sCommand, static_cast<uint8_t>(pChatCommand->m_ui32CommandLen), sUtfNick.data(), 65);
    if (szUtfNickLen == 0)
    {
        return false;
    }
    sUtfNick.resize(szUtfNickLen);

    if (pChatCommand->m_bFromPM)
    {
        g_iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                             ServerManager::m_szGlobalBufferSize,
                             "$To: %s From: %s $<%s> ",
                             pChatCommand->m_pUser->m_sNick.c_str(),
                             SettingManager::HubSec(),
                             SettingManager::HubSec());
    }
    else
    {
        g_iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", SettingManager::HubSec());
    }

    if (g_iMsgLen <= 0)
    {
        return false;
    }

    g_iAfterHubSecMsgLen = g_iMsgLen;

    g_bFirst = true;
    g_bSecond = false;

    std::string sSQLCommand;
    sSQLCommand.resize(256);
    sqlite3_snprintf(256,
                     sSQLCommand.data(),
                     "SELECT nick, %s, ip_address, share, description, tag, connection, email FROM userinfo WHERE nick_lower LIKE LOWER(%Q) ORDER BY "
                     "last_updated DESC LIMIT 50;",
                     "strftime('%s', last_updated)",
                     sUtfNick.c_str());
    sSQLCommand.resize(strlen(sSQLCommand.data()));

    if (!SqlExec(sSQLCommand.c_str(), "search for nick", SelectCallBack))
    {
        return false;
    }

    if (g_iMsgLen == g_iAfterHubSecMsgLen)
    {
        return false;
    }

    ServerManager::m_pGlobalBuffer[g_iMsgLen] = '|';
    ServerManager::m_pGlobalBuffer[g_iMsgLen + 1] = '\0';

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, g_iMsgLen + 1);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Second of two fnctions to search data in database. Now using IP.
bool DBSQLite::SearchIP(ChatCommand* pChatCommand)
{
    if (!m_bConnected)
    {
        return false;
    }

    if (pChatCommand->m_bFromPM)
    {
        g_iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                             ServerManager::m_szGlobalBufferSize,
                             "$To: %s From: %s $<%s> ",
                             pChatCommand->m_pUser->m_sNick.c_str(),
                             SettingManager::HubSec(),
                             SettingManager::HubSec());
    }
    else
    {
        g_iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", SettingManager::HubSec());
    }

    if (g_iMsgLen <= 0)
    {
        return false;
    }

    g_iAfterHubSecMsgLen = g_iMsgLen;

    g_bFirst = true;
    g_bSecond = false;

    std::string sSQLCommand;
    sSQLCommand.resize(256);
    sqlite3_snprintf(
        256,
        sSQLCommand.data(),
        "SELECT nick, %s, ip_address, share, description, tag, connection, email FROM userinfo WHERE ip_address LIKE %Q ORDER BY last_updated DESC LIMIT 50;",
        "strftime('%s', last_updated)",
        pChatCommand->m_sCommand);
    sSQLCommand.resize(strlen(sSQLCommand.data()));

    if (!SqlExec(sSQLCommand.c_str(), "search for ip", SelectCallBack))
    {
        return false;
    }

    if (g_iMsgLen == g_iAfterHubSecMsgLen)
    {
        return false;
    }

    ServerManager::m_pGlobalBuffer[g_iMsgLen] = '|';
    ServerManager::m_pGlobalBuffer[g_iMsgLen + 1] = '\0';

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, g_iMsgLen + 1);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef FLYLINKDC_USE_SQLITE_REMOVE_OLD_RECORD
// Function to remove X days old records from database.
void DBSQLite::RemoveOldRecords(const uint16_t ui16Days)
{
    if (!m_bConnected)
    {
        return;
    }

    std::string sSQLCommand;
    sSQLCommand.resize(256);
    const int iSQLLen =
        snprintf(sSQLCommand.data(), sSQLCommand.size(), "DELETE FROM userinfo WHERE last_updated < DATETIME('now', '-%hu days', 'localtime');", ui16Days);
    if (iSQLLen <= 0)
    {
        LogDbg("[WARN] DBSQLite::RemoveOldRecords skipped — snprintf failed.");
        return;
    }
    sSQLCommand.resize(static_cast<size_t>(iSQLLen));

    SqlExec(sSQLCommand.c_str(), "remove old records");

    const int iRet = sqlite3_changes(m_pSqliteDB);
    if (iRet != 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite removed old records: %d", iRet);
    }
}
#endif // FLYLINKDC_USE_SQLITE_REMOVE_OLD_RECORD
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // FLYLINKDC_USE_DB
