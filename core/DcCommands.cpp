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
#include "DcCommands.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
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
#include <span>
#ifdef USE_FLYLINKDC_EXT_JSON
#include <nlohmann/json.hpp>
#include <algorithm>
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "DeFlood.h"
#include "HubCommands.h"
#include "IP2Country.h"
#include "ResNickManager.h"
#include "TextFileManager.h"
//---------------------------------------------------------------------------

#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#endif

//---------------------------------------------------------------------------
std::unique_ptr<DcCommands> DcCommands::m_Ptr;
//---------------------------------------------------------------------------

// Table-driven bad-state command dispatch.
// Each entry: suffix to match after sCommand[0]+offset, suffix length, required command length (0=any), stat counter, command name for logging.
struct BadStateCmd
{
    const char* suffix;
    uint16_t suffixLen;
    uint16_t exactCmdLen; // 0 = no exact length check
    uint32_t DcCommands::* statField;
    const char* cmdName;
};

// Matches suffix starting at sCommand[offset] (default offset=2 for '$' + first char)
static constexpr BadStateCmd g_BadStateCmds[] = { // NOLINT(modernize-avoid-c-arrays)
    {.suffix = "onnectToMe ", .suffixLen = 11, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdConnectToMe, .cmdName = "$ConnectToMe"},
    {.suffix = "lose ", .suffixLen = 5, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdClose, .cmdName = "$Close"},
    {.suffix = "etINFO ", .suffixLen = 7, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdGetInfo, .cmdName = "$GetINFO"},
    {.suffix = "etNickList", .suffixLen = 10, .exactCmdLen = 13, .statField = &DcCommands::m_ui32StatCmdGetNickList, .cmdName = "$GetNickList"},
    {.suffix = "ey ", .suffixLen = 3, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdKey, .cmdName = "$Key"},
    {.suffix = "ick ", .suffixLen = 4, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdKick, .cmdName = "$Kick"},
    {.suffix = "ultiConnectToMe ", .suffixLen = 16, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdMultiConnectToMe, .cmdName = "$MultiConnectToMe"},
    {.suffix = "ultiSearch ", .suffixLen = 11, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdMultiSearch, .cmdName = "$MultiSearch"},
    {.suffix = "yINFO $ALL ", .suffixLen = 11, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdMyInfo, .cmdName = "$MyINFO"},
    {.suffix = "yPass ", .suffixLen = 6, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdMyPass, .cmdName = "$MyPass"},
    {.suffix = "pForceMove $Who:", .suffixLen = 16, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdOpForceMove, .cmdName = "$OpForceMove"},
    {.suffix = "evConnectToMe ", .suffixLen = 14, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdRevCTM, .cmdName = "$RevConnectToMe"},
    {.suffix = "o: ", .suffixLen = 3, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdTo, .cmdName = "$To"},
    {.suffix = "alidateNick ", .suffixLen = 12, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdValidate, .cmdName = "$ValidateNick"},
    {.suffix = "ersion ", .suffixLen = 7, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdVersion, .cmdName = "$Version"},
};

// Second-level entries for commands starting with "$S*" (offset=3 after "$S")
static constexpr BadStateCmd g_BadStateCmdsS[] = { // NOLINT(modernize-avoid-c-arrays)
    {.suffix = "earch ", .suffixLen = 6, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdSearch, .cmdName = "$Search"},
    {.suffix = "R ", .suffixLen = 2, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdSR, .cmdName = "$SR"},
    {.suffix = "upports ", .suffixLen = 8, .exactCmdLen = 0, .statField = &DcCommands::m_ui32StatCmdSupports, .cmdName = "$Supports"},
};

// Try to match a bad-state command from the table. Returns true if matched (and user was closed).
bool DcCommands::TryBadStateClose(DcCommand* pDcCommand, const void* pRawTable, size_t szCount, uint8_t ui8Offset)
{
    const auto* pTable = static_cast<const BadStateCmd*>(pRawTable);
    const char* sCmd = pDcCommand->m_sCommand;
    for (const auto& entry : std::span<const BadStateCmd>(pTable, szCount))
    {
        if (memcmp(sCmd + ui8Offset, entry.suffix, entry.suffixLen) == 0 && (entry.exactCmdLen == 0 || pDcCommand->m_ui32CommandLen == entry.exactCmdLen))
        {
            ++(this->*(entry.statField));

            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in %s %s (%s) - user closed.",
                                             std::to_underlying(pDcCommand->m_pUser->m_ui8State),
                                             entry.cmdName,
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data());
            m_ui32StatBadState++;
            pDcCommand->m_pUser->Close();
            return true;
        }
    }
    return false;
}

DcCommands::PassBf::PassBf(const uint8_t* ui128Hash)
{
    m_ui128IpHash.init(ui128Hash);
}
//---------------------------------------------------------------------------

struct CmdTimer
{
    uint64_t& m_acc;
    struct timespec m_ts;
    CmdTimer(uint64_t& acc) : m_acc(acc)
    {
        clock_gettime(CLOCK_MONOTONIC, &m_ts);
    }
    ~CmdTimer()
    {
        struct timespec ts_end;
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        m_acc += (ts_end.tv_sec - m_ts.tv_sec) * 1000000000ULL + (ts_end.tv_nsec - m_ts.tv_nsec);
    }
};

// Whitelist of known DC protocol commands for the Prometheus "command" label.
// The command token is attacker-controlled, so without a bounded label set every
// distinct "$AAAA..." would allocate a new Counter in the registry → unbounded
// memory growth. Unknown commands are bucketed as "other".
constexpr std::string_view g_sKnownCommands[] = { // NOLINT(modernize-avoid-c-arrays)
    "$BotINFO",      "$Close",        "$ConnectToMe",    "$ExtJSON",       "$ExtJSON2",      "$ForceMove",
    "$GetINFO",      "$GetNickList",  "$GetPass",        "$Hello",         "$HubName",       "$HubURL",
    "$IP64",         "$IPv4",         "$Key",            "$Kick",          "$Lock",          "$LogedIn",
    "$MoveToMe",     "$MultiConnectToMe", "$MultiSearch", "$MyINFO",       "$MyNick",        "$MyPass",
    "$NickList",     "$OpForceMove",  "$OpList",         "$Pass",          "$Pipe",          "$Pipe0",
    "$QuickList",    "$Quit",         "$R",              "$RecvSearch",    "$RevConnectToMe", "$SR",
    "$Search",       "$Supports",     "$TLS2",           "$To",            "$UGetINFO",      "$UGetNickList",
    "$UserCommand",  "$UserIP2",      "$ValidateNick",   "$Version",       "$ZOff",          "$ZOn",
};

[[nodiscard]] auto GetCommandMetricLabel(std::string_view sCmd) -> std::string
{
    for (const auto s : g_sKnownCommands)
    {
        if (sCmd == s)
        {
            return std::string(s);
        }
    }
    return "other";
}

//---------------------------------------------------------------------------

DcCommands::~DcCommands() = default;
//---------------------------------------------------------------------------

// Process DC data form User
void DcCommands::PreProcessData(DcCommand* pDcCommand)
{
#ifdef USE_FLYLINKDC_EXT_JSON
    bool bCheck = true;
#endif

    // micro spam
    if (pDcCommand->m_ui32CommandLen < 5) [[unlikely]]
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Garbage DATA from %s (%s) -> %s", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_sCommand);

        m_ui32StatGarbageData++;
        pDcCommand->m_pUser->Close();
        return;
    }

    if (pDcCommand->m_sCommand[0] == '$') [[likely]]
    {
        if (auto* const l_end_command = strchr(pDcCommand->m_sCommand, ' '))
        {
            const auto c = *l_end_command;
            *l_end_command = 0x00;
            GlobalDataQueue::m_Ptr->m_metrics.counter_inc("flylinkdc_hub_dc_commands_total", 1,
                                                          {{"command", GetCommandMetricLabel(pDcCommand->m_sCommand)}});
            *l_end_command = c;
        }
    }

    // Diagnostic: log every command with state (Search spam goes to debug)
    {
        const std::string_view sCmdPreview(pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen < 63 ? pDcCommand->m_ui32CommandLen : 63);
        if (pDcCommand->m_sCommand[0] == '$' && MatchBytes(pDcCommand->m_sCommand + 1, "Search "))
        {
            LogDbg("[PROTO] state={} cmd='{}' nick='{}' len={}",
                   std::to_underlying(pDcCommand->m_pUser->m_ui8State),
                   sCmdPreview,
                   pDcCommand->m_pUser->m_sNick,
                   pDcCommand->m_ui32CommandLen);
        }
        else
        {
            LogInfo("[PROTO] state={} cmd='{}' nick='{}' len={}",
                    std::to_underlying(pDcCommand->m_pUser->m_ui8State),
                    sCmdPreview,
                    pDcCommand->m_pUser->m_sNick,
                    pDcCommand->m_ui32CommandLen);
        }
    }

    switch (pDcCommand->m_pUser->m_ui8State) // NOLINT(bugprone-switch-missing-default-case) — default at line 1094
    {
    case User::UserStates::STATE_SOCKET_ACCEPTED:
        if (pDcCommand->m_sCommand[0] == '$')
        {
            if (MatchBytes(pDcCommand->m_sCommand + 1, "MyNick "))
            {
                MyNick(pDcCommand);
                return;
            }
        }
        break;
    case User::UserStates::STATE_KEY_OR_SUP:
        if (pDcCommand->m_sCommand[0] == '$')
        {
            if (MatchBytes(pDcCommand->m_sCommand + 1, "Supports "))
            {
                m_ui32StatCmdSupports++;
                Supports(pDcCommand);
                return;
            }
            if (MatchBytes(pDcCommand->m_sCommand + 1, "Key "))
            {
                m_ui32StatCmdKey++;
                Key(pDcCommand);
                return;
            }
            else if (MatchBytes(pDcCommand->m_sCommand + 1, "MyNick "))
            {
                MyNick(pDcCommand);
                return;
            }
        }
        break;
    case User::UserStates::STATE_VALIDATE:
    {
        if (pDcCommand->m_sCommand[0] == '$')
        {
            switch (pDcCommand->m_sCommand[1])
            {
            case 'K':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "ey "))
                {
                    m_ui32StatCmdKey++;
                    if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS))
                    {
                        Key(pDcCommand);
                    }
                    else
                    {
                        pDcCommand->m_pUser->FreeBuffer();
                    }

                    return;
                }
                break;
            case 'V':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "alidateNick "))
                {
                    m_ui32StatCmdValidate++;
                    ValidateNick(pDcCommand);
                    return;
                }
                break;
#ifdef USE_FLYLINKDC_EXT_JSON
            case 'E':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "xtJSON "))
                {
                    m_ui32StatCmdExtJSON++;
                }
                break;
#endif // USE_FLYLINKDC_EXT_JSON

            case 'M':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "yINFO $ALL "))
                {
                    m_ui32StatCmdMyInfo++;
                    if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
                    {
                        // bad state
                        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $MyINFO %s (%s) - user closed.",
                                                         std::to_underlying(pDcCommand->m_pUser->m_ui8State),
                                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                                         pDcCommand->m_pUser->m_sIP.data());

                        m_ui32StatBadState++;
                        pDcCommand->m_pUser->Close();
                        return;
                    }

                    if (!MyINFODeflood(pDcCommand))
                    {
                        return;
                    }

                    // PPK [ Strikes back ;) ] ... get nick from MyINFO
                    char* cTemp;
                    if (!(cTemp = strchr(pDcCommand->m_sCommand + 13, ' ')))
                    {
                        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Attempt to validate empty nick  from %s (%s) - user closed. (QuickList -> %s)",
                                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                                         pDcCommand->m_pUser->m_sIP.data(),
                                                         pDcCommand->m_sCommand);

                        pDcCommand->m_pUser->Close();
                        return;
                    }
                    // PPK ... one null please :)
                    cTemp[0] = '\0';

                    if (!ValidateUserNick(pDcCommand, pDcCommand->m_pUser, pDcCommand->m_sCommand + 13, (cTemp - pDcCommand->m_sCommand) - 13, false))
                        return;

                    cTemp[0] = ' ';

                    // 1st time MyINFO, user is being added to nicklist
                    if (!MyINFO(pDcCommand) || (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS ||
                        ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
                    {
                        return;
                    }

                    pDcCommand->m_pUser->AddMeOrIPv4Check();

                    return;
                }
                break;
            case 'G':
                if (pDcCommand->m_ui32CommandLen == 13 && MatchBytes(pDcCommand->m_sCommand + 2, "etNickList"))
                {
                    m_ui32StatCmdGetNickList++;
                    if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) &&
                        !((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
                    {
                        // bad state
                        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $GetNickList %s (%s) - user closed.",
                                                         std::to_underlying(pDcCommand->m_pUser->m_ui8State),
                                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                                         pDcCommand->m_pUser->m_sIP.data());

                        m_ui32StatBadState++;
                        pDcCommand->m_pUser->Close();
                        return;
                    }
                    (void)GetNickList(pDcCommand);
                    return;
                }
                break;
            default:
                break;
            }
        }
        break;
    }
    case User::UserStates::STATE_VERSION_OR_MYPASS:
    {
        if (pDcCommand->m_sCommand[0] == '$')
        {
            switch (pDcCommand->m_sCommand[1])
            {
            case 'V':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "ersion "))
                {
                    m_ui32StatCmdVersion++;
                    Version(pDcCommand);
                    return;
                }
                break;
            case 'G':
                if (pDcCommand->m_ui32CommandLen == 13 && MatchBytes(pDcCommand->m_sCommand + 2, "etNickList"))
                {
                    m_ui32StatCmdGetNickList++;
                    if (GetNickList(pDcCommand) && !((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
                    {
                        pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                    }
                    return;
                }
                break;
#ifdef USE_FLYLINKDC_EXT_JSON
            case 'E':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "xtJSON "))
                {
                    m_ui32StatCmdExtJSON++;
                }
                break;
#endif // USE_FLYLINKDC_EXT_JSON
            case 'M':
                if (pDcCommand->m_sCommand[2] == 'y')
                {
                    if (MatchBytes(pDcCommand->m_sCommand + 3, "INFO $ALL "))
                    {
                        m_ui32StatCmdMyInfo++;
                        if (!MyINFODeflood(pDcCommand))
                        {
                            return;
                        }

                        // Am I sending MyINFO of someone other ?
                        // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                        if ((pDcCommand->m_sCommand[13 + pDcCommand->m_pUser->m_sNick.size()] != ' ') ||
                            (memcmp(pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_sCommand + 13, pDcCommand->m_pUser->m_sNick.size()) != 0))
                        {
                            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)",
                                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                                             pDcCommand->m_pUser->m_sIP.data(),
                                                             pDcCommand->m_sCommand);

                            m_ui32StatNickSpoofing++;
                            pDcCommand->m_pUser->Close();
                            return;
                        }

                        if (!MyINFO(pDcCommand) || (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS ||
                            ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
                        {
                            return;
                        }

                        pDcCommand->m_pUser->AddMeOrIPv4Check();

                        return;
                    }
                    if (MatchBytes(pDcCommand->m_sCommand + 3, "Pass "))
                    {
                        m_ui32StatCmdMyPass++;
                        MyPass(pDcCommand);
                        return;
                    }
                }
                break;
            default:
                break;
            }
        }
        break;
    }
    case User::UserStates::STATE_GETNICKLIST_OR_MYINFO:
    {
        if (pDcCommand->m_sCommand[0] == '$')
        {
            if (pDcCommand->m_ui32CommandLen == 13 && MatchBytes(pDcCommand->m_sCommand + 1, "GetNickList"))
            {
                m_ui32StatCmdGetNickList++;
                if (GetNickList(pDcCommand))
                {
                    pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                }
                return;
            }
            if (MatchBytes(pDcCommand->m_sCommand + 1, "MyINFO $ALL "))
            {
                m_ui32StatCmdMyInfo++;
                if (!MyINFODeflood(pDcCommand))
                {
                    return;
                }

                // Am I sending MyINFO of someone other ?
                // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                if ((pDcCommand->m_sCommand[13 + pDcCommand->m_pUser->m_sNick.size()] != ' ') ||
                    (memcmp(pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_sCommand + 13, pDcCommand->m_pUser->m_sNick.size()) != 0))
                {
                    UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)",
                                                     pDcCommand->m_pUser->m_sNick.c_str(),
                                                     pDcCommand->m_pUser->m_sIP.data(),
                                                     pDcCommand->m_sCommand);

                    m_ui32StatNickSpoofing++;
                    pDcCommand->m_pUser->Close();
                    return;
                }

                if (!MyINFO(pDcCommand) || (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS ||
                    ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
                {
                    return;
                }

                pDcCommand->m_pUser->AddMeOrIPv4Check();

                return;
            }
#ifdef USE_FLYLINKDC_EXT_JSON
            else if (MatchBytes(pDcCommand->m_sCommand + 1, "ExtJSON "))
            {
                m_ui32StatCmdExtJSON++;
            }
#endif // USE_FLYLINKDC_EXT_JSON
        }
        break;
    }
    case User::UserStates::STATE_IPV4_CHECK:
    case User::UserStates::STATE_ADDME:
    case User::UserStates::STATE_ADDME_1LOOP:
    case User::UserStates::STATE_ADDME_2LOOP:
    {
        if (pDcCommand->m_sCommand[0] == '$')
        {
            if (auto* const l_end_command = strchr(pDcCommand->m_sCommand, ' '))
            {
                const auto c = *l_end_command;
                *l_end_command = 0x00;
                GlobalDataQueue::m_Ptr->m_metrics.counter_inc("flylinkdc_hub_dc_commands_total", 1,
                                                              {{"command", GetCommandMetricLabel(pDcCommand->m_sCommand)}});
                *l_end_command = c;
            }

            switch (pDcCommand->m_sCommand[1])
            {
            case 'G':
                if (pDcCommand->m_ui32CommandLen == 13 && MatchBytes(pDcCommand->m_sCommand + 2, "etNickList"))
                {
                    m_ui32StatCmdGetNickList++;
                    if (GetNickList(pDcCommand))
                    {
                        pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                    }
                    return;
                }
                break;
#ifdef USE_FLYLINKDC_EXT_JSON
            case 'E':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "xtJSON "))
                {
                    m_ui32StatCmdExtJSON++; // ExtJSON Step 1 First Login
                    if (!ExtJSONDeflood(pDcCommand->m_pUser, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, bCheck))
                    {
                        return;
                    }
                    {
                        // using json = nlohmann::json;
                        auto* const l_pos_json = strchr(pDcCommand->m_sCommand + 10, ' ');
                        std::string l_json_str;
                        std::string l_nick;
                        if (l_pos_json)
                        {
                            const auto l_len_header = static_cast<int>(l_pos_json - pDcCommand->m_sCommand);
                            l_json_str = std::string(l_pos_json, pDcCommand->m_ui32CommandLen - l_len_header);
                            l_nick = std::string(pDcCommand->m_sCommand, l_len_header);
                            if (l_json_str.size() > 3 && l_json_str.back() == '|' && l_json_str[l_json_str.size() - 2] == '}')
                            {
                                l_json_str.pop_back();
                                for (auto& c : l_json_str)
                                {
                                    if (static_cast<unsigned char>(c) >= 0x80)
                                    {
                                        c = '.';
                                    }
                                }
                            }
                        }
                        try
                        {
                            const auto l_json = nlohmann::json::parse(l_json_str);
                        }
                        catch (const std::exception& e)
                        {
                            pDcCommand->m_pUser->m_is_invalid_json = true;
                            m_ui32StatInvalidJson++;
                            LogWarn("[JSON] Parse error: nick='{}' ip='{}' len={} error='{}' json='{}'",
                                    pDcCommand->m_pUser->m_sNick,
                                    pDcCommand->m_pUser->m_sIP.data(),
                                    pDcCommand->m_ui32CommandLen,
                                    e.what(),
                                    l_json_str.size() > 200 ? l_json_str.substr(0, 200) + "..." : l_json_str);

                            pDcCommand->m_pUser->Close();
                            return;
                        }
                    }
                    pDcCommand->m_pUser->initExtJSON(pDcCommand->m_sCommand);
#ifdef USE_FLYLINKDC_EXT_JSON
                    (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::EXTJSON_ARRIVAL);
#endif
                }
                break;
#endif // USE_FLYLINKDC_EXT_JSON
            case 'M':
            {
                if (MatchBytes(pDcCommand->m_sCommand + 2, "yINFO $ALL "))
                {
                    m_ui32StatCmdMyInfo++;
                    if (!MyINFODeflood(pDcCommand))
                    {
                        return;
                    }

                    // Am I sending MyINFO of someone other ?
                    // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                    if ((pDcCommand->m_sCommand[13 + pDcCommand->m_pUser->m_sNick.size()] != ' ') ||
                        (memcmp(pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_sCommand + 13, pDcCommand->m_pUser->m_sNick.size()) != 0))
                    {
                        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)",
                                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                                         pDcCommand->m_pUser->m_sIP.data(),
                                                         pDcCommand->m_sCommand);

                        m_ui32StatNickSpoofing++;
                        pDcCommand->m_pUser->Close();
                        return;
                    }

                    (void)MyINFO(pDcCommand);

                    return;
                }
                if (MatchBytes(pDcCommand->m_sCommand + 2, "ultiSearch "))
                {
                    m_ui32StatCmdMultiSearch++;
                    (void)SearchDeflood(pDcCommand, true);
                    return;
                }
                break;
            }
            case 'S':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "earch "))
                {
                    m_ui32StatCmdSearch++;
                    (void)SearchDeflood(pDcCommand, false);
                    return;
                }
                break;
            default:
                break;
            }
        }
        else if (pDcCommand->m_sCommand[0] == '<')
        {
            LogInfo("[CHAT-RECV] nick='{}' cmd='{}' len={}",
                    pDcCommand->m_pUser->m_sNick,
                    std::string(pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen > 120 ? 120 : pDcCommand->m_ui32CommandLen),
                    pDcCommand->m_ui32CommandLen);
            m_ui32StatChat++;
            (void)ChatDeflood(pDcCommand);
            return;
        }
        break;
    }
    case User::UserStates::STATE_ADDED:
    {
        if (pDcCommand->m_sCommand[0] == '$')
        {
            switch (pDcCommand->m_sCommand[1])
            {
            case 'S':
            {
                if (MatchBytes(pDcCommand->m_sCommand + 2, "earch "))
                {
                    m_ui32StatCmdSearch++;
                    if (SearchDeflood(pDcCommand, false))
                    {
                        Search(pDcCommand, false);
                    }
                    return;
                }
                if (MatchBytes(pDcCommand->m_sCommand + 2, "R "))
                {
                    m_ui32StatCmdSR++;
                    SR(pDcCommand);
                    return;
                }
                break;
            }
            case 'C':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "onnectToMe "))
                {
                    m_ui32StatCmdConnectToMe++;
                    ConnectToMe(pDcCommand, false);
                    return;
                }
                else if (MatchBytes(pDcCommand->m_sCommand + 2, "lose "))
                {
                    m_ui32StatCmdClose++;
                    Close(pDcCommand);
                    return;
                }
                break;
            case 'R':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "evConnectToMe "))
                {
                    m_ui32StatCmdRevCTM++;
                    RevConnectToMe(pDcCommand);
                    return;
                }
                break;
