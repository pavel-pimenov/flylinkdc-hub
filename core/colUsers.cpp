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
#include "colUsers.h"
//---------------------------------------------------------------------------
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include <cstring>
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
static constexpr uint32_t g_ui32MyInfoListSize = 1024 * 256;
static constexpr uint32_t g_ui32IpListSize = 1024 * 64;
static constexpr uint32_t g_ui32ZListSize = 1024 * 16;
static constexpr uint32_t g_ui32ZMyInfoListSize = 1024 * 128;
//---------------------------------------------------------------------------
std::unique_ptr<Users> Users::m_Ptr;
//---------------------------------------------------------------------------

Users::RecTime::RecTime(const uint8_t* pIpHash)
{
    std::copy_n(pIpHash, 16, m_ui128IpHash.begin());
}
//---------------------------------------------------------------------------

Users::Users()
{
    m_NickList.resize(NICKLISTSIZE);
    memcpy(m_NickList.data(), "$NickList |", 11);
    m_NickList[11] = '\0';
    m_ui32NickListLen = 11;
    m_ui32NickListSize = NICKLISTSIZE - 1;

    m_ZNickList.resize(g_ui32ZListSize);
    m_ui32ZNickListLen = 0;
    m_ui32ZNickListSize = g_ui32ZListSize - 1;

    m_OpList.resize(OPLISTSIZE);
    memcpy(m_OpList.data(), "$OpList |", 9);
    m_OpList[9] = '\0';
    m_ui32OpListLen = 9;
    m_ui32OpListSize = OPLISTSIZE - 1;

    m_ZOpList.resize(g_ui32ZListSize);
    m_ui32ZOpListLen = 0;
    m_ui32ZOpListSize = g_ui32ZListSize - 1;

    if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 0)
    {
        m_MyInfos.resize(g_ui32MyInfoListSize);
        m_ui32MyInfosSize = g_ui32MyInfoListSize - 1;

        m_ZMyInfos.resize(g_ui32ZMyInfoListSize);
        m_ui32ZMyInfosSize = g_ui32ZMyInfoListSize - 1;
    }
    else
    {
        m_MyInfos = {};
        m_ui32MyInfosSize = 0;
        m_ZMyInfos = {};
        m_ui32ZMyInfosSize = 0;
    }
    m_ui32MyInfosLen = 0;
    m_ui32ZMyInfosLen = 0;

    if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 2)
    {
        m_MyInfosTag.resize(g_ui32MyInfoListSize);
        m_ui32MyInfosTagSize = g_ui32MyInfoListSize - 1;

        m_ZMyInfosTag.resize(g_ui32ZMyInfoListSize);
        m_ui32ZMyInfosTagSize = g_ui32ZMyInfoListSize - 1;
    }
    else
    {
        m_MyInfosTag = {};
        m_ui32MyInfosTagSize = 0;
        m_ZMyInfosTag = {};
        m_ui32ZMyInfosTagSize = 0;
    }
    m_ui32MyInfosTagLen = 0;
    m_ui32ZMyInfosTagLen = 0;

    m_UserIPList.resize(g_ui32IpListSize);
    memcpy(m_UserIPList.data(), "$UserIP |", 9);
    m_UserIPList[9] = '\0';
    m_ui32UserIPListLen = 9;
    m_ui32UserIPListSize = g_ui32IpListSize - 1;

    m_ZUserIPList.resize(g_ui32ZListSize);
    m_ui32ZUserIPListLen = 0;
    m_ui32ZUserIPListSize = g_ui32ZListSize - 1;
}
//---------------------------------------------------------------------------

Users::~Users() = default;
//---------------------------------------------------------------------------

void Users::AddUser(std::unique_ptr<User> pUser)
{
    m_UserList.push_back(std::move(pUser));
}
//---------------------------------------------------------------------------

