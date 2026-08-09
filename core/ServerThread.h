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
#ifndef ServerThreadH
#define ServerThreadH
//---------------------------------------------------------------------------

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <unordered_map>

class ServerThread
{
private:
    struct AntiConFlood
    {
        uint64_t m_ui64Time = 0;

        int16_t m_ui16Hits = 0;

        std::array<uint8_t, 16> m_ui128IpHash{};

        explicit AntiConFlood(const uint8_t* pIpHash);

        AntiConFlood(AntiConFlood&& o) noexcept : m_ui64Time(o.m_ui64Time), m_ui16Hits(o.m_ui16Hits), m_ui128IpHash(o.m_ui128IpHash) {}

        auto operator=(AntiConFlood&&) -> AntiConFlood& = delete;
        AntiConFlood(const AntiConFlood&) = delete;
        auto operator=(const AntiConFlood&) -> AntiConFlood& = delete;
    };

    struct AntiConFloodKey
    {
        std::array<uint8_t, 16> m_ui128IpHash{};
        explicit AntiConFloodKey(const uint8_t* pIpHash)
        {
            memcpy(m_ui128IpHash.data(), pIpHash, 16);
        }
    };

    struct AntiConFloodKeyHash
    {
        auto operator()(const AntiConFloodKey& k) const noexcept -> size_t
        {
            // FNV-1a
            size_t h = 14695981039346656037ULL;
            for (const auto b : k.m_ui128IpHash)
            {
                h ^= b;
                h *= 1099511628211ULL;
            }
            return h;
        }
    };

    struct AntiConFloodKeyEqual
    {
        auto operator()(const AntiConFloodKey& a, const AntiConFloodKey& b) const noexcept -> bool
        {
            return a.m_ui128IpHash == b.m_ui128IpHash;
        }
    };

    std::unordered_map<AntiConFloodKey, AntiConFlood, AntiConFloodKeyHash, AntiConFloodKeyEqual> m_AntiFloodMap;

    // Tracks m_AntiFloodMap.size() atomically. The map itself is mutated only by its own
    // accept thread, but size is read cross-thread by GetTotalAntiFloodCount() (main thread
    // Prometheus tick + other server threads) — a plain .size() read concurrent with
    // mutation is a data race on unordered_map internals.
    std::atomic<uint32_t> m_ui32AntiFloodCount{0};

    CriticalSection m_csServerThread;

    pthread_t m_ThreadId = 0;

    // pthread_mutex_t m_mtxServerThread;

    int m_Server = -1;
    uint32_t m_ui32SuspendTime = 0;

    int m_iAdressFamily;

    std::atomic<bool> m_bTerminated{false};

public:
    ServerThread(const ServerThread&) = delete;
    auto operator=(const ServerThread&) -> ServerThread& = delete;

    uint16_t m_ui16Port;

    std::atomic<bool> m_bActive{false}, m_bSuspended{false};

    ServerThread(int iAddrFamily, uint16_t ui16PortNumber);
    ~ServerThread();

    void Resume();
    void Run();
    void Close();
    void WaitFor();
    [[nodiscard]] auto Listen(bool bSilent = false) -> bool;
    [[nodiscard]] auto isFlooder(int& s, const sockaddr_storage& addr) -> bool;
    void ResumeSck();
    void SuspendSck(uint32_t ui32Time);

    static std::atomic<uint32_t> m_ui32ConnectionFloodCount;

    [[nodiscard]] static auto GetTotalAntiFloodCount() -> uint32_t;
};
//---------------------------------------------------------------------------

#endif