#ifdef USE_FLYLINKDC_EXT_JSON
            case 'E':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "xtJSON "))
                {
                    m_ui32StatCmdExtJSON++; // ExtJSON Step 3 - Change ExtJSON
                    if (!ExtJSONDeflood(pDcCommand->m_pUser, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, bCheck))
                    {
                        return;
                    }

                    (void)SetExtJSON(pDcCommand->m_pUser, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen);
                    return;
                }
                break;
#endif // USE_FLYLINKDC_EXT_JSON

            case 'M':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "yINFO $ALL "))
                {
                    m_ui32StatCmdMyInfo++;
                    if (!MyINFODeflood(pDcCommand))
                    {
                        return;
                    }

                    // Am I sending MyINFO of someone other ?
                    // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                    if ((pDcCommand->m_sCommand[13 + pDcCommand->m_pUser->m_sNick.size()] != ' ') ||
                        (memcmp(pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_sCommand + 13, pDcCommand->m_pUser->m_sNick.size()) != 0))
                    {
                        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)",
                                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                                         pDcCommand->m_pUser->m_sIP.data(),
                                                         pDcCommand->m_sCommand);

                        m_ui32StatNickSpoofing++;
                        pDcCommand->m_pUser->Close();
                        return;
                    }

                    if (MyINFO(pDcCommand))
                    {
                        pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_PRCSD_MYINFO;
                    }
                    return;
                }
                else if (MatchBytes(pDcCommand->m_sCommand + 2, "ulti"))
                {
                    if (MatchBytes(pDcCommand->m_sCommand + 6, "Search "))
                    {
                        m_ui32StatCmdMultiSearch++;
                        if (SearchDeflood(pDcCommand, true))
                        {
                            Search(pDcCommand, true);
                        }
                        return;
                    }
                    if (MatchBytes(pDcCommand->m_sCommand + 6, "ConnectToMe "))
                    {
                        m_ui32StatCmdMultiConnectToMe++;
                        ConnectToMe(pDcCommand, true);
                        return;
                    }
                }
                else if (MatchBytes(pDcCommand->m_sCommand + 2, "yPass "))
                {
                    m_ui32StatCmdMyPass++;
                    if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS)
                    {
                        pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_WAITING_FOR_PASS;

                        if (!pDcCommand->m_pUser->m_LogInOut.m_Buffer.empty())
                        {
                            const auto optProfile = ProfileManager::m_Ptr->GetProfileIndex(pDcCommand->m_pUser->m_LogInOut.m_Buffer.data());
                            if (!optProfile)
                            {
                                pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser1",
                                                                true,
                                                                "<%s> %s.|",
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST)].c_str());

                                pDcCommand->m_pUser->m_LogInOut.m_Buffer.clear();

                                return;
                            }

                            if (pDcCommand->m_ui32CommandLen < 10)
                            {
                                pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser2",
                                                                true,
                                                                "<%s> %s!|",
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PASS_MUST_SPECIFIED)].c_str());

                                pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
                                return;
                            }

                            if (pDcCommand->m_ui32CommandLen > 73)
                            {
                                pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser2-1",
                                                                true,
                                                                "<%s> %s!|",
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_PASS_LEN_64_CHARS)].c_str());

                                pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
                                return;
                            }

                            pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

                            if (!RegManager::m_Ptr->AddNew(pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_sCommand + 8, *optProfile))
                            {
                                pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser3",
                                                                true,
                                                                "<%s> %s.|",
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_YOU_ARE_ALREADY_REGISTERED)].c_str());
                            }
                            else
                            {
                                pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser4",
                                                                true,
                                                                "<%s> %s %s.|",
                                                                SettingManager::HubSec(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THANK_YOU_FOR_PASSWORD_YOU_ARE_NOW_REGISTERED_AS)].c_str(),
                                                                pDcCommand->m_pUser->m_LogInOut.m_Buffer.data());
                            }

                            pDcCommand->m_pUser->m_LogInOut.m_Buffer.clear();

                            pDcCommand->m_pUser->m_i32Profile = *optProfile;

                            if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
                            {
                                if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::HASKEYICON))
                                {
                                    return;
                                }

                                pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_OPERATOR;

                                // alex82 ... HideUserKey / ������ ���� �����
                                if (!((pDcCommand->m_pUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
                                {
                                    Users::m_Ptr->Add2OpList(pDcCommand->m_pUser);
                                    GlobalDataQueue::m_Ptr->OpListStore(pDcCommand->m_pUser->m_sNick.c_str());
                                }
                                if (ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::ALLOWEDOPCHAT))
                                {
                                    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] &&
                                        (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] || !SettingManager::m_Ptr->m_bBotsSameNick))
                                    {
                                        if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO))
                                        {
                                            pDcCommand->m_pUser->SendCharDelayed(
                                                SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO)].c_str(),
                                                SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO)].size());
                                        }

                                        pDcCommand->m_pUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO)]);
                                        pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser5",
                                                                        true,
                                                                        "$OpList %s$$|",
                                                                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
                                    }
                                }
                            }
                        }

                        return;
                    }
                }
                break;
            case 'G':
            {
                if (pDcCommand->m_ui32CommandLen == 13 && MatchBytes(pDcCommand->m_sCommand + 2, "etNickList"))
                {
                    m_ui32StatCmdGetNickList++;
                    if (GetNickList(pDcCommand))
                    {
                        pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                    }
                    return;
                }
                if (MatchBytes(pDcCommand->m_sCommand + 2, "etINFO "))
                {
                    m_ui32StatCmdGetInfo++;
                    GetINFO(pDcCommand);
                    return;
                }
                break;
            }
            case 'T':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "o: "))
                {
                    m_ui32StatCmdTo++;
                    To(pDcCommand);
                    return;
                }
                break;
            case 'K':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "ick "))
                {
                    m_ui32StatCmdKick++;
                    Kick(pDcCommand);
                    return;
                }
                break;
            case 'O':
                if (MatchBytes(pDcCommand->m_sCommand + 2, "pForceMove $Who:"))
                {
                    m_ui32StatCmdOpForceMove++;
                    OpForceMove(pDcCommand);
                    return;
                }
                break;
            default:
                break;
            }
        }
        else if (pDcCommand->m_sCommand[0] == '<')
        {
            LogInfo("[CHAT-RECV] nick='{}' len={}", pDcCommand->m_pUser->m_sNick, pDcCommand->m_ui32CommandLen);
            m_ui32StatChat++;
            if (ChatDeflood(pDcCommand))
            {
                Chat(pDcCommand);
            }

            return;
        }
        break;
    }
    case User::UserStates::STATE_CLOSING:
    case User::UserStates::STATE_REMME:
        return;
    }

    // PPK ... fallback to full command identification and disconnect on bad state or unknown command not handled by script
    switch (pDcCommand->m_sCommand[0])
    {
    case '$':
    {
        // Special cases with non-standard handling
        switch (pDcCommand->m_sCommand[1])
        {
        case 'B':
            if (MatchBytes(pDcCommand->m_sCommand + 2, "otINFO"))
            {
                m_ui32StatBotINFO++;
                BotINFO(pDcCommand);
                return;
            }
            break;
#ifdef USE_FLYLINKDC_EXT_JSON
        case 'E':
            if (MatchBytes(pDcCommand->m_sCommand + 2, "xtJSON "))
            {
                m_ui32StatCmdExtJSON++;
                return;
            }
            break;
#endif
        default:
            break;
        }

        // Table-driven bad-state dispatch for standard commands
        // Increment stat counters per command, then close on bad state
        switch (pDcCommand->m_sCommand[1])
        {
        case 'C':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds, 2, 2))
                return; // ConnectToMe, Close
            break;
        case 'G':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds + 2, 2, 2))
                return; // GetINFO, GetNickList
            break;
        case 'K':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds + 4, 2, 2))
                return; // Key, Kick
            break;
        case 'M':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds + 6, 4, 2))
                return; // Multi*, MyINFO, MyPass
            break;
        case 'O':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds + 10, 1, 2))
                return; // OpForceMove
            break;
        case 'R':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds + 11, 1, 2))
                return; // RevConnectToMe
            break;
        case 'S':
            if (TryBadStateClose(pDcCommand, g_BadStateCmdsS, 3, 3))
                return; // Search, SR, Supports
            break;
        case 'T':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds + 12, 1, 2))
                return; // To
            break;
        case 'V':
            if (TryBadStateClose(pDcCommand, g_BadStateCmds + 13, 2, 2))
                return; // ValidateNick, Version
            break;
        default:
            break;
        }
        break;
    }
    case '<':
    {
        m_ui32StatChat++;

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in Chat %s (%s) - user closed.",
                                         std::to_underlying(pDcCommand->m_pUser->m_ui8State),
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        m_ui32StatBadState++;
        pDcCommand->m_pUser->Close();
        return;
    }
    default:
        break;
    }

    Unknown(pDcCommand);
}
//---------------------------------------------------------------------------

// $BotINFO pinger identification|
void DcCommands::BotINFO(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdBotINFO);
    if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) ||
        ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO))
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Not pinger $BotINFO or $BotINFO flood from %s (%s)", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        DcCommands::m_Ptr->m_ui32StatFlood++;
        pDcCommand->m_pUser->Close();
        return;
    }

    if (pDcCommand->m_ui32CommandLen < 9)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $BotINFO (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_BOTINFO;

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::BOTINFO_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    const auto& hubDesc = SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_DESCRIPTION)];
    const auto& hubEmail = SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_OWNER_EMAIL)];

    pDcCommand->m_pUser->SendFormat(
        "DcCommands::BotINFO",
        true,
        "$HubINFO %s$%s:%hu$%s.px.$%hd$%" PRIu64 "$%hd$%hd$PtokaX$%s|",
        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].c_str(),
        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS)].c_str(),
        SettingManager::m_Ptr->m_ui16PortNumbers[0],
        hubDesc.empty() ? "" : hubDesc.c_str(),
        SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS)],
        SettingManager::m_Ptr->m_ui64MinShare,
        SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SLOTS_LIMIT)],
        SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_HUBS_LIMIT)],
        hubEmail.empty() ? "" : hubEmail.c_str());

    if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST))
    {
        pDcCommand->m_pUser->Close();
    }
}
//---------------------------------------------------------------------------

