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
#include <sstream>
//---------------------------------------------------------------------------
#include "ServerManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "HubCommands.h"
#include "IP2Country.h"
#include "LuaScript.h"
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextConverter.h"
#include "TextFileManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#endif
//---------------------------------------------------------------------------

template <typename Ptr>
inline void allocMgr(Ptr& ptr)
{
    ptr = std::make_unique<typename Ptr::element_type>();
    if (!ptr)
    {
        LogDbg("[MEM] Cannot allocate manager");
        exit(EXIT_FAILURE);
    }
}

//---------------------------------------------------------------------------

#ifdef __MACH__
clock_serv_t ServerManager::m_csMachClock;
#endif

std::string ServerManager::m_sPath, ServerManager::m_sScriptPath;
size_t ServerManager::m_szGlobalBufferSize = 0;
char* ServerManager::m_pGlobalBuffer = nullptr;
bool ServerManager::m_bCmdAutoStart = false, ServerManager::m_bCmdNoAutoStart = false, ServerManager::m_bCmdNoTray = false, ServerManager::m_bUseIPv4 = true,
     ServerManager::m_bUseIPv6 = true, ServerManager::m_bIPv6DualStack = false;

#ifdef FLYLINKDC_USE_CPU_STAT
std::array<double, 60> ServerManager::m_dCpuUsages{};
double ServerManager::m_dCpuUsage = 0;
#endif

std::atomic<uint64_t> ServerManager::m_ui64ActualTick{0};
uint64_t ServerManager::m_ui64TotalShare = 0;
uint64_t ServerManager::m_ui64BytesRead = 0, ServerManager::m_ui64BytesSent = 0, ServerManager::m_ui64BytesSentSaved = 0;
uint64_t ServerManager::m_ui64LastBytesRead = 0, ServerManager::m_ui64LastBytesSent = 0;
uint64_t ServerManager::m_ui64Mins = 0, ServerManager::m_ui64Hours = 0, ServerManager::m_ui64Days = 0;

uint32_t ServerManager::m_ui32CpuCount = 0;

std::array<uint32_t, 60> ServerManager::m_ui32UploadSpeed{}, ServerManager::m_ui32DownloadSpeed{};
uint32_t ServerManager::m_ui32Joins = 0, ServerManager::m_ui32Parts = 0, ServerManager::m_ui32Logged = 0, ServerManager::m_ui32Peak = 0;
uint32_t ServerManager::m_ui32ConnectionsAccepted = 0, ServerManager::m_ui32ConnectionsClosed = 0;
uint32_t ServerManager::m_ui32ActualBytesRead = 0, ServerManager::m_ui32ActualBytesSent = 0;
uint32_t ServerManager::m_ui32AverageBytesRead = 0, ServerManager::m_ui32AverageBytesSent = 0;

std::list<std::unique_ptr<ServerThread>> ServerManager::m_Servers;

time_t ServerManager::m_tStartTime = 0;

bool ServerManager::m_bServerRunning = false, ServerManager::m_bServerTerminated = false, ServerManager::m_bIsRestart = false,
     ServerManager::m_bIsClose = false;

bool ServerManager::m_bDaemon = false;

std::array<char, 16> ServerManager::m_sHubIP{};
std::array<char, 40> ServerManager::m_sHubIP6{};

uint8_t ServerManager::m_ui8SrCntr = 0, ServerManager::m_ui8MinTick = 0;
//---------------------------------------------------------------------------

