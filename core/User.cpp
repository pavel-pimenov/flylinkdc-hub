/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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

//---------------------------------------------------------------------------
#include "stdinc.h"
#include <fstream>
//---------------------------------------------------------------------------
#include "User.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
#include "UdpDebug.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#elif _WITH_POSTGRES
#include "DB-PostgreSQL.h"
#elif _WITH_MYSQL
#include "DB-MySQL.h"
#endif
#include "DeFlood.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
#include "../gui.win/GuiUtil.h"
#include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
static const size_t ZMINDATALEN = 128;
static const char * sBadTag = "BAD TAG!"; // 8
static const char * sOtherNoTag = "OTHER (NO TAG)"; // 14
static const char * sUnknownTag = "UNKNOWN TAG"; // 11
static const char * sDefaultNick = "<unknown>"; // 9
//---------------------------------------------------------------------------
DcCommand ActualDcCommand;
//---------------------------------------------------------------------------

static bool UserProcessLines(User * pUser, const uint32_t ui32NewDataStart)
{
	char c = 0;
	
	char * pBuffer = pUser->m_pRecvBuf;
	char * buffer = pUser->m_pRecvBuf;
	static int g_id = 0;
	++g_id;

	for (uint32_t ui32i = ui32NewDataStart; ui32i < pUser->m_ui32RecvBufDataLen; ++ui32i)
	{
		// look for pipes in the data - process lines one by one
		if (pUser->m_pRecvBuf[ui32i] == '|')
		{
			c = pUser->m_pRecvBuf[ui32i + 1];
			pUser->m_pRecvBuf[ui32i + 1] = '\0';
			
			const uint32_t ui32CommandLen = (uint32_t)(((pUser->m_pRecvBuf + ui32i) - pBuffer) + 1);
			if (pBuffer[0] == '|')
			{
				//UdpDebug->BroadcastFormat("[SYS] heartbeat from %s (%s).", pUser->Nick, pUser->m_sIP);
				//send(Sck, "|", 1, 0);
			}
			else if (ui32CommandLen <= (pUser->m_ui8State < User::STATE_ADDME ? 1024U : 65536U))
			{
				ActualDcCommand.m_pUser = pUser;
				ActualDcCommand.m_sCommand = pBuffer;
				ActualDcCommand.m_ui32CommandLen = ui32CommandLen;
				
				DcCommands::m_Ptr->PreProcessData(&ActualDcCommand);
				extern bool g_isUseLog;
				if (pUser->m_is_proxy_user || pUser->m_is_invalid_json || g_isUseLog ||
				        pUser->m_is_bad_len_port || pUser->m_is_bad_port || pUser->m_is_ddos_udp ||
				        pUser->m_is_bad_len_number_myinfo || pUser->m_is_max_int8 || pUser->m_is_max_ip_len)
				{  
					pUser->logInvalidUser(g_id, buffer, ui32CommandLen, false);
				}
			}
			else
			{
				pUser->SendFormat("UserProcessLines1", false, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_CMD_TOO_LONG]);
				pUser->Close();
				
				UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s): Received command too long. User disconnected.", pUser->m_sNick, pUser->m_sIP);
				
				return false;
			}
			
			pUser->m_pRecvBuf[ui32i + 1] = c;
			pBuffer += ui32CommandLen;
			if (pUser->m_ui8State >= User::STATE_CLOSING)
			{
				return true;
			}
		}
		else if (pUser->m_pRecvBuf[ui32i] == '\0')
		{
			// look for NULL character and replace with zero
			pUser->m_pRecvBuf[ui32i] = '0';
			continue;
		}
	}
	
	pUser->m_ui32RecvBufDataLen -= (uint32_t)(pBuffer - pUser->m_pRecvBuf);
	
	if (pUser->m_ui32RecvBufDataLen == 0)
	{
		DcCommands::m_Ptr->ProcessCmds(pUser);
		
		pUser->m_pRecvBuf[0] = '\0';
		
		return false;
	}
	else if (pUser->m_ui32RecvBufDataLen == 1)
	{
		DcCommands::m_Ptr->ProcessCmds(pUser);
		
		pUser->m_pRecvBuf[0] = pBuffer[0];
		pUser->m_pRecvBuf[1] = '\0';
		
		return true;
	}
	else
	{
		if (pUser->m_ui32RecvBufDataLen > (pUser->m_ui8State < User::STATE_ADDME ? 1024U : 65536U))
		{
			// PPK ... we don't want commands longer than 64 kB, drop this user !
			pUser->SendFormat("UserProcessLines2", false, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_CMD_TOO_LONG]);
			pUser->Close();
			
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s): RecvBuffer overflow. User disconnected.", pUser->m_sNick, pUser->m_sIP);
			
			return false;
		}
		
		DcCommands::m_Ptr->ProcessCmds(pUser);
		
		memmove(pUser->m_pRecvBuf, pBuffer, pUser->m_ui32RecvBufDataLen);
		pUser->m_pRecvBuf[pUser->m_ui32RecvBufDataLen] = '\0';
		
			return true;
		}
	}
//------------------------------------------------------------------------------

static void UserSetBadTag(User * pUser, char * sDesc, const uint8_t ui8DescLen)
{
	// PPK ... clear all tag related things
	pUser->m_sTagVersion = NULL;
	pUser->m_ui8TagVersionLen = 0;
	
	pUser->m_sModes[0] = '\0';
	pUser->m_ui32Hubs = pUser->m_ui32Slots = pUser->m_ui32OLimit = pUser->m_ui32LLimit = pUser->m_ui32DLimit = pUser->m_ui32NormalHubs = pUser->m_ui32RegHubs = pUser->m_ui32OpHubs = 0;
	pUser->m_ui32BoolBits |= User::BIT_OLDHUBSTAG;
	pUser->m_ui32BoolBits |= User::BIT_HAVE_BADTAG;
	
	pUser->m_sDescription = sDesc;
	pUser->m_ui8DescriptionLen = ui8DescLen;
	
	// PPK ... clear (fake) tag
	pUser->m_sTag = NULL;
	pUser->m_ui8TagLen = 0;
	
	// PPK ... set bad tag
	pUser->m_sClient = (char *)sBadTag;
	pUser->m_ui8ClientLen = 8;
	
	// PPK ... send report to udp debug
	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) have bad TAG (%s) ?!?", pUser->m_sNick, pUser->m_sIP, pUser->m_sMyInfoOriginal);
}
//---------------------------------------------------------------------------

static void UserParseMyInfo(User * pUser)
{
	memcpy(ServerManager::m_pGlobalBuffer, pUser->m_sMyInfoOriginal, pUser->m_ui16MyInfoOriginalLen);
	
	char *sMyINFOParts[] = { NULL, NULL, NULL, NULL, NULL };
	uint16_t iMyINFOPartsLen[] = { 0, 0, 0, 0, 0 };
	
	unsigned char cPart = 0;
	
	sMyINFOParts[cPart] = ServerManager::m_pGlobalBuffer + 14 + pUser->m_ui8NickLen; // desription start
	
	
	for (uint32_t ui32i = 14 + pUser->m_ui8NickLen; ui32i < pUser->m_ui16MyInfoOriginalLen - 1u; ui32i++)
	{
		if (ServerManager::m_pGlobalBuffer[ui32i] == '$')
		{
			ServerManager::m_pGlobalBuffer[ui32i] = '\0';
			iMyINFOPartsLen[cPart] = (uint16_t)((ServerManager::m_pGlobalBuffer + ui32i) - sMyINFOParts[cPart]);
			
			// are we on end of myinfo ???
			if (cPart == 4)
				break;
				
			cPart++;
			sMyINFOParts[cPart] = ServerManager::m_pGlobalBuffer + ui32i + 1;
		}
	}
	
	// check if we have all myinfo parts, connection and sharesize must have length more than 0 !
	if (sMyINFOParts[0] == NULL || sMyINFOParts[1] == NULL || iMyINFOPartsLen[1] != 1 || sMyINFOParts[2] == NULL || iMyINFOPartsLen[2] == 0 || sMyINFOParts[3] == NULL || sMyINFOParts[4] == NULL || iMyINFOPartsLen[4] == 0)
	{
		pUser->SendFormat("UserParseMyInfo1", false, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_MyINFO_IS_CORRUPTED]);
		
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) with bad MyINFO (%s) disconnected.", pUser->m_sNick, pUser->m_sIP, pUser->m_sMyInfoOriginal);
		
		pUser->Close();
		return;
	}
	
	// connection
	pUser->m_ui8MagicByte = sMyINFOParts[2][iMyINFOPartsLen[2] - 1];
	pUser->m_sConnection = pUser->m_sMyInfoOriginal + (sMyINFOParts[2] - ServerManager::m_pGlobalBuffer);
	pUser->m_ui8ConnectionLen = (uint8_t)(iMyINFOPartsLen[2] - 1);
	
	// email
	if (iMyINFOPartsLen[3] != 0)
	{
		pUser->m_sEmail = pUser->m_sMyInfoOriginal + (sMyINFOParts[3] - ServerManager::m_pGlobalBuffer);
		pUser->m_ui8EmailLen = (uint8_t)iMyINFOPartsLen[3];
	}
	
	// share
	// PPK ... check for valid numeric share, kill fakers !
	if (HaveOnlyNumbers(sMyINFOParts[4], iMyINFOPartsLen[4]) == false)
	{
		//UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) with non-numeric sharesize disconnected.", pUser->Nick, pUser->IP);
		pUser->Close();
		return;
	}
	
	if (((pUser->m_ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED) == true)
	{
		ServerManager::m_ui64TotalShare -= pUser->m_ui64SharedSize;
#ifdef _WIN32
		pUser->m_ui64SharedSize = _strtoui64(sMyINFOParts[4], NULL, 10);
#else
		pUser->m_ui64SharedSize = strtoull(sMyINFOParts[4], NULL, 10);
#endif
		ServerManager::m_ui64TotalShare += pUser->m_ui64SharedSize;
	}
	else
	{
#ifdef _WIN32
		pUser->m_ui64SharedSize = _strtoui64(sMyINFOParts[4], NULL, 10);
#else
		pUser->m_ui64SharedSize = strtoull(sMyINFOParts[4], NULL, 10);
#endif
	}
	
	// Reset all tag infos...
	pUser->m_sModes[0] = '\0';
	pUser->m_ui32Hubs = 0;
	pUser->m_ui32NormalHubs = 0;
	pUser->m_ui32RegHubs = 0;
	pUser->m_ui32OpHubs = 0;
	pUser->m_ui32Slots = 0;
	pUser->m_ui32OLimit = 0;
	pUser->m_ui32LLimit = 0;
	pUser->m_ui32DLimit = 0;
	
	// description
	if (iMyINFOPartsLen[0] != 0)
	{
		if (sMyINFOParts[0][iMyINFOPartsLen[0] - 1] == '>')
		{
			char *DCTag = strrchr(sMyINFOParts[0], '<');
			if (DCTag == NULL)
			{
				pUser->m_sDescription = pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer);
				pUser->m_ui8DescriptionLen = (uint8_t)iMyINFOPartsLen[0];
				
				pUser->m_sClient = (char*)sOtherNoTag;
				pUser->m_ui8ClientLen = 14;
				return;
			}
			
			pUser->m_sTag = pUser->m_sMyInfoOriginal + (DCTag - ServerManager::m_pGlobalBuffer);
			pUser->m_ui8TagLen = (uint8_t)(iMyINFOPartsLen[0] - (DCTag - sMyINFOParts[0]));
			
			static const uint16_t ui16plusplus = *((uint16_t *)"++");
			if (DCTag[3] == ' ' && *((uint16_t *)(DCTag + 1)) == ui16plusplus)
			{
				pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
			}
			
			static const uint16_t ui16V = *((uint16_t *)"V:");
			
			char * sTemp = strchr(DCTag, ' ');
			
			if (sTemp != NULL && *((uint16_t *)(sTemp + 1)) == ui16V)
			{
				sTemp[0] = '\0';
				pUser->m_sClient = pUser->m_sMyInfoOriginal + ((DCTag + 1) - ServerManager::m_pGlobalBuffer);
				pUser->m_ui8ClientLen = (uint8_t)((sTemp - DCTag) - 1);
			}
			else
			{
				pUser->m_sClient = (char *)sUnknownTag;
				pUser->m_ui8ClientLen = 11;
				pUser->m_sTag = NULL;
				pUser->m_ui8TagLen = 0;
				sMyINFOParts[0][iMyINFOPartsLen[0] - 1] = '>'; // not valid DC Tag, add back > tag ending
				pUser->m_sDescription = pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer);
				pUser->m_ui8DescriptionLen = (uint8_t)iMyINFOPartsLen[0];
				return;
			}
			
			size_t szTagPattLen = ((sTemp - DCTag) + 1);
			
			sMyINFOParts[0][iMyINFOPartsLen[0] - 1] = ','; // terminate tag end with ',' for easy tag parsing
			
			uint32_t reqVals = 0;
			char * sTagPart = DCTag + szTagPattLen;
			
			for (size_t szi = szTagPattLen; szi < (size_t)(iMyINFOPartsLen[0] - (DCTag - sMyINFOParts[0])); szi++)
			{
				if (DCTag[szi] == ',')
				{
					DCTag[szi] = '\0';
					if (sTagPart[1] != ':')
					{
						UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
						return;
					}
					
					switch (sTagPart[0])
					{
						case 'V':
							// PPK ... fix for potencial memory leak with fake tag
							if (sTagPart[2] == '\0' || pUser->m_sTagVersion)
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							pUser->m_sTagVersion = pUser->m_sMyInfoOriginal + ((sTagPart + 2) - ServerManager::m_pGlobalBuffer);
							pUser->m_ui8TagVersionLen = (uint8_t)((DCTag + szi) - (sTagPart + 2));
							reqVals++;
							break;
						case 'M':
							if ((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && (pUser->m_ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64)
							{
								if (sTagPart[2] == '\0' || sTagPart[3] == '\0' || sTagPart[4] != '\0')
								{
									UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
									return;
								}
								pUser->m_sModes[0] = sTagPart[2];
								pUser->m_sModes[1] = sTagPart[3];
								pUser->m_sModes[2] = '\0';
								
								if (toupper(sTagPart[3]) == 'A')
								{
									pUser->m_ui32BoolBits |= User::BIT_IPV6_ACTIVE;
								}
								else
								{
									pUser->m_ui32BoolBits &= ~User::BIT_IPV6_ACTIVE;
								}
							}
							else
							{
								if (sTagPart[2] == '\0' || sTagPart[3] != '\0')
								{
									UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
									return;
								}
								pUser->m_sModes[0] = sTagPart[2];
								pUser->m_sModes[1] = '\0';
							}
							
							if (toupper(sTagPart[2]) == 'A')
							{
								pUser->m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
							}
							else
							{
								pUser->m_ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
							}
							
							reqVals++;
							break;
						case 'H':
						{
							if (sTagPart[2] == '\0')
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							
							DCTag[szi] = '/';
							
							char *sHubsParts[] = { NULL, NULL, NULL };
							uint16_t iHubsPartsLen[] = { 0, 0, 0 };
							
							uint8_t ui8Part = 0;
							
							sHubsParts[ui8Part] = sTagPart + 2;
							
							
							for (uint32_t ui32j = 3; ui32j < (uint32_t)((DCTag + szi + 1) - sTagPart); ui32j++)
							{
								if (sTagPart[ui32j] == '/')
								{
									sTagPart[ui32j] = '\0';
									iHubsPartsLen[ui8Part] = (uint16_t)((sTagPart + ui32j) - sHubsParts[ui8Part]);
									
									// are we on end of hubs tag part ???
									if (ui8Part == 2)
										break;
										
									ui8Part++;
									sHubsParts[ui8Part] = sTagPart + ui32j + 1;
								}
							}
							
							if (sHubsParts[0] != NULL && sHubsParts[1] != NULL && sHubsParts[2] != NULL)
							{
								if (iHubsPartsLen[0] != 0 && iHubsPartsLen[1] != 0 && iHubsPartsLen[2] != 0)
								{
									if (HaveOnlyNumbers(sHubsParts[0], iHubsPartsLen[0]) == false ||
									        HaveOnlyNumbers(sHubsParts[1], iHubsPartsLen[1]) == false ||
									        HaveOnlyNumbers(sHubsParts[2], iHubsPartsLen[2]) == false)
									{
										UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
										return;
									}
									pUser->m_ui32NormalHubs = atoi(sHubsParts[0]);
									pUser->m_ui32RegHubs = atoi(sHubsParts[1]);
									pUser->m_ui32OpHubs = atoi(sHubsParts[2]);
									pUser->m_ui32Hubs = pUser->m_ui32NormalHubs + pUser->m_ui32RegHubs + pUser->m_ui32OpHubs;
									// PPK ... kill LAM3R with fake hubs
									if (pUser->m_ui32Hubs != 0)
									{
										pUser->m_ui32BoolBits &= ~User::BIT_OLDHUBSTAG;
										reqVals++;
										break;
									}
								}
							}
							else if (sHubsParts[1] == DCTag + szi + 1 && sHubsParts[2] == NULL)
							{
								DCTag[szi] = '\0';
								pUser->m_ui32Hubs = atoi(sHubsParts[0]);
								reqVals++;
								pUser->m_ui32BoolBits |= User::BIT_OLDHUBSTAG;
								break;
							}
							
							pUser->SendFormat("UserParseMyInfo2", false, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_FAKE_TAG]);
							
							pUser->m_sTag[pUser->m_ui8TagLen] = '\0';
							UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) with fake Tag disconnected: %s", pUser->m_sNick, pUser->m_sIP, pUser->m_sTag);
							
							//
							// [-] FlylinkDC++ fix User JuniorSD_R166 (178.68.64.129) with fake Tag disconnected: <FlylinkDC++ V:r503-x64-19807,M ,H:0/0/0,S:15>


							// pUser->Close();
							// return;
							//
							pUser->m_ui32NormalHubs = 1; // [+]FlylinkDC++
							if (pUser->m_ui32Hubs == 0)
							{
								pUser->m_ui32Hubs = 1;
							}
							break;
						}
						case 'S':
						{
							if (sTagPart[2] == '\0')
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							const uint16_t l_slot_len = uint16_t(strlen(sTagPart + 2));
							if (l_slot_len > 12)
							{
								pUser->m_is_bad_len_number_myinfo = true;
							}
							if (HaveOnlyNumbers(sTagPart + 2, l_slot_len) == false)
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							pUser->m_ui32Slots = atoi(sTagPart + 2);
							reqVals++;
							break;
						}
						case 'O':
							if (sTagPart[2] == '\0')
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							if (strlen(sTagPart) > 12)
							{
								pUser->m_is_bad_len_number_myinfo = true;
							}
							pUser->m_ui32OLimit = atoi(sTagPart + 2);
							break;
						case 'B':
							if (sTagPart[2] == '\0')
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							if (strlen(sTagPart) > 12)
							{
								pUser->m_is_bad_len_number_myinfo = true;
							}
							pUser->m_ui32LLimit = atoi(sTagPart + 2);
							break;
						case 'L':
							if (sTagPart[2] == '\0')
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							if (strlen(sTagPart) > 12)
							{
								pUser->m_is_bad_len_number_myinfo = true;
							}
							pUser->m_ui32LLimit = atoi(sTagPart + 2);
							break;
						case 'D':
							if (sTagPart[2] == '\0')
							{
								UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
								return;
							}
							if (strlen(sTagPart) > 12)
							{
								pUser->m_is_bad_len_number_myinfo = true;
							}
							pUser->m_ui32DLimit = atoi(sTagPart + 2);
							break;
						default:
							//UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s): Extra info in DC tag: %s", pUser->Nick, pUser->m_sIP, sTag);
							break;
					}
					sTagPart = DCTag + szi + 1;
				}
			}
			
			if (reqVals < 4)
			{
				UserSetBadTag(pUser, pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), (uint8_t)iMyINFOPartsLen[0]);
				return;
			}
			else
			{
				pUser->m_sDescription = pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer);
				pUser->m_ui8DescriptionLen = (uint8_t)(DCTag - sMyINFOParts[0]);
				return;
			}
		}
		else
		{
			pUser->m_sDescription = pUser->m_sMyInfoOriginal + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer);
			pUser->m_ui8DescriptionLen = (uint8_t)iMyINFOPartsLen[0];
		}
	}
	
	pUser->m_sClient = (char *)sOtherNoTag;
	pUser->m_ui8ClientLen = 14;
	
	pUser->m_sTag = NULL;
	pUser->m_ui8TagLen = 0;
	
	pUser->m_sTagVersion = NULL;
	pUser->m_ui8TagVersionLen = 0;
	
	pUser->m_sModes[0] = '\0';
	pUser->m_ui32Hubs = 0;
	pUser->m_ui32NormalHubs = 0;
	pUser->m_ui32RegHubs = 0;
	pUser->m_ui32OpHubs = 0;
	pUser->m_ui32Slots = 0;
	pUser->m_ui32OLimit = 0;
	pUser->m_ui32LLimit = 0;
	pUser->m_ui32DLimit = 0;
}
//---------------------------------------------------------------------------

