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

#ifndef GlobalDataQueueH
#define GlobalDataQueueH

#include <array>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include "PrometheusMetrics.h"

struct User;

class CFlyBuffer
{
public:
    std::vector<char> m_pBuffer;
    size_t m_szLen = 0;
    void clean()
    {
        if (m_szLen == 0 && m_pBuffer.capacity() > 256)
        {
            m_pBuffer = std::vector<char>(256);
        }
    }
    CFlyBuffer() = default;
    CFlyBuffer(const CFlyBuffer&) = delete;
    auto operator=(const CFlyBuffer&) -> CFlyBuffer& = delete;
};

class GlobalDataQueue
{
public:
    enum class Cmd : uint8_t
    {
        HUBNAME,
        CHAT,
        HELLO,
        MYINFO,
        QUIT,
        OPS,
        LUA,
        ACTIVE_SEARCH_V6,
        ACTIVE_SEARCH_V64,
        ACTIVE_SEARCH_V4,
        PASSIVE_SEARCH_V6,
        PASSIVE_SEARCH_V64,
        PASSIVE_SEARCH_V4,
        PASSIVE_SEARCH_V4_ONLY,
        PASSIVE_SEARCH_V6_ONLY,
#ifdef USE_FLYLINKDC_EXT_JSON
        EXTJSON
#endif
    };

    enum class SendItem : uint8_t
    {
        PM2ALL,
        PM2OPS,
        OPCHAT,
        TOPROFILE,
        PM2PROFILE,
    };

    struct QueueItem
    {
        std::array<char*, 2> m_pCommand = {nullptr, nullptr};
        std::array<uint32_t, 2> m_ui32CmdLen = {0, 0};
        Cmd m_eCmdType = Cmd::HUBNAME;
        QueueItem() = default;
        QueueItem(const QueueItem&) = delete;
        auto operator=(const QueueItem&) -> QueueItem& = delete;
    };

    static void DestroyQueueItem(QueueItem* pItem);
    using QueueItemPtr = std::unique_ptr<QueueItem, decltype(&DestroyQueueItem)>;

private:
    struct GlobalQueue : public CFlyBuffer
    {
        bool m_bCreated = false, m_bZlined = false;
        std::vector<char> m_Zbuffer;
        uint32_t m_szZlen = 0;
        GlobalQueue() = default;
        GlobalQueue(const GlobalQueue&) = delete;
        auto operator=(const GlobalQueue&) -> GlobalQueue& = delete;
    };

    struct OpsQueue : public CFlyBuffer
    {
    };

    struct IPsQueue : public CFlyBuffer
    {
        bool m_bHaveDollars = false;
        IPsQueue() = default;
        IPsQueue(const IPsQueue&) = delete;
        auto operator=(const IPsQueue&) -> IPsQueue& = delete;
    };

    struct SingleDataItem
    {
        User* m_pFromUser = nullptr;
        std::vector<char> m_Data;
        size_t m_szDataLen = 0;
        int32_t m_i32Profile = 0;
        SendItem m_eType = SendItem::PM2ALL;
        SingleDataItem() = default;
        SingleDataItem(const SingleDataItem&) = delete;
        auto operator=(const SingleDataItem&) -> SingleDataItem& = delete;
    };

    std::array<GlobalQueue, 144> m_GlobalQueues{};
    OpsQueue m_OpListQueue;
    IPsQueue m_UserIPQueue;
    std::list<GlobalQueue*> m_CreatedGlobalQueues;
    std::list<QueueItemPtr> m_NewQueueItems;
    std::list<QueueItemPtr> m_QueueItems;
    std::list<std::unique_ptr<SingleDataItem>> m_NewSingleItems;

    static void AddDataToQueue(GlobalQueue& rQueue, const char* sData, size_t szLen);
    static void AddDataToQueueStr(GlobalQueue& pQueue, const std::string& sData);
    static void AddDataToQueueStr(GlobalQueue& pQueue, const char* sData, size_t szLen);
    [[nodiscard]] static auto CreateQueueItem(const char* sCmd1, size_t szLen1, const char* sCmd2, size_t szLen2, Cmd eCmdType) -> QueueItemPtr;

public:
    static std::unique_ptr<GlobalDataQueue> m_Ptr;