// $ConnectToMe <nickname> <ownip>:<ownlistenport>
// $MultiConnectToMe <nick> <ownip:port> <hub[:port]>
void DcCommands::ConnectToMe(DcCommand* pDcCommand, const bool bMulti)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdConnectToMe);
    if ((!bMulti && pDcCommand->m_ui32CommandLen < 23) || (bMulti && pDcCommand->m_ui32CommandLen < 28))
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $%sConnectToMe (%s) from %s (%s) - user closed.",
                                         !bMulti ? "" : "Multi",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODCTM))
    {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::CTM,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION)],
                                     pDcCommand->m_pUser->m_ui16CTMs,
                                     pDcCommand->m_pUser->m_ui64CTMsTick,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_MESSAGES)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_TIME)])))
            {
                return;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION2)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::CTM,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION2)],
                                     pDcCommand->m_pUser->m_ui16CTMs2,
                                     pDcCommand->m_pUser->m_ui64CTMsTick2,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_MESSAGES2)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CTM_TIME2)])))
            {
                return;
            }
        }
    }

    if (pDcCommand->m_ui32CommandLen > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_CTM_LEN)]))
    {
        pDcCommand->m_pUser->SendFormat(
            "DcCommands::ConnectToMe", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CTM_TOO_LONG)].c_str());

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Long $ConnectToMe from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... $CTM means user is active ?!? Probably yes, let set it active and use on another places ;)
    if (pDcCommand->m_pUser->m_sTag.empty())
    {
        pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
    }

    // full data only and allow blocking
    if (ScriptManager::m_Ptr->Arrival(pDcCommand, std::to_underlying(!bMulti ? ScriptManager::CONNECTTOME_ARRIVAL : ScriptManager::MULTICONNECTTOME_ARRIVAL)) ||
        User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    char* pSpace = strchr(pDcCommand->m_sCommand + (!bMulti ? 13 : 18), ' ');
    if (!pSpace)
    {
        LogDbg(
            "[SYS] {}CTM missing space separator from {} ({}).", !bMulti ? "" : "M", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());
        return;
    }

    pSpace[0] = '\0';

    User* const pOtherUser =
        HashManager::m_Ptr->FindUser(std::string_view(pDcCommand->m_sCommand + (!bMulti ? 13 : 18), pSpace - (pDcCommand->m_sCommand + (!bMulti ? 13 : 18))));
    // PPK ... no connection to yourself !!!
    if (!pOtherUser || pOtherUser == pDcCommand->m_pUser || pOtherUser->m_ui8State != User::UserStates::STATE_ADDED)
    {
        LogDbg(
            "[SYS] {}CTM target not found/invalid from {} ({}).", !bMulti ? "" : "M", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());
        return;
    }

    pSpace[0] = ' ';

    // IP check
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOIPCHECK))
    {
        // At end of sData is | and we need to remove it.
        pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0';

        // Now between pSpace+1 and end of string is something that should be IP:Port .. let's check that and start with length.
        const auto ui32IpPortLen = static_cast<uint32_t>((pDcCommand->m_ui32CommandLen - 1) - ((pSpace + 1) - pDcCommand->m_sCommand));

        // Check minimal (shortest ip:port can be [::1]:1) and maximal (longest ip:port can be [1234:1234:1234:1234:1234:1234:1234:1234]:12345S) length.
        if (ui32IpPortLen < 7 || ui32IpPortLen > 48)
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::ConnectToMe bad IP:Port len",
                                            true,
                                            "<%s> %s '%s'!|",
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_CLIENT_SEND_INCORRECT_IPPORT_IN_COMMAND)].c_str(),
                                            pSpace + 1);

            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad IP:Port length in %sCTM from %s (%s). (%s)",
                                             !bMulti ? "" : "M",
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data(),
                                             pDcCommand->m_sCommand);
            pDcCommand->m_pUser->m_is_bad_len_port = true; // FlylinkDC++
            return;
        }

        // Check if Port is valid. Here can be S (as secure [what a joke lol]) to indicate TLS connection request after port ...
        uint32_t ui32PortLen = 0;
        uint16_t ui16Port = 0; // Zero == invalid port ...
        bool bSecure = false;  // To restore secure status after checks

        if (((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) &&
            pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 2] == 'S')
        {
            // Remove S character and set secure to true
            pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 2] = '\0';
            bSecure = true;

            // Check for port validity and get Port when valid
            ui16Port = CheckAndGetPort(
                pSpace + 1 + (ui32IpPortLen - 7), static_cast<uint8_t>(ui32IpPortLen - ((pSpace + (ui32IpPortLen - 6)) - pSpace)), ui32PortLen); //-V1065
        }
        else
        {
            ui16Port = CheckAndGetPort(
                pSpace + 1 + (ui32IpPortLen - 6), static_cast<uint8_t>(ui32IpPortLen - ((pSpace + (ui32IpPortLen - 6)) - pSpace)), ui32PortLen); //-V1065
        }

        // Check if we get valid port number
        if (ui16Port == 0)
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::ConnectToMe invalid Port",
                                            true,
                                            "<%s> %s '%s'!|",
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_CLIENT_SEND_INCORRECT_PORT_IN_CTM)].c_str(),
                                            pSpace + 1);

            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Port in %sCTM from %s (%s). (%s)",
                                             !bMulti ? "" : "M",
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data(),
                                             pDcCommand->m_sCommand);

            pDcCommand->m_pUser->Close();
            return;
        }

        uint32_t ui32IpLen = (ui32IpPortLen - ui32PortLen) - 1;
        if (bSecure)
        {
            ui32IpLen--;
        }

        bool bInvalidIP = true;
        char* pIP = pSpace + 1;

        // Check if we get valid IP address
        if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
        {
            if ((pDcCommand->m_pUser->m_ui8IpLen + 2U) == ui32IpLen && pIP[0] == '[' && pIP[1 + pDcCommand->m_pUser->m_ui8IpLen] == ']' &&
                strncmp(pIP + 1, pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_pUser->m_ui8IpLen) == 0)
            {
                bInvalidIP = false;
            }
            else if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) && pDcCommand->m_pUser->m_ui8IPv4Len == ui32IpLen &&
                     strncmp(pIP, pDcCommand->m_pUser->m_sIPv4.data(), pDcCommand->m_pUser->m_ui8IPv4Len) == 0)
            {
                bInvalidIP = false;
            }
        }
        else if (pDcCommand->m_pUser->m_ui8IpLen == ui32IpLen && strncmp(pIP, pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_pUser->m_ui8IpLen) == 0)
        {
            bInvalidIP = false;
        }

        if (bInvalidIP)
        {
            if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP))
            {
                UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Ip in %sCTM from %s (%s/%s). (%s)",
                                                 !bMulti ? "" : "M",
                                                 pDcCommand->m_pUser->m_sNick.c_str(),
                                                 pDcCommand->m_pUser->m_sIP.data(),
                                                 pDcCommand->m_pUser->m_sIPv4.data(),
                                                 pDcCommand->m_sCommand);
            }

            if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && (pOtherUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
            {
                const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                       ServerManager::m_szGlobalBufferSize,
                                       "$ConnectToMe %s [%s]:%hu%s|",
                                       pOtherUser->m_sNick.c_str(),
                                       pDcCommand->m_pUser->m_sIP.data(),
                                       ui16Port,
                                       bSecure ? "S" : "");
                if (iMsgLen > 0)
                {
                    pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, ServerManager::m_pGlobalBuffer, iMsgLen, pOtherUser);
                }
            }
            else if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4 &&
                     (pOtherUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4)
            {
                const char* sIP = pDcCommand->m_pUser->m_ui8IPv4Len == 0 ? pDcCommand->m_pUser->m_sIP.data() : pDcCommand->m_pUser->m_sIPv4.data();

                const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                       ServerManager::m_szGlobalBufferSize,
                                       "$ConnectToMe %s %s:%hu%s|",
                                       pOtherUser->m_sNick.c_str(),
                                       sIP,
                                       ui16Port,
                                       bSecure ? "S" : "");
                if (iMsgLen > 0)
                {
                    pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, ServerManager::m_pGlobalBuffer, iMsgLen, pOtherUser);
                }
            }

            if (ui32IpLen != 0 && pIP[ui32IpLen - 1] == ']')
            {
                pIP[ui32IpLen - 1] = '\0';
            }
            else
            {
                pIP[ui32IpLen] = '\0';
            }

            if (pIP[0] == '[')
            {
                pIP++;
            }

            SendIPFixedMsg(pDcCommand->m_pUser, pIP, pDcCommand->m_pUser->m_sIP.data());
            return;
        }

        if (bSecure)
        {
            pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 2] = 'S';
        }
        pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|';
    }

    if (bMulti)
    {
        pDcCommand->m_sCommand[5] = '$';
    }

    pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO,
                                     !bMulti ? pDcCommand->m_sCommand : pDcCommand->m_sCommand + 5,
                                     !bMulti ? pDcCommand->m_ui32CommandLen : pDcCommand->m_ui32CommandLen - 5,
                                     pOtherUser);
}
//---------------------------------------------------------------------------

// $GetINFO <nickname> <ownnickname>
void DcCommands::GetINFO(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdGetINFO);
    if (((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) ||
        ((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) ||
        ((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Not allowed user %s (%s) send $GetINFO - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... code change, added own nick and space on right place check
    if (pDcCommand->m_ui32CommandLen < (12u + pDcCommand->m_pUser->m_sNick.size()) ||
        pDcCommand->m_ui32CommandLen > (75u + pDcCommand->m_pUser->m_sNick.size()) ||
        pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - pDcCommand->m_pUser->m_sNick.size() - 2] != ' ' ||
        memcmp(pDcCommand->m_sCommand + (pDcCommand->m_ui32CommandLen - pDcCommand->m_pUser->m_sNick.size() - 1),
               pDcCommand->m_pUser->m_sNick.c_str(),
               pDcCommand->m_pUser->m_sNick.size()) != 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $GetINFO from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::GETINFO_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }
}
//---------------------------------------------------------------------------
namespace {
// Helper: send data to user, compressing with ZPipe if supported and beneficial.
void SendListOrZCompressed(User* pUser, const char* pData, const uint32_t ui32Len, std::vector<char>& zBuf, uint32_t& ui32ZLen)
{
    if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == 0)
    {
        pUser->SendCharDelayed(pData, ui32Len);
        return;
    }

    if (ui32ZLen == 0)
    {
        ZlibUtility::m_Ptr->CreateZPipe(std::string_view(pData, ui32Len), zBuf, ui32ZLen);
        if (ui32ZLen == 0)
        {
            pUser->SendCharDelayed(pData, ui32Len);
            return;
        }
    }

    (void)pUser->PutInSendBuf(zBuf.data(), ui32ZLen);
    ServerManager::m_ui64BytesSentSaved += ui32Len - ui32ZLen;
}
} // namespace
//---------------------------------------------------------------------------

// $GetNickList
bool DcCommands::GetNickList(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdGetNickList);
    if (((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) &&
        ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST))
    {
        // PPK ... refresh not allowed !
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Bad $GetNickList request %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return false;
    }
    if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
    {
        if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST))
        {
            pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_BIG_SEND_BUFFER;
            if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) && Users::m_Ptr->m_ui32NickListLen > 11)
            {
                SendListOrZCompressed(pDcCommand->m_pUser,
                                      Users::m_Ptr->m_NickList.data(),
                                      Users::m_Ptr->m_ui32NickListLen,
                                      Users::m_Ptr->m_ZNickList,
                                      Users::m_Ptr->m_ui32ZNickListLen);
            }

            if (SettingManager::m_Ptr->m_ui8FullMyINFOOption == 2)
            {
                if (Users::m_Ptr->m_ui32MyInfosLen != 0)
                {
                    SendListOrZCompressed(pDcCommand->m_pUser,
                                          Users::m_Ptr->m_MyInfos.data(),
                                          Users::m_Ptr->m_ui32MyInfosLen,
                                          Users::m_Ptr->m_ZMyInfos,
                                          Users::m_Ptr->m_ui32ZMyInfosLen);
                }
            }
            else if (Users::m_Ptr->m_ui32MyInfosTagLen != 0)
            {
                SendListOrZCompressed(pDcCommand->m_pUser,
                                      Users::m_Ptr->m_MyInfosTag.data(),
                                      Users::m_Ptr->m_ui32MyInfosTagLen,
                                      Users::m_Ptr->m_ZMyInfosTag,
                                      Users::m_Ptr->m_ui32ZMyInfosTagLen);
            }

            if (Users::m_Ptr->m_ui32OpListLen > 9)
            {
                SendListOrZCompressed(
                    pDcCommand->m_pUser, Users::m_Ptr->m_OpList.data(), Users::m_Ptr->m_ui32OpListLen, Users::m_Ptr->m_ZOpList, Users::m_Ptr->m_ui32ZOpListLen);
            }

            if (pDcCommand->m_pUser->m_ui32SendBufDataLen != 0)
            {
                (void)pDcCommand->m_pUser->Try2Send();
            }

            pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;

            if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REPORT_PINGERS)])
            {
                GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::GetNickList",
                                                            "<%s> *** %s: %s %s: %s %s.|",
                                                            SettingManager::HubSec(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PINGER_FROM_IP)].c_str(),
                                                            pDcCommand->m_pUser->m_sIP.data(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_NICK)].c_str(),
                                                            pDcCommand->m_pUser->m_sNick.c_str(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DETECTED_LWR)].c_str());
            }

            if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO))
            {
                pDcCommand->m_pUser->Close();
            }
            return false;
        }
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] $GetNickList flood from pinger %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        DcCommands::m_Ptr->m_ui32StatFlood++;
        pDcCommand->m_pUser->Close();
        return false;
    }

    pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;

    // PPK ... check flood...
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODGETNICKLIST) &&
        SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_ACTION)] != 0)
    {
        if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                 DefloodTypes::GETNICKLIST,
                                 SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_ACTION)],
                                 pDcCommand->m_pUser->m_ui16GetNickLists,
                                 pDcCommand->m_pUser->m_ui64GetNickListsTick,
                                 SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_MESSAGES)],
                                 (static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_TIME)])) * 60))
        {
            return false;
        }
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::GETNICKLIST_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State) ||
        ((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

// $Key
void DcCommands::Key(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdKey);
    if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_KEY) == User::BIT_HAVE_KEY))
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] $Key flood from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        DcCommands::m_Ptr->m_ui32StatFlood++;
        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_KEY;

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    const char* receivedKey = pDcCommand->m_sCommand + 5;

    if (pDcCommand->m_ui32CommandLen < 6 || !VerifyLockKey(pDcCommand->m_pUser->m_LogInOut.m_Buffer.data(), receivedKey))
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Bad $Key from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_pUser->FreeBuffer();

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|'; // add back pipe

    (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::KEY_ARRIVAL);

    if (User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pDcCommand->m_pUser->m_ui8State = User::UserStates::STATE_VALIDATE;
}
//---------------------------------------------------------------------------