void Users::RemUser(User* pUser)
{
    for (auto it = m_UserList.begin(); it != m_UserList.end(); ++it)
    {
        if (it->get() == pUser)
        {
            const bool bError = (pUser->m_ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR;
            LogDbg("[DCONN] user removed: nick='{}' ip='{}' state={} error={}",
                   pUser->m_sNick.empty() ? "<unknown>" : pUser->m_sNick,
                   pUser->m_sIP.data(),
                   std::to_underlying(pUser->m_ui8State),
                   bError);
            m_UserList.erase(it);
            return;
        }
    }
}
//---------------------------------------------------------------------------

Users::UserList::iterator Users::RemUser(UserList::iterator it)
{
    return m_UserList.erase(it);
}
//---------------------------------------------------------------------------

void Users::DisconnectAll()
{
    uint32_t iCloseLoops = 0;

    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 50000000;

    while (!m_UserList.empty() && iCloseLoops <= 100)
    {
        auto it = m_UserList.begin();
        while (it != m_UserList.end())
        {
            User* u = it->get();

            if (((u->m_ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) || u->m_ui32SendBufDataLen == 0)
            {
                shutdown_and_close(u->m_Socket, SHUT_RD);
                it = m_UserList.erase(it);
            }
            else
            {
                (void)u->Try2Send();
                ++it;
            }
        }
        iCloseLoops++;
        nanosleep(&sleeptime, nullptr);
    }

    for (const auto& pUser : m_UserList)
    {
        shutdown_and_close(pUser->m_Socket, SHUT_RDWR);
    }
    m_UserList.clear();
}
//---------------------------------------------------------------------------

void Users::Add2NickList(User* pUser)
{
    // $NickList nick$$nick2$$|

    if (m_ui32NickListSize < m_ui32NickListLen + pUser->m_sNick.size() + 2)
    {
        try
        {
            m_NickList.resize(m_ui32NickListSize + NICKLISTSIZE + 1);
        }
        catch (const std::bad_alloc&)
        {
            pUser->m_ui32BoolBits |= User::BIT_ERROR;

            LogDbg("Cannot reallocate {} bytes in Users::Add2NickList for m_NickList", m_ui32NickListSize + NICKLISTSIZE + 1);

            pUser->Close();
            return;
        }
        m_ui32NickListSize = static_cast<uint32_t>(m_NickList.size()) - 1;
    }

    memcpy(m_NickList.data() + m_ui32NickListLen - 1, pUser->m_sNick.data(), pUser->m_sNick.size());
    m_ui32NickListLen += static_cast<uint32_t>(pUser->m_sNick.size() + 2);

    m_NickList[m_ui32NickListLen - 3] = '$';
    m_NickList[m_ui32NickListLen - 2] = '$';
    m_NickList[m_ui32NickListLen - 1] = '|';
    m_NickList[m_ui32NickListLen] = '\0';

    m_ui32ZNickListLen = 0;

    // alex82 ... HideUserKey / ������ ���� �����
    if (!((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) || ((pUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
    {
        return;
    }

    if (m_ui32OpListSize < m_ui32OpListLen + pUser->m_sNick.size() + 2)
    {
        try
        {
            m_OpList.resize(m_ui32OpListSize + OPLISTSIZE + 1);
        }
        catch (const std::bad_alloc&)
        {
            pUser->m_ui32BoolBits |= User::BIT_ERROR;

            LogDbgErr("[MEM] Cannot reallocate {} bytes in Users::Add2NickList for m_OpList", m_ui32OpListSize + OPLISTSIZE + 1);

            pUser->Close();
            return;
        }
        m_ui32OpListSize = static_cast<uint32_t>(m_OpList.size()) - 1;
    }

    memcpy(m_OpList.data() + m_ui32OpListLen - 1, pUser->m_sNick.data(), pUser->m_sNick.size());
    m_ui32OpListLen += static_cast<uint32_t>(pUser->m_sNick.size() + 2);

    m_OpList[m_ui32OpListLen - 3] = '$';
    m_OpList[m_ui32OpListLen - 2] = '$';
    m_OpList[m_ui32OpListLen - 1] = '|';
    m_OpList[m_ui32OpListLen] = '\0';

    m_ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void Users::AddBot2NickList(const char* sNick, const size_t szNickLen, const bool bIsOp)
{
    // $NickList nick$$nick2$$|

    if (m_ui32NickListSize < m_ui32NickListLen + szNickLen + 2)
    {
        try
        {
            m_NickList.resize(m_ui32NickListSize + NICKLISTSIZE + 1);
        }
        catch (const std::bad_alloc&)
        {
            LogDbgErr("[MEM] Cannot reallocate {} bytes in Users::AddBot2NickList for m_NickList", m_ui32NickListSize + NICKLISTSIZE + 1);

            return;
        }
        m_ui32NickListSize = static_cast<uint32_t>(m_NickList.size()) - 1;
    }

    memcpy(m_NickList.data() + m_ui32NickListLen - 1, sNick, szNickLen);
    m_ui32NickListLen += static_cast<uint32_t>(szNickLen + 2);

    m_NickList[m_ui32NickListLen - 3] = '$';
    m_NickList[m_ui32NickListLen - 2] = '$';
    m_NickList[m_ui32NickListLen - 1] = '|';
    m_NickList[m_ui32NickListLen] = '\0';

    m_ui32ZNickListLen = 0;

    if (!bIsOp)
    {
        return;
    }

    if (m_ui32OpListSize < m_ui32OpListLen + szNickLen + 2)
    {
        try
        {
            m_OpList.resize(m_ui32OpListSize + OPLISTSIZE + 1);
        }
        catch (const std::bad_alloc&)
        {
            LogDbgErr("[MEM] Cannot reallocate {} bytes in Users::AddBot2NickList for m_OpList", m_ui32OpListSize + OPLISTSIZE + 1);

            return;
        }
        m_ui32OpListSize = static_cast<uint32_t>(m_OpList.size()) - 1;
    }

    memcpy(m_OpList.data() + m_ui32OpListLen - 1, sNick, szNickLen);
    m_ui32OpListLen += static_cast<uint32_t>(szNickLen + 2);

    m_OpList[m_ui32OpListLen - 3] = '$';
    m_OpList[m_ui32OpListLen - 2] = '$';
    m_OpList[m_ui32OpListLen - 1] = '|';
    m_OpList[m_ui32OpListLen] = '\0';

    m_ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void Users::Add2OpList(User* pUser)
{
    if (m_ui32OpListSize < m_ui32OpListLen + pUser->m_sNick.size() + 2)
    {
        try
        {
            m_OpList.resize(m_ui32OpListSize + OPLISTSIZE + 1);
        }
        catch (const std::bad_alloc&)
        {
            pUser->m_ui32BoolBits |= User::BIT_ERROR;

            LogDbgErr("[MEM] Cannot reallocate {} bytes in Users::Add2OpList for m_OpList", m_ui32OpListSize + OPLISTSIZE + 1);

            pUser->Close();
            return;
        }
        m_ui32OpListSize = static_cast<uint32_t>(m_OpList.size()) - 1;
    }

    memcpy(m_OpList.data() + m_ui32OpListLen - 1, pUser->m_sNick.data(), pUser->m_sNick.size());
    m_ui32OpListLen += static_cast<uint32_t>(pUser->m_sNick.size() + 2);

    m_OpList[m_ui32OpListLen - 3] = '$';
    m_OpList[m_ui32OpListLen - 2] = '$';
    m_OpList[m_ui32OpListLen - 1] = '|';
    m_OpList[m_ui32OpListLen] = '\0';

    m_ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void Users::DelFromNickList(const char* sNick, const bool bIsOp)
{
    const int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s$", sNick);
    if (iRet <= 0)
    {
        return;
    }

    m_NickList[9] = '$';
    auto* sFound = static_cast<char*>(memmem(m_NickList.data(), m_ui32NickListLen, ServerManager::m_pGlobalBuffer, static_cast<size_t>(iRet)));
    m_NickList[9] = ' ';

    if (sFound)
    {
        memmove(sFound + 1, sFound + (iRet + 1), m_ui32NickListLen - static_cast<size_t>((sFound + iRet) - m_NickList.data()));
        m_ui32NickListLen -= iRet;
        m_ui32ZNickListLen = 0;
    }

    if (!bIsOp)
        return;

    m_OpList[7] = '$';
    sFound = static_cast<char*>(memmem(m_OpList.data(), m_ui32OpListLen, ServerManager::m_pGlobalBuffer, static_cast<size_t>(iRet)));
    m_OpList[7] = ' ';

    if (sFound)
    {
        memmove(sFound + 1, sFound + (iRet + 1), m_ui32OpListLen - static_cast<size_t>((sFound + iRet) - m_OpList.data()));
        m_ui32OpListLen -= iRet;
        m_ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::DelFromOpList(const char* sNick)
{
    const int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s$", sNick);
    if (iRet <= 0)
    {
        return;
    }

    m_OpList[7] = '$';
    auto* sFound = static_cast<char*>(memmem(m_OpList.data(), m_ui32OpListLen, ServerManager::m_pGlobalBuffer, static_cast<size_t>(iRet)));
    m_OpList[7] = ' ';

    if (sFound)
    {
        memmove(sFound + 1, sFound + (iRet + 1), m_ui32OpListLen - static_cast<size_t>((sFound + iRet) - m_OpList.data()));
        m_ui32OpListLen -= iRet;
        m_ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

// PPK ... check global mainchat flood and add to global queue
void Users::SendChat2All(User* pUser, const char* sData, const size_t szChatLen, GlobalDataQueue::QueueItem* pQueueItem)
{
    LogInfo("[CHAT-BROADCAST] nick='{}' len={} queueItem={} chatlocked={}", pUser->m_sNick, szChatLen, (pQueueItem ? "yes" : "no"), m_bChatLocked);
    UdpDebug::m_Ptr->Broadcast(sData, szChatLen);

    if (!ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::NODEFLOODMAINCHAT) &&
        SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_ACTION)] != 0)
    {
        if (m_ui16ChatMsgs == 0)
        {
            m_ui64ChatMsgsTick = ServerManager::m_ui64ActualTick;
            m_ui64ChatLockFromTick = ServerManager::m_ui64ActualTick;
            m_ui16ChatMsgs = 0;
            m_bChatLocked = false;
        }
        else if ((m_ui64ChatMsgsTick + SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_TIME)]) < ServerManager::m_ui64ActualTick)
        {
            m_ui64ChatMsgsTick = ServerManager::m_ui64ActualTick;
            m_ui16ChatMsgs = 0;
        }

        m_ui16ChatMsgs++;
        GlobalDataQueue::m_Ptr->PrometheusMessagesGauge(1);

        if (m_ui16ChatMsgs > static_cast<uint16_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES)]))
        {
            m_ui64ChatLockFromTick = ServerManager::m_ui64ActualTick;
            if (!m_bChatLocked)
            {
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_DEFLOOD_REPORT)])
                {
                    GlobalDataQueue::m_Ptr->StatusMessageFormat("Users::SendChat2All",
                                                                "<%s> *** %s.|",
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_GLOBAL_CHAT_FLOOD_DETECTED)].c_str());
                }

                m_bChatLocked = true;
            }
        }

        if (m_bChatLocked)
        {
            if ((m_ui64ChatLockFromTick + SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT)]) > ServerManager::m_ui64ActualTick)
            {
                if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_ACTION)] == 1)
                {
                    return;
                }
                if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_ACTION)] == 2)
                {
                    const size_t szNeeded = pUser->m_ui8IpLen + 1 + szChatLen + 1;
                    if (szNeeded > ServerManager::m_szGlobalBufferSize)
                    {
                        return;
                    }
                    memcpy(ServerManager::m_pGlobalBuffer, pUser->m_sIP.data(), pUser->m_ui8IpLen);
                    ServerManager::m_pGlobalBuffer[pUser->m_ui8IpLen] = ' ';
                    size_t szLen = pUser->m_ui8IpLen + 1;
                    memcpy(ServerManager::m_pGlobalBuffer + szLen, sData, szChatLen);
                    szLen += szChatLen;
                    ServerManager::m_pGlobalBuffer[szLen] = '\0';
                    GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, szLen, nullptr, 0, GlobalDataQueue::Cmd::OPS);

                    return;
                }
            }
            else
            {
                m_bChatLocked = false;
            }
        }
    }

    if (!pQueueItem)
    {
        GlobalDataQueue::m_Ptr->AddQueueItem(sData, szChatLen, nullptr, 0, GlobalDataQueue::Cmd::CHAT);
    }
    else
    {
        GlobalDataQueue::m_Ptr->FillBlankQueueItem(sData, szChatLen, pQueueItem);
    }
}
//---------------------------------------------------------------------------

