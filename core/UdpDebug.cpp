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
#include <string>
#include <spdlog/spdlog.h>
//---------------------------------------------------------------------------
std::unique_ptr<UdpDebug> UdpDebug::m_Ptr;
//---------------------------------------------------------------------------

UdpDebug::UdpDbgItem::UdpDbgItem()
{
}
//---------------------------------------------------------------------------

UdpDebug::UdpDbgItem::~UdpDbgItem()
{
    safe_closesocket(s);
}
//---------------------------------------------------------------------------

UdpDebug::~UdpDebug()
{
    DeleteAllItems();
}
//---------------------------------------------------------------------------

void UdpDebug::DeleteAllItems()
{
    m_DbgItemList.clear();
    m_ui32SubscriberCount = 0;
}
//---------------------------------------------------------------------------

void UdpDebug::Broadcast(const char* msg, const size_t szMsgLen) const
{
    if (m_DbgItemList.empty())
    {
        return;
    }

    Lock lock(m_csUdpDebug);

    {
        const uint16_t tmp = static_cast<uint16_t>(szMsgLen);
        memcpy(m_sDebugBuffer.data() + 2, &tmp, sizeof(tmp));
    }
    memcpy(m_sDebugHead, msg, szMsgLen);
    const size_t szLen = (m_sDebugHead - m_sDebugBuffer.data()) + szMsgLen;

    for (const auto& pItem : m_DbgItemList)
    {
        if (!pItem->m_bAllData)
        {
            break;
        }

        sendto(pItem->s, m_sDebugBuffer.data(), szLen, 0, reinterpret_cast<struct sockaddr*>(&pItem->sas_to), pItem->sas_len);
        ServerManager::m_ui64BytesSent += szLen;
    }
}
//---------------------------------------------------------------------------

void UdpDebug::BroadcastFormat(const char* sFormatMsg, ...) const
{
    if (m_DbgItemList.empty())
    {
        return;
    }

    Lock lock(m_csUdpDebug);

    va_list vlArgs;
    va_start(vlArgs, sFormatMsg);

    const int iRet = vsnprintf(m_sDebugHead, UINT16_MAX, sFormatMsg, vlArgs);

    va_end(vlArgs);

    if (iRet < 0 || iRet >= UINT16_MAX)
    {
        LogDbgErr("[ERR] vsnprintf wrong value {} in UdpDebug::Broadcast", iRet);

        return;
    }

    {
        const uint16_t tmp = static_cast<uint16_t>(iRet);
        memcpy(m_sDebugBuffer.data() + 2, &tmp, sizeof(tmp));
    }
    const size_t szLen = (m_sDebugHead - m_sDebugBuffer.data()) + iRet;

    for (const auto& pItem : m_DbgItemList)
    {
        if (!pItem->m_bAllData)
        {
            break;
        }

        sendto(pItem->s, m_sDebugBuffer.data(), szLen, 0, reinterpret_cast<struct sockaddr*>(&pItem->sas_to), pItem->sas_len);
        ServerManager::m_ui64BytesSent += szLen;
    }
}
//---------------------------------------------------------------------------

void UdpDebug::CreateBuffer()
{
    if (!m_sDebugBuffer.empty())
    {
        return;
    }

    m_sDebugBuffer.resize(4 + 256 + UINT16_MAX);

    UpdateHubName();
}
//---------------------------------------------------------------------------