    std::list<std::unique_ptr<SingleDataItem>> m_SingleItems;
    bool m_bHaveItems = false;

    GlobalDataQueue(const GlobalDataQueue&) = delete;
    auto operator=(const GlobalDataQueue&) -> GlobalDataQueue& = delete;

    enum : uint16_t
    {
        BIT_LONG_MYINFO = 0x1,
        BIT_ALL_SEARCHES_IPV64 = 0x2,
        BIT_ALL_SEARCHES_IPV6 = 0x4,
        BIT_ALL_SEARCHES_IPV4 = 0x8,
        BIT_ACTIVE_SEARCHES_IPV64 = 0x10,
        BIT_ACTIVE_SEARCHES_IPV6 = 0x20,
        BIT_ACTIVE_SEARCHES_IPV4 = 0x40,
        BIT_HELLO = 0x80,
        BIT_OPERATOR = 0x100,
        BIT_USERIP = 0x200,
        BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4 = 0x400,
        BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4 = 0x800,
    };

    GlobalDataQueue();
    ~GlobalDataQueue();

    void AddQueueItem(const char* sCommand1, size_t szLen1, const char* sCommand2, size_t szLen2, Cmd eCmdType);
    void AddQueueItem(const std::string& sCommand1, const std::string& sCommand2, Cmd eCmdType)
    {
        AddQueueItem(sCommand1.c_str(), sCommand1.size(), sCommand2.c_str(), sCommand2.size(), eCmdType);
    }
    void OpListStore(const char* sNick);
    void UserIPStore(User* pUser);
    void PrepareQueueItems();
    void ClearQueues();
    void ProcessQueues(User* pUser);
    void AddSearchDataToQueue(const User* pUser, uint32_t ui32QueueType, const QueueItem* pCur);
    void ProcessSingleItems(User* pUser) const;
    void SingleItemStore(const char* sData, size_t szDataLen, User* pFromUser, int32_t i32Profile, SendItem eType);
    void SendFinalQueue();
    [[nodiscard]] auto GetLastQueueItem() -> QueueItem*;
    [[nodiscard]] auto GetFirstQueueItem() -> QueueItem*;
    [[nodiscard]] auto InsertBlankQueueItem(QueueItem* pAfterItem, Cmd eCmdType) -> QueueItem*;
    static void FillBlankQueueItem(const char* sCommand, size_t szLen, QueueItem* pQueueItem);
    void StatusMessageFormat(const char* sFrom, const char* sFormatMsg, ...);

    PrometheusMetrics m_metrics;

    void PrometheusRecvBytes(const char* p_type, int p_len)
    {
        auto* cBytes = m_metrics.counter_get("flylinkdc_hub_recv_bytes_total", "type", p_type);
        auto* cPkts = m_metrics.counter_get("flylinkdc_hub_recv_packets_total", "type", p_type);
        if (cBytes)
            cBytes->Increment(p_len);
        if (cPkts)
            cPkts->Increment(1);
    }

    void PrometheusSendBytes(const char* p_type, int p_len)
    {
        auto* cBytes = m_metrics.counter_get("flylinkdc_hub_send_bytes_total", "type", p_type);
        auto* cPkts = m_metrics.counter_get("flylinkdc_hub_send_packets_total", "type", p_type);
        if (cBytes)
            cBytes->Increment(p_len);
        if (cPkts)
            cPkts->Increment(1);
    }

    void PrometheusZlibBytes(const char* p_type, int p_len)
    {
        auto* cBytes = m_metrics.counter_get("flylinkdc_hub_compress_bytes_total", "type", p_type);
        if (cBytes)
            cBytes->Increment(p_len);
    }

    void PrometheusLogBytes(const char* p_type, int p_len)
    {
        auto* cBytes = m_metrics.counter_get("flylinkdc_hub_log_bytes_total", "type", p_type);
        if (cBytes)
            cBytes->Increment(p_len);
    }

