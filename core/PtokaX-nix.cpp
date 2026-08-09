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
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "IP2Country.h"
#include "Log.h"
#include "LuaScript.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ResNickManager.h"
#include "ServerManager.h"
#include "ServerThread.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"

#include <fmt/format.h>
#include <execinfo.h>
#include <csignal>
#include <dirent.h>
#include <sstream>
#include <cxxabi.h>
#include <exception>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <map>
#include <civetweb.h>
#include <lua.hpp>
#include <spdlog/spdlog.h>

using std::endl;
using std::ostringstream;

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

//---------------------------------------------------------------------------
namespace {
bool g_bTerminatedBySignal = false;
int g_iSignal = 0;
pthread_t g_test_port_thread{};
} // namespace
//---------------------------------------------------------------------------
static void SigHandler(int iSig)
{
    g_bTerminatedBySignal = true;

    g_iSignal = iSig;

    // restore to default...
    struct sigaction sigact;
    sigact.sa_handler = SIG_DFL;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    sigaction(iSig, &sigact, nullptr);
}

//---------------------------------------------------------------------------
static void showUsage()
{
    printf("Usage: PtokaX [-d] [-v] [-m] [-c configdir] [-p pidfile]\n\n"
           "Options:\n"
           "\t-d\t\t- run as daemon.\n"
           "\t-c configdir\t- absolute path to PtokaX configuration directory.\n"
           "\t-p pidfile\t-p <pidfile>	- path with filename where PtokaX PID will be stored.\n"
           "\t-v\t\t- show PtokaX version with build date and time.\n"
           "\t-m\t\t- show PtokaX configuration menu.\n"
           "\t-use-log\t\t- Use log.\n");
}
//---------------------------------------------------------------------------
extern "C" void* run_fly_server_test_port(void*);
//---------------------------------------------------------------------------
static constexpr const char* g_sCrashDir = "/app/crashes"; // NOLINT(modernize-avoid-c-arrays)
//---------------------------------------------------------------------------
// Async-signal-safe write to stderr. No locks, no heap, no stdio.
static void WriteStr(int fd, const char* s)
{
    const size_t len = __builtin_strlen(s);
    [[maybe_unused]] const ssize_t written = write(fd, s, len); // NOLINT
}
//---------------------------------------------------------------------------
// Signal handler: only async-signal-safe functions allowed.
// backtrace() + backtrace_symbols_fd() are async-signal-safe on glibc.
static void SignalHandler(int iSig)
{
    constexpr int STDERR = 2;
    WriteStr(STDERR, "\n=== CRASH (signal ");
    char sigbuf[4] = {}; // NOLINT(modernize-avoid-c-arrays)
    sigbuf[0] = '0' + iSig / 10;
    sigbuf[1] = '0' + iSig % 10;
    [[maybe_unused]] const ssize_t written = write(STDERR, sigbuf, 2); // NOLINT
    WriteStr(STDERR, ") STACK TRACE ===\n");

    void* addrlist[64]; // NOLINT(modernize-avoid-c-arrays)
    const int addrlen = backtrace(addrlist, 64);
    backtrace_symbols_fd(addrlist, addrlen, STDERR);

    WriteStr(STDERR, "=== END STACK TRACE ===\n");

    // Re-raise with default handler for proper core dump
    struct sigaction sa {};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(iSig, &sa, nullptr);
    raise(iSig);
}
//---------------------------------------------------------------------------
// Full crash report for terminate handler (not called from signal context).
static void DoCrashReport(const char* sReason)
{
    void* addrlist[64]; // NOLINT(modernize-avoid-c-arrays)
    const int addrlen = backtrace(addrlist, 64);

    if (addrlen == 0)
    {
        std::cerr << "Stack backtrace is empty, possibly corrupt" << endl;
        return;
    }

    std::unique_ptr<char*, decltype(&free)> symbollist(backtrace_symbols(addrlist, addrlen), free);

    // Demangled backtrace
    std::vector<char> funcname(256);
    ostringstream bt;

    for (int i = 1; i < addrlen; i++)
    {
        char* sym = symbollist ? symbollist.get()[i] : nullptr;
        if (!sym)
        {
            bt << "  #" << i << " <unknown>\n";
            continue;
        }

        char *begin_name = nullptr, *begin_offset = nullptr, *end_offset = nullptr;

        for (char* p = sym; *p != '\0'; ++p)
        {
            if (*p == '(')
            {
                begin_name = p;
            }
            else if (*p == '+')
            {
                begin_offset = p;
            }
            else if ((*p == ')') && begin_offset)
            {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset && (begin_name < begin_offset))
        {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';
            int status = 0;
            size_t szFuncNameSize = funcname.size();
            char* ret = abi::__cxa_demangle(begin_name, funcname.data(), &szFuncNameSize, &status);

            bt << "  #" << i << " ";
            if (status == 0 && ret)
            {
                bt << ret << " +" << begin_offset << "\n";
            }
            else
            {
                bt << begin_name << "()" << " +" << begin_offset << "\n";
            }
        }
        else
        {
            bt << "  #" << i << " " << sym << "\n";
        }
    }

    // addr2line for file:line resolution
    ostringstream addr2line_out;
    addr2line_out << "\n--- addr2line file:line ---\n";
    for (int i = 1; i < addrlen; i++)
    {
        std::array<char, 512> cmd = {};
        snprintf(cmd.data(), cmd.size(), "addr2line -e /usr/local/bin/PtokaX -f -C %p 2>/dev/null", addrlist[i]);
        PipePtr pipe(popen(cmd.data(), "r"));
        if (pipe)
        {
            std::array<char, 256> line = {};
            std::string func_name, file_line;
            if (fgets(line.data(), static_cast<int>(line.size()), pipe.get()))
            {
                func_name = line.data();
                func_name.erase(func_name.find_last_not_of("\n\r") + 1);
            }
            if (fgets(line.data(), static_cast<int>(line.size()), pipe.get()))
            {
                file_line = line.data();
                file_line.erase(file_line.find_last_not_of("\n\r") + 1);
            }
            if (!func_name.empty() && !file_line.empty())
            {
                addr2line_out << "  " << i << ": " << func_name << " at " << file_line << "\n";
            }
        }
    }

    const auto t = std::time(nullptr);
    const auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d-%H%M%S");
    const auto ts = oss.str();

    std::ostringstream report;
    report << "=== CRASH REPORT ===\n"
           << "Reason: " << sReason << "\n"
           << "Time:   " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n"
           << "PID:    " << getpid() << "\n\n"
           << "--- backtrace ---\n"
           << bt.str()
           << addr2line_out.str()
           << "=== END CRASH REPORT ===\n";

    const std::string& report_str = report.str();

    // Write to stderr
    std::cerr << "\n" << report_str << endl;

    // Write to crash directory
    mkdir(g_sCrashDir, 0755); // NOLINT — create dir if it doesn't exist
    const std::string log_path = std::string(g_sCrashDir) + "/crash_" + ts + ".log";
    std::ofstream out_file(log_path, std::ios_base::out | std::ios::binary);
    if (out_file.is_open())
    {
        out_file << report_str;
        out_file.close();
        std::cerr << "Crash report saved to " << log_path << endl;
    }
    else
    {
        std::cerr << "Failed to save crash report to " << log_path << endl;
    }
}

static void myTerminateHandler()
{
    try
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (const std::exception& e)
    {
        DoCrashReport(e.what());
    }
    catch (...)
    {
        DoCrashReport("uncaught non-std exception");
    }
    std::abort();
}
//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGBUS, SignalHandler);
    signal(SIGFPE, SignalHandler);
    std::set_terminate(myTerminateHandler);

    bool bSetup = false;

    char* sPidFile = nullptr;

    for (int i = 1; i < argc; i++)
    {
        if (strcasecmp(argv[i], "-d") == 0)
        {
            ServerManager::m_bDaemon = true;
        }
        else if (strcasecmp(argv[i], "-c") == 0)
        {
            if (++i == argc)
            {
                printf("Missing config directory!\n");
                return EXIT_FAILURE;
            }

            if (argv[i][0] != '/')
            {
                printf("Config directory must be absolute path!\n");
                return EXIT_FAILURE;
            }

            const size_t szLen = strlen(argv[i]);
            if (argv[i][szLen - 1] == '/')
            {
                ServerManager::m_sPath = std::string(argv[i], szLen - 1);
            }
            else
            {
                ServerManager::m_sPath = std::string(argv[i], szLen);
            }

            if (!DirExist(ServerManager::m_sPath.c_str()))
            {
                if (mkdir(ServerManager::m_sPath.c_str(), 0755) == -1)
                {
                    if (ServerManager::m_bDaemon)
                    {
                        spdlog::error("Config directory not exist and can't be created!");
                    }
                    else
                    {
                        printf("Config directory not exist and can't be created!");
                    }
                }
            }
        }
        else if (strcasecmp(argv[i], "-v") == 0)
        {
            printf("%s built on %s %s\n", g_sPtokaXTitle, __DATE__, __TIME__);
            return EXIT_SUCCESS;
        }
        else if (strcasecmp(argv[i], "-h") == 0)
        {
            showUsage();
            return EXIT_SUCCESS;
        }
        else if (strcasecmp(argv[i], "-p") == 0)
        {
            if (++i == argc)
            {
                printf("Missing pid file!\n");
                return EXIT_FAILURE;
            }

            sPidFile = argv[i];
        }
        else if (strcasecmp(argv[i], "/generatexmllanguage") == 0)
        {
            LanguageManager::GenerateXmlExample();
            return EXIT_SUCCESS;
        }
        else if (strcasecmp(argv[i], "-m") == 0)
        {
            bSetup = true;
        }
        else
        {
            printf("Unknown parameter %s.\n", argv[i]);
            showUsage();
            return EXIT_SUCCESS;
        }
    }

    if (ServerManager::m_sPath.empty())
    {
        char* home;
        std::array<char, PATH_MAX> curdir = {};
        if (ServerManager::m_bDaemon && (home = getenv("HOME")))
        {
            ServerManager::m_sPath = std::string(home) + "/.PtokaX";

            if (!DirExist(ServerManager::m_sPath.c_str()))
            {
                if (mkdir(ServerManager::m_sPath.c_str(), 0755) == -1)
                {
                    spdlog::error("Config directory not exist and can't be created!");
                }
            }
        }
        else if (getcwd(curdir.data(), curdir.size()))
        {
            ServerManager::m_sPath = curdir.data();
        }
        else
        {
            ServerManager::m_sPath = ".";
        }
    }

    PXLog::Init(ServerManager::m_sPath);

    if (bSetup)
    {
        ServerManager::Initialize();

        ServerManager::CommandLineSetup();

        ServerManager::FinalClose();

        return EXIT_SUCCESS;
    }

    if (ServerManager::m_bDaemon)
    {
        printf("Starting %s as daemon using %s as config directory.\n", g_sPtokaXTitle, ServerManager::m_sPath.c_str());

        pid_t pid1 = fork();
        if (pid1 == -1)
        {
            spdlog::error("First fork failed!");
            return EXIT_FAILURE;
        }
        if (pid1 > 0)
        {
            return EXIT_SUCCESS;
        }

        if (setsid() == -1)
        {
            spdlog::error("Setsid failed!");
            return EXIT_FAILURE;
        }

        pid_t pid2 = fork();
        if (pid2 == -1)
        {
            spdlog::error("Second fork failed!");
            return EXIT_FAILURE;
        }
        if (pid2 > 0)
        {
            return EXIT_SUCCESS;
        }

        if (sPidFile)
        {
            const std::string sPid = fmt::format("{}\n", static_cast<long>(getpid()));
            if (!WriteWholeFile(sPidFile, sPid))
            {
                spdlog::error("Failed to write PID file: {}", sPidFile);
            }
        }

        if (chdir("/") == -1)
        {
            spdlog::error("chdir failed!");
            return EXIT_FAILURE;
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        int fdNull = open("/dev/null", O_RDWR);
        if (fdNull == -1)
        {
            spdlog::error("Failed to open /dev/null!");
            return EXIT_FAILURE;
        }
        if (fdNull != STDIN_FILENO)
        {
            close(fdNull);
        }

        if (dup(0) == -1)
        {
            spdlog::error("First dup(0) failed!");
            return EXIT_FAILURE;
        }

        if (dup(0) == -1)
        {
            spdlog::error("Second dup(0) failed!");
            return EXIT_FAILURE;
        }
    }

    sigset_t sst;
    sigemptyset(&sst);
    sigaddset(&sst, SIGPIPE);
    sigaddset(&sst, SIGURG);
    sigaddset(&sst, SIGALRM);

    if (ServerManager::m_bDaemon)
    {
        sigaddset(&sst, SIGHUP);
    }

    pthread_sigmask(SIG_BLOCK, &sst, nullptr);

    struct sigaction sigact;
    sigact.sa_handler = SigHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    if (sigaction(SIGINT, &sigact, nullptr) == -1)
    {
        LogDbg("[ERR] Cannot create sigaction SIGINT in main");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sigact, nullptr) == -1)
    {
        LogDbg("[ERR] Cannot create sigaction SIGTERM in main");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGQUIT, &sigact, nullptr) == -1)
    {
        LogDbg("[ERR] Cannot create sigaction SIGQUIT in main");
        exit(EXIT_FAILURE);
    }

    if (!ServerManager::m_bDaemon && sigaction(SIGHUP, &sigact, nullptr) == -1)
    {
        LogDbg("[ERR] Cannot create sigaction SIGHUP in main");
        exit(EXIT_FAILURE);
    }

    ServerManager::Initialize();

    if (!ServerManager::Start())
    {
        if (!ServerManager::m_bDaemon)
        {
            printf("Server start failed!\n");
        }
        else
        {
            spdlog::error("Server start failed!");
        }
        return EXIT_FAILURE;
    }

    // Start the test-port (37015) server on a joinable thread so we can wait for
    // all its logging workers before spdlog is torn down at exit.
    if (pthread_create(&g_test_port_thread, nullptr, run_fly_server_test_port, nullptr) != 0)
    {
        spdlog::error("Failed to start test-port server thread!");
    }

    if (!ServerManager::m_bDaemon)
    {
        printf("%s running...\n", g_sPtokaXTitle);
    }

    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 100000000;
    using namespace prometheus;

    while (true)
    {
        ServiceLoop::m_Ptr->Looper();

        if (ServerManager::m_bServerTerminated)
        {
            break;
        }

        if (g_bTerminatedBySignal)
        {
            extern std::atomic<int> g_test_port_exit_flag;
            g_test_port_exit_flag = 1;
            if (ServerManager::m_bIsClose)
            {
                break;
            }

            std::string str("Received signal ");

            switch (g_iSignal)
            {
            case SIGINT:
                str += "SIGINT";
                break;
            case SIGTERM:
                str += "SIGTERM";
                break;
            case SIGQUIT:
                str += "SIGQUIT";
                break;
            case SIGHUP:
                str += "SIGHUP";
                break;
            default:
                str += std::to_string(g_iSignal);
                break;
            }

            str += " ending...";

            LogInfo("{}", str);

            ServerManager::m_bIsClose = true;
            ServerManager::Stop();

            // tell the scripts about the end
            ScriptManager::m_Ptr->OnExit();

            // send last possible global data
            GlobalDataQueue::m_Ptr->SendFinalQueue();

            ServerManager::FinalStop(true);

            break;
        }
        nanosleep(&sleeptime, nullptr);
        static int g_count_rusage = 0;
        if ((++g_count_rusage % 10) == 0)
        {
            struct rusage ru;
            if (getrusage(RUSAGE_SELF, &ru) == 0)
            {
                struct // NOLINT(modernize-avoid-c-arrays)
                {
                    const char* name;
                    long val;
                } ruEntries[] = { // NOLINT(modernize-avoid-c-arrays)
                    {"ru_maxrss", ru.ru_maxrss},
                    {"ru_ixrss", ru.ru_ixrss},
                    {"ru_idrss", ru.ru_idrss},
                    {"ru_isrss", ru.ru_isrss},
                    {"ru_nswap", ru.ru_nswap},
                    {"ru_inblock", ru.ru_inblock},
                    {"ru_oublock", ru.ru_oublock},
                    {"ru_majflt", ru.ru_majflt},
                    {"ru_minflt", ru.ru_minflt},
                    {"ru_nvcsw", ru.ru_nvcsw},
                    {"ru_nivcsw", ru.ru_nivcsw},
                    {"ru_nsignals", ru.ru_nsignals},
                    {"ru_utime", ru.ru_utime.tv_sec},
                    {"ru_stime", ru.ru_stime.tv_sec},
                };
                for (const auto& e : ruEntries)
                {
                    GlobalDataQueue::m_Ptr->PrometheusRusageValue(e.name, e.val);
                }
            }
            GlobalDataQueue::m_Ptr->PrometheusUsersLoggedIn(ServerManager::m_ui32Logged);
            GlobalDataQueue::m_Ptr->PrometheusJoinsTotal(ServerManager::m_ui32Joins);
            GlobalDataQueue::m_Ptr->PrometheusPartsTotal(ServerManager::m_ui32Parts);
            GlobalDataQueue::m_Ptr->PrometheusUsersPeak(ServerManager::m_ui32Peak);
            GlobalDataQueue::m_Ptr->PrometheusShareBytes(static_cast<double>(ServerManager::m_ui64TotalShare));
            GlobalDataQueue::m_Ptr->PrometheusBandwidth("read", ServerManager::m_ui32AverageBytesRead);
            GlobalDataQueue::m_Ptr->PrometheusBandwidth("write", ServerManager::m_ui32AverageBytesSent);
            GlobalDataQueue::m_Ptr->PrometheusUsersConnecting(ServerManager::m_ui32Joins - ServerManager::m_ui32Logged - ServerManager::m_ui32Parts);
            GlobalDataQueue::m_Ptr->PrometheusSendRests(!ServiceLoop::m_Ptr ? 0 : ServiceLoop::m_Ptr->m_ui32LastSendRest);
            GlobalDataQueue::m_Ptr->PrometheusRecvRests(!ServiceLoop::m_Ptr ? 0 : ServiceLoop::m_Ptr->m_ui32LastRecvRest);
            GlobalDataQueue::m_Ptr->PrometheusCompressionSavedBytes(static_cast<double>(ServerManager::m_ui64BytesSentSaved));
            GlobalDataQueue::m_Ptr->PrometheusScriptsCount(
                !ScriptManager::m_Ptr ? 0 : static_cast<uint8_t>(ScriptManager::m_Ptr->m_ppScriptTable.size()));
            GlobalDataQueue::m_Ptr->PrometheusBotsCount(!ScriptManager::m_Ptr ? 0 : ScriptManager::m_Ptr->m_ui8BotsCount);
            GlobalDataQueue::m_Ptr->PrometheusProfilesCount(!ProfileManager::m_Ptr ? 0 : ProfileManager::m_Ptr->m_ui16ProfileCount);
            GlobalDataQueue::m_Ptr->PrometheusActiveSearches(!Users::m_Ptr ? 0 : Users::m_Ptr->m_ui16ActSearchs);
            GlobalDataQueue::m_Ptr->PrometheusPassiveSearches(!Users::m_Ptr ? 0 : Users::m_Ptr->m_ui16PasSearchs);
            GlobalDataQueue::m_Ptr->PrometheusBuildInfo(g_sPtokaXTitle);

            if (DcCommands::m_Ptr)
            {
                const auto& dc = *DcCommands::m_Ptr;
                struct // NOLINT(modernize-avoid-c-arrays)
                {
                    const char* name;
                    uint32_t DcCommands::* field;
                } dcEntries[] = { // NOLINT(modernize-avoid-c-arrays)
                    {"chat", &DcCommands::m_ui32StatChat},
                    {"pm", &DcCommands::m_ui32StatCmdTo},
                    {"search", &DcCommands::m_ui32StatCmdSearch},
                    {"search_result", &DcCommands::m_ui32StatCmdSR},
                    {"myinfo", &DcCommands::m_ui32StatCmdMyInfo},
                    {"ctm", &DcCommands::m_ui32StatCmdConnectToMe},
                    {"rctm", &DcCommands::m_ui32StatCmdRevCTM},
                    {"validate", &DcCommands::m_ui32StatCmdValidate},
                    {"key", &DcCommands::m_ui32StatCmdKey},
                    {"version", &DcCommands::m_ui32StatCmdVersion},
                    {"supports", &DcCommands::m_ui32StatCmdSupports},
                    {"unknown", &DcCommands::m_ui32StatCmdUnknown},
                    {"opforcemove", &DcCommands::m_ui32StatCmdOpForceMove},
                    {"mypass", &DcCommands::m_ui32StatCmdMyPass},
                    {"getinfo", &DcCommands::m_ui32StatCmdGetInfo},
                    {"getnicklist", &DcCommands::m_ui32StatCmdGetNickList},
                    {"kick", &DcCommands::m_ui32StatCmdKick},
                    {"botinfo", &DcCommands::m_ui32StatBotINFO},
                    {"zpipe", &DcCommands::m_ui32StatZPipe},
                    {"multisearch", &DcCommands::m_ui32StatCmdMultiSearch},
                    {"multiconnecttome", &DcCommands::m_ui32StatCmdMultiConnectToMe},
                    {"close", &DcCommands::m_ui32StatCmdClose},
#ifdef USE_FLYLINKDC_EXT_JSON
                    {"extjson", &DcCommands::m_ui32StatCmdExtJSON},
#endif
                };
                for (const auto& e : dcEntries)
                {
                    GlobalDataQueue::m_Ptr->PrometheusDcCommandStat(e.name, dc.*(e.field));
                }
            }

            GlobalDataQueue::m_Ptr->PrometheusEventQueueDepth(!EventQueue::m_Ptr ? 0 : EventQueue::m_Ptr->m_ui32NormalDepth);

            // Per-command latency
            if (DcCommands::m_Ptr)
            {
                const auto& dc = *DcCommands::m_Ptr;
                struct // NOLINT(modernize-avoid-c-arrays)
                {
                    const char* name;
                    uint64_t DcCommands::* field;
                } latEntries[] = { // NOLINT(modernize-avoid-c-arrays)
                    {"chat", &DcCommands::m_ui64TimeCmdChat},
                    {"search", &DcCommands::m_ui64TimeCmdSearch},
                    {"myinfo", &DcCommands::m_ui64TimeCmdMyInfo},
                    {"pm", &DcCommands::m_ui64TimeCmdTo},
                    {"sr", &DcCommands::m_ui64TimeCmdSR},
                    {"key", &DcCommands::m_ui64TimeCmdKey},
                    {"validatenick", &DcCommands::m_ui64TimeCmdValidateNick},
                    {"ctm", &DcCommands::m_ui64TimeCmdConnectToMe},
                    {"rctm", &DcCommands::m_ui64TimeCmdRevConnectToMe},
                    {"supports", &DcCommands::m_ui64TimeCmdSupports},
                    {"version", &DcCommands::m_ui64TimeCmdVersion},
                    {"getnicklist", &DcCommands::m_ui64TimeCmdGetNickList},
                    {"getinfo", &DcCommands::m_ui64TimeCmdGetINFO},
                    {"close", &DcCommands::m_ui64TimeCmdClose},
                    {"kick", &DcCommands::m_ui64TimeCmdKick},
                    {"opforcemove", &DcCommands::m_ui64TimeCmdOpForceMove},
                    {"mypass", &DcCommands::m_ui64TimeCmdMyPass},
                    {"botinfo", &DcCommands::m_ui64TimeCmdBotINFO},
                    {"processcmds", &DcCommands::m_ui64TimeProcessCmds},
                    {"other", &DcCommands::m_ui64TimeCmdOther},
                };
                for (const auto& e : latEntries)
                {
                    GlobalDataQueue::m_Ptr->PrometheusCommandLatency(e.name, static_cast<double>(dc.*(e.field)));
                }
            }

            // Connections accepted/closed
            GlobalDataQueue::m_Ptr->PrometheusConnectionsAccepted(ServerManager::m_ui32ConnectionsAccepted);
            GlobalDataQueue::m_Ptr->PrometheusConnectionsClosed(ServerManager::m_ui32ConnectionsClosed);

#ifdef FLYLINKDC_USE_CPU_STAT
            GlobalDataQueue::m_Ptr->PrometheusCpuUsage(ServerManager::m_dCpuUsage);
#endif

            // Lua timers active
            {
                uint32_t timerCount = 0;
                if (ScriptManager::m_Ptr)
                {
                    timerCount = static_cast<uint32_t>(ScriptManager::m_Ptr->m_TimerList.size());
                }
                GlobalDataQueue::m_Ptr->PrometheusLuaTimersActive(timerCount);
            }

            // UDP debug subscribers
            GlobalDataQueue::m_Ptr->PrometheusUdpDebugSubscribers(!UdpDebug::m_Ptr ? 0 : UdpDebug::m_Ptr->GetSubscriberCount());

            // Send/Recv rests peak
            GlobalDataQueue::m_Ptr->PrometheusSendRestsPeak(!ServiceLoop::m_Ptr ? 0 : ServiceLoop::m_Ptr->m_ui32SendRestsPeak);
            GlobalDataQueue::m_Ptr->PrometheusRecvRestsPeak(!ServiceLoop::m_Ptr ? 0 : ServiceLoop::m_Ptr->m_ui32RecvRestsPeak);

            // Reserved nicks count
            GlobalDataQueue::m_Ptr->PrometheusReservedNicks(!ReservedNicksManager::m_Ptr ? 0 : ReservedNicksManager::m_Ptr->GetCount());

            // IP2Country ranges loaded
            GlobalDataQueue::m_Ptr->PrometheusIp2CountryRanges("ipv4", !IpP2Country::m_Ptr ? 0 : IpP2Country::m_Ptr->m_ui32Count);
            GlobalDataQueue::m_Ptr->PrometheusIp2CountryRanges("ipv6", !IpP2Country::m_Ptr ? 0 : IpP2Country::m_Ptr->m_ui32IPv6Count);

            // Server threads suspended
            for (const auto& st : ServerManager::m_Servers)
            {
                GlobalDataQueue::m_Ptr->PrometheusServerThreadSuspended(std::to_string(st->m_ui16Port).c_str(), st->m_bSuspended ? 1 : 0);
            }

            // Connection flood watchlist total
            GlobalDataQueue::m_Ptr->PrometheusConFloodWatchlist(ServerThread::GetTotalAntiFloodCount());

            // Global queue buffer memory
            if (GlobalDataQueue::m_Ptr)
            {
                GlobalDataQueue::m_Ptr->EmitGlobalQueueBufferMetrics();
            }

            // Users list buffer sizes
            if (Users::m_Ptr && GlobalDataQueue::m_Ptr)
            {
                GlobalDataQueue::m_Ptr->PrometheusUsersListBufferBytes("myinfo", Users::m_Ptr->m_ui32MyInfosLen);
                GlobalDataQueue::m_Ptr->PrometheusUsersListBufferBytes("zmyinfo", Users::m_Ptr->m_ui32ZMyInfosLen);
                GlobalDataQueue::m_Ptr->PrometheusUsersListBufferBytes("nicklist", Users::m_Ptr->m_ui32NickListLen);
                GlobalDataQueue::m_Ptr->PrometheusUsersListBufferBytes("oplist", Users::m_Ptr->m_ui32OpListLen);
                GlobalDataQueue::m_Ptr->PrometheusUsersListBufferBytes("userip", Users::m_Ptr->m_ui32UserIPListLen);
            }

            // Aggregate user send/recv buffer bytes
            if (GlobalDataQueue::m_Ptr)
            {
                uint64_t sendBuf = 0, recvBuf = 0;
                if (Users::m_Ptr)
                {
                    for (const auto& curPtr : Users::m_Ptr->m_UserList)
                    {
                        sendBuf += curPtr->m_ui32SendBufDataLen;
                        recvBuf += curPtr->m_ui32RecvBufDataLen;
                    }
                }
                GlobalDataQueue::m_Ptr->PrometheusUsersBufferBytes("send", static_cast<double>(sendBuf));
                GlobalDataQueue::m_Ptr->PrometheusUsersBufferBytes("recv", static_cast<double>(recvBuf));
            }
        }
        GlobalDataQueue::m_Ptr->m_metrics.gauge_set("flylinkdc_hub_connection_flood_total", static_cast<double>(ServerThread::m_ui32ConnectionFloodCount));
        if (DcCommands::m_Ptr)
        {
            GlobalDataQueue::m_Ptr->m_metrics.gauge_set("flylinkdc_hub_protocol_errors_total", DcCommands::m_Ptr->m_ui32StatBadState, {{"error", "bad_state"}});
            GlobalDataQueue::m_Ptr->m_metrics.gauge_set(
                "flylinkdc_hub_protocol_errors_total", DcCommands::m_Ptr->m_ui32StatGarbageData, {{"error", "garbage_data"}});
            GlobalDataQueue::m_Ptr->m_metrics.gauge_set(
                "flylinkdc_hub_protocol_errors_total", DcCommands::m_Ptr->m_ui32StatNickSpoofing, {{"error", "nick_spoofing"}});
            GlobalDataQueue::m_Ptr->m_metrics.gauge_set(
                "flylinkdc_hub_protocol_errors_total", DcCommands::m_Ptr->m_ui32StatBruteforce, {{"error", "bruteforce"}});
            GlobalDataQueue::m_Ptr->m_metrics.gauge_set("flylinkdc_hub_protocol_errors_total", DcCommands::m_Ptr->m_ui32StatFlood, {{"error", "flood"}});
            GlobalDataQueue::m_Ptr->m_metrics.gauge_set(
                "flylinkdc_hub_protocol_errors_total", DcCommands::m_Ptr->m_ui32StatInvalidJson, {{"error", "invalid_json"}});
            GlobalDataQueue::m_Ptr->m_metrics.gauge_set(
                "flylinkdc_hub_protocol_errors_total", DcCommands::m_Ptr->m_ui32StatUnknownCmd, {{"error", "unknown_cmd"}});
        }
        // Test port (37015) metrics
        {
            extern unsigned long long g_count_query;
            extern unsigned long long g_sum_out_size;
            extern unsigned long long g_sum_in_size;
            extern unsigned long long g_z_sum_out_size;
            extern unsigned long long g_z_sum_in_size;
            GlobalDataQueue::m_Ptr->PrometheusTestPortQueries(static_cast<double>(g_count_query));
            GlobalDataQueue::m_Ptr->PrometheusTestPortBytesIn(static_cast<double>(g_sum_in_size));
            GlobalDataQueue::m_Ptr->PrometheusTestPortBytesOut(static_cast<double>(g_sum_out_size));
            GlobalDataQueue::m_Ptr->PrometheusTestPortCompressedBytesIn(static_cast<double>(g_z_sum_in_size));
            GlobalDataQueue::m_Ptr->PrometheusTestPortCompressedBytesOut(static_cast<double>(g_z_sum_out_size));
        }
        static int g_count_slow = 0;
        if ((++g_count_slow % 600) == 0)
        {
            struct stat st;
            if (stat((ServerManager::m_sPath + "/cfg/RegisteredUsers.pxb").c_str(), &st) == 0)
            {
                GlobalDataQueue::m_Ptr->PrometheusSqliteSizeBytes(static_cast<double>(st.st_size));
            }
            const char* stateNames[] = {"socket_accepted", // NOLINT(modernize-avoid-c-arrays)
                                        "key_or_sup",
                                        "validate",
                                        "version_or_mypass",
                                        "getnicklist_or_myinfo",
                                        "ipv4_check",
                                        "addme",
                                        "addme_1loop",
                                        "addme_2loop",
                                        "added",
                                        "closing",
                                        "remme"};
            std::array<uint32_t, 12> stateCounts = {};
            uint32_t ops = 0, hidden = 0, gagged = 0, active = 0, ipv6 = 0, sharing = 0;
            uint64_t totalSlots = 0;
            std::map<int, uint32_t> profileCounts;
            for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
            {
                User* curUser = curUserPtr.get();
                stateCounts[std::to_underlying(curUser->m_ui8State)]++;
                if (curUser->m_ui8State != User::UserStates::STATE_ADDED)
                {
                    continue;
                }
                if ((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR)
                {
                    ops++;
                }
                if ((curUser->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN)
                {
                    hidden++;
                }
                if ((curUser->m_ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED)
                {
                    gagged++;
                }
                if ((curUser->m_ui32BoolBits & (User::BIT_IPV4_ACTIVE | User::BIT_IPV6_ACTIVE)) != 0)
                {
                    active++;
                }
                if ((curUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
                {
                    ipv6++;
                }
                if (curUser->m_ui64SharedSize > 0)
                {
                    sharing++;
                }
                totalSlots += curUser->m_ui32Slots;
                profileCounts[curUser->m_i32Profile]++;
            }
            for (int si = 0; si < 12; si++)
            {
                GlobalDataQueue::m_Ptr->PrometheusUsersByState(stateNames[si], stateCounts[si]);
            }
            GlobalDataQueue::m_Ptr->PrometheusOperatorsOnline(ops);
            GlobalDataQueue::m_Ptr->PrometheusUsersHidden(hidden);
            GlobalDataQueue::m_Ptr->PrometheusUsersGagged(gagged);
            GlobalDataQueue::m_Ptr->PrometheusUsersActive(active);
            GlobalDataQueue::m_Ptr->PrometheusUsersIpv6(ipv6);
            GlobalDataQueue::m_Ptr->PrometheusUsersSharing(sharing);
            GlobalDataQueue::m_Ptr->PrometheusTotalSlots(static_cast<double>(totalSlots));
            for (const auto& [profile, count] : profileCounts)
            {
                GlobalDataQueue::m_Ptr->PrometheusUsersByProfile(profile, count);
            }

            const auto reg_count = static_cast<uint32_t>(RegManager::m_Ptr->m_RegList.size());
            GlobalDataQueue::m_Ptr->PrometheusUsersRegistered(reg_count);

            const auto temp_bans = static_cast<uint32_t>(BanManager::m_Ptr->m_TempBanList.size());
            const auto perm_bans = static_cast<uint32_t>(BanManager::m_Ptr->m_PermBanList.size());
            const auto range_bans = static_cast<uint32_t>(BanManager::m_Ptr->m_RangeBanList.size());
            GlobalDataQueue::m_Ptr->PrometheusBansCount("temp", temp_bans);
            GlobalDataQueue::m_Ptr->PrometheusBansCount("perm", perm_bans);
            GlobalDataQueue::m_Ptr->PrometheusBansCount("range", range_bans);

            // Lua per-script stats
            if (ScriptManager::m_Ptr)
            {
                uint32_t timersByScript = 0;
                for (auto* s : ScriptManager::m_Ptr->m_ppScriptTable)
                {
                    if (s && s->m_pLua)
                    {
                        int kb = static_cast<int>(lua_gc(s->m_pLua, LUA_GCCOUNT, 0));
                        GlobalDataQueue::m_Ptr->PrometheusLuaMemoryBytes(s->m_sName.c_str(), kb);

                        GlobalDataQueue::m_Ptr->PrometheusLuaCallCount(s->m_sName.c_str(), s->m_ui32LuaCallCount);
                        GlobalDataQueue::m_Ptr->PrometheusLuaTimeNsec(s->m_sName.c_str(), static_cast<double>(s->m_ui64LuaTimeNsec));

                        // Lua GC pause time
                        struct timespec gc_ts_b, gc_ts_a;
                        clock_gettime(CLOCK_MONOTONIC, &gc_ts_b);
                        lua_gc(s->m_pLua, LUA_GCCOUNT, 0);
                        clock_gettime(CLOCK_MONOTONIC, &gc_ts_a);
                        double gc_nsec =
                            static_cast<double>(gc_ts_a.tv_sec - gc_ts_b.tv_sec) * 1000000000.0 + static_cast<double>(gc_ts_a.tv_nsec - gc_ts_b.tv_nsec);
                        GlobalDataQueue::m_Ptr->PrometheusLuaGcPauseNsec(s->m_sName.c_str(), gc_nsec);

                        // Count timers for this script
                        timersByScript = 0;
                        for (const auto& t : ScriptManager::m_Ptr->m_TimerList)
                        {
                            if (t->m_pLua == s->m_pLua)
                                timersByScript++;
                        }
                        GlobalDataQueue::m_Ptr->PrometheusLuaTimersPerScript(s->m_sName.c_str(), timersByScript);
                    }
                }
            }

            // Pending connections (sum of non-ADDED states)
            {
                uint32_t pending = 0;
                if (Users::m_Ptr)
                {
                    for (const auto& curPtr : Users::m_Ptr->m_UserList)
                    {
                        if (curPtr->m_ui8State != User::UserStates::STATE_ADDED)
                        {
                            pending++;
                        }
                    }
                }
                GlobalDataQueue::m_Ptr->PrometheusPendingConnections(pending);
            }

            // File descriptor count
            {
                uint32_t fdCount = 0;
                DIR* fdDir = opendir("/proc/self/fd");
                if (fdDir)
                {
                    struct dirent* entry;
                    while ((entry = readdir(fdDir)))
                    {
                        if (entry->d_name[0] != '.')
                        {
                            fdCount++;
                        }
                    }
                    closedir(fdDir);
                }
                GlobalDataQueue::m_Ptr->PrometheusFdCount(fdCount);
            }

            // Config reload timestamp
            GlobalDataQueue::m_Ptr->PrometheusConfigReloadTime(
                !SettingManager::m_Ptr ? 0 : static_cast<double>(SettingManager::m_Ptr->m_tConfigLoadTime));

            // Per-IP max connections
            GlobalDataQueue::m_Ptr->PrometheusIpSpread(!HashManager::m_Ptr ? 0 : HashManager::m_Ptr->GetMaxIpCount());
        }
    }

    // Stop the test-port server thread and wait for it to finish all its logging
    // workers, so the spdlog logger is not used-after-free at process exit.
    {
        extern std::atomic<int> g_test_port_exit_flag;
        g_test_port_exit_flag = 1;
        if (g_test_port_thread != 0)
        {
            pthread_join(g_test_port_thread, nullptr);
        }
    }
    PXLog::Shutdown();

    if (!ServerManager::m_bDaemon)
    {
        printf("%s ending...\n", g_sPtokaXTitle);
    }
    else if (sPidFile)
    {
        unlink(sPidFile);
    }

    return EXIT_SUCCESS;
}
//---------------------------------------------------------------------------
