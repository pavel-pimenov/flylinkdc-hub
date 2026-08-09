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
#include "serviceLoop.h"
//---------------------------------------------------------------------------
#include <fstream>
#include <string>
#include <algorithm>

#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#endif
#include "LuaScript.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::unique_ptr<ServiceLoop> ServiceLoop::m_Ptr;

ServiceLoop::CFlyP2PGuardArray ServiceLoop::g_ProxyArray;
bool ServiceLoop::g_ProxyGuardLoad;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void ServiceLoop::Looper()
{
    // PPK ... two loop stategy for saving badwith
    if (m_bRecv)
    {
        ReceiveLoop();
    }
    else
    {
        SendLoop();
        EventQueue::m_Ptr->ProcessEvents();
    }

    if (!ServerManager::m_bServerTerminated) [[likely]]
    {
        m_bRecv = !m_bRecv;
    }
    else
    {

        // tell the scripts about the end
        ScriptManager::m_Ptr->OnExit();

        // send last possible global data
        GlobalDataQueue::m_Ptr->SendFinalQueue();

        ServerManager::FinalStop(true);
    }
}
//---------------------------------------------------------------------------

ServiceLoop::ServiceLoop() : m_ui64LstUptmTck(ServerManager::m_ui64ActualTick)
{
    ServerManager::m_bServerTerminated = false;

    m_ui64LastSecond = NowMonotonicMs() / 1000;
}

//---------------------------------------------------------------------------

ServiceLoop::~ServiceLoop()
{
    for (const auto& pSck : m_AcceptedSockets)
    {
        shutdown_and_close(pSck->m_Socket, SHUT_RDWR);
    }
    m_AcceptedSockets.clear();

    LogInfo("MainLoop terminated.");
}
//---------------------------------------------------------------------------

void ServiceLoop::loadProxyGuard()
{
    if (!g_ProxyGuardLoad)
    {
        g_ProxyGuardLoad = true;
        std::ifstream l_file("proxy_list.ini");
        std::string l_currentLine;
        if (l_file.is_open())
        {
            g_ProxyArray.reserve(5400);
            uint32_t a = 0, b = 0, c = 0, d = 0, a2 = 0, b2 = 0, c2 = 0, d2 = 0;
            bool l_end_file;
            do
            {
                l_end_file = getline(l_file, l_currentLine).eof();

                if (!l_currentLine.empty() && isdigit(static_cast<unsigned char>(l_currentLine[0])))
                {
                    if (l_currentLine.contains('-') && std::count(l_currentLine.begin(), l_currentLine.end(), '.') >= 6)
                    {
                        const int l_Items = sscanf(l_currentLine.c_str(), "%u.%u.%u.%u-%u.%u.%u.%u", &a, &b, &c, &d, &a2, &b2, &c2, &d2);
                        if (l_Items == 8)
                        {
                            const uint32_t l_startIP = (a << 24) + (b << 16) + (c << 8) + d;
                            const uint32_t l_endIP = (a2 << 24) + (b2 << 16) + (c2 << 8) + d2 + 1;
                            if (l_startIP >= l_endIP)
                            {
                                LogWarn("[PROXY] Error range: [{}]", l_currentLine);
                            }
                            else
                            {
                                g_ProxyArray.emplace_back(l_startIP, l_endIP);
                            }
                        }
                        else
                        {
                            LogWarn("[PROXY] Error mask d.d.d.d-d.d.d.d: [{}]", l_currentLine);
                        }
                    }
                }
                else
                {
                }
            } while (!l_end_file);
            g_ProxyArray.shrink_to_fit();
            std::ranges::sort(g_ProxyArray, [](const CFlyIPRange& left, const CFlyIPRange& right) { return left.m_start_ip < right.m_start_ip; });
            LogInfo("[PROXY] Load proxy list from RoLex: count: [{}]", g_ProxyArray.size());
        }
    }
}
//---------------------------------------------------------------------------
bool ServiceLoop::isProxy(const in_addr& p_ip4)
{
    const uint32_t l_ip4 = htonl(p_ip4.s_addr);
    // Binary search: find first range where start_ip > l_ip4, then check the preceding range
    auto it = std::upper_bound(g_ProxyArray.begin(), g_ProxyArray.end(), l_ip4, [](uint32_t ip, const CFlyIPRange& range) { return ip < range.m_start_ip; });
    if (it != g_ProxyArray.begin())
    {
        --it;
        return l_ip4 >= it->m_start_ip && l_ip4 < it->m_stop_ip;
    }
    return false;
}
//---------------------------------------------------------------------------
void ServiceLoop::AcceptUser(AcceptedSocket* pAccptSocket)
{
    bool bIPv6 = false;
    std::array<char, 40> sIP = {};
    Hash128 ui128IpHash;
    uint16_t ui16IpTableIdx = 0;

    loadProxyGuard();

    in_addr ipv4addr = {0};
    if (pAccptSocket->m_Addr.ss_family == AF_INET6)
    {
        memcpy(ui128IpHash, &reinterpret_cast<struct sockaddr_in6*>(&pAccptSocket->m_Addr)->sin6_addr.s6_addr, 16); //-V641 standard sockaddr_storage cast

        if (IN6_IS_ADDR_V4MAPPED(&reinterpret_cast<struct sockaddr_in6*>(&pAccptSocket->m_Addr)->sin6_addr)) //-V641 standard sockaddr_storage cast
        {
            memcpy(&ipv4addr, reinterpret_cast<struct sockaddr_in6*>(&pAccptSocket->m_Addr)->sin6_addr.s6_addr + 12, 4); //-V641 standard sockaddr_storage cast
            inet_ntop(AF_INET, &ipv4addr, sIP.data(), sIP.size());

            ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
        }
        else
        {
            bIPv6 = true;
            inet_ntop(AF_INET6,
                      &reinterpret_cast<struct sockaddr_in6*>(&pAccptSocket->m_Addr)->sin6_addr,
                      sIP.data(),
                      sIP.size()); //-V641 standard sockaddr_storage cast
            ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
        }
    }
    else
    {
        inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in*>(&pAccptSocket->m_Addr)->sin_addr, sIP.data(), sIP.size());

        ui128IpHash[10] = 255;
        ui128IpHash[11] = 255;
        memcpy(ui128IpHash + 12, &reinterpret_cast<struct sockaddr_in*>(&pAccptSocket->m_Addr)->sin_addr.s_addr, 4);

        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    }

