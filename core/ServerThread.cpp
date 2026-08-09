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
#include "eventqueue.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "ServerThread.h"
//---------------------------------------------------------------------------

static constexpr int g_iListenBacklog = 512;

std::atomic<uint32_t> ServerThread::m_ui32ConnectionFloodCount{0};

uint32_t ServerThread::GetTotalAntiFloodCount()
{
    uint32_t count = 0;
    for (const auto& pServer : ServerManager::m_Servers)
    {
        count += pServer->m_ui32AntiFloodCount.load(std::memory_order_relaxed);
    }
    return count;
}

ServerThread::AntiConFlood::AntiConFlood(const uint8_t* pIpHash) : m_ui64Time(ServerManager::m_ui64ActualTick), m_ui16Hits(1)
{
    memcpy(m_ui128IpHash.data(), pIpHash, 16);
}
//---------------------------------------------------------------------------

ServerThread::ServerThread(const int iAddrFamily, const uint16_t ui16PortNumber) : m_iAdressFamily(iAddrFamily), m_ui16Port(ui16PortNumber) {}
//---------------------------------------------------------------------------

ServerThread::~ServerThread()
{
    if (m_ThreadId != 0)
    {
        Close();
        WaitFor();
    }

    m_AntiFloodMap.clear();
    m_ui32AntiFloodCount = 0;
}
//---------------------------------------------------------------------------

namespace {
void* ExecuteServerThread(void* pThread)
{
    (reinterpret_cast<ServerThread*>(pThread))->Run();

    return nullptr;
}
} // namespace
//---------------------------------------------------------------------------

void ServerThread::Resume()
{
    const int iRet = pthread_create(&m_ThreadId, nullptr, ExecuteServerThread, this);
    if (iRet != 0)
    {
        LogDbg("[ERR] Failed to create new ServerThread");
    }
}
//---------------------------------------------------------------------------

void ServerThread::Run()
{
    m_bActive = true;
    int s = -1;
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 1000000;

    while (!m_bTerminated)
    {
        s = accept(m_Server, reinterpret_cast<struct sockaddr*>(&addr), &len);

        if (m_ui32SuspendTime == 0)
        {
            if (m_bTerminated)
            {
                shutdown_and_close(s, SHUT_RDWR);
                continue;
            }

            if (s == -1)
            {
                if (errno != EWOULDBLOCK) [[unlikely]]
                {
                    if (errno == EMFILE) // max opened file descriptors limit reached
                    {
                        sleep(1); // longer sleep give us better chance to have free file descriptor available on next accept call
                    }
                    else
                    {
                        EventQueue::m_Ptr->AddThread(EventQueue::EventType::SRVTHREAD_MSG,
                                                     ("[ERR] accept() for port " + std::to_string(m_ui16Port) + " has returned error.").c_str());
                    }
                }
            }
            else
            {
                if (isFlooder(s, addr)) [[unlikely]]
                {
                    shutdown_and_close(s, SHUT_RDWR);
                }

                nanosleep(&sleeptime, nullptr);
            }
        }
        else
        {
            uint32_t iSec = 0;
            while (!m_bTerminated)
            {
                if (m_ui32SuspendTime > iSec)
                {
                    sleep(1);
                    if (!m_bSuspended)
                    {
                        iSec++;
                    }
                    continue;
                }

                {
                    Lock l(m_csServerThread);
                    m_ui32SuspendTime = 0;
                }
                if (Listen(true))
                {
                    EventQueue::m_Ptr->AddThread(
                        EventQueue::EventType::SRVTHREAD_MSG,
                        ("[SYS] Server socket for port " + std::to_string(m_ui16Port) + " sucessfully recovered from suspend state.").c_str());
                }
                else
                {
                    Close();
                }
                break;
            }
        }
    }

    m_bActive = false;
}
//---------------------------------------------------------------------------

void ServerThread::Close()
{
    m_bTerminated = true;
    shutdown(m_Server, SHUT_RDWR);
    safe_closesocket(m_Server);
}
//---------------------------------------------------------------------------