    void PrometheusLuaInc(const char* p_command)
    {
        auto* cCalls = m_metrics.counter_get("flylinkdc_hub_lua_calls_total", "func", p_command);
        if (cCalls)
            cCalls->Increment(1);
    }

    void PrometheusLuaUserValueInc(uint8_t p_id)
    {
        m_metrics.counter_inc("flylinkdc_hub_lua_user_value_total", 1, {{"value", std::to_string(p_id)}});
    }

    void PrometheusLuaUserDataInc(uint8_t p_id)
    {
        m_metrics.counter_inc("flylinkdc_hub_lua_userdata_total", 1, {{"value", std::to_string(p_id)}});
    }

    void PrometheusRusageValue(const char* p_type, long p_val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_rusage", "resource", p_type);
        if (g)
            g->Set(static_cast<double>(p_val));
    }

    void PrometheusUsersGauge(double val)
    {
        m_metrics.gauge_inc("flylinkdc_hub_users_online", val);
    }

    void PrometheusMessagesGauge(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_messages_queue", val);
    }

    void PrometheusSqliteSizeBytes(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_sqlite_size_bytes", val);
    }

    void PrometheusUsersLoggedIn(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_logged_in", val);
    }

    void PrometheusJoinsTotal(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_joins_total", val);
    }

    void PrometheusPartsTotal(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_parts_total", val);
    }

    void PrometheusUsersPeak(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_peak", val);
    }

    void PrometheusShareBytes(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_share_bytes", val);
    }

    void PrometheusStartTime(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_start_time_seconds", val);
    }

    void PrometheusBandwidth(const char* direction, long val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_bandwidth_bytes_per_sec", "direction", direction);
        if (g)
            g->Set(static_cast<double>(val));
    }

    void PrometheusOperatorsOnline(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_operators_online", val);
    }

    void PrometheusUsersConnecting(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_connecting", val);
    }

    void PrometheusUsersHidden(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_hidden", val);
    }

    void PrometheusUsersGagged(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_gagged", val);
    }

    void PrometheusUsersActive(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_active", val);
    }

    void PrometheusUsersIpv6(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_ipv6", val);
    }

    void PrometheusUsersSharing(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_sharing", val);
    }

    void PrometheusUsersRegistered(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_users_registered", val);
    }

    void PrometheusBansCount(const char* type, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_bans_count", "type", type);
        if (g)
            g->Set(val);
    }

    void PrometheusSendRests(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_send_rests", val);
    }

    void PrometheusRecvRests(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_recv_rests", val);
    }

    void PrometheusCompressionSavedBytes(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_compression_saved_bytes", val);
    }

    void PrometheusScriptsCount(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_scripts_count", val);
    }

    void PrometheusBotsCount(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_bots_count", val);
    }

    void PrometheusProfilesCount(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_profiles_count", val);
    }

    void PrometheusActiveSearches(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_active_searches", val);
    }

    void PrometheusPassiveSearches(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_passive_searches", val);
    }

    void PrometheusTotalSlots(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_total_slots", val);
    }

    void PrometheusUsersByProfile(int profile, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_users_by_profile", "profile", std::to_string(profile));
        if (g)
            g->Set(val);
    }

    void PrometheusBuildInfo(const char* version)
    {
        m_metrics.gauge_set("flylinkdc_hub_build_info", 1, {{"version", version}});
    }

    void PrometheusDcCommandStat(const char* command, double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_command_stats", val, {{"command", command}});
    }