void ServerManager::OnSecTimer()
{
#ifdef FLYLINKDC_USE_CPU_STAT
    struct rusage rs;

    getrusage(RUSAGE_SELF, &rs);

    const double dcpuSec =
        double(rs.ru_utime.tv_sec) + (double(rs.ru_utime.tv_usec) / 1000000) + double(rs.ru_stime.tv_sec) + (double(rs.ru_stime.tv_usec) / 1000000);
    m_dCpuUsage = dcpuSec - m_dCpuUsages[m_ui8MinTick];
    m_dCpuUsages[m_ui8MinTick] = dcpuSec;
#endif

    if (++m_ui8MinTick == 60)
    {
        m_ui8MinTick = 0;
    }

    m_ui64ActualTick++;

    m_ui32ActualBytesRead = static_cast<uint32_t>(m_ui64BytesRead - m_ui64LastBytesRead);
    m_ui32ActualBytesSent = static_cast<uint32_t>(m_ui64BytesSent - m_ui64LastBytesSent);
    m_ui64LastBytesRead = m_ui64BytesRead;
    m_ui64LastBytesSent = m_ui64BytesSent;

    m_ui32AverageBytesSent -= m_ui32UploadSpeed[m_ui8MinTick];
    m_ui32AverageBytesRead -= m_ui32DownloadSpeed[m_ui8MinTick];

    m_ui32UploadSpeed[m_ui8MinTick] = m_ui32ActualBytesSent;
    m_ui32DownloadSpeed[m_ui8MinTick] = m_ui32ActualBytesRead;

    m_ui32AverageBytesSent += m_ui32UploadSpeed[m_ui8MinTick];
    m_ui32AverageBytesRead += m_ui32DownloadSpeed[m_ui8MinTick];
}
//---------------------------------------------------------------------------
void ServerManager::Initialize()
{
    setlocale(LC_ALL, "");

    time_t acctime;
    time(&acctime);
    srandom(acctime);

    if (!DirExist((ServerManager::m_sPath + "/logs").c_str()))
    {
        if (mkdir((ServerManager::m_sPath + "/logs").c_str(), 0755) == -1)
        {
            if (m_bDaemon)
            {
                spdlog::error("Creating of logs directory failed!");
            }
            else
            {
                printf("Creating  of logs directory failed!");
            }
        }
    }
    if (!DirExist((ServerManager::m_sPath + "/cfg").c_str()))
    {
        if (mkdir((ServerManager::m_sPath + "/cfg").c_str(), 0755) == -1)
        {
            LogInfo("Creating of cfg directory failed!");
        }
    }
    if (!DirExist((ServerManager::m_sPath + "/scripts").c_str()))
    {
        if (mkdir((ServerManager::m_sPath + "/scripts").c_str(), 0755) == -1)
        {
            LogInfo("Creating of scripts directory failed!");
        }
    }
    if (!DirExist((ServerManager::m_sPath + "/texts").c_str()))
    {
        if (mkdir((ServerManager::m_sPath + "/texts").c_str(), 0755) == -1)
        {
            LogInfo("Creating of texts directory failed!");
        }
    }

    ServerManager::m_sScriptPath = ServerManager::m_sPath + "/scripts/";

    // get cpu count
    std::string sCpuInfo;
    if (ReadWholeFile("/proc/cpuinfo", sCpuInfo))
    {
        std::istringstream iss(sCpuInfo);
        std::string sLine;
        while (std::getline(iss, sLine))
        {
            if (iequals(std::string_view(sLine).substr(0, 10), "model name") || sLine.starts_with("Processor") || sLine.starts_with("cpu model"))
            {
                m_ui32CpuCount++;
            }
        }
    }

    if (m_ui32CpuCount == 0)
    {
        m_ui32CpuCount = 1;
    }
    CreateGlobalBuffer();

    CheckForIPv6();

#ifdef __MACH__
    mach_port_t mpMachHost = mach_host_self();
    host_get_clock_service(mpMachHost, SYSTEM_CLOCK, &ServerManager::m_csMachClock);
    mach_port_deallocate(mach_task_self(), mpMachHost);
#endif

    allocMgr(ReservedNicksManager::m_Ptr);

    m_ui64ActualTick = m_ui64TotalShare = 0;
    m_ui64BytesRead = m_ui64BytesSent = m_ui64BytesSentSaved = 0;

    m_ui32ActualBytesRead = m_ui32ActualBytesSent = m_ui32AverageBytesRead = m_ui32AverageBytesSent = 0;

    m_ui32Joins = m_ui32Parts = m_ui32Logged = m_ui32Peak = 0;

    m_Servers.clear();

    m_tStartTime = 0;

    m_ui64Mins = m_ui64Hours = m_ui64Days = 0;

    m_bServerRunning = m_bIsRestart = m_bIsClose = false;

    m_sHubIP[0] = '\0';
    m_sHubIP6[0] = '\0';

    m_ui8SrCntr = 0;

    allocMgr(ZlibUtility::m_Ptr);

    m_ui8MinTick = 0;

    m_ui64LastBytesRead = m_ui64LastBytesSent = 0;

#ifdef FLYLINKDC_USE_CPU_STAT
    m_dCpuUsage = 0.0;
#endif

    allocMgr(SettingManager::m_Ptr);
#ifdef FLYLINKDC_USE_DB
#if defined(_WITH_SQLITE)

    allocMgr(TextConverter::m_Ptr);
#endif
#endif

    allocMgr(LanguageManager::m_Ptr);

    LanguageManager::m_Ptr->Load();

    allocMgr(ProfileManager::m_Ptr);

    allocMgr(RegManager::m_Ptr);

    // Load registered users
    RegManager::m_Ptr->Load();

    allocMgr(BanManager::m_Ptr);

    // load banlist
    BanManager::m_Ptr->Load();

    allocMgr(TextFilesManager::m_Ptr);

    allocMgr(UdpDebug::m_Ptr);

    allocMgr(ScriptManager::m_Ptr);

    SettingManager::m_Ptr->UpdateAll();
}
//---------------------------------------------------------------------------

