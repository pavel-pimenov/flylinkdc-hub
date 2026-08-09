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
#ifndef ServerManagerH
#define ServerManagerH
//---------------------------------------------------------------------------
#include <array>
#include <atomic>
#include <list>
#include <memory>
#include <string>

//---------------------------------------------------------------------------
class ServerThread;
//---------------------------------------------------------------------------

class ServerManager
{
public:
    static std::string m_sPath, m_sScriptPath;

#ifdef FLYLINKDC_USE_CPU_STAT
    static std::array<double, 60> m_dCpuUsages;
    static double m_dCpuUsage;
#endif
    static std::atomic<uint64_t> m_ui64ActualTick;
    static uint64_t m_ui64TotalShare;

    static uint64_t m_ui64BytesRead, m_ui64BytesSent, m_ui64BytesSentSaved;
    static uint64_t m_ui64LastBytesRead, m_ui64LastBytesSent;
    static uint64_t m_ui64Mins, m_ui64Hours, m_ui64Days;

#ifdef __MACH__
    static clock_serv_t m_csMachClock;
#endif

    static time_t m_tStartTime;

    static std::list<std::unique_ptr<ServerThread>> m_Servers;

    // Single shared buffer for all snprintf/string operations in the event loop.
    // SAFE only because the hub is single-threaded (one event loop).
    // If multi-threading is ever added, this MUST become thread-local or per-call-site.
    static char* m_pGlobalBuffer;

    static size_t m_szGlobalBufferSize;

    static uint32_t m_ui32CpuCount;

    static uint32_t m_ui32Joins, m_ui32Parts, m_ui32Logged, m_ui32Peak;
    static uint32_t m_ui32ConnectionsAccepted, m_ui32ConnectionsClosed;
    static std::array<uint32_t, 60> m_ui32UploadSpeed, m_ui32DownloadSpeed;
    static uint32_t m_ui32ActualBytesRead, m_ui32ActualBytesSent;
    static uint32_t m_ui32AverageBytesRead, m_ui32AverageBytesSent;

    static bool m_bServerRunning, m_bServerTerminated, m_bIsRestart, m_bIsClose;

    static bool m_bDaemon;

    static bool m_bCmdAutoStart, m_bCmdNoAutoStart, m_bCmdNoTray, m_bUseIPv4, m_bUseIPv6, m_bIPv6DualStack;

    static std::array<char, 16> m_sHubIP;
    static std::array<char, 40> m_sHubIP6;

    static uint8_t m_ui8SrCntr, m_ui8MinTick;

    static void OnSecTimer();

    static void Initialize();

    static void FinalStop(bool bDeleteServiceLoop);
    static void ResumeAccepts();
    static void SuspendAccepts(uint32_t ui32Time);

    [[nodiscard]] static auto Start() -> bool;
    static void UpdateServers();
    static void Stop();
    static void FinalClose();
    static void CreateServerThread(int iAddrFamily, uint16_t ui16PortNumber, bool bResume = false);

    static void CommandLineSetup();

    [[nodiscard]] static auto ResolveHubAddress(bool bSilent = false) -> bool;
};
//---------------------------------------------------------------------------

#endif