UserBan::UserBan() : m_sMessage(NULL), m_ui32Len(0), m_ui32NickHash(0)
{
	// ...
}
//---------------------------------------------------------------------------

UserBan::~UserBan()
{
	safe_free(m_sMessage);
}
//---------------------------------------------------------------------------

UserBan * UserBan::CreateUserBan(const char * sMess, const uint32_t ui32MessLen, const uint32_t ui32Hash)
{
	UserBan * pUserBan = new (std::nothrow) UserBan();
	
	if (pUserBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate new pUserBan in UserBan::CreateUserBan\n");
		
		return NULL;
	}
	
	pUserBan->m_sMessage = (char *)malloc(ui32MessLen + 1);
	if (pUserBan->m_sMessage == NULL)
	{
		AppendDebugLogFormat("[MEM] UserBan::CreateUserBan cannot allocate %u bytes for sMessage\n", ui32MessLen + 1);
		
		delete pUserBan;
		return NULL;
	}
	
	memcpy(pUserBan->m_sMessage, sMess, ui32MessLen);
	pUserBan->m_sMessage[ui32MessLen] = '\0';
	
	pUserBan->m_ui32Len = ui32MessLen;
	pUserBan->m_ui32NickHash = ui32Hash;
	
	return pUserBan;
}
//---------------------------------------------------------------------------

LoginLogout::LoginLogout() : m_ui64LogonTick(0), m_ui64IPv4CheckTick(0), m_pBan(NULL), m_pBuffer(NULL), m_ui32ToCloseLoops(0), m_ui32UserConnectedLen(0)
{
	// ...
}
//---------------------------------------------------------------------------
void LoginLogout::Clean()
{
	safe_delete(m_pBan);
	safe_free(m_pBuffer);
}
LoginLogout::~LoginLogout()
{
	Clean();
}
//---------------------------------------------------------------------------

User::User() : m_ui64SharedSize(0), m_ui64ChangedSharedSizeShort(0), m_ui64ChangedSharedSizeLong(0), m_ui64GetNickListsTick(0), m_ui64MyINFOsTick(0), m_ui64SearchsTick(0),
	m_ui64ChatMsgsTick(0), m_ui64PMsTick(0), m_ui64SameSearchsTick(0), m_ui64SamePMsTick(0), m_ui64SameChatsTick(0), m_ui64LastMyINFOSendTick(0), m_ui64LastNicklist(0), m_ui64ReceivedPmTick(0),
	m_ui64ChatMsgsTick2(0), m_ui64PMsTick2(0), m_ui64SearchsTick2(0), m_ui64MyINFOsTick2(0), m_ui64CTMsTick(0), m_ui64CTMsTick2(0), m_ui64RCTMsTick(0), m_ui64RCTMsTick2(0),
	m_ui64SRsTick(0), m_ui64SRsTick2(0), m_ui64RecvsTick(0), m_ui64RecvsTick2(0), m_ui64ChatIntMsgsTick(0), m_ui64PMsIntTick(0), m_ui64SearchsIntTick(0),
	m_tLoginTime(0),
	m_pCmdToUserStrt(NULL), m_pCmdToUserEnd(NULL), m_pCmdStrt(NULL), m_pCmdEnd(NULL), m_pCmdActive4Search(NULL), m_pCmdActive6Search(NULL), m_pCmdPassiveSearch(NULL),
	m_pPrev(NULL), m_pNext(NULL), m_pHashTablePrev(NULL), m_pHashTableNext(NULL), m_pHashIpTablePrev(NULL), m_pHashIpTableNext(NULL),
	m_sNick((char *)sDefaultNick), m_sMyInfoOriginal(NULL), m_sMyInfoShort(NULL), m_sMyInfoLong(NULL),
	m_sDescription(NULL), m_sTag(NULL), m_sConnection(NULL), m_sEmail(NULL), m_sClient((char *)sOtherNoTag), m_sTagVersion(NULL),
	m_sLastChat(NULL), m_sLastPM(NULL), m_sLastSearch(NULL), m_pSendBuf(NULL), m_pRecvBuf(NULL), m_pSendBufHead(NULL),
	m_sChangedDescriptionShort(NULL), m_sChangedDescriptionLong(NULL), m_sChangedTagShort(NULL), m_sChangedTagLong(NULL),
	m_sChangedConnectionShort(NULL), m_sChangedConnectionLong(NULL), m_sChangedEmailShort(NULL), m_sChangedEmailLong(NULL),
	m_ui32Recvs(0), m_ui32Recvs2(0),
	m_ui32Hubs(0), m_ui32Slots(0), m_ui32OLimit(0), m_ui32LLimit(0), m_ui32DLimit(0), m_ui32NormalHubs(0), m_ui32RegHubs(0), m_ui32OpHubs(0),
	m_ui32SendCalled(0), m_ui32RecvCalled(0), m_ui32ReceivedPmCount(0), m_ui32SR(0), m_ui32DefloodWarnings(0),
	m_ui32BoolBits(0), m_ui32InfoBits(0), m_ui32SupportBits(0),
	m_ui32SendBufLen(0), m_ui32RecvBufLen(0), m_ui32SendBufDataLen(0), m_ui32RecvBufDataLen(0),
	m_ui32NickHash(0), m_i32Profile(-1),
#ifdef _WIN32
	m_Socket(INVALID_SOCKET),
#else
	m_Socket(-1),
#endif
	m_ui16MyInfoOriginalLen(0), m_ui16MyInfoShortLen(0), m_ui16MyInfoLongLen(0), m_ui16GetNickLists(0), m_ui16MyINFOs(0), m_ui16Searchs(0),
	m_ui16ChatMsgs(0), m_ui16PMs(0), m_ui16SameSearchs(0), m_ui16LastSearchLen(0), m_ui16SamePMs(0), m_ui16LastPMLen(0), m_ui16SameChatMsgs(0),
	m_ui16LastChatLen(0), m_ui16LastPmLines(0), m_ui16SameMultiPms(0), m_ui16LastChatLines(0), m_ui16SameMultiChats(0), m_ui16ChatMsgs2(0), m_ui16PMs2(0),
	m_ui16Searchs2(0), m_ui16MyINFOs2(0), m_ui16CTMs(0), m_ui16CTMs2(0), m_ui16RCTMs(0), m_ui16RCTMs2(0), m_ui16SRs(0), m_ui16SRs2(0), m_ui16ChatIntMsgs(0),
	m_ui16PMsInt(0), m_ui16SearchsInt(0), m_ui16IpTableIdx(0),
	m_ui8MagicByte(0),
	m_ui8NickLen(9), m_ui8IpLen(0), m_ui8ConnectionLen(0), m_ui8DescriptionLen(0), m_ui8EmailLen(0), m_ui8TagLen(0), m_ui8ClientLen(14), m_ui8TagVersionLen(0),
	m_ui8Country(246), m_ui8State(User::STATE_SOCKET_ACCEPTED), m_ui8IPv4Len(0),
	m_ui8ChangedDescriptionShortLen(0), m_ui8ChangedDescriptionLongLen(0), m_ui8ChangedTagShortLen(0), m_ui8ChangedTagLongLen(0),
	m_ui8ChangedConnectionShortLen(0), m_ui8ChangedConnectionLongLen(0), m_ui8ChangedEmailShortLen(0), m_ui8ChangedEmailLongLen(0),
    m_last_recv_tick(0),m_is_invalid_json(false), m_is_json_user(false)

