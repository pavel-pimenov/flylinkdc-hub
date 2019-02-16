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

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
//---------------------------------------------------------------------------
#include "DcCommands.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
#include "RegThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
#include "../gui.win/GuiUtil.h"
#include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
EventQueue * EventQueue::m_Ptr = NULL;
//---------------------------------------------------------------------------

EventQueue::Event::Event(const char* p_message) : m_pPrev(NULL), m_pNext(NULL), m_ui8Id(0)
{
    memset(&m_ui128IpHash, 0, 16);
	if (p_message)
	{
		m_sMsg = p_message;
	}
}
//---------------------------------------------------------------------------

EventQueue::EventQueue() : m_pNormalE(NULL), m_pThreadE(NULL), m_pNormalS(NULL), m_pThreadS(NULL)
{
}
//---------------------------------------------------------------------------

EventQueue::~EventQueue()
{
	Event * cur = NULL,
	        * next = m_pNormalS;
	        
	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;
		
		delete cur;
	}
	
	next = m_pThreadS;
	
	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;
		delete cur;
	}
	
}
//---------------------------------------------------------------------------

void EventQueue::AddNormal(const uint8_t ui8Id, const char * sMsg)
{
	if (ui8Id != EVENT_RSTSCRIPT && ui8Id != EVENT_STOPSCRIPT)
	{
		Event * cur = NULL,
		        * next = m_pNormalS;
		        
		while (next != NULL)
		{
			cur = next;
			next = cur->m_pNext;
			
			if (cur->m_ui8Id == ui8Id)
			{
				return;
			}
		}
	}
	
	Event * pNewEvent = new (std::nothrow) Event(sMsg);
	
	if (pNewEvent == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pNewEvent in EventQueue::AddNormal\n");
		return;
	}
	
	pNewEvent->m_ui8Id = ui8Id;
	
	if (m_pNormalS == NULL)
	{
		m_pNormalS = pNewEvent;
		pNewEvent->m_pPrev = nullptr;
	}
	else
	{
		pNewEvent->m_pPrev = m_pNormalE;
		m_pNormalE->m_pNext = pNewEvent;
	}
	
	m_pNormalE = pNewEvent;
	pNewEvent->m_pNext = nullptr;
}
//---------------------------------------------------------------------------

void EventQueue::AddThread(const uint8_t ui8Id, const char * sMsg, const sockaddr_storage * sas/* = NULL*/)
{
	Event * pNewEvent = new (std::nothrow) Event(sMsg);
	
	if (pNewEvent == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pNewEvent in EventQueue::AddThread\n");
		return;
	}
	
	pNewEvent->m_ui8Id = ui8Id;
	
	if (sas != NULL)
	{
		if (sas->ss_family == AF_INET6)
		{
			memcpy(pNewEvent->m_ui128IpHash, &((struct sockaddr_in6 *)sas)->sin6_addr.s6_addr, 16);
		}
		else
		{
			memset(pNewEvent->m_ui128IpHash, 0, 16);
			pNewEvent->m_ui128IpHash[10] = 255;
			pNewEvent->m_ui128IpHash[11] = 255;
			memcpy(pNewEvent->m_ui128IpHash + 12, &((struct sockaddr_in *)sas)->sin_addr.s_addr, 4);
		}
	}
	else
	{
		memset(pNewEvent->m_ui128IpHash, 0, 16);
	}
	
	Lock l(m_csEventQueue);
	
	if (m_pThreadS == NULL)
	{
		m_pThreadS = pNewEvent;
		pNewEvent->m_pPrev = NULL;
	}
	else
	{
		pNewEvent->m_pPrev = m_pThreadE;
		m_pThreadE->m_pNext = pNewEvent;
	}
	
	m_pThreadE = pNewEvent;
	pNewEvent->m_pNext = NULL;
	
}
//---------------------------------------------------------------------------