void Users::Add2MyInfos(User* pUser)
{
    if (m_ui32MyInfosSize < m_ui32MyInfosLen + pUser->m_ui16MyInfoShortLen)
    {
        try
        {
            m_MyInfos.resize(m_ui32MyInfosSize + g_ui32MyInfoListSize + 1);
        }
        catch (const std::bad_alloc&)
        {
            pUser->m_ui32BoolBits |= User::BIT_ERROR;

            LogDbgErr("[MEM] Cannot reallocate {} bytes in Users::Add2MyInfos", m_ui32MyInfosSize + g_ui32MyInfoListSize + 1);

            pUser->Close();
            return;
        }
        m_ui32MyInfosSize = static_cast<uint32_t>(m_MyInfos.size()) - 1;
    }

    memcpy(m_MyInfos.data() + m_ui32MyInfosLen, pUser->m_sMyInfoShort.data(), pUser->m_ui16MyInfoShortLen);
    m_ui32MyInfosLen += pUser->m_ui16MyInfoShortLen;

    m_MyInfos[m_ui32MyInfosLen] = '\0';

    m_ui32ZMyInfosLen = 0;
#ifdef USE_FLYLINKDC_EXT_JSON
    Add2ExtJSON(pUser);
#endif
}
//---------------------------------------------------------------------------
#ifdef USE_FLYLINKDC_EXT_JSON
void Users::Add2ExtJSON(const User* pUser)
{
    if (pUser->m_user_ext_info)
    {
        const auto& l_ext_json_info = pUser->m_user_ext_info->GetExtJSONCommand();
        if (!m_AllExtJSON.contains(l_ext_json_info))
        {
            m_AllExtJSON += l_ext_json_info;
        }
    }
}
//---------------------------------------------------------------------------
void Users::DelFromExtJSONInfos(const User* pUser)
{
    if (pUser->m_user_ext_info && !m_AllExtJSON.empty())
    {
        const auto& l_ext_json_info = pUser->m_user_ext_info->GetExtJSONCommand();
        const size_t i = m_AllExtJSON.find(l_ext_json_info);
        if (i != std::string::npos)
        {
            m_AllExtJSON.erase(i, l_ext_json_info.size());
        }
    }
}
#endif
//---------------------------------------------------------------------------