// $Kick <name>
void DcCommands::Kick(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdKick);
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::KICK))
    {
        pDcCommand->m_pUser->SendFormat(
            "DcCommands::Kick1", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD)].c_str());
        return;
    }

    if (pDcCommand->m_ui32CommandLen < 8)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Kick (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::KICK_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    User* OtherUser = HashManager::m_Ptr->FindUser(std::string_view(pDcCommand->m_sCommand + 6, pDcCommand->m_ui32CommandLen - 7));
    if (OtherUser)
    {
        // Self-kick
        if (OtherUser == pDcCommand->m_pUser)
        {
            pDcCommand->m_pUser->SendFormat(
                "DcCommands::Kick2", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_CANT_KICK_YOURSELF)].c_str());
            return;
        }

        if (OtherUser->m_i32Profile != -1 && pDcCommand->m_pUser->m_i32Profile > OtherUser->m_i32Profile)
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::Kick3",
                                            true,
                                            "<%s> %s %s!|",
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALLOWED_TO_KICK)].c_str(),
                                            OtherUser->m_sNick.c_str());
            return;
        }

        {
            auto& rList = pDcCommand->m_pUser->m_CmdToUserList;
            for (auto it = rList.begin(); it != rList.end(); ++it)
            {
                if (OtherUser == (*it)->m_pToUser)
                {
                    (*it)->m_pToUser->SendChar((*it)->m_sCommand);
                    rList.erase(it);
                    break;
                }
            }
        }

        char* sBanTime;
        if (!OtherUser->m_LogInOut.m_Buffer.empty() && (sBanTime = stristr(OtherUser->m_LogInOut.m_Buffer.data(), "_BAN_")))
        {
            sBanTime[0] = '\0';

            if (sBanTime[5] == '\0' || sBanTime[5] == ' ') // permban
            {
                if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::BAN))
                {
                    pDcCommand->m_pUser->SendFormat("DcCommands::Kick4",
                                                    true,
                                                    "<%s> %s!|",
                                                    SettingManager::HubSec(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD)].c_str());
                    return;
                }

                BanManager::m_Ptr->Ban(OtherUser, OtherUser->m_LogInOut.m_Buffer.data(), pDcCommand->m_pUser->m_sNick.c_str(), false);

                GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Kick1",
                                                            "<%s> *** %s %s %s %s %s %s %s.|",
                                                            SettingManager::HubSec(),
                                                            OtherUser->m_sNick.c_str(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                            OtherUser->m_sIP.data(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                            pDcCommand->m_pUser->m_sNick.c_str());

                if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)] ||
                    !((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
                {
                    pDcCommand->m_pUser->SendFormat("DcCommands::Kick5",
                                                    true,
                                                    "<%s> *** %s %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    OtherUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                    OtherUser->m_sIP.data(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str());
                }

                // disconnect the user
                UdpDebug::m_Ptr->BroadcastFormat(
                    "[SYS] User %s (%s) kicked by %s", OtherUser->m_sNick.c_str(), OtherUser->m_sIP.data(), pDcCommand->m_pUser->m_sNick.c_str());

                OtherUser->Close();

                return;
            }
            if (isdigit(static_cast<unsigned char>(sBanTime[5]))) // tempban
            {
                if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::TEMP_BAN))
                {
                    pDcCommand->m_pUser->SendFormat("DcCommands::Kick6",
                                                    true,
                                                    "<%s> %s!|",
                                                    SettingManager::HubSec(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD)].c_str());
                    return;
                }

                uint32_t i = 6;
                while (sBanTime[i] != '\0' && isdigit(static_cast<unsigned char>(sBanTime[i])))
                {
                    i++;
                }

                const char cTime = sBanTime[i];
                sBanTime[i] = '\0';
                int iTime = 0;
                if (!safe_stoi(sBanTime + 5, iTime))
                {
                    return;
                }
                time_t acc_time, ban_time;

                if (cTime != '\0' && iTime > 0 && GenerateTempBanTime(cTime, iTime, acc_time, ban_time))
                {
                    BanManager::m_Ptr->TempBan(OtherUser, OtherUser->m_LogInOut.m_Buffer.data(), pDcCommand->m_pUser->m_sNick.c_str(), 0, ban_time, false);

                    const std::string sTime = formatTime((ban_time - acc_time) / 60);

                    GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Kick2",
                                                                "<%s> *** %s %s %s %s %s %s %s %s: %s.|",
                                                                SettingManager::HubSec(),
                                                                OtherUser->m_sNick.c_str(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                                OtherUser->m_sIP.data(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                                pDcCommand->m_pUser->m_sNick.c_str(),
                                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                                sTime.c_str());

                    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)] ||
                        !((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
                    {
                        pDcCommand->m_pUser->SendFormat("DcCommands::Kick7",
                                                        true,
                                                        "<%s> *** %s %s %s %s %s %s: %s.|",
                                                        SettingManager::HubSec(),
                                                        OtherUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                        OtherUser->m_sIP.data(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                        sTime.c_str());
                    }

                    // disconnect the user
                    UdpDebug::m_Ptr->BroadcastFormat(
                        "[SYS] User %s (%s) kicked by %s", OtherUser->m_sNick.c_str(), OtherUser->m_sIP.data(), pDcCommand->m_pUser->m_sNick.c_str());

                    OtherUser->Close();

                    return;
                }
            }
        }

        BanManager::m_Ptr->TempBan(OtherUser,
                                   !OtherUser->m_LogInOut.m_Buffer.empty() ? OtherUser->m_LogInOut.m_Buffer.data() : nullptr,
                                   pDcCommand->m_pUser->m_sNick.c_str(),
                                   0,
                                   0,
                                   false);

        GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Kick3",
                                                    "<%s> *** %s %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    OtherUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                    OtherUser->m_sIP.data(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_KICKED_BY)].c_str(),
                                                    pDcCommand->m_pUser->m_sNick.c_str());

        if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)] ||
            !((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::Kick8",
                                            true,
                                            "<%s> *** %s %s %s %s.|",
                                            SettingManager::HubSec(),
                                            OtherUser->m_sNick.c_str(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                            OtherUser->m_sIP.data(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_KICKED)].c_str());
        }

        // disconnect the user
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] User %s (%s) kicked by %s", OtherUser->m_sNick.c_str(), OtherUser->m_sIP.data(), pDcCommand->m_pUser->m_sNick.c_str());

        OtherUser->Close();
    }
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
bool DcCommands::SearchDeflood(DcCommand* pDcCommand, const bool bMulti)
{
    // search flood protection ... modified by PPK ;-)
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODSEARCH))
    {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::SEARCH,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION)],
                                     pDcCommand->m_pUser->m_ui16Searchs,
                                     pDcCommand->m_pUser->m_ui64SearchsTick,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_MESSAGES)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_TIME)])))
            {
                return false;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION2)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::SEARCH,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION2)],
                                     pDcCommand->m_pUser->m_ui16Searchs2,
                                     pDcCommand->m_pUser->m_ui64SearchsTick2,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_MESSAGES2)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_TIME2)])))
            {
                return false;
            }
        }

        // 2nd check for same search flood
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_ACTION)] != 0)
        {
            bool bNewData = false;
            if (DeFloodCheckForSameFlood(pDcCommand->m_pUser,
                                         DefloodTypes::SAME_SEARCH,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_ACTION)],
                                         pDcCommand->m_pUser->m_ui16SameSearchs,
                                         pDcCommand->m_pUser->m_ui64SameSearchsTick,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_MESSAGES)],
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_TIME)],
                                         pDcCommand->m_sCommand + (bMulti ? 13 : 8),
                                         pDcCommand->m_ui32CommandLen - (bMulti ? 13 : 8),
                                         pDcCommand->m_pUser->m_LastSearch.c_str(),
                                         static_cast<uint16_t>(pDcCommand->m_pUser->m_LastSearch.size()),
                                         bNewData))
            {
                return false;
            }

            if (bNewData)
            {
                pDcCommand->m_pUser->SetLastSearch(
                    std::string_view(pDcCommand->m_sCommand + (bMulti ? 13 : 8), pDcCommand->m_ui32CommandLen - (bMulti ? 13 : 8)));
            }
        }
    }

    return true;
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
void DcCommands::Search(DcCommand* pDcCommand, const bool bMulti)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdSearch);

    uint32_t iAfterCmd;
    if (!bMulti)
    {
        if (pDcCommand->m_ui32CommandLen < 10 || pDcCommand->m_ui32CommandLen > 512) // FlylinkDC++
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Search (%s) from %s (%s) - user closed.",
                                             pDcCommand->m_sCommand,
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data());

            pDcCommand->m_pUser->Close();
            return;
        }
        iAfterCmd = 8;
    }
    else
    {
        if (pDcCommand->m_ui32CommandLen < 15 || pDcCommand->m_ui32CommandLen > 512) // FlylinkDC++
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MultiSearch (%s) from %s (%s) - user closed.",
                                             pDcCommand->m_sCommand,
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data());

            pDcCommand->m_pUser->Close();
            return;
        }
        iAfterCmd = 13;
    }

    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOSEARCHINTERVAL))
    {
        if (DeFloodCheckInterval(pDcCommand->m_pUser,
                                 DefloodTypes::INTERVAL_SEARCH,
                                 pDcCommand->m_pUser->m_ui16SearchsInt,
                                 pDcCommand->m_pUser->m_ui64SearchsIntTick,
                                 SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_INTERVAL_MESSAGES)],
                                 static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SEARCH_INTERVAL_TIME)])))
        {
            return;
        }
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::SEARCH_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    // send search from actives to all, from passives to actives only
    // PPK ... optimization ;o)
    if (!bMulti && MatchBytes(pDcCommand->m_sCommand + iAfterCmd, "Hub:"))
    {
        if (pDcCommand->m_pUser->m_sTag.empty())
        {
            pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
        }

        // PPK ... check nick !!!
        if ((pDcCommand->m_sCommand[iAfterCmd + 4 + pDcCommand->m_pUser->m_sNick.size()] != ' ') ||
            (memcmp(pDcCommand->m_sCommand + iAfterCmd + 4, pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sNick.size()) != 0))
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in search from %s (%s) - user closed. (%s)",
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data(),
                                             pDcCommand->m_sCommand);

            DcCommands::m_Ptr->m_ui32StatNickSpoofing++;
            pDcCommand->m_pUser->Close();
            return;
        }

        if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOSEARCHLIMITS) &&
            (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN)] != 0 || SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)] != 0))
        {
            // PPK ... search string len check
            // $Search Hub:PPK F?T?0?2?test|
            uint32_t iChar = iAfterCmd + 8 + pDcCommand->m_pUser->m_sNick.size() + 1;
            uint32_t iCount = 0;
            for (; iChar < pDcCommand->m_ui32CommandLen; iChar++)
            {
                if (pDcCommand->m_sCommand[iChar] == '?')
                {
                    iCount++;
                    if (iCount == 2)
                    {
                        break;
                    }
                }
            }

            iCount = pDcCommand->m_ui32CommandLen - 2 - iChar;

            if (iCount < static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN)]))
            {
                pDcCommand->m_pUser->SendFormat("DcCommands::Search1",
                                                true,
                                                "<%s> %s %hd.|",
                                                SettingManager::HubSec(),
                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_MIN_SEARCH_LEN_IS)].c_str(),
                                                SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN)]);
                return;
            }
            if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)] != 0 &&
                iCount > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)]))
            {
                pDcCommand->m_pUser->SendFormat("DcCommands::Search2",
                                                true,
                                                "<%s> %s %hd.|",
                                                SettingManager::HubSec(),
                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_MAX_SEARCH_LEN_IS)].c_str(),
                                                SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)]);
                return;
            }
        }

        pDcCommand->m_pUser->m_ui32SR = 0;

        pDcCommand->m_pUser->m_pCmdPassiveSearch =
            AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, false);
    }
    else
    {
        if (pDcCommand->m_pUser->m_sTag.empty())
        {
            pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
        }

        if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOSEARCHLIMITS) &&
            (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN)] != 0 || SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)] != 0))
        {
            // PPK ... search string len check
            // $Search 1.2.3.4:1 F?F?0?2?test| / $Search [::1]:1 F?F?0?2?test|
            uint32_t ui32Char = iAfterCmd + 9;
            uint32_t ui32QCount = 0;

            for (; ui32Char < pDcCommand->m_ui32CommandLen; ui32Char++)
            {
                if (pDcCommand->m_sCommand[ui32Char] == '?')
                {
                    ui32QCount++;
                    if (ui32QCount == 4)
                    {
                        break;
                    }
                }
            }

            const uint32_t ui32Count = pDcCommand->m_ui32CommandLen - 2 - ui32Char;

            if (ui32QCount != 4 || ui32Count < static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN)]))
            {
                pDcCommand->m_pUser->SendFormat("DcCommands::Search3",
                                                true,
                                                "<%s> %s %hd.|",
                                                SettingManager::HubSec(),
                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_MIN_SEARCH_LEN_IS)].c_str(),
                                                SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN)]);
                return;
            }
            if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)] != 0 &&
                ui32Count > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)]))
            {
                pDcCommand->m_pUser->SendFormat("DcCommands::Search4",
                                                true,
                                                "<%s> %s %hd.|",
                                                SettingManager::HubSec(),
                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_MAX_SEARCH_LEN_IS)].c_str(),
                                                SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN)]);
                return;
            }
        }

        // Get space after IP:Port
        char* pSpace = strchr(pDcCommand->m_sCommand + iAfterCmd, ' ');
        if (!pSpace)
        {
            return;
        }

        pSpace[0] = '\0';

        // Now we have between sData+iAfterCmd and pSpace IP:Port ... or we should have. Let's check that and start with length.
        const auto ui32IpPortLen = static_cast<uint32_t>(pSpace - (pDcCommand->m_sCommand + iAfterCmd));

        // Check minimal (shortest ip:port can be [::1]:1) and maximal (longest ip:port can be [1234:1234:1234:1234:1234:1234:1234:1234]:12345S) length.
        if (ui32IpPortLen < 7 || ui32IpPortLen > 48)
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::Search bad IP:Port len",
                                            true,
                                            "<%s> %s '%s'!|",
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_CLIENT_SEND_INCORRECT_IPPORT_IN_COMMAND)].c_str(),
                                            pDcCommand->m_sCommand + iAfterCmd);

            pSpace[0] = ' ';
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad IP:Port length in %sSearch from %s (%s). (%s)",
                                             !bMulti ? "" : "M",
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data(),
                                             pDcCommand->m_sCommand);
            pDcCommand->m_pUser->m_is_bad_port = true; // FlylinkDC++

            return;
        }

        // Check if Port is valid.
        uint32_t ui32PortLen = 0;
        uint16_t ui16Port = 0; // Zero == invalid port ...

        // Check for port validity and get Port when valid
        ui16Port = CheckAndGetPort(
            pDcCommand->m_sCommand + iAfterCmd + (ui32IpPortLen - 6),
            static_cast<uint8_t>(ui32IpPortLen - (((pDcCommand->m_sCommand + iAfterCmd) + (ui32IpPortLen - 6)) - (pDcCommand->m_sCommand + iAfterCmd))),
            ui32PortLen); //-V1065

        // Check if we get valid port number
        if (ui16Port == 0)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Port in %sSearch from %s (%s). Port string: '%s' ui32IpPortLen=%u",
                                             !bMulti ? "" : "M",
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data(),
                                             pDcCommand->m_sCommand + iAfterCmd,
                                             ui32IpPortLen);

            pSpace[0] = ' ';
            return;
        }

        const uint32_t ui32IpLen = (ui32IpPortLen - ui32PortLen) - 1;
        bool bInvalidIP = true;
        char* pIP = pDcCommand->m_sCommand + iAfterCmd;

        // Check if we get valid IP address
        if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
        {
            if ((pDcCommand->m_pUser->m_ui8IpLen + 2U) == ui32IpLen && pIP[0] == '[' && pIP[1 + pDcCommand->m_pUser->m_ui8IpLen] == ']' &&
                strncmp(pIP + 1, pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_pUser->m_ui8IpLen) == 0)
            {
                bInvalidIP = false;
            }
            else if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) && pDcCommand->m_pUser->m_ui8IPv4Len == ui32IpLen &&
                     strncmp(pIP, pDcCommand->m_pUser->m_sIPv4.data(), pDcCommand->m_pUser->m_ui8IPv4Len) == 0)
            {
                bInvalidIP = false;
            }
        }
        else if (pDcCommand->m_pUser->m_ui8IpLen == ui32IpLen && strncmp(pIP, pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_pUser->m_ui8IpLen) == 0)
        {
            bInvalidIP = false;
        }

        // IP check
        if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOIPCHECK) && bInvalidIP)
        {
            if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP))
            {
                UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Ip in %sSearch from %s (%s/%s). (%s)",
                                                 !bMulti ? "" : "M",
                                                 pDcCommand->m_pUser->m_sNick.c_str(),
                                                 pDcCommand->m_pUser->m_sIP.data(),
                                                 pDcCommand->m_pUser->m_sIPv4.data(),
                                                 pDcCommand->m_sCommand);
            }

            if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
            {
                if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE)
                {
                    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                           ServerManager::m_szGlobalBufferSize,
                                           "$Search [%s]:%hu %s",
                                           pDcCommand->m_pUser->m_sIP.data(),
                                           ui16Port,
                                           pSpace + 1);
                    if (iMsgLen > 0)
                    {
                        pDcCommand->m_pUser->m_pCmdActive6Search =
                            AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive6Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                    }
                }
                else
                {
                    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                           ServerManager::m_szGlobalBufferSize,
                                           "$Search Hub:%s %s",
                                           pDcCommand->m_pUser->m_sNick.c_str(),
                                           pSpace + 1);
                    if (iMsgLen > 0)
                    {
                        pDcCommand->m_pUser->m_pCmdPassiveSearch =
                            AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, ServerManager::m_pGlobalBuffer, iMsgLen, false);
                    }
                }

                if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4)
                {
                    if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE)
                    {
                        const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                               ServerManager::m_szGlobalBufferSize,
                                               "$Search %s:%hu %s",
                                               pDcCommand->m_pUser->m_sIPv4.data(),
                                               ui16Port,
                                               pSpace + 1);
                        if (iMsgLen > 0)
                        {
                            pDcCommand->m_pUser->m_pCmdActive4Search =
                                AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                        }
                    }
                    else
                    {
                        const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                               ServerManager::m_szGlobalBufferSize,
                                               "$Search Hub:%s %s",
                                               pDcCommand->m_pUser->m_sNick.c_str(),
                                               pSpace + 1);
                        if (iMsgLen > 0)
                        {
                            pDcCommand->m_pUser->m_pCmdPassiveSearch =
                                AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, ServerManager::m_pGlobalBuffer, iMsgLen, false);
                        }
                    }
                }
            }
            else if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4)
            {
                const char* sIP = pDcCommand->m_pUser->m_ui8IPv4Len == 0 ? pDcCommand->m_pUser->m_sIP.data() : pDcCommand->m_pUser->m_sIPv4.data();

                const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search %s:%hu %s", sIP, ui16Port, pSpace + 1);
                if (iMsgLen > 0)
                {
                    pDcCommand->m_pUser->m_pCmdActive4Search =
                        AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                }
            }

            if (ui32IpLen != 0 && pIP[ui32IpLen - 1] == ']')
            {
                pIP[ui32IpLen - 1] = '\0';
            }
            else
            {
                pIP[ui32IpLen] = '\0';
            }

            if (pIP[0] == '[')
            {
                pIP++;
            }

            SendIPFixedMsg(pDcCommand->m_pUser, pIP, pDcCommand->m_pUser->m_sIP.data());
            return;
        }

        // Restore space after IP:Port
        pSpace[0] = ' ';

        if (bMulti)
        {
            pDcCommand->m_sCommand[5] = '$';
            pDcCommand->m_sCommand += 5;
            pDcCommand->m_ui32CommandLen -= 5;
        }

        if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
        {
            if (pDcCommand->m_sCommand[8] == '[')
            {
                pDcCommand->m_pUser->m_pCmdActive6Search =
                    AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive6Search, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, true);

                // IPv6 user sent active request.. when he have available IPv4 then create proper IPv4 request too.
                if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4)
                {
                    if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE)
                    {
                        const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                               ServerManager::m_szGlobalBufferSize,
                                               "$Search %s:%hu %s",
                                               pDcCommand->m_pUser->m_sIPv4.data(),
                                               ui16Port,
                                               pSpace + 1);
                        if (iMsgLen > 0)
                        {
                            pDcCommand->m_pUser->m_pCmdActive4Search =
                                AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                        }
                    }
                    else
                    {
                        const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                               ServerManager::m_szGlobalBufferSize,
                                               "$Search Hub:%s %s",
                                               pDcCommand->m_pUser->m_sNick.c_str(),
                                               pSpace + 1);
                        if (iMsgLen > 0)
                        {
                            pDcCommand->m_pUser->m_pCmdPassiveSearch =
                                AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, ServerManager::m_pGlobalBuffer, iMsgLen, false);
                        }
                    }
                }
            }
            else
            {
                // When IPv6 user sent active search request with IPv4 address (he should't do that) then just send this request...
                pDcCommand->m_pUser->m_pCmdActive4Search =
                    AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, true);
            }
        }
        else
        {
            pDcCommand->m_pUser->m_pCmdActive4Search =
                AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, true);
        }
    }
}
//---------------------------------------------------------------------------
#ifdef USE_FLYLINKDC_EXT_JSON
//---------------------------------------------------------------------------
// $ExtJSON |
bool DcCommands::ExtJSONDeflood(User* pUser, const char* sData, const uint32_t ui32Len, const bool /* bCheck */)
{
    if (!CheckExtJSON(pUser, sData, ui32Len))
    {
        return false;
    }
    /* TODO
    // PPK ... check flood ...
    if (bCheck && !ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::NODEFLOODMYINFO)) {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION)] != 0) {
            if (DeFloodCheckForFlood(pUser, DefloodTypes::MYINFO, SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION)],
                pUser->m_ui16MyINFOs, pUser->m_ui64MyINFOsTick, SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES)],
                static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME)]))) {
                return false;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION2)] != 0) {
            if (DeFloodCheckForFlood(pUser, DefloodTypes::MYINFO, SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION2)],
                pUser->m_ui16MyINFOs2, pUser->m_ui64MyINFOsTick2, SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES2)],
                static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME2)]))) {
                return false;
            }
        }
    }

    if (ui32Len > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_MYINFO_LEN)])) {
        pUser->SendFormat("DcCommands::ExtJSONDeflood", true, "<%s> %s!|", SettingManager::HubSec(),
    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MYINFO_TOO_LONG)].c_str());

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $ExtJSON from %s (%s) - user closed. (%s)", pUser->m_sNick.c_str(), pUser->m_sIP.data(), sData);

        pUser->Close();
        return false;
    }
    */
    return true;
}
//---------------------------------------------------------------------------
bool DcCommands::CheckExtJSON(User* pUser, const char* sData, const uint32_t ui32Len)
{
    if (!pUser->m_sNick.empty())
    {
        if (ui32Len > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_MYINFO_LEN)]) * 10)
        {
            pUser->SendFormat("DcCommands::CheckExtJSON", true, "<%s> %s!|", "Error", "strlen(ExtJSON) > (MaxMyINFOLen * 10)");
            UdpDebug::m_Ptr->BroadcastFormat(
                "[SYS] Bad $ExtJSON len (%u) from %s (%s) - user closed. (%s)", ui32Len, pUser->m_sNick.c_str(), pUser->m_sIP.data(), sData);
            pUser->Close();
            return false;
        }
        if (ui32Len < pUser->m_sNick.size() + unsigned(9))
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $ExtJSON [1] (%s) from %s (%s) - user closed.", sData, pUser->m_sNick.c_str(), pUser->m_sIP.data());
            pUser->Close();
            return false;
        }
        if ((ui32Len > pUser->m_sNick.size() + unsigned(9)) && memcmp(sData + 9, pUser->m_sNick.c_str(), pUser->m_sNick.size()) != 0)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $ExtJSON [2] (%s) from %s (%s) - user closed.", sData, pUser->m_sNick.c_str(), pUser->m_sIP.data());
            pUser->Close();
            return false;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
// $ExtJSON |
bool DcCommands::SetExtJSON(User* pUser, const char* sData, const uint32_t ui32Len)
{
    if (!CheckExtJSON(pUser, sData, ui32Len))
    {
        return false;
    }
    if (pUser->ComparExtJSON(sData, ui32Len))
    {
        return false;
    }

    pUser->SetExtJSONOriginal(sData, static_cast<uint16_t>(ui32Len));

    LogInfo("[EXTJSON] User sent ExtJSON nick='{}' ip='{}' tag='{}'",
            pUser->m_sNick,
            pUser->m_sIP.data(),
            std::string(pUser->m_sMyInfoLong.data(), pUser->m_sMyInfoLong.size()));

    if (User::isPastLogin(pUser->m_ui8State))
    {
        return false;
    }

    if (!pUser->ProcessRules())
    {
        pUser->Close();
        return false;
    }
    /*
    // PPK ... moved lua here -> another "optimization" ;o)
    ScriptManager::m_Ptr->Arrival(pUser, sData, ui32Len, ScriptManager::MYINFO_ARRIVAL);

    if (User::isPastLogin(pUser->m_ui8State)) {
        return false;
    }
    */
    pUser->m_ui32BoolBits |= User::BIT_PRCSD_EXT_JSON;
    return true;
}

#endif

// $MyINFO $ALL  $ $$$$|
bool DcCommands::MyINFODeflood(DcCommand* pDcCommand)
{
    if (pDcCommand->m_ui32CommandLen < (22u + pDcCommand->m_pUser->m_sNick.size()))
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyINFO (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return false;
    }

    // PPK ... check flood ...
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODMYINFO))
    {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::MYINFO,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION)],
                                     pDcCommand->m_pUser->m_ui16MyINFOs,
                                     pDcCommand->m_pUser->m_ui64MyINFOsTick,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME)])))
            {
                return false;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION2)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::MYINFO,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION2)],
                                     pDcCommand->m_pUser->m_ui16MyINFOs2,
                                     pDcCommand->m_pUser->m_ui64MyINFOsTick2,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES2)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME2)])))
            {
                return false;
            }
        }
    }

    if (pDcCommand->m_ui32CommandLen > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_MYINFO_LEN)]))
    {
        pDcCommand->m_pUser->SendFormat(
            "DcCommands::MyINFODeflood", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MYINFO_TOO_LONG)].c_str());

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyINFO from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