void EventQueue::ProcessEvents()
{
	Event * cur = NULL,
	        * next = m_pNormalS;
	        
	m_pNormalS = NULL;
	m_pNormalE = NULL;
	
	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;
		
		switch (cur->m_ui8Id)
		{
			case EVENT_RESTART:
				ServerManager::m_bIsRestart = true;
				ServerManager::Stop();
				break;
			case EVENT_RSTSCRIPTS:
				ScriptManager::m_Ptr->Restart();
				break;
			case EVENT_RSTSCRIPT:
			{
				Script * pScript = ScriptManager::m_Ptr->FindScript(cur->m_sMsg.c_str());
				if (pScript == NULL || pScript->m_bEnabled == false || pScript->m_pLua == NULL)
				{
					return;
				}
				
				ScriptManager::m_Ptr->StopScript(pScript, false);
				
				ScriptManager::m_Ptr->StartScript(pScript, false);
				
				break;
			}
			case EVENT_STOPSCRIPT:
			{
				Script * pScript = ScriptManager::m_Ptr->FindScript(cur->m_sMsg.c_str());
				if (pScript == NULL || pScript->m_bEnabled == false || pScript->m_pLua == NULL)
				{
					return;
				}
				
				ScriptManager::m_Ptr->StopScript(pScript, true);
				
				break;
			}
			case EVENT_STOP_SCRIPTING:
				if (SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == true)
				{
					SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] = false;
					ScriptManager::m_Ptr->OnExit(true);
					ScriptManager::m_Ptr->Stop();
				}
				
				break;
			case EVENT_SHUTDOWN:
				if (ServerManager::m_bIsClose == true)
				{
					break;
				}
				
				ServerManager::m_bIsClose = true;
				ServerManager::Stop();
				
				break;
			default:
				break;
		}
		delete cur;
	}
	
	{
		Lock l(m_csEventQueue);
		
		next = m_pThreadS;
		
		m_pThreadS = nullptr;
		m_pThreadE = nullptr;
	}
	
	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;
		
		switch (cur->m_ui8Id)
		{
			case EVENT_REGSOCK_MSG:
			{
				UdpDebug::m_Ptr->Broadcast(cur->m_sMsg);
				break;
			}
			case EVENT_SRVTHREAD_MSG:
			{
				UdpDebug::m_Ptr->Broadcast(cur->m_sMsg);
				break;
			}
#ifdef FLYLINKDC_USE_UDP_THREAD
			case EVENT_UDP_SR:
			{
				const size_t szMsgLen = cur->m_sMsg.length();
				ServerManager::m_ui64BytesRead += (uint64_t)szMsgLen;
				if (szMsgLen > 4)
				{
					char *temp = (char *)strchr(cur->m_sMsg.c_str() + 4, ' ');
					if (temp == NULL)
					{
						break;
					}
					
					size_t szLen = (temp - cur->m_sMsg.c_str()) - 4;
					if (szLen > 64 || szLen == 0)
					{
						break;
					}
					
					// terminate nick, needed for strcasecmp in HashManager
					temp[0] = '\0';
					
					User *pUser = HashManager::m_Ptr->FindUser(cur->m_sMsg.c_str() + 4, szLen);
					if (pUser == NULL)
					{
						break;
					}
					
					// add back space after nick...
					temp[0] = ' ';
					
					if (memcmp(cur->m_ui128IpHash, pUser->m_ui128IpHash, 16) != 0)
					{
						break;
					}
					
#ifdef _BUILD_GUI
					if (::SendMessage(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED)
					{
				char sMsg[128];
				int iMsgLen = snprintf(sMsg, 128, "UDP > %s (%s) > ", pUser->m_sNick, pUser->m_sIP);
				if(iMsgLen > 0)
						{
					RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], (string(sMsg, iMsgLen)+cur->m_sMsg).c_str());
						}
					}
#endif
					
			DcCommand command = { pUser, cur->m_sMsg.c_str(), cur->m_sMsg.length() };
			DcCommands::m_Ptr->SRFromUDP(&command);
				break;
			}
#endif // FLYLINKDC_USE_UDP_THREAD
			default:
				break;
		}
		
		delete cur;
	}
}
//---------------------------------------------------------------------------
