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
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "UDPThread.h"
#ifdef FLYLINKDC_USE_UDP_THREAD
//---------------------------------------------------------------------------
UDPThread* UDPThread::m_PtrIPv4 = nullptr;
#ifdef FLYLINKDC_USE_UDP_THREAD_IP6
UDPThread* UDPThread::m_PtrIPv6 = nullptr;
#endif
//---------------------------------------------------------------------------

UDPThread::UDPThread() : m_ThreadId(0), m_Sock(-1), m_bTerminated(false)
{
    rcvbuf[0] = '\0';
}

bool UDPThread::Listen(int iAddressFamily)
{
    m_Sock = socket(iAddressFamily, SOCK_DGRAM, IPPROTO_UDP);

    if (m_Sock == -1)
    {
        LogError("UDP Socket creation error.");
        return false;
    }

    const int on = 1;
    setsockopt(m_Sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    sockaddr_storage sas = {};
    socklen_t sas_len;

    if (iAddressFamily == AF_INET6)
    {
        reinterpret_cast<sockaddr_in6*>(&sas)->sin6_family = AF_INET6;
        int iUdpPort = 0;
        if (safe_stoi(SettingManager::m_Ptr->GetText(std::to_underlying(SetTxtIds::SETTXT_UDP_PORT)).c_str(), iUdpPort))
        {
            reinterpret_cast<sockaddr_in6*>(&sas)->sin6_port = htons(static_cast<unsigned short>(iUdpPort));
        }
        sas_len = sizeof(struct sockaddr_in6);

        if (SettingManager::m_Ptr->GetBool(std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)) && ServerManager::m_sHubIP6[0] != '\0')
        {
            inet_pton(AF_INET6, ServerManager::m_sHubIP6.data(), &reinterpret_cast<sockaddr_in6*>(&sas)->sin6_addr);
        }
        else
        {
            reinterpret_cast<sockaddr_in6*>(&sas)->sin6_addr = in6addr_any;

            if (iAddressFamily == AF_INET6 && ServerManager::m_bIPv6DualStack)
            {
                const int iIPv6 = 0;
                setsockopt(m_Sock, IPPROTO_IPV6, IPV6_V6ONLY, &iIPv6, sizeof(iIPv6));
            }
        }
    }
    else
    {
        reinterpret_cast<sockaddr_in*>(&sas)->sin_family = AF_INET;
        int iUdpPort = 0;
        if (safe_stoi(SettingManager::m_Ptr->GetText(std::to_underlying(SetTxtIds::SETTXT_UDP_PORT)).c_str(), iUdpPort))
        {
            reinterpret_cast<sockaddr_in*>(&sas)->sin_port = htons(static_cast<unsigned short>(iUdpPort));
        }
        sas_len = sizeof(struct sockaddr_in);

        if (SettingManager::m_Ptr->GetBool(std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)) && ServerManager::m_sHubIP[0] != '\0')
        {
            reinterpret_cast<sockaddr_in*>(&sas)->sin_addr.s_addr = inet_addr(ServerManager::m_sHubIP.data());
        }
        else
        {
            reinterpret_cast<sockaddr_in*>(&sas)->sin_addr.s_addr = INADDR_ANY;
        }
    }

    if (bind(m_Sock, reinterpret_cast<struct sockaddr*>(&sas), sas_len) == -1)
    {
        LogError("UDP Socket bind error: {} ({})", ErrnoStr(errno), errno);
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

UDPThread::~UDPThread()
{
    if (m_ThreadId != 0)
    {
        Close();
        WaitFor();
    }
}
//---------------------------------------------------------------------------

namespace {
void* ExecuteUDP(void* pThread)
{
    (reinterpret_cast<UDPThread*>(pThread))->Run();

    return 0;
}
} // namespace
//---------------------------------------------------------------------------

void UDPThread::Resume()
{
    const int iRet = pthread_create(&m_ThreadId, nullptr, ExecuteUDP, this);
    if (iRet != 0)
    {
        LogDbg("[ERR] Failed to create new UDPThread");
    }
}
//---------------------------------------------------------------------------

void UDPThread::Run()
{
    sockaddr_storage sas;
    socklen_t sas_len = sizeof(sockaddr_storage);

    while (!m_bTerminated)
    {
        const int len = recvfrom(m_Sock, rcvbuf.data(), rcvbuf.size() - 1, 0, reinterpret_cast<struct sockaddr*>(&sas), &sas_len);

        if (len < 5 || !MatchBytes(rcvbuf.data(), "$SR "))
        {
            continue;
        }

        rcvbuf[len] = '\0';

        // added ip check, we don't want fake $SR causing kick of innocent user...
        EventQueue::m_Ptr->AddThread(EventQueue::EventType::UDP_SR, rcvbuf.data(), &sas);
    }
}
//---------------------------------------------------------------------------

void UDPThread::Close()
{
    m_bTerminated = true;
    shutdown(m_Sock, SHUT_RDWR);
    safe_closesocket(m_Sock);
}
//---------------------------------------------------------------------------

void UDPThread::WaitFor()
{
    if (m_ThreadId != 0)
    {
        pthread_join(m_ThreadId, nullptr);
        m_ThreadId = 0;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

UDPThread* UDPThread::Create(const int iAddressFamily)
{
    auto pUDPThread = std::make_unique<UDPThread>();

    if (pUDPThread->Listen(iAddressFamily))
    {
        pUDPThread->Resume();
        return pUDPThread.release();
    }

    return nullptr;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void UDPThread::Destroy(UDPThread*& pUDPThread)
{
    if (pUDPThread)
    {
        pUDPThread->Close();
        pUDPThread->WaitFor();
        delete pUDPThread;
        pUDPThread = nullptr;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // FLYLINKDC_USE_UDP_THREAD