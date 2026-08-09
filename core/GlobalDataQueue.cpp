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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GlobalDataQueue.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "colUsers.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"

// Align size up to next power-of-2-ish boundary (legacy behavior: n+1)
namespace {
inline size_t AlignUp(size_t n)
{
    return n + 1;
}
} // namespace

static constexpr size_t g_ui32InitialQueueBufferSize = 256;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::unique_ptr<GlobalDataQueue> GlobalDataQueue::m_Ptr;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GlobalDataQueue::GlobalDataQueue() // NOLINT(modernize-use-equals-default) real init: resize, metrics pre-registration
{
    // OpList buffer
    m_OpListQueue.m_pBuffer.resize(g_ui32InitialQueueBufferSize, 0);
    m_OpListQueue.m_szLen = 0;

    // UserIP buffer
    m_UserIPQueue.m_pBuffer.resize(g_ui32InitialQueueBufferSize, 0);
    m_UserIPQueue.m_szLen = 0;
    m_UserIPQueue.m_bHaveDollars = false;

    for (auto& queue : m_GlobalQueues)
    {
        queue.m_szLen = 0;
        queue.m_szZlen = 0;
        queue.m_pBuffer.resize(g_ui32InitialQueueBufferSize, 0);
        queue.m_Zbuffer.resize(g_ui32InitialQueueBufferSize, 0);

        queue.m_bCreated = false;
        queue.m_bZlined = false;
    }

    // Pre-register all metrics so they appear in /metrics even before first use
    m_metrics.counter_inc("flylinkdc_hub_recv_bytes_total", 0, {{"type", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_recv_packets_total", 0, {{"type", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_send_bytes_total", 0, {{"type", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_send_packets_total", 0, {{"type", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_dc_commands_total", 0, {{"command", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_compress_bytes_total", 0, {{"type", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_log_bytes_total", 0, {{"type", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_lua_calls_total", 0, {{"func", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_lua_user_value_total", 0, {{"value", "init"}});
    m_metrics.counter_inc("flylinkdc_hub_lua_userdata_total", 0, {{"value", "init"}});
    m_metrics.gauge_set("flylinkdc_hub_users_online", 0);
    m_metrics.gauge_set("flylinkdc_hub_messages_queue", 0);
    m_metrics.gauge_set("flylinkdc_hub_rusage", 0, {{"resource", "init"}});
    m_metrics.gauge_set("flylinkdc_hub_sqlite_size_bytes", 0);
    m_metrics.gauge_set("flylinkdc_hub_users_logged_in", 0);
    m_metrics.gauge_set("flylinkdc_hub_joins_total", 0);
    m_metrics.gauge_set("flylinkdc_hub_parts_total", 0);
    m_metrics.gauge_set("flylinkdc_hub_users_peak", 0);
    m_metrics.gauge_set("flylinkdc_hub_share_bytes", 0);
    m_metrics.gauge_set("flylinkdc_hub_start_time_seconds", 0);
    m_metrics.gauge_set("flylinkdc_hub_bandwidth_bytes_per_sec", 0, {{"direction", "init"}});
    m_metrics.gauge_set("flylinkdc_hub_operators_online", 0);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GlobalDataQueue::~GlobalDataQueue() = default;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::AddQueueItem(const char* sCommand1, const size_t szLen1, const char* sCommand2, const size_t szLen2, Cmd eCmdType)
{
    auto pNewItem = CreateQueueItem(sCommand1, szLen1, sCommand2, szLen2, eCmdType);
    if (!pNewItem)
    {
        LogDbg("[MEM] Cannot allocate pNewItem in GlobalDataQueue::AddQueueItem");
        return;
    }

    pNewItem->m_eCmdType = eCmdType;

    m_NewQueueItems.push_back(std::move(pNewItem));
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// appends data to the m_OpListQueue
void GlobalDataQueue::OpListStore(const char* sNick)
{
    if (m_OpListQueue.m_szLen == 0)
    {
        const int iLen = snprintf(m_OpListQueue.m_pBuffer.data(), m_OpListQueue.m_pBuffer.capacity(), "$OpList %s$$|", sNick);
        if (iLen <= 0)
        {
            m_OpListQueue.m_szLen = 0;
        }
        else
        {
            m_OpListQueue.m_szLen = iLen;
        }
    }
    else
    {
        const size_t szNickLen = strlen(sNick) + 3;
        if (m_OpListQueue.m_pBuffer.capacity() < m_OpListQueue.m_szLen + szNickLen + 1)
        {
            const size_t szAllignLen = AlignUp(m_OpListQueue.m_szLen + szNickLen);
            try
            {
                m_OpListQueue.m_pBuffer.resize(szAllignLen);
            }
            catch (std::bad_alloc&)
            {
                LogDbgErr("[MEM] Cannot allocate {} bytes in GlobalDataQueue::OpListStore", szAllignLen);
                return;
            }
        }

        const int iDataLen = snprintf(
            m_OpListQueue.m_pBuffer.data() + m_OpListQueue.m_szLen - 1, m_OpListQueue.m_pBuffer.capacity() - (m_OpListQueue.m_szLen - 1), "%s$$|", sNick);
        if (iDataLen <= 0)
        {
            m_OpListQueue.m_pBuffer[m_OpListQueue.m_szLen - 1] = '|';
            m_OpListQueue.m_pBuffer[m_OpListQueue.m_szLen] = '\0';
        }
        else
        {
            m_OpListQueue.m_szLen += iDataLen - 1;
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// appends data to the UserIPQueue
void GlobalDataQueue::UserIPStore(User* pUser)
{
    if (m_UserIPQueue.m_szLen == 0)
    {
        m_UserIPQueue.m_szLen =
            snprintf(m_UserIPQueue.m_pBuffer.data(), m_UserIPQueue.m_pBuffer.capacity(), "$UserIP %s %s|", pUser->m_sNick.c_str(), pUser->m_sIP.data());
        if (m_UserIPQueue.m_szLen > 0)
        {
            m_UserIPQueue.m_bHaveDollars = false;
        }
        else
        {
            m_UserIPQueue.m_szLen = 0;
        }
    }
    else
    {
        const size_t szLen = pUser->m_sNick.size() + pUser->m_ui8IpLen + 4;
        if (m_UserIPQueue.m_pBuffer.capacity() < m_UserIPQueue.m_szLen + szLen + 1)
        {
            const size_t szAllignLen = AlignUp(m_UserIPQueue.m_szLen + szLen);
            try
            {
                m_UserIPQueue.m_pBuffer.resize(szAllignLen);
            }
            catch (std::bad_alloc&)
            {
                LogDbgErr("[MEM] Cannot allocate {} bytes in GlobalDataQueue::UserIPStore", szAllignLen);
                return;
            }
        }

        if (!m_UserIPQueue.m_bHaveDollars)
        {
            m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen - 1] = '$';
            m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen] = '$';
            m_UserIPQueue.m_bHaveDollars = true;
            m_UserIPQueue.m_szLen += 2;
        }

        const int iDataLen = snprintf(m_UserIPQueue.m_pBuffer.data() + m_UserIPQueue.m_szLen - 1,
                                m_UserIPQueue.m_pBuffer.capacity() - (m_UserIPQueue.m_szLen - 1),
                                "%s %s$$|",
                                pUser->m_sNick.c_str(),
                                pUser->m_sIP.data());
        if (iDataLen <= 0)
        {
            m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen - 1] = '|';
            m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen] = '\0';
        }
        else
        {
            m_UserIPQueue.m_szLen += iDataLen - 1;
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::PrepareQueueItems()
{
    m_QueueItems = std::move(m_NewQueueItems);
    m_NewQueueItems.clear();

    if (!m_QueueItems.empty())
    {
        LogInfo("[QUEUE] PrepareQueueItems: {} items, first type={}", m_QueueItems.size(), std::to_underlying(m_QueueItems.front()->m_eCmdType));
    }

    if (!m_QueueItems.empty() || m_OpListQueue.m_szLen != 0 || m_UserIPQueue.m_szLen != 0)
    {
        m_bHaveItems = true;
    }

    m_SingleItems = std::move(m_NewSingleItems);
    m_NewSingleItems.clear();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::ClearQueues()
{
    m_bHaveItems = false;

    for (GlobalQueue* pCur : m_CreatedGlobalQueues)
    {
        pCur->m_szLen = 0;
        pCur->clean(); // [+]FlylilinkDC++
        pCur->m_szZlen = 0;
        pCur->m_bCreated = false;
        pCur->m_bZlined = false;
    }

    m_CreatedGlobalQueues.clear();

    m_OpListQueue.m_pBuffer[0] = '\0';
    m_OpListQueue.m_szLen = 0;

    m_UserIPQueue.m_pBuffer[0] = '\0';
    m_UserIPQueue.m_szLen = 0;

    m_QueueItems.clear();

    m_SingleItems.clear();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GlobalDataQueue::AddSearchDataToQueue(const User* pUser, uint32_t ui32QueueType, const QueueItem* pCur) // FlylinkDC++
{
    if (pUser->m_ui64SharedSize != 0)
    {
        AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::ProcessQueues(User* pUser)
{
    uint32_t ui32QueueType = 0; // short myinfos
    uint16_t ui16QueueBits = 0;

    if (SettingManager::m_Ptr->m_ui8FullMyINFOOption == 1 && ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::SENDFULLMYINFOS))
    {
        ui32QueueType = 1; // full myinfos
        ui16QueueBits |= BIT_LONG_MYINFO;
    }

    if (pUser->m_ui64SharedSize != 0)
    {
        if (((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6))
        {
            if (((pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4))
            {
                if (((pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE))
                {
                    if (((pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE))
                    {
                        ui32QueueType += 6; // all searches both ipv4 and ipv6
                        ui16QueueBits |= BIT_ALL_SEARCHES_IPV64;
                    }
                    else
                    {
                        ui32QueueType += 14; // all searches ipv6 + active searches ipv4
                        ui16QueueBits |= BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4;
                    }
                }
                else
                {
                    if (((pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE))
                    {
                        ui32QueueType += 16; // active searches ipv6 + all searches ipv4
                        ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4;
                    }
                    else
                    {
                        ui32QueueType += 12; // active searches both ipv4 and ipv6
                        ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV64;
                    }
                }
            }
            else
            {
                if (((pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE))
                {
                    ui32QueueType += 4; // all searches ipv6 only
                    ui16QueueBits |= BIT_ALL_SEARCHES_IPV6;
                }
                else
                {
                    ui32QueueType += 10; // active searches ipv6 only
                    ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV6;
                }
            }
        }
        else
        {
            if (((pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE))
            {
                ui32QueueType += 2; // all searches ipv4 only
                ui16QueueBits |= BIT_ALL_SEARCHES_IPV4;
            }
            else
            {
                ui32QueueType += 8; // active searches ipv4 only
                ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV4;
            }
        }
    }

    if (!((pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO))
    {
        ui32QueueType += 14; // send hellos
        ui16QueueBits |= BIT_HELLO;
    }

    if (((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
    {
        ui32QueueType += 28; // send operator data
        ui16QueueBits |= BIT_OPERATOR;
    }

    if (pUser->m_i32Profile != -1 && ((pUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) &&
        ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::SENDALLUSERIP))
    {
        ui32QueueType += 56; // send userips
        ui16QueueBits |= BIT_USERIP;
    }

    if (!m_GlobalQueues[ui32QueueType].m_bCreated)
    {
        if (!m_QueueItems.empty())
        {
            for (const auto& pItem : m_QueueItems)
            {
                QueueItem* pCur = pItem.get();

                switch (pCur->m_eCmdType)
                {
                case Cmd::HELLO:
                    if ((ui16QueueBits & BIT_HELLO) == BIT_HELLO)
                    {
                        AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
                    }
                    break;
#ifdef USE_FLYLINKDC_EXT_JSON
                case Cmd::EXTJSON:
                    if (pCur->m_ui32CmdLen[0] != 0)
                    {
                        if (pUser->isSupportExtJSON())
                        {
                            AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
                        }
                        else
                        {
                            LogDbg("[EXTJSON] Skipped for non-ExtJSON user='{}'", pUser->m_sNick);
                        }
                    }
                    break;
#endif
                case Cmd::MYINFO:
                    if ((ui16QueueBits & BIT_LONG_MYINFO) == BIT_LONG_MYINFO)
                    {
                        if (pCur->m_ui32CmdLen[1] != 0)
                        {
                            LogDbg("[MYINFO] send user='{}' size={} head='{}'",
                                   pUser->m_sNick,
                                   pCur->m_ui32CmdLen[1],
                                   std::string(pCur->m_pCommand[1], std::min<uint32_t>(pCur->m_ui32CmdLen[1], 40)));
                        }
                        AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[1], pCur->m_ui32CmdLen[1]);
                        break;
                    }

                    if (pCur->m_ui32CmdLen[0] != 0)
                    {
                        LogDbg("[MYINFO] send user='{}' size={} head='{}'",
                               pUser->m_sNick,
                               pCur->m_ui32CmdLen[0],
                               std::string(pCur->m_pCommand[0], std::min<uint32_t>(pCur->m_ui32CmdLen[0], 40)));
                    }
                    AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);

                    break;
                case Cmd::OPS:
                    if ((ui16QueueBits & BIT_OPERATOR) == BIT_OPERATOR)
                    {
#ifdef USE_FLYLINKDC_EXT_JSON
                        // ExtJSON data queued as Cmd::OPS (rate-limit fallback) must only go to ExtJSON-aware operators
                        if (MatchBytes(pCur->m_pCommand[0], "$ExtJSON "))
                        {
                            if (pUser->isSupportExtJSON())
                            {
                                AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
                            }
                            else
                            {
                                LogDbg("[EXTJSON] Skipped for non-ExtJSON operator user='{}'", pUser->m_sNick);
                            }
                        }
                        else
#endif
                        {
                            AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
                        }
                    }
                    break;
                case Cmd::ACTIVE_SEARCH_V6:
                    if (pUser->m_ui64SharedSize != 0 && !((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4) &&
                        !((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV4) == BIT_ACTIVE_SEARCHES_IPV4))
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::ACTIVE_SEARCH_V64:
                    if (pUser->m_ui64SharedSize != 0 && !((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4) &&
                        !((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV4) == BIT_ACTIVE_SEARCHES_IPV4))
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    else if (!((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6) &&
                             !((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6) == BIT_ACTIVE_SEARCHES_IPV6))
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::ACTIVE_SEARCH_V4:
                    if (pUser->m_ui64SharedSize != 0 && !((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6) &&
                        !((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6) == BIT_ACTIVE_SEARCHES_IPV6))
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::PASSIVE_SEARCH_V6:
                    if (pUser->m_ui64SharedSize != 0 && ((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6 ||
                                                         (ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 ||
                                                         (ui16QueueBits & BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) == BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4))
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::PASSIVE_SEARCH_V64:
                    if (pUser->m_ui64SharedSize != 0 &&
                        ((ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 ||
                         (ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4 ||
                         (ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4) == BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4 ||
                         (ui16QueueBits & BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) == BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4))
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::PASSIVE_SEARCH_V4:
                    if (pUser->m_ui64SharedSize != 0 && ((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4 ||
                                                         (ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 ||
                                                         (ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4) == BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4))
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::PASSIVE_SEARCH_V4_ONLY:
                    if (pUser->m_ui64SharedSize != 0 && (ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4)
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::PASSIVE_SEARCH_V6_ONLY:
                    if (pUser->m_ui64SharedSize != 0 && (ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6)
                    {
                        AddSearchDataToQueue(pUser, ui32QueueType, pCur); // [+]FlylinkDC++
                    }
                    break;
                case Cmd::CHAT:
                    LogDbg("[QUEUE-BUILD] Cmd::CHAT len={} queueType={} user='{}'", pCur->m_ui32CmdLen[0], ui32QueueType, pUser->m_sNick);
                    AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
                    break;
                case Cmd::HUBNAME:
                case Cmd::QUIT:
                case Cmd::LUA:
                    AddDataToQueueStr(m_GlobalQueues[ui32QueueType], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
                    break;
                default:
                    break; // should not happen ;)
                }
            }
        }

        if (m_OpListQueue.m_szLen != 0)
        {
            AddDataToQueue(m_GlobalQueues[ui32QueueType], m_OpListQueue.m_pBuffer.data(), m_OpListQueue.m_szLen);
        }

        if (m_UserIPQueue.m_szLen != 0 && (ui16QueueBits & BIT_USERIP) == BIT_USERIP)
        {
            AddDataToQueue(m_GlobalQueues[ui32QueueType], m_UserIPQueue.m_pBuffer.data(), m_UserIPQueue.m_szLen);
        }

        m_GlobalQueues[ui32QueueType].m_bCreated = true;
        m_CreatedGlobalQueues.push_front(&m_GlobalQueues[ui32QueueType]);
    }

    if (m_GlobalQueues[ui32QueueType].m_szLen == 0)
    {
        if (ServerManager::m_ui8SrCntr == 0)
        {
            User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
            User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
        }

        return;
    }

    if (ServerManager::m_ui8SrCntr == 0)
    {
        if (pUser->m_ui64SharedSize == 0)
        {
            if (pUser->m_pCmdActive6Search)
            {
                User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
                pUser->m_pCmdActive6Search = nullptr;
            }

            if (pUser->m_pCmdActive4Search)
            {
                User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
                pUser->m_pCmdActive4Search = nullptr;
            }
        }
        else
        {
            if ((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
            {
                if (((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE))
                {
                    if (!m_GlobalQueues[ui32QueueType].m_bZlined)
                    {
                        m_GlobalQueues[ui32QueueType].m_bZlined = true;
                        ZlibUtility::m_Ptr->CreateZPipe(std::string_view(m_GlobalQueues[ui32QueueType].m_pBuffer.data(), m_GlobalQueues[ui32QueueType].m_szLen),
                                                        m_GlobalQueues[ui32QueueType].m_Zbuffer,
                                                        m_GlobalQueues[ui32QueueType].m_szZlen);
                    }

                    size_t szSearchLens = 0;
                    if (pUser->m_pCmdActive6Search)
                    {
                        szSearchLens += pUser->m_pCmdActive6Search->m_ui32Len;
                    }
                    if (pUser->m_pCmdActive4Search)
                    {
                        szSearchLens += pUser->m_pCmdActive4Search->m_ui32Len;
                    }

                    if (m_GlobalQueues[ui32QueueType].m_szZlen != 0 &&
                        (m_GlobalQueues[ui32QueueType].m_szZlen <= (m_GlobalQueues[ui32QueueType].m_szLen - szSearchLens)))
                    {
                        (void)pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_Zbuffer.data(), m_GlobalQueues[ui32QueueType].m_szZlen);
                        ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[ui32QueueType].m_szLen - m_GlobalQueues[ui32QueueType].m_szZlen);

                        if (pUser->m_pCmdActive6Search)
                        {
                            User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
                            pUser->m_pCmdActive6Search = nullptr;
                        }

                        if (pUser->m_pCmdActive4Search)
                        {
                            User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
                            pUser->m_pCmdActive4Search = nullptr;
                        }

                        return;
                    }
                }

                uint32_t ui32SbLen = pUser->m_ui32SendBufDataLen;
                (void)pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pBuffer.data(), m_GlobalQueues[ui32QueueType].m_szLen);

                // PPK ... check if adding of searchs not cause buffer overflow !
                if (pUser->m_ui32SendBufDataLen <= ui32SbLen)
                {
                    ui32SbLen = 0;
                }

                if (pUser->m_pCmdActive6Search)
                {
                    pUser->RemFromSendBuf(pUser->m_pCmdActive6Search->m_sCommand.data(), pUser->m_pCmdActive6Search->m_ui32Len, ui32SbLen);

                    User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
                    pUser->m_pCmdActive6Search = nullptr;
                }

                if (pUser->m_pCmdActive4Search)
                {
                    pUser->RemFromSendBuf(pUser->m_pCmdActive4Search->m_sCommand.data(), pUser->m_pCmdActive4Search->m_ui32Len, ui32SbLen);

                    User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
                    pUser->m_pCmdActive4Search = nullptr;
                }

                return;
            }

            if (pUser->m_pCmdActive4Search)
            {
                if (((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE))
                {
                    if (!m_GlobalQueues[ui32QueueType].m_bZlined)
                    {
                        m_GlobalQueues[ui32QueueType].m_bZlined = true;
                        ZlibUtility::m_Ptr->CreateZPipe(std::string_view(m_GlobalQueues[ui32QueueType].m_pBuffer.data(), m_GlobalQueues[ui32QueueType].m_szLen),
                                                        m_GlobalQueues[ui32QueueType].m_Zbuffer,
                                                        m_GlobalQueues[ui32QueueType].m_szZlen);
                    }

                    if (m_GlobalQueues[ui32QueueType].m_szZlen != 0 &&
                        (m_GlobalQueues[ui32QueueType].m_szZlen <= (m_GlobalQueues[ui32QueueType].m_szLen - pUser->m_pCmdActive4Search->m_ui32Len)))
                    {
                        (void)pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_Zbuffer.data(), m_GlobalQueues[ui32QueueType].m_szZlen);
                        ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[ui32QueueType].m_szLen - m_GlobalQueues[ui32QueueType].m_szZlen);

                        User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
                        pUser->m_pCmdActive4Search = nullptr;

                        return;
                    }
                }

                uint32_t ui32SbLen = pUser->m_ui32SendBufDataLen;
                (void)pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pBuffer.data(), m_GlobalQueues[ui32QueueType].m_szLen);

                // PPK ... check if adding of searchs not cause buffer overflow !
                if (pUser->m_ui32SendBufDataLen <= ui32SbLen)
                {
                    ui32SbLen = 0;
                }

                pUser->RemFromSendBuf(pUser->m_pCmdActive4Search->m_sCommand.data(), pUser->m_pCmdActive4Search->m_ui32Len, ui32SbLen);

                User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
                pUser->m_pCmdActive4Search = nullptr;

                return;
            }
        }
    }

    if (((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE))
    {
        if (!m_GlobalQueues[ui32QueueType].m_bZlined)
        {
            m_GlobalQueues[ui32QueueType].m_bZlined = true;
            ZlibUtility::m_Ptr->CreateZPipe(std::string_view(m_GlobalQueues[ui32QueueType].m_pBuffer.data(), m_GlobalQueues[ui32QueueType].m_szLen),
                                            m_GlobalQueues[ui32QueueType].m_Zbuffer,
                                            m_GlobalQueues[ui32QueueType].m_szZlen);
        }

        if (m_GlobalQueues[ui32QueueType].m_szZlen != 0)
        {
            (void)pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_Zbuffer.data(), m_GlobalQueues[ui32QueueType].m_szZlen);
            ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[ui32QueueType].m_szLen - m_GlobalQueues[ui32QueueType].m_szZlen);

            return;
        }
    }
    // LogDbg("[QUEUE-SEND] nick='{}' queueType={} rawLen={} zlen={}", pUser->m_sNick, ui32QueueType, m_GlobalQueues[ui32QueueType].m_szLen,
    // m_GlobalQueues[ui32QueueType].m_szZlen);
    (void)pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pBuffer.data(), m_GlobalQueues[ui32QueueType].m_szLen);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::ProcessSingleItems(User* pUser) const
{
    size_t szLen = 0, szWanted = 0;
    int iRet = 0;

    for (const auto& pItem : m_SingleItems)
    {
        SingleDataItem* pCur = pItem.get();

        if (pCur->m_pFromUser != pUser)
        {
            switch (pCur->m_eType)
            {
            case SendItem::PM2ALL: // send PM to ALL
            {
                szWanted = szLen + pCur->m_szDataLen + pUser->m_sNick.size() + 13;
                if (ServerManager::m_szGlobalBufferSize < szWanted)
                {
                    if (!CheckAndResizeGlobalBuffer(szWanted))
                    {
                        LogDbgErr("[MEM] Cannot reallocate {} bytes in GlobalDataQueue::ProcessSingleItems", AlignUp(szWanted));
                        break;
                    }
                }
                iRet = snprintf(ServerManager::m_pGlobalBuffer + szLen, ServerManager::m_szGlobalBufferSize - szLen, "$To: %s From: ", pUser->m_sNick.c_str());
                if (iRet > 0)
                {
                    szLen += iRet;

                    memcpy(ServerManager::m_pGlobalBuffer + szLen, pCur->m_Data.data(), pCur->m_szDataLen);
                    szLen += pCur->m_szDataLen;
                    ServerManager::m_pGlobalBuffer[szLen] = '\0';
                }

                break;
            }
            case SendItem::PM2OPS: // send PM only to operators
            {
                if (((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
                {
                    szWanted = szLen + pCur->m_szDataLen + pUser->m_sNick.size() + 13;
                    if (ServerManager::m_szGlobalBufferSize < szWanted)
                    {
                        if (!CheckAndResizeGlobalBuffer(szWanted))
                        {
                            LogDbgErr("[MEM] Cannot reallocate {} bytes in GlobalDataQueue::ProcessSingleItems1", AlignUp(szWanted));
                            break;
                        }
                    }
                    iRet =
                        snprintf(ServerManager::m_pGlobalBuffer + szLen, ServerManager::m_szGlobalBufferSize - szLen, "$To: %s From: ", pUser->m_sNick.c_str());
                    if (iRet > 0)
                    {
                        szLen += iRet;

                        memcpy(ServerManager::m_pGlobalBuffer + szLen, pCur->m_Data.data(), pCur->m_szDataLen);
                        szLen += pCur->m_szDataLen;
                        ServerManager::m_pGlobalBuffer[szLen] = '\0';
                    }
                }
                break;
            }
            case SendItem::OPCHAT: // send OpChat only to allowed users...
            {
                if (ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::ALLOWEDOPCHAT))
                {
                    szWanted = szLen + pCur->m_szDataLen + pUser->m_sNick.size() + 13;
                    if (ServerManager::m_szGlobalBufferSize < szWanted)
                    {
                        if (!CheckAndResizeGlobalBuffer(szWanted))
                        {
                            LogDbgErr("[MEM] Cannot reallocate {} bytes in GlobalDataQueue::ProcessSingleItems2", AlignUp(szWanted));
                            break;
                        }
                    }
                    iRet =
                        snprintf(ServerManager::m_pGlobalBuffer + szLen, ServerManager::m_szGlobalBufferSize - szLen, "$To: %s From: ", pUser->m_sNick.c_str());
                    if (iRet > 0)
                    {
                        szLen += iRet;

                        memcpy(ServerManager::m_pGlobalBuffer + szLen, pCur->m_Data.data(), pCur->m_szDataLen);
                        szLen += pCur->m_szDataLen;
                        ServerManager::m_pGlobalBuffer[szLen] = '\0';
                    }
                }
                break;
            }
            case SendItem::TOPROFILE: // send data only to given profile...
            {
                if (pUser->m_i32Profile == pCur->m_i32Profile)
                {
                    szWanted = szLen + pCur->m_szDataLen;
                    if (ServerManager::m_szGlobalBufferSize < szWanted)
                    {
                        if (!CheckAndResizeGlobalBuffer(szWanted))
                        {
                            LogDbgErr("[MEM] Cannot reallocate {} bytes in GlobalDataQueue::ProcessSingleItems3", AlignUp(szWanted));
                            break;
                        }
                    }
                    memcpy(ServerManager::m_pGlobalBuffer + szLen, pCur->m_Data.data(), pCur->m_szDataLen);
                    szLen += pCur->m_szDataLen;
                    ServerManager::m_pGlobalBuffer[szLen] = '\0';
                }
                break;
            }
            case SendItem::PM2PROFILE: // send pm only to given profile...
            {
                if (pUser->m_i32Profile == pCur->m_i32Profile)
                {
                    szWanted = szLen + pCur->m_szDataLen + pUser->m_sNick.size() + 13;
                    if (ServerManager::m_szGlobalBufferSize < szWanted)
                    {
                        if (!CheckAndResizeGlobalBuffer(szWanted))
                        {
                            LogDbgErr("[MEM] Cannot reallocate {} bytes in GlobalDataQueue::ProcessSingleItems4", AlignUp(szWanted));
                            break;
                        }
                    }
                    iRet =
                        snprintf(ServerManager::m_pGlobalBuffer + szLen, ServerManager::m_szGlobalBufferSize - szLen, "$To: %s From: ", pUser->m_sNick.c_str());
                    if (iRet > 0)
                    {
                        szLen += iRet;

                        memcpy(ServerManager::m_pGlobalBuffer + szLen, pCur->m_Data.data(), pCur->m_szDataLen);
                        szLen += pCur->m_szDataLen;
                        ServerManager::m_pGlobalBuffer[szLen] = '\0';
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }

    if (szLen != 0)
    {
        pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, szLen);
    }

    ReduceGlobalBuffer();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::SingleItemStore(const char* sData, const size_t szDataLen, User* pFromUser, const int32_t i32Profile, SendItem eType)
{
    auto pNewItem = std::make_unique<SingleDataItem>();

    if (sData)
    {
        try
        {
            pNewItem->m_Data.resize(szDataLen + 1);
        }
        catch (std::bad_alloc&)
        {
            LogDbgErr("[MEM] Cannot allocate {} bytes in GlobalDataQueue::SingleItemStore", szDataLen + 1);

            return;
        }

        memcpy(pNewItem->m_Data.data(), sData, szDataLen);
        pNewItem->m_Data[szDataLen] = '\0';
    }
    else
    {
        pNewItem->m_Data.clear();
    }

    pNewItem->m_szDataLen = szDataLen;

    pNewItem->m_pFromUser = pFromUser;

    pNewItem->m_eType = eType;

    pNewItem->m_i32Profile = i32Profile;

    m_NewSingleItems.push_back(std::move(pNewItem));
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::SendFinalQueue()
{
    for (const auto& pItem : m_QueueItems)
    {
        QueueItem* pCur = pItem.get();

        switch (pCur->m_eCmdType)
        {
        case Cmd::OPS:
        case Cmd::CHAT:
        case Cmd::LUA:
#ifdef USE_FLYLINKDC_EXT_JSON
            // Skip ExtJSON fallback data during final send to avoid unknown-command errors
            if (MatchBytes(pCur->m_pCommand[0], "$ExtJSON "))
            {
                LogDbg("[EXTJSON] SendFinalQueue skipped ExtJSON final send len={}", pCur->m_ui32CmdLen[0]);
            }
            else
#endif
            {
                AddDataToQueueStr(m_GlobalQueues[0], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
            }
            break;
        default:
            break;
        }
    }

    for (const auto& pItem : m_NewQueueItems)
    {
        QueueItem* pCur = pItem.get();

        switch (pCur->m_eCmdType)
        {
        case Cmd::OPS:
        case Cmd::CHAT:
        case Cmd::LUA:
#ifdef USE_FLYLINKDC_EXT_JSON
            // Skip ExtJSON fallback data during final send
            if (MatchBytes(pCur->m_pCommand[0], "$ExtJSON "))
            {
                LogDbg("[EXTJSON] SendFinalQueue new queue skipped ExtJSON len={}", pCur->m_ui32CmdLen[0]);
            }
            else
#endif
            {
                AddDataToQueueStr(m_GlobalQueues[0], pCur->m_pCommand[0], pCur->m_ui32CmdLen[0]);
            }
            break;
        default:
            break;
        }
    }

    if (m_GlobalQueues[0].m_szLen == 0)
    {
        return;
    }

    ZlibUtility::m_Ptr->CreateZPipe(
        std::string_view(m_GlobalQueues[0].m_pBuffer.data(), m_GlobalQueues[0].m_szLen), m_GlobalQueues[0].m_Zbuffer, m_GlobalQueues[0].m_szZlen);

    for (const auto& pCurPtr : Users::m_Ptr->m_UserList)
    {
        User* pCur = pCurPtr.get();

        if (m_GlobalQueues[0].m_szZlen != 0)
        {
            (void)pCur->PutInSendBuf(m_GlobalQueues[0].m_Zbuffer.data(), m_GlobalQueues[0].m_szZlen);
            ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[0].m_szLen - m_GlobalQueues[0].m_szZlen);
        }
        else
        {
            (void)pCur->PutInSendBuf(m_GlobalQueues[0].m_pBuffer.data(), m_GlobalQueues[0].m_szLen);
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GlobalDataQueue::AddDataToQueueStr(GlobalQueue& pQueue, const std::string& sData)
{
    if (!sData.empty())
    {
        AddDataToQueue(pQueue, sData.data(), sData.size());
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GlobalDataQueue::AddDataToQueueStr(GlobalQueue& pQueue, const char* sData, const size_t szLen)
{
    if (sData && szLen != 0)
    {
        AddDataToQueue(pQueue, sData, szLen);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
GlobalDataQueue::QueueItemPtr GlobalDataQueue::CreateQueueItem(const char* sCmd1, size_t szLen1, const char* sCmd2, size_t szLen2, Cmd eCmdType)
{
    // Single allocation: QueueItem + command data in one block
    const size_t szAlign1 = (szLen1 + 7) & ~size_t(7);
    const size_t szAlign2 = (szLen2 + 7) & ~size_t(7);
    const size_t szTotal = sizeof(QueueItem) + szAlign1 + szAlign2;

    auto* pBuf = new (std::nothrow) char[szTotal];
    if (!pBuf)
    {
        LogDbgErr("[MEM] Cannot allocate {} bytes in GlobalDataQueue::CreateQueueItem", szTotal);
        return {nullptr, DestroyQueueItem};
    }

    auto* pItem = new (pBuf) QueueItem();
    pItem->m_eCmdType = eCmdType;

    char* pData = pBuf + sizeof(QueueItem);
    if (sCmd1 && szLen1 != 0)
    {
        memcpy(pData, sCmd1, szLen1);
        pItem->m_pCommand[0] = pData;
        pItem->m_ui32CmdLen[0] = static_cast<uint32_t>(szLen1);
    }
    pData += szAlign1;
    if (sCmd2 && szLen2 != 0)
    {
        memcpy(pData, sCmd2, szLen2);
        pItem->m_pCommand[1] = pData;
        pItem->m_ui32CmdLen[1] = static_cast<uint32_t>(szLen2);
    }

    return {pItem, DestroyQueueItem};
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GlobalDataQueue::DestroyQueueItem(QueueItem* pItem)
{
    if (pItem)
    {
        pItem->~QueueItem();
        delete[] reinterpret_cast<char*>(pItem);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GlobalDataQueue::AddDataToQueue(GlobalQueue& rQueue, const char* sData, const size_t szLen)
{
    if (rQueue.m_pBuffer.capacity() < (rQueue.m_szLen + szLen + 1))
    {
        const size_t szAllignLen = AlignUp(rQueue.m_szLen + szLen);
        try
        {
            rQueue.m_pBuffer.resize(szAllignLen);
        }
        catch (std::bad_alloc&)
        {
            LogDbgErr("[MEM] Cannot allocate {} bytes in GlobalDataQueue::AddDataToQueue", szAllignLen);
            return;
        }
    }

    memcpy(rQueue.m_pBuffer.data() + rQueue.m_szLen, sData, szLen);
    rQueue.m_szLen += szLen;
    rQueue.m_pBuffer.data()[rQueue.m_szLen] = '\0';
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GlobalDataQueue::QueueItem* GlobalDataQueue::GetLastQueueItem()
{
    return m_NewQueueItems.empty() ? nullptr : m_NewQueueItems.back().get();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GlobalDataQueue::QueueItem* GlobalDataQueue::GetFirstQueueItem()
{
    return m_NewQueueItems.empty() ? nullptr : m_NewQueueItems.front().get();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GlobalDataQueue::QueueItem* GlobalDataQueue::InsertBlankQueueItem(QueueItem* pAfterItem, Cmd eCmdType)
{
    // Pre-allocate space for command data (chat messages are typically < 8KB)
    constexpr size_t EXTRA_SPACE = 8192;
    const size_t szTotal = sizeof(QueueItem) + EXTRA_SPACE;
    auto* pBuf = new (std::nothrow) char[szTotal];
    if (!pBuf)
    {
        LogDbg("[MEM] Cannot allocate pNewItem in GlobalDataQueue::InsertBlankQueueItem");
        return nullptr;
    }

    auto* pNewItem = new (pBuf) QueueItem();
    pNewItem->m_eCmdType = eCmdType;
    // m_pCommand[0] points into the extra space after the struct
    pNewItem->m_pCommand[0] = pBuf + sizeof(QueueItem);

    QueueItemPtr pOwned(pNewItem, DestroyQueueItem);

    // Insert after the node whose address == pAfterItem; if pAfterItem is the
    // first item, insert at the front. Otherwise append at the end.
    if (!pAfterItem || (!m_NewQueueItems.empty() && pAfterItem == m_NewQueueItems.front().get()))
    {
        m_NewQueueItems.push_front(std::move(pOwned));
        return pNewItem;
    }

    for (auto it = m_NewQueueItems.begin(); it != m_NewQueueItems.end(); ++it)
    {
        if (it->get() == pAfterItem)
        {
            m_NewQueueItems.insert(std::next(it), std::move(pOwned));
            return pNewItem;
        }
    }

    m_NewQueueItems.push_back(std::move(pOwned));

    return pNewItem;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::FillBlankQueueItem(const char* sCommand, const size_t szLen, GlobalDataQueue::QueueItem* pQueueItem)
{
    if (szLen != 0)
    {
        constexpr size_t MAX_BLANK_CMD_SIZE = 8192;
        const size_t szCopyLen = szLen > MAX_BLANK_CMD_SIZE ? MAX_BLANK_CMD_SIZE : szLen;
        memcpy(pQueueItem->m_pCommand[0], sCommand, szCopyLen);
        pQueueItem->m_ui32CmdLen[0] = static_cast<uint32_t>(szCopyLen);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::StatusMessageFormat(const char* sFrom, const char* sFormatMsg, ...)
{
    int iMsgLen = 0;

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES_AS_PM)])
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $", SettingManager::HubSec());
        if (iMsgLen <= 0)
        {
            LogDbgErr("[ERR] snprintf wrong value {} in GlobalDataQueue::StatusMessageFormat from: {}", iMsgLen, sFrom);
            return;
        }
    }

    va_list vlArgs;
    va_start(vlArgs, sFormatMsg);

    const int iRet = vsnprintf(ServerManager::m_pGlobalBuffer + iMsgLen, ServerManager::m_szGlobalBufferSize - iMsgLen, sFormatMsg, vlArgs);

    va_end(vlArgs);

    if (iRet <= 0)
    {
        LogDbgErr("[ERR] vsnprintf wrong value {} in GlobalDataQueue::StatusMessageFormat from: {}", iRet, sFrom);
        return;
    }

    iMsgLen += iRet;

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES_AS_PM)])
    {
        SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::SendItem::PM2OPS);
    }
    else
    {
        AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::OPS);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