#ifdef FLYLINKDC_USE_VERSION
	,m_sVersion(NULL)
#endif
{
	m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
	m_ui32BoolBits |= User::BIT_OLDHUBSTAG;
	
	time(&m_tLoginTime);
	
	memset(&m_ui128IpHash, 0, 16);
	
	m_sIP[0] = '\0';
	m_sIPv4[0] = '\0';
	m_sModes[0] = '\0';
#ifdef USE_FLYLINKDC_EXT_JSON
	m_user_ext_info = nullptr;
	m_is_invalid_json = false;
	m_is_json_user = false;
#endif
	m_is_bad_len_port = false;
	m_is_bad_len_number_myinfo = false;
	m_is_bad_port = false;
	m_is_ddos_udp = false;
	m_is_proxy_user = false;
	m_is_max_int8 = false;
	m_is_max_ip_len = false;
}
//---------------------------------------------------------------------------

User::~User()
{
	free(m_pRecvBuf);
	free(m_pSendBuf);
	free(m_sLastChat);
	free(m_sLastPM);
	free(m_sMyInfoShort);
	free(m_sMyInfoLong);
	free(m_sMyInfoOriginal);
#ifdef FLYLINKDC_USE_VERSION
	free(m_sVersion);
#endif
	free(m_sChangedDescriptionShort);
	free(m_sChangedDescriptionLong);
	free(m_sChangedTagShort);
	free(m_sChangedTagLong);
	free(m_sChangedConnectionShort);
	free(m_sChangedConnectionLong);
	free(m_sChangedEmailShort);
	free(m_sChangedEmailLong);
	
	if (((m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true)
		DcCommands::m_Ptr->m_ui32StatZPipe--;
		
	ServerManager::m_ui32Parts++;
	
#ifdef _BUILD_GUI
	if (::SendMessage(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], ("x User removed: " + px_string(m_sNick, m_ui8NickLen) + " (Socket " + px_string(m_Socket) + ")").c_str());
	}
#endif
	
	if (m_sNick != sDefaultNick)
	{
		free(m_sNick);
	}
	
	User::DeletePrcsdUsrCmd(m_pCmdActive4Search);
	User::DeletePrcsdUsrCmd(m_pCmdActive6Search);
	User::DeletePrcsdUsrCmd(m_pCmdPassiveSearch);
	
	PrcsdUsrCmd * cur = NULL,
	              * next = m_pCmdStrt;
	              
	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;
		
		safe_free(cur->m_sCommand);
		
		delete cur;
	}
	
	PrcsdToUsrCmd * curto = NULL,
	                * nextto = m_pCmdToUserStrt;
	                
	while (nextto != NULL)
	{
		curto = nextto;
		nextto = curto->m_pNext;
		
		safe_free(curto->m_sCommand);
		
		delete curto;
	}
	
	
	m_pCmdToUserStrt = nullptr;
	m_pCmdToUserEnd = nullptr;
#ifdef USE_FLYLINKDC_EXT_JSON
	safe_delete(m_user_ext_info);
	m_is_json_user = false;
#endif
}
//---------------------------------------------------------------------------

bool User::MakeLock()
{
	// This code computes the valid Lock string including the Pk= string
	// For maximum speed we just find two random numbers - start and step
	// Step is added each cycle to the start and the ascii 122 boundary is
	// checked. If overflow occurs then the overflowed value is added to the
	// ascii 48 value ("0") and continues.
	// The lock has fixed length 63 bytes
	
#ifdef _WIN32
#ifdef _BUILD_GUI
#ifndef _M_X64
	static const char sLock[] = "$Lock EXTENDEDPROTOCOL                           win Pk=PtokaX|";
#else
	static const char sLock[] = "$Lock EXTENDEDPROTOCOL                           wg6 Pk=PtokaX|";
#endif
#else
#ifndef _M_X64
	static const char sLock[] = "$Lock EXTENDEDPROTOCOL                           wis Pk=PtokaX|";
#elif _M_ARM
	static const char sLock[] = "$Lock EXTENDEDPROTOCOL                           wsa Pk=PtokaX|";
#else
	static const char sLock[] = "$Lock EXTENDEDPROTOCOL                           ws6 Pk=PtokaX|";
#endif
#endif
#else
	static const char sLock[] = "$Lock EXTENDEDPROTOCOL                           nix Pk=PtokaX|";
#endif
	static const size_t szLockLen = sizeof(sLock) - 1;
	
	size_t szAllignLen = Allign1024(m_ui32SendBufDataLen + szLockLen);
	
	char * pOldBuf = m_pSendBuf;
	m_pSendBuf = (char *)realloc(pOldBuf, szAllignLen);
	if (m_pSendBuf == NULL)
	{
		m_pSendBuf = pOldBuf;
		m_ui32BoolBits |= BIT_ERROR;
		
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes in User::MakeLock\n", szAllignLen);
		
		return false;
	}
	m_ui32SendBufLen = (uint32_t)(szAllignLen - 1);
	m_pSendBufHead = m_pSendBuf;
	
	// append data to the buffer
	memcpy(m_pSendBuf, sLock, szLockLen);
	m_ui32SendBufDataLen += szLockLen;
	m_pSendBuf[m_ui32SendBufDataLen] = '\0';
	
	for (uint8_t ui8i = 22; ui8i < 49; ui8i++)
	{
#ifdef _WIN32
		m_pSendBuf[ui8i] = (char)((rand() % 74) + 48);
#else
		m_pSendBuf[ui8i] = (char)((random() % 74) + 48);
#endif
	}
	
//	Memo(string(m_pSendBuf, m_ui32SendBufDataLen));

	m_LogInOut.m_pBuffer = (char *)malloc(64);
	if (m_LogInOut.m_pBuffer == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate 64 bytes for pBuffer in User::MakeLock\n");
		return false;
	}
	
	memcpy(m_LogInOut.m_pBuffer, m_pSendBuf, szLockLen);
	m_LogInOut.m_pBuffer[szLockLen] = '\0';
	
	return true;
}
//---------------------------------------------------------------------------

bool User::DoRecv()
{
	if ((m_ui32BoolBits & BIT_ERROR) == BIT_ERROR || m_ui8State >= STATE_CLOSING)
		return false;
		
#ifdef _WIN32
	u_long iAvailBytes = 0;
	if (ioctlsocket(m_Socket, FIONREAD, &iAvailBytes) == SOCKET_ERROR)
	{
		int iError = WSAGetLastError();
#else
	int iAvailBytes = 0;
	if (ioctl(m_Socket, FIONREAD, &iAvailBytes) == -1)
	{
#endif
		UdpDebug::m_Ptr->BroadcastFormat("[ERR] %s (%s): ioctlsocket(FIONREAD) error %s (%d). User is being closed.", m_sNick, m_sIP,
#ifdef _WIN32
		                                   WSErrorStr(iError), iError);
#else
		                                   ErrnoStr(errno), errno);
#endif
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		return false;
	}
	
	// PPK ... check flood ...
	if (iAvailBytes != 0 && ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NODEFLOODRECV) == false)
	{
		if (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_ACTION] != 0)
		{
			if (m_ui32Recvs == 0)
			{
				m_ui64RecvsTick = ServerManager::m_ui64ActualTick;
			}
			
			m_ui32Recvs += iAvailBytes;
			
			if (DeFloodCheckForDataFlood(this, DEFLOOD_MAX_DOWN, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_ACTION],
			                             m_ui32Recvs, m_ui64RecvsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_KB],
			                             (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_TIME]) == true)
			{
				return false;
			}
			
			if (m_ui32Recvs != 0)
			{
				m_ui32Recvs -= iAvailBytes;
			}
		}
		
		if (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_ACTION2] != 0)
		{
			if (m_ui32Recvs2 == 0)
			{
				m_ui64RecvsTick2 = ServerManager::m_ui64ActualTick;
			}
			
			m_ui32Recvs2 += iAvailBytes;
			
			if (DeFloodCheckForDataFlood(this, DEFLOOD_MAX_DOWN, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_ACTION2],
			                             m_ui32Recvs2, m_ui64RecvsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_KB2],
			                             (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_DOWN_TIME2]) == true)
			{
				return false;
			}
			
			if (m_ui32Recvs2 != 0)
			{
				m_ui32Recvs2 -= iAvailBytes;
			}
		}
	}
	
	if (iAvailBytes == 0)
	{
		const int64_t l_delta = ServerManager::m_ui64ActualTick - m_last_recv_tick;
		if (l_delta > 5 || l_delta < 0)
		{
			m_last_recv_tick = ServerManager::m_ui64ActualTick;
			// we need to try recv to catch connection error or closed connection
			iAvailBytes = 16;
		}
		else
		{
#ifdef _WIN32
			//printf("[%5d] Skip revc %d \r\n", GetTickCount(), m_Socket);
#endif
			return true;
		}
	}
	else if (iAvailBytes > 8 * 1024)
	{
		// receive max. 8k bytes to receive buffer
		iAvailBytes = 8 * 1024;
	}
	
	size_t szAllignLen = 0;
	
	if (m_ui32RecvBufLen < m_ui32RecvBufDataLen + iAvailBytes)
	{
		szAllignLen = Allign512(m_ui32RecvBufDataLen + iAvailBytes);
	}
	else if (m_ui32RecvCalled > 60)
	{
		szAllignLen = Allign512(m_ui32RecvBufDataLen + iAvailBytes);
		if (m_ui32RecvBufLen <= szAllignLen)
		{
			szAllignLen = 0;
		}
		
		m_ui32RecvCalled = 0;
	}
	
	if (szAllignLen != 0)
	{
		char * pOldBuf = m_pRecvBuf;
		
		m_pRecvBuf = (char *)realloc(pOldBuf, szAllignLen);
		if (m_pRecvBuf == NULL)
		{
			m_pRecvBuf = pOldBuf;
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			
			AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes for pRecvBuf in User::DoRecv\n", szAllignLen);
			
			return false;
		}
		
		m_ui32RecvBufLen = (uint32_t)(szAllignLen - 1);
	}
	
	// receive new data to pRecvBuf
	int recvlen = recv(m_Socket, m_pRecvBuf + m_ui32RecvBufDataLen, m_ui32RecvBufLen - m_ui32RecvBufDataLen, 0);
	m_ui32RecvCalled++;
	
#ifdef _WIN32
//#ifdef _DEBUG
	//printf("[%5d] recv %d recvlen = %d\r\n", GetTickCount(), m_Socket, recvlen);
//#endif
	if (recvlen == SOCKET_ERROR)
	{
		int iError = WSAGetLastError();
		if (iError != WSAEWOULDBLOCK)
		{
#else
	if (recvlen == -1)
	{
		if (errno != EAGAIN)
		{
#endif
			UdpDebug::m_Ptr->BroadcastFormat("[ERR] %s (%s): recv() error %s (%d). User is being closed.", m_sNick, m_sIP,
#ifdef _WIN32
			                                   WSErrorStr(iError), iError);
#else
			                                   ErrnoStr(errno), errno);
#endif
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			return false;
		}
		else
		{
			return false;
		}
	}
	else if (recvlen == 0)    // regular close
	{
#ifdef _WIN32
#ifdef _BUILD_GUI
		if (::SendMessage(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "- User has closed the connection: %s (%s)", m_sNick, m_sIP);
			if (iRet > 0)
			{
				RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], ServerManager::m_pGlobalBuffer);
			}
		}
#endif
#endif
		
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		return false;
	}
	
	m_ui32Recvs += recvlen;
	m_ui32Recvs2 += recvlen;
	ServerManager::m_ui64BytesRead += recvlen;
	m_ui32RecvBufDataLen += recvlen;
	m_pRecvBuf[m_ui32RecvBufDataLen] = '\0';
	if (UserProcessLines(this, m_ui32RecvBufDataLen - recvlen) == true)
	{
		return true;
	}
	
	return false;
}
//---------------------------------------------------------------------------

void User::SendChar(const char * sText, const size_t szTextLen)
{
	if (m_ui8State >= STATE_CLOSING || szTextLen == 0)
		return;
		
	if (((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false || szTextLen < ZMINDATALEN)
	{
		if (PutInSendBuf(sText, szTextLen))
		{
			Try2Send();
		}
	}
	else
	{
		uint32_t iLen = 0;
		char *sData = ZlibUtility::m_Ptr->CreateZPipe(sText, szTextLen, iLen);
		
		if (iLen == 0)
		{
			if (PutInSendBuf(sText, szTextLen))
			{
				Try2Send();
			}
		}
		else
		{
			ServerManager::m_ui64BytesSentSaved += szTextLen - iLen;
			if (PutInSendBuf(sData, iLen))
			{
				Try2Send();
			}
		}
	}
}
//---------------------------------------------------------------------------
#ifdef _WIN32
static const char g_badChars[] =
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, '<', '>', '\\', '/', '"', '|', '?', '*', 0
};
#else
static const char g_badChars[] =
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, '<', '>', '\\', '/', '"', '|', '?', '*', 0
};
#endif

