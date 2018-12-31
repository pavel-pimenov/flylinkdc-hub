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
#include "DB-MySQL.h"
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
#include <mysql.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
DBMySQL * DBMySQL::mPtr = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBMySQL::DBMySQL()
{
	if (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_DATABASE] == false || clsSettingManager::mPtr->sTexts[SETTXT_MYSQL_PASS] == NULL)
	{
		bConnected = false;
		
		return;
	}
	
	pDBHandle = mysql_init(NULL);
	
	if (pDBHandle == NULL)
	{
		bConnected = false;
		AppendLog("DBMySQL init failed");
		
		return;
	}
	
	mysql_options(pDBHandle, MYSQL_SET_CHARSET_NAME, "utf8mb4");
	
	if (mysql_real_connect(pDBHandle, clsSettingManager::mPtr->sTexts[SETTXT_MYSQL_HOST], clsSettingManager::mPtr->sTexts[SETTXT_MYSQL_USER], clsSettingManager::mPtr->sTexts[SETTXT_MYSQL_PASS], clsSettingManager::mPtr->sTexts[SETTXT_MYSQL_DBNAME], atoi(clsSettingManager::mPtr->sTexts[SETTXT_MYSQL_PORT]), NULL, 0) == NULL)
	{
		bConnected = false;
		AppendLog(string("DBMySQL connection failed: ") + mysql_error(pDBHandle));
		mysql_close(pDBHandle);
		
		return;
	}
	
	if (mysql_query(pDBHandle,
	                "CREATE TABLE IF NOT EXISTS userinfo ("
	                "nick VARCHAR(64) NOT NULL,"
	                "last_updated DATETIME NOT NULL,"
	                "ip_address VARCHAR(39) NOT NULL,"
	                "share VARCHAR(24) NOT NULL,"
	                "description VARCHAR(192),"
	                "tag VARCHAR(192),"
	                "connection VARCHAR(32),"
	                "email VARCHAR(96),"
	                "UNIQUE (nick)"
	                ")"
	                "DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
	               ) != 0)
	{
		bConnected = false;
		AppendLog(string("DBMySQL check/create table failed: ") + mysql_error(pDBHandle));
		mysql_close(pDBHandle);
		
		return;
	}
	
	bConnected = true;
	
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBMySQL::~DBMySQL()
{
	// When user don't want to save data in database forever then he can set to remove records older than X days.
	if (clsSettingManager::mPtr->i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS] != 0)
	{
		RemoveOldRecords(clsSettingManager::mPtr->i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS]);
	}
	
	if (bConnected == true)
	{
		mysql_close(pDBHandle);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Now that important part. Function to update or insert user to database.
void DBMySQL::UpdateRecord(User * pUser)
{
	if (bConnected == false)
	{
		return;
	}
	
	char sUtfNick[65];
	size_t szLen = TextConverter::mPtr->CheckUtf8AndConvert(pUser->sNick, pUser->ui8NickLen, sUtfNick, 65);
	if (szLen == 0)
	{
		return;
	}
	
	char sNick[129];
	if (mysql_real_escape_string(pDBHandle, sNick, sUtfNick, szLen) == 0)
	{
		return;
	}
	
	char sShare[24];
	sprintf(sShare, "%0.02f GB", (double)pUser->ui64SharedSize / 1073741824);
	
	char sDescription[385];
	if (pUser->sDescription != NULL)
	{
		char sUtfDescription[193];
		
		szLen = TextConverter::mPtr->CheckUtf8AndConvert(pUser->sDescription, pUser->ui8DescriptionLen, sUtfDescription, 193);
		if (szLen != 0)
		{
			mysql_real_escape_string(pDBHandle, sDescription, sUtfDescription, szLen);
		}
		else
		{
			sDescription[0] = '\0';
		}
	}
	else
	{
		sDescription[0] = '\0';
	}
	
	char sTag[385];
	if (pUser->sTag != NULL)
	{
		char sUtfTag[193];
		
		szLen = TextConverter::mPtr->CheckUtf8AndConvert(pUser->sTag, pUser->ui8TagLen, sUtfTag, 193);
		if (szLen != 0)
		{
			mysql_real_escape_string(pDBHandle, sTag, sUtfTag, szLen);
		}
		else
		{
			sTag[0] = '\0';
		}
	}
	else
	{
		sTag[0] = '\0';
	}
	
	char sConnection[65];
	if (pUser->sConnection != NULL)
	{
		char sUtfConnection[33];
		
		szLen = TextConverter::mPtr->CheckUtf8AndConvert(pUser->sConnection, pUser->ui8ConnectionLen, sUtfConnection, 33);
		if (szLen != 0)
		{
			mysql_real_escape_string(pDBHandle, sConnection, sUtfConnection, szLen);
		}
		else
		{
			sConnection[0] = '\0';
		}
	}
	else
	{
		sConnection[0] = '\0';
	}
	
	char sEmail[193];
	if (pUser->sEmail != NULL)
	{
		char sUtfEmail[97];
		
		szLen = TextConverter::mPtr->CheckUtf8AndConvert(pUser->sEmail, pUser->ui8EmailLen, sUtfEmail, 97);
		if (szLen != 0)
		{
			mysql_real_escape_string(pDBHandle, sEmail, sUtfEmail, szLen);
		}
		else
		{
			sEmail[0] = '\0';
		}
	}
	else
	{
		sEmail[0] = '\0';
	}
	
	char sSQLCommand[4096];
	sprintf(sSQLCommand,
	        "INSERT INTO userinfo (nick, last_updated, ip_address, share, description, tag, connection, email) VALUES ("
	        "'%s', NOW(), '%s', '%s', '%s', '%s', '%s', '%s'"
	        ")  ON DUPLICATE KEY UPDATE "
	        "last_updated = NOW(), ip_address = '%s', share = '%s',"
	        "description = '%s', tag = '%s', connection = '%s', email = '%s'",
	        sNick, pUser->sIP, sShare, sDescription, sTag, sConnection, sEmail,
	        pUser->sIP, sShare, sDescription, sTag, sConnection, sEmail
	       );
	       
	if (mysql_query(pDBHandle, sSQLCommand) != 0)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL insert/update record failed: %s", mysql_error(pDBHandle));
		
		return;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to send data returned by SELECT to Operator who requested them.
bool DBMySQL::SendQueryResults(User * pUser, const bool &bFromPM, MYSQL_RES * pResult, uint64_t &ui64Rows)
{
	unsigned int uiCount = mysql_num_fields(pResult);
	if (uiCount != 8)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL mysql_num_fields wrong fields count: %u", uiCount);
		return false;
	}
	
	if (ui64Rows == 1)  // When we have only one result then we send whole info about user.
	{
		MYSQL_ROW mRow = mysql_fetch_row(pResult);
		if (mRow == NULL)
		{
			clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_row failed: %s", mysql_error(pDBHandle));
			return false;
		}
		
		unsigned long * pLengths = mysql_fetch_lengths(pResult);
		if (pLengths == NULL)
		{
			clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_lengths failed: %s", mysql_error(pDBHandle));
			return false;
		}
		
		int iMsgLen = 0;
		
		if (bFromPM == true)
		{
			iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "$To: %s From: %s $", pUser->sNick, clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC]);
			if (CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults1") == false)
			{
				return false;
			}
		}
		
		if (pLengths[0] <= 0 || pLengths[0] > 64)
		{
			clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid nick length: " PRIu64, (uint64_t)pLengths[0]);
			return false;
		}
		
		int iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "<%s> \n%s: %s", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_NICK], mRow[0]);
		iMsgLen += iRet;
		if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults2") == false)
		{
			return false;
		}
		
		RegUser * pReg = clsRegManager::mPtr->Find(mRow[0], pLengths[0]);
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
		User * pOnlineUser = clsHashManager::mPtr->FindUser(mRow[0], pLengths[0]);
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
			time_t tTime = (time_t)_strtoui64(mRow[1], NULL, 10);
