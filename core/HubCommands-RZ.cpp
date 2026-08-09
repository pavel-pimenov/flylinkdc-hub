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
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaInc.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "HubCommands.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#endif

#include "LuaScript.h"
#include "TextFileManager.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RestartScripts(ChatCommand* pChatCommand) // !restartscripts
{
    if (!CheckPermission(pChatCommand, ProfileManager::RSTSCRIPTS))
    {
        return true;
    }

    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScripts1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERR_SCRIPTS_DISABLED)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    // post restart message
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RestartScripts2",
                                                    "<%s> *** %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESTARTED_SCRIPTS)].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScripts3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTS_RESTARTED)].c_str());
    }

    ScriptManager::m_Ptr->Restart();

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Restart(ChatCommand* pChatCommand) // !restart
{
    if (!CheckPermission(pChatCommand, ProfileManager::RSTHUB))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    // Send message to all that we are going to restart the hub
    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "<%s> %s. %s.|",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HUB_WILL_BE_RESTARTED)].c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BACK_IN_FEW_MINUTES)].c_str());
    if (iMsgLen > 0)
    {
        Users::m_Ptr->SendChat2All(pChatCommand->m_pUser, ServerManager::m_pGlobalBuffer, iMsgLen, nullptr);
    }

    // post a restart hub message
    EventQueue::m_Ptr->AddNormal(EventQueue::EventType::RESTART, nullptr);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ReloadTxt(ChatCommand* pChatCommand) // !reloadtxt
{
    if (!CheckPermission(pChatCommand, ProfileManager::REFRESHTXT))
    {
        return true;
    }

    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_TEXT_FILES)])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ReloadTxt1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TXT_SUP_NOT_ENABLED)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    TextFilesManager::m_Ptr->RefreshTextFiles();

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::ReloadTxt2",
                                                    "<%s> *** %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RELOAD_TXT_FILES_LWR)].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ReloadTxt3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TXT_FILES_RELOADED)].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RestartScript(ChatCommand* pChatCommand) // !restartscript scriptname
{
    if (!CheckPermission(pChatCommand, ProfileManager::RSTSCRIPTS))
    {
        return true;
    }

    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERR_SCRIPTS_DISABLED)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 15 || pChatCommand->m_sCommand[14] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %crestartscript <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTNAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 14;
    pChatCommand->m_ui32CommandLen -= 14;

    if (pChatCommand->m_ui32CommandLen > 256)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %crestartscript <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTNAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS)].c_str());
        return true;
    }

    Script* const curScript = ScriptManager::m_Ptr->FindScript(pChatCommand->m_sCommand);
    if (!curScript || !curScript->m_bEnabled || !curScript->m_pLua)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_SCRIPT)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NOT_RUNNING)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    // stop script
    ScriptManager::m_Ptr->StopScript(curScript, false);

    // try to start script
    if (ScriptManager::m_Ptr->StartScript(curScript, false))
    {
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RestartScript5",
                                                        "<%s> *** %s %s: %s.|",
                                                        SettingManager::HubSec(),
                                                        pChatCommand->m_pUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESTARTED_SCRIPT)].c_str(),
                                                        pChatCommand->m_sCommand);
        }

        if (ShouldReplyPM(pChatCommand))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript6",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> %s %s %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPT)].c_str(),
                                                     pChatCommand->m_sCommand,
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESTARTED_LWR)].c_str());
        }
        return true;
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript7",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> *** %s %s %s.|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_SCRIPT)].c_str(),
                                             pChatCommand->m_sCommand,
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RESTART_FAILED)].c_str());
    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeBan(ChatCommand* pChatCommand) // !rangeban fromip toip reason
{
    if (!CheckPermission(pChatCommand, ProfileManager::RANGE_BAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 24)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %crangeban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 9;
    pChatCommand->m_ui32CommandLen -= 9;

    return RangeBan(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeTempBan(
    ChatCommand* pChatCommand) // !rangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
{
    if (!CheckPermission(pChatCommand, ProfileManager::RANGE_TBAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 31)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %crangetempban <%s> <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 13;
    pChatCommand->m_ui32CommandLen -= 13;

    return RangeTempBan(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeUnBan(ChatCommand* pChatCommand) // !rangeunban fromip toip
{
    if (!ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN) &&
        !ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN))
    {
        SendNoPermission(pChatCommand);
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 26)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnBan",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %crangeunban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 11;
    pChatCommand->m_ui32CommandLen -= 11;

    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN))
    {
        return RangeUnban(pChatCommand);
    }
    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN))
    {
        return RangeUnban(pChatCommand, BanManager::TEMP);
    }
    SendNoPermission(pChatCommand);
    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeTempUnBan(ChatCommand* pChatCommand) // !rangetempunban fromip toip
{
    if (!CheckPermission(pChatCommand, ProfileManager::RANGE_TUNBAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 30)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempUnBan",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %crangetempunban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 15;
    pChatCommand->m_ui32CommandLen -= 15;

    return RangeUnban(pChatCommand, BanManager::TEMP);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangePermUnBan(ChatCommand* pChatCommand) // !rangepermunban fromip toip
{
    if (!CheckPermission(pChatCommand, ProfileManager::RANGE_UNBAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 30)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangePermUnBan",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %crangepermunban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 15;
    pChatCommand->m_ui32CommandLen -= 15;

    return RangeUnban(pChatCommand, BanManager::PERM);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RegNewUser(ChatCommand* pChatCommand) // !reguser nick profile_name
{
    if (!CheckPermission(pChatCommand, ProfileManager::ADDREGUSER))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 255)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CMD_TOO_LONG)].c_str());
        return true;
    }

    char* sNick = pChatCommand->m_sCommand + 8; // nick start

    char* sProfile = strchr(pChatCommand->m_sCommand + 8, ' ');
    if (!sProfile)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %creguser <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILENAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    const auto ui32NickLen = static_cast<uint32_t>(sProfile - sNick);

    sProfile[0] = '\0';
    sProfile++;

    const auto optProfile = ProfileManager::m_Ptr->GetProfileIndex(sProfile);
    if (!optProfile)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST)].c_str());
        return true;
    }

    // check hierarchy
    // deny if pUser is not Master and tries add equal or higher profile
    if (pChatCommand->m_pUser->m_i32Profile > 0 && *optProfile <= pChatCommand->m_pUser->m_i32Profile)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE)].c_str());
        return true;
    }

    // check if user is registered
    if (RegManager::m_Ptr->Find(std::string_view(sNick, ui32NickLen)))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USER)].c_str(),
                                                 sNick,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREDY_REGISTERED)].c_str());
        return true;
    }

    User* pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(sNick, ui32NickLen));
    if (!pOtherUser)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 sNick,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_ONLINE)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    pOtherUser->SetBuffer(sProfile);
    pOtherUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;

    pOtherUser->SendFormat("HubCommands::RegNewUser7",
                           true,
                           "<%s> %s.|$GetPass|",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_WERE_REGISTERED_PLEASE_ENTER_YOUR_PASSWORD)].c_str());

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RegNewUser8",
                                                    "<%s> *** %s %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REGISTERED)].c_str(),
                                                    sNick,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_AS)].c_str(),
                                                    sProfile);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser9",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 sNick,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REGISTERED)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_AS)].c_str(),
                                                 sProfile);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Stats(ChatCommand* pChatCommand) // !stat !stats !statistic
{
    int iMsgLen = CheckFromPm(pChatCommand);

    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "<%s>", SettingManager::HubSec()))
    {
        return true;
    }

    std::string Statinfo(ServerManager::m_pGlobalBuffer, iMsgLen);

    Statinfo += "\n------------------------------------------------------------\n";
    Statinfo += "Current stats:\n";
    Statinfo += "------------------------------------------------------------\n";
    Statinfo += "Uptime: " + std::to_string(ServerManager::m_ui64Days) + " days, " + std::to_string(ServerManager::m_ui64Hours) + " hours, " +
                std::to_string(ServerManager::m_ui64Mins) + " minutes\n";

    Statinfo += "Version: PtokaX DC Hub " PtokaXVersionString

