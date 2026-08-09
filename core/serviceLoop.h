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
#ifndef serviceLoopH
#define serviceLoopH

#include <list>
#include <memory>
#include <vector>
#include "CriticalSection.h"

//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class ServiceLoop
{
private:
    uint64_t m_ui64LstUptmTck = 0;
    CriticalSection m_csAcceptQueue;

    uint64_t m_ui64LastSecond = 0;

    struct AcceptedSocket
    {
        sockaddr_storage m_Addr = {};

        int m_Socket = -1;

        AcceptedSocket() = default;

        AcceptedSocket(const AcceptedSocket&) = delete;

        auto operator=(const AcceptedSocket&) -> AcceptedSocket& = delete;
    };

    std::list<std::unique_ptr<AcceptedSocket>> m_AcceptedSockets;

    static void AcceptUser(AcceptedSocket* pAccptSocket);

protected:
public:
    double m_dLoggedUsers = 0, m_dActualSrvLoopLogins = 0;

    static std::unique_ptr<ServiceLoop> m_Ptr;

    uint32_t m_ui32LastSendRest = 0, m_ui32SendRestsPeak = 0, m_ui32LastRecvRest = 0, m_ui32RecvRestsPeak = 0, m_ui32LoopsForLogins = 0;

    bool m_bRecv = true;

    ServiceLoop(const ServiceLoop&) = delete;
    auto operator=(const ServiceLoop&) -> ServiceLoop& = delete;

    ServiceLoop();
    ~ServiceLoop();

    void AcceptSocket(int& s, const sockaddr_storage& addr);
    void ReceiveLoop();
    void SendLoop();
    void Looper();

private:
    struct CFlyIPRange
    {
        uint32_t m_start_ip;
        uint32_t m_stop_ip;
        CFlyIPRange(uint32_t p_start_ip, uint32_t p_stop_ip) : m_start_ip(p_start_ip), m_stop_ip(p_stop_ip) {}
    };
    using CFlyP2PGuardArray = std::vector<CFlyIPRange>;
    static CFlyP2PGuardArray g_ProxyArray;
    static bool g_ProxyGuardLoad;
    static void loadProxyGuard();
    [[nodiscard]] static auto isProxy(const in_addr& p_ip4) -> bool;
};
//---------------------------------------------------------------------------

#endif