// $MyINFO $ALL  $ $$$$|
bool DcCommands::MyINFO(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdMyInfo);
    // if no change, just return
    // else store MyINFO and perform all checks again
    if (!pDcCommand->m_pUser->m_sMyInfoOriginal.empty()) // PPK ... optimizations
    {
        if (pDcCommand->m_ui32CommandLen == pDcCommand->m_pUser->m_ui16MyInfoOriginalLen &&
            memcmp(pDcCommand->m_pUser->m_sMyInfoOriginal.data() + 14 + pDcCommand->m_pUser->m_sNick.size(),
                   pDcCommand->m_sCommand + 14 + pDcCommand->m_pUser->m_sNick.size(),
                   pDcCommand->m_pUser->m_ui16MyInfoOriginalLen - 14 - pDcCommand->m_pUser->m_sNick.size()) == 0)
        {
            return false;
        }
    }

    // PPK ... guard against uint16_t truncation: a command of exactly 65536 bytes
    // would cast to 0, making UserParseMyInfo read OOB (m_ui16MyInfoOriginalLen - 1u underflows).
    if (pDcCommand->m_ui32CommandLen > UINT16_MAX)
    {
        LogWarn("[SECURITY] User %s (%s): $MyINFO too long (%u bytes) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(),
                   pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_ui32CommandLen);

        pDcCommand->m_pUser->SendFormat(
            "MyINFO1", false, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_MyINFO_IS_CORRUPTED)].c_str());

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) sent oversized $MyINFO (%u bytes) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_ui32CommandLen);

        pDcCommand->m_pUser->Close();
        return false;
    }

    pDcCommand->m_pUser->SetMyInfoOriginal(pDcCommand->m_sCommand, static_cast<uint16_t>(pDcCommand->m_ui32CommandLen));

    if (User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return false;
    }

    if (!pDcCommand->m_pUser->ProcessRules())
    {
        pDcCommand->m_pUser->Close();
        return false;
    }

    // SEND myinfo to others (including me) only if this is
    // a refresh MyINFO event. Else dispatch it in addMe condition
    // of service loop

    // PPK ... moved lua here -> another "optimization" ;o)
    (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::MYINFO_ARRIVAL);

    if (User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

// $MyPass
void DcCommands::MyPass(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdMyPass);
    RegUser* pReg = RegManager::m_Ptr->Find(pDcCommand->m_pUser);
    if (pReg && (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS)
    {
        pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_WAITING_FOR_PASS;
    }
    else
    {
        // We don't send $GetPass!
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] $MyPass without request from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    if (pDcCommand->m_ui32CommandLen < 10 || pDcCommand->m_ui32CommandLen > 73)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyPass from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    bool bBadPass = false;

    if (pReg->m_bPassHash)
    {
        std::array<uint8_t, 64> ui8Hash;

        const size_t szLen = pDcCommand->m_ui32CommandLen - 9;

        if (!HashPassword(pDcCommand->m_sCommand + 8, szLen, ui8Hash.data()) || memcmp(pReg->m_vPassData.data(), ui8Hash.data(), 64) != 0)
        {
            bBadPass = true;
        }
    }
    else
    {
        if (strcmp(reinterpret_cast<const char*>(pReg->m_vPassData.data()), pDcCommand->m_sCommand + 8) != 0)
        {
            bBadPass = true;
        }
    }

    // if password is wrong, close the connection
    if (bBadPass)
    {
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ADVANCED_PASS_PROTECTION)])
        {
            time(&pReg->m_tLastBadPass);
            if (pReg->m_ui8BadPassCount < 255)
            {
                pReg->m_ui8BadPassCount++;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE)] != 0)
        {
            // brute force password protection
            PassBf* PassBfItem = Find(pDcCommand->m_pUser->m_ui128IpHash.data());
            if (!PassBfItem)
            {
                m_PasswdBfCheck.push_front(std::make_unique<PassBf>(pDcCommand->m_pUser->m_ui128IpHash.data()));
            }
            else
            {
                if (PassBfItem->m_iCount == 2)
                {
                    BanItem* pBan = BanManager::m_Ptr->FindFull(pDcCommand->m_pUser->m_ui128IpHash.data());
                    if (!pBan || !((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
                    {
                        const int iRet = snprintf(ServerManager::m_pGlobalBuffer,
                                            ServerManager::m_szGlobalBufferSize,
                                            "3x bad password for nick %s",
                                            pDcCommand->m_pUser->m_sNick.c_str());
                        if (iRet <= 0)
                        {
                            ServerManager::m_pGlobalBuffer[0] = '\0';
                        }

                        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE)] == 1)
                        {
                            (void)BanManager::m_Ptr->BanIp(pDcCommand->m_pUser, nullptr, ServerManager::m_pGlobalBuffer, nullptr, true);
                        }
                        else
                        {
                            (void)BanManager::m_Ptr->TempBanIp(pDcCommand->m_pUser,
                                                               nullptr,
                                                               ServerManager::m_pGlobalBuffer,
                                                               nullptr,
                                                               SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME)] * 60,
                                                               0,
                                                               true);
                        }
                        Remove(PassBfItem);

                        pDcCommand->m_pUser->SendFormat("DcCommands::MyPass1",
                                                        false,
                                                        "<%s> %s.|",
                                                        SettingManager::HubSec(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_IP_BANNED_BRUTE_FORCE_ATTACK)].c_str());
                    }
                    else
                    {
                        pDcCommand->m_pUser->SendFormat(
                            "DcCommands::MyPass2", false, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_IS_BANNED)].c_str());
                    }
                    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REPORT_3X_BAD_PASS)])
                    {
                        GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::MyPass",
                                                                    "<%s> *** %s %s %s %s|",
                                                                    SettingManager::HubSec(),
                                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                                    pDcCommand->m_pUser->m_sIP.data(),
                                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_BECAUSE_3X_BAD_PASS_FOR_NICK)].c_str(),
                                                                    pDcCommand->m_pUser->m_sNick.c_str());
                    }

                    UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad 3x password from %s (%s) - user banned. (%s)",
                                                     pDcCommand->m_pUser->m_sNick.c_str(),
                                                     pDcCommand->m_pUser->m_sIP.data(),
                                                     pDcCommand->m_sCommand);

                    pDcCommand->m_pUser->Close();
                    return;
                }

                PassBfItem->m_iCount++;
            }
        }

        pDcCommand->m_pUser->SendFormat(
            "DcCommands::MyPass3", false, "$BadPass|<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_INCORRECT_PASSWORD)].c_str());

        m_ui32StatBruteforce++;
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad password from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_pUser->m_i32Profile = static_cast<int32_t>(pReg->m_ui16Profile);

    pReg->m_ui8BadPassCount = 0;

    PassBf* PassBfItem = Find(pDcCommand->m_pUser->m_ui128IpHash.data());
    if (PassBfItem)
    {
        Remove(PassBfItem);
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|'; // add back pipe

    // PPK ... Lua DataArrival only if pass is ok
    (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::PASSWORD_ARRIVAL);

    if (User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    if (ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::HASKEYICON))
    {
        pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_OPERATOR;
    }
    else
    {
        pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
    }

    // PPK ... addition for registered users, kill your own ghost >:-]
    if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HASHED) == User::BIT_HASHED))
    {
        User* OtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_pUser->m_sNick);
        if (OtherUser)
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Ghost %s (%s) closed.", OtherUser->m_sNick.c_str(), OtherUser->m_sIP.data());

            if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
            {
                OtherUser->Close();
            }
            else
            {
                OtherUser->Close(true);
            }
        }
        if (!HashManager::m_Ptr->Add(pDcCommand->m_pUser))
        {
            return;
        }
        pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HASHED;
    }
    if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
    {
        // welcome the new user
        // PPK ... fixed bad DC protocol implementation, $LogedIn is only for OPs !!!
        // registered DC1 users have enabled OP menu :)))))))))
        if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            pDcCommand->m_pUser->SendFormat(
                "DcCommands::MyPass4", true, "$Hello %s|$LogedIn %s|", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sNick.c_str());
        }
        else
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::MyPass5", true, "$Hello %s|", pDcCommand->m_pUser->m_sNick.c_str());
        }

        return;
    }
    else
    {
        if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
        {
            pDcCommand->m_pUser->AddMeOrIPv4Check();
        }
    }
}
//---------------------------------------------------------------------------

// $OpForceMove $Who:<nickname>$Where:<iptoredirect>$Msg:<a message>
void DcCommands::OpForceMove(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdOpForceMove);
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::REDIRECT))
    {
        pDcCommand->m_pUser->SendFormat("DcCommands::OpForceMove1",
                                        true,
                                        "<%s> %s!|",
                                        SettingManager::HubSec(),
                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD)].c_str());
        return;
    }

    if (pDcCommand->m_ui32CommandLen < 31)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $OpForceMove (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::OPFORCEMOVE_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    std::array<char*, 3> sCmdParts = {nullptr, nullptr, nullptr};
    std::array<uint16_t, 3> iCmdPartsLen = {0, 0, 0};

    uint8_t cPart = 0;

    sCmdParts[cPart] = pDcCommand->m_sCommand + 18; // nick start

    for (uint32_t ui32i = 18; ui32i < pDcCommand->m_ui32CommandLen; ui32i++)
    {
        if (pDcCommand->m_sCommand[ui32i] == '$')
        {
            pDcCommand->m_sCommand[ui32i] = '\0';
            iCmdPartsLen[cPart] = static_cast<uint16_t>((pDcCommand->m_sCommand + ui32i) - sCmdParts[cPart]);

            // are we on last $ ???
            if (cPart == 1)
            {
                sCmdParts[2] = pDcCommand->m_sCommand + ui32i + 1;
                iCmdPartsLen[2] = static_cast<uint16_t>(pDcCommand->m_ui32CommandLen - ui32i - 1);
                break;
            }

            cPart++;
            sCmdParts[cPart] = pDcCommand->m_sCommand + ui32i + 1;
        }
    }

    if (iCmdPartsLen[0] == 0 || iCmdPartsLen[1] < 7 || iCmdPartsLen[2] < 5 || iCmdPartsLen[1] > 4096 || iCmdPartsLen[2] > 16384)
    {
        return;
    }

    User* OtherUser = HashManager::m_Ptr->FindUser(std::string_view(sCmdParts[0], iCmdPartsLen[0]));
    if (OtherUser)
    {
        // Self redirect
        if (OtherUser == pDcCommand->m_pUser)
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::OpForceMove2",
                                            true,
                                            "<%s> %s!|",
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_CANT_REDIRECT_YOURSELF)].c_str());
            return;
        }

        if (OtherUser->m_i32Profile != -1 && pDcCommand->m_pUser->m_i32Profile > OtherUser->m_i32Profile)
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::OpForceMove3",
                                            true,
                                            "<%s> %s %s|",
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALLOWED_TO_REDIRECT)].c_str(),
                                            OtherUser->m_sNick.c_str());
            return;
        }

        OtherUser->SendFormat("DcCommands::OpForceMove4",
                              false,
                              "<%s> %s %s %s %s. %s: %s|$ForceMove %s|",
                              SettingManager::HubSec(),
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_REDIRECTED_TO)].c_str(),
                              sCmdParts[1] + 6,
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                              pDcCommand->m_pUser->m_sNick.c_str(),
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE)].c_str(),
                              sCmdParts[2] + 4,
                              sCmdParts[1] + 6);

        // PPK ... close user !!!
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] User %s (%s) redirected by %s", OtherUser->m_sNick.c_str(), OtherUser->m_sIP.data(), pDcCommand->m_pUser->m_sNick.c_str());

        OtherUser->Close();

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::OpForceMove",
                                                        "<%s> *** %s %s %s %s %s. %s: %s|",
                                                        SettingManager::HubSec(),
                                                        OtherUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_REDIRECTED_TO)].c_str(),
                                                        sCmdParts[1] + 6,
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                        pDcCommand->m_pUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE)].c_str(),
                                                        sCmdParts[2] + 4);
        }

        if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)] ||
            !((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::OpForceMove4",
                                            true,
                                            "<%s> *** %s %s %s. %s: %s|",
                                            SettingManager::HubSec(),
                                            OtherUser->m_sNick.c_str(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_REDIRECTED_TO)].c_str(),
                                            sCmdParts[1] + 6,
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE)].c_str(),
                                            sCmdParts[2] + 4);
        }
    }
}
//---------------------------------------------------------------------------

// $RevConnectToMe <ownnick> <nickname>
void DcCommands::RevConnectToMe(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdRevConnectToMe);
    if (pDcCommand->m_ui32CommandLen < 19)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $RevConnectToMe (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... optimizations
    if ((pDcCommand->m_sCommand[16 + pDcCommand->m_pUser->m_sNick.size()] != ' ') ||
        (memcmp(pDcCommand->m_sCommand + 16, pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sNick.size()) != 0))
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in RCTM from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        DcCommands::m_Ptr->m_ui32StatNickSpoofing++;
        pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODRCTM))
    {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::RCTM,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION)],
                                     pDcCommand->m_pUser->m_ui16RCTMs,
                                     pDcCommand->m_pUser->m_ui64RCTMsTick,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_MESSAGES)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_TIME)])))
            {
                return;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION2)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::RCTM,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION2)],
                                     pDcCommand->m_pUser->m_ui16RCTMs2,
                                     pDcCommand->m_pUser->m_ui64RCTMsTick2,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_MESSAGES2)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_RCTM_TIME2)])))
            {
                return;
            }
        }
    }

    if (pDcCommand->m_ui32CommandLen > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_RCTM_LEN)]))
    {
        pDcCommand->m_pUser->SendFormat(
            "DcCommands::RevConnectToMe", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RCTM_TOO_LONG)].c_str());

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Long $RevConnectToMe from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... $RCTM means user is pasive ?!? Probably yes, let set it not active and use on another places ;)
    if (pDcCommand->m_pUser->m_sTag.empty())
    {
        pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::REVCONNECTTOME_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    User* OtherUser = HashManager::m_Ptr->FindUser(std::string_view(pDcCommand->m_sCommand + 17 + pDcCommand->m_pUser->m_sNick.size(),
                                                                    pDcCommand->m_ui32CommandLen - (18 + pDcCommand->m_pUser->m_sNick.size())));
    // PPK ... no connection to yourself !!!
    if (OtherUser && OtherUser != pDcCommand->m_pUser && OtherUser->m_ui8State == User::UserStates::STATE_ADDED)
    {
        pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|'; // add back pipe
        pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, OtherUser);
    }
}
//---------------------------------------------------------------------------

// $SR <nickname> - Search Respond for passive users
void DcCommands::SR(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdSR);
    if (pDcCommand->m_ui32CommandLen < 6u + pDcCommand->m_pUser->m_sNick.size())
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Bad $SR (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODSR))
    {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_ACTION)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::SR,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_ACTION)],
                                     pDcCommand->m_pUser->m_ui16SRs,
                                     pDcCommand->m_pUser->m_ui64SRsTick,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_MESSAGES)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_TIME)])))
            {
                return;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_ACTION2)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::SR,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_ACTION2)],
                                     pDcCommand->m_pUser->m_ui16SRs2,
                                     pDcCommand->m_pUser->m_ui64SRsTick2,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_MESSAGES2)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SR_TIME2)])))
            {
                return;
            }
        }
    }

    if (pDcCommand->m_ui32CommandLen > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SR_LEN)]))
    {
        pDcCommand->m_pUser->SendFormat(
            "DcCommands::SR", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SR_TOO_LONG)].c_str());

        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Long $SR from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    // check $SR spoofing (thanx Fusbar)
    // PPK... added checking for empty space after nick
    if (pDcCommand->m_sCommand[4 + pDcCommand->m_pUser->m_sNick.size()] != ' ' ||
        memcmp(pDcCommand->m_sCommand + 4, pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sNick.size()) != 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in SR from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        DcCommands::m_Ptr->m_ui32StatNickSpoofing++;
        pDcCommand->m_pUser->Close();
        return;
    }

    // past SR to script only if it's not a data for SlotFetcher
    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::SR_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    char* pToNick = strrchr(pDcCommand->m_sCommand, '\5');
    if (!pToNick)
        return;

    User* OtherUser = HashManager::m_Ptr->FindUser(std::string_view(pToNick + 1, pDcCommand->m_ui32CommandLen - 2 - (pToNick - pDcCommand->m_sCommand)));
    // PPK ... no $SR to yourself !!!
    if (OtherUser && OtherUser != pDcCommand->m_pUser && OtherUser->m_ui8State == User::UserStates::STATE_ADDED)
    {
        // PPK ... search replies limiting
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PASIVE_SR)] != 0)
        {
            if (OtherUser->m_ui32SR >= static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PASIVE_SR)]))
            {
                return;
            }

            OtherUser->m_ui32SR++;
        }

        // cutoff the last part // PPK ... and do it fast ;)
        pToNick[0] = '|';
        pToNick[1] = '\0';
        pDcCommand->m_pUser->AddPrcsdCmd(
            PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen - OtherUser->m_sNick.size() - 1, OtherUser);
    }
}

//---------------------------------------------------------------------------
#ifdef FLYLINKDC_USE_UDP_THREAD
// $SR <nickname> - Search Respond for active users from UDP
void DcCommands::SRFromUDP(DcCommand* pDcCommand)
{
    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::UDP_SR_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }
}
#endif
//---------------------------------------------------------------------------