#ifdef USE_FLYLINKDC_EXT_JSON
void User::SendCharDelayedExtJSON()
{
	if (isSupportExtJSON())
	{
		if (!Users::m_Ptr->m_AllExtJSON.empty())
		{
			SendCharDelayed(Users::m_Ptr->m_AllExtJSON.c_str(), Users::m_Ptr->m_AllExtJSON.size());
#ifdef _WIN32
#ifdef _PtokaX_TESTING_
			//  string l_log = string(Users::m_Ptr->m_AllExtJSON.c_str(), Users::m_Ptr->m_AllExtJSON.size());
			//  l_log += "\r\n";
			//  AppendDebugLog(l_log.c_str());
#endif
#endif
		}
	}
}
#endif // USE_FLYLINKDC_EXT_JSON
//---------------------------------------------------------------------------

void User::logInvalidUser(int p_id, const char * p_buffer, int p_len, bool p_is_send)
{
	std::string l_name = "/home/dc/src/PtokaX/logs/";
	if (p_is_send)
		l_name += "send-";
	else
		l_name += "recv-";
	l_name += m_sIP;
	l_name += "_";
	if (m_sNick)
	{
		std::string l_nick = m_sNick;
		std::string::size_type i = 0;
		while ((i = l_nick.find_first_of(g_badChars, i)) != std::string::npos)
		{
			l_nick[i] = '_';
			i++;
		}
		l_name += l_nick;
	}
	if (m_is_invalid_json)
		l_name += "-error-json";
	if (m_is_proxy_user)
		l_name += "-proxy-user";
	if (m_is_bad_len_port)
		l_name += "-error-len-port";
	if (m_is_bad_len_number_myinfo)
		l_name += "-error-len-MYINFO";
	if (m_is_bad_port)
		l_name += "-error-port";
	if (m_is_ddos_udp)
		l_name += "-error-DDOS-UDP";
	if (m_is_max_int8)
		l_name += "-error-MAX_INT8";
	if (m_is_max_ip_len)
		l_name += "-error-MAX_IP_LEN";

	l_name += ".log";
	std::ofstream l_file_out(l_name, std::ios_base::app);
	if (!l_file_out.is_open())
	{
#ifndef _WIN32
		syslog(LOG_NOTICE, "Error create %s code = %d", l_name.c_str(), errno);
#endif
		printf("Error create %s code = %d\r\n", l_name.c_str(), errno);
	}
	else
	{
		//l_file_out.seekp(0, std::ios_base::end);
		time_t acc_time;
		time(&acc_time);
		struct tm * acc_tm;
		acc_tm = localtime(&acc_time);
		char sBuf[64];
		sBuf[0] = 0;
		strftime(sBuf, 64, "\r\n%c :::::", acc_tm);
		l_file_out << sBuf << " Nick = [ " << m_sNick <<
			" ] RecvBufDataLen = " << m_ui32RecvBufDataLen <<
			" RecvBufLen = " << m_ui32RecvBufLen <<
			" ] SendBufDataLen = " << m_ui32SendBufDataLen <<
			" SendBufLen = " << m_ui32SendBufLen <<
			" Command: " << p_id << ": ";
		l_file_out.write(p_buffer, p_len);
		//l_file_out << std::endl;
	}

}
//---------------------------------------------------------------------------

void User::SendCharDelayed(const char * sText, const size_t szTextLen)
{
	if (m_ui8State >= STATE_CLOSING || szTextLen == 0)
	{
		return;
	}
	
	if (((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false || szTextLen < ZMINDATALEN)
	{
		PutInSendBuf(sText, szTextLen);
	}
	else
	{
		uint32_t iLen = 0;
		char *sPipeData = ZlibUtility::m_Ptr->CreateZPipe(sText, szTextLen, iLen);
		
		if (iLen == 0)
		{
			PutInSendBuf(sText, szTextLen);
		}
		else
		{
			PutInSendBuf(sPipeData, iLen);
			ServerManager::m_ui64BytesSentSaved += szTextLen - iLen;
		}
	}
}
//---------------------------------------------------------------------------

void User::SendTextDelayed(const string &sText)
{
	SendCharDelayed(sText.c_str(), sText.size());
}

void User::SendFormat(const char * sFrom, const bool bDelayed, const char * sFormatMsg, ...)
{
	if (m_ui8State >= STATE_CLOSING)
	{
		return;
	}
	
	va_list vlArgs;
	va_start(vlArgs, sFormatMsg);
	
	int iRet = vsnprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, sFormatMsg, vlArgs);
	
	va_end(vlArgs);
	
	if (iRet <= 0)
	{
		AppendDebugLogFormat("[ERR] vsnprintf wrong value %d in User::SendFormatDelayed from: %s\n", iRet, sFrom);
		
		return;
	}
	
	if (((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false || (size_t)iRet < ZMINDATALEN)
	{
		if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iRet) == true && bDelayed == false)
		{
			Try2Send();
		}
	}
	else
	{
		uint32_t iLen = 0;
		char *sData = ZlibUtility::m_Ptr->CreateZPipe(ServerManager::m_pGlobalBuffer, iRet, iLen);
		
		if (iLen == 0)
		{
			if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iRet) == true && bDelayed == false)
			{
				Try2Send();
			}
		}
		else
		{
			if (PutInSendBuf(sData, iLen) == true && bDelayed == false)
			{
				Try2Send();
			}
			ServerManager::m_ui64BytesSentSaved += iRet - iLen;
		}
	}
}
//---------------------------------------------------------------------------

void User::SendFormatCheckPM(const char * sFrom, const char * sOtherNick, const bool bDelayed, const char * sFormatMsg, ...)
{
	if (m_ui8State >= STATE_CLOSING)
	{
		return;
	}
	
	int iMsgLen = 0;
	
	if (sOtherNick != NULL)
	{
		iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $", m_sNick, sOtherNick);
		if (iMsgLen <= 0)
		{
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in User::SendFormatCheckPM from: %s\n", iMsgLen, sFrom);
			
			return;
		}
	}
	
	va_list vlArgs;
	va_start(vlArgs, sFormatMsg);
	
	int iRet = vsnprintf(ServerManager::m_pGlobalBuffer + iMsgLen, ServerManager::m_szGlobalBufferSize - iMsgLen, sFormatMsg, vlArgs);
	
	va_end(vlArgs);
	
	if (iRet <= 0)
	{
		AppendDebugLogFormat("[ERR] vsnprintf wrong value %d in User::SendFormatCheckPM from: %s\n", iRet, sFrom);
		
		return;
	}
	
	iMsgLen += iRet;
	
	if (((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) == false || (size_t)iMsgLen < ZMINDATALEN)
	{
		if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iMsgLen) == true && bDelayed == false)
		{
			Try2Send();
		}
	}
	else
	{
		uint32_t iLen = 0;
		char *sData = ZlibUtility::m_Ptr->CreateZPipe(ServerManager::m_pGlobalBuffer, iMsgLen, iLen);
		
		if (iLen == 0)
		{
			if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iMsgLen) == true && bDelayed == false)
			{
				Try2Send();
			}
		}
		else
		{
			if (PutInSendBuf(sData, iLen) == true && bDelayed == false)
			{
				Try2Send();
			}
			ServerManager::m_ui64BytesSentSaved += iMsgLen - iLen;
		}
	}
}
//---------------------------------------------------------------------------

bool User::PutInSendBuf(const char * sText, const size_t szTxtLen)
{
	m_ui32SendCalled++;
	
	size_t szAllignLen = 0;
#ifdef _DEBUG
	if (strstr(Text, "$ExtJSON ") != NULL)
	{
		int a = 0;
		a++;
	}
#endif
	
	if (m_ui32SendBufLen < m_ui32SendBufDataLen + szTxtLen)
	{
		if (m_pSendBuf == NULL)
		{
			szAllignLen = Allign1024(m_ui32SendBufDataLen + szTxtLen);
		}
		else
		{
			if ((size_t)(m_pSendBufHead - m_pSendBuf) > szTxtLen)
			{
				uint32_t offset = (uint32_t)(m_pSendBufHead - m_pSendBuf);
				memmove(m_pSendBuf, m_pSendBufHead, (m_ui32SendBufDataLen - offset));
				m_pSendBufHead = m_pSendBuf;
				m_ui32SendBufDataLen = m_ui32SendBufDataLen - offset;
			}
			else
			{
				szAllignLen = Allign1024(m_ui32SendBufDataLen + szTxtLen);
				size_t szMaxBufLen = (size_t)(((m_ui32BoolBits & BIT_BIG_SEND_BUFFER) == BIT_BIG_SEND_BUFFER) == true ?
				                              ((Users::m_Ptr->m_ui32MyInfosTagLen > Users::m_Ptr->m_ui32MyInfosLen ? Users::m_Ptr->m_ui32MyInfosTagLen : Users::m_Ptr->m_ui32MyInfosLen) * 2) :
				                              (Users::m_Ptr->m_ui32MyInfosTagLen > Users::m_Ptr->m_ui32MyInfosLen ? Users::m_Ptr->m_ui32MyInfosTagLen : Users::m_Ptr->m_ui32MyInfosLen));
				szMaxBufLen = szMaxBufLen < 262144 ? 262144 : szMaxBufLen;
				if (szAllignLen > szMaxBufLen)
				{
					// does the buffer size reached the maximum
					if (SettingManager::m_Ptr->m_bBools[SETBOOL_KEEP_SLOW_USERS] == false || (m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE)
					{
						// we want to drop the slow user
						m_ui32BoolBits |= BIT_ERROR;
						Close();
						
						UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s) SendBuffer overflow (AL:%zu[SL:%u|NL:%zu|FL:%zu]/ML:%zu). User disconnected.",
						                                 m_sNick, m_sIP, szAllignLen, m_ui32SendBufDataLen, szTxtLen, m_pSendBufHead-m_pSendBuf, szMaxBufLen);
						return false;
					}
					else
					{
						UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s) SendBuffer overflow (AL:%zu[SL:%u|NL:%zu|FL:%zu]/ML:%zu). Buffer cleared - user stays online.",
						                                 m_sNick, m_sIP, szAllignLen, m_ui32SendBufDataLen, szTxtLen, m_pSendBufHead-m_pSendBuf, szMaxBufLen);
					}
					
					// we want to keep the slow user online
					// PPK ... i don't want to corrupt last command, get rest of it and add to new buffer ;)
					char *sTemp = (char *)memchr(m_pSendBufHead, '|', m_ui32SendBufDataLen - (m_pSendBufHead - m_pSendBuf));
					if (sTemp != NULL)
					{
						uint32_t iOldSBDataLen = m_ui32SendBufDataLen;
						
						uint32_t iRestCommandLen = (uint32_t)((sTemp - m_pSendBufHead) + 1);
						if (m_pSendBuf != m_pSendBufHead)
						{
							memmove(m_pSendBuf, m_pSendBufHead, iRestCommandLen);
						}
						m_ui32SendBufDataLen = iRestCommandLen;
						
						// If is not needed then don't lost all data, try to find some space with removing only few oldest commands
						if (szTxtLen < szMaxBufLen && iOldSBDataLen > (uint32_t)((sTemp + 1) - m_pSendBuf) && (iOldSBDataLen - ((sTemp + 1) - m_pSendBuf)) > (uint32_t)szTxtLen)
						{
							char *sTemp1;
							// try to remove min half of send bufer
							if (iOldSBDataLen > (m_ui32SendBufLen / 2) && (uint32_t)((sTemp + 1 + szTxtLen) - m_pSendBuf) < (m_ui32SendBufLen / 2))
							{
								sTemp1 = (char *)memchr(m_pSendBuf + (m_ui32SendBufLen / 2), '|', iOldSBDataLen - (m_ui32SendBufLen / 2));
							}
							else
							{
								sTemp1 = (char *)memchr(sTemp + 1 + szTxtLen, '|', iOldSBDataLen - ((sTemp + 1 + szTxtLen) - m_pSendBuf));
							}
							
							if (sTemp1 != NULL)
							{
								iRestCommandLen = (uint32_t)(iOldSBDataLen - ((sTemp1 + 1) - m_pSendBuf));
								memmove(m_pSendBuf + m_ui32SendBufDataLen, sTemp1 + 1, iRestCommandLen);
								m_ui32SendBufDataLen += iRestCommandLen;
							}
						}
					}
					else
					{
						m_pSendBuf[0] = '|';
						m_pSendBuf[1] = '\0';
						m_ui32SendBufDataLen = 1;
					}
					
					size_t szAllignTxtLen = Allign1024(szTxtLen + m_ui32SendBufDataLen);
					
					char * pOldBuf = m_pSendBuf;
					m_pSendBuf = (char *)realloc(pOldBuf, szAllignTxtLen);
					if (m_pSendBuf == NULL)
					{
						m_pSendBuf = pOldBuf;
						m_ui32BoolBits |= BIT_ERROR;
						Close();
						
						AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in User::PutInSendBuf-keepslow\n", szAllignLen);
						
						return false;
					}
					m_ui32SendBufLen = (uint32_t)(szAllignTxtLen - 1);
					m_pSendBufHead = m_pSendBuf;
					
					szAllignLen = 0;
				}
				else
				{
					szAllignLen = Allign1024(m_ui32SendBufDataLen + szTxtLen);
				}
			}
		}
	}
	else if (m_ui32SendCalled > 100)
	{
		szAllignLen = Allign1024(m_ui32SendBufDataLen + szTxtLen);
		if (m_ui32SendBufLen <= szAllignLen)
		{
			szAllignLen = 0;
		}
		
		m_ui32SendCalled = 0;
	}
	
	if (szAllignLen != 0)
	{
		const uint32_t offset = (m_pSendBuf == NULL ? 0 : (uint32_t)(m_pSendBufHead - m_pSendBuf));
		
		char * pOldBuf = m_pSendBuf;
		m_pSendBuf = (char *)realloc(pOldBuf, szAllignLen);
		if (m_pSendBuf == NULL)
		{
			m_pSendBuf = pOldBuf;
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			
			AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes for new pSendBuf in User::PutInSendBuf\n", szAllignLen);
			
			return false;
		}
		
		m_ui32SendBufLen = (uint32_t)(szAllignLen - 1);
		m_pSendBufHead = m_pSendBuf + offset;
	}
	
	// append data to the buffer
	memcpy(m_pSendBuf + m_ui32SendBufDataLen, sText, szTxtLen);
	m_ui32SendBufDataLen += (uint32_t)szTxtLen;
	m_pSendBuf[m_ui32SendBufDataLen] = '\0';
	
	return true;
}
//---------------------------------------------------------------------------