void ServerThread::WaitFor()
{
    if (m_ThreadId != 0)
    {
        pthread_join(m_ThreadId, nullptr);
        m_ThreadId = 0;
    }
}
//---------------------------------------------------------------------------

bool ServerThread::Listen(const bool bSilent /* = false*/)
{
    m_Server = socket(m_iAdressFamily, SOCK_STREAM, IPPROTO_TCP);
    if (m_Server == -1)
    {
        if (bSilent)
        {
            EventQueue::m_Ptr->AddThread(
                EventQueue::EventType::SRVTHREAD_MSG,
                ("[ERR] Unable to create server socket for port " + std::to_string(m_ui16Port) + " ! ErrorCode " + std::to_string(errno)).c_str());
        }
        else
        {
            LogInfo(
                "{} {} ! {} {}", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNB_CRT_SRVR_SCK)], m_ui16Port, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_CODE)], errno);
        }
        return false;
    }

    constexpr int on = 1;
    if (setsockopt(m_Server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
    {
        if (bSilent)
        {
            EventQueue::m_Ptr->AddThread(
                EventQueue::EventType::SRVTHREAD_MSG,
                ("[ERR] Server socket setsockopt error: " + std::to_string(errno) + " for port: " + std::to_string(m_ui16Port)).c_str());
        }
        else
        {
            LogInfo("{}: {} ({}) {}: {}",
                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SRV_SCKOPT_ERR)],
                    ErrnoStr(errno),
                    errno,
                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FOR_PORT_LWR)],
                    m_ui16Port);
        }
        close(m_Server);
        return false;
    }

    // set the socket properties
    sockaddr_storage sas{};
    socklen_t sas_len;

    if (m_iAdressFamily == AF_INET6)
    {
        reinterpret_cast<sockaddr_in6*>(&sas)->sin6_family = AF_INET6;
        reinterpret_cast<sockaddr_in6*>(&sas)->sin6_port = htons(m_ui16Port);
        sas_len = sizeof(sockaddr_in6);

        if (SettingManager::m_Ptr->GetBool(std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)) && ServerManager::m_sHubIP6[0] != '\0')
        {
            inet_pton(AF_INET6, ServerManager::m_sHubIP6.data(), &reinterpret_cast<sockaddr_in6*>(&sas)->sin6_addr); //-V641 standard sockaddr_storage cast
        }
        else
        {
            reinterpret_cast<sockaddr_in6*>(&sas)->sin6_addr = in6addr_any;

            if (ServerManager::m_bIPv6DualStack && !SettingManager::m_Ptr->GetBool(std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)))
            {
                constexpr int iIPv6 = 0;
                setsockopt(m_Server, IPPROTO_IPV6, IPV6_V6ONLY, &iIPv6, sizeof(iIPv6));
            }
        }
    }
    else
    {
        reinterpret_cast<sockaddr_in*>(&sas)->sin_family = AF_INET;
        reinterpret_cast<sockaddr_in*>(&sas)->sin_port = htons(m_ui16Port);
        sas_len = sizeof(sockaddr_in);

        if (SettingManager::m_Ptr->GetBool(std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)) && ServerManager::m_sHubIP[0] != '\0')
        {
            reinterpret_cast<sockaddr_in*>(&sas)->sin_addr.s_addr = inet_addr(ServerManager::m_sHubIP.data());
        }
        else
        {
            reinterpret_cast<sockaddr_in*>(&sas)->sin_addr.s_addr = INADDR_ANY;
        }
    }

    // bind it
    if (bind(m_Server, reinterpret_cast<sockaddr*>(&sas), sas_len) == -1)
    {
        if (bSilent)
        {
            const std::string errMsg =
                "[ERR] Server socket bind error: " + std::string(ErrnoStr(errno)) + " (" + std::to_string(errno) + ") for port: " + std::to_string(m_ui16Port);
            EventQueue::m_Ptr->AddThread(EventQueue::EventType::SRVTHREAD_MSG, errMsg.c_str());
        }
        else
        {
            LogInfo("{}: {} ({}) {}: {}",
                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SRV_BIND_ERR)],
                    ErrnoStr(errno),
                    errno,
                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FOR_PORT_LWR)],
                    m_ui16Port);
        }
        safe_closesocket(m_Server);
        return false;
    }

    // set listen mode
    if (listen(m_Server, g_iListenBacklog) == -1)
    {
        if (bSilent)
        {
            EventQueue::m_Ptr->AddThread(EventQueue::EventType::SRVTHREAD_MSG,
                                         ("[ERR] Server socket listen() error: " + std::to_string(errno) + " for port: " + std::to_string(m_ui16Port)).c_str());
        }
        else
        {
            LogInfo(
                "{}: {} {}: {}", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SRV_LISTEN_ERR)], errno, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FOR_PORT_LWR)], m_ui16Port);
        }
        safe_closesocket(m_Server);
        return false;
    }
    LogInfo("Listen port: {}", m_ui16Port);

    return true;
}
//---------------------------------------------------------------------------

