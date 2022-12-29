/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "UdpDebug.h"
//---------------------------------------------------------------------------
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
#pragma hdrstop
#else
#include <string>
#include <syslog.h>
#endif
//---------------------------------------------------------------------------
UdpDebug * UdpDebug::m_Ptr = nullptr;
bool g_isUseSyslog = false;
bool g_isUseLog = false;
//---------------------------------------------------------------------------

UdpDebug::UdpDbgItem::UdpDbgItem() : m_pPrev(NULL), m_pNext(NULL),
#ifdef _WIN32
	s(INVALID_SOCKET),
#else
	s(-1),
#endif
	sas_len(0), ui32Hash(0), bIsScript(false), bAllData(true)
{
	memset(&sas_to, 0, sizeof(sockaddr_storage));
}
//---------------------------------------------------------------------------

UdpDebug::UdpDbgItem::~UdpDbgItem()
{
	safe_closesocket(s);
}
//---------------------------------------------------------------------------

UdpDebug::UdpDebug() : sDebugBuffer(NULL), sDebugHead(NULL), pDbgItemList(NULL)
{
	// ...
}
//---------------------------------------------------------------------------

UdpDebug::~UdpDebug()
{
	free(sDebugBuffer);

	UdpDbgItem * cur = NULL,
	             * next = pDbgItemList;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		delete cur;
	}
}
//---------------------------------------------------------------------------

void UdpDebug::Broadcast(const char * msg, const size_t szMsgLen) const
{
#ifndef _WIN32
	if (g_isUseSyslog)
	{
		std::string l_str(msg, szMsgLen);
		//printf("%s\r", l_str.c_str());
		syslog(LOG_NOTICE, "%s", l_str.c_str());
	}
#endif

	if (pDbgItemList == NULL)
	{
		return;
	}

	((uint16_t *)sDebugBuffer)[1] = (uint16_t)szMsgLen;
	memcpy(sDebugHead, msg, szMsgLen);
	size_t szLen = (sDebugHead - sDebugBuffer) + szMsgLen;

	UdpDbgItem * pCur = NULL,
	             * m_pNext = pDbgItemList;

	while (m_pNext != NULL && m_pNext->bAllData == true)
	{
		pCur = m_pNext;
		m_pNext = pCur->m_pNext;
#ifdef _WIN32
		sendto(pCur->s, sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pCur->sas_to, pCur->sas_len);
#else
		sendto(pCur->s, sDebugBuffer, szLen, 0, (struct sockaddr *)&pCur->sas_to, pCur->sas_len);
#endif
		ServerManager::m_ui64BytesSent += szLen;
	}
}
//---------------------------------------------------------------------------

void UdpDebug::BroadcastFormat(const char * sFormatMsg, ...) const
{

#ifndef _WIN32
	if (g_isUseSyslog)
	{
		std::string l_str;
		l_str.resize(65535);
		va_list vlArgs;
		va_start(vlArgs, sFormatMsg);
		vsprintf(&l_str[0], sFormatMsg, vlArgs);
		va_end(vlArgs);
		syslog(LOG_NOTICE, "%s", l_str.c_str());
	}
#endif

	if (pDbgItemList == NULL)
	{
		return;
	}
	va_list vlArgs;
	va_start(vlArgs, sFormatMsg);

	const int iRet = vsprintf(sDebugHead, sFormatMsg, vlArgs);

	va_end(vlArgs);

	if (iRet < 0 || iRet >= 65535)
	{
		AppendDebugLogFormat("[ERR] vsprintf wrong value %d in UdpDebug::Broadcast\n", iRet);

		return;
	}

	((uint16_t *)sDebugBuffer)[1] = (uint16_t)iRet;
	size_t szLen = (sDebugHead - sDebugBuffer) + iRet;

	UdpDbgItem * pCur = NULL,
	             * m_pNext = pDbgItemList;

	while (m_pNext != NULL && m_pNext->bAllData == true)
	{
		pCur = m_pNext;
		m_pNext = pCur->m_pNext;
#ifdef _WIN32
		sendto(pCur->s, sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pCur->sas_to, pCur->sas_len);
#else
		sendto(pCur->s, sDebugBuffer, szLen, 0, (struct sockaddr *)&pCur->sas_to, pCur->sas_len);
#endif
		ServerManager::m_ui64BytesSent += szLen;
	}
}
//---------------------------------------------------------------------------