bool User::Try2Send()
{
	if ((m_ui32BoolBits & BIT_ERROR) == BIT_ERROR || m_ui32SendBufDataLen == 0)
	{
		return false;
	}
	
	// compute length of unsent data
	int32_t offset = (int32_t)(m_pSendBufHead - m_pSendBuf);
	int32_t len = m_ui32SendBufDataLen - offset;
	
	if (offset < 0 || len < 0)
	{
		AppendDebugLogFormat("[ERR] Negative send values!\nSendBuf: %p\nPlayHead: %p\nDataLen: %u\n", m_pSendBuf, m_pSendBufHead, m_ui32SendBufDataLen);
		
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		return false;
	}
#ifdef _DEBUG
	if (strstr(m_pSendBufHead, "$ExtJSON ") != NULL)
	{
		int a = 0;
		a++;
	}
#endif
	
	const int n = send(m_Socket, m_pSendBufHead, len < 32768 ? len : 32768, 0);
	
#ifdef _WIN32
	if (n == SOCKET_ERROR)
	{
		int iError = WSAGetLastError();
		if (iError != WSAEWOULDBLOCK)
		{
#else
	if (n == -1)
	{
		if (errno != EAGAIN)
		{
#endif
			UdpDebug::m_Ptr->BroadcastFormat("[ERR] %s (%s): send() error %s (%d). User is being closed.", m_sNick, m_sIP,
#ifdef _WIN32
			                                   WSErrorStr(iError), iError);
#else
			                                   ErrnoStr(errno), errno);
#endif
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			return false;
		}
		else
		{
			return true;
		}
	}
	
	ServerManager::m_ui64BytesSent += n;
	static int g_id = 0;
	extern bool g_isUseLog;
	if (g_isUseLog)
	{
		logInvalidUser(++g_id, m_pSendBufHead, len, true);
	}
	
	// if buffer is sent then mark it as empty (first byte = 0)
	// else move remaining data on new place and free old buffer
	if (n < len)
	{
		m_pSendBufHead += n;
		return true;
	}
	else
	{
		// PPK ... we need to free memory allocated for big buffer on login (userlist, motd...)
		if (((m_ui32BoolBits & BIT_BIG_SEND_BUFFER) == BIT_BIG_SEND_BUFFER) == true
		        ||
		        m_ui32SendBufLen > 1024
		   )
		{
			if (m_pSendBuf != NULL)
			{
				safe_free(m_pSendBuf);
				m_pSendBufHead = m_pSendBuf;
				m_ui32SendBufLen = 0;
				m_ui32SendBufDataLen = 0;
			}
			m_ui32BoolBits &= ~BIT_BIG_SEND_BUFFER;
		}
		else
		{
			m_pSendBuf[0] = '\0';
			m_pSendBufHead = m_pSendBuf;
			m_ui32SendBufDataLen = 0;
		}
		return false;
	}
}
//---------------------------------------------------------------------------

void User::SetIP(const char * sIP)
{
	strcpy(m_sIP, sIP);
	m_ui8IpLen = (uint8_t)strlen(sIP);
}
//------------------------------------------------------------------------------

void User::SetNick(const char * sNick, const uint8_t ui8NickLen)
{
	if (m_sNick != sDefaultNick && m_sNick != NULL)
	{
		safe_free(m_sNick);
	}
	
	m_sNick = (char *)malloc(ui8NickLen + 1);
	if (m_sNick == NULL)
	{
		m_sNick = (char *)sDefaultNick;
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for m_sNick in User::SetNick\n", m_ui8NickLen + 1);
		
		return;
	}
	memcpy(m_sNick, sNick, ui8NickLen);
	m_sNick[ui8NickLen] = '\0';
	m_ui8NickLen = ui8NickLen;
	m_ui32NickHash = HashNick(m_sNick, m_ui8NickLen);
}
//------------------------------------------------------------------------------
#ifdef USE_FLYLINKDC_EXT_JSON
void User::SetExtJSONOriginal(const char * sNewExtJSON, const uint16_t ui16NewExtJSONLen)
{
	if (m_user_ext_info)
	{
		Users::m_Ptr->DelFromExtJSONInfos(this);
		std::string l_info(sNewExtJSON, ui16NewExtJSONLen);
		m_user_ext_info->SetJSONOriginal(l_info);
		Users::m_Ptr->Add2ExtJSON(this);
	}
}
#endif
//------------------------------------------------------------------------------

void User::SetMyInfoOriginal(const char * sMyInfo, const uint16_t ui16MyInfoLen)
{
	char * sOldMyInfo = m_sMyInfoOriginal;
	
	const char * sOldDescription = m_sDescription;
	uint8_t ui8OldDescriptionLen = m_ui8DescriptionLen;
	
	char * sOldTag = m_sTag;
	uint8_t ui8OldTagLen = m_ui8TagLen;
	
	char * sOldConnection = m_sConnection;
	uint8_t ui8OldConnectionLen = m_ui8ConnectionLen;
	
	char * sOldEmail = m_sEmail;
	uint8_t ui8OldEmailLen = m_ui8EmailLen;
	
	uint64_t ui64OldShareSize = m_ui64SharedSize;
	
	if (m_sMyInfoOriginal != NULL)
	{
		m_sConnection = NULL;
		m_ui8ConnectionLen = 0;
		
		m_sDescription = NULL;
		m_ui8DescriptionLen = 0;
		
		m_sEmail = NULL;
		m_ui8EmailLen = 0;
		
		m_sTag = NULL;
		m_ui8TagLen = 0;
		
		m_sClient = NULL;
		m_ui8ClientLen = 0;
		
		m_sTagVersion = NULL;
		m_ui8TagVersionLen = 0;
		
		m_sMyInfoOriginal = NULL;
	}
	
	m_sMyInfoOriginal = (char *)malloc(ui16MyInfoLen + 1);
	if (m_sMyInfoOriginal == NULL)
	{
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for m_sMyInfoOriginal in UserSetMyInfoOriginal\n", ui16MyInfoLen + 1);
		
		return;
	}
	memcpy(m_sMyInfoOriginal, sMyInfo, ui16MyInfoLen);
	m_sMyInfoOriginal[ui16MyInfoLen] = '\0';
	m_ui16MyInfoOriginalLen = ui16MyInfoLen;
	
	UserParseMyInfo(this);
	
	if (ui8OldDescriptionLen != m_ui8DescriptionLen || (m_ui8DescriptionLen > 0 && memcmp(sOldDescription, m_sDescription, m_ui8DescriptionLen) != 0))
	{
		m_ui32InfoBits |= INFOBIT_DESCRIPTION_CHANGED;
	}
	else
	{
		m_ui32InfoBits &= ~INFOBIT_DESCRIPTION_CHANGED;
	}
	
	if (ui8OldTagLen != m_ui8TagLen || (m_ui8TagLen > 0 && memcmp(sOldTag, m_sTag, m_ui8TagLen) != 0))
	{
		m_ui32InfoBits |= INFOBIT_TAG_CHANGED;
	}
	else
	{
		m_ui32InfoBits &= ~INFOBIT_TAG_CHANGED;
	}
	
	if (ui8OldConnectionLen != m_ui8ConnectionLen || (m_ui8ConnectionLen > 0 && memcmp(sOldConnection, m_sConnection, m_ui8ConnectionLen) != 0))
	{
		m_ui32InfoBits |= INFOBIT_CONNECTION_CHANGED;
	}
	else
	{
		m_ui32InfoBits &= ~INFOBIT_CONNECTION_CHANGED;
	}
	
	if (ui8OldEmailLen != m_ui8EmailLen || (m_ui8EmailLen > 0 && memcmp(sOldEmail, m_sEmail, m_ui8EmailLen) != 0))
	{
		m_ui32InfoBits |= INFOBIT_EMAIL_CHANGED;
	}
	else
	{
		m_ui32InfoBits &= ~INFOBIT_EMAIL_CHANGED;
	}
	
	if (ui64OldShareSize != m_ui64SharedSize)
	{
		m_ui32InfoBits |= INFOBIT_SHARE_CHANGED;
	}
	else
	{
		m_ui32InfoBits &= ~INFOBIT_SHARE_CHANGED;
	}
	
	safe_free(sOldMyInfo);
	
	if (((m_ui32InfoBits & INFOBIT_SHARE_SHORT_PERM) == INFOBIT_SHARE_SHORT_PERM) == false)
	{
		m_ui64ChangedSharedSizeShort = m_ui64SharedSize;
	}
	
	if (((m_ui32InfoBits & INFOBIT_SHARE_LONG_PERM) == INFOBIT_SHARE_LONG_PERM) == false)
	{
		m_ui64ChangedSharedSizeLong = m_ui64SharedSize;
	}
	
}
//------------------------------------------------------------------------------

static void UserSetMyInfoLong(User * pUser, char * sMyInfoLong, const uint16_t &ui16MyInfoLongLen)
{
	if (pUser->m_sMyInfoLong != NULL)
	{
		if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 2)
		{
			Users::m_Ptr->DelFromMyInfosTag(pUser);
		}
		
		safe_free(pUser->m_sMyInfoLong);
		
	}
	
	pUser->m_sMyInfoLong = (char *)malloc(ui16MyInfoLongLen + 1);
	if (pUser->m_sMyInfoLong == NULL)
	{
		pUser->m_ui32BoolBits |= User::BIT_ERROR;
		pUser->Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for m_sMyInfoLong in UserSetMyInfoLong\n", ui16MyInfoLongLen + 1);
		
		return;
	}
	memcpy(pUser->m_sMyInfoLong, sMyInfoLong, ui16MyInfoLongLen);
	pUser->m_sMyInfoLong[ui16MyInfoLongLen] = '\0';
	pUser->m_ui16MyInfoLongLen = ui16MyInfoLongLen;
}
//------------------------------------------------------------------------------

static void UserSetMyInfoShort(User * pUser, char * sMyInfoShort, const uint16_t &ui16MyInfoShortLen)
{
	if (pUser->m_sMyInfoShort != NULL)
	{
		if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 0)
		{
			Users::m_Ptr->DelFromMyInfos(pUser);
		}
		
		safe_free(pUser->m_sMyInfoShort);
	}
	
	pUser->m_sMyInfoShort = (char *)malloc(ui16MyInfoShortLen + 1);
	if (pUser->m_sMyInfoShort == NULL)
	{
		pUser->m_ui32BoolBits |= User::BIT_ERROR;
		pUser->Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for m_sMyInfoShort in UserSetMyInfoShort\n", ui16MyInfoShortLen + 1);
		
		return;
	}
	memcpy(pUser->m_sMyInfoShort, sMyInfoShort, ui16MyInfoShortLen);
	pUser->m_sMyInfoShort[ui16MyInfoShortLen] = '\0';
	pUser->m_ui16MyInfoShortLen = ui16MyInfoShortLen;
}
//------------------------------------------------------------------------------

void User::SetVersion(const char * /* sVersion*/)
{
#ifdef FLYLINKDC_USE_VERSION
	free(m_sVersion);
	size_t szLen = strlen(sNewVer);
	m_sVersion = (char *)malloc(szLen + 1);
	if (m_sVersion == NULL)
	{
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sVersion in User::SetVersion\n", szLen+1);
		
		return;
	}
	memcpy(m_sVersion, sNewVer, szLen);
	m_sVersion[szLen] = '\0';
#endif
}
//------------------------------------------------------------------------------

void User::SetLastChat(const char * sData, const size_t szLen)
{
	free(m_sLastChat);
	
	m_sLastChat = (char *)malloc(szLen + 1);
	if (m_sLastChat == NULL)
	{
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sLastChat in User::SetLastChat\n", szLen+1);
		
		return;
	}
	memcpy(m_sLastChat, sData, szLen);
	m_sLastChat[szLen] = '\0';
	m_ui16SameChatMsgs = 1;
	m_ui64SameChatsTick = ServerManager::m_ui64ActualTick;
	m_ui16LastChatLen = (uint16_t)szLen;
	m_ui16SameMultiChats = 0;
	m_ui16LastChatLines = 0;
}
//------------------------------------------------------------------------------

void User::SetLastPM(const char * sData, const size_t szLen)
{
	free(m_sLastPM);
	
	m_sLastPM = (char *)malloc(szLen + 1);
	if (m_sLastPM == NULL)
	{
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sLastPM in User::SetLastPM\n", szLen+1);
		
		return;
	}
	
	memcpy(m_sLastPM, sData, szLen);
	m_sLastPM[szLen] = '\0';
	m_ui16SamePMs = 1;
	m_ui64SamePMsTick = ServerManager::m_ui64ActualTick;
	m_ui16LastPMLen = (uint16_t)szLen;
	m_ui16SameMultiPms = 0;
	m_ui16LastPmLines = 0;
}
//------------------------------------------------------------------------------

void User::SetLastSearch(const char * sData, const size_t szLen)
{
	m_LastSearch = std::string(sData, szLen);
	m_ui16SameSearchs = 1;
	m_ui64SameSearchsTick = ServerManager::m_ui64ActualTick;
}
//------------------------------------------------------------------------------