#else
			time_t tTime = (time_t)strtoull(mRow[1], NULL, 10);
#endif
			struct tm *tm = localtime(&tTime);
			iRet = (int)strftime(clsServerManager::pGlobalBuffer + iMsgLen, 256, "%c", tm);
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults14") == false)
			{
				return false;
			}
			
			if (pLengths[2] <= 0 || pLengths[2] > 39)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid ip length: " PRIu64, (uint64_t)pLengths[2]);
				return false;
			}
			
			if (pLengths[3] <= 0 || pLengths[3] > 24)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid share length: " PRIu64, (uint64_t)pLengths[3]);
				return false;
			}
			
			char * sIP = mRow[2];
			
			iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_IP], sIP, clsLanguageManager::mPtr->sTexts[LAN_SHARE_SIZE], mRow[3]);
			iMsgLen += iRet;
			if (CheckSprintf1(iRet, iMsgLen, 1024, "SendQueryResults15") == false)
			{
				return false;
			}
			
			if (mRow[4] != NULL)
			{
				if (pLengths[4] > 192)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid description length: " PRIu64, (uint64_t)pLengths[4]);
					return  false;
				}
				
				if (pLengths[4] > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION], mRow[4]);
					iMsgLen += iRet;
					if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults16") == false)
					{
						return false;
					}
				}
			}
			
			if (mRow[5] != NULL)
			{
				if (pLengths[5] > 192)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid tag length: " PRIu64, (uint64_t)pLengths[5]);
					return false;
				}
				
				if (pLengths[5] > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_TAG], mRow[5]);
					iMsgLen += iRet;
					if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults17") == false)
					{
						return false;
					}
				}
			}
			
			if (mRow[6] != NULL)
			{
				if (pLengths[6] > 32)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid connection length: " PRIu64, (uint64_t)pLengths[6]);
					return false;
				}
				
				if (pLengths[6] > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_CONNECTION], mRow[6]);
					iMsgLen += iRet;
					if (CheckSprintf1(iRet, iMsgLen, clsServerManager::szGlobalBufferSize, "SendQueryResults18") == false)
					{
						return false;
					}
				}
			}
			
			if (mRow[7] != NULL)
			{
				if (pLengths[7] > 96)
				{
					clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid email length: " PRIu64, (uint64_t)pLengths[7]);
					return false;
				}
				
				if (pLengths[7] > 0)
				{
					iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s", clsLanguageManager::mPtr->sTexts[LAN_EMAIL], mRow[7]);
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
		
		for (uint64_t ui64ActualRow = 0; ui64ActualRow < ui64Rows; ui64ActualRow++)
		{
			MYSQL_ROW mRow = mysql_fetch_row(pResult);
			if (mRow == NULL)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_row failed: %s", mysql_error(pDBHandle));
				return false;
			}
			
			unsigned long * pLengths = mysql_fetch_lengths(pResult);
			if (pLengths == NULL)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_lengths failed: %s", mysql_error(pDBHandle));
				return false;
			}
			
			if (pLengths[0] <= 0 || pLengths[0] > 64)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid nick length: " PRIu64, (uint64_t)pLengths[0]);
				return false;
			}
			
			if (pLengths[2] <= 0 || pLengths[2] > 39)
			{
				clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search returned invalid ip length: " PRIu64, (uint64_t)pLengths[2]);
				return false;
			}
			
			int iRet = sprintf(clsServerManager::pGlobalBuffer + iMsgLen, "\n%s: %s\t\t%s: %s", clsLanguageManager::mPtr->sTexts[LAN_NICK], mRow[0], clsLanguageManager::mPtr->sTexts[LAN_IP], mRow[2]);
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
bool DBMySQL::SearchNick(char * sNick, const uint8_t &ui8NickLen, User * pUser, const bool &bFromPM)
{
	if (bConnected == false)
	{
		return false;
	}
	
	char sUtfNick[65];
	size_t szLen = TextConverter::mPtr->CheckUtf8AndConvert(sNick, ui8NickLen, sUtfNick, 65);
	if (szLen == 0)
	{
		return false;
	}
	
	char sEscapedNick[129];
	if (mysql_real_escape_string(pDBHandle, sEscapedNick, sUtfNick, szLen) == 0)
	{
		return false;
	}
	
	char sSQLCommand[256];
	sprintf(sSQLCommand, "SELECT nick, UNIX_TIMESTAMP(last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE LOWER(nick) LIKE LOWER('%s') ORDER BY last_updated DESC LIMIT 50", sEscapedNick);
	
	if (mysql_query(pDBHandle, sSQLCommand) != 0)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search for nick failed: %s", mysql_error(pDBHandle));
		
		return false;
	}
	
	MYSQL_RES * pResult = mysql_store_result(pDBHandle);
	
	if (pResult == NULL)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search for nick store result failed: %s", mysql_error(pDBHandle));
		
		return false;
	}
	
	uint64_t ui64Rows = (uint64_t)mysql_num_rows(pResult);
	
	bool bResult = false;
	
	if (ui64Rows != 0)
	{
		bResult = SendQueryResults(pUser, bFromPM, pResult, ui64Rows);
	}
	
	mysql_free_result(pResult);
	
	return bResult;
	
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Second of two fnctions to search data in database. Now using IP.
bool DBMySQL::SearchIP(char * sIP, User * pUser, const bool &bFromPM)
{
	if (bConnected == false)
	{
		return false;
	}
	
	size_t szLen = strlen(sIP);
	
	char sEscapedIP[79];
	if (mysql_real_escape_string(pDBHandle, sEscapedIP, sIP, szLen < 40 ? szLen : 39) == 0)
	{
		return false;
	}
	
	char sSQLCommand[256];
	sprintf(sSQLCommand, "SELECT nick, UNIX_TIMESTAMP(last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE ip_address LIKE '%s' ORDER BY last_updated DESC LIMIT 50", sEscapedIP);
	
	if (mysql_query(pDBHandle, sSQLCommand) != 0)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search for IP failed: %s", mysql_error(pDBHandle));
		
		return false;
	}
	
	my_ulonglong mullRows = mysql_affected_rows(pDBHandle);
	
	if (mullRows == 0)
	{
		return false;
	}
	
	MYSQL_RES * pResult = mysql_store_result(pDBHandle);
	
	if (pResult == NULL)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL search for IP store result failed: %s", mysql_error(pDBHandle));
		
		return false;
	}
	
	uint64_t ui64Rows = (uint64_t)mysql_num_rows(pResult);
	
	bool bResult = false;
	
	if (ui64Rows != 0)
	{
		bResult = SendQueryResults(pUser, bFromPM, pResult, ui64Rows);
	}
	
	mysql_free_result(pResult);
	
	return bResult;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to remove X days old records from database.
void DBMySQL::RemoveOldRecords(const uint16_t &ui16Days)
{
	if (bConnected == false)
	{
		return;
	}
	
	char sSQLCommand[256];
	sprintf(sSQLCommand, "DELETE FROM userinfo WHERE last_updated < UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL %hu DAY))", ui16Days);
	
	if (mysql_query(pDBHandle, sSQLCommand) != 0)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL remove old records failed: %s", mysql_error(pDBHandle));
		
		return;
	}
	
	uint64_t ui64Rows = (uint64_t)mysql_affected_rows(pDBHandle);
	
	if (ui64Rows != 0)
	{
		clsUdpDebug::mPtr->BroadcastFormat("[LOG] DBMySQL removed old records: " PRIu64, ui64Rows);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