#ifdef _PtokaX_TESTING_
                " [build " BUILD_NUMBER "]"
#endif
                " built on " __DATE__ " " __TIME__ "\n"

#if LUA_VERSION_NUM > 501
                "Lua: " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE "\n";
#else
        LUA_RELEASE "\n";
#endif

#ifdef _WITH_SQLITE
    Statinfo += "SQLite: " SQLITE_VERSION "\n";
#endif

    struct utsname osname;
    if (uname(&osname) >= 0)
    {
        Statinfo += "OS: " + std::string(osname.sysname) + " " + std::string(osname.release) + " (" + std::string(osname.machine) +
                    ")"
#ifdef __clang__
                    " / Clang " __clang_version__
#elif __GNUC__
                    " / GCC " __VERSION__
#endif
                    "\n";
    }

    Statinfo += "Users (Max/Actual Peak (Max Peak)/Logged): " + std::to_string(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS)]) + " / " +
                std::to_string(ServerManager::m_ui32Peak) + " (" + std::to_string(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS_PEAK)]) + ") / " +
                std::to_string(ServerManager::m_ui32Logged) + "\n";
    Statinfo += "Joins / Parts: " + std::to_string(ServerManager::m_ui32Joins) + " / " + std::to_string(ServerManager::m_ui32Parts) + "\n";
    Statinfo += "Users shared size: " + std::to_string(ServerManager::m_ui64TotalShare) + " Bytes / " + formatBytes(ServerManager::m_ui64TotalShare) + "\n";
    Statinfo += "Chat messages: " + std::to_string(DcCommands::m_Ptr->m_ui32StatChat) + " x\n";
    Statinfo += "Unknown commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdUnknown) + " x\n";
    Statinfo += "PM commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdTo) + " x\n";
    Statinfo += "Key commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdKey) + " x\n";
    Statinfo += "Supports commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdSupports) + " x\n";
    Statinfo += "MyINFO commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdMyInfo) + " x\n";
    Statinfo += "ValidateNick commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdValidate) + " x\n";
    Statinfo += "GetINFO commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdGetInfo) + " x\n";
    Statinfo += "Password commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdMyPass) + " x\n";
    Statinfo += "Version commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdVersion) + " x\n";
    Statinfo += "GetNickList commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdGetNickList) + " x\n";
    Statinfo += "Search commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdSearch) + " x (" +
                std::to_string(DcCommands::m_Ptr->m_ui32StatCmdMultiSearch) + " x)\n";
    Statinfo += "SR commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdSR) + " x\n";
    Statinfo += "CTM commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdConnectToMe) + " x (" +
                std::to_string(DcCommands::m_Ptr->m_ui32StatCmdMultiConnectToMe) + " x)\n";
    Statinfo += "RevCTM commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdRevCTM) + " x\n";
    Statinfo += "BotINFO commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatBotINFO) + " x\n";
    Statinfo += "Close commands: " + std::to_string(DcCommands::m_Ptr->m_ui32StatCmdClose) + " x\n";
    Statinfo += "------------------------------------------------------------\n";

    Statinfo += "------------------------------------------------------------\n";
    Statinfo +=
        "SendRests (Peak): " + std::to_string(ServiceLoop::m_Ptr->m_ui32LastSendRest) + " (" + std::to_string(ServiceLoop::m_Ptr->m_ui32SendRestsPeak) + ")\n";
    Statinfo +=
        "RecvRests (Peak): " + std::to_string(ServiceLoop::m_Ptr->m_ui32LastRecvRest) + " (" + std::to_string(ServiceLoop::m_Ptr->m_ui32RecvRestsPeak) + ")\n";
    Statinfo += "Compression saved: " + formatBytes(ServerManager::m_ui64BytesSentSaved) + " (" + std::to_string(DcCommands::m_Ptr->m_ui32StatZPipe) + ")\n";
    Statinfo += "Data sent: " + formatBytes(ServerManager::m_ui64BytesSent) + "\n";
    Statinfo += "Data received: " + formatBytes(ServerManager::m_ui64BytesRead) + "\n";
    Statinfo += "Tx (60 sec avg): " + formatBytesPerSecond(ServerManager::m_ui32ActualBytesSent) + " (" +
                formatBytesPerSecond(ServerManager::m_ui32AverageBytesSent / 60) + ")\n";
    Statinfo += "Rx (60 sec avg): " + formatBytesPerSecond(ServerManager::m_ui32ActualBytesRead) + " (" +
                formatBytesPerSecond(ServerManager::m_ui32AverageBytesRead / 60) + ")|";

    pChatCommand->m_pUser->SendCharDelayed(Statinfo);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::StopScript(ChatCommand* pChatCommand) // !stopscript scriptname
{
    if (!CheckPermission(pChatCommand, ProfileManager::RSTSCRIPTS))
    {
        return true;
    }

    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERR_SCRIPTS_DISABLED)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 12)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cstopscript <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTNAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 11;
    pChatCommand->m_ui32CommandLen -= 11;

    if (pChatCommand->m_ui32CommandLen > 256)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cstopscript <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTNAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS)].c_str());
        return true;
    }

    Script* const curScript = ScriptManager::m_Ptr->FindScript(pChatCommand->m_sCommand);
    if (!curScript || !curScript->m_bEnabled || !curScript->m_pLua)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_SCRIPT)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NOT_RUNNING)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    ScriptManager::m_Ptr->StopScript(curScript, true);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::StopScript5",
                                                    "<%s> *** %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STOPPED_SCRIPT)].c_str(),
                                                    pChatCommand->m_sCommand);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPT)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STOPPED_LWR)].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::StartScript(ChatCommand* pChatCommand) // !startscript scriptname
{
    if (!CheckPermission(pChatCommand, ProfileManager::RSTSCRIPTS))
    {
        return true;
    }

    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERR_SCRIPTS_DISABLED)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 13)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cstartscript <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTNAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 12;
    pChatCommand->m_ui32CommandLen -= 12;

    if (pChatCommand->m_ui32CommandLen > 256)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cstartscript <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPTNAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS)].c_str());
        return true;
    }

    const char* sBadChar = strpbrk(pChatCommand->m_sCommand, "/\\");
    if (sBadChar || !FileExist((ServerManager::m_sScriptPath + pChatCommand->m_sCommand).c_str()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_SCRIPT_NOT_EXIST)].c_str());
        return true;
    }

    Script* const pScript = ScriptManager::m_Ptr->FindScript(pChatCommand->m_sCommand);
    if (pScript)
    {
        if (pScript->m_bEnabled && pScript->m_pLua)
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript5",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_SCRIPT_ALREDY_RUNNING)].c_str());
            return true;
        }

        UncountDeflood(pChatCommand);

        if (ScriptManager::m_Ptr->StartScript(pScript, true))
        {
            if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
            {
                GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::StartScript6",
                                                            "<%s> *** %s %s: %s.|",
                                                            SettingManager::HubSec(),
                                                            pChatCommand->m_pUser->m_sNick.c_str(),
                                                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STARTED_SCRIPT)].c_str(),
                                                            pChatCommand->m_sCommand);
            }

            if (ShouldReplyPM(pChatCommand))
            {
                pChatCommand->m_pUser->SendFormatCheckPM("HHubCommands::StartScript7",
                                                         GetHubSecPM(pChatCommand),
                                                         true,
                                                         "<%s> %s %s %s.|",
                                                         SettingManager::HubSec(),
                                                         LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPT)].c_str(),
                                                         pChatCommand->m_sCommand,
                                                         LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STARTED_LWR)].c_str());
            }
            return true;
        }

        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript8",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_SCRIPT)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_START_FAILED)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (ScriptManager::m_Ptr->AddScript(pChatCommand->m_sCommand, true, true) &&
        ScriptManager::m_Ptr->StartScript(ScriptManager::m_Ptr->m_ppScriptTable[static_cast<uint8_t>(ScriptManager::m_Ptr->m_ppScriptTable.size()) - 1], false))
    {
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::StartScript9",
                                                        "<%s> *** %s %s: %s.|",
                                                        SettingManager::HubSec(),
                                                        pChatCommand->m_pUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STARTED_SCRIPT)].c_str(),
                                                        pChatCommand->m_sCommand);
        }

        if (ShouldReplyPM(pChatCommand))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript10",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> %s %s %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SCRIPT)].c_str(),
                                                     pChatCommand->m_sCommand,
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STARTED_LWR)].c_str());
        }
        return true;
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript11",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> *** %s %s %s.|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_SCRIPT)].c_str(),
                                             pChatCommand->m_sCommand,
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_START_FAILED)].c_str());
    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempBan(ChatCommand* pChatCommand) // !tempban nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
{
    if (!CheckPermission(pChatCommand, ProfileManager::TEMP_BAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 11)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ctempban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 8;
    pChatCommand->m_ui32CommandLen -= 8;

    return TempBan(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempBanIp(ChatCommand* pChatCommand) // !tempbanip nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
{
    if (!CheckPermission(pChatCommand, ProfileManager::TEMP_BAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 14)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ctempbanip <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 10;
    pChatCommand->m_ui32CommandLen -= 10;

    return TempBanIp(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempUnban(ChatCommand* pChatCommand) // !tempunban nick/ip
{
    if (!CheckPermission(pChatCommand, ProfileManager::TEMP_UNBAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 11 || pChatCommand->m_sCommand[10] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ctempunban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ctempunban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 10;
    pChatCommand->m_ui32CommandLen -= 10;

    if (!BanManager::m_Ptr->TempUnban(pChatCommand->m_sCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_IN_MY_TEMP_BANS)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempUnban4",
                                                    "<%s> *** %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVED_LWR)].c_str(),
                                                    pChatCommand->m_sCommand,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROM_TEMP_BANS)].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVED_FROM_TEMP_BANS)].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Topic(ChatCommand* pChatCommand) // !topic text/off
{
    if (!CheckPermission(pChatCommand, ProfileManager::TOPIC))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 7 || pChatCommand->m_sCommand[6] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NEW_TOPIC)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 6;
    pChatCommand->m_ui32CommandLen -= 6;

    UncountDeflood(pChatCommand);

    if (pChatCommand->m_ui32CommandLen > 256)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NEW_TOPIC)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_TOPIC_LEN_256_CHARS)].c_str());
        return true;
    }

    if (strcasecmp(pChatCommand->m_sCommand, "off") == 0)
    {
        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC), "");

        GlobalDataQueue::m_Ptr->AddQueueItem(
            SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_NAME)], "", GlobalDataQueue::Cmd::HUBNAME);

        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOPIC_HAS_BEEN_CLEARED)].c_str());
    }
    else
    {
        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC), std::string_view(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen));

        GlobalDataQueue::m_Ptr->AddQueueItem(
            SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_NAME)], "", GlobalDataQueue::Cmd::HUBNAME);

        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s: %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOPIC_HAS_BEEN_SET_TO)].c_str(),
                                                 pChatCommand->m_sCommand);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Unban(ChatCommand* pChatCommand) // !unban nick/ip
{
    if (!CheckPermission(pChatCommand, ProfileManager::UNBAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 7 || pChatCommand->m_sCommand[6] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cunban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 106)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cunban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 6;
    pChatCommand->m_ui32CommandLen -= 6;

    if (!BanManager::m_Ptr->Unban(pChatCommand->m_sCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_IN_BANS)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Unban4",
                                                    "<%s> *** %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVED_LWR)].c_str(),
                                                    pChatCommand->m_sCommand,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROM_BANS)].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVED_FROM_BANS)].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Ungag(ChatCommand* pChatCommand) // !ungag nick
{
    if (!CheckPermission(pChatCommand, ProfileManager::GAG))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 7 || pChatCommand->m_sCommand[6] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cungag <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 106)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cungag <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 6;
    pChatCommand->m_ui32CommandLen -= 6;

    User* pOtherUser = FindUserOrReply(pChatCommand, pChatCommand->m_ui32CommandLen, "HubCommands::DoCommand->ungag3", std::to_underlying(LangIds::LAN_IS_NOT_IN_USERLIST));
    if (!pOtherUser)
    {
        return true;
    }

    if (!((pOtherUser->m_ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_GAGGED)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    pOtherUser->m_ui32BoolBits &= ~User::BIT_GAGGED;

    pOtherUser->SendFormat("HubCommands::DoCommand->ungag",
                           pChatCommand->m_bFromPM,
                           "<%s> %s %s.|",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_UNGAGGED_BY)].c_str(),
                           pChatCommand->m_pUser->m_sNick.c_str());

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::DoCommand->ungag",
                                                    "<%s> *** %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_UNGAGGED)].c_str(),
                                                    pOtherUser->m_sNick.c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pOtherUser->m_sNick.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_UNGAGGED)].c_str());
    }

    return true;
}
