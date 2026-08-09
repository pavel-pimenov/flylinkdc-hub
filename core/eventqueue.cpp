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
//---------------------------------------------------------------------------
#include "LuaScript.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::unique_ptr<EventQueue> EventQueue::m_Ptr;
//---------------------------------------------------------------------------

EventQueue::Event::Event(const char* p_message)
{
    m_ui128IpHash.fill(0);
    if (p_message)
    {
        m_sMsg = p_message;
    }
}
//---------------------------------------------------------------------------

EventQueue::~EventQueue() = default;
//---------------------------------------------------------------------------

void EventQueue::AddNormal(EventType eId, const char* sMsg)
{
    if (eId != EventType::RSTSCRIPT && eId != EventType::STOPSCRIPT)
    {
        for (const auto& pCur : m_NormalEvents)
        {
            if (pCur->m_eId == eId)
            {
                return;
            }
        }
    }

    auto pNewEvent = std::make_unique<Event>(sMsg);

    pNewEvent->m_eId = eId;

    m_NormalEvents.push_back(std::move(pNewEvent));
    m_ui32NormalDepth++;
}
//---------------------------------------------------------------------------

void EventQueue::AddThread(EventType eId, const char* sMsg, const sockaddr_storage* sas /* = nullptr*/)
{
    auto pNewEvent = std::make_unique<Event>(sMsg);

    pNewEvent->m_eId = eId;

    if (sas)
    {
        if (sas->ss_family == AF_INET6)
        {
            std::copy_n(reinterpret_cast<const uint8_t*>(&reinterpret_cast<const sockaddr_in6*>(sas)->sin6_addr.s6_addr), 16, pNewEvent->m_ui128IpHash.begin());
        }
        else
        {
            pNewEvent->m_ui128IpHash.fill(0);
            pNewEvent->m_ui128IpHash[10] = 255;
            pNewEvent->m_ui128IpHash[11] = 255;
            std::copy_n(reinterpret_cast<const uint8_t*>(&reinterpret_cast<const sockaddr_in*>(sas)->sin_addr.s_addr), 4, pNewEvent->m_ui128IpHash.begin() + 12);
        }
    }
    else
    {
        pNewEvent->m_ui128IpHash.fill(0);
    }

    Lock l(m_csEventQueue);

    m_ThreadEvents.push_back(std::move(pNewEvent));
}
//---------------------------------------------------------------------------

void EventQueue::ProcessEvents()
{
    std::list<std::unique_ptr<Event>> normalEvents;
    normalEvents.swap(m_NormalEvents);
    m_ui32NormalDepth = 0;

    for (const auto& cur : normalEvents)
    {
        switch (cur->m_eId)
        {
        case EventType::RESTART:
            ServerManager::m_bIsRestart = true;
            ServerManager::Stop();
            break;
        case EventType::RSTSCRIPTS:
            ScriptManager::m_Ptr->Restart();
            break;
        case EventType::RSTSCRIPT:
        {
            Script* pScript = ScriptManager::m_Ptr->FindScript(cur->m_sMsg.c_str());
            if (!pScript || !pScript->m_bEnabled || !pScript->m_pLua)
            {
                break;
            }

            ScriptManager::m_Ptr->StopScript(pScript, false);

            if (!ScriptManager::m_Ptr->StartScript(pScript, false))
            {
                LogDbgErr("[EVENT] Failed to restart script: {}", pScript->m_sName);
            }

            break;
        }
        case EventType::STOPSCRIPT:
        {
            Script* pScript = ScriptManager::m_Ptr->FindScript(cur->m_sMsg.c_str());
            if (!pScript || !pScript->m_bEnabled || !pScript->m_pLua)
            {
                break;
            }

            ScriptManager::m_Ptr->StopScript(pScript, true);

            break;
        }
        case EventType::STOP_SCRIPTING:
            if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
            {
                SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)] = false;
                ScriptManager::m_Ptr->OnExit(true);
                ScriptManager::m_Ptr->Stop();
            }

            break;
        case EventType::SHUTDOWN:
            if (ServerManager::m_bIsClose)
            {
                break;
            }

            ServerManager::m_bIsClose = true;
            ServerManager::Stop();

            break;
        default:
            break;
        }
    }

    std::list<std::unique_ptr<Event>> threadEvents;
    {
        Lock l(m_csEventQueue);

        threadEvents.swap(m_ThreadEvents);
    }

    for (const auto& cur : threadEvents)
    {
        switch (cur->m_eId)
        {
        case EventType::REGSOCK_MSG:
        case EventType::SRVTHREAD_MSG:
        {
            UdpDebug::m_Ptr->Broadcast(cur->m_sMsg);
            break;
        }
#ifdef FLYLINKDC_USE_UDP_THREAD
        case EventType::UDP_SR:
        {
            const size_t szMsgLen = cur->m_sMsg.length();
            ServerManager::m_ui64BytesRead += static_cast<uint64_t>(szMsgLen);
            if (szMsgLen > 4)
            {
                const char* temp = strchr(cur->m_sMsg.c_str() + 4, ' ');
                if (!temp)
                {
                    break;
                }

                const size_t szLen = (temp - cur->m_sMsg.c_str()) - 4;
                if (szLen > 64 || szLen == 0)
                {
                    break;
                }

                // terminate nick, needed for strcasecmp in HashManager
                temp[0] = '\0';

                User* const pUser = HashManager::m_Ptr->FindUser(std::string_view(cur->m_sMsg.c_str() + 4, szLen));
                if (!pUser)
                {
                    break;
                }

                // add back space after nick...
                temp[0] = ' ';

                if (cur->m_ui128IpHash != pUser->m_ui128IpHash)
                {
                    break;
                }

                DcCommand command = {pUser, const_cast<char*>(cur->m_sMsg.c_str()), cur->m_sMsg.length()};
                DcCommands::m_Ptr->SRFromUDP(&command);
                break;
            }
#endif // FLYLINKDC_USE_UDP_THREAD
        default:
            break;
        }
        }
    }
    //---------------------------------------------------------------------------