bool ServerManager::Start()
{
    time(&m_tStartTime);

    SettingManager::m_Ptr->UpdateAll();

    TextFilesManager::m_Ptr->RefreshTextFiles();

    m_ui64ActualTick = m_ui64TotalShare = 0;

    m_ui64BytesRead = m_ui64BytesSent = m_ui64BytesSentSaved = 0;

    m_ui32ActualBytesRead = m_ui32ActualBytesSent = m_ui32AverageBytesRead = m_ui32AverageBytesSent = 0;

    m_ui32Joins = m_ui32Parts = m_ui32Logged = m_ui32Peak = 0;

    m_ui64Mins = m_ui64Hours = m_ui64Days = 0;

    m_ui8SrCntr = 0;

    m_sHubIP[0] = '\0';
    m_sHubIP6[0] = '\0';

    (void)ResolveHubAddress();

    for (unsigned short m_ui16PortNumber : SettingManager::m_Ptr->m_ui16PortNumbers)
    {
        if (m_ui16PortNumber == 0)
        {
            break;
        }

        if (!m_bUseIPv6)
        {
            CreateServerThread(AF_INET, m_ui16PortNumber);
            continue;
        }

        CreateServerThread(AF_INET6, m_ui16PortNumber);

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)] || !m_bIPv6DualStack)
        {
            CreateServerThread(AF_INET, m_ui16PortNumber);
        }
    }

    if (m_Servers.empty())
    {
        LogInfo("{}", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_VALID_TCP_PORT_SPECIFIED)]);
        return false;
    }

#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
    allocMgr(DBSQLite::m_Ptr);
#endif
#endif // FLYLINKDC_USE_DB
    allocMgr(IpP2Country::m_Ptr);

    allocMgr(EventQueue::m_Ptr);

    allocMgr(HashManager::m_Ptr);

    allocMgr(Users::m_Ptr);

    allocMgr(GlobalDataQueue::m_Ptr);

    GlobalDataQueue::m_Ptr->PrometheusStartTime(static_cast<double>(ServerManager::m_tStartTime));

    LogInfo("Serving started");

    allocMgr(DcCommands::m_Ptr);

    // add botname to reserved nicks
    ReservedNicksManager::m_Ptr->AddReservedNick(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str());
    SettingManager::m_Ptr->UpdateBot();

    // add opchat botname to reserved nicks
    ReservedNicksManager::m_Ptr->AddReservedNick(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
    SettingManager::m_Ptr->UpdateOpChat();

    ReservedNicksManager::m_Ptr->AddReservedNick(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_ADMIN_NICK)].c_str());

#ifdef FLYLINKDC_USE_UDP_THREAD
    int iUdpPort = 0;
    if (safe_stoi(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_UDP_PORT)].c_str(), iUdpPort) && static_cast<uint16_t>(iUdpPort) != 0)
    {
        if (!m_bUseIPv6)
        {
            UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
        }
        else
        {
            UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET6);

            if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)] || !m_bIPv6DualStack)
            {
                UDPThread::Destroy(UDPThread::m_PtrIPv6);
                UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
            }
        }
    }
