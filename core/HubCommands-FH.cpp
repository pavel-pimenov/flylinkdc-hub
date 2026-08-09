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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "HubCommands.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#endif
#include "IP2Country.h"
#include "LuaScript.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Helper macro for Help(): snprintf + overflow check + append to help string
#define HELP_LINE(fmt, ...)                                                                                                                                    \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int _iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, fmt, __VA_ARGS__);                                           \
        if (_iRet <= 0)                                                                                                                                        \
            return true;                                                                                                                                       \
        help.append(ServerManager::m_pGlobalBuffer, _iRet);                                                                                                    \
    } while (0)

bool HubCommands::FullBan(ChatCommand* pChatCommand) // !fullban nick reason
{
    return FullBanDelegate(pChatCommand, ProfileManager::BAN, 9, 8, "HubCommands::FullBan", "%cfullban <%s> <%s>", Ban);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullBanIp(ChatCommand* pChatCommand) // !fullbanip ip reason
{
    return FullBanDelegate(pChatCommand, ProfileManager::BAN, 16, 10, "HubCommands::FullBanIp", "%cfullbanip <%s> <%s>", BanIp);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullTempBan(ChatCommand* pChatCommand) // !fulltempban nick time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
{
    return FullBanDelegate(pChatCommand, ProfileManager::TEMP_BAN, 16, 12, "HubCommands::FullTempBan", "%cfulltempban <%s> <%s> <%s>", TempBan);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullTempBanIp(
    ChatCommand* pChatCommand) // !fulltempbanip ip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
{
    return FullBanDelegate(pChatCommand, ProfileManager::TEMP_BAN, 24, 14, "HubCommands::FullTempBanIp", "%cfulltempbanip <%s> <%s> <%s>", TempBanIp);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullRangeBan(ChatCommand* pChatCommand) // !fullrangeban fromip toip reason
{
    return FullBanDelegate(pChatCommand, ProfileManager::RANGE_BAN, 28, 13, "HubCommands::FullRangeBan", "%cfullrangeban <%s> <%s> <%s>", RangeBan);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullRangeTempBan(
    ChatCommand* pChatCommand) // !fullrangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
{
    return FullBanDelegate(
        pChatCommand, ProfileManager::RANGE_TBAN, 35, 17, "HubCommands::FullRangeTempBan", "%cfullrangetempban <%s> <%s> <%s> <%s>", RangeTempBan);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Ban listing helpers — extract common iteration/formatting logic
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint32_t HubCommands::ListTempBans(std::string& out)
{
    if (BanManager::m_Ptr->m_TempBanList.empty())
    {
        return 0;
    }

    uint32_t ui32BanNum = 0;

    time_t acc_time;
    time(&acc_time);

    auto it = BanManager::m_Ptr->m_TempBanList.begin();
    while (it != BanManager::m_Ptr->m_TempBanList.end())
    {
        BanItem* const curBan = it->get();
        ++it;

        if (acc_time > curBan->m_tTempBanExpire)
        {
            BanManager::m_Ptr->Rem(curBan);
            std::unique_ptr<BanItem> guard(curBan);

            continue;
        }

        if (ui32BanNum == 0)
        {
            out += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANS)] + ":\n\n";
        }

        FormatBanEntry(out, ui32BanNum, curBan, true);
    }

    return ui32BanNum;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint32_t HubCommands::ListPermBans(std::string& out)
{
    if (BanManager::m_Ptr->m_PermBanList.empty())
    {
        return 0;
    }

    uint32_t ui32BanNum = 0;

    out += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BANS)] + ":\n\n";

    for (const auto& pBan : BanManager::m_Ptr->m_PermBanList)
    {
        FormatBanEntry(out, ui32BanNum, pBan.get(), false);
    }

    return ui32BanNum;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint32_t HubCommands::ListTempRangeBans(std::string& out)
{
    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    if (rangeList.empty())
    {
        return 0;
    }

    uint32_t ui32BanNum = 0;

    time_t acc_time;
    time(&acc_time);

    auto it = rangeList.begin();
    while (it != rangeList.end())
    {
        const RangeBanItem* curBan = it->get();

        if (!((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
        {
            ++it;
            continue;
        }

        if (acc_time > curBan->m_tTempBanExpire)
        {
            it = rangeList.erase(it);

            continue;
        }

        if (ui32BanNum == 0)
        {
            out += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_RANGE_BANS)] + ":\n\n";
        }

        FormatRangeBanEntry(out, ui32BanNum, curBan, true);
        ++it;
    }

    return ui32BanNum;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint32_t HubCommands::ListPermRangeBans(std::string& out)
{
    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    if (rangeList.empty())
    {
        return 0;
    }

    uint32_t ui32BanNum = 0;

    for (const auto& pCurBan : rangeList)
    {
        const RangeBanItem* curBan = pCurBan.get();

        if (!((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
        {
            continue;
        }

        if (ui32BanNum == 0)
        {
            out += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_RANGE_BANS)] + ":\n\n";
        }

        FormatRangeBanEntry(out, ui32BanNum, curBan, false);
    }

    return ui32BanNum;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// End of ban listing helpers
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetBans(ChatCommand* pChatCommand) // !getbans
{
    const int iMsgLen = BeginBanList(pChatCommand, ProfileManager::GETBANLIST);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);

    const uint32_t iTempCount = ListTempBans(BanList);
    const uint32_t iPermCount = ListPermBans(BanList);

    if (iTempCount != 0 && iPermCount != 0)
    {
        BanList += "\n\n";
    }

    if (iTempCount + iPermCount == 0)
    {
        BanList += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_BANS_FOUND)] + "...|";
    }
    else
    {
        BanList += "|";
    }

    pChatCommand->m_pUser->SendCharDelayed(BanList);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Gag(ChatCommand* pChatCommand) // !gag nick
{
    if (!CheckPermission(pChatCommand, ProfileManager::GAG))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 5 || pChatCommand->m_sCommand[4] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cgag <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cgag <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 4;

    // Self-gag ?
    if (!CheckSelfPermission(pChatCommand, std::to_underlying(LangIds::LAN_YOU_CANT_GAG_YOURSELF)))
    {
        return true;
    }

    User* pOtherUser = FindUserOrReply(pChatCommand, pChatCommand->m_ui32CommandLen - 4, "HubCommands::Gag4", std::to_underlying(LangIds::LAN_IS_NOT_IN_USERLIST));
    if (!pOtherUser)
    {
        return true;
    }

    if (((pOtherUser->m_ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s: %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 pOtherUser->m_sNick.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREDY_GAGGED)].c_str());
        return true;
    }

    // PPK don't gag user with higher profile
    if (!CheckHigherProfile(pChatCommand, pOtherUser, std::to_underlying(LangIds::LAN_NOT_ALW_TO_GAG)))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    pOtherUser->m_ui32BoolBits |= User::BIT_GAGGED;
    pOtherUser->SendFormat("HubCommands::Gag7",
                           true,
                           "<%s> %s %s.|",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_GAGGED_BY)].c_str(),
                           pChatCommand->m_pUser->m_sNick.c_str());

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Gag8",
                                                    "<%s> *** %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_GAGGED)].c_str(),
                                                    pOtherUser->m_sNick.c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag9",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pOtherUser->m_sNick.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_GAGGED)].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetInfo(ChatCommand* pChatCommand) // !getinfo nick
{
    if (!CheckPermission(pChatCommand, ProfileManager::GETINFO))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 9 || pChatCommand->m_sCommand[8] == 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetInfo1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cgetinfo <%s>. %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetInfo2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cgetinfo <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    StripPrefix(pChatCommand, 8);

    User* const pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen));
    if (!pOtherUser)
    {
#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
        if (DBSQLite::m_Ptr->SearchNick(pChatCommand))
        {
            UncountDeflood(pChatCommand);
            return true;
        }
#endif
#endif
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetInfo3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s: %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NOT_FOUND)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                        iMsgLen,
                        ServerManager::m_szGlobalBufferSize,
                        "<%s> \n%s: %s",
                        SettingManager::HubSec(),
                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                        pOtherUser->m_sNick.c_str()))
    {
        return true;
    }

    if (pOtherUser->m_i32Profile != -1)
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILE)].c_str(),
                            ProfileManager::m_Ptr->m_vpProfilesTable[pOtherUser->m_i32Profile]->m_sName.c_str()))
        {
            return true;
        }
    }

    if (!BuildUserOnlineInfo(iMsgLen, pOtherUser))
    {
        return true;
    }

    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    ServerManager::m_pGlobalBuffer[iMsgLen + 1] = '\0';

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen + 1);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetIpInfo(ChatCommand* pChatCommand) // !getipinfo ip
{
#if !defined(_WITH_SQLITE)
    return false;
#endif
    if (!CheckPermission(pChatCommand, ProfileManager::GETINFO))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 11 || pChatCommand->m_sCommand[10] == 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetIpInfo1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cgetipinfo <%s>. %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 102)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HHubCommands::GetIpInfo2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cgetipinfo <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_IP_LEN_39_CHARS)].c_str());
        return true;
    }

    StripPrefix(pChatCommand, 10);