void User::SetBuffer(const char * sKickMsg, size_t szLen /* = 0*/)
{
	if (szLen == 0)
	{
		szLen = strlen(sKickMsg);
	}
	
	void * pOldBuf = m_LogInOut.m_pBuffer;
	
	if (szLen < 512)
	{
		m_LogInOut.m_pBuffer = (char *)realloc(pOldBuf, szLen + 1);
		if (m_LogInOut.m_pBuffer == NULL)
		{
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			
			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for pBuffer in User::SetBuffer\n", szLen+1);
			
			return;
		}
		memcpy(m_LogInOut.m_pBuffer, sKickMsg, szLen);
		m_LogInOut.m_pBuffer[szLen] = '\0';
	}
	else
	{
		m_LogInOut.m_pBuffer = (char *)realloc(pOldBuf, 512);
		if (m_LogInOut.m_pBuffer == NULL)
		{
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			
			AppendDebugLog("%s - [MEM] Cannot allocate 512 bytes for pBuffer in User::SetBuffer\n");
			
			return;
		}
		memcpy(m_LogInOut.m_pBuffer, sKickMsg, 508);
		m_LogInOut.m_pBuffer[510] = '.';
		m_LogInOut.m_pBuffer[509] = '.';
		m_LogInOut.m_pBuffer[508] = '.';
		m_LogInOut.m_pBuffer[511] = '\0';
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void User::FreeBuffer()
{
	safe_free(m_LogInOut.m_pBuffer);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void User::Close(const bool bNoQuit/* = false*/)
{
	m_is_json_user = false;
	if (m_ui8State >= STATE_CLOSING)
	{
		return;
	}
	
	// nick in hash table ?
	if ((m_ui32BoolBits & BIT_HASHED) == BIT_HASHED)
	{
		HashManager::m_Ptr->Remove(this);
	}
	
	// nick in nick/op list ?
	if (m_ui8State >= STATE_ADDME_2LOOP)
	{
		Users::m_Ptr->DelFromNickList(m_sNick, (m_ui32BoolBits & BIT_OPERATOR) == BIT_OPERATOR);
		Users::m_Ptr->DelFromUserIP(this);
		
		// PPK ... fix for QuickList nad ghost...
		// and fixing redirect all too ;)
		// and fix disconnect on send error too =)
		if (bNoQuit == false)
		{
			// alex82 ... HideUser / ������� �����
			// alex82 ... NoQuit / ��������� $Quit ��� �����
			if (((m_ui32InfoBits & INFOBIT_HIDDEN) == INFOBIT_HIDDEN) == false && ((m_ui32InfoBits & INFOBIT_NO_QUIT) == INFOBIT_NO_QUIT) == false)
			{
			int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", m_sNick);
				if (CheckSprintf(iMsgLen, ServerManager::m_szGlobalBufferSize, "User::Close") == true)
			{
				GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
			}
			}
			Users::m_Ptr->Add2RecTimes(this);
		}
		
#ifdef _BUILD_GUI
		if (::SendMessage(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			MainWindowPageUsersChat::m_Ptr->RemoveUser(this);
		}
#endif
		
		//sqldb->FinalizeVisit(u);
#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
		DBSQLite::m_Ptr->UpdateRecord(this);
#elif _WITH_POSTGRES
		DBPostgreSQL::m_Ptr->UpdateRecord(this);
#elif _WITH_MYSQL
		DBMySQL::m_Ptr->UpdateRecord(this);
#endif
#endif
		if (((m_ui32BoolBits & BIT_HAVE_SHARECOUNTED) == BIT_HAVE_SHARECOUNTED) == true)
		{
			ServerManager::m_ui64TotalShare -= m_ui64SharedSize;
			m_ui32BoolBits &= ~BIT_HAVE_SHARECOUNTED;
		}
		
		ScriptManager::m_Ptr->UserDisconnected(this);
	}
	
	// + alex82 ... HideUser / ������� �����
	if (((m_ui32InfoBits & INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN) == false && m_ui8State > STATE_ADDME_2LOOP)
	{
		ServerManager::m_ui32Logged--;
	}
	
	m_ui8State = STATE_CLOSING;
	
	User::DeletePrcsdUsrCmd(m_pCmdActive4Search);
	User::DeletePrcsdUsrCmd(m_pCmdActive6Search);
	User::DeletePrcsdUsrCmd(m_pCmdPassiveSearch);
	
	PrcsdUsrCmd * cur = NULL,
	              * next = m_pCmdStrt;
	              
	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;
		
		safe_free(cur->m_sCommand);
		
		delete cur;
	}
	
	m_pCmdStrt = NULL;
	m_pCmdEnd = NULL;
	
	PrcsdToUsrCmd * curto = NULL,
	                * nextto = m_pCmdToUserStrt;
	                
	while (nextto != NULL)
	{
		curto = nextto;
		nextto = curto->m_pNext;
		
		safe_free(curto->m_sCommand);
		
		delete curto;
	}
	
	
	m_pCmdToUserStrt = NULL;
	m_pCmdToUserEnd = NULL;
	
	if (m_sMyInfoLong)
	{
		if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 2)
		{
			Users::m_Ptr->DelFromMyInfosTag(this);
		}
	}
	
	if (m_sMyInfoShort)
	{
		if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 0)
		{
			Users::m_Ptr->DelFromMyInfos(this);
		}
	}
	
#ifdef USE_FLYLINKDC_EXT_JSON
	Users::m_Ptr->DelFromExtJSONInfos(this);
#endif
	
	if (m_ui32SendBufDataLen == 0 || (m_ui32BoolBits & BIT_ERROR) == BIT_ERROR)
	{
		m_ui8State = STATE_REMME;
	}
	else
	{
		m_LogInOut.m_ui32ToCloseLoops = 100;
	}
}
//---------------------------------------------------------------------------

void User::Add2Userlist()
{
	Users::m_Ptr->Add2NickList(this);
	Users::m_Ptr->Add2UserIP(this);
	
	switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
	{
		case 0:
		{
			GenerateMyInfoLong();
			Users::m_Ptr->Add2MyInfosTag(this);
			return;
		}
		case 1:
		{
			GenerateMyInfoLong();
			Users::m_Ptr->Add2MyInfosTag(this);
			GenerateMyInfoShort();
			Users::m_Ptr->Add2MyInfos(this);
			return;
		}
		case 2:
		{
			GenerateMyInfoShort();
			Users::m_Ptr->Add2MyInfos(this);
			return;
		}
		default:
			break;
	}
}
//------------------------------------------------------------------------------

void User::AddUserList()
{
	m_ui32BoolBits |= BIT_BIG_SEND_BUFFER;
	m_ui64LastNicklist = ServerManager::m_ui64ActualTick;
	
	if (((m_ui32SupportBits & SUPPORTBIT_NOHELLO) == SUPPORTBIT_NOHELLO) == false)
	{
		if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::ALLOWEDOPCHAT) == false || (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == false ||
		        (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true && SettingManager::m_Ptr->m_bBotsSameNick == true)))
		{
			if (isSupportZpipe() == false)
			{
				SendCharDelayed(Users::m_Ptr->m_pNickList, Users::m_Ptr->m_ui32NickListLen);
			}
			else
			{
				if (Users::m_Ptr->m_ui32ZNickListLen == 0)
				{
					Users::m_Ptr->m_pZNickList = ZlibUtility::m_Ptr->CreateZPipeAlign(Users::m_Ptr->m_pNickList, Users::m_Ptr->m_ui32NickListLen, Users::m_Ptr->m_pZNickList,
					                                                                    Users::m_Ptr->m_ui32ZNickListLen, Users::m_Ptr->m_ui32ZNickListSize);
					if (Users::m_Ptr->m_ui32ZNickListLen == 0)
					{
						SendCharDelayed(Users::m_Ptr->m_pNickList, Users::m_Ptr->m_ui32NickListLen);
					}
					else
					{
						PutInSendBuf(Users::m_Ptr->m_pZNickList, Users::m_Ptr->m_ui32ZNickListLen);
						ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32NickListLen - Users::m_Ptr->m_ui32ZNickListLen;
					}
				}
				else
				{
					PutInSendBuf(Users::m_Ptr->m_pZNickList, Users::m_Ptr->m_ui32ZNickListLen);
					ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32NickListLen - Users::m_Ptr->m_ui32ZNickListLen;
				}
			}
		}
		else
		{
			// PPK ... OpChat bot is now visible only for OPs ;)
			int iLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s$$|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
			if (iLen > 0)
			{
				if (Users::m_Ptr->m_ui32NickListSize < Users::m_Ptr->m_ui32NickListLen + iLen)
				{
					char * pOldBuf = Users::m_Ptr->m_pNickList;
					Users::m_Ptr->m_pNickList = (char *)realloc(pOldBuf, Users::m_Ptr->m_ui32NickListSize + NICKLISTSIZE + 1);
					if (Users::m_Ptr->m_pNickList == NULL)
					{
						Users::m_Ptr->m_pNickList = pOldBuf;
						m_ui32BoolBits |= BIT_ERROR;
						Close();
						
						AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes for m_pNickList in User::AddUserList\n", Users::m_Ptr->m_ui32NickListSize + NICKLISTSIZE + 1);
						
						return;
					}
					Users::m_Ptr->m_ui32NickListSize += NICKLISTSIZE;
				}
				
				memcpy(Users::m_Ptr->m_pNickList + Users::m_Ptr->m_ui32NickListLen - 1, ServerManager::m_pGlobalBuffer, iLen);
				Users::m_Ptr->m_pNickList[Users::m_Ptr->m_ui32NickListLen + (iLen - 1)] = '\0';
				SendCharDelayed(Users::m_Ptr->m_pNickList, Users::m_Ptr->m_ui32NickListLen + (iLen - 1));
				Users::m_Ptr->m_pNickList[Users::m_Ptr->m_ui32NickListLen - 1] = '|';
				Users::m_Ptr->m_pNickList[Users::m_Ptr->m_ui32NickListLen] = '\0';
			}
		}
	}
	
	switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
	{
		case 0:
		{
#ifdef USE_FLYLINKDC_EXT_JSON
			// TODO ExtJSON - ZPipe
			SendCharDelayedExtJSON();
#endif
			if (Users::m_Ptr->m_ui32MyInfosTagLen == 0)
			{
				break;
			}
			
			if (isSupportZpipe() == false)
			{
				SendCharDelayed(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen);
			}
			else
			{
				if (Users::m_Ptr->m_ui32ZMyInfosTagLen == 0)
				{
					Users::m_Ptr->m_pZMyInfosTag = ZlibUtility::m_Ptr->CreateZPipeAlign(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen, Users::m_Ptr->m_pZMyInfosTag,
					                                                                      Users::m_Ptr->m_ui32ZMyInfosTagLen, Users::m_Ptr->m_ui32ZMyInfosTagSize);
					if (Users::m_Ptr->m_ui32ZMyInfosTagLen == 0)
					{
						SendCharDelayed(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen);
					}
					else
					{
						PutInSendBuf(Users::m_Ptr->m_pZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
						ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosTagLen - Users::m_Ptr->m_ui32ZMyInfosTagLen;
					}
				}
				else
				{
					PutInSendBuf(Users::m_Ptr->m_pZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
					ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosTagLen - Users::m_Ptr->m_ui32ZMyInfosTagLen;
				}
			}
			break;
		}
		case 1:
		{
#ifdef USE_FLYLINKDC_EXT_JSON
			// TODO ExtJSON - ZPipe
			// ExtJSON Step reconnect - 1
			SendCharDelayedExtJSON();
#endif
			if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::SENDFULLMYINFOS) == false)
			{
				if (Users::m_Ptr->m_ui32MyInfosLen == 0)
				{
					break;
				}
				
				if (isSupportZpipe() == false)
				{
					SendCharDelayed(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen);
				}
				else
				{
					if (Users::m_Ptr->m_ui32ZMyInfosLen == 0)
					{
						Users::m_Ptr->m_pZMyInfos = ZlibUtility::m_Ptr->CreateZPipeAlign(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen, Users::m_Ptr->m_pZMyInfos,
						                                                                   Users::m_Ptr->m_ui32ZMyInfosLen, Users::m_Ptr->m_ui32ZMyInfosSize);
						if (Users::m_Ptr->m_ui32ZMyInfosLen == 0)
						{
							SendCharDelayed(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen);
						}
						else
						{
							PutInSendBuf(Users::m_Ptr->m_pZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
							ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosLen - Users::m_Ptr->m_ui32ZMyInfosLen;
						}
					}
					else
					{
						PutInSendBuf(Users::m_Ptr->m_pZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
						ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosLen - Users::m_Ptr->m_ui32ZMyInfosLen;
					}
				}
			}
			else
			{
			
#ifdef USE_FLYLINKDC_EXT_JSON
				// TODO ExtJSON
#endif
				if (Users::m_Ptr->m_ui32MyInfosTagLen == 0)
				{
					break;
				}
				if (isSupportZpipe() == false)
				{
					SendCharDelayed(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen);
				}
				else
				{
					if (Users::m_Ptr->m_ui32ZMyInfosTagLen == 0)
					{
						Users::m_Ptr->m_pZMyInfosTag = ZlibUtility::m_Ptr->CreateZPipeAlign(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen, Users::m_Ptr->m_pZMyInfosTag,
						                                                                      Users::m_Ptr->m_ui32ZMyInfosTagLen, Users::m_Ptr->m_ui32ZMyInfosTagSize);
						if (Users::m_Ptr->m_ui32ZMyInfosTagLen == 0)
						{
							SendCharDelayed(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen);
						}
						else
						{
							PutInSendBuf(Users::m_Ptr->m_pZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
							ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosTagLen - Users::m_Ptr->m_ui32ZMyInfosTagLen;
						}
					}
					else
					{
						PutInSendBuf(Users::m_Ptr->m_pZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
						ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosTagLen - Users::m_Ptr->m_ui32ZMyInfosTagLen;
					}
				}
			}
			break;
		}
		case 2:
		{
#ifdef USE_FLYLINKDC_EXT_JSON
			// TODO ExtJSON ZPipe
			SendCharDelayedExtJSON();
#endif
			if (Users::m_Ptr->m_ui32MyInfosLen == 0)
			{
				break;
			}
			
			if (isSupportZpipe() == false)
			{
				SendCharDelayed(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen);
			}
			else
			{
				if (Users::m_Ptr->m_ui32ZMyInfosLen == 0)
				{
					Users::m_Ptr->m_pZMyInfos = ZlibUtility::m_Ptr->CreateZPipeAlign(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen, Users::m_Ptr->m_pZMyInfos,
					                                                                   Users::m_Ptr->m_ui32ZMyInfosLen, Users::m_Ptr->m_ui32ZMyInfosSize);
					if (Users::m_Ptr->m_ui32ZMyInfosLen == 0)
					{
						SendCharDelayed(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen);
					}
					else
					{
						PutInSendBuf(Users::m_Ptr->m_pZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
						ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosLen - Users::m_Ptr->m_ui32ZMyInfosLen;
					}
				}
				else
				{
					PutInSendBuf(Users::m_Ptr->m_pZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
					ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosLen - Users::m_Ptr->m_ui32ZMyInfosLen;
				}
			}
		}
		default:
			break;
	}
	
	if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::ALLOWEDOPCHAT) == false || (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == false ||
	        (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true && SettingManager::m_Ptr->m_bBotsSameNick == true)))
	{
		if (Users::m_Ptr->m_ui32OpListLen > 9)
		{
			if (isSupportZpipe() == false)
			{
				SendCharDelayed(Users::m_Ptr->m_pOpList, Users::m_Ptr->m_ui32OpListLen);
			}
			else
			{
				if (Users::m_Ptr->m_ui32ZOpListLen == 0)
				{
					Users::m_Ptr->m_pZOpList = ZlibUtility::m_Ptr->CreateZPipeAlign(Users::m_Ptr->m_pOpList, Users::m_Ptr->m_ui32OpListLen, Users::m_Ptr->m_pZOpList,
					                                                                  Users::m_Ptr->m_ui32ZOpListLen, Users::m_Ptr->m_ui32ZOpListSize);
					if (Users::m_Ptr->m_ui32ZOpListLen == 0)
					{
						SendCharDelayed(Users::m_Ptr->m_pOpList, Users::m_Ptr->m_ui32OpListLen);
					}
					else
					{
						PutInSendBuf(Users::m_Ptr->m_pZOpList, Users::m_Ptr->m_ui32ZOpListLen);
						ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32OpListLen - Users::m_Ptr->m_ui32ZOpListLen;
					}
				}
				else
				{
					PutInSendBuf(Users::m_Ptr->m_pZOpList, Users::m_Ptr->m_ui32ZOpListLen);
					ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32OpListLen - Users::m_Ptr->m_ui32ZOpListLen;
				}
			}
		}
	}
	else
	{
		// PPK ... OpChat bot is now visible only for OPs ;)
		SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_MYINFO],
		                SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_MYINFO]);
		int iLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s$$|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
		if (iLen > 0)
		{
			if (Users::m_Ptr->m_ui32OpListSize < Users::m_Ptr->m_ui32OpListLen + iLen)
			{
				char * pOldBuf = Users::m_Ptr->m_pOpList;
				Users::m_Ptr->m_pOpList = (char *)realloc(pOldBuf, Users::m_Ptr->m_ui32OpListSize + OPLISTSIZE + 1);
				if (Users::m_Ptr->m_pOpList == NULL)
				{
					Users::m_Ptr->m_pOpList = pOldBuf;
					m_ui32BoolBits |= BIT_ERROR;
					Close();
					
					AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes for m_pOpList in User::AddUserList\n", Users::m_Ptr->m_ui32OpListSize + OPLISTSIZE + 1);
					
					return;
				}
				Users::m_Ptr->m_ui32OpListSize += OPLISTSIZE;
			}
			
			memcpy(Users::m_Ptr->m_pOpList + Users::m_Ptr->m_ui32OpListLen - 1, ServerManager::m_pGlobalBuffer, iLen);
			Users::m_Ptr->m_pOpList[Users::m_Ptr->m_ui32OpListLen + (iLen - 1)] = '\0';
			SendCharDelayed(Users::m_Ptr->m_pOpList, Users::m_Ptr->m_ui32OpListLen + (iLen - 1));
			Users::m_Ptr->m_pOpList[Users::m_Ptr->m_ui32OpListLen - 1] = '|';
			Users::m_Ptr->m_pOpList[Users::m_Ptr->m_ui32OpListLen] = '\0';
		}
	}
	
	if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::SENDALLUSERIP) == true && ((m_ui32SupportBits & SUPPORTBIT_USERIP2) == SUPPORTBIT_USERIP2) == true)
	{
		if (Users::m_Ptr->m_ui32UserIPListLen > 9)
		{
			if (isSupportZpipe() == false)
			{
				SendCharDelayed(Users::m_Ptr->m_pUserIPList, Users::m_Ptr->m_ui32UserIPListLen);
			}
			else
			{
				if (Users::m_Ptr->m_ui32ZUserIPListLen == 0)
				{
					Users::m_Ptr->m_pZUserIPList = ZlibUtility::m_Ptr->CreateZPipeAlign(Users::m_Ptr->m_pUserIPList, Users::m_Ptr->m_ui32UserIPListLen, Users::m_Ptr->m_pZUserIPList,
					                                                                      Users::m_Ptr->m_ui32ZUserIPListLen, Users::m_Ptr->m_ui32ZUserIPListSize);
					if (Users::m_Ptr->m_ui32ZUserIPListLen == 0)
					{
						SendCharDelayed(Users::m_Ptr->m_pUserIPList, Users::m_Ptr->m_ui32UserIPListLen);
					}
					else
					{
						PutInSendBuf(Users::m_Ptr->m_pZUserIPList, Users::m_Ptr->m_ui32ZUserIPListLen);
						ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32UserIPListLen - Users::m_Ptr->m_ui32ZUserIPListLen;
					}
				}
				else
				{
					PutInSendBuf(Users::m_Ptr->m_pZUserIPList, Users::m_Ptr->m_ui32ZUserIPListLen);
					ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32UserIPListLen - Users::m_Ptr->m_ui32ZUserIPListLen;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------

bool User::GenerateMyInfoLong()   // true == changed
{
	// Prepare myinfo with nick
	int iLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$MyINFO $ALL %s ", m_sNick);
	if (iLen <= 0)
	{
		return false;
	}
	
	// Add description
	if (m_ui8ChangedDescriptionLongLen != 0)
	{
		if (m_sChangedDescriptionLong != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedDescriptionLong, m_ui8ChangedDescriptionLongLen);
			iLen += m_ui8ChangedDescriptionLongLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_DESCRIPTION_LONG_PERM) == INFOBIT_DESCRIPTION_LONG_PERM) == false)
		{
			User::FreeInfo(m_sChangedDescriptionLong, m_ui8ChangedDescriptionLongLen);
            m_ui8ChangedDescriptionLongLen = 0;
		}
	}
	else if (m_sDescription != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sDescription, (size_t)m_ui8DescriptionLen);
		iLen += m_ui8DescriptionLen;
	}
	
	// Add tag
	if (m_ui8ChangedTagLongLen != 0)
	{
		if (m_sChangedTagLong != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedTagLong, m_ui8ChangedTagLongLen);
			iLen += m_ui8ChangedTagLongLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_TAG_LONG_PERM) == INFOBIT_TAG_LONG_PERM) == false)
		{
			User::FreeInfo(m_sChangedTagLong, m_ui8ChangedTagLongLen);
		}
	}
	else if (m_sTag != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sTag, (size_t)m_ui8TagLen);
		iLen += (int)m_ui8TagLen;
	}
	
	memcpy(ServerManager::m_pGlobalBuffer + iLen, "$ $", 3);
	iLen += 3;
	
	// Add connection
	if (m_ui8ChangedConnectionLongLen != 0)
	{
		if (m_sChangedConnectionLong != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedConnectionLong, m_ui8ChangedConnectionLongLen);
			iLen += m_ui8ChangedConnectionLongLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_CONNECTION_LONG_PERM) == INFOBIT_CONNECTION_LONG_PERM) == false)
		{
			User::FreeInfo(m_sChangedConnectionLong, m_ui8ChangedConnectionLongLen);
		}
	}
	else if (m_sConnection != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sConnection, m_ui8ConnectionLen);
		iLen += m_ui8ConnectionLen;
	}
	
	// add magicbyte
	uint8_t ui8Magic = m_ui8MagicByte;
	
	if (((m_ui32BoolBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) == false)
	{
		// should not be set if user not have TLS2 support
		ui8Magic &= ~0x10;
		ui8Magic &= ~0x20;
	}
	
	if ((m_ui32BoolBits & BIT_IPV4) == BIT_IPV4)
	{
		ui8Magic |= 0x40; // IPv4 support
	}
	else
	{
		ui8Magic &= ~0x40; // IPv4 support
	}
	
	if ((m_ui32BoolBits & BIT_IPV6) == BIT_IPV6)
	{
		ui8Magic |= 0x80; // IPv6 support
	}
	else
	{
		ui8Magic &= ~0x80; // IPv6 support
	}
	
	ServerManager::m_pGlobalBuffer[iLen] = ui8Magic;
	ServerManager::m_pGlobalBuffer[iLen + 1] = '$';
	iLen += 2;
	
	// Add email
	if (m_ui8ChangedEmailLongLen != 0)
	{
		if (m_sChangedEmailLong != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedEmailLong, m_ui8ChangedEmailLongLen);
			iLen += m_ui8ChangedEmailLongLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_EMAIL_LONG_PERM) == INFOBIT_EMAIL_LONG_PERM) == false)
		{
			User::FreeInfo(m_sChangedEmailLong, m_ui8ChangedEmailLongLen);
		}
	}
	else if (m_sEmail != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sEmail, (size_t)m_ui8EmailLen);
		iLen += (int)m_ui8EmailLen;
	}
	
	// Add share and end of myinfo
	int iRet = snprintf(ServerManager::m_pGlobalBuffer + iLen, ServerManager::m_szGlobalBufferSize - iLen, "$%" PRIu64 "$|", m_ui64ChangedSharedSizeLong);
	
	if (((m_ui32InfoBits & INFOBIT_SHARE_LONG_PERM) == INFOBIT_SHARE_LONG_PERM) == false)
	{
		m_ui64ChangedSharedSizeLong = m_ui64SharedSize;
	}
	
	if (iRet <= 0)
	{
		return false;
	}
	iLen += iRet;
	
	if (m_sMyInfoLong != NULL)
	{
		if (m_ui16MyInfoLongLen == (uint16_t)iLen && memcmp(m_sMyInfoLong + 14 + m_ui8NickLen, ServerManager::m_pGlobalBuffer + 14 + m_ui8NickLen, m_ui16MyInfoLongLen - 14 - m_ui8NickLen) == 0)
		{
			return false;
		}
	}
	
	UserSetMyInfoLong(this, ServerManager::m_pGlobalBuffer, (uint16_t)iLen);
	
	return true;
}
//---------------------------------------------------------------------------