#endif

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        ScriptManager::m_Ptr->Start();
    }

    allocMgr(ServiceLoop::m_Ptr);

    // Start the server socket threads
    for (const auto& pServer : m_Servers)
    {
        pServer->Resume();
    }

    m_bServerRunning = true;

    // Call lua_Main
    ScriptManager::m_Ptr->OnStartup();

    return true;
}
//---------------------------------------------------------------------------

void ServerManager::Stop()
{

    const int iRet = snprintf(m_pGlobalBuffer,
                        m_szGlobalBufferSize,
                        "Serving stopped (UL: %" PRIu64 " [%" PRIu64 "], DL: %" PRIu64 ")",
                        m_ui64BytesSent,
                        m_ui64BytesSentSaved,
                        m_ui64BytesRead);
    if (iRet > 0)
    {
        LogInfo("{}", m_pGlobalBuffer);
    }

    for (const auto& pServer : m_Servers)
    {
        pServer->Close();
        pServer->WaitFor();
    }

    m_Servers.clear();

    // stop the main hub loop
    if (ServiceLoop::m_Ptr)
    {
        m_bServerTerminated = true;
    }
    else
    {
        FinalStop(false);
    }
}
//---------------------------------------------------------------------------

void ServerManager::FinalStop(bool bDeleteServiceLoop)
{
    if (bDeleteServiceLoop)
    {
        ServiceLoop::m_Ptr.reset();
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        ScriptManager::m_Ptr->Stop();
    }

#ifdef FLYLINKDC_USE_UDP_THREAD
#ifdef FLYLINKDC_USE_UDP_THREAD_IP6
    UDPThread::Destroy(UDPThread::m_PtrIPv6);
#endif

    UDPThread::Destroy(UDPThread::m_PtrIPv4);
#endif

    // delete userlist field
    if (Users::m_Ptr)
    {
        Users::m_Ptr->DisconnectAll();
        Users::m_Ptr.reset();
    }

    DcCommands::m_Ptr.reset();

    // delete hashed userlist manager
    HashManager::m_Ptr.reset();

    GlobalDataQueue::m_Ptr.reset();
    EventQueue::m_Ptr.reset();

    IpP2Country::m_Ptr.reset();

#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
    DBSQLite::m_Ptr.reset();
#endif
#endif // FLYLINKDC_USE_DB

    // userstat  // better here ;)

    m_ui8SrCntr = 0;
    m_ui32Joins = m_ui32Parts = m_ui32Logged = 0;

    UdpDebug::m_Ptr->Cleanup();

    m_bServerRunning = false;

    if (m_bIsRestart)
    {
        m_bIsRestart = false;

        // start hub
        if (!Start())
        {
            LogError("Server start failed in ServerFinalStop");
            exit(EXIT_FAILURE);
        }
    }
    else if (m_bIsClose)
    {
        FinalClose();
    }
}
//---------------------------------------------------------------------------

void ServerManager::FinalClose()
{

    BanManager::m_Ptr->Save(true);

    ProfileManager::m_Ptr->SaveProfiles();

    RegManager::m_Ptr->Save();

    ScriptManager::m_Ptr->SaveScripts();

    SettingManager::m_Ptr->Save();

    ScriptManager::m_Ptr.reset();
    TextFilesManager::m_Ptr.reset();
    ProfileManager::m_Ptr.reset();
    UdpDebug::m_Ptr.reset();
    RegManager::m_Ptr.reset();
    BanManager::m_Ptr.reset();
    ZlibUtility::m_Ptr.reset();
    LanguageManager::m_Ptr.reset();
#ifdef FLYLINKDC_USE_DB
#if defined(_WITH_SQLITE)
    TextConverter::m_Ptr.reset();
#endif
#endif
    SettingManager::m_Ptr.reset();
    ReservedNicksManager::m_Ptr->Save();
    ReservedNicksManager::m_Ptr.reset();

#ifdef __MACH__
    mach_port_deallocate(mach_task_self(), m_csMachClock);
#endif

    DeleteGlobalBuffer();
}
//---------------------------------------------------------------------------