#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
    if (DBSQLite::m_Ptr->SearchIP(pChatCommand))
    {
        UncountDeflood(pChatCommand);
        return true;
    }
#endif
#endif
    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetIpInfo4",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> *** %s: %s %s.|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                             pChatCommand->m_sCommand,
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NOT_FOUND)].c_str());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetTempBans(ChatCommand* pChatCommand) // !gettempbans
{
    const int iMsgLen = BeginBanList(pChatCommand, ProfileManager::GETBANLIST);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string BanList = px_str(ServerManager::m_pGlobalBuffer, iMsgLen);

    const uint32_t ui32BanNum = ListTempBans(BanList);

    if (ui32BanNum > 0)
    {
        BanList += "|";
    }
    else
    {
        BanList += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_TEMP_BANS_FOUND)] + "...|";
    }

    pChatCommand->m_pUser->SendCharDelayed(BanList);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetScripts(ChatCommand* pChatCommand) // !getscripts
{
    if (!CheckPermission(pChatCommand, ProfileManager::RSTSCRIPTS))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    const int iMsgLen = PrepareReply(pChatCommand);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string ScriptList(ServerManager::m_pGlobalBuffer, iMsgLen);

    ScriptList += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTS)] + ":\n\n";

    for (const auto& pScript : ScriptManager::m_Ptr->m_ppScriptTable)
    {
        ScriptList += "[ " + std::string(pScript->m_bEnabled ? "1" : "0") + " ] " +
                      std::string(pScript->m_sName);

        if (pScript->m_bEnabled)
        {
            ScriptList += " (" + std::to_string(ScriptGetGC(pScript)) + " kB)\n";
        }
        else
        {
            ScriptList += "\n";
        }
    }

    ScriptList += "|";
    pChatCommand->m_pUser->SendCharDelayed(ScriptList);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetPermBans(ChatCommand* pChatCommand) // !getpermbans
{
    const int iMsgLen = BeginBanList(pChatCommand, ProfileManager::GETBANLIST);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);

    const uint32_t ui32BanNum = ListPermBans(BanList);

    if (ui32BanNum > 0)
    {
        BanList += "|";
    }
    else
    {
        BanList += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PERM_BANS_FOUND)] + "...|";
    }

    pChatCommand->m_pUser->SendCharDelayed(BanList);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetRangeBans(ChatCommand* pChatCommand) // !getrangebans
{
    const int iMsgLen = BeginBanList(pChatCommand, ProfileManager::GET_RANGE_BANS);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);

    const uint32_t iTempCount = ListTempRangeBans(BanList);
    const uint32_t iPermCount = ListPermRangeBans(BanList);

    if (iTempCount != 0 && iPermCount != 0)
    {
        BanList += "\n\n";
    }

    if (iTempCount + iPermCount == 0)
    {
        BanList += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_RANGE_BANS_FOUND)] + "...|";
    }
    else
    {
        BanList += "|";
    }

    pChatCommand->m_pUser->SendCharDelayed(BanList);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetRangePermBans(ChatCommand* pChatCommand) // !getrangepermbans
{
    const int iMsgLen = BeginBanList(pChatCommand, ProfileManager::GET_RANGE_BANS);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);

    const uint32_t ui32BanNum = ListPermRangeBans(BanList);

    if (ui32BanNum > 0)
    {
        BanList += "|";
    }
    else
    {
        BanList += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_RANGE_PERM_BANS_FOUND)] + "...|";
    }

    pChatCommand->m_pUser->SendCharDelayed(BanList);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetRangeTempBans(ChatCommand* pChatCommand) // !getrangetempbans
{
    const int iMsgLen = BeginBanList(pChatCommand, ProfileManager::GET_RANGE_BANS);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);

    const uint32_t ui32BanNum = ListTempRangeBans(BanList);

    if (ui32BanNum > 0)
    {
        BanList += "|";
    }
    else
    {
        BanList += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_RANGE_TEMP_BANS_FOUND)] + "...|";
    }

    pChatCommand->m_pUser->SendCharDelayed(BanList);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Help(ChatCommand* pChatCommand) // !help
{
    int iMsgLen = PrepareReply(pChatCommand);
    if (iMsgLen == -1)
    {
        return true;
    }

    std::string help(ServerManager::m_pGlobalBuffer, iMsgLen);
    bool bFull = false;
    bool bTemp = false;

    HELP_LINE("%s:\n", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FOLOW_COMMANDS_AVAILABLE_TO_YOU)].c_str());

    if (pChatCommand->m_pUser->m_i32Profile != -1)
    {
        HELP_LINE("\n%s:\n", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILE_SPECIFIC_CMDS)].c_str());
        HELP_LINE("\t%cpasswd <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NEW_PASSWORD)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CHANGE_YOUR_PASSWORD)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::BAN))
    {
        bFull = true;
        HELP_LINE("\t%cban <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT)].c_str());
        HELP_LINE("\t%cbanip <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BAN_IP_ADDRESS)].c_str());
        HELP_LINE("\t%cfullban <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT)].c_str());
        HELP_LINE("\t%cfullbanip <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BAN_IP_ADDRESS)].c_str());
        HELP_LINE("\t%cnickban <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAN_USERS_NICK_IFCONN_THENDISCONN)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_BAN))
    {
        bFull = true;
        bTemp = true;
        HELP_LINE("\t%ctempban <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT)].c_str());
        HELP_LINE("\t%ctempbanip <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BAN_IP_ADDRESS)].c_str());
        HELP_LINE("\t%cfulltempban <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT)].c_str());
        HELP_LINE("\t%cfulltempbanip <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BAN_IP_ADDRESS)].c_str());
        HELP_LINE("\t%cnicktempban <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BAN_USERS_NICK_IFCONN_THENDISCONN)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::UNBAN) ||
        ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_UNBAN))
    {
        HELP_LINE("\t%cunban <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNBAN_IP_OR_NICK)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::UNBAN))
    {
        HELP_LINE("\t%cpermunban <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNBAN_PERM_BANNED_IP_OR_NICK)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_UNBAN))
    {
        HELP_LINE("\t%ctempunban <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNBAN_TEMP_BANNED_IP_OR_NICK)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST))
    {
        HELP_LINE("\t%cgetbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_LIST_OF_BANS)].c_str());
        HELP_LINE("\t%cgetpermbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_LIST_OF_PERM_BANS)].c_str());
        HELP_LINE("\t%cgettempbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_LIST_OF_TEMP_BANS)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLRPERMBAN))
    {
        HELP_LINE("\t%cclrpermbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CLEAR_PERM_BANS_LWR)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLRTEMPBAN))
    {
        HELP_LINE("\t%cclrtempbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CLEAR_TEMP_BANS_LWR)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_BAN))
    {
        bFull = true;
        HELP_LINE("\t%crangeban <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BAN_GIVEN_IP_RANGE)].c_str());
        HELP_LINE("\t%cfullrangeban <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BAN_GIVEN_IP_RANGE)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TBAN))
    {
        bFull = true;
        bTemp = true;
        HELP_LINE("\t%crangetempban <%s> <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BAN_GIVEN_IP_RANGE)].c_str());
        HELP_LINE("\t%cfullrangetempban <%s> <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BAN_GIVEN_IP_RANGE)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN) ||
        ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN))
    {
        HELP_LINE("\t%crangeunban <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNBAN_BANNED_IP_RANGE)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN))
    {
        HELP_LINE("\t%crangepermunban <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNBAN_PERM_BANNED_IP_RANGE)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN))
    {
        HELP_LINE("\t%crangetempunban <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNBAN_TEMP_BANNED_IP_RANGE)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS))
    {
        bFull = true;
        HELP_LINE("\t%cgetrangebans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_LIST_OF_RANGE_BANS)].c_str());
        HELP_LINE("\t%cgetrangepermbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_LIST_OF_RANGE_PERM_BANS)].c_str());
        HELP_LINE("\t%cgetrangetempbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_LIST_OF_RANGE_TEMP_BANS)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLR_RANGE_BANS))
    {
        HELP_LINE("\t%cclrrangepermbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CLEAR_PERM_RANGE_BANS_LWR)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLR_RANGE_TBANS))
    {
        HELP_LINE("\t%cclrrangetempbans - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CLEAR_TEMP_RANGE_BANS_LWR)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST))
    {
        HELP_LINE("\t%cchecknickban <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_BAN_FOUND_FOR_GIVEN_NICK)].c_str());
        HELP_LINE("\t%ccheckipban <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_BANS_FOUND_FOR_GIVEN_IP)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS))
    {
        HELP_LINE("\t%ccheckrangeban <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_RANGE_BAN_FOR_GIVEN_RANGE)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::DROP))
    {
        HELP_LINE("\t%cdrop <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISCONNECT_WITH_TEMPBAN)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETINFO))
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "\t%cgetinfo <%s> - %s."
#ifdef _WITH_SQLITE
                           " %s."
#endif
                           "\n",
                           SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_INFO_GIVEN_NICK)].c_str()
#ifdef _WITH_SQLITE
                               ,
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_CAN_USE_SQL_WILDCARDS)].c_str()
#endif
        );
        if (iMsgLen <= 0)
            return true;
        help.append(ServerManager::m_pGlobalBuffer, iMsgLen);
#ifdef _WITH_SQLITE
        HELP_LINE("\t%cgetipinfo <%s> - %s. %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_INFO_GIVEN_IP)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_CAN_USE_SQL_WILDCARDS)].c_str());
#endif
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMPOP))
    {
        HELP_LINE("\t%cop <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_GIVE_TEMP_OP)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GAG))
    {
        HELP_LINE("\t%cgag <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISALLOW_USER_TO_POST_IN_MAIN)].c_str());
        HELP_LINE("\t%cungag <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USER_CAN_POST_IN_MAIN_AGAIN)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTHUB))
    {
        HELP_LINE("\t%crestart - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESTART_HUB_LWR)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTSCRIPTS))
    {
        HELP_LINE("\t%cstartscript <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FILENAME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_START_SCRIPT_GIVEN_FILENAME)].c_str());
        HELP_LINE("\t%cstopscript <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FILENAME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STOP_SCRIPT_GIVEN_FILENAME)].c_str());
        HELP_LINE("\t%crestartscript <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FILENAME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESTART_SCRIPT_GIVEN_FILENAME)].c_str());
        HELP_LINE("\t%crestartscripts - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESTART_SCRIPTING_PART_OF_THE_HUB)].c_str());
        HELP_LINE("\t%cgetscripts - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISPLAY_LIST_OF_SCRIPTS)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::REFRESHTXT))
    {
        HELP_LINE("\t%creloadtxt - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RELOAD_ALL_TEXT_FILES)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::ADDREGUSER))
    {
        HELP_LINE("\t%creguser <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILENAME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REG_USER_WITH_PROFILE)].c_str());
        HELP_LINE("\t%caddreguser <%s> <%s> <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PASSWORD_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILENAME_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ADD_REG_USER_WITH_PROFILE)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::DELREGUSER))
    {
        HELP_LINE("\t%cdelreguser <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVE_REG_USER)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TOPIC))
    {
        HELP_LINE("\t%ctopic <%s> - %s %ctopic <off> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NEW_TOPIC)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SET_NEW_TOPIC_OR)].c_str(),
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CLEAR_TOPIC)].c_str());
    }

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::MASSMSG))
    {
        HELP_LINE("\t%cmassmsg <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SEND_MSG_TO_ALL_USERS)].c_str());
        HELP_LINE("\t%copmassmsg <%s> - %s.\n",
                  SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SEND_MSG_TO_ALL_OPS)].c_str());
    }

    if (bFull)
    {
        HELP_LINE("*** %s.\n", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_IS_ALWAYS_OPTIONAL)].c_str());
        HELP_LINE("*** %s.\n", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULLBAN_HELP_TXT)].c_str());
    }

    if (bTemp)
    {
        HELP_LINE("*** %s: m = %s, h = %s, d = %s, w = %s, M = %s, Y = %s.\n",
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMPBAN_TIME_VALUES)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MINUTES_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HOURS_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DAYS_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WEEKS_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MONTHS_LWR)].c_str(),
                  LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YEARS_LWR)].c_str());
    }

    HELP_LINE("\n%s:\n", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_GLOBAL_COMMANDS)].c_str());
    HELP_LINE("\t%cme <%s> - %s.\n",
              SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE_LWR)].c_str(),
              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SPEAK_IN_3RD_PERSON)].c_str());
    HELP_LINE("\t%cmyip - %s.|",
              SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
              LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SHOW_YOUR_IP)].c_str());

    pChatCommand->m_pUser->SendCharDelayed(help);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
