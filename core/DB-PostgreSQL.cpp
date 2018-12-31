/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2015 Petr Kozelka, PPK at PtokaX dot org

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
#include "DB-PostgreSQL.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "hashRegManager.h"
#include "hashUsrManager.h"
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
#include <libpq-fe.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
DBPostgreSQL * DBPostgreSQL::mPtr = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBPostgreSQL::DBPostgreSQL()
{
	if (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_DATABASE] == false || clsSettingManager::mPtr->sTexts[SETTXT_POSTGRES_PASS] == NULL)
	{
		bConnected = false;
		
		return;
	}
	
	// Connect to PostgreSQL with our settings.
	const char * sKeyWords[] = { "host", "port", "dbname", "user", "password", NULL };
	const char * sValues[] = { clsSettingManager::mPtr->sTexts[SETTXT_POSTGRES_HOST], clsSettingManager::mPtr->sTexts[SETTXT_POSTGRES_PORT], clsSettingManager::mPtr->sTexts[SETTXT_POSTGRES_DBNAME], clsSettingManager::mPtr->sTexts[SETTXT_POSTGRES_USER], clsSettingManager::mPtr->sTexts[SETTXT_POSTGRES_PASS], NULL };
	
	pDBConn = PQconnectdbParams(sKeyWords, sValues, 0);
	
	if (PQstatus(pDBConn) == CONNECTION_BAD)  // Connection to PostgreSQL failed. Save error to log and give up.
	{
		bConnected = false;
		AppendLog(string("DBPostgreSQL connection failed: ") + PQerrorMessage(pDBConn));
		PQfinish(pDBConn);
		
		return;
	}
	
	/*  Connection to PostgreSQL was created.
	    Now we check if our table exist in database.
	    If not then we create table.
	    Then we created unique index for column nick based on lower(nick). That will make it case insensitive. */
	PGresult * pResult = PQexec(pDBConn,
	                            "DO $$\n"
	                            "BEGIN\n"
	                            "IF NOT EXISTS ("
	                            "SELECT * FROM pg_catalog.pg_tables WHERE tableowner = 'ptokax' AND tablename = 'userinfo'"
	                            ") THEN\n"
	                            "CREATE TABLE userinfo ("
	                            "nick VARCHAR(64) NOT NULL,"
	                            "last_updated TIMESTAMPTZ NOT NULL,"
	                            "ip_address VARCHAR(39) NOT NULL,"
	                            "share VARCHAR(24) NOT NULL,"
	                            "description VARCHAR(192),"
	                            "tag VARCHAR(192),"
	                            "connection VARCHAR(32),"
	                            "email VARCHAR(96)"
	                            ");"
	                            "CREATE UNIQUE INDEX nick_unique ON userinfo(LOWER(nick));"
	                            "END IF;"
	                            "END;"
	                            "$$;"
	                           );
	                           
	if (PQresultStatus(pResult) != PGRES_COMMAND_OK)  // Table exist or was succesfully created.
	{
		bConnected = false;
		AppendLog(string("DBPostgreSQL check/create table failed: ") + PQresultErrorMessage(pResult));
		PQclear(pResult);
		PQfinish(pDBConn);
		
		return;
	}
	
	PQclear(pResult);
	
	bConnected = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBPostgreSQL::~DBPostgreSQL()
{
	// When user don't want to save data in database forever then he can set to remove records older than X days.
	if (clsSettingManager::mPtr->i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS] != 0)
	{
		RemoveOldRecords(clsSettingManager::mPtr->i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS]);
	}
	
	if (bConnected == true)
	{
		PQfinish(pDBConn);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Now that important part. Function to update or insert user to database.
void DBPostgreSQL::UpdateRecord(User * pUser)
{
	if (bConnected == false)
	{
		return;
	}
	
	/*  Prepare params for PQexecParams.
	    With this function params are separated from command string and we don't need to do annoying quoting and escaping for them.
	    Of course we do conversion to utf-8 when needed. */
	char * paramValues[7];
	memset(&paramValues, 0, sizeof(paramValues));
	
	
	char sNick[65];
	paramValues[0] = sNick;
	if (TextConverter::mPtr->CheckUtf8AndConvert(pUser->sNick, pUser->ui8NickLen, sNick, 65) == 0)
	{
		return;
	}
	
	paramValues[1] = pUser->sIP;
	
	char sShare[24];
	sprintf(sShare, "%0.02f GB", (double)pUser->ui64SharedSize / 1073741824);
	
	paramValues[2] = sShare;
	
	char sDescription[193];
	if (pUser->sDescription != NULL && TextConverter::mPtr->CheckUtf8AndConvert(pUser->sDescription, pUser->ui8DescriptionLen, sDescription, 193) != 0)
	{
		paramValues[3] = sDescription;
	}
	
	char sTag[193];
	if (pUser->sTag != NULL && TextConverter::mPtr->CheckUtf8AndConvert(pUser->sTag, pUser->ui8TagLen, sTag, 193) != 0)
	{
		paramValues[4] = sTag;
	}
	
	char sConnection[33];
	if (pUser->sConnection != NULL && TextConverter::mPtr->CheckUtf8AndConvert(pUser->sConnection, pUser->ui8ConnectionLen, sConnection, 33) != 0)
	{
		paramValues[5] = sConnection;
	}
	
	char sEmail[97];
	if (pUser->sEmail != NULL && TextConverter::mPtr->CheckUtf8AndConvert(pUser->sEmail, pUser->ui8EmailLen, sEmail, 97) != 0)
	{
		paramValues[6] = sEmail;
	}
	
	/*  PostgreSQL don't support UPSERT.
	    So we need to do it hard way.
	    First try UPDATE.
	    Timestamp is truncated to seconds. */
	PGresult * pResult = PQexecParams(pDBConn,
	                                  "UPDATE userinfo SET "
	                                  "last_updated = DATE_TRUNC('second', NOW())," // last_updated
	                                  "ip_address = $2," // ip
	                                  "share = $3," // share
	                                  "description = $4," // description
	                                  "tag = $5," // tag
	                                  "connection = $6," // connection
	                                  "email = $7" // email
	                                  "WHERE LOWER(nick) = LOWER($1);", // nick
	                                  7,
	                                  NULL,
	                                  paramValues,
	                                  NULL,
	                                  NULL,
	                                  0
	                                 );
	                                 
	if (PQresultStatus(pResult) != PGRES_COMMAND_OK)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL update record failed: %s", PQresultErrorMessage(pResult));
	}
	
	char * sRows = PQcmdTuples(pResult);
	
	/*  UPDATE should always return OK, but that not mean anything was changed.
	    We need to check how much rows was changed.
	    When 0 then it means that user not exist in table. */
	if (sRows[0] != '0' || sRows[1] != '\0')
	{
		PQclear(pResult);
		return;
	}
	
	PQclear(pResult);
	
	// When update changed 0 rows then we need to insert user to table.
	pResult = PQexecParams(pDBConn,
	                       "INSERT INTO userinfo (nick, last_updated, ip_address, share, description, tag, connection, email) VALUES ("
	                       "$1," // nick
	                       "DATE_TRUNC('second', NOW())," // last_updated
	                       "$2," // ip
	                       "$3," // share
	                       "$4," // description
	                       "$5," // tag
	                       "$6," // connection
	                       "$7" // email
	                       ");",
	                       7,
	                       NULL,
	                       paramValues,
	                       NULL,
	                       NULL,
	                       0
	                      );
	                      
	if (PQresultStatus(pResult) != PGRES_COMMAND_OK)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL insert record failed: %s", PQresultErrorMessage(pResult));
	}
	
	PQclear(pResult);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to send data returned by SELECT to Operator who requested them.