void ServerManager::UpdateServers()
{
    bool bFound = false;

    // Remove servers for ports we don't want use anymore
    for (auto it = m_Servers.begin(); it != m_Servers.end();)
    {
        bFound = false;

        for (const unsigned short m_ui16PortNumber : SettingManager::m_Ptr->m_ui16PortNumbers)
        {
            if (m_ui16PortNumber == 0)
            {
                break;
            }

            if ((*it)->m_ui16Port == m_ui16PortNumber)
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            LogInfo("[SYS] Stopping listener on port {} (removed from settings).", (*it)->m_ui16Port);
            (*it)->Close();
            (*it)->WaitFor();
            it = m_Servers.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Add servers for ports that not running
    for (const unsigned short m_ui16PortNumber : SettingManager::m_Ptr->m_ui16PortNumbers)
    {
        if (m_ui16PortNumber == 0)
        {
            break;
        }

        bFound = false;

        for (const auto& pServer : m_Servers)
        {
            if (pServer->m_ui16Port == m_ui16PortNumber)
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            if (!m_bUseIPv6)
            {
                CreateServerThread(AF_INET, m_ui16PortNumber);
                continue;
            }

            CreateServerThread(AF_INET6, m_ui16PortNumber);

            if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)] || !m_bIPv6DualStack)
            {
                CreateServerThread(AF_INET, m_ui16PortNumber);
            }
        }
    }
}
//---------------------------------------------------------------------------

void ServerManager::ResumeAccepts()
{
    if (!m_bServerRunning)
    {
        LogDbg("[SYS] ResumeAccepts called when server is not running.");
        return;
    }

    for (const auto& pServer : m_Servers)
    {
        pServer->ResumeSck();
    }
}
//---------------------------------------------------------------------------

void ServerManager::SuspendAccepts(const uint32_t ui32Time)
{
    if (!m_bServerRunning)
    {
        LogDbg("[SYS] SuspendAccepts called when server is not running.");
        return;
    }

    if (ui32Time != 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Suspending listening threads to %u seconds.", ui32Time);
    }
    else
    {
        const char sSuspendMsg[] = "[SYS] Suspending listening threads."; // NOLINT(modernize-avoid-c-arrays)
        UdpDebug::m_Ptr->Broadcast(sSuspendMsg, sizeof(sSuspendMsg) - 1);
    }

    for (const auto& pServer : m_Servers)
    {
        pServer->SuspendSck(ui32Time);
    }
}
//---------------------------------------------------------------------------