bool User::GenerateMyInfoShort()   // true == changed
{
	// Prepare myinfo with nick
	int iLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$MyINFO $ALL %s ", m_sNick);
	if (iLen <= 0)
	{
		return false;
	}
	
	// Add mode to start of description if is enabled
	if (SettingManager::m_Ptr->m_bBools[SETBOOL_MODE_TO_DESCRIPTION] == true && m_sModes[0] != 0)
	{
		char * sActualDescription = NULL;
		
		if (m_ui8ChangedDescriptionShortLen != 0)
		{
			sActualDescription = m_sChangedDescriptionShort;
		}
		else if (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_DESCRIPTION] == true)
		{
			sActualDescription = m_sDescription;
		}
		
		if (sActualDescription == NULL)
		{
			ServerManager::m_pGlobalBuffer[iLen] = m_sModes[0];
			iLen++;
		}
		else if (sActualDescription[0] != m_sModes[0] && sActualDescription[1] != ' ')
		{
			ServerManager::m_pGlobalBuffer[iLen] = m_sModes[0];
			ServerManager::m_pGlobalBuffer[iLen + 1] = ' ';
			iLen += 2;
		}
	}
	
	// Add description
	if (m_ui8ChangedDescriptionShortLen != 0)
	{
		if (m_sChangedDescriptionShort != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedDescriptionShort, m_ui8ChangedDescriptionShortLen);
			iLen += m_ui8ChangedDescriptionShortLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_DESCRIPTION_SHORT_PERM) == INFOBIT_DESCRIPTION_SHORT_PERM) == false)
		{
			User::FreeInfo(m_sChangedDescriptionShort, m_ui8ChangedDescriptionShortLen);
		}
	}
	else if (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_DESCRIPTION] == false && m_sDescription != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sDescription, m_ui8DescriptionLen);
		iLen += m_ui8DescriptionLen;
	}
	
	// Add tag
	if (m_ui8ChangedTagShortLen != 0)
	{
		if (m_sChangedTagShort != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedTagShort, m_ui8ChangedTagShortLen);
			iLen += m_ui8ChangedTagShortLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_TAG_SHORT_PERM) == INFOBIT_TAG_SHORT_PERM) == false)
		{
			User::FreeInfo(m_sChangedTagShort, m_ui8ChangedTagShortLen);
		}
	}
	else if (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_TAG] == false && m_sTag != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sTag, (size_t)m_ui8TagLen);
		iLen += (int)m_ui8TagLen;
	}
	
	// Add mode to myinfo if is enabled
	if (SettingManager::m_Ptr->m_bBools[SETBOOL_MODE_TO_MYINFO] == true && m_sModes[0] != 0)
	{
		int iRet = snprintf(ServerManager::m_pGlobalBuffer + iLen, ServerManager::m_szGlobalBufferSize - iLen, "$%c$", m_sModes[0]);
		if (iRet <= 0)
		{
			return false;
		}
		iLen += iRet;
	}
	else
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, "$ $", 3);
		iLen += 3;
	}
	
	// Add connection
	if (m_ui8ChangedConnectionShortLen != 0)
	{
		if (m_sChangedConnectionShort != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedConnectionShort, m_ui8ChangedConnectionShortLen);
			iLen += m_ui8ChangedConnectionShortLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_CONNECTION_SHORT_PERM) == INFOBIT_CONNECTION_SHORT_PERM) == false)
		{
			User::FreeInfo(m_sChangedConnectionShort, m_ui8ChangedConnectionShortLen);
		}
	}
	else if (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_CONNECTION] == false && m_sConnection != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sConnection, m_ui8ConnectionLen);
		iLen += m_ui8ConnectionLen;
	}
	
	// add magicbyte
	uint8_t ui8Magic = m_ui8MagicByte;
	
	if (((m_ui32BoolBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) == false)
	{
		// should not be set if user not have TLS2 support
		ui8Magic &= ~0x10;
		ui8Magic &= ~0x20;
	}
	
	if ((m_ui32BoolBits & BIT_IPV4) == BIT_IPV4)
	{
		ui8Magic |= 0x40; // IPv4 support
	}
	else
	{
		ui8Magic &= ~0x40; // IPv4 support
	}
	
	if ((m_ui32BoolBits & BIT_IPV6) == BIT_IPV6)
	{
		ui8Magic |= 0x80; // IPv6 support
	}
	else
	{
		ui8Magic &= ~0x80; // IPv6 support
	}
	
	ServerManager::m_pGlobalBuffer[iLen] = ui8Magic;
	ServerManager::m_pGlobalBuffer[iLen + 1] = '$';
	iLen += 2;
	
	// Add email
	if (m_ui8ChangedEmailShortLen != 0)
	{
		if (m_sChangedEmailShort != NULL)
		{
			memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedEmailShort, m_ui8ChangedEmailShortLen);
			iLen += m_ui8ChangedEmailShortLen;
		}
		
		if (((m_ui32InfoBits & INFOBIT_EMAIL_SHORT_PERM) == INFOBIT_EMAIL_SHORT_PERM) == false)
		{
			User::FreeInfo(m_sChangedEmailShort, m_ui8ChangedEmailShortLen);
		}
	}
	else if (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_EMAIL] == false && m_sEmail != NULL)
	{
		memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sEmail, (size_t)m_ui8EmailLen);
		iLen += (int)m_ui8EmailLen;
	}
	
	// Add share and end of myinfo
	int iRet = snprintf(ServerManager::m_pGlobalBuffer + iLen, ServerManager::m_szGlobalBufferSize - iLen, "$%" PRIu64 "$|", m_ui64ChangedSharedSizeShort);
	
	if (((m_ui32InfoBits & INFOBIT_SHARE_SHORT_PERM) == INFOBIT_SHARE_SHORT_PERM) == false)
	{
		m_ui64ChangedSharedSizeShort = m_ui64SharedSize;
	}
	
	if (iRet <= 0)
	{
		return false;
	}
	iLen += iRet;
	
	if (m_sMyInfoShort != NULL)
	{
		if (m_ui16MyInfoShortLen == (uint16_t)iLen && memcmp(m_sMyInfoShort + 14 + m_ui8NickLen, ServerManager::m_pGlobalBuffer + 14 + m_ui8NickLen, m_ui16MyInfoShortLen - 14 - m_ui8NickLen) == 0)
		{
			return false;
		}
	}
	
	UserSetMyInfoShort(this, ServerManager::m_pGlobalBuffer, (uint16_t)iLen);
	
	return true;
}
//---------------------------------------------------------------------------