bool UdpDebug::New(User* pUser, const uint16_t ui16Port)
{
    auto pNewDbg = std::make_unique<UdpDbgItem>();

    Lock lock(m_csUdpDebug);

    pNewDbg->m_sNick = pUser->m_sNick;

    pNewDbg->m_ui32Hash = pUser->m_ui32NickHash;

    struct in6_addr i6addr;
    memcpy(&i6addr, pUser->m_ui128IpHash.data(), 16);

    const bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

    if (bIPv6)
    {
        reinterpret_cast<struct sockaddr_in6*>(&pNewDbg->sas_to)->sin6_family = AF_INET6;
        reinterpret_cast<struct sockaddr_in6*>(&pNewDbg->sas_to)->sin6_port = htons(ui16Port);
        memcpy(reinterpret_cast<struct sockaddr_in6*>(&pNewDbg->sas_to)->sin6_addr.s6_addr, pUser->m_ui128IpHash.data(), 16);
        pNewDbg->sas_len = sizeof(struct sockaddr_in6);
    }
    else
    {
        reinterpret_cast<struct sockaddr_in*>(&pNewDbg->sas_to)->sin_family = AF_INET;
        reinterpret_cast<struct sockaddr_in*>(&pNewDbg->sas_to)->sin_port = htons(ui16Port);
        reinterpret_cast<struct sockaddr_in*>(&pNewDbg->sas_to)->sin_addr.s_addr = inet_addr(pUser->m_sIP.data());
        pNewDbg->sas_len = sizeof(struct sockaddr_in);
    }

    pNewDbg->s = socket((bIPv6 ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);
    if (pNewDbg->s == -1)
    {
        pUser->SendFormat(
            "UdpDebug::New1", true, "*** [ERR] %s: %s (%d).|", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UDP_SCK_CREATE_ERR)].c_str(), ErrnoStr(errno), errno);
        return false;
    }

    // set non-blocking
    const int oldFlag = fcntl(pNewDbg->s, F_GETFL, 0);
    if (fcntl(pNewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1)
    {
        pUser->SendFormat(
            "UdpDebug::New2", true, "*** [ERR] %s: %s (%d).|", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UDP_NON_BLOCK_FAIL)].c_str(), ErrnoStr(errno), errno);
        return false;
    }

    if (m_DbgItemList.empty())
    {
        CreateBuffer();
    }

    pNewDbg->m_bIsScript = false;
    m_DbgItemList.push_front(std::move(pNewDbg));
    UdpDbgItem* pFront = m_DbgItemList.front().get();
    m_ui32SubscriberCount++;

    const int iLen = snprintf(m_sDebugHead, UINT16_MAX, "[HUB] Subscribed, users online: %u", ServerManager::m_ui32Logged);
    if (iLen < 0 || iLen > UINT16_MAX)
    {
        LogDbgErr("[ERR] snprintf wrong value {} in UdpDebug::New", iLen);

        return true;
    }

    // create packet
    {
        const uint16_t tmp = static_cast<uint16_t>(iLen);
        memcpy(m_sDebugBuffer.data() + 2, &tmp, sizeof(tmp));
    }
    const size_t szLen = (m_sDebugHead - m_sDebugBuffer.data()) + iLen;
    sendto(pFront->s, m_sDebugBuffer.data(), szLen, 0, reinterpret_cast<struct sockaddr*>(&pFront->sas_to), pFront->sas_len);
    ServerManager::m_ui64BytesSent += szLen;

    return true;
}
//---------------------------------------------------------------------------