void ServerManager::CreateServerThread(const int iAddrFamily, const uint16_t ui16PortNumber, const bool bResume /* = false*/)
{
    auto pServer = std::make_unique<ServerThread>(iAddrFamily, ui16PortNumber);
    if (!pServer)
    {
        LogDbg("[MEM] Cannot allocate pServer in ServerCreateServerThread");
        exit(EXIT_FAILURE);
    }

    if (pServer->Listen())
    {
        LogInfo("[SYS] Listening on port {} ({}).", ui16PortNumber, iAddrFamily == AF_INET6 ? "IPv6" : "IPv4");

        if (bResume)
        {
            pServer->Resume();
        }

        m_Servers.push_back(std::move(pServer));
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ServerManager::CommandLineSetup()
{
    printf("%s built on %s %s\n\n", g_sPtokaXTitle, __DATE__, __TIME__);
    printf("Welcome to PtokaX configuration setup.\nDirectory for PtokaX configuration is: %s\nWhen this directory is wrong, then exit this setup.\nTo specify "
           "correct configuration directory start PtokaX with -c configdir parameter.",
           m_sPath.c_str());

    const char sMenu[] = "\n\nAvailable options:\n" // NOLINT(modernize-avoid-c-arrays)
                         "1. Basic setup. Only few things required for PtokaX run.\n"
                         "2. Complete setup. Long setup, where you can change all PtokaX setings.\n"
                         "3. Add registered user.\n"
                         "4. Exit this setup.\n\n"
                         "Your choice: ";

    printf("%s", sMenu);

    while (true)
    {
        const int iChar = getchar();

        while (getchar() != '\n')
        {
            // boredom...
        };

        switch (iChar)
        {
        case '1':
            SettingManager::m_Ptr->CmdLineBasicSetup();
            printf("%s", sMenu);
            continue;
        case '2':
            SettingManager::m_Ptr->CmdLineCompleteSetup();
            printf("%s", sMenu);
            continue;
        case '3':
            RegManager::m_Ptr->AddRegCmdLine();
            printf("%s", sMenu);
            continue;
        case '4':
            printf("%s ending...\n", g_sPtokaXTitle);
            break;
        default:
            printf("Unknown option: %c\nYour choice: ", iChar);
            continue;
        }

        break;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool ServerManager::ResolveHubAddress(const bool bSilent /* = false*/)
{
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_RESOLVE_TO_IP)])
    {
        if (!isIP(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)].c_str()))
        {

            addrinfo hints = {};

            if (m_bUseIPv6)
            {
                hints.ai_family = AF_UNSPEC;
            }
            else
            {
                hints.ai_family = AF_INET;
            }

            struct addrinfo* res;

            if (::getaddrinfo(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)].c_str(), nullptr, &hints, &res) != 0 ||
                (res->ai_family != AF_INET && res->ai_family != AF_INET6))
            {
                if (!bSilent)
                {
                    LogInfo("{} '{}' {}.\n{}. ",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESOLVING_OF_HOSTNAME)],
                            SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)],
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_FAILED)],
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CHECK_THE_ADDRESS_PLEASE)]);
                }

                return false;
            }

            LogInfo("*** {} {}.", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)], LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESOLVED_SUCCESSFULLY)]);

            if (m_bUseIPv6)
            {
                struct addrinfo* next = res;
                while (next)
                {
                    if (next->ai_family == AF_INET)
                    {
                        if ((reinterpret_cast<sockaddr_in*>(next->ai_addr))->sin_addr.s_addr != INADDR_ANY)
                        {
                            inet_ntop(AF_INET, &(reinterpret_cast<sockaddr_in*>(next->ai_addr))->sin_addr, m_sHubIP.data(), m_sHubIP.size());
                        }
                    }
                    else if (next->ai_family == AF_INET6)
                    {
                        inet_ntop(AF_INET6, &(reinterpret_cast<sockaddr_in6*>(next->ai_addr))->sin6_addr, m_sHubIP6.data(), m_sHubIP6.size());
                    }

                    next = next->ai_next;
                }
            }
            else if ((reinterpret_cast<sockaddr_in*>(res->ai_addr))->sin_addr.s_addr != INADDR_ANY)
            {
                inet_ntop(AF_INET, &(reinterpret_cast<sockaddr_in*>(res->ai_addr))->sin_addr, m_sHubIP.data(), m_sHubIP.size());
            }

            if (m_sHubIP[0] != '\0')
            {
                std::string msg = "*** " + std::string(m_sHubIP.data());
                if (IsPrivateIP(m_sHubIP.data()))
                {
                    SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_AUTO_REG), false);
                }
                if (m_sHubIP6[0] != '\0')
                {
                    msg += " / " + std::string(m_sHubIP6.data());
                    if (IsPrivateIP(m_sHubIP6.data()))
                    {
                        SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_AUTO_REG), false);
                    }
                }

                LogInfo("{}", msg);
            }
            else if (m_sHubIP6[0] != '\0')
            {
                LogInfo("*** {}", m_sHubIP6.data());
                if (IsPrivateIP(m_sHubIP6.data()))
                {
                    SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_AUTO_REG), false);
                }
            }

            freeaddrinfo(res);
        }
        else
        {
            SafeStrCopy(m_sHubIP, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)].c_str());
            if (IsPrivateIP(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)].c_str()))
            {
                SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_AUTO_REG), false);
            }
        }
    }
    else
    {
        if (!SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_IPV4_ADDRESS)].empty())
        {
            SafeStrCopy(m_sHubIP, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_IPV4_ADDRESS)].c_str());
        }
        else
        {
            m_sHubIP[0] = '\0';
        }

        if (!SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_IPV6_ADDRESS)].empty())
        {
            SafeStrCopy(m_sHubIP6, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_IPV6_ADDRESS)].c_str());
        }
        else
        {
            m_sHubIP6[0] = '\0';
        }

        if (isIP(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)].c_str()))
        {
            if (IsPrivateIP(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)].c_str()))
            {
                SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_AUTO_REG), false);
            }
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