void Users::DelFromMyInfos(User* pUser)
{
#ifdef USE_FLYLINKDC_EXT_JSON
    DelFromExtJSONInfos(pUser);
#endif
    auto* sMatch = static_cast<char*>(memmem(m_MyInfos.data(), m_ui32MyInfosLen, pUser->m_sMyInfoShort.data() + 8, pUser->m_ui16MyInfoShortLen - 8));
    if (sMatch)
    {
        sMatch -= 8;
        memmove(sMatch,
                sMatch + pUser->m_ui16MyInfoShortLen,
                m_ui32MyInfosLen - static_cast<size_t>((sMatch + (pUser->m_ui16MyInfoShortLen - 1)) - m_MyInfos.data()));
        m_ui32MyInfosLen -= pUser->m_ui16MyInfoShortLen;
        m_ui32ZMyInfosLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::Add2MyInfosTag(User* pUser)
{
    if (m_ui32MyInfosTagSize < m_ui32MyInfosTagLen + pUser->m_ui16MyInfoLongLen)
    {
        try
        {
            m_MyInfosTag.resize(m_ui32MyInfosTagSize + g_ui32MyInfoListSize + 1);
        }
        catch (const std::bad_alloc&)
        {
            pUser->m_ui32BoolBits |= User::BIT_ERROR;

            LogDbgErr("[MEM] Cannot reallocate {} bytes in Users::Add2MyInfosTag", m_ui32MyInfosTagSize + g_ui32MyInfoListSize + 1);

            pUser->Close();
            return;
        }
        m_ui32MyInfosTagSize = static_cast<uint32_t>(m_MyInfosTag.size()) - 1;
    }

    memcpy(m_MyInfosTag.data() + m_ui32MyInfosTagLen, pUser->m_sMyInfoLong.data(), pUser->m_ui16MyInfoLongLen);
    m_ui32MyInfosTagLen += pUser->m_ui16MyInfoLongLen;

    m_MyInfosTag[m_ui32MyInfosTagLen] = '\0';

    m_ui32ZMyInfosTagLen = 0;
#ifdef USE_FLYLINKDC_EXT_JSON
    Add2ExtJSON(pUser);
#endif
}
//---------------------------------------------------------------------------

void Users::DelFromMyInfosTag(User* pUser)
{
#ifdef USE_FLYLINKDC_EXT_JSON
    DelFromExtJSONInfos(pUser);
#endif
    auto* sMatch = static_cast<char*>(memmem(m_MyInfosTag.data(), m_ui32MyInfosTagLen, pUser->m_sMyInfoLong.data() + 8, pUser->m_ui16MyInfoLongLen - 8));
    if (sMatch)
    {
        sMatch -= 8;
        memmove(sMatch,
                sMatch + pUser->m_ui16MyInfoLongLen,
                m_ui32MyInfosTagLen - static_cast<size_t>((sMatch + (pUser->m_ui16MyInfoLongLen - 1)) - m_MyInfosTag.data()));
        m_ui32MyInfosTagLen -= pUser->m_ui16MyInfoLongLen;
        m_ui32ZMyInfosTagLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::AddBot2MyInfos(const char* sMyInfo)
{
    const size_t szLen = strlen(sMyInfo);
    if (!m_MyInfosTag.empty())
    {
        if (!memmem(m_MyInfosTag.data(), m_ui32MyInfosTagLen, sMyInfo, szLen))
        {
            if (m_ui32MyInfosTagSize < m_ui32MyInfosTagLen + szLen)
            {
                try
                {
                    m_MyInfosTag.resize(m_ui32MyInfosTagSize + g_ui32MyInfoListSize + 1);
                }
                catch (const std::bad_alloc&)
                {
                    LogDbgErr("[MEM] Cannot reallocate {} bytes for m_MyInfosTag in Users::AddBot2MyInfos", m_ui32MyInfosTagSize + g_ui32MyInfoListSize + 1);

                    return;
                }
                m_ui32MyInfosTagSize = static_cast<uint32_t>(m_MyInfosTag.size()) - 1;
            }
            memcpy(m_MyInfosTag.data() + m_ui32MyInfosTagLen, sMyInfo, szLen);
            m_ui32MyInfosTagLen += static_cast<uint32_t>(szLen);
            m_MyInfosTag[m_ui32MyInfosTagLen] = '\0';
            m_ui32ZMyInfosLen = 0;
        }
    }

    if (!m_MyInfos.empty())
    {
        if (!memmem(m_MyInfos.data(), m_ui32MyInfosLen, sMyInfo, szLen))
        {
            if (m_ui32MyInfosSize < m_ui32MyInfosLen + szLen)
            {
                try
                {
                    m_MyInfos.resize(m_ui32MyInfosSize + g_ui32MyInfoListSize + 1);
                }
                catch (const std::bad_alloc&)
                {
                    LogDbgErr("[MEM] Cannot reallocate {} bytes for m_MyInfos in Users::AddBot2MyInfos", m_ui32MyInfosSize + g_ui32MyInfoListSize + 1);

                    return;
                }
                m_ui32MyInfosSize = static_cast<uint32_t>(m_MyInfos.size()) - 1;
            }
            memcpy(m_MyInfos.data() + m_ui32MyInfosLen, sMyInfo, szLen);
            m_ui32MyInfosLen += static_cast<uint32_t>(szLen);
            m_MyInfos[m_ui32MyInfosLen] = '\0';
            m_ui32ZMyInfosTagLen = 0;
        }
    }
}
//---------------------------------------------------------------------------

void Users::DelBotFromMyInfos(const char* sMyInfo)
{
    const size_t szLen = strlen(sMyInfo);
    if (!m_MyInfosTag.empty())
    {
        auto* sMatch = static_cast<char*>(memmem(m_MyInfosTag.data(), m_ui32MyInfosTagLen, sMyInfo, szLen));
        if (sMatch)
        {
            memmove(sMatch, sMatch + szLen, m_ui32MyInfosTagLen - static_cast<size_t>((sMatch + (szLen - 1)) - m_MyInfosTag.data()));
            m_ui32MyInfosTagLen -= static_cast<uint32_t>(szLen);
            m_ui32ZMyInfosTagLen = 0;
        }
    }

    if (!m_MyInfos.empty())
    {
        auto* sMatch = static_cast<char*>(memmem(m_MyInfos.data(), m_ui32MyInfosLen, sMyInfo, szLen));
        if (sMatch)
        {
            memmove(sMatch, sMatch + szLen, m_ui32MyInfosLen - static_cast<size_t>((sMatch + (szLen - 1)) - m_MyInfos.data()));
            m_ui32MyInfosLen -= static_cast<uint32_t>(szLen);
            m_ui32ZMyInfosLen = 0;
        }
    }
}
//---------------------------------------------------------------------------

void Users::Add2UserIP(User* pUser)
{
    const int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s %s$", pUser->m_sNick.c_str(), pUser->m_sIP.data());
    if (iRet <= 0)
    {
        return;
    }

    if (m_ui32UserIPListSize < m_ui32UserIPListLen + iRet)
    {
        try
        {
            m_UserIPList.resize(m_ui32UserIPListSize + g_ui32IpListSize + 1);
        }
        catch (const std::bad_alloc&)
        {
            pUser->m_ui32BoolBits |= User::BIT_ERROR;

            LogDbgErr("[MEM] Cannot reallocate {} bytes in Users::Add2UserIP", m_ui32UserIPListSize + g_ui32IpListSize + 1);

            pUser->Close();
            return;
        }
        m_ui32UserIPListSize = static_cast<uint32_t>(m_UserIPList.size()) - 1;
    }

    memcpy(m_UserIPList.data() + m_ui32UserIPListLen - 1, ServerManager::m_pGlobalBuffer + 1, iRet - 1);
    m_ui32UserIPListLen += iRet;

    m_UserIPList[m_ui32UserIPListLen - 2] = '$';
    m_UserIPList[m_ui32UserIPListLen - 1] = '|';
    m_UserIPList[m_ui32UserIPListLen] = '\0';

    m_ui32ZUserIPListLen = 0;
}
//---------------------------------------------------------------------------

void Users::DelFromUserIP(User* pUser)
{
    const int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s %s$", pUser->m_sNick.c_str(), pUser->m_sIP.data());
    if (iRet <= 0)
    {
        return;
    }

    m_UserIPList[7] = '$';
    auto* sFound = static_cast<char*>(memmem(m_UserIPList.data(), m_ui32UserIPListLen, ServerManager::m_pGlobalBuffer, static_cast<size_t>(iRet)));
    m_UserIPList[7] = ' ';

    if (sFound)
    {
        memmove(sFound + 1, sFound + (iRet + 1), m_ui32UserIPListLen - static_cast<size_t>((sFound + iRet) - m_UserIPList.data()));
        m_ui32UserIPListLen -= iRet;
        m_ui32ZUserIPListLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::Add2RecTimes(User* pUser)
{
    time_t tmAccTime;
    time(&tmAccTime);

    if (ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::NOUSRSAMEIP) ||
        (tmAccTime - pUser->m_tLoginTime) >= SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_RECONN_TIME)])
    {
        return;
    }

    auto pNewRecTime = std::make_unique<RecTime>(pUser->m_ui128IpHash.data());

    pNewRecTime->m_sNick = pUser->m_sNick;
    pNewRecTime->m_ui64DisConnTick = ServerManager::m_ui64ActualTick - (tmAccTime - pUser->m_tLoginTime);
    pNewRecTime->m_ui32NickHash = pUser->m_ui32NickHash;

    m_RecTimeList.push_front(std::move(pNewRecTime));
}
//---------------------------------------------------------------------------

bool Users::CheckRecTime(User* pUser)
{
    auto it = m_RecTimeList.begin();
    while (it != m_RecTimeList.end())
    {
        RecTime* pCur = it->get();

        // check expires...
        if (pCur->m_ui64DisConnTick + SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_RECONN_TIME)] <= ServerManager::m_ui64ActualTick)
        {
            it = m_RecTimeList.erase(it);
            continue;
        }

        if (pCur->m_ui32NickHash == pUser->m_ui32NickHash && pCur->m_ui128IpHash == pUser->m_ui128IpHash &&
            iequals(pCur->m_sNick, pUser->m_sNick))
        {
            pUser->SendFormat("Users::CheckRecTime",
                              false,
                              "<%s> %s %" PRIu64 " %s.|",
                              SettingManager::HubSec(),
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PLEASE_WAIT)].c_str(),
                              (pCur->m_ui64DisConnTick + SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_RECONN_TIME)]) - ServerManager::m_ui64ActualTick,
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SECONDS_BEFORE_RECONN)].c_str());

            return true;
        }

        ++it;
    }

    return false;
}
//---------------------------------------------------------------------------