#ifdef FLYLINKDC_USE_SET_SOCKET_BUFFER
    // set the recv buffer
    int bufsize = 8192;
    if (setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %s (%d)", sIP.data(), ErrnoStr(errno), errno);
        shutdown_and_close(pAccptSocket->m_Socket, SHUT_RDWR);
        return;
    }

    // set the send buffer
    bufsize = 32768;
    if (setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %s (%d)", sIP.data(), ErrnoStr(errno), errno);
        shutdown_and_close(pAccptSocket->m_Socket, SHUT_RDWR);
        return;
    }
#endif // FLYLINKDC_USE_SET_SOCKET_BUFFER
    // set sending of keepalive packets
    const int iKeepAlive = 1;
    if (setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_KEEPALIVE, &iKeepAlive, sizeof(iKeepAlive)) == -1)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_KEEPALIVE. IP: %s Err: %s (%d)", sIP.data(), ErrnoStr(errno), errno);

        shutdown_and_close(pAccptSocket->m_Socket, SHUT_RDWR);

        return;
    }

    // set non-blocking mode
    int oldFlag = fcntl(pAccptSocket->m_Socket, F_GETFL, 0);
    if (fcntl(pAccptSocket->m_Socket, F_SETFL, oldFlag | O_NONBLOCK) == -1)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] fcntl failed on attempt to set O_NONBLOCK. IP: %s Err: %s (%d)", sIP.data(), ErrnoStr(errno), errno);
        shutdown_and_close(pAccptSocket->m_Socket, SHUT_RDWR);
        return;
    }

#ifdef FLYLINKDC_USE_REDIR
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REDIRECT_ALL)])
    {
        if (!SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_REDIRECT_ADDRESS)].empty())
        {
            const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                          ServerManager::m_szGlobalBufferSize,
                                          "<%s> %s %s|%s",
                                          SettingManager::HubSec(),
                                          LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_REDIR_TO)].c_str(),
                                          SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_REDIRECT_ADDRESS)].c_str(),
                                          SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].c_str());
            if (iMsgLen > 0)
            {
                send(pAccptSocket->m_Socket, ServerManager::m_pGlobalBuffer, iMsgLen, 0);
                ServerManager::m_ui64BytesSent += iMsgLen;
                GlobalDataQueue::m_Ptr->PrometheusSendBytes(__func__, iMsgLen);
            }
        }
        shutdown_and_close(pAccptSocket->m_Socket, SHUT_RDWR);
        return;
    }