bool ServerThread::isFlooder(int& s, const sockaddr_storage& addr)
{
    Hash128 ui128IpHash;

    if (addr.ss_family == AF_INET6)
    {
        memcpy(ui128IpHash, &reinterpret_cast<const sockaddr_in6*>(&addr)->sin6_addr, 16); //-V641 standard sockaddr_storage cast
    }
    else
    {
        const auto l_ip4 = reinterpret_cast<const sockaddr_in*>(&addr)->sin_addr.s_addr;
        ui128IpHash[10] = 255;
        ui128IpHash[11] = 255;
        memcpy(ui128IpHash, &l_ip4, 4);
    }

    const int16_t iConDefloodCount = SettingManager::m_Ptr->GetShort(std::to_underlying(SetShortIds::SETSHORT_NEW_CONNECTIONS_COUNT));
    const int16_t iConDefloodTime = SettingManager::m_Ptr->GetShort(std::to_underlying(SetShortIds::SETSHORT_NEW_CONNECTIONS_TIME));

    const AntiConFloodKey key(ui128IpHash);
    const auto it = m_AntiFloodMap.find(key);
    if (it != m_AntiFloodMap.end())
    {
        if (it->second.m_ui64Time + static_cast<uint64_t>(iConDefloodTime) >= ServerManager::m_ui64ActualTick) [[likely]]
        {
            it->second.m_ui16Hits++;
            if (it->second.m_ui16Hits > iConDefloodCount) [[unlikely]]
            {
                m_ui32ConnectionFloodCount++;
                return true;
            }
            ServiceLoop::m_Ptr->AcceptSocket(s, addr);
            return false;
        }
        else
        {
            m_AntiFloodMap.erase(it);
            m_ui32AntiFloodCount.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    // Cap AntiConFlood list to prevent memory exhaustion DoS from distributed connections
    if (GetTotalAntiFloodCount() > 100000)
    {
        LogInfo("[SECURITY] AntiConFlood list exceeded 100000 entries - dropping connection");
        m_ui32ConnectionFloodCount++;
        return true;
    }

    m_AntiFloodMap.emplace(key, AntiConFlood(ui128IpHash));
    m_ui32AntiFloodCount.fetch_add(1, std::memory_order_relaxed);

    ServiceLoop::m_Ptr->AcceptSocket(s, addr);

    return false;
}
//---------------------------------------------------------------------------

void ServerThread::ResumeSck()
{
    if (m_bActive)
    {
        Lock l(m_csServerThread);
        m_bSuspended = false;
        m_ui32SuspendTime = 0;
    }
}
//---------------------------------------------------------------------------

void ServerThread::SuspendSck(const uint32_t ui32Time)
{
    if (m_bActive)
    {
        Lock l(m_csServerThread);
        if (ui32Time != 0)
        {
            m_ui32SuspendTime = ui32Time;
        }
        else
        {
            m_bSuspended = true;
            m_ui32SuspendTime = 1;
        }
        safe_closesocket(m_Server);
    }
}
//---------------------------------------------------------------------------