    void PrometheusLuaMemoryBytes(const char* script, int kb)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_lua_memory_bytes", "script", script);
        if (g)
            g->Set(kb * 1024);
    }

    void PrometheusLuaCallCount(const char* script, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_lua_call_count", "script", script);
        if (g)
            g->Set(val);
    }

    void PrometheusLuaTimeNsec(const char* script, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_lua_time_nsec", "script", script);
        if (g)
            g->Set(val);
    }

    void PrometheusEventQueueDepth(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_event_queue_depth", val);
    }

    void PrometheusProtocolErrorsInc(const char* error)
    {
        auto* c = m_metrics.counter_get("flylinkdc_hub_protocol_errors_total", "error", error);
        if (c)
            c->Increment(1);
    }

    void PrometheusConnectionFloodInc()
    {
        m_metrics.counter_inc("flylinkdc_hub_connection_flood_total", 1);
    }

    void PrometheusBruteforceAttemptsInc()
    {
        m_metrics.counter_inc("flylinkdc_hub_bruteforce_attempts_total", 1);
    }

    void PrometheusFastReconnectInc()
    {
        m_metrics.counter_inc("flylinkdc_hub_fast_reconnect_total", 1);
    }

    void PrometheusIpSpread(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_ip_spread", val);
    }

    void PrometheusCpuUsage(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_cpu_usage_percent", val);
    }

    void PrometheusUsersByState(const char* state, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_users_by_state", "state", state);
        if (g)
            g->Set(val);
    }

    void PrometheusLuaTimersActive(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_lua_timers_active", val);
    }

    void PrometheusUdpDebugSubscribers(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_udp_debug_subscribers", val);
    }

    void PrometheusSendRestsPeak(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_send_rests_peak", val);
    }

    void PrometheusRecvRestsPeak(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_recv_rests_peak", val);
    }

    void PrometheusReservedNicks(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_reserved_nicks", val);
    }

    void PrometheusIp2CountryRanges(const char* family, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_ip2country_ranges", "family", family);
        if (g)
            g->Set(val);
    }

    void PrometheusServerThreadSuspended(const char* port, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_server_thread_suspended", "port", port);
        if (g)
            g->Set(val);
    }

    void PrometheusConFloodWatchlist(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_con_flood_watchlist", val);
    }

    void PrometheusGlobalQueueBufferBytes(const char* queue, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_global_queue_buffer_bytes", "queue", queue);
        if (g)
            g->Set(val);
    }

    void EmitGlobalQueueBufferMetrics()
    {
        uint64_t globalQ = 0;
        for (const auto& q : m_GlobalQueues)
        {
            globalQ += q.m_szLen;
        }
        PrometheusGlobalQueueBufferBytes("global", static_cast<double>(globalQ));
        PrometheusGlobalQueueBufferBytes("ops", static_cast<double>(m_OpListQueue.m_szLen));
        PrometheusGlobalQueueBufferBytes("userip", static_cast<double>(m_UserIPQueue.m_szLen));
    }

    void PrometheusUsersListBufferBytes(const char* list, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_users_list_buffer_bytes", "list", list);
        if (g)
            g->Set(val);
    }

    void PrometheusUsersBufferBytes(const char* buffer, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_users_buffer_bytes", "buffer", buffer);
        if (g)
            g->Set(val);
    }

    void PrometheusCommandLatency(const char* cmd, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_command_latency_nsec", "command", cmd);
        if (g)
            g->Set(val);
    }

    void PrometheusConnectionsAccepted(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_connections_accepted", val);
    }

    void PrometheusConnectionsClosed(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_connections_closed", val);
    }

    void PrometheusFdCount(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_fd_count", val);
    }

    void PrometheusLuaGcPauseNsec(const char* script, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_lua_gc_pause_nsec", "script", script);
        if (g)
            g->Set(val);
    }

    void PrometheusLuaTimersPerScript(const char* script, double val)
    {
        auto* g = m_metrics.gauge_get("flylinkdc_hub_lua_timers_per_script", "script", script);
        if (g)
            g->Set(val);
    }

    void PrometheusPendingConnections(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_pending_connections", val);
    }

    void PrometheusConfigReloadTime(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_config_reload_time_seconds", val);
    }

    void PrometheusTestPortQueries(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_test_port_queries_total", val);
    }

    void PrometheusTestPortBytesIn(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_test_port_bytes_in_total", val);
    }

    void PrometheusTestPortBytesOut(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_test_port_bytes_out_total", val);
    }

    void PrometheusTestPortCompressedBytesIn(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_test_port_compressed_bytes_in_total", val);
    }

    void PrometheusTestPortCompressedBytesOut(double val)
    {
        m_metrics.gauge_set("flylinkdc_hub_test_port_compressed_bytes_out_total", val);
    }
};

#endif