void UdpDebug::CreateBuffer()
{
	if (sDebugBuffer != NULL)
	{
		return;
	}

	sDebugBuffer = (char *)malloc(4 + 256 + 65535);
	if (sDebugBuffer == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate 4+256+65535 bytes for sDebugBuffer in UdpDebug::CreateBuffer\n");

		exit(EXIT_FAILURE);
	}

	UpdateHubName();
}
//---------------------------------------------------------------------------

bool UdpDebug::New(User * pUser, const uint16_t ui16Port)
{
	UdpDbgItem * pNewDbg = new (std::nothrow) UdpDbgItem();
	if (pNewDbg == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pNewDbg in UdpDebug::New\n");
		return false;
	}
	if (pUser->m_sNick)
	{
		pNewDbg->m_sNick = pUser->m_sNick;
	}

	pNewDbg->ui32Hash = pUser->m_ui32NickHash;

	struct in6_addr i6addr;
	memcpy(&i6addr, &pUser->m_ui128IpHash, 16);

	bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

	if (bIPv6 == true)
	{
		((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_port = htons(ui16Port);
		memcpy(((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_addr.s6_addr, pUser->m_ui128IpHash, 16);
		pNewDbg->sas_len = sizeof(struct sockaddr_in6);
	}
	else
	{
		((struct sockaddr_in *)&pNewDbg->sas_to)->sin_family = AF_INET;
		((struct sockaddr_in *)&pNewDbg->sas_to)->sin_port = htons(ui16Port);
		((struct sockaddr_in *)&pNewDbg->sas_to)->sin_addr.s_addr = inet_addr(pUser->m_sIP);
		pNewDbg->sas_len = sizeof(struct sockaddr_in);
	}

	pNewDbg->s = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
	if (pNewDbg->s == INVALID_SOCKET)
	{
		int iErr = WSAGetLastError();
#else
	if (pNewDbg->s == -1)
	{
#endif
		pUser->SendFormat("UdpDebug::New1", true, "*** [ERR] %s: %s (%d).|", LanguageManager::m_Ptr->m_sTexts[LAN_UDP_SCK_CREATE_ERR],
#ifdef _WIN32
		                  WSErrorStr(iErr), iErr);
#else
		                  ErrnoStr(errno), errno);
#endif
		delete pNewDbg;
		return false;
	}

	// set non-blocking
#ifdef _WIN32
	uint32_t block = 1;
	if (SOCKET_ERROR == ioctlsocket(pNewDbg->s, FIONBIO, (unsigned long *)&block))
	{
		int iErr = WSAGetLastError();
#else
	int oldFlag = fcntl(pNewDbg->s, F_GETFL, 0);
	if (fcntl(pNewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1)
	{
#endif
		pUser->SendFormat("UdpDebug::New2", true, "*** [ERR] %s: %s (%d).|", LanguageManager::m_Ptr->m_sTexts[LAN_UDP_NON_BLOCK_FAIL],
#ifdef _WIN32
		                  WSErrorStr(iErr), iErr);
#else
		                  ErrnoStr(errno), errno);
#endif
		delete pNewDbg;
		return false;
	}

	pNewDbg->m_pPrev = nullptr;
	pNewDbg->m_pNext = nullptr;

	if (pDbgItemList == NULL)
	{
		CreateBuffer();
		pDbgItemList = pNewDbg;
	}
	else
	{
		pDbgItemList->m_pPrev = pNewDbg;
		pNewDbg->m_pNext = pDbgItemList;
		pDbgItemList = pNewDbg;
	}

	pNewDbg->bIsScript = false;

	int iLen = sprintf(sDebugHead, "[HUB] Subscribed, users online: %u", ServerManager::m_ui32Logged);
	if (iLen < 0 || iLen > 65535)
	{
		AppendDebugLogFormat("[ERR] sprintf wrong value %d in UdpDebug::New\n", iLen);

		return true;
	}

	// create packet
	((uint16_t *)sDebugBuffer)[1] = (uint16_t)iLen;
	size_t szLen = (sDebugHead - sDebugBuffer) + iLen;
#ifdef _WIN32
	sendto(pNewDbg->s, sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#else
	sendto(pNewDbg->s, sDebugBuffer, szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#endif
	ServerManager::m_ui64BytesSent += szLen;

	return true;
}
//---------------------------------------------------------------------------

bool UdpDebug::New(const char * sIP, const uint16_t ui16Port, const bool bAllData, const char * sScriptName)
{
	UdpDbgItem * pNewDbg = new (std::nothrow) UdpDbgItem();
	if (pNewDbg == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pNewDbg in UdpDebug::New\n");
		return false;
	}

	// initialize dbg item
	pNewDbg->m_sNick = sScriptName;

	pNewDbg->ui32Hash = 0;

	uint8_t ui128IP[16];
	HashIP(sIP, ui128IP);

	struct in6_addr i6addr;
	memcpy(&i6addr, &ui128IP, 16);

	bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

	if (bIPv6 == true)
	{
		((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_port = htons(ui16Port);
		memcpy(((struct sockaddr_in6 *)&pNewDbg->sas_to)->sin6_addr.s6_addr, ui128IP, 16);
		pNewDbg->sas_len = sizeof(struct sockaddr_in6);
	}
	else
	{
		((struct sockaddr_in *)&pNewDbg->sas_to)->sin_family = AF_INET;
		((struct sockaddr_in *)&pNewDbg->sas_to)->sin_port = htons(ui16Port);
		memcpy(&((struct sockaddr_in *)&pNewDbg->sas_to)->sin_addr.s_addr, ui128IP + 12, 4);
		pNewDbg->sas_len = sizeof(struct sockaddr_in);
	}

	pNewDbg->s = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);

#ifdef _WIN32
	if (pNewDbg->s == INVALID_SOCKET)
	{
#else
	if (pNewDbg->s == -1)
	{
#endif
		delete pNewDbg;
		return false;
	}

	// set non-blocking
#ifdef _WIN32
	uint32_t block = 1;
	if (SOCKET_ERROR == ioctlsocket(pNewDbg->s, FIONBIO, (unsigned long *)&block))
	{
#else
	int oldFlag = fcntl(pNewDbg->s, F_GETFL, 0);
	if (fcntl(pNewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1)
	{
#endif
		delete pNewDbg;
		return false;
	}

	pNewDbg->m_pPrev = nullptr;
	pNewDbg->m_pNext = nullptr;

	if (pDbgItemList == NULL)
	{
		CreateBuffer();
		pDbgItemList = pNewDbg;
	}
	else
	{
		pDbgItemList->m_pPrev = pNewDbg;
		pNewDbg->m_pNext = pDbgItemList;
		pDbgItemList = pNewDbg;
	}

	pNewDbg->bIsScript = true;
	pNewDbg->bAllData = bAllData;

	int iLen = sprintf(sDebugHead, "[HUB] Subscribed, users online: %u", ServerManager::m_ui32Logged);
	if (iLen < 0 || iLen > 65535)
	{
		AppendDebugLogFormat("[ERR] sprintf wrong value %d in UdpDebug::New2\n", iLen);

		return true;
	}

	// create packet
	((uint16_t *)sDebugBuffer)[1] = (uint16_t)iLen;
	size_t szLen = (sDebugHead - sDebugBuffer) + iLen;
#ifdef _WIN32
	sendto(pNewDbg->s, sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#else
	sendto(pNewDbg->s, sDebugBuffer, szLen, 0, (struct sockaddr *)&pNewDbg->sas_to, pNewDbg->sas_len);
#endif
	ServerManager::m_ui64BytesSent += szLen;

	return true;
}
//---------------------------------------------------------------------------

void UdpDebug::DeleteBuffer()
{
	safe_free(sDebugBuffer);
	sDebugHead = nullptr;
}
//---------------------------------------------------------------------------

bool UdpDebug::Remove(User * pUser)
{
	UdpDbgItem * pCur = NULL,
	             * m_pNext = pDbgItemList;

	while (m_pNext != NULL)
	{
		pCur = m_pNext;
		m_pNext = pCur->m_pNext;

		if (pCur->bIsScript == false && pCur->ui32Hash == pUser->m_ui32NickHash && strcasecmp(pCur->m_sNick.c_str(), pUser->m_sNick) == 0)
		{
			if (pCur->m_pPrev == NULL)
			{
				if (pCur->m_pNext == NULL)
				{
					pDbgItemList = nullptr;
					DeleteBuffer();
				}
				else
				{
					pCur->m_pNext->m_pPrev = nullptr;
					pDbgItemList = pCur->m_pNext;
				}
			}
			else if (pCur->m_pNext == NULL)
			{
				pCur->m_pPrev->m_pNext = nullptr;
			}
			else
			{
				pCur->m_pPrev->m_pNext = pCur->m_pNext;
				pCur->m_pNext->m_pPrev = pCur->m_pPrev;
			}

			delete pCur;
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------

void UdpDebug::Remove(const char * sScriptName)
{
	UdpDbgItem * pCur = NULL,
	             * m_pNext = pDbgItemList;

	while (m_pNext != NULL)
	{
		pCur = m_pNext;
		m_pNext = pCur->m_pNext;

		if (pCur->bIsScript == true && strcasecmp(pCur->m_sNick.c_str(), sScriptName) == 0)
		{
			if (pCur->m_pPrev == NULL)
			{
				if (pCur->m_pNext == NULL)
				{
					pDbgItemList = nullptr;
					DeleteBuffer();
				}
				else
				{
					pCur->m_pNext->m_pPrev = nullptr;
					pDbgItemList = pCur->m_pNext;
				}
			}
			else if (pCur->m_pNext == NULL)
			{
				pCur->m_pPrev->m_pNext = nullptr;
			}
			else
			{
				pCur->m_pPrev->m_pNext = pCur->m_pNext;
				pCur->m_pNext->m_pPrev = pCur->m_pPrev;
			}

			delete pCur;
			return;
		}
	}
}
//---------------------------------------------------------------------------

bool UdpDebug::CheckUdpSub(User * pUser, bool bSndMess/* = false*/) const
{
	UdpDbgItem * pCur = NULL,
	             * m_pNext = pDbgItemList;

	while (m_pNext != NULL)
	{
		pCur = m_pNext;
		m_pNext = pCur->m_pNext;

		if (pCur->bIsScript == false && pCur->ui32Hash == pUser->m_ui32NickHash && strcasecmp(pCur->m_sNick.c_str(), pUser->m_sNick) == 0)
		{
			if (bSndMess == true)
			{
				pUser->SendFormat("UdpDebug::CheckUdpSub", true, "<%s> *** %s %hu. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_SUBSCRIBED_UDP_DBG],
				                  ntohs(pCur->sas_to.ss_family == AF_INET6 ? ((struct sockaddr_in6 *)&pCur->sas_to)->sin6_port : ((struct sockaddr_in *)&pCur->sas_to)->sin_port), LanguageManager::m_Ptr->m_sTexts[LAN_TO_UNSUB_UDP_DBG]);
			}

			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------

void UdpDebug::Send(const char * sScriptName, const char * sMessage, const size_t szMsgLen) const
{
	if (pDbgItemList == NULL)
	{
		return;
	}

	UdpDbgItem * pCur = NULL,
	             * m_pNext = pDbgItemList;

	while (m_pNext != NULL)
	{
		pCur = m_pNext;
		m_pNext = pCur->m_pNext;

		if (pCur->bIsScript == true && strcasecmp(pCur->m_sNick.c_str(), sScriptName) == 0)
		{
			// create packet
			((uint16_t *)sDebugBuffer)[1] = (uint16_t)szMsgLen;
			memcpy(sDebugHead, sMessage, szMsgLen);
			size_t szLen = (sDebugHead - sDebugBuffer) + szMsgLen;

#ifdef _WIN32
			sendto(pCur->s, sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pCur->sas_to, pCur->sas_len);
#else
			sendto(pCur->s, sDebugBuffer, szLen, 0, (struct sockaddr *)&pCur->sas_to, pCur->sas_len);
#endif
			ServerManager::m_ui64BytesSent += szLen;

			return;
		}
	}
}
//---------------------------------------------------------------------------

void UdpDebug::Cleanup()
{
	UdpDbgItem * pCur = NULL,
	             * m_pNext = pDbgItemList;

	while (m_pNext != NULL)
	{
		pCur = m_pNext;
		m_pNext = pCur->m_pNext;

		delete pCur;
	}

	pDbgItemList = nullptr;
}
//---------------------------------------------------------------------------

void UdpDebug::UpdateHubName()
{
	if (sDebugBuffer == NULL)
	{
		return;
	}

	((uint16_t *)sDebugBuffer)[0] = (uint16_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME];
	memcpy(sDebugBuffer + 4, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME]);

	sDebugHead = sDebugBuffer + 4 + SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME];
}
//---------------------------------------------------------------------------