bool SendQueryResults(User * pUser, const bool &bFromPM, PGresult * pResult, int &iTuples)
{
	if (iTuples == 1)  // When we have only one result then we send whole info about user.
	{
		int iMsgLen = 0;
		
		if (bFromPM == true)
		{
			iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$To: %s From: %s $", pUser->sNick, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
			if (CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults1") == false)
			{
				return false;
			}
		}
		
		int iLength = PQgetlength(pResult, 0, 0);
		if (iLength <= 0 || iLength > 64)
		{
			clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid nick length: %d", iLength);
			return false;
		}
		
		char * sValue = PQgetvalue(pResult, 0, 0);
		int iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "<%s> \n%s: %s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NICK], sValue);
		iMsgLen += iRet;
		if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults2") == false)
		{
			return false;
		}
		
		RegUser * pReg = clsRegManager::mPtr->Find(sValue, iLength);
		if (pReg != NULL)
		{
			iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_PROFILE], clsProfileManager::mPtr->ppProfilesTable[pReg->ui16Profile]->sName);
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults3") == false)
			{
				return false;
			}
		}
		
		// In case when SQL wildcards were used is possible that user is online. Then we don't use data from database, but data that are in server memory.
		User * pOnlineUser = clsHashManager::mPtr->FindUser(sValue, iLength);
		if (pOnlineUser != NULL)
		{
			iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s ", clsLanguageManager::mPtr->sTexts[LAN_STATUS], clsLanguageManager::mPtr->sTexts[LAN_ONLINE_FROM]);
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults4") == false)
			{
				return false;
			}
			
			struct tm *tm = localtime(&pOnlineUser->tLoginTime);
			iRet = (int)strftime(clsServerManager::pGlobalBuffer + iMsgLen, 256, "%c", tm);
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults5") == false)
			{
				return false;
			}
			
			if (pOnlineUser->sIPv4[0] != '\0')
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s / %s\n%s: %0.02f %s", clsLanguageManager::mPtr->sTexts[LAN_IP], pOnlineUser->sIP, pOnlineUser->sIPv4, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->ui64SharedSize / 1073741824, clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults6") == false)
				{
					return false;
				}
			}
			else
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s\n%s: %0.02f %s", clsLanguageManager::mPtr->sTexts[LAN_IP], pOnlineUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->ui64SharedSize / 1073741824, clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults7") == false)
				{
					return false;
				}
			}
			
			if (pOnlineUser->sDescription != NULL)
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults8") == false)
				{
					return false;
				}
				memcpy(clsServerManager::pGlobalBuffer + iMsgLen, pOnlineUser->sDescription, pOnlineUser->ui8DescriptionLen);
				iMsgLen += (int)pOnlineUser->ui8DescriptionLen;
			}
			
			if (pOnlineUser->sTag != NULL)
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_TAG]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults9") == false)
				{
					return false;
				}
				memcpy(clsServerManager::pGlobalBuffer + iMsgLen, pOnlineUser->sTag, pOnlineUser->ui8TagLen);
				iMsgLen += (int)pOnlineUser->ui8TagLen;
			}
			
			if (pOnlineUser->sConnection != NULL)
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_CONNECTION]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults10") == false)
				{
					return false;
				}
				memcpy(clsServerManager::pGlobalBuffer + iMsgLen, pOnlineUser->sConnection, pOnlineUser->ui8ConnectionLen);
				iMsgLen += (int)pOnlineUser->ui8ConnectionLen;
			}
			
			if (pOnlineUser->sEmail != NULL)
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_EMAIL]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults11") == false)
				{
					return false;
				}
				memcpy(clsServerManager::pGlobalBuffer + iMsgLen, pOnlineUser->sEmail, pOnlineUser->ui8EmailLen);
				iMsgLen += (int)pOnlineUser->ui8EmailLen;
			}
			
			if (clsIpP2Country::mPtr->ui32Count != 0)
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_COUNTRY]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults12") == false)
				{
					return false;
				}
				memcpy(clsServerManager::pGlobalBuffer + iMsgLen, clsIpP2Country::mPtr->GetCountry(pOnlineUser->ui8Country, false), 2);
				iMsgLen += 2;
			}
			
			clsServerManager::pGlobalBuffer[iMsgLen] = '|';
			clsServerManager::pGlobalBuffer[iMsgLen + 1] = '\0';
			pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen + 1);
			
			return true;
		}
		else     // User is offline, then we use data from database.
		{
			iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s ", clsLanguageManager::mPtr->sTexts[LAN_STATUS], clsLanguageManager::mPtr->sTexts[LAN_OFFLINE_FROM]);
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults13") == false)
			{
				return false;
			}
#ifdef _WIN32
			time_t tTime = (time_t)_strtoui64(PQgetvalue(pResult, 0, 1), NULL, 10);
#else
			time_t tTime = (time_t)strtoull(PQgetvalue(pResult, 0, 1), NULL, 10);
#endif
			struct tm *tm = localtime(&tTime);
			iRet = (int)strftime(clsServerManager::pGlobalBuffer + iMsgLen, 256, "%c", tm);
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults14") == false)
			{
				return false;
			}
			
			iLength = PQgetlength(pResult, 0, 2);
			if (iLength <= 0 || iLength > 39)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid ip length: %d", iLength);
				return false;
			}
			
			iLength = PQgetlength(pResult, 0, 3);
			if (iLength <= 0 || iLength > 24)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid share length: %d", iLength);
				return false;
			}
			
			char * sIP = PQgetvalue(pResult, 0, 2);
			
			iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_IP], sIP, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], PQgetvalue(pResult, 0, 3));
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, 1024, "SendQueryResults15") == false)
			{
				return false;
			}
			
			if (PQgetisnull(pResult, 0, 4) == 0)
			{
				iLength = PQgetlength(pResult, 0, 4);
				if (iLength > 192)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid description length: %d", iLength);
					return  false;
				}
				
				if (iLength > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION], PQgetvalue(pResult, 0, 4));
					iMsgLen += iRet;
					if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults16") == false)
					{
						return false;
					}
				}
			}
			
			if (PQgetisnull(pResult, 0, 5) == 0)
			{
				iLength = PQgetlength(pResult, 0, 5);
				if (iLength > 192)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid tag length: %d", iLength);
					return false;
				}
				
				if (iLength > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_TAG], PQgetvalue(pResult, 0, 5));
					iMsgLen += iRet;
					if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults17") == false)
					{
						return false;
					}
				}
			}
			
			if (PQgetisnull(pResult, 0, 6) == 0)
			{
				iLength = PQgetlength(pResult, 0, 6);
				if (iLength > 32)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid connection length: %d", iLength);
					return false;
				}
				
				if (iLength > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_CONNECTION], PQgetvalue(pResult, 0, 6));
					iMsgLen += iRet;
					if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults18") == false)
					{
						return false;
					}
				}
			}
			
			if (PQgetisnull(pResult, 0, 7) == 0)
			{
				iLength = PQgetlength(pResult, 0, 7);
				if (iLength > 96)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid email length: %d", iLength);
					return false;
				}
				
				if (iLength > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_EMAIL], PQgetvalue(pResult, 0, 7));
					iMsgLen += iRet;
					if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults19") == false)
					{
						return false;
					}
				}
			}
			
			uint8_t ui128IPHash[16];
			memset(ui128IPHash, 0, 16);
			
			if (clsIpP2Country::mPtr->ui32Count != 0 && HashIP(sIP, ui128IPHash) == true)
			{
				iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: ", clsLanguageManager::mPtr->sTexts[LAN_COUNTRY]);
				iMsgLen += iRet;
				if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults20") == false)
				{
					return false;
				}
				
				memcpy(clsServerManager::pGlobalBuffer + iMsgLen, clsIpP2Country::mPtr->Find(ui128IPHash, false), 2);
				iMsgLen += 2;
			}
			
			clsServerManager::pGlobalBuffer[iMsgLen] = '|';
			clsServerManager::pGlobalBuffer[iMsgLen + 1] = '\0';
			pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen + 1);
			
			return true;
		}
	}
	else     // When we have more than 1 result from database then we send only short info with user nick and ip address.
	{
		int iMsgLen = 0;
		
		if (bFromPM == true)
		{
			iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$To: %s From: %s $<%s> ", pUser->sNick, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC],
			                  clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
			if (CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults21") == false)
			{
				return false;
			}
		}
		else
		{
			iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "<%s> ", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
			if (CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults22") == false)
			{
				return false;
			}
		}
		
		for (int iActualTuple = 0; iActualTuple < iTuples; iActualTuple++)
		{
			int iLength = PQgetlength(pResult, iActualTuple, 0);
			if (iLength <= 0 || iLength > 64)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid nick length: %d", iLength);
				return false;
			}
			
			iLength = PQgetlength(pResult, iActualTuple, 2);
			if (iLength <= 0 || iLength > 39)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid ip length: %d", iLength);
				return false;
			}
			
			int iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s\t\t%s: %s", clsLanguageManager::mPtr->sTexts[LAN_NICK], PQgetvalue(pResult, iActualTuple, 0), clsLanguageManager::mPtr->sTexts[LAN_IP], PQgetvalue(pResult, iActualTuple, 2));
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults23") == false)
			{
				return false;
			}
		}
		
		clsServerManager::pGlobalBuffer[iMsgLen] = '|';
		clsServerManager::pGlobalBuffer[iMsgLen + 1] = '\0';
		pUser->SendCharDelayed(clsServerManager::pGlobalBuffer, iMsgLen + 1);
		
		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// First of two functions to search data in database. Nick will be probably most used.
bool DBPostgreSQL::SearchNick(char * sNick, const uint8_t &ui8NickLen, User * pUser, const bool &bFromPM)
{
	if (bConnected == false)
	{
		return false;
	}
	
	// Prepare param for PQexecParams.
	char * paramValues[1] = { NULL };
	
	char sUtfNick[65];
	paramValues[0] = sUtfNick;
	if (TextConverter::mPtr->CheckUtf8AndConvert(sNick, ui8NickLen, sUtfNick, 65) == 0)
	{
		return false;
	}
	
	/*  Run SELECT command.
	    We need timestamp as epoch.
	    That allow us to use that result for standard C date/time formating functions and send result to user correctly formated using system locales.
	    LIKE allow us to use SQL wildcards. Very usefull :)
	    And we will return max 50 results sorted by timestamp from newest. */
	PGresult * pResult = PQexecParams(pDBConn, "SELECT nick, EXTRACT(EPOCH FROM last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE LOWER(nick) LIKE LOWER($1) ORDER BY last_updated DESC LIMIT 50;", 1, NULL, paramValues, NULL, NULL, 0);
	
	if (PQresultStatus(pResult) != PGRES_TUPLES_OK)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search for nick failed: %s", PQresultErrorMessage(pResult));
		PQclear(pResult);
		
		return false;
	}
	
	int iTuples = PQntuples(pResult);
	
	if (iTuples == 0)
	{
		PQclear(pResult);
		return false;
	}
	
	bool bResult = SendQueryResults(pUser, bFromPM, pResult, iTuples);
	
	PQclear(pResult);
	
	return bResult;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Second of two fnctions to search data in database. Now using IP.
bool DBPostgreSQL::SearchIP(char * sIP, User * pUser, const bool &bFromPM)
{
	if (bConnected == false)
	{
		return false;
	}
	
	// Prepare param for PQexecParams.
	char * paramValues[1] = { sIP };
	
	//  Run SELECT command. Similar as in SearchNick.
	PGresult * pResult = PQexecParams(pDBConn, "SELECT nick, EXTRACT(EPOCH FROM last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE ip_address LIKE $1 ORDER BY last_updated DESC LIMIT 50;", 1, NULL, paramValues, NULL, NULL, 0);
	
	if (PQresultStatus(pResult) != PGRES_TUPLES_OK)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL search for IP failed: %s", PQresultErrorMessage(pResult));
		PQclear(pResult);
		
		return false;
	}
	
	int iTuples = PQntuples(pResult);
	
	if (iTuples == 0)
	{
		PQclear(pResult);
		return false;
	}
	
	bool bResult = SendQueryResults(pUser, bFromPM, pResult, iTuples);
	
	PQclear(pResult);
	
	return bResult;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to remove X days old records from database.
void DBPostgreSQL::RemoveOldRecords(const uint16_t &ui16Days)
{
	if (bConnected == false)
	{
		return;
	}
	
	// Prepare X days old time in C
	time_t acc_time = 0;
	time(&acc_time);
	
	struct tm *tm = localtime(&acc_time);
	
	tm->tm_mday -= ui16Days;
	tm->tm_isdst = -1;
	
	time_t tTime = mktime(tm);
	
	if (tTime == (time_t) - 1)
	{
		return;
	}
	
	// Prepare param for PQexecParams.
	string sTimeStamp = (uint64_t)tTime;
	char * paramValues[1] = { sTimeStamp.c_str() };
	
	// Run DELETE command. It is simple. We given timestamp in epoch and to_timestamp will convert it to PostgreSQL timestamp format.
	PGresult * pResult = PQexecParams(pDBConn, "DELETE FROM userinfo WHERE last_updated < to_timestamp($1);", 1, NULL, paramValues, NULL, NULL, 0);
	
	if (PQresultStatus(pResult) != PGRES_COMMAND_OK)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL remove old records failed: %s", PQresultErrorMessage(pResult));
	}
	
	char * sRows = PQcmdTuples(pResult);
	
	/*  When non-zero rows was affected then send info to UDP debug */
	if (sRows[0] != '0' || sRows[1] != '\0')
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBPostgreSQL removed old records: %s", sRows);
	}
	
	PQclear(pResult);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