void User::HasSuspiciousTag()
{
	if (SettingManager::m_Ptr->m_bBools[SETBOOL_REPORT_SUSPICIOUS_TAG] == true && SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true)
	{
		m_sDescription[m_ui8DescriptionLen] = '\0';
		GlobalDataQueue::m_Ptr->StatusMessageFormat("User::HasSuspiciousTag", "<%s> *** %s (%s) %s. %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], m_sNick, m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM],
		                                            LanguageManager::m_Ptr->m_sTexts[LAN_FULL_DESCRIPTION], m_sDescription);
		m_sDescription[m_ui8DescriptionLen] = '$';
	}
	m_ui32BoolBits &= ~BIT_HAVE_BADTAG;
}
//---------------------------------------------------------------------------

bool User::ProcessRules()
{
	// if share limit enabled, check it
	if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOSHARELIMIT) == false)
	{
		if ((SettingManager::m_Ptr->m_ui64MinShare != 0 && m_ui64SharedSize < SettingManager::m_Ptr->m_ui64MinShare) ||
		        (SettingManager::m_Ptr->m_ui64MaxShare != 0 && m_ui64SharedSize > SettingManager::m_Ptr->m_ui64MaxShare))
		{
			SendChar(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_SHARE_LIMIT_MSG], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_SHARE_LIMIT_MSG]);
			//UdpDebug::m_Ptr->BroadcastFormat("[SYS] User with low or high share %s (%s) disconnected.", sNick, sIP);
			return false;
		}
	}
	
	// no Tag? Apply rule
	if (m_sTag == NULL)
	{
		if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOTAGCHECK) == false)
		{
			if (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_NO_TAG_OPTION] != 0)
			{
				SendChar(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_NO_TAG_MSG], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_NO_TAG_MSG]);
				//UdpDebug::m_Ptr->BroadcastFormat("[SYS] User without Tag %s (%s) redirected.", sNick, sIP);
				return false;
			}
		}
	}
	else
	{
		// min and max slot check
		if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOSLOTCHECK) == false)
		{
			// TODO 2 -oPTA -ccheckers: $SR based slots fetching for no_tag users
			
			if ((SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SLOTS_LIMIT] != 0 && m_ui32Slots < (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SLOTS_LIMIT]) ||
			        (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SLOTS_LIMIT] != 0 && m_ui32Slots > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SLOTS_LIMIT]))
			{
				SendChar(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_SLOTS_LIMIT_MSG], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_SLOTS_LIMIT_MSG]);
				//UdpDebug::m_Ptr->BroadcastFormat("[SYS] User with bad slots %s (%s) disconnected.", sNick, sIP);
				return false;
			}
		}
		
		// slots/hub ration check
		if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOSLOTHUBRATIO) == false &&
		        SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS] != 0 && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] != 0)
		{
			const uint32_t slots = m_ui32Slots;
			const uint32_t hubs = m_ui32Hubs > 0 ? m_ui32Hubs : 1;
			if (((double)slots / hubs) < ((double)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] / SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS]))
			{
				SendChar(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SLOT_RATIO_MSG], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_HUB_SLOT_RATIO_MSG]);
				//UdpDebug::m_Ptr->BroadcastFormat("[SYS] User with bad hub/slot ratio %s (%s) disconnected.", sNick, sIP);
				return false;
			}
		}
		
		// hub checker
		if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOMAXHUBCHECK) == false && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_HUBS_LIMIT] != 0)
		{
			if (m_ui32Hubs > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_HUBS_LIMIT])
			{
				SendChar(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_MAX_HUBS_LIMIT_MSG], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_MAX_HUBS_LIMIT_MSG]);
				//UdpDebug::m_Ptr->BroadcastFormat("[SYS] User with bad hubs count %s (%s) disconnected.", sNick, sIP);
				return false;
			}
		}
	}
	
	return true;
}

//------------------------------------------------------------------------------

void User::AddPrcsdCmd(const uint8_t ui8Type, const char * sCommand, const size_t szCommandLen, User * pToUser, const bool bIsPm/* = false*/)
{
	if (ui8Type == PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO)
	{
		PrcsdToUsrCmd * cur = NULL,
		                * next = m_pCmdToUserStrt;
		                
		while (next != NULL)
		{
			cur = next;
			next = cur->m_pNext;
			
			if (cur->m_pToUser == pToUser)
			{
				char * pOldBuf = cur->m_sCommand;
				cur->m_sCommand = (char *)realloc(pOldBuf, cur->m_ui32Len + szCommandLen + 1);
				if (cur->m_sCommand == NULL)
				{
					cur->m_sCommand = pOldBuf;
					m_ui32BoolBits |= BIT_ERROR;
					Close();
					
					AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in User::AddPrcsdCmd\n", cur->m_ui32Len+szCommandLen+1);
					
					return;
				}
				memcpy(cur->m_sCommand + cur->m_ui32Len, sCommand, szCommandLen);
				cur->m_sCommand[cur->m_ui32Len + szCommandLen] = '\0';
				cur->m_ui32Len += (uint32_t)szCommandLen;
				cur->m_ui32PmCount += bIsPm == true ? 1 : 0;
				return;
			}
		}
		
		PrcsdToUsrCmd * pNewToCmd = new (std::nothrow) PrcsdToUsrCmd;
		if (pNewToCmd == NULL)
		{
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			
			AppendDebugLog("%s - [MEM] User::AddPrcsdCmd cannot allocate new pNewToCmd\n");
			
			return;
		}
		
		pNewToCmd->m_sCommand = (char *)malloc(szCommandLen + 1);
		if (pNewToCmd->m_sCommand == NULL)
		{
			m_ui32BoolBits |= BIT_ERROR;
			Close();
			
			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sCommand in User::AddPrcsdCmd\n", szCommandLen+1);
			
			delete pNewToCmd;
			
			return;
		}
		
		memcpy(pNewToCmd->m_sCommand, sCommand, szCommandLen);
		pNewToCmd->m_sCommand[szCommandLen] = '\0';
		
		pNewToCmd->m_ui32Len = (uint32_t)szCommandLen;
		pNewToCmd->m_ui32PmCount = bIsPm == true ? 1 : 0;
		pNewToCmd->m_ui32Loops = 0;
		pNewToCmd->m_pToUser = pToUser;
		
		pNewToCmd->m_nick = std::string(pToUser->m_sNick, pToUser->m_ui8NickLen);
		pNewToCmd->m_pNext = nullptr;
		
		if (m_pCmdToUserStrt == NULL)
		{
			m_pCmdToUserStrt = pNewToCmd;
			m_pCmdToUserEnd = pNewToCmd;
		}
		else
		{
			m_pCmdToUserEnd->m_pNext = pNewToCmd;
			m_pCmdToUserEnd = pNewToCmd;
		}
		
		return;
	}
	
	PrcsdUsrCmd * pNewcmd = new (std::nothrow) PrcsdUsrCmd;
	if (pNewcmd == NULL)
	{
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		AppendDebugLog("%s - [MEM] User::AddPrcsdCmd cannot allocate new pNewcmd1\n");
		
		return;
	}
	
	pNewcmd->m_sCommand = (char *)malloc(szCommandLen + 1);
	if (pNewcmd->m_sCommand == NULL)
	{
		m_ui32BoolBits |= BIT_ERROR;
		Close();
		
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sCommand in User::AddPrcsdCmd1\n", szCommandLen+1);
		
		delete pNewcmd;
		
		return;
	}
	
	memcpy(pNewcmd->m_sCommand, sCommand, szCommandLen);
	pNewcmd->m_sCommand[szCommandLen] = '\0';
	
	pNewcmd->m_ui32Len = (uint32_t)szCommandLen;
	pNewcmd->m_ui8Type = ui8Type;
	pNewcmd->m_pNext = nullptr;
	pNewcmd->m_pPtr = (void *)pToUser; // TODO - ������
	
	if (m_pCmdStrt == NULL)
	{
		m_pCmdStrt = pNewcmd;
		m_pCmdEnd = pNewcmd;
	}
	else
	{
		m_pCmdEnd->m_pNext = pNewcmd;
		m_pCmdEnd = pNewcmd;
	}
}
//---------------------------------------------------------------------------

void User::AddMeOrIPv4Check()
{
	if (((m_ui32BoolBits & BIT_IPV6) == BIT_IPV6) && ((m_ui32SupportBits & SUPPORTBIT_IPV4) == SUPPORTBIT_IPV4) && ServerManager::m_sHubIP[0] != '\0' && ServerManager::m_bUseIPv4 == true)
	{
		m_ui8State = STATE_IPV4_CHECK;
		m_LogInOut.m_ui64IPv4CheckTick = ServerManager::m_ui64ActualTick;
		
		SendFormat("AddMeOrIPv4Check", true, "$ConnectToMe %s %s:%hu|", m_sNick, ServerManager::m_sHubIP, SettingManager::m_Ptr->m_ui16PortNumbers[0]);
	}
	else
	{
		m_ui8State = STATE_ADDME;
	}
}
//---------------------------------------------------------------------------

char * User::SetUserInfo(char* sOldData, uint8_t &ui8OldDataLen, const char * sNewData, const size_t szNewDataLen, const char * /* sDataName */)
{
	User::FreeInfo(sOldData, ui8OldDataLen);
	
	if (szNewDataLen > 0)
	{
		sOldData = (char *)malloc(szNewDataLen + 1);
		if (sOldData == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes in User::SetUserInfo\n", szNewDataLen+1);
			return sOldData;
		}
		
		memcpy(sOldData, sNewData, szNewDataLen);
		sOldData[szNewDataLen] = '\0';
		ui8OldDataLen = (uint8_t)szNewDataLen;
	}
	else
	{
		ui8OldDataLen = 1;
	}
	
	return sOldData;
}
//---------------------------------------------------------------------------

void User::RemFromSendBuf(const char * sData, const uint32_t ui32Len, const uint32_t ui32SendBufLen)
{
	char *match = strstr(m_pSendBuf + ui32SendBufLen, sData);
	if (match != NULL)
	{
		memmove(match, match + ui32Len, m_ui32SendBufDataLen - ((match + (ui32Len)) - m_pSendBuf));
		m_ui32SendBufDataLen -= ui32Len;
		m_pSendBuf[m_ui32SendBufDataLen] = '\0';
	}
}
//------------------------------------------------------------------------------

void User::DeletePrcsdUsrCmd(PrcsdUsrCmd *& pCommand)
{
	if (pCommand != NULL)
	{
		free(pCommand->m_sCommand);
		delete pCommand;
		pCommand = nullptr; // [+] FlylinkDC++
	}
}
//------------------------------------------------------------------------------