bool UdpDebug::New(const char* sIP, const uint16_t ui16Port, const bool bAllData, const char* sScriptName)
{
    auto pNewDbg = std::make_unique<UdpDbgItem>();

    Lock lock(m_csUdpDebug);

    // initialize dbg item
    pNewDbg->m_sNick = sScriptName;

    pNewDbg->m_ui32Hash = 0;

    std::array<uint8_t, 16> ui128IP;
    (void)HashIP(sIP, ui128IP.data());

    struct in6_addr i6addr;
    memcpy(&i6addr, ui128IP.data(), 16);

    const bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

    if (bIPv6)
    {
        reinterpret_cast<struct sockaddr_in6*>(&pNewDbg->sas_to)->sin6_family = AF_INET6;
        reinterpret_cast<struct sockaddr_in6*>(&pNewDbg->sas_to)->sin6_port = htons(ui16Port);
        memcpy(reinterpret_cast<struct sockaddr_in6*>(&pNewDbg->sas_to)->sin6_addr.s6_addr, ui128IP.data(), 16);
        pNewDbg->sas_len = sizeof(struct sockaddr_in6);
    }
    else
    {
        reinterpret_cast<struct sockaddr_in*>(&pNewDbg->sas_to)->sin_family = AF_INET;
        reinterpret_cast<struct sockaddr_in*>(&pNewDbg->sas_to)->sin_port = htons(ui16Port);
        memcpy(&reinterpret_cast<struct sockaddr_in*>(&pNewDbg->sas_to)->sin_addr.s_addr, ui128IP.data() + 12, 4);
        pNewDbg->sas_len = sizeof(struct sockaddr_in);
    }

    pNewDbg->s = socket((bIPv6 ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);

    if (pNewDbg->s == -1)
    {
        return false;
    }

    // set non-blocking
    const int oldFlag = fcntl(pNewDbg->s, F_GETFL, 0);
    if (fcntl(pNewDbg->s, F_SETFL, oldFlag | O_NONBLOCK) == -1)
    {
        return false;
    }

    if (m_DbgItemList.empty())
    {
        CreateBuffer();
    }

    pNewDbg->m_bIsScript = true;
    pNewDbg->m_bAllData = bAllData;
    m_DbgItemList.push_front(std::move(pNewDbg));
    UdpDbgItem* pFront = m_DbgItemList.front().get();
    m_ui32SubscriberCount++;

    const int iLen = snprintf(m_sDebugHead, UINT16_MAX, "[HUB] Subscribed, users online: %u", ServerManager::m_ui32Logged);
    if (iLen < 0 || iLen > UINT16_MAX)
    {
        LogDbgErr("[ERR] snprintf wrong value {} in UdpDebug::New2", iLen);

        return true;
    }

    // create packet
    {
        const uint16_t tmp = static_cast<uint16_t>(iLen);
        memcpy(m_sDebugBuffer.data() + 2, &tmp, sizeof(tmp));
    }
    const size_t szLen = (m_sDebugHead - m_sDebugBuffer.data()) + iLen;
    sendto(pFront->s, m_sDebugBuffer.data(), szLen, 0, reinterpret_cast<struct sockaddr*>(&pFront->sas_to), pFront->sas_len);
    ServerManager::m_ui64BytesSent += szLen;

    return true;
}
//---------------------------------------------------------------------------

void UdpDebug::DeleteBuffer()
{
    m_sDebugBuffer.clear();
    m_sDebugBuffer.shrink_to_fit();
    m_sDebugHead = nullptr;
}
//---------------------------------------------------------------------------

bool UdpDebug::Remove(User* pUser)
{
    Lock lock(m_csUdpDebug);

    for (auto it = m_DbgItemList.begin(); it != m_DbgItemList.end(); ++it)
    {
        UdpDbgItem* pCur = it->get();

        if (!pCur->m_bIsScript && pCur->m_ui32Hash == pUser->m_ui32NickHash && iequals(pCur->m_sNick, pUser->m_sNick))
        {
            m_DbgItemList.erase(it);
            m_ui32SubscriberCount--;

            if (m_DbgItemList.empty())
            {
                DeleteBuffer();
            }

            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void UdpDebug::Remove(const char* sScriptName)
{
    Lock lock(m_csUdpDebug);

    for (auto it = m_DbgItemList.begin(); it != m_DbgItemList.end(); ++it)
    {
        UdpDbgItem* pCur = it->get();

        if (pCur->m_bIsScript && iequals(pCur->m_sNick, sScriptName))
        {
            m_DbgItemList.erase(it);
            m_ui32SubscriberCount--;

            if (m_DbgItemList.empty())
            {
                DeleteBuffer();
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

bool UdpDebug::CheckUdpSub(User* pUser, bool bSndMess /* = false*/) const
{
    Lock lock(m_csUdpDebug);

    for (const auto& pItem : m_DbgItemList)
    {
        UdpDbgItem* pCur = pItem.get();

        if (!pCur->m_bIsScript && pCur->m_ui32Hash == pUser->m_ui32NickHash && iequals(pCur->m_sNick, pUser->m_sNick))
        {
            if (bSndMess)
            {
                pUser->SendFormat("UdpDebug::CheckUdpSub",
                                  true,
                                  "<%s> *** %s %hu. %s.|",
                                  SettingManager::HubSec(),
                                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_SUBSCRIBED_UDP_DBG)].c_str(),
                                  ntohs(pCur->sas_to.ss_family == AF_INET6 ? reinterpret_cast<struct sockaddr_in6*>(&pCur->sas_to)->sin6_port
                                                                           : reinterpret_cast<struct sockaddr_in*>(&pCur->sas_to)->sin_port),
                                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_UNSUB_UDP_DBG)].c_str()); //-V641 standard sockaddr_storage cast
            }

            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void UdpDebug::Send(const char* sScriptName, const char* sMessage, const size_t szMsgLen) const
{
    if (m_DbgItemList.empty())
    {
        return;
    }

    Lock lock(m_csUdpDebug);

    for (const auto& pItem : m_DbgItemList)
    {
        UdpDbgItem* pCur = pItem.get();

        if (pCur->m_bIsScript && iequals(pCur->m_sNick, sScriptName))
        {
            // create packet
            {
        const uint16_t tmp = static_cast<uint16_t>(szMsgLen);
                memcpy(m_sDebugBuffer.data() + 2, &tmp, sizeof(tmp));
            }
            memcpy(m_sDebugHead, sMessage, szMsgLen);
            const size_t szLen = (m_sDebugHead - m_sDebugBuffer.data()) + szMsgLen;

            sendto(pCur->s, m_sDebugBuffer.data(), szLen, 0, reinterpret_cast<struct sockaddr*>(&pCur->sas_to), pCur->sas_len);
            ServerManager::m_ui64BytesSent += szLen;

            return;
        }
    }
}
//---------------------------------------------------------------------------

void UdpDebug::Cleanup()
{
    Lock lock(m_csUdpDebug);
    DeleteAllItems();
}
//---------------------------------------------------------------------------

void UdpDebug::UpdateHubName()
{
    Lock lock(m_csUdpDebug);

    if (m_sDebugBuffer.empty())
    {
        return;
    }

    {
        const uint16_t tmp = static_cast<uint16_t>(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].size());
        memcpy(m_sDebugBuffer.data(), &tmp, sizeof(tmp));
    }
    memcpy(m_sDebugBuffer.data() + 4, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].data(), SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].size());

    m_sDebugHead = m_sDebugBuffer.data() + 4 + SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].size();
}
//---------------------------------------------------------------------------