#endif

    time_t acc_time;
    time(&acc_time);

    BanItem* Ban = BanManager::m_Ptr->FindFull(ui128IpHash, acc_time);

    if (Ban)
    {
        if (((Ban->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
        {
            const int iMsgLen = GenerateBanMessage(Ban, acc_time);
            if (iMsgLen != 0)
            {
                send(pAccptSocket->m_Socket, ServerManager::m_pGlobalBuffer, iMsgLen, 0);
            }
            shutdown_and_close(pAccptSocket->m_Socket, SHUT_RD);

            return;
        }
    }

    RangeBanItem* RangeBan = BanManager::m_Ptr->FindFullRange(ui128IpHash, acc_time);

    if (RangeBan)
    {
        if (((RangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
        {
            const int iMsgLen = GenerateRangeBanMessage(RangeBan, acc_time);
            if (iMsgLen != 0)
            {
                send(pAccptSocket->m_Socket, ServerManager::m_pGlobalBuffer, iMsgLen, 0);
            }
            shutdown_and_close(pAccptSocket->m_Socket, SHUT_RD);

            return;
        }
    }

    const bool l_is_proxy = isProxy(ipv4addr);
    if (l_is_proxy)
    {
        LogInfo("[SYS] Proxy user ip {}", sIP.data());
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Proxy user ip %s", sIP.data());
    }

    ServerManager::m_ui32Joins++;

    // set properties of the new user object
    auto pUser = std::make_unique<User>();

    pUser->m_LogInOut.m_ui64LogonTick = ServerManager::m_ui64ActualTick;
    pUser->m_Socket = pAccptSocket->m_Socket;

    memcpy(pUser->m_ui128IpHash.data(), ui128IpHash, 16);
    pUser->m_ui16IpTableIdx = ui16IpTableIdx;

    pUser->SetIP(sIP.data());
    pUser->m_is_proxy_user = l_is_proxy;

    if (bIPv6)
    {
        pUser->m_ui32BoolBits |= User::BIT_IPV6;
    }
    else
    {
        pUser->m_ui32BoolBits |= User::BIT_IPV4;
    }

    if (Ban)
    {
        uint32_t hash = 0;
        if (((Ban->m_ui8Bits & BanManager::NICK) == BanManager::NICK))
        {
            hash = Ban->m_ui32NickHash;
        }
        const int iMsglen = GenerateBanMessage(Ban, acc_time);
        pUser->m_LogInOut.m_pBan = UserBan::CreateUserBan(ServerManager::m_pGlobalBuffer, iMsglen, hash);
        if (!pUser->m_LogInOut.m_pBan)
        {
            shutdown_and_close(pAccptSocket->m_Socket, SHUT_RDWR);

            LogDbg("[MEM] Cannot allocate new uBan in ServiceLoop::AcceptUser");

            return;
        }
    }
    else if (RangeBan)
    {
        const int iMsgLen = GenerateRangeBanMessage(RangeBan, acc_time);
        pUser->m_LogInOut.m_pBan = UserBan::CreateUserBan(ServerManager::m_pGlobalBuffer, iMsgLen, 0);
        if (!pUser->m_LogInOut.m_pBan)
        {
            shutdown_and_close(pAccptSocket->m_Socket, SHUT_RDWR);

            LogDbg("[MEM] Cannot allocate new uBan in ServiceLoop::AcceptUser1");

            return;
        }
    }

    // Everything is ok, now add to users...
    Users::m_Ptr->AddUser(std::move(pUser));
}
//---------------------------------------------------------------------------

void ServiceLoop::ReceiveLoop()
{
    timespec ts;
#ifdef __MACH__
    mach_timespec_t mts;
    clock_get_time(ServerManager::m_csMachClock, &mts);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif

    if (static_cast<uint64_t>(ts.tv_sec) != m_ui64LastSecond)
    {
        m_ui64LastSecond = ts.tv_sec;

        ServerManager::OnSecTimer();
    }

    ScriptOnTimer((uint64_t(ts.tv_sec) * 1000) + (uint64_t(ts.tv_nsec) / 1000000));

    // Receiving loop for process all incoming data and store in queues
    uint32_t iRecvRests = 0;

    ServerManager::m_ui8SrCntr++;

    if (ServerManager::m_ui8SrCntr >= 7 || (Users::m_Ptr->m_ui16ActSearchs + Users::m_Ptr->m_ui16PasSearchs) > 8 || Users::m_Ptr->m_ui16ActSearchs > 5)
    {
        ServerManager::m_ui8SrCntr = 0;
    }

    if (ServerManager::m_ui64ActualTick - m_ui64LstUptmTck > SECONDS_PER_MINUTE)
    {
        time_t acctime;
        time(&acctime);
        acctime -= ServerManager::m_tStartTime;

        uint64_t iValue = acctime / SECONDS_PER_DAY;
        acctime -= static_cast<time_t>(SECONDS_PER_DAY * iValue);
        ServerManager::m_ui64Days = iValue;

        iValue = acctime / SECONDS_PER_HOUR;
        acctime -= static_cast<time_t>(SECONDS_PER_HOUR * iValue);
        ServerManager::m_ui64Hours = iValue;

        iValue = acctime / SECONDS_PER_MINUTE;
        ServerManager::m_ui64Mins = iValue;

        if (ServerManager::m_ui64Mins == 0 || ServerManager::m_ui64Mins == 15 || ServerManager::m_ui64Mins == 30 || ServerManager::m_ui64Mins == 45)
        {
            RegManager::m_Ptr->Save(false, true);
        }

        m_ui64LstUptmTck = ServerManager::m_ui64ActualTick;
    }

    std::list<std::unique_ptr<AcceptedSocket>> acceptedSockets;
    {
        Lock l(m_csAcceptQueue);
        acceptedSockets.swap(m_AcceptedSockets);
    }
    for (const auto& pSck : acceptedSockets)
    {
        AcceptUser(pSck.get());
    }

    for (auto itUser = Users::m_Ptr->m_UserList.begin(); itUser != Users::m_Ptr->m_UserList.end();)
    {
        // Advance to the next user up-front so that every `continue` in the
        // state-machine switch below moves on to the next user, matching the
        // original intrusive-linked-list loop (which advanced at the top).
        // The current user is processed via curUser/itCur; state transitions
        // are handled on subsequent service-loop calls, not by re-processing
        // the same user within this loop.
        const auto itCur = itUser;
        ++itUser;

        User* curUser = itCur->get();
        if (ServerManager::m_bServerTerminated) [[unlikely]]
        {
            break;
        }

        // PPK ... true == we have rest ;)
        if (curUser->DoRecv())
        {
            iRecvRests++;
        }

        switch (curUser->m_ui8State)
        {
        case User::UserStates::STATE_SOCKET_ACCEPTED:
        {
            if (ServerManager::m_ui64ActualTick != curUser->m_LogInOut.m_ui64LogonTick) [[likely]]
            {
                if (!curUser->MakeLock())
                {
                    curUser->Close();
                    continue;
                }

                curUser->m_ui8State = User::UserStates::STATE_KEY_OR_SUP;
            }

            break;
        }
        case User::UserStates::STATE_KEY_OR_SUP:
        {
            // check logon timeout for iState 1
            if (ServerManager::m_ui64ActualTick - curUser->m_LogInOut.m_ui64LogonTick > 20) [[unlikely]]
            {
                UdpDebug::m_Ptr->BroadcastFormat("[SYS] Login timeout 1 for %s - user (%s) disconnected.", curUser->m_sIP.data(), curUser->m_sNick.c_str());

                curUser->Close();
                continue;
            }
            break;
        }
        case User::UserStates::STATE_IPV4_CHECK:
        {
            // check IPv4Check timeout
            if ((ServerManager::m_ui64ActualTick - curUser->m_LogInOut.m_ui64IPv4CheckTick) > 10) [[unlikely]]
            {
                UdpDebug::m_Ptr->BroadcastFormat("[SYS] IPv4Check timeout for %s (%s).", curUser->m_sNick.c_str(), curUser->m_sIP.data());

                curUser->m_ui8State = User::UserStates::STATE_ADDME;
                continue;
            }
            break;
        }
        case User::UserStates::STATE_ADDME:
        {
            // PPK ... Add user, but only if send $GetNickList (or have quicklist supports) <- important, used by flooders !!!
            if (!((curUser->m_ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) &&
                !((curUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) &&
                ((curUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER)) [[unlikely]]
            {
                continue;
            }
            curUser->SendFormat("ServiceLoop::ReceiveLoop->User::UserStates::STATE_ADDME",
                                true,
                                "%s%" PRIu64 " %s, %" PRIu64 " %s, %" PRIu64 " %s / %s: %u)|",
                                SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_NAME_WLCM)].c_str(),
                                ServerManager::m_ui64Days,
                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DAYS_LWR)].c_str(),
                                ServerManager::m_ui64Hours,
                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HOURS_LWR)].c_str(),
                                ServerManager::m_ui64Mins,
                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MINUTES_LWR)].c_str(),
                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USERS)].c_str(),
                                ServerManager::m_ui32Logged);
            curUser->m_ui8State = User::UserStates::STATE_ADDME_1LOOP;
            continue;
        }
        case User::UserStates::STATE_ADDME_1LOOP:
        {
            // PPK ... added login delay.
            if (m_dLoggedUsers >= m_dActualSrvLoopLogins && !((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR)) [[unlikely]]
            {
                if (ServerManager::m_ui64ActualTick - curUser->m_LogInOut.m_ui64LogonTick > 300) [[unlikely]]
                {
                    UdpDebug::m_Ptr->BroadcastFormat("[SYS] Login timeout (%d) 3 for %s (%s) - user disconnected.",
                                                     std::to_underlying(curUser->m_ui8State),
                                                     curUser->m_sNick.c_str(),
                                                     curUser->m_sIP.data());

                    curUser->Close();
                }
                continue;
            }

            // PPK ... is not more needed, free mem ;)
            curUser->FreeBuffer();

            if ((curUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && !((curUser->m_ui32BoolBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4))
            {
                in_addr ipv4addr;
                ipv4addr.s_addr = INADDR_NONE;

                if (curUser->m_ui128IpHash[0] == 32 && curUser->m_ui128IpHash[1] == 2) // 6to4 tunnel
                {
                    memcpy(&ipv4addr, curUser->m_ui128IpHash.data() + 2, 4);
                }
                else if (curUser->m_ui128IpHash[0] == 32 && curUser->m_ui128IpHash[1] == 1 && curUser->m_ui128IpHash[2] == 0 &&
                         curUser->m_ui128IpHash[3] == 0) // teredo tunnel
                {
                    uint32_t ui32Ip = 0;
                    memcpy(&ui32Ip, curUser->m_ui128IpHash.data() + 12, 4);
                    ui32Ip ^= 0xffffffff;
                    memcpy(&ipv4addr, &ui32Ip, 4);
                }

                if (ipv4addr.s_addr != INADDR_NONE)
                {
                    inet_ntop(AF_INET, &ipv4addr, curUser->m_sIPv4.data(), curUser->m_sIPv4.size());
                    curUser->m_ui8IPv4Len = static_cast<uint8_t>(strlen(curUser->m_sIPv4.data()));
                    curUser->m_ui32BoolBits |= User::BIT_IPV4;
                }
            }

            // New User Connected ... the user is operator ? invoke lua User/OpConnected
            const uint32_t iBeforeLuaLen = curUser->m_ui32SendBufDataLen;

            const bool bRet = ScriptManager::m_Ptr->UserConnected(curUser);
            if (User::isPastLogin(curUser->m_ui8State)) // connection closed by script?
            {
                if (!bRet) // only when all scripts process userconnected
                {
                    ScriptManager::m_Ptr->UserDisconnected(curUser);
                }

                continue;
            }

            if (iBeforeLuaLen < curUser->m_ui32SendBufDataLen)
            {
                const size_t szNeededLen = curUser->m_ui32SendBufDataLen - iBeforeLuaLen;

                curUser->m_LogInOut.m_Buffer.resize(szNeededLen + 1);
                memcpy(curUser->m_LogInOut.m_Buffer.data(), curUser->m_pSendBuf.get() + iBeforeLuaLen, szNeededLen);
                curUser->m_LogInOut.m_ui32UserConnectedLen = static_cast<uint32_t>(szNeededLen);
                curUser->m_LogInOut.m_Buffer[curUser->m_LogInOut.m_ui32UserConnectedLen] = '\0';
                curUser->m_ui32SendBufDataLen = iBeforeLuaLen;
                curUser->m_pSendBuf[curUser->m_ui32SendBufDataLen] = '\0';
            }

            // PPK ... wow user is accepted, now add it :)
            if (((curUser->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG))
            {
                curUser->HasSuspiciousTag();
            }

            // alex82 ... HideUser / ������� �����
            if (!((curUser->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
            {
                curUser->Add2Userlist();

                ServerManager::m_ui64TotalShare += curUser->m_ui64SharedSize;
                curUser->m_ui32BoolBits |= User::BIT_HAVE_SHARECOUNTED;
            }

            m_dLoggedUsers++;
            curUser->m_ui8State = User::UserStates::STATE_ADDME_2LOOP;

#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
            DBSQLite::m_Ptr->UpdateRecord(curUser);
#endif
#endif
            // PPK ... change to NoHello supports
            const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Hello %s|", curUser->m_sNick.c_str());
            if (iMsgLen > 0)
            {
                GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::HELLO);
            }

            // alex82 ... HideUser / ������� �����
            if (!((curUser->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
            {
                GlobalDataQueue::m_Ptr->UserIPStore(curUser);

                switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
                {
                case 0:
                    GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_sMyInfoLong.data(), curUser->m_ui16MyInfoLongLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    break;
                case 1:
                    GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_sMyInfoShort.data(),
                                                         curUser->m_ui16MyInfoShortLen,
                                                         curUser->m_sMyInfoLong.data(),
                                                         curUser->m_ui16MyInfoLongLen,
                                                         GlobalDataQueue::Cmd::MYINFO);
                    break;
                case 2:
                    GlobalDataQueue::m_Ptr->AddQueueItem(
                        curUser->m_sMyInfoShort.data(), curUser->m_ui16MyInfoShortLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    break;
                default:
                    break;
                }

                // alex82 ... HideUserKey / ������ ���� �����
                if (((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) &&
                    !((curUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
                {
                    GlobalDataQueue::m_Ptr->OpListStore(curUser->m_sNick.c_str());
                }
            }

            curUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
            break;
        }
        case User::UserStates::STATE_ADDED:
            if (!curUser->m_CmdToUserList.empty())
            {
                std::list<std::unique_ptr<PrcsdToUsrCmd>> queue = std::move(curUser->m_CmdToUserList);

                for (auto& pItem : queue)
                {
                    PrcsdToUsrCmd* cur = pItem.get();

                    if (cur->m_ui32Loops >= 2)
                    {
                        User* ToUser = HashManager::m_Ptr->FindUser(cur->m_nick);
                        if (ToUser == cur->m_pToUser)
                        {
                            if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PM_COUNT_TO_USER)] == 0 || cur->m_ui32PmCount == 0)
                            {
                                cur->m_pToUser->SendCharDelayed(cur->m_sCommand);
                            }
                            else
                            {
                                if (cur->m_pToUser->m_ui32ReceivedPmCount == 0)
                                {
                                    cur->m_pToUser->m_ui64ReceivedPmTick = ServerManager::m_ui64ActualTick;
                                }
                                else if (cur->m_pToUser->m_ui32ReceivedPmCount >=
                                         static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PM_COUNT_TO_USER)]))
                                {
                                    if (cur->m_pToUser->m_ui64ReceivedPmTick + 60 < ServerManager::m_ui64ActualTick)
                                    {
                                        cur->m_pToUser->m_ui64ReceivedPmTick = ServerManager::m_ui64ActualTick;
                                        cur->m_pToUser->m_ui32ReceivedPmCount = 0;
                                    }
                                    else
                                    {
                                        if (cur->m_ui32PmCount == 1)
                                        {
                                            curUser->SendFormat("ServiceLoop::ReceiveLoop->User::UserStates::STATE_ADDED1",
                                                                true,
                                                                "$To: %s From: %s $<%s> %s %s %s!|",
                                                                curUser->m_sNick.c_str(),
                                                                cur->m_nick.c_str(),
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SRY_LST_MSG_BCS)].c_str(),
                                                                cur->m_nick.c_str(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXC_MSG_LIMIT)].c_str());
                                        }
                                        else
                                        {
                                            curUser->SendFormat("ServiceLoop::ReceiveLoop->User::UserStates::STATE_ADDED2",
                                                                true,
                                                                "$To: %s From: %s $<%s> %s %u %s %s %s!|",
                                                                curUser->m_sNick.c_str(),
                                                                cur->m_nick.c_str(),
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_LAST)].c_str(),
                                                                cur->m_ui32PmCount,
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MSGS_NOT_SENT)].c_str(),
                                                                cur->m_nick.c_str(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXC_MSG_LIMIT)].c_str());
                                        }
                                        // dropped (limit exceeded): pItem released below
                                        continue;
                                    }
                                }
                                cur->m_pToUser->SendCharDelayed(cur->m_sCommand);
                                cur->m_pToUser->m_ui32ReceivedPmCount += cur->m_ui32PmCount;
                            }
                        }
                        // delivered or target gone: drop node (pItem owns it, freed at scope end)
                    }
                    else
                    {
                        cur->m_ui32Loops++;
                        curUser->m_CmdToUserList.push_back(std::move(pItem));
                    }
                }
            }

            if (ServerManager::m_ui8SrCntr == 0)
            {
                if (curUser->m_pCmdActive6Search)
                {
                    if (curUser->m_pCmdActive4Search)
                    {
                        GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_pCmdActive6Search->m_sCommand.data(),
                                                             curUser->m_pCmdActive6Search->m_ui32Len,
                                                             curUser->m_pCmdActive4Search->m_sCommand.data(),
                                                             curUser->m_pCmdActive4Search->m_ui32Len,
                                                             GlobalDataQueue::Cmd::ACTIVE_SEARCH_V64);
                    }
                    else
                    {
                        GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_pCmdActive6Search->m_sCommand.data(),
                                                             curUser->m_pCmdActive6Search->m_ui32Len,
                                                             nullptr,
                                                             0,
                                                             GlobalDataQueue::Cmd::ACTIVE_SEARCH_V6);
                    }
                }
                else if (curUser->m_pCmdActive4Search)
                {
                    GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_pCmdActive4Search->m_sCommand.data(),
                                                         curUser->m_pCmdActive4Search->m_ui32Len,
                                                         nullptr,
                                                         0,
                                                         GlobalDataQueue::Cmd::ACTIVE_SEARCH_V4);
                }

                if (curUser->m_pCmdPassiveSearch)
                {
                    GlobalDataQueue::Cmd eCmdType = GlobalDataQueue::Cmd::PASSIVE_SEARCH_V4;
                    if ((curUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
                    {
                        if ((curUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4)
                        {
                            if ((curUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE)
                            {
                                eCmdType = GlobalDataQueue::Cmd::PASSIVE_SEARCH_V4_ONLY;
                            }
                            else if ((curUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE)
                            {
                                eCmdType = GlobalDataQueue::Cmd::PASSIVE_SEARCH_V6_ONLY;
                            }
                            else
                            {
                                eCmdType = GlobalDataQueue::Cmd::PASSIVE_SEARCH_V64;
                            }
                        }
                        else
                        {
                            eCmdType = GlobalDataQueue::Cmd::PASSIVE_SEARCH_V6;
                        }
                    }

                    GlobalDataQueue::m_Ptr->AddQueueItem(
                        curUser->m_pCmdPassiveSearch->m_sCommand.data(), curUser->m_pCmdPassiveSearch->m_ui32Len, nullptr, 0, eCmdType);

                    User::DeletePrcsdUsrCmd(curUser->m_pCmdPassiveSearch);
                }

                // PPK ... deflood memory cleanup, if is not needed anymore
                if (!curUser->m_sLastChat.empty() && curUser->m_ui16LastChatLines < 2 &&
                    (curUser->m_ui64SameChatsTick + SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_TIME)]) < ServerManager::m_ui64ActualTick)
                {
                    curUser->m_sLastChat.clear();
                    curUser->m_ui16SameMultiChats = 0;
                    curUser->m_ui16LastChatLines = 0;
                }

                if (!curUser->m_sLastPM.empty() && curUser->m_ui16LastPmLines < 2 &&
                    (curUser->m_ui64SamePMsTick + SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_PM_TIME)]) < ServerManager::m_ui64ActualTick)
                {
                    curUser->m_sLastPM.clear();
                    curUser->m_ui16SameMultiPms = 0;
                    curUser->m_ui16LastPmLines = 0;
                }

                if (!curUser->m_LastSearch.empty() &&
                    (curUser->m_ui64SameSearchsTick + SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_TIME)]) < ServerManager::m_ui64ActualTick)
                {
                    curUser->m_LastSearch.clear();
                }
            }
            continue;
        case User::UserStates::STATE_CLOSING:
        {
            if (!((curUser->m_ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) && curUser->m_ui32SendBufDataLen != 0)
            {
                if (curUser->m_LogInOut.m_ui32ToCloseLoops != 0 || ((curUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
                {
                    (void)curUser->Try2Send();
                    curUser->m_LogInOut.m_ui32ToCloseLoops--;
                    continue;
                }
            }
            curUser->m_ui8State = User::UserStates::STATE_REMME;
            continue;
        }
            // if user is marked as dead, remove him
        case User::UserStates::STATE_REMME:
        {
            shutdown_and_close(curUser->m_Socket, SHUT_RD);

            // itUser already points to the next user; erase the current one.
            Users::m_Ptr->RemUser(itCur);
            continue;
        }
        default:
        {
            // check logon timeout
            if (ServerManager::m_ui64ActualTick - curUser->m_LogInOut.m_ui64LogonTick > 60)
            {
                UdpDebug::m_Ptr->BroadcastFormat("[SYS] Login timeout (%d) 2 for %s (%s) - user disconnected.",
                                                 std::to_underlying(curUser->m_ui8State),
                                                 curUser->m_sNick.c_str(),
                                                 curUser->m_sIP.data());

                curUser->Close();
                continue;
            }
            break;
        }
        }
    }

    if (ServerManager::m_ui8SrCntr == 0)
    {
        Users::m_Ptr->m_ui16ActSearchs = 0;
        Users::m_Ptr->m_ui16PasSearchs = 0;
    }

    m_ui32LastRecvRest = iRecvRests;
    m_ui32RecvRestsPeak = iRecvRests > m_ui32RecvRestsPeak ? iRecvRests : m_ui32RecvRestsPeak;
}
//---------------------------------------------------------------------------

void ServiceLoop::SendLoop()
{
    GlobalDataQueue::m_Ptr->PrepareQueueItems();

    // PPK ... send loop
    // now logging users get changed myinfo with myinfos
    // old users get it in this loop from queue -> badwith saving !!! no more twice myinfo =)
    // Sending Loop
    uint32_t iSendRests = 0;

    for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
    {
        User* curUser = curUserPtr.get();
        if (ServerManager::m_bServerTerminated) [[unlikely]]
        {
            break;
        }

        switch (curUser->m_ui8State)
        {
        case User::UserStates::STATE_ADDME_2LOOP:
        {
            // alex82 ... HideUser / ������� �����
            if (!((curUser->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
            {
                ServerManager::m_ui32Logged++;
            }

            if (ServerManager::m_ui32Peak < ServerManager::m_ui32Logged)
            {
                ServerManager::m_ui32Peak = ServerManager::m_ui32Logged;
                if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS_PEAK)] < static_cast<int16_t>(ServerManager::m_ui32Peak))
                {
                    SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MAX_USERS_PEAK), static_cast<int16_t>(ServerManager::m_ui32Peak));
                }
            }

            curUser->m_ui8State = User::UserStates::STATE_ADDED;

            // finaly send the nicklist/myinfos/oplist
            curUser->AddUserList();

            // PPK ... UserIP2 supports
            if (((curUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) &&
                !((curUser->m_ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) &&
                !ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::SENDALLUSERIP))
            {
                curUser->SendFormat("ServiceLoop::SendLoop->User::UserStates::STATE_ADDME_2LOOP1",
                                    true,
                                    "$UserIP %s %s|",
                                    curUser->m_sNick.c_str(),
                                    (curUser->m_sIPv4[0] == '\0' ? curUser->m_sIP.data() : curUser->m_sIPv4.data()));
            }

            curUser->m_ui32BoolBits &= ~User::BIT_GETNICKLIST;

            // PPK ... send motd ???
            if (!SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_MOTD)].empty())
            {
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_MOTD_AS_PM)])
                {
                    curUser->SendFormat("ServiceLoop::SendLoop->User::UserStates::STATE_ADDME_2LOOP2",
                                        true,
                                        SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_MOTD)].c_str(),
                                        curUser->m_sNick.c_str());
                }
                else
                {
                    curUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_MOTD)]);
                }
            }

            // check for Debug subscription
            if (((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
            {
                (void)UdpDebug::m_Ptr->CheckUdpSub(curUser, true);
            }

            if (curUser->m_LogInOut.m_ui32UserConnectedLen != 0)
            {
                (void)curUser->PutInSendBuf(curUser->m_LogInOut.m_Buffer.data(), curUser->m_LogInOut.m_ui32UserConnectedLen);

                curUser->FreeBuffer();
            }

            // Login struct no more needed, free mem ! ;)
            curUser->m_LogInOut.Clean();

            // PPK ... send all login data from buffer !
            (void)curUser->Try2Send();

            // apply one loop delay to avoid double sending of hello and oplist
            continue;
        }
        case User::UserStates::STATE_ADDED:
        {
            if (((curUser->m_ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST))
            {
                curUser->AddUserList();
                curUser->m_ui32BoolBits &= ~User::BIT_GETNICKLIST;
            }

            // process global data queues
            if (GlobalDataQueue::m_Ptr->m_bHaveItems) [[likely]]
            {
                GlobalDataQueue::m_Ptr->ProcessQueues(curUser);
            }

            if (!GlobalDataQueue::m_Ptr->m_SingleItems.empty())
            {
                GlobalDataQueue::m_Ptr->ProcessSingleItems(curUser);
            }

            // send data acumulated by queues above
            // if sending caused error, close the user
            if (curUser->m_ui32SendBufDataLen != 0) [[likely]]
            {
                // PPK ... true = we have rest ;)
                if (curUser->Try2Send())
                {
                    iSendRests++;
                }
            }
            break;
        }
        case User::UserStates::STATE_CLOSING:
        case User::UserStates::STATE_REMME:
            continue;
        default:
            (void)curUser->Try2Send();
            break;
        }
    }

    GlobalDataQueue::m_Ptr->ClearQueues();

    if (m_ui32LoopsForLogins >= 40)
    {
        m_dLoggedUsers = 0;
        m_ui32LoopsForLogins = 0;
        m_dActualSrvLoopLogins = 0;
    }

    m_dActualSrvLoopLogins += static_cast<double>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SIMULTANEOUS_LOGINS)]) / 40;
    m_ui32LoopsForLogins++;

    m_ui32LastSendRest = iSendRests;
    m_ui32SendRestsPeak = iSendRests > m_ui32SendRestsPeak ? iSendRests : m_ui32SendRestsPeak;
}
//---------------------------------------------------------------------------

void ServiceLoop::AcceptSocket(int& s, const sockaddr_storage& addr)
{
    auto pNewSocket = std::make_unique<AcceptedSocket>();

    pNewSocket->m_Socket = s;

    pNewSocket->m_Addr = addr;

    ServerManager::m_ui32ConnectionsAccepted++;

    Lock l(m_csAcceptQueue);

    m_AcceptedSockets.push_back(std::move(pNewSocket));
}
//---------------------------------------------------------------------------