// $Supports item item item... PPK $Supports UserCommand NoGetINFO NoHello UserIP2 QuickList|
void DcCommands::Supports(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdSupports);
    if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS))
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] $Supports flood from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        m_ui32StatFlood++;
        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_SUPPORTS;

    if (pDcCommand->m_ui32CommandLen < 13)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Supports (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    if (pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 2] == ' ')
    {
        if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_NO_QUACK_SUPPORTS)])
        {
            pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_QUACK_SUPPORTS;
        }
        else
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Quack $Supports from %s (%s) - user closed. (%s)",
                                             pDcCommand->m_pUser->m_sNick.c_str(),
                                             pDcCommand->m_pUser->m_sIP.data(),
                                             pDcCommand->m_sCommand);

            pDcCommand->m_pUser->SendFormat(
                "DcCommands::Supports1", false, "<%s> %s.|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_QUACK_SUPPORTS)].c_str());

            pDcCommand->m_pUser->Close();
            return;
        }
    }

    (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::SUPPORTS_ARRIVAL);

    if (User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    char* sSupport = pDcCommand->m_sCommand + 10;
    size_t szDataLen;

    for (uint32_t ui32i = 10; ui32i < pDcCommand->m_ui32CommandLen - 1; ui32i++)
    {
        if (pDcCommand->m_sCommand[ui32i] == ' ')
        {
            pDcCommand->m_sCommand[ui32i] = '\0';
        }
        else if (ui32i != pDcCommand->m_ui32CommandLen - 2)
        {
            continue;
        }
        else
        {
            ui32i++;
        }

        szDataLen = (pDcCommand->m_sCommand + ui32i) - sSupport;

        switch (sSupport[0])
        {
        case 'N':
            if (sSupport[1] == 'o')
            {
                if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) && szDataLen == 7 &&
                    MatchBytes(sSupport + 2, "Hello"))
                {
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
                }
                else if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) && szDataLen == 9 &&
                         MatchBytes(sSupport + 2, "GetINFO"))
                {
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOGETINFO;
                }
            }
            break;
        case 'Q':
        {
            if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) && szDataLen == 9 &&
                MatchBytes(sSupport + 1, "uickList"))
            {
                pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_QUICKLIST;
                // PPK ... in fact NoHello is only not fully implemented Quicklist (without diferent login sequency)
                // That's why i overide NoHello here and use bQuicklist only for login, on other places is same as NoHello ;)
                pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
            }
            break;
        }
        case 'H':
        {
            if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_HUBURL) == User::SUPPORTBIT_HUBURL) && szDataLen == 6 &&
                MatchBytes(sSupport + 1, "ubURL"))
            {
                pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_HUBURL;
            }
            break; // fix PVS Studio
        }
        case 'U':
        {
            if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND) && szDataLen == 11 &&
                MatchBytes(sSupport + 1, "serCommand"))
            {
                pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_USERCOMMAND;
            }
            else if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) && szDataLen == 7 &&
                     MatchBytes(sSupport + 1, "serIP2"))
            {
                pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_USERIP2;
            }
            break;
        }
        case 'B':
        {
            if (!((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) && szDataLen == 7 && MatchBytes(sSupport + 1, "otINFO"))
            {
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_DONT_ALLOW_PINGERS)])
                {
                    pDcCommand->m_pUser->SendFormat("DcCommands::Supports2",
                                                    false,
                                                    "<%s> %s.|",
                                                    SettingManager::HubSec(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_THIS_HUB_NOT_ALLOW_PINGERS)].c_str());
                    pDcCommand->m_pUser->Close();
                    return;
                }

                pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_PINGER;
                pDcCommand->m_pUser->SendFormat("DcCommands::Supports4",
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
            }
            break;
        }
        case 'Z':
        {
            if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE))
            {
                if (szDataLen == 6 && MatchBytes(sSupport + 1, "Pipe0"))
                {
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_ZPIPE0;
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_ZPIPE;
                    m_ui32StatZPipe++;
                }
                else if (szDataLen == 5 && MatchBytes(sSupport + 1, "Pipe"))
                {
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_ZPIPE;
                    m_ui32StatZPipe++;
                }
            }
            break;
        }
        case 'I':
        {
            if (szDataLen == 4)
            {
                if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) && MatchBytes(sSupport, "IP64"))
                {
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_IP64;
                }
                else if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) && MatchBytes(sSupport, "IPv4"))
                {
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_IPV4;
                }
            }
            break;
        }
        case 'T':
        {
            if (szDataLen == 4)
            {
                if (!((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) && MatchBytes(sSupport, "TLS2"))
                {
                    pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_TLS2;
                }
            }
            break;
        }
#ifdef USE_FLYLINKDC_EXT_JSON
        case 'E':
        {
            if (szDataLen == 8)
            {
                if (!pDcCommand->m_pUser->isSupportExtJSON() && MatchBytes(sSupport + 1, "xtJSON2"))
                {
                    {
                        std::string rawSupports(pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen);
                        for (size_t pos = 0; (pos = rawSupports.find('\0', pos)) != std::string::npos; pos++)
                        {
                            rawSupports[pos] = ' ';
                        }
                        LogInfo("[EXTJSON] ExtJSON2 detected nick='{}' ip='{}' raw='{}'",
                                pDcCommand->m_pUser->m_sNick,
                                pDcCommand->m_pUser->m_sIP.data(),
                                rawSupports);
                    }
                    pDcCommand->m_pUser->m_is_json_user = true; // |= User::SUPPORTBIT_EXTJSON2;
                }
            }
            break;
        }
#endif
        case '\0':
        {
            // PPK ... corrupted $Supports ???
            UdpDebug::m_Ptr->BroadcastFormat(
                "[SYS] Bad $Supports from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

            pDcCommand->m_pUser->Close();
            return;
        }
        default:
            // PPK ... unknown supports
            break;
        }

        sSupport = pDcCommand->m_sCommand + ui32i + 1;
    }

    pDcCommand->m_pUser->m_ui8State = User::UserStates::STATE_VALIDATE;

    pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::SUPPORTS, nullptr, 0, nullptr);
}
//---------------------------------------------------------------------------

// $To: nickname From: ownnickname $<ownnickname> <message>
void DcCommands::To(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdTo);
    char* pTemp = strchr(pDcCommand->m_sCommand + 5, ' ');

    if (pDcCommand->m_ui32CommandLen < 19 || !pTemp)
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] Bad To from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data(), pDcCommand->m_sCommand);

        pDcCommand->m_pUser->Close();
        return;
    }

    const size_t szNickLen = pTemp - (pDcCommand->m_sCommand + 5);

    if (szNickLen > 64)
    {
        pDcCommand->m_pUser->SendFormat(
            "DcCommands::To1", true, "<%s> *** %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return;
    }

    // is the mesg really from us ?
    // PPK ... replaced by better and faster code ;)
    const int iRet = snprintf(ServerManager::m_pGlobalBuffer,
                        ServerManager::m_szGlobalBufferSize,
                        "From: %s $<%s> ",
                        pDcCommand->m_pUser->m_sNick.c_str(),
                        pDcCommand->m_pUser->m_sNick.c_str());
    if (iRet <= 0 || strncmp(pTemp + 1, ServerManager::m_pGlobalBuffer, iRet) != 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in To from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        DcCommands::m_Ptr->m_ui32StatNickSpoofing++;
        pDcCommand->m_pUser->Close();
        return;
    }

    // FloodCheck
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODPM))
    {
        // PPK ... pm antiflood
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_ACTION)] != 0)
        {
            pTemp[0] = '\0';
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::PM,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_ACTION)],
                                     pDcCommand->m_pUser->m_ui16PMs,
                                     pDcCommand->m_pUser->m_ui64PMsTick,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_MESSAGES)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_TIME)]),
                                     pDcCommand->m_sCommand + 5))
            {
                return;
            }
            pTemp[0] = ' ';
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_ACTION2)] != 0)
        {
            pTemp[0] = '\0';
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::PM,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_ACTION2)],
                                     pDcCommand->m_pUser->m_ui16PMs2,
                                     pDcCommand->m_pUser->m_ui64PMsTick2,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_MESSAGES2)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_TIME2)]),
                                     pDcCommand->m_sCommand + 5))
            {
                return;
            }
            pTemp[0] = ' ';
        }

        // 2nd check for PM flooding
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_PM_ACTION)] != 0)
        {
            bool bNewData = false;
            pTemp[0] = '\0';
            if (DeFloodCheckForSameFlood(pDcCommand->m_pUser,
                                         DefloodTypes::SAME_PM,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_PM_ACTION)],
                                         pDcCommand->m_pUser->m_ui16SamePMs,
                                         pDcCommand->m_pUser->m_ui64SamePMsTick,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_PM_MESSAGES)],
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_PM_TIME)],
                                         pTemp + (12 + (2 * pDcCommand->m_pUser->m_sNick.size())),
                                         (pDcCommand->m_ui32CommandLen - (pTemp - pDcCommand->m_sCommand)) - (12 + (2 * pDcCommand->m_pUser->m_sNick.size())),
                                         pDcCommand->m_pUser->m_sLastPM.c_str(),
                                         pDcCommand->m_pUser->m_sLastPM.size(),
                                         bNewData,
                                         pDcCommand->m_sCommand + 5))
            {
                return;
            }
            pTemp[0] = ' ';

            if (bNewData)
            {
                pDcCommand->m_pUser->SetLastPM(
                    std::string_view(pTemp + (12 + (2 * pDcCommand->m_pUser->m_sNick.size())),
                                     (pDcCommand->m_ui32CommandLen - (pTemp - pDcCommand->m_sCommand)) - (12 + (2 * pDcCommand->m_pUser->m_sNick.size()))));
            }
        }
    }

    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOCHATLIMITS))
    {
        // 1st check for length limit for PM message
        const size_t szMessLen = pDcCommand->m_ui32CommandLen - (2 * pDcCommand->m_pUser->m_sNick.size()) - (pTemp - pDcCommand->m_sCommand) - 13;
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LEN)] != 0 &&
            szMessLen > static_cast<size_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LEN)]))
        {
            // PPK ... hubsec alias
            pTemp[0] = '\0';
            pDcCommand->m_pUser->SendFormat("DcCommands::To2",
                                            true,
                                            "$To: %s From: %s $<%s> %s!|",
                                            pDcCommand->m_pUser->m_sNick.c_str(),
                                            pDcCommand->m_sCommand + 5,
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THE_MESSAGE_WAS_TOO_LONG)].c_str());
            return;
        }

        // PPK ... check for message lines limit
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LINES)] != 0 || SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_ACTION)] != 0)
        {
            if (pDcCommand->m_pUser->m_ui16SamePMs < 2)
            {
                uint16_t iLines = 1;
                for (uint32_t ui32i = 9 + pDcCommand->m_pUser->m_sNick.size(); ui32i < pDcCommand->m_ui32CommandLen - (pTemp - pDcCommand->m_sCommand) - 1;
                     ui32i++)
                {
                    if (pTemp[ui32i] == '\n')
                    {
                        iLines++;
                    }
                }
                pDcCommand->m_pUser->m_ui16LastPmLines = iLines;
                if (pDcCommand->m_pUser->m_ui16LastPmLines > 1)
                {
                    pDcCommand->m_pUser->m_ui16SameMultiPms++;
                }
            }
            else if (pDcCommand->m_pUser->m_ui16LastPmLines > 1)
            {
                pDcCommand->m_pUser->m_ui16SameMultiPms++;
            }

            if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODPM) &&
                SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_ACTION)] != 0)
            {
                if (pDcCommand->m_pUser->m_ui16SameMultiPms > SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_MESSAGES)] &&
                    pDcCommand->m_pUser->m_ui16LastPmLines >= SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_LINES)])
                {
                    pTemp[0] = '\0';
                    uint16_t lines = 0;
                    DeFloodDoAction(pDcCommand->m_pUser,
                                    DefloodTypes::SAME_MULTI_PM,
                                    SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_ACTION)],
                                    lines,
                                    pDcCommand->m_sCommand + 5);
                    return;
                }
            }

            if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LINES)] != 0 &&
                pDcCommand->m_pUser->m_ui16LastPmLines > SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LINES)])
            {
                pTemp[0] = '\0';
                pDcCommand->m_pUser->SendFormat("DcCommands::To3",
                                                true,
                                                "$To: %s From: %s $<%s> %s!|",
                                                pDcCommand->m_pUser->m_sNick.c_str(),
                                                pDcCommand->m_sCommand + 5,
                                                SettingManager::HubSec(),
                                                LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THE_MESSAGE_WAS_TOO_LONG)].c_str());
                return;
            }
        }
    }

    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOPMINTERVAL))
    {
        pTemp[0] = '\0';
        if (DeFloodCheckInterval(pDcCommand->m_pUser,
                                 DefloodTypes::INTERVAL_PM,
                                 pDcCommand->m_pUser->m_ui16PMsInt,
                                 pDcCommand->m_pUser->m_ui64PMsIntTick,
                                 SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_INTERVAL_MESSAGES)],
                                 static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_PM_INTERVAL_TIME)]),
                                 pDcCommand->m_sCommand + 5))
        {
            return;
        }
        pTemp[0] = ' ';
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::TO_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pTemp[0] = '\0';

    // ignore the silly debug messages !!!
    if (memcmp(pDcCommand->m_sCommand + 5, "Vandel\\Debug", 12) == 0)
    {
        return;
    }

    // Everything's ok lets chat
    // if this is a PM to OpChat or Hub bot, process the message
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] && strcmp(pDcCommand->m_sCommand + 5, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str()) == 0)
    {
        pTemp += 9 + pDcCommand->m_pUser->m_sNick.size();
        // PPK ... check message length, return if no mess found
        const auto ui32Len1 = static_cast<uint32_t>((pDcCommand->m_ui32CommandLen - (pTemp - pDcCommand->m_sCommand)) + 1);
        if (ui32Len1 <= pDcCommand->m_pUser->m_sNick.size() + 4u)
        {
            return;
        }

        // find chat message data
        char* sBuff = pTemp + pDcCommand->m_pUser->m_sNick.size() + 3;

        // non-command chat msg
        for (const char prefix : SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)])
        {
            if (sBuff[0] == prefix)
            {
                pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe
                // built-in commands
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_TEXT_FILES)] &&
                    TextFilesManager::m_Ptr->ProcessTextFilesCmd(pDcCommand->m_pUser, sBuff + 1, true))
                {
                    return;
                }

                // HubCommands
                if (ui32Len1 - pDcCommand->m_pUser->m_sNick.size() >= 10)
                {
                    if (HubCommands::DoCommand(pDcCommand->m_pUser, sBuff, ui32Len1, true))
                        return;
                }

                pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|'; // add back pipe
                break;
            }
        }
        // PPK ... if i am here is not textfile request or hub command, try opchat
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] && ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::ALLOWEDOPCHAT) &&
            SettingManager::m_Ptr->m_bBotsSameNick)
        {
            const uint32_t iOpChatLen = SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].size();
            memcpy(pTemp - iOpChatLen - 2, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].data(), iOpChatLen);
            pDcCommand->m_pUser->AddPrcsdCmd(
                PrcsdUsrCmd::TO_OP_CHAT, pTemp - iOpChatLen - 2, pDcCommand->m_ui32CommandLen - ((pTemp - iOpChatLen - 2) - pDcCommand->m_sCommand), nullptr);
        }
    }
    else if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] && ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::ALLOWEDOPCHAT) &&
             strcmp(pDcCommand->m_sCommand + 5, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str()) == 0)
    {
        pTemp += 9 + pDcCommand->m_pUser->m_sNick.size();
        const uint32_t iOpChatLen = SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].size();
        memcpy(pTemp - iOpChatLen - 2, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str(), iOpChatLen);
        pDcCommand->m_pUser->AddPrcsdCmd(
            PrcsdUsrCmd::TO_OP_CHAT, pTemp - iOpChatLen - 2, (pDcCommand->m_ui32CommandLen - (pTemp - pDcCommand->m_sCommand)) + iOpChatLen + 2, nullptr);
    }
    else
    {
        User* OtherUser = HashManager::m_Ptr->FindUser(std::string_view(pDcCommand->m_sCommand + 5, szNickLen));
        // PPK ... pm to yourself ?!? NO!
        if (OtherUser && OtherUser != pDcCommand->m_pUser && OtherUser->m_ui8State == User::UserStates::STATE_ADDED)
        {
            pTemp[0] = ' ';
            pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, OtherUser, true);
        }
    }
}
//---------------------------------------------------------------------------

// $ValidateNick
void DcCommands::ValidateNick(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdValidateNick);
    if (((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] $ValidateNick with QuickList support from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    if (pDcCommand->m_ui32CommandLen < 16)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Attempt to Validate empty nick (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe
    if (!ValidateUserNick(pDcCommand, pDcCommand->m_pUser, pDcCommand->m_sCommand + 14, pDcCommand->m_ui32CommandLen - 15, true))
        return;

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|'; // add back pipe

    (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::VALIDATENICK_ARRIVAL);
}
//---------------------------------------------------------------------------

// $Version
void DcCommands::Version(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdVersion);
    if (pDcCommand->m_ui32CommandLen < 11)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Version (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    pDcCommand->m_pUser->m_ui8State = User::UserStates::STATE_GETNICKLIST_OR_MYINFO;

    (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::VERSION_ARRIVAL);

    if (User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe
    pDcCommand->m_pUser->SetVersion(pDcCommand->m_sCommand + 9);
}
//---------------------------------------------------------------------------

// Chat message
bool DcCommands::ChatDeflood(DcCommand* pDcCommand)
{

    // if the user is sending chat as other user, kick him
    if (pDcCommand->m_sCommand[1 + pDcCommand->m_pUser->m_sNick.size()] != '>' || pDcCommand->m_sCommand[2 + pDcCommand->m_pUser->m_sNick.size()] != ' ' ||
        memcmp(pDcCommand->m_pUser->m_sNick.c_str(), pDcCommand->m_sCommand + 1, pDcCommand->m_pUser->m_sNick.size()) != 0)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in chat from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        DcCommands::m_Ptr->m_ui32StatNickSpoofing++;
        pDcCommand->m_pUser->Close();
        return false;
    }

    // PPK ... check flood...
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODMAINCHAT))
    {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::CHAT,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION)],
                                     pDcCommand->m_pUser->m_ui16ChatMsgs,
                                     pDcCommand->m_pUser->m_ui64ChatMsgsTick,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_MESSAGES)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_TIME)])))
            {
                return false;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION2)] != 0)
        {
            if (DeFloodCheckForFlood(pDcCommand->m_pUser,
                                     DefloodTypes::CHAT,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION2)],
                                     pDcCommand->m_pUser->m_ui16ChatMsgs2,
                                     pDcCommand->m_pUser->m_ui64ChatMsgsTick2,
                                     SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_MESSAGES2)],
                                     static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_TIME2)])))
            {
                return false;
            }
        }

        // 2nd check for chatmessage flood
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_ACTION)] != 0)
        {
            bool bNewData = false;
            if (DeFloodCheckForSameFlood(pDcCommand->m_pUser,
                                         DefloodTypes::SAME_CHAT,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_ACTION)],
                                         pDcCommand->m_pUser->m_ui16SameChatMsgs,
                                         pDcCommand->m_pUser->m_ui64SameChatsTick,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_MESSAGES)],
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_TIME)],
                                         pDcCommand->m_sCommand + pDcCommand->m_pUser->m_sNick.size() + 3,
                                         pDcCommand->m_ui32CommandLen - (pDcCommand->m_pUser->m_sNick.size() + 3),
                                         pDcCommand->m_pUser->m_sLastChat.c_str(),
                                         pDcCommand->m_pUser->m_sLastChat.size(),
                                         bNewData))
            {
                return false;
            }

            if (bNewData)
            {
                pDcCommand->m_pUser->SetLastChat(std::string_view(pDcCommand->m_sCommand + pDcCommand->m_pUser->m_sNick.size() + 3,
                                                                  pDcCommand->m_ui32CommandLen - (pDcCommand->m_pUser->m_sNick.size() + 3)));
            }
        }
    }

    // PPK ... ignore empty chat ;)
    if (pDcCommand->m_ui32CommandLen < pDcCommand->m_pUser->m_sNick.size() + 5u)
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

// Chat message
void DcCommands::Chat(DcCommand* pDcCommand)
{
    LogInfo(
        "[CHAT] nick='{}' msg='{}' len={}",
        pDcCommand->m_pUser->m_sNick,
        std::string(pDcCommand->m_sCommand + pDcCommand->m_pUser->m_sNick.size() + 3, pDcCommand->m_ui32CommandLen - pDcCommand->m_pUser->m_sNick.size() - 4),
        pDcCommand->m_ui32CommandLen);

    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdChat);
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOCHATLIMITS))
    {
        // PPK ... check for message limit length
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LEN)] != 0 &&
            (pDcCommand->m_ui32CommandLen - pDcCommand->m_pUser->m_sNick.size() - 4) >
                static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LEN)]))
        {
            pDcCommand->m_pUser->SendFormat(
                "DcCommands::Chat1", true, "<%s> %s !|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THE_MESSAGE_WAS_TOO_LONG)].c_str());
            return;
        }

        // PPK ... check for message lines limit
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LINES)] != 0 || SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION)] != 0)
        {
            if (pDcCommand->m_pUser->m_ui16SameChatMsgs < 2)
            {
                uint16_t iLines = 1;

                for (uint32_t ui32i = pDcCommand->m_pUser->m_sNick.size() + 3; ui32i < pDcCommand->m_ui32CommandLen - 1; ui32i++)
                {
                    if (pDcCommand->m_sCommand[ui32i] == '\n')
                    {
                        iLines++;
                    }
                }

                pDcCommand->m_pUser->m_ui16LastChatLines = iLines;

                if (pDcCommand->m_pUser->m_ui16LastChatLines > 1)
                {
                    pDcCommand->m_pUser->m_ui16SameMultiChats++;
                }
            }
            else if (pDcCommand->m_pUser->m_ui16LastChatLines > 1)
            {
                pDcCommand->m_pUser->m_ui16SameMultiChats++;
            }

            if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODMAINCHAT) &&
                SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION)] != 0)
            {
                if (pDcCommand->m_pUser->m_ui16SameMultiChats > SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES)] &&
                    pDcCommand->m_pUser->m_ui16LastChatLines >= SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_LINES)])
                {
                    uint16_t lines = 0;
                    DeFloodDoAction(pDcCommand->m_pUser,
                                    DefloodTypes::SAME_MULTI_CHAT,
                                    SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION)],
                                    lines,
                                    nullptr);
                    return;
                }
            }

            if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LINES)] != 0 &&
                pDcCommand->m_pUser->m_ui16LastChatLines > SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LINES)])
            {
                pDcCommand->m_pUser->SendFormat(
                    "DcCommands::Chat2", true, "<%s> %s !|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THE_MESSAGE_WAS_TOO_LONG)].c_str());
                return;
            }
        }
    }

    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOCHATINTERVAL))
    {
        if (DeFloodCheckInterval(pDcCommand->m_pUser,
                                 DefloodTypes::INTERVAL_CHAT,
                                 pDcCommand->m_pUser->m_ui16ChatIntMsgs,
                                 pDcCommand->m_pUser->m_ui64ChatIntMsgsTick,
                                 SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CHAT_INTERVAL_MESSAGES)],
                                 static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_CHAT_INTERVAL_TIME)])))
        {
            return;
        }
    }

    if (((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED))
    {
        return;
    }

    auto* const pQueueItem1 = GlobalDataQueue::m_Ptr->GetLastQueueItem();

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::CHAT_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    GlobalDataQueue::QueueItem* pQueueItem = nullptr;
    auto* const pQueueItem2 = GlobalDataQueue::m_Ptr->GetLastQueueItem();

    if (pQueueItem1 != pQueueItem2)
    {
        if (!pQueueItem1)
        {
            pQueueItem = GlobalDataQueue::m_Ptr->InsertBlankQueueItem(GlobalDataQueue::m_Ptr->GetFirstQueueItem(), GlobalDataQueue::Cmd::CHAT);
        }
        else
        {
            pQueueItem = GlobalDataQueue::m_Ptr->InsertBlankQueueItem(pQueueItem1, GlobalDataQueue::Cmd::CHAT);
        }

        if (pQueueItem)
        {
            pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_CHAT_INSERT;
        }
    }
    else if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_CHAT_INSERT) == User::BIT_CHAT_INSERT)
    {
        pQueueItem = GlobalDataQueue::m_Ptr->InsertBlankQueueItem(pQueueItem1, GlobalDataQueue::Cmd::CHAT);
    }

    // PPK ... filtering kick messages
    if (ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::KICK))
    {
        if (pDcCommand->m_ui32CommandLen > pDcCommand->m_pUser->m_sNick.size() + 21u)
        {
            char* pTemp = strchr(pDcCommand->m_sCommand + pDcCommand->m_pUser->m_sNick.size() + 3, '\n');
            if (pTemp)
            {
                pTemp[0] = '\0';
            }

            char* pIsKicking = stristr(pDcCommand->m_sCommand + pDcCommand->m_pUser->m_sNick.size() + 3, "is kicking ");
            if (pIsKicking)
            {
                char* pBecause = stristr(pIsKicking + 12, " because: ");
                if (pBecause)
                {
                    // PPK ... catch kick message and store for later use in $Kick for tempban reason
                    pBecause[0] = '\0';
                    User* KickedUser = HashManager::m_Ptr->FindUser(std::string_view(pIsKicking + 11, pBecause - (pIsKicking + 11)));
                    pBecause[0] = ' ';
                    if (KickedUser)
                    {
                        // PPK ... valid kick messages for existing user, remove this message from deflood ;)
                        if (pDcCommand->m_pUser->m_ui16ChatMsgs != 0)
                        {
                            pDcCommand->m_pUser->m_ui16ChatMsgs--;
                            pDcCommand->m_pUser->m_ui16ChatMsgs2--;
                        }
                        if (pBecause[10] != '|')
                        {
                            pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // get rid of the pipe
                            KickedUser->SetBuffer(pBecause + 10, pDcCommand->m_ui32CommandLen - (pBecause - pDcCommand->m_sCommand) - 11);
                            pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|'; // add back pipe
                        }
                    }

                    if (pTemp)
                    {
                        pTemp[0] = '\n';
                    }

                    // PPK ... kick messages filtering
                    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_FILTER_KICK_MESSAGES)])
                    {
                        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_KICK_MESSAGES_TO_OPS)])
                        {
                            if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES_AS_PM)])
                            {
                                const int iRet = snprintf(ServerManager::m_pGlobalBuffer,
                                                    ServerManager::m_szGlobalBufferSize,
                                                    "%s $%s",
                                                    SettingManager::HubSec(),
                                                    pDcCommand->m_sCommand);
                                if (iRet > 0)
                                {
                                    GlobalDataQueue::m_Ptr->SingleItemStore(
                                        ServerManager::m_pGlobalBuffer, iRet, nullptr, 0, GlobalDataQueue::SendItem::PM2OPS);
                                }
                            }
                            else
                            {
                                GlobalDataQueue::m_Ptr->AddQueueItem(
                                    pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, nullptr, 0, GlobalDataQueue::Cmd::OPS);
                            }
                        }
                        else
                        {
                            pDcCommand->m_pUser->SendCharDelayed(pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen);
                        }
                        return;
                    }
                }
            }

            if (pTemp)
            {
                pTemp[0] = '\n';
            }
        }
    }

    pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CHAT, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, static_cast<void*>(pQueueItem));
#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
    DBSQLite::m_Ptr->IncMessageCount(pDcCommand->m_pUser);
#endif
#endif
}
//---------------------------------------------------------------------------

// $Close nick|
void DcCommands::Close(DcCommand* pDcCommand)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdClose);
    if (!ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::CLOSE))
    {
        pDcCommand->m_pUser->SendFormat(
            "DcCommands::Close1", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD)].c_str());
        return;
    }

    if (pDcCommand->m_ui32CommandLen < 9)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Close (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        pDcCommand->m_pUser->Close();
        return;
    }

    if (ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::CLOSE_ARRIVAL) || User::isPastLogin(pDcCommand->m_pUser->m_ui8State))
    {
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    User* OtherUser = HashManager::m_Ptr->FindUser(std::string_view(pDcCommand->m_sCommand + 7, pDcCommand->m_ui32CommandLen - 8));
    if (OtherUser)
    {
        // Self-kick
        if (OtherUser == pDcCommand->m_pUser)
        {
            pDcCommand->m_pUser->SendFormat(
                "DcCommands::Close2", true, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_CANT_CLOSE_YOURSELF)].c_str());
            return;
        }

        if (OtherUser->m_i32Profile != -1 && pDcCommand->m_pUser->m_i32Profile > OtherUser->m_i32Profile)
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::Close3",
                                            true,
                                            "<%s> %s %s!|",
                                            SettingManager::HubSec(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALLOWED_TO_CLOSE)].c_str(),
                                            OtherUser->m_sNick.c_str());
            return;
        }

        // disconnect the user
        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] User %s (%s) closed by %s", OtherUser->m_sNick.c_str(), OtherUser->m_sIP.data(), pDcCommand->m_pUser->m_sNick.c_str());

        OtherUser->Close();

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Close",
                                                        "<%s> *** %s %s %s %s %s.|",
                                                        SettingManager::HubSec(),
                                                        OtherUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                        OtherUser->m_sIP.data(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_CLOSED_BY)].c_str(),
                                                        pDcCommand->m_pUser->m_sNick.c_str());
        }

        if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)] ||
            !((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            pDcCommand->m_pUser->SendFormat("DcCommands::Close4",
                                            true,
                                            "<%s> *** %s %s %s %s.|",
                                            SettingManager::HubSec(),
                                            OtherUser->m_sNick.c_str(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                            OtherUser->m_sIP.data(),
                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_CLOSED)].c_str());
        }
    }
}
//---------------------------------------------------------------------------

void DcCommands::Unknown(DcCommand* pDcCommand, const bool bMyNick /* = false*/)
{
    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeCmdOther);
    m_ui32StatCmdUnknown++;

#ifdef _DBG
    // Debug logging removed
#endif

    // if we got unknown command sooner than full login finished
    // PPK ... fixed posibility to send (or flood !!!) hub with unknown command before full login
    // Give him chance with script...
    // if this is unkncwn command and script dont clarify that it's ok, disconnect the user
    if (!ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::UNKNOWN_ARRIVAL))
    {
        m_ui32StatUnknownCmd++;
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Unknown command from %s (%s) - user closed. (%s)",
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data(),
                                         pDcCommand->m_sCommand);

        if (bMyNick)
        {
            pDcCommand->m_pUser->SendCharDelayed("$Error CTM2HUB|", 15);
        }

        pDcCommand->m_pUser->Close();
    }
}
//---------------------------------------------------------------------------

bool DcCommands::ValidateUserNick(DcCommand* pDcCommand, User* pUser, const char* sNick, const size_t szNickLen, const bool ValidateNick)
{
    // illegal characters in nick?
    for (const auto& c : std::string_view(sNick, szNickLen))
    {
        switch (c)
        {
        case ' ':
        case '$':
        case '|':
        {
            pUser->SendFormat("DcCommands::ValidateUserNick1",
                              false,
                              "<%s> %s '%c' ! %s.|",
                              SettingManager::HubSec(),
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_NICK_CONTAINS_ILLEGAL_CHARACTER)].c_str(),
                               c,
                               LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PLS_CORRECT_IT_AND_GET_BACK_AGAIN)].c_str());

            pUser->Close();
            return false;
        }
        default:
            if (static_cast<unsigned char>(c) < 32)
            {
                pUser->SendFormat("DcCommands::ValidateUserNick2",
                                  false,
                                  "<%s> %s! %s.|",
                                  SettingManager::HubSec(),
                                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_NICK_CONTAINS_ILLEGAL_WHITE_CHARACTER)].c_str(),
                                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PLS_CORRECT_IT_AND_GET_BACK_AGAIN)].c_str());

                pUser->Close();
                return false;
            }

            continue;
        }
    }

    pUser->SetNick(std::string_view(sNick, szNickLen));

    // check for reserved nicks
    if (ReservedNicksManager::m_Ptr->CheckReserved(pUser->m_sNick.c_str(), pUser->m_ui32NickHash))
    {
        pUser->SendFormat("DcCommands::ValidateUserNick3",
                          false,
                          "<%s> %s. %s.|$ValidateDenide %s|",
                          SettingManager::HubSec(),
                          LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THE_NICK_IS_RESERVED_FOR_SOMEONE_OTHER)].c_str(),
                          LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CHANGE_YOUR_NICK_AND_GET_BACK_AGAIN)].c_str(),
                          sNick);

        pUser->Close();
        return false;
    }

    // PPK ... check if we already have ban for this user
    if (pUser->m_LogInOut.m_pBan && pUser->m_ui32NickHash == pUser->m_LogInOut.m_pBan->m_ui32NickHash)
    {
        pUser->SendChar(pUser->m_LogInOut.m_pBan->m_sMessage);
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Banned user %s (%s) - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

        pUser->Close();
        return false;
    }

    time_t tmAccTime;
    time(&tmAccTime);

    // check for banned nicks
    BanItem* pBan = BanManager::m_Ptr->FindNick(pUser);
    if (pBan)
    {
        const int iMsgLen = GenerateBanMessage(pBan, tmAccTime);
        if (iMsgLen != 0)
        {
            pUser->SendChar(ServerManager::m_pGlobalBuffer, iMsgLen);
        }

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Banned user %s (%s) - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

        pUser->Close();
        return false;
    }

    int32_t i32Profile = -1;

    // Nick is ok, check for registered nick
    RegUser* Reg = RegManager::m_Ptr->Find(pUser);
    if (Reg)
    {
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ADVANCED_PASS_PROTECTION)] && Reg->m_ui8BadPassCount != 0)
        {
            const auto iMinutes2Wait = static_cast<uint32_t>(pow(2.0, static_cast<double>(Reg->m_ui8BadPassCount) - 1));

            if (tmAccTime < static_cast<time_t>(Reg->m_tLastBadPass + (60 * iMinutes2Wait)))
            {
                pUser->SendFormat("DcCommands::ValidateUserNick4",
                                  false,
                                  "<%s> %s %s %s!|",
                                  SettingManager::HubSec(),
                                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_LAST_PASS_WAS_WRONG_YOU_NEED_WAIT)].c_str(),
                                  formatSecTime((Reg->m_tLastBadPass + (60 * iMinutes2Wait)) - tmAccTime).c_str(),
                                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BEFORE_YOU_TRY_AGAIN)].c_str());

                UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) not allowed to send password (%" PRIu64 ") - user closed.",
                                                 pUser->m_sNick.c_str(),
                                                 pUser->m_sIP.data(),
                                                 static_cast<uint64_t>((Reg->m_tLastBadPass + (60 * iMinutes2Wait)) - tmAccTime));

                pUser->Close();
                return false;
            }
        }

        i32Profile = static_cast<int32_t>(Reg->m_ui16Profile);
    }

    // PPK ... moved IP ban check here, we need to allow registered users on shared IP to log in if not have banned nick, but only IP.
    if (!ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::ENTERIFIPBAN))
    {
        // PPK ... check if we already have ban for this user
        if (pUser->m_LogInOut.m_pBan)
        {
            pUser->SendChar(pUser->m_LogInOut.m_pBan->m_sMessage);

            pUser->Close();
            return false;
        }
    }

    // PPK ... delete user ban if we have it
    pUser->m_LogInOut.m_pBan.reset();

    // first check for user limit ! PPK ... allow hublist pinger to check hub any time ;)
    if (!ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::ENTERFULLHUB) && !((pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
    {
        // user is NOT allowed enter full hub, check for maxClients
        if (ServerManager::m_ui32Joins - ServerManager::m_ui32Parts > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS)]))
        {
            pUser->SendFormat("DcCommands::ValidateUserNick5",
                              false,
                              "$HubIsFull|<%s> %s. %u %s.|%s",
                              SettingManager::HubSec(),
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THIS_HUB_IS_FULL)].c_str(),
                              ServerManager::m_ui32Logged,
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USERS_ONLINE_LWR)].c_str(),
                              (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REDIRECT_WHEN_HUB_FULL)] &&
                               !SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].empty())
                                  ? SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].c_str()
                                  : "");

            pUser->Close();
            return false;
        }
    }

    // Check for maximum connections from same IP
    if (!ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::NOUSRSAMEIP))
    {
        const uint32_t ui32Count = HashManager::m_Ptr->GetUserIpCount(pUser);
        if (ui32Count >= static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_CONN_SAME_IP)]))
        {
            pUser->SendFormat("DcCommands::ValidateUserNick6",
                              false,
                              "<%s> %s.|%s",
                              SettingManager::HubSec(),
                              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_ALREADY_MAX_IP_CONNS)].c_str(),
                              (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REDIRECT_WHEN_HUB_FULL)] &&
                               !SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].empty())
                                  ? SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].c_str()
                                  : "");

            const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                   ServerManager::m_szGlobalBufferSize,
                                   "[SYS] Max connections from same IP (%u) for %s (%s) - user closed. ",
                                   ui32Count,
                                   pUser->m_sNick.c_str(),
                                   pUser->m_sIP.data());
            if (iMsgLen <= 0)
            {
                pUser->Close();
                return false;
            }

            std::string tmp(ServerManager::m_pGlobalBuffer, iMsgLen);

            User *cur = nullptr, *nxt = HashManager::m_Ptr->FindUser(pUser->m_ui128IpHash.data());

            while (nxt)
            {
                cur = nxt;
                nxt = cur->m_pHashIpTableNext;

                tmp += " " + cur->m_sNick;
            }

            UdpDebug::m_Ptr->Broadcast(tmp);

            pUser->Close();
            return false;
        }
    }

    // Check for reconnect time
    if (!ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::NORECONNTIME) && Users::m_Ptr->CheckRecTime(pUser))
    {
        GlobalDataQueue::m_Ptr->PrometheusFastReconnectInc();
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Fast reconnect from %s (%s) - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

        pUser->Close();
        return false;
    }

    pUser->m_ui8Country = IpP2Country::m_Ptr->Find(pUser->m_ui128IpHash.data());

    // check for nick in userlist. If taken, check for dupe's socket state
    // if still active, send $ValidateDenide and close()
    User* OtherUser = HashManager::m_Ptr->FindUser(pUser);

    if (OtherUser)
    {
        if (!User::isPastLogin(OtherUser->m_ui8State))
        {
            // check for socket error, or if user closed connection
            const ssize_t iRet = recv(OtherUser->m_Socket, ServerManager::m_pGlobalBuffer, 16, MSG_PEEK);

            // if socket error or user closed connection then allow new user to log in
            if ((iRet == -1 && errno != EAGAIN) || iRet == 0)
            {
                OtherUser->m_ui32BoolBits |= User::BIT_ERROR;

                UdpDebug::m_Ptr->BroadcastFormat("[SYS] Detected clone of yourself, byebye! Nick %s (%s) - user closed. Please close the second DC++ client! ",
                                                 OtherUser->m_sNick.c_str(),
                                                 OtherUser->m_sIP.data());

                OtherUser->Close();
                return false;
            }

            if (!Reg)
            {
                // alex82 ... �������� ValidateDenideArrival
                (void)ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::VALIDATE_DENIDE_ARRIVAL);
#ifdef FLYLINKDC_USE_REMOVE_CLONE // ���� ��������. ��������
                if (              // OtherUser->m_ui64SharedSize == pUser->m_ui64SharedSize &&  // disabled: too aggressive
                    OtherUser->m_sNick == pUser->m_sNick &&
                    OtherUser->m_sIP == pUser->m_sIP) //[+] FlylinkDC++
                {
                    OtherUser->SendFormat("DcCommands::ValidateUserNick8",
                                          false,
                                          "Detected clone of yourself, byebye!Nick %s(%s) - user closed.Please close the second DC++ client!|",
                                          SettingManager::HubSec(),
                                          sNick);
                    OtherUser->m_ui32BoolBits |= User::BIT_ERROR;
                    UdpDebug::m_Ptr->BroadcastFormat(
                        "[SYS] Remove clone user nick %s (%s) - user closed.", OtherUser->m_sNick.c_str(), OtherUser->m_sIP.data());
                    OtherUser->Close();
                    return ValidateUserNickFinally(!Reg, pUser, szNickLen, ValidateNick); // [+] FlylinkDC++
                }
                else
#endif // FLYLINKDC_USE_REMOVE_CLONE
                {
                    pUser->SendFormat("DcCommands::ValidateUserNick7", false, "$ValidateDenide %s|", sNick);

                    if (OtherUser->m_sIP != pUser->m_sIP || OtherUser->m_sNick != pUser->m_sNick)
                    {
                        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick taken [%s (%s)] %s (%s) - user closed.",
                                                         OtherUser->m_sNick.c_str(),
                                                         OtherUser->m_sIP.data(),
                                                         pUser->m_sNick.c_str(),
                                                         pUser->m_sIP.data());
                    }

                    pUser->Close();
                    return false;
                }
            }
            else
            {
                // PPK ... addition for registered users, kill your own ghost >:-]
                pUser->m_ui8State = User::UserStates::STATE_VERSION_OR_MYPASS;
                pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
                pUser->AddPrcsdCmd(PrcsdUsrCmd::GETPASS, nullptr, 0, nullptr);
                return true;
            }
        }
    }
    return ValidateUserNickFinally(!Reg, pUser, szNickLen, ValidateNick); // [+] FlylinkDC++
}
// [+] FlylinkDC++
//---------------------------------------------------------------------------
bool DcCommands::ValidateUserNickFinally(bool pIsNotReg, User* pUser, const size_t szNickLen, const bool ValidateNick)
{
    if (pIsNotReg)
    {
        // user is NOT registered

        // nick length check
        if ((SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_NICK_LEN)] != 0 &&
             szNickLen < static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_NICK_LEN)])) ||
            (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_NICK_LEN)] != 0 &&
             szNickLen > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_NICK_LEN)])))
        {
            pUser->SendChar(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_NICK_LIMIT_MSG)]);

            pUser->Close();
            return false;
        }

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_ONLY)] && !((pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER))
        {
            pUser->SendChar(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REG_ONLY_MSG)]);

            pUser->Close();
            return false;
        }

        // hub is public, proceed to Hello
        if (!HashManager::m_Ptr->Add(pUser))
        {
            return false;
        }

        pUser->m_ui32BoolBits |= User::BIT_HASHED;

        if (ValidateNick)
        {
            pUser->m_ui8State = User::UserStates::STATE_VERSION_OR_MYPASS; // waiting for $Version
            pUser->AddPrcsdCmd(PrcsdUsrCmd::LOGINHELLO, nullptr, 0, nullptr);
        }
        return true;
    }
    else
    {
        // user is registered, wait for password
        if (!HashManager::m_Ptr->Add(pUser))
        {
            return false;
        }

        pUser->m_ui32BoolBits |= User::BIT_HASHED;
        pUser->m_ui8State = User::UserStates::STATE_VERSION_OR_MYPASS;
        pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
        pUser->AddPrcsdCmd(PrcsdUsrCmd::GETPASS, nullptr, 0, nullptr);
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------

DcCommands::PassBf* DcCommands::Find(const uint8_t* ui128IpHash)
{
    for (const auto& pItem : m_PasswdBfCheck)
    {
        if (memcmp(pItem->m_ui128IpHash.data(), ui128IpHash, 16) == 0)
        {
            return pItem.get();
        }
    }
    return nullptr;
}
//---------------------------------------------------------------------------

void DcCommands::Remove(PassBf* pPassBfItem)
{
    for (auto it = m_PasswdBfCheck.begin(); it != m_PasswdBfCheck.end(); ++it)
    {
        if (it->get() == pPassBfItem)
        {
            m_PasswdBfCheck.erase(it);
            return;
        }
    }
}
//---------------------------------------------------------------------------

void DcCommands::ProcessCmds(User* pUser)
{
    // Log queue size
    if (!pUser->m_CmdList.empty())
    {
        LogInfo("[CMDS] nick='{}' queue_size={} types={}", pUser->m_sNick, pUser->m_CmdList.size(), static_cast<int>(pUser->m_CmdList.front()->m_ui8Type));
    }

    CmdTimer _t(DcCommands::m_Ptr->m_ui64TimeProcessCmds);
    pUser->m_ui32BoolBits &= ~User::BIT_CHAT_INSERT;

    std::list<std::unique_ptr<PrcsdUsrCmd>> queue = std::move(pUser->m_CmdList);

    for (const auto& pItem : queue)
    {
        PrcsdUsrCmd* cur = pItem.get();

        switch (cur->m_ui8Type)
        {
        case PrcsdUsrCmd::SUPPORTS:
        {
            memcpy(ServerManager::m_pGlobalBuffer, "$Supports", 9);
            uint32_t iSupportsLen = 9;

            // PPK ... why dc++ have that 0 on end ? that was not in original documentation.. stupid duck...
            if (((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE0) == User::SUPPORTBIT_ZPIPE0))
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " ZPipe0", 7);
                iSupportsLen += 7;
            }
            else if (((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE))
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " ZPipe", 6);
                iSupportsLen += 6;
            }

            // PPK ... yes yes yes finally QuickList support in PtokaX !!! ;))
            if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST)
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " QuickList", 10);
                iSupportsLen += 10;
            }
            else if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO)
            {
                // PPK ... Hmmm Client not really need it, but for now send it ;-)
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " NoHello", 8);
                iSupportsLen += 8;
            }
            else if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO)
            {
                // PPK ... if client support NoHello automatically supports NoGetINFO another badwith wasting !
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " NoGetINFO", 10);
                iSupportsLen += 10;
            }
#ifdef USE_FLYLINKDC_EXT_JSON
            if (pUser->isSupportExtJSON())
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " ExtJSON2", 9);
                iSupportsLen += 9;
            }
#endif
            if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_HUBURL) == User::SUPPORTBIT_HUBURL)
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " HubURL", 7);
                iSupportsLen += 7;
            }

            if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64)
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " IP64", 5);
                iSupportsLen += 5;
            }

            if (((pUser->m_ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) && ((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6))
            {
                // Only client connected with IPv6 sending this, so only that client is getting reply
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " IPv4", 5);
                iSupportsLen += 5;
            }

            if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND)
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " UserCommand", 12);
                iSupportsLen += 12;
            }

            if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2 &&
                !((pUser->m_ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS))
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " UserIP2", 8);
                iSupportsLen += 8;
            }

            if ((pUser->m_ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2)
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " TLS2", 5);
                iSupportsLen += 5;
            }

            if ((pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER)
            {
                memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, " BotINFO HubINFO", 16);
                iSupportsLen += 16;
            }

            memcpy(ServerManager::m_pGlobalBuffer + iSupportsLen, "|\0", 2);
            pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iSupportsLen + 1);
            break;
        }
        case PrcsdUsrCmd::LOGINHELLO:
        {
            pUser->SendFormat("DcCommands::ProcessCmds", true, "$Hello %s|", pUser->m_sNick.c_str());
            break;
        }
        case PrcsdUsrCmd::GETPASS:
        {
            const uint32_t ui32Len = 9;
            pUser->SendCharDelayed("$GetPass|", ui32Len); // query user for password
            break;
        }
        case PrcsdUsrCmd::CHAT:
        {
            LogInfo("[CHAT-PROC] Processing CHAT for nick='{}' cmd='{}' len={}",
                    pUser->m_sNick,
                    std::string(cur->m_sCommand.data(), cur->m_ui32Len > 100 ? 100 : cur->m_ui32Len),
                    cur->m_ui32Len);
            // find chat message data
            char* sBuff = cur->m_sCommand.data() + pUser->m_sNick.size() + 3;

            // non-command chat msg
            bool bNonChat = false;
            for (const char prefix : SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)])
            {
                if (sBuff[0] == prefix)
                {
                    bNonChat = true;
                    break;
                }
            }

            if (bNonChat)
            {
                // text files...
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_TEXT_FILES)])
                {
                    cur->m_sCommand[cur->m_ui32Len - 1] = '\0'; // get rid of the pipe

                    if (TextFilesManager::m_Ptr->ProcessTextFilesCmd(pUser, sBuff + 1))
                    {
                        break;
                    }

                    cur->m_sCommand[cur->m_ui32Len - 1] = '|'; // add back pipe
                }

                // built-in commands
                // alex82 ... ��������� ����������� ����� �������
                if (cur->m_ui32Len - pUser->m_sNick.size() >= 6)
                {
                    if (HubCommands::DoCommand(pUser, sBuff - (pUser->m_sNick.size() - 1), cur->m_ui32Len))
                    {
                        break;
                    }

                    cur->m_sCommand[cur->m_ui32Len - 1] = '|'; // add back pipe
                }
            }

            // everything's ok, let's chat
            Users::m_Ptr->SendChat2All(pUser, cur->m_sCommand.data(), cur->m_ui32Len, static_cast<GlobalDataQueue::QueueItem*>(cur->m_pPtr));

            break;
        }
        case PrcsdUsrCmd::TO_OP_CHAT:
        {
            GlobalDataQueue::m_Ptr->SingleItemStore(cur->m_sCommand.data(), cur->m_ui32Len, pUser, 0, GlobalDataQueue::SendItem::OPCHAT);
            break;
        }
        default:
            break;
        }

        cur->m_sCommand.clear();
    }

    if ((pUser->m_ui32BoolBits & User::BIT_PRCSD_MYINFO) == User::BIT_PRCSD_MYINFO)
    {
        pUser->m_ui32BoolBits &= ~User::BIT_PRCSD_MYINFO;

        if (((pUser->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG))
        {
            pUser->HasSuspiciousTag();
        }
        // alex82 ... HideUser / ������� �����
        if (!((pUser->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
        {
            if (SettingManager::m_Ptr->m_ui8FullMyINFOOption == 0)
            {
                if (!pUser->GenerateMyInfoLong())
                {
                    return;
                }

                Users::m_Ptr->Add2MyInfosTag(pUser);

                if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY)] == 0 ||
                    ServerManager::m_ui64ActualTick > ((60 * SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY)]) + pUser->m_ui64LastMyINFOSendTick))
                {
                    GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoLong.data(), pUser->m_ui16MyInfoLongLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    pUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
                }
                else
                {
                    GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoLong.data(), pUser->m_ui16MyInfoLongLen, nullptr, 0, GlobalDataQueue::Cmd::OPS);
                }
                return;
            }
            if (SettingManager::m_Ptr->m_ui8FullMyINFOOption == 2)
            {
                if (!pUser->GenerateMyInfoShort())
                {
                    return;
                }

                Users::m_Ptr->Add2MyInfos(pUser);

                if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY)] == 0 ||
                    ServerManager::m_ui64ActualTick > ((60 * SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY)]) + pUser->m_ui64LastMyINFOSendTick))
                {
                    GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoShort.data(), pUser->m_ui16MyInfoShortLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    pUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
                }
                else
                {
                    GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoShort.data(), pUser->m_ui16MyInfoShortLen, nullptr, 0, GlobalDataQueue::Cmd::OPS);
                }
                return;
            }

            if (!pUser->GenerateMyInfoLong())
            {
                return;
            }

            Users::m_Ptr->Add2MyInfosTag(pUser);

            char* sShortMyINFO = nullptr;
            size_t szShortMyINFOLen = 0;

            if (pUser->GenerateMyInfoShort())
            {
                Users::m_Ptr->Add2MyInfos(pUser);

                if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY)] == 0 ||
                    ServerManager::m_ui64ActualTick > ((60 * SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY)]) + pUser->m_ui64LastMyINFOSendTick))
                {
                    sShortMyINFO = pUser->m_sMyInfoShort.data();
                    szShortMyINFOLen = pUser->m_ui16MyInfoShortLen;
                    pUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
                }
            }

            GlobalDataQueue::m_Ptr->AddQueueItem(
                sShortMyINFO, szShortMyINFOLen, pUser->m_sMyInfoLong.data(), pUser->m_ui16MyInfoLongLen, GlobalDataQueue::Cmd::MYINFO);

#ifdef USE_FLYLINKDC_EXT_JSON
            if ((pUser->m_ui32BoolBits & User::BIT_PRCSD_EXT_JSON) == User::BIT_PRCSD_EXT_JSON)
            {
                pUser->m_ui32BoolBits &= ~User::BIT_PRCSD_EXT_JSON;
                if (pUser->isSupportExtJSON())
                {
                    if (pUser->m_user_ext_info)
                    {
                        const auto& l_ext_json_info = pUser->m_user_ext_info->GetExtJSONCommand();
                        if (!l_ext_json_info.empty())
                        {
                            if (pUser->getLastExtJSONSendTick() != 0 || ServerManager::m_ui64ActualTick > pUser->getLastExtJSONSendTick() + 60)
                            {
                                GlobalDataQueue::m_Ptr->AddQueueItem(l_ext_json_info, "", GlobalDataQueue::Cmd::EXTJSON);
                                pUser->setLastExtJSONSendTick(ServerManager::m_ui64ActualTick);
                            }
                            else
                            {
                                LogDbg("[EXTJSON] Rate-limited, CMD_OPS fallback user='{}' len={}", pUser->m_sNick, l_ext_json_info.size());
                                GlobalDataQueue::m_Ptr->AddQueueItem(l_ext_json_info, "", GlobalDataQueue::Cmd::OPS);
                            }
                        }
                    }
                }
            }
#endif // USE_FLYLINKDC_EXT_JSON
        } // HideUser
    } // MyINFO
}
//----------------------------------------------------------------------------------------------------------------------------

void DcCommands::MyNick(DcCommand* pDcCommand)
{
    if ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] IPv6 $MyNick (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        Unknown(pDcCommand, true);
        return;
    }

    if (pDcCommand->m_ui32CommandLen < 10)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Short $MyNick (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        Unknown(pDcCommand, true);
        return;
    }

    pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // cutoff pipe

    User* pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(pDcCommand->m_sCommand + 8, pDcCommand->m_ui32CommandLen - 9));

    if (!pOtherUser || pOtherUser->m_ui8State != User::UserStates::STATE_IPV4_CHECK)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyNick (%s) from %s (%s) - user closed.",
                                         pDcCommand->m_sCommand,
                                         pDcCommand->m_pUser->m_sNick.c_str(),
                                         pDcCommand->m_pUser->m_sIP.data());

        Unknown(pDcCommand, true);
        return;
    }

    memcpy(pOtherUser->m_sIPv4.data(), pDcCommand->m_pUser->m_sIP.data(), pOtherUser->m_sIPv4.size() - 1);
    pOtherUser->m_sIPv4[pOtherUser->m_sIPv4.size() - 1] = '\0';
    pOtherUser->m_ui8IPv4Len = pDcCommand->m_pUser->m_ui8IpLen;
    pOtherUser->m_ui32BoolBits |= User::BIT_IPV4;

    pOtherUser->m_ui8State = User::UserStates::STATE_ADDME;

    pDcCommand->m_pUser->Close();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint16_t DcCommands::CheckAndGetPort(const char* pPort, const uint8_t ui8PortLen, uint32_t& ui32PortLen)
{
    uint8_t ui8Index = ui8PortLen - 1;
    while (true)
    {
        if (pPort[ui8Index] == ':')
        {
            // We have something which looks like port.. return values
            ui32PortLen = (ui8PortLen - ui8Index) - 1;
            int iPort = 0;
            if (!safe_stoi((pPort + ui8Index) + 1, iPort) || iPort < 0 || iPort > 65535)
            {
                return 0;
            }
            return static_cast<uint16_t>(iPort);
        }
        if (!isdigit(static_cast<unsigned char>(pPort[ui8Index])))
        {
            // It is not : and it is not number... this port is not valid
            return 0;
        }

        if (ui8Index == 0)
        {
            break;
        }

        ui8Index--;
    }

    // Valid port not found
    return 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DcCommands::SendIPFixedMsg(User* pUser, const char* pBadIP, const char* pRealIP)
{
    if ((pUser->m_ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP)
    {
        return;
    }

    pUser->SendFormat("DcCommands::SendIPFixedMsg",
                      true,
                      "<%s> %s '%s' %s %s !|",
                      SettingManager::HubSec(),
                      LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_CLIENT_SEND_INCORRECT_IP)].c_str(),
                      pBadIP,
                      LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IN_COMMAND_HUB_REPLACED_IT_WITH_YOUR_REAL_IP)].c_str(),
                      pRealIP);

    pUser->m_ui32BoolBits |= User::BIT_WARNED_WRONG_IP;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PrcsdUsrCmd* DcCommands::AddSearch([[maybe_unused]] User* pUser, PrcsdUsrCmd* pCmdSearch, const char* sSearch, const size_t szLen, const bool bActive)
{
    if (pCmdSearch)
    {
        pCmdSearch->m_sCommand.resize(pCmdSearch->m_ui32Len + szLen + 1);
        memcpy(pCmdSearch->m_sCommand.data() + pCmdSearch->m_ui32Len, sSearch, szLen);
        pCmdSearch->m_ui32Len += static_cast<uint32_t>(szLen);
        pCmdSearch->m_sCommand[pCmdSearch->m_ui32Len] = '\0';

        if (bActive)
        {
            Users::m_Ptr->m_ui16ActSearchs++;
        }
        else
        {
            Users::m_Ptr->m_ui16PasSearchs++;
        }
    }
    else
    {
        auto pCmdSearchOwned = std::make_unique<PrcsdUsrCmd>();
        pCmdSearch = pCmdSearchOwned.release();

        pCmdSearch->m_sCommand.resize(szLen + 1);
        memcpy(pCmdSearch->m_sCommand.data(), sSearch, szLen);
        pCmdSearch->m_sCommand[szLen] = '\0';

        pCmdSearch->m_ui32Len = static_cast<uint32_t>(szLen);

        if (bActive)
        {
            Users::m_Ptr->m_ui16ActSearchs++;
        }
        else
        {
            Users::m_Ptr->m_ui16PasSearchs++;
        }
    }

    return pCmdSearch;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
