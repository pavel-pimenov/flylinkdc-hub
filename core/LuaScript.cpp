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
#include "LuaInc.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "LuaCoreLib.h"
#include "LuaBanManLib.h"
#include "LuaIP2CountryLib.h"
#include "LuaProfManLib.h"
#include "LuaRegManLib.h"
#include "LuaScriptManLib.h"
#include "LuaSetManLib.h"
#include "LuaTmrManLib.h"
#include "ResNickManager.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static int ScriptPanic(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    size_t szLen = 0;
    const char* stmp = lua_tolstring(pLua, -1, &szLen);

    LogInfo("[LUA] At panic -> {}", std::string(stmp ? stmp : "(null)", szLen));

    return 0;
}
//------------------------------------------------------------------------------

ScriptBot::ScriptBot()
{
    ScriptManager::m_Ptr->m_ui8BotsCount++;
}
//------------------------------------------------------------------------------

ScriptBot::~ScriptBot()
{
    ScriptManager::m_Ptr->m_ui8BotsCount--;
}
//------------------------------------------------------------------------------

ScriptBot* ScriptBot::CreateScriptBot(const char* sBotNick,
                                      const size_t szNickLen,
                                      const char* sDescription,
                                      [[maybe_unused]] const size_t szDscrLen,
                                      const char* sEmail,
                                      [[maybe_unused]] const size_t szEmailLen,
                                      const bool bOP)
{
    auto pScriptBot = std::make_unique<ScriptBot>();

    pScriptBot->m_sNick.assign(sBotNick, szNickLen);
    pScriptBot->m_bIsOP = bOP;

    std::string buf;
    buf.resize(2048);
    const int iBufLen = snprintf(
        buf.data(), buf.size(), "$MyINFO $ALL %s %s$ $	$%s$$|", sBotNick, sDescription ? sDescription : "", sEmail ? sEmail : "");
    buf.resize(static_cast<size_t>(iBufLen));
    pScriptBot->m_sMyINFO = buf;

    return pScriptBot.release();
}
//------------------------------------------------------------------------------
// alex82 ... RegBot / �������� �������������� ������� ��� �������� ���� � ����������� $MyINFO
ScriptBot* ScriptBot::CreateScriptBot(const char* sBotNick, const size_t szNickLen, const char* sBotMyINFO, const size_t szMyINFOLen, const bool bOP)
{
    auto pScriptBot = std::make_unique<ScriptBot>();

    pScriptBot->m_sNick.assign(sBotNick, szNickLen);
    pScriptBot->m_bIsOP = bOP;

    pScriptBot->m_sMyINFO.assign(sBotMyINFO, szMyINFOLen);
    if (sBotMyINFO[szMyINFOLen - 1] != '|')
    {
        pScriptBot->m_sMyINFO += '|';
    }

    return pScriptBot.release();
}
//------------------------------------------------------------------------------

// ScriptTimer constructor and destructor are = default in header
//------------------------------------------------------------------------------

ScriptTimer* ScriptTimer::CreateScriptTimer(const char* sFunctName, const size_t szLen, const int iRef, lua_State* pLuaState)
{
    auto pScriptTimer = std::make_unique<ScriptTimer>();

    pScriptTimer->m_pLua = pLuaState;

    if (sFunctName)
    {
        pScriptTimer->m_sFunctionName.assign(sFunctName, szLen);
    }
    else
    {
        pScriptTimer->m_iFunctionRef = iRef;
    }

    return pScriptTimer.release();
}
//------------------------------------------------------------------------------

Script::~Script()
{
    if (m_pLua)
    {
        lua_close(m_pLua);
    }
}
//------------------------------------------------------------------------------

Script* Script::CreateScript(const char* sName, const bool enabled)
{
    auto pScript = std::make_unique<Script>();

    pScript->m_sName = sName;
    pScript->m_bEnabled = enabled;

    return pScript.release();
}
//------------------------------------------------------------------------------

static int OsExit(lua_State* /* pLua*/)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    EventQueue::m_Ptr->AddNormal(EventQueue::EventType::SHUTDOWN, nullptr);

    return 0;
}
//------------------------------------------------------------------------------

static void AddSettingIds(lua_State* pLua)
{
    const int iTable = lua_gettop(pLua);

    lua_newtable(pLua);
    int iNewTable = lua_gettop(pLua);

    const uint8_t ui8Bools[] = { // NOLINT(modernize-avoid-c-arrays)
        std::to_underlying(SetBoolIds::SETBOOL_ANTI_MOGLO),
        std::to_underlying(SetBoolIds::SETBOOL_AUTO_START),
        std::to_underlying(SetBoolIds::SETBOOL_REDIRECT_ALL),
        std::to_underlying(SetBoolIds::SETBOOL_REDIRECT_WHEN_HUB_FULL),
        std::to_underlying(SetBoolIds::SETBOOL_AUTO_REG),
        std::to_underlying(SetBoolIds::SETBOOL_REG_ONLY),
        std::to_underlying(SetBoolIds::SETBOOL_REG_ONLY_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_SHARE_LIMIT_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_SLOTS_LIMIT_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_HUB_SLOT_RATIO_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_MAX_HUBS_LIMIT_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_MODE_TO_MYINFO),
        std::to_underlying(SetBoolIds::SETBOOL_MODE_TO_DESCRIPTION),
        std::to_underlying(SetBoolIds::SETBOOL_STRIP_DESCRIPTION),
        std::to_underlying(SetBoolIds::SETBOOL_STRIP_TAG),
        std::to_underlying(SetBoolIds::SETBOOL_STRIP_CONNECTION),
        std::to_underlying(SetBoolIds::SETBOOL_STRIP_EMAIL),
        std::to_underlying(SetBoolIds::SETBOOL_REG_BOT),
        std::to_underlying(SetBoolIds::SETBOOL_USE_BOT_NICK_AS_HUB_SEC),
        std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT),
        std::to_underlying(SetBoolIds::SETBOOL_TEMP_BAN_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_PERM_BAN_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING),
        std::to_underlying(SetBoolIds::SETBOOL_KEEP_SLOW_USERS),
        std::to_underlying(SetBoolIds::SETBOOL_CHECK_NEW_RELEASES),
        std::to_underlying(SetBoolIds::SETBOOL_ENABLE_TRAY_ICON),
        std::to_underlying(SetBoolIds::SETBOOL_START_MINIMIZED),
        std::to_underlying(SetBoolIds::SETBOOL_FILTER_KICK_MESSAGES),
        std::to_underlying(SetBoolIds::SETBOOL_SEND_KICK_MESSAGES_TO_OPS),
        std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES),
        std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES_AS_PM),
        std::to_underlying(SetBoolIds::SETBOOL_ENABLE_TEXT_FILES),
        std::to_underlying(SetBoolIds::SETBOOL_SEND_TEXT_FILES_AS_PM),
        std::to_underlying(SetBoolIds::SETBOOL_STOP_SCRIPT_ON_ERROR),
        std::to_underlying(SetBoolIds::SETBOOL_MOTD_AS_PM),
        std::to_underlying(SetBoolIds::SETBOOL_DEFLOOD_REPORT),
        std::to_underlying(SetBoolIds::SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM),
        std::to_underlying(SetBoolIds::SETBOOL_DISABLE_MOTD),
        std::to_underlying(SetBoolIds::SETBOOL_DONT_ALLOW_PINGERS),
        std::to_underlying(SetBoolIds::SETBOOL_REPORT_PINGERS),
        std::to_underlying(SetBoolIds::SETBOOL_REPORT_3X_BAD_PASS),
        std::to_underlying(SetBoolIds::SETBOOL_ADVANCED_PASS_PROTECTION),
        std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP),
        std::to_underlying(SetBoolIds::SETBOOL_RESOLVE_TO_IP),
        std::to_underlying(SetBoolIds::SETBOOL_NICK_LIMIT_REDIR),
        std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_IP),
        std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_RANGE),
        std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_NICK),
        std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_REASON),
        std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_BY),
        std::to_underlying(SetBoolIds::SETBOOL_REPORT_SUSPICIOUS_TAG),
        std::to_underlying(SetBoolIds::SETBOOL_LOG_SCRIPT_ERRORS),
        std::to_underlying(SetBoolIds::SETBOOL_NO_QUACK_SUPPORTS),
        std::to_underlying(SetBoolIds::SETBOOL_HASH_PASSWORDS),
#ifdef _WITH_SQLITE
        std::to_underlying(SetBoolIds::SETBOOL_ENABLE_DATABASE),
#endif
    };

    const char* pBoolsNames[] = { // NOLINT(modernize-avoid-c-arrays)
        "AntiMoGlo",
        "AutoStart",
        "RedirectAll",
        "RedirectWhenHubFull",
        "AutoReg",
        "RegOnly",
        "RegOnlyRedir",
        "ShareLimitRedir",
        "SlotLimitRedir",
        "HubSlotRatioRedir",
        "MaxHubsLimitRedir",
        "ModeToMyInfo",
        "ModeToDescription",
        "StripDescription",
        "StripTag",
        "StripConnection",
        "StripEmail",
        "RegBot",
        "UseBotAsHubSec",
        "RegOpChat",
        "TempBanRedir",
        "PermBanRedir",
        "EnableScripting",
        "KeepSlowUsers",
        "CheckNewReleases",
        "EnableTrayIcon",
        "StartMinimized",
        "FilterKickMessages",
        "SendKickMessagesToOps",
        "SendStatusMessages",
        "SendStatusMessagesAsPm",
        "EnableTextFiles",
        "SendTextFilesAsPm",
        "StopScriptOnError",
        "SendMotdAsPm",
        "DefloodReport",
        "ReplyToHubCommandsAsPm",
        "DisableMotd",
        "DontAllowPingers",
        "ReportPingers",
        "Report3xBadPass",
        "AdvancedPassProtection",
        "ListenOnlySingleIp",
        "ResolveToIp",
        "NickLimitRedir",
        "BanMsgShowIp",
        "BanMsgShowRange",
        "BanMsgShowNick",
        "BanMsgShowReason",
        "BanMsgShowBy",
        "ReportSuspiciousTag",
        "LogScriptErrors",
        "DisallowBadSupports",
        "HashPasswords",
#ifdef _WITH_SQLITE
        "EnableDatabase",
#endif
    };

    for (size_t ui8i = 0; ui8i < sizeof(ui8Bools); ui8i++)
    {
        lua_pushinteger(pLua, ui8Bools[ui8i]);
        lua_setfield(pLua, iNewTable, pBoolsNames[ui8i]);
    }

    lua_setfield(pLua, iTable, "tBooleans");

    lua_newtable(pLua);
    iNewTable = lua_gettop(pLua);

    const uint8_t ui8Numbers[] = { // NOLINT(modernize-avoid-c-arrays)
        std::to_underlying(SetShortIds::SETSHORT_MAX_USERS),
        std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT),
        std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_UNITS),
        std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT),
        std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_UNITS),
        std::to_underlying(SetShortIds::SETSHORT_MIN_SLOTS_LIMIT),
        std::to_underlying(SetShortIds::SETSHORT_MAX_SLOTS_LIMIT),
        std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_HUBS),
        std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_SLOTS),
        std::to_underlying(SetShortIds::SETSHORT_MAX_HUBS_LIMIT),
        std::to_underlying(SetShortIds::SETSHORT_NO_TAG_OPTION),
        std::to_underlying(SetShortIds::SETSHORT_FULL_MYINFO_OPTION),
        std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LEN),
        std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LINES),
        std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LEN),
        std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LINES),
        std::to_underlying(SetShortIds::SETSHORT_DEFAULT_TEMP_BAN_TIME),
        std::to_underlying(SetShortIds::SETSHORT_MAX_PASIVE_SR),
        std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY),
        std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_TIME),
        std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_TIME),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_LINES),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_PM_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_PM_TIME),
        std::to_underlying(SetShortIds::SETSHORT_PM_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_SAME_PM_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SAME_PM_TIME),
        std::to_underlying(SetShortIds::SETSHORT_SAME_PM_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_LINES),
        std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_TIME),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_TIME),
        std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME),
        std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_TIME),
        std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_NEW_CONNECTIONS_COUNT),
        std::to_underlying(SetShortIds::SETSHORT_NEW_CONNECTIONS_TIME),
        std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_COUNT),
        std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_TEMP_BAN_TIME),
        std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_TIME),
        std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT),
        std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN),
        std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN),
        std::to_underlying(SetShortIds::SETSHORT_MIN_NICK_LEN),
        std::to_underlying(SetShortIds::SETSHORT_MAX_NICK_LEN),
        std::to_underlying(SetShortIds::SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE),
        std::to_underlying(SetShortIds::SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME),
        std::to_underlying(SetShortIds::SETSHORT_MAX_PM_COUNT_TO_USER),
        std::to_underlying(SetShortIds::SETSHORT_MAX_SIMULTANEOUS_LOGINS),
        std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_MESSAGES2),
        std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_PM_MESSAGES2),
        std::to_underlying(SetShortIds::SETSHORT_PM_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_PM_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_MESSAGES2),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES2),
        std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_MAX_MYINFO_LEN),
        std::to_underlying(SetShortIds::SETSHORT_CTM_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_CTM_TIME),
        std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_CTM_MESSAGES2),
        std::to_underlying(SetShortIds::SETSHORT_CTM_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_RCTM_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_RCTM_TIME),
        std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_RCTM_MESSAGES2),
        std::to_underlying(SetShortIds::SETSHORT_RCTM_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_MAX_CTM_LEN),
        std::to_underlying(SetShortIds::SETSHORT_MAX_RCTM_LEN),
        std::to_underlying(SetShortIds::SETSHORT_SR_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SR_TIME),
        std::to_underlying(SetShortIds::SETSHORT_SR_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_SR_MESSAGES2),
        std::to_underlying(SetShortIds::SETSHORT_SR_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_SR_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_MAX_SR_LEN),
        std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION),
        std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_KB),
        std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_TIME),
        std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION2),
        std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_KB2),
        std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_TIME2),
        std::to_underlying(SetShortIds::SETSHORT_CHAT_INTERVAL_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_CHAT_INTERVAL_TIME),
        std::to_underlying(SetShortIds::SETSHORT_PM_INTERVAL_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_PM_INTERVAL_TIME),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_INTERVAL_MESSAGES),
        std::to_underlying(SetShortIds::SETSHORT_SEARCH_INTERVAL_TIME),
        std::to_underlying(SetShortIds::SETSHORT_MAX_CONN_SAME_IP),
        std::to_underlying(SetShortIds::SETSHORT_MIN_RECONN_TIME),
#ifdef _WITH_SQLITE
        std::to_underlying(SetShortIds::SETSHORT_DB_REMOVE_OLD_RECORDS),
#endif
    };

    const char* pNumbersNames[] = { // NOLINT(modernize-avoid-c-arrays)
        "MaxUsers",
        "MinShareLimit",
        "MinShareUnits",
        "MaxShareLimit",
        "MaxShareUnits",
        "MinSlotsLimit",
        "MaxSlotsLimit",
        "HubSlotRatioHubs",
        "HubSlotRatioSlots",
        "MaxHubsLimit",
        "NoTagOption",
        "LongMyinfoOption",
        "MaxChatLen",
        "MaxChatLines",
        "MaxPmLen",
        "MaxPmLines",
        "DefaultTempBanTime",
        "MaxPasiveSr",
        "MyInfoDelay",
        "MainChatMessages",
        "MainChatTime",
        "MainChatAction",
        "SameMainChatMessages",
        "SameMainChatTime",
        "SameMainChatAction",
        "SameMultiMainChatMessages",
        "SameMultiMainChatLines",
        "SameMultiMainChatAction",
        "PmMessages",
        "PmTime",
        "PmAction",
        "SamePmMessages",
        "SamePmTime",
        "SamePmAction",
        "SameMultiPmMessages",
        "SameMultiPmLines",
        "SameMultiPmAction",
        "SearchMessages",
        "SearchTime",
        "SearchAction",
        "SameSearchMessages",
        "SameSearchTime",
        "SameSearchAction",
        "MyinfoMessages",
        "MyinfoTime",
        "MyinfoAction",
        "GetnicklistMessages",
        "GetnicklistTime",
        "GetnicklistAction",
        "NewConnectionsCount",
        "NewConnectionsTime",
        "DefloodWarningCount",
        "DefloodWarningAction",
        "DefloodTempBanTime",
        "GlobalMainChatMessages",
        "GlobalMainChatTime",
        "GlobalMainChatTimeout",
        "GlobalMainChatAction",
        "MinSearchLen",
        "MaxSearchLen",
        "MinNickLen",
        "MaxNickLen",
        "BruteForcePassProtectBanType",
        "BruteForcePassProtectTempBanTime",
        "MaxPmCountToUser",
        "MaxSimultaneousLogins",
        "MainChatMessages2",
        "MainChatTime2",
        "MainChatAction2",
        "PmMessages2",
        "PmTime2",
        "PmAction2",
        "SearchMessages2",
        "SearchTime2",
        "SearchAction2",
        "MyinfoMessages2",
        "MyinfoTime2",
        "MyinfoAction2",
        "MaxMyinfoLen",
        "CtmMessages",
        "CtmTime",
        "CtmAction",
        "CtmMessages2",
        "CtmTime2",
        "CtmAction2",
        "RctmMessages",
        "RctmTime",
        "RctmAction",
        "RctmMessages2",
        "RctmTime2",
        "RctmAction2",
        "MaxCtmLen",
        "MaxRctmLen",
        "SrMessages",
        "SrTime",
        "SrAction",
        "SrMessages2",
        "SrTime2",
        "SrAction2",
        "MaxSrLen",
        "MaxDownAction",
        "MaxDownKB",
        "MaxDownTime",
        "MaxDownAction2",
        "MaxDownKB2",
        "MaxDownTime2",
        "ChatIntervalMessages",
        "ChatIntervalTime",
        "PmIntervalMessages",
        "PmIntervalTime",
        "SearchIntervalMessages",
        "SearchIntervalTime",
        "MaxConnsSameIp",
        "MinReconnTime",
#ifdef _WITH_SQLITE
        "DbRemoveOldRecords",
#endif
    };

    for (size_t ui8i = 0; ui8i < sizeof(ui8Numbers); ui8i++)
    {
        lua_pushinteger(pLua, ui8Numbers[ui8i]);
        lua_setfield(pLua, iNewTable, pNumbersNames[ui8i]);
    }

    lua_setfield(pLua, iTable, "tNumbers");

    lua_newtable(pLua);
    iNewTable = lua_gettop(pLua);

    const uint8_t ui8Strings[] = { // NOLINT(modernize-avoid-c-arrays)
        std::to_underlying(SetTxtIds::SETTXT_HUB_NAME),
        std::to_underlying(SetTxtIds::SETTXT_ADMIN_NICK),
        std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS),
        std::to_underlying(SetTxtIds::SETTXT_UDP_PORT),
        std::to_underlying(SetTxtIds::SETTXT_HUB_DESCRIPTION),
        std::to_underlying(SetTxtIds::SETTXT_REDIRECT_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_REGISTER_SERVERS),
        std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_MSG),
        std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC),
        std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG),
        std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG),
        std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG),
        std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG),
        std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_NO_TAG_MSG),
        std::to_underlying(SetTxtIds::SETTXT_NO_TAG_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_BOT_NICK),
        std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION),
        std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL),
        std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK),
        std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION),
        std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL),
        std::to_underlying(SetTxtIds::SETTXT_TEMP_BAN_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_PERM_BAN_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES),
        std::to_underlying(SetTxtIds::SETTXT_HUB_OWNER_EMAIL),
        std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG),
        std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_REDIR_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_MSG_TO_ADD_TO_BAN_MSG),
        std::to_underlying(SetTxtIds::SETTXT_LANGUAGE),
        std::to_underlying(SetTxtIds::SETTXT_IPV4_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_IPV6_ADDRESS),
        std::to_underlying(SetTxtIds::SETTXT_ENCODING),
    };

    const char* pStringsNames[] = { // NOLINT(modernize-avoid-c-arrays)
        "HubName",
        "AdminNick",
        "HubAddress",
        "TCPPorts",
        "UDPPort",
        "HubDescription",
        "MainRedirectAddress",
        "HublistRegisterAddresses",
        "RegOnlyMessage",
        "RegOnlyRedirAddress",
        "HubTopic",
        "ShareLimitMessage",
        "ShareLimitRedirAddress",
        "SlotLimitMessage",
        "SlotLimitRedirAddress",
        "HubSlotRatioMessage",
        "HubSlotRatioRedirAddress",
        "MaxHubsLimitMessage",
        "MaxHubsLimitRedirAddress",
        "NoTagMessage",
        "NoTagRedirAddress",
        "HubBotNick",
        "HubBotDescription",
        "HubBotEmail",
        "OpChatNick",
        "OpChatDescription",
        "OpChatEmail",
        "TempBanRedirAddress",
        "PermBanRedirAddress",
        "ChatCommandsPrefixes",
        "HubOwnerEmail",
        "NickLimitMessage",
        "NickLimitRedirAddress",
        "MessageToAddToBanMessage",
        "Language",
        "IPv4Address",
        "IPv6Address",
        "Encoding",
    };

    for (size_t ui8i = 0; ui8i < sizeof(ui8Strings); ui8i++)
    {
        lua_pushinteger(pLua, ui8Strings[ui8i]);
        lua_setfield(pLua, iNewTable, pStringsNames[ui8i]);
    }

    lua_setfield(pLua, iTable, "tStrings");

    lua_pop(pLua, 1);
}
//------------------------------------------------------------------------------

static void AddPermissionsIds(lua_State* pLua)
{
    const int iTable = lua_gettop(pLua);

    lua_newtable(pLua);
    const int iNewTable = lua_gettop(pLua);

    const uint8_t ui8Permissions[] = {ProfileManager::HASKEYICON, // NOLINT(modernize-avoid-c-arrays)
                                      ProfileManager::NODEFLOODGETNICKLIST,
                                      ProfileManager::NODEFLOODMYINFO,
                                      ProfileManager::NODEFLOODSEARCH,
                                      ProfileManager::NODEFLOODPM,
                                      ProfileManager::NODEFLOODMAINCHAT,
                                      ProfileManager::MASSMSG,
                                      ProfileManager::TOPIC,
                                      ProfileManager::TEMP_BAN,
                                      ProfileManager::REFRESHTXT,
                                      ProfileManager::NOTAGCHECK,
                                      ProfileManager::TEMP_UNBAN,
                                      ProfileManager::DELREGUSER,
                                      ProfileManager::ADDREGUSER,
                                      ProfileManager::NOCHATLIMITS,
                                      ProfileManager::NOMAXHUBCHECK,
                                      ProfileManager::NOSLOTHUBRATIO,
                                      ProfileManager::NOSLOTCHECK,
                                      ProfileManager::NOSHARELIMIT,
                                      ProfileManager::CLRPERMBAN,
                                      ProfileManager::CLRTEMPBAN,
                                      ProfileManager::GETINFO,
                                      ProfileManager::GETBANLIST,
                                      ProfileManager::RSTSCRIPTS,
                                      ProfileManager::RSTHUB,
                                      ProfileManager::TEMPOP,
                                      ProfileManager::GAG,
                                      ProfileManager::REDIRECT,
                                      ProfileManager::BAN,
                                      ProfileManager::KICK,
                                      ProfileManager::DROP,
                                      ProfileManager::ENTERFULLHUB,
                                      ProfileManager::ENTERIFIPBAN,
                                      ProfileManager::ALLOWEDOPCHAT,
                                      ProfileManager::SENDALLUSERIP,
                                      ProfileManager::RANGE_BAN,
                                      ProfileManager::RANGE_UNBAN,
                                      ProfileManager::RANGE_TBAN,
                                      ProfileManager::RANGE_TUNBAN,
                                      ProfileManager::GET_RANGE_BANS,
                                      ProfileManager::CLR_RANGE_BANS,
                                      ProfileManager::CLR_RANGE_TBANS,
                                      ProfileManager::UNBAN,
                                      ProfileManager::NOSEARCHLIMITS,
                                      ProfileManager::SENDFULLMYINFOS,
                                      ProfileManager::NOIPCHECK,
                                      ProfileManager::CLOSE,
                                      ProfileManager::NODEFLOODCTM,
                                      ProfileManager::NODEFLOODRCTM,
                                      ProfileManager::NODEFLOODSR,
                                      ProfileManager::NODEFLOODRECV,
                                      ProfileManager::NOCHATINTERVAL,
                                      ProfileManager::NOPMINTERVAL,
                                      ProfileManager::NOSEARCHINTERVAL,
                                      ProfileManager::NOUSRSAMEIP,
                                      ProfileManager::NORECONNTIME};

    const char* pPermissionsNames[] = {"IsOperator", // NOLINT(modernize-avoid-c-arrays)
                                       "NoDefloodGetnicklist",
                                       "NoDefloodMyinfo",
                                       "NoDefloodSearch",
                                       "NoDefloodPm",
                                       "NoDefloodMainChat",
                                       "MassMsg",
                                       "Topic",
                                       "TempBan",
                                       "ReloadTxtFiles",
                                       "NoTagCheck",
                                       "TempUnban",
                                       "DelRegUser",
                                       "AddRegUser",
                                       "NoChatLimits",
                                       "NoMaxHubsCheck",
                                       "NoSlotHubRatioCheck",
                                       "NoSlotCheck",
                                       "NoShareLimit",
                                       "ClrPermBan",
                                       "ClrTempBan",
                                       "GetInfo",
                                       "GetBans",
                                       "ScriptControl",
                                       "RstHub",
                                       "TempOp",
                                       "GagUngag",
                                       "Redirect",
                                       "Ban",
                                       "Kick",
                                       "Drop",
                                       "EnterIfHubFull",
                                       "EnterIfIpBan",
                                       "AllowedOpChat",
                                       "SendAllUsersIp",
                                       "RangeBan",
                                       "RangeUnban",
                                       "RangeTempBan",
                                       "RangeTempUnban",
                                       "GetRangeBans",
                                       "ClrRangePermBans",
                                       "ClrRangeTempBans",
                                       "Unban",
                                       "NoSearchLimits",
                                       "SendLongMyinfos",
                                       "NoIpCheck",
                                       "Close",
                                       "NoDefloodCtm",
                                       "NoDefloodRctm",
                                       "NoDefloodSr",
                                       "NoDefloodRecv",
                                       "NoChatInterval",
                                       "NoPmInterval",
                                       "NoSearchInterval",
                                       "NoMaxUsrSameIp",
                                       "NoReconnTime"};

    for (size_t ui8i = 0; ui8i < sizeof(ui8Permissions); ui8i++)
    {
        lua_pushinteger(pLua, ui8Permissions[ui8i]);
        lua_setfield(pLua, iNewTable, pPermissionsNames[ui8i]);
    }

    lua_setfield(pLua, iTable, "tPermissions");

    lua_pop(pLua, 1);
}
//------------------------------------------------------------------------------

bool ScriptStart(Script* pScript)
{
    pScript->m_ui16Functions = UINT16_MAX;
    pScript->m_ui32DataArrivals = UINT32_MAX;

    pScript->m_pLua = luaL_newstate();

    if (!pScript->m_pLua)
    {
        return false;
    }

    luaL_openlibs(pScript->m_pLua);

    lua_atpanic(pScript->m_pLua, ScriptPanic);

    // replace internal lua os.exit with correct shutdown
    lua_getglobal(pScript->m_pLua, "os");

    if (lua_istable(pScript->m_pLua, -1))
    {
        lua_pushcfunction(pScript->m_pLua, OsExit);
        lua_setfield(pScript->m_pLua, -2, "exit");

        lua_pop(pScript->m_pLua, 1);
    }

    luaL_requiref(pScript->m_pLua, "Core", RegCore, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "SetMan", RegSetMan, 1);
    AddSettingIds(pScript->m_pLua);

    luaL_requiref(pScript->m_pLua, "RegMan", RegRegMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "BanMan", RegBanMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "ProfMan", RegProfMan, 1);
    AddPermissionsIds(pScript->m_pLua);

    luaL_requiref(pScript->m_pLua, "TmrMan", RegTmrMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "ScriptMan", RegScriptMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "IP2Country", RegIP2Country, 1);
    lua_pop(pScript->m_pLua, 1);

    if (luaL_dofile(pScript->m_pLua, (ServerManager::m_sScriptPath + pScript->m_sName).c_str()) == 0)
    {

        return true;
    }
    else
    {
        size_t szLen = 0;
        const char* stmp = lua_tolstring(pScript->m_pLua, -1, &szLen);

        const std::string sMsg(stmp ? stmp : "(null)", szLen);

        UdpDebug::m_Ptr->BroadcastFormat("[LUA] %s", sMsg.c_str());

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_LOG_SCRIPT_ERRORS)])
        {
            LogScript("{}", sMsg);
        }

        lua_close(pScript->m_pLua);
        pScript->m_pLua = nullptr;

        return false;
    }
}
//------------------------------------------------------------------------------

void ScriptStop(Script* pScript)
{
    auto& timerList = ScriptManager::m_Ptr->m_TimerList;
    for (auto it = timerList.begin(); it != timerList.end();)
    {
        if (pScript->m_pLua == (*it)->m_pLua)
        {
            it = timerList.erase(it);
            ScriptManager::m_Ptr->m_ui64TimerListGen++;
        }
        else
        {
            ++it;
        }
    }

    if (pScript->m_pLua)
    {
        lua_close(pScript->m_pLua);
        pScript->m_pLua = nullptr;
    }

    std::unique_ptr<ScriptBot> pBot;
    ScriptBot* next = pScript->m_pBotList;

    while (next)
    {
        pBot.reset(next);
        next = pBot->m_pNext;

        ReservedNicksManager::m_Ptr->DelReservedNick(pBot->m_sNick.c_str(), true);

        if (ServerManager::m_bServerRunning)
        {
            Users::m_Ptr->DelFromNickList(pBot->m_sNick.c_str(), pBot->m_bIsOP);

            Users::m_Ptr->DelBotFromMyInfos(pBot->m_sMyINFO.c_str());

            const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", pBot->m_sNick.c_str());
            if (iMsgLen > 0)
            {
                GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::QUIT);
            }
        }
    }

    pScript->m_pBotList = nullptr;
}
//------------------------------------------------------------------------------

int ScriptGetGC(Script* pScript)
{
    return lua_gc(pScript->m_pLua, LUA_GCCOUNT, 0);
}
//------------------------------------------------------------------------------

void ScriptOnStartup(Script* pScript)
{
    lua_pushcfunction(pScript->m_pLua, ScriptTraceback);
    const int iTraceback = lua_gettop(pScript->m_pLua);

    lua_getglobal(pScript->m_pLua, "OnStartup");
    const int i = lua_gettop(pScript->m_pLua);

    if (lua_isfunction(pScript->m_pLua, i) == 0)
    {
        pScript->m_ui16Functions &= ~Script::ONSTARTUP;
        lua_settop(pScript->m_pLua, 0);
        return;
    }

    if (lua_pcall(pScript->m_pLua, 0, 0, iTraceback) != 0)
    {
        ScriptError(pScript);

        lua_settop(pScript->m_pLua, 0);
        return;
    }

    // clear the stack for sure
    lua_settop(pScript->m_pLua, 0);
}
//------------------------------------------------------------------------------

void ScriptOnExit(Script* pScript)
{
    lua_pushcfunction(pScript->m_pLua, ScriptTraceback);
    const int iTraceback = lua_gettop(pScript->m_pLua);

    lua_getglobal(pScript->m_pLua, "OnExit");
    const int i = lua_gettop(pScript->m_pLua);
    if (lua_isfunction(pScript->m_pLua, i) == 0)
    {
        pScript->m_ui16Functions &= ~Script::ONEXIT;
        lua_settop(pScript->m_pLua, 0);
        return;
    }

    if (lua_pcall(pScript->m_pLua, 0, 0, iTraceback) != 0)
    {
        ScriptError(pScript);

        lua_settop(pScript->m_pLua, 0);
        return;
    }

    // clear the stack for sure
    lua_settop(pScript->m_pLua, 0);
}
//------------------------------------------------------------------------------

static bool ScriptOnError(Script* pScript, const char* sErrorMsg, const size_t szMsgLen)
{
    lua_pushcfunction(pScript->m_pLua, ScriptTraceback);
    const int iTraceback = lua_gettop(pScript->m_pLua);

    lua_getglobal(pScript->m_pLua, "OnError");
    const int i = lua_gettop(pScript->m_pLua);
    if (lua_isfunction(pScript->m_pLua, i) == 0)
    {
        pScript->m_ui16Functions &= ~Script::ONERROR;
        lua_settop(pScript->m_pLua, 0);
        return true;
    }

    ScriptManager::m_Ptr->m_pActualUser = nullptr;

    lua_pushlstring(pScript->m_pLua, sErrorMsg, szMsgLen);

    if (lua_pcall(pScript->m_pLua, 1, 0, iTraceback) != 0) // 1 passed parameters, zero returned
    {
        size_t szLen = 0;
        const char* stmp = lua_tolstring(pScript->m_pLua, -1, &szLen);

        const std::string sMsg(stmp ? stmp : "(null)", szLen);

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_LOG_SCRIPT_ERRORS)])
        {
            LogScript("{}", sMsg);
        }

        lua_settop(pScript->m_pLua, 0);
        return false;
    }

    // clear the stack for sure
    lua_settop(pScript->m_pLua, 0);
    return true;
}
//------------------------------------------------------------------------------

void ScriptPushUser(lua_State* pLua, User* pUser, const bool bFullTable /* = false*/)
{
    lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    const int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sNick");
    lua_pushlstring(pLua, pUser->m_sNick.c_str(), pUser->m_sNick.size());
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "uptr");
    lua_pushlightuserdata(pLua, static_cast<void*>(pUser));
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sIP");
    lua_pushlstring(pLua, pUser->m_sIP.data(), pUser->m_ui8IpLen);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iProfile");
    lua_pushinteger(pLua, pUser->m_i32Profile);
    lua_rawset(pLua, i);

    if (bFullTable)
    {
        ScriptPushUserExtended(pLua, pUser, i);
    }
}
//------------------------------------------------------------------------------

void ScriptPushUserExtended(lua_State* pLua, User* pUser, const int iTable)
{
    lua_pushliteral(pLua, "sMode");
    if (pUser->m_sModes[0] != '\0')
    {
        lua_pushstring(pLua, pUser->m_sModes.data());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sMyInfoString");
    if (!pUser->m_sMyInfoOriginal.empty())
    {
        lua_pushlstring(pLua, pUser->m_sMyInfoOriginal.data(), pUser->m_ui16MyInfoOriginalLen);
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sDescription");
    if (!pUser->m_sDescription.empty())
    {
        lua_pushlstring(pLua, pUser->m_sDescription.data(), pUser->m_sDescription.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sTag");
    if (!pUser->m_sTag.empty())
    {
        lua_pushlstring(pLua, pUser->m_sTag.data(), pUser->m_sTag.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sConnection");
    if (!pUser->m_sConnection.empty())
    {
        lua_pushlstring(pLua, pUser->m_sConnection.data(), pUser->m_sConnection.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sEmail");
    if (!pUser->m_sEmail.empty())
    {
        lua_pushlstring(pLua, pUser->m_sEmail.data(), pUser->m_sEmail.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sClient");
    if (!pUser->m_sClient.empty())
    {
        lua_pushlstring(pLua, pUser->m_sClient.data(), pUser->m_sClient.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sClientVersion");
    if (!pUser->m_sTagVersion.empty())
    {
        lua_pushlstring(pLua, pUser->m_sTagVersion.data(), pUser->m_sTagVersion.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);
#ifdef FLYLINKDC_USE_VERSION
    lua_pushliteral(pLua, "sVersion");
    if (!pUser->m_sVersion.empty())
    {
        lua_pushstring(pLua, pUser->m_sVersion.c_str());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);
#endif

    lua_pushliteral(pLua, "sCountryCode");
    if (IpP2Country::m_Ptr->m_ui32Count != 0)
    {
        lua_pushlstring(pLua, IpP2Country::m_Ptr->GetCountry(pUser->m_ui8Country, false), 2);
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bConnected");
    pUser->m_ui8State == User::UserStates::STATE_ADDED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bActive");
    if ((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
    {
        (pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    }
    else
    {
        (pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bOperator");
    (pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bUserCommand");
    (pUser->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bQuickList");
    (pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bSuspiciousTag");
    (pUser->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iShareSize");
    lua_pushinteger(pLua, static_cast<lua_Integer>(pUser->m_ui64SharedSize));

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iHubs");
    lua_pushinteger(pLua, pUser->m_ui32Hubs);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iNormalHubs");
    (pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, pUser->m_ui32NormalHubs);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iRegHubs");
    (pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, pUser->m_ui32RegHubs);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iOpHubs");
    (pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, pUser->m_ui32OpHubs);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iSlots");
    lua_pushinteger(pLua, pUser->m_ui32Slots);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iLlimit");
    lua_pushinteger(pLua, pUser->m_ui32LLimit);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iDefloodWarns");
    lua_pushinteger(pLua, pUser->m_ui32DefloodWarnings);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iMagicByte");
    lua_pushinteger(pLua, pUser->m_ui8MagicByte);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iLoginTime");
    lua_pushinteger(pLua, pUser->m_tLoginTime);

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sMac");
    std::array<char, 18> sMac = {};
    if (GetMacAddress(pUser->m_sIP.data(), sMac.data()))
    {
        lua_pushlstring(pLua, sMac.data(), 17);
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bDescriptionChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bTagChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bConnectionChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bEmailChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bShareChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedDescriptionShort");
    if (!pUser->m_sChangedDescriptionShort.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedDescriptionShort.data(), pUser->m_sChangedDescriptionShort.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedDescriptionLong");
    if (!pUser->m_sChangedDescriptionLong.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedDescriptionLong.data(), pUser->m_sChangedDescriptionLong.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedTagShort");
    if (!pUser->m_sChangedTagShort.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedTagShort.data(), pUser->m_sChangedTagShort.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedTagLong");
    if (!pUser->m_sChangedTagLong.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedTagLong.data(), pUser->m_sChangedTagLong.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedConnectionShort");
    if (!pUser->m_sChangedConnectionShort.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedConnectionShort.data(), pUser->m_sChangedConnectionShort.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedConnectionLong");
    if (!pUser->m_sChangedConnectionLong.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedConnectionLong.data(), pUser->m_sChangedConnectionLong.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedEmailShort");
    if (!pUser->m_sChangedEmailShort.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedEmailShort.data(), pUser->m_sChangedEmailShort.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedEmailLong");
    if (!pUser->m_sChangedEmailLong.empty())
    {
        lua_pushlstring(pLua, pUser->m_sChangedEmailLong.data(), pUser->m_sChangedEmailLong.size());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);

#ifdef USE_FLYLINKDC_EXT_JSON
    lua_pushliteral(pLua, "sExtJson");
    if (pUser->m_user_ext_info && !pUser->m_user_ext_info->GetExtJSONCommand().empty())
    {
        lua_pushlstring(pLua, pUser->m_user_ext_info->GetExtJSONCommand().c_str(), pUser->m_user_ext_info->GetExtJSONCommand().length());
    }
    else
    {
        lua_pushnil(pLua);
    }
    lua_rawset(pLua, iTable);
#endif

    lua_pushliteral(pLua, "iScriptediShareSizeShort");
    lua_pushinteger(pLua, static_cast<lua_Integer>(pUser->m_ui64ChangedSharedSizeShort));

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iScriptediShareSizeLong");
    lua_pushinteger(pLua, static_cast<lua_Integer>(pUser->m_ui64ChangedSharedSizeLong));

    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "tIPs");
    lua_newtable(pLua);

    const int t = lua_gettop(pLua);

    lua_pushinteger(pLua, 1);

    lua_pushlstring(pLua, pUser->m_sIP.data(), pUser->m_ui8IpLen);
    lua_rawset(pLua, t);

    if (pUser->m_sIPv4[0] != '\0')
    {
        lua_pushinteger(pLua, 2);

        lua_pushlstring(pLua, pUser->m_sIPv4.data(), pUser->m_ui8IPv4Len);
        lua_rawset(pLua, t);
    }

    lua_rawset(pLua, iTable);

    // alex82 ... HideUser / ������� �����
    lua_pushliteral(pLua, "bHidden");
    (pUser->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    // alex82 ... NoQuit / ��������� $Quit ��� �����
    lua_pushliteral(pLua, "bNoQuit");
    (pUser->m_ui32InfoBits & User::INFOBIT_NO_QUIT) == User::INFOBIT_NO_QUIT ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    // alex82 ... HideUserKey / ������ ���� �����
    lua_pushliteral(pLua, "bHiddenKey");
    (pUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);
}
//------------------------------------------------------------------------------

User* ScriptGetUser(lua_State* pLua, const int iTop, const char* sFunction)
{
    lua_pushliteral(pLua, "uptr");
    lua_gettable(pLua, 1);

    if (lua_gettop(pLua) != iTop + 1 || lua_type(pLua, iTop + 1) != LUA_TLIGHTUSERDATA)
    {
        luaL_error(pLua, "bad argument #1 to '%s' (it's not user table)", sFunction);
        return nullptr;
    }

    User* u = reinterpret_cast<User*>(lua_touserdata(pLua, iTop + 1));

    if (!u)
    {
        return nullptr;
    }

    if (u != ScriptManager::m_Ptr->m_pActualUser)
    {
        lua_pushliteral(pLua, "sNick");
        lua_gettable(pLua, 1);

        if (lua_gettop(pLua) != iTop + 2 || lua_type(pLua, iTop + 2) != LUA_TSTRING)
        {
            luaL_error(pLua, "bad argument #1 to '%s' (it's not user table)", sFunction);
            return nullptr;
        }

        size_t szNickLen;
        const char* sNick = lua_tolstring(pLua, iTop + 2, &szNickLen);

        if (u != HashManager::m_Ptr->FindUser(std::string_view(sNick, szNickLen)))
        {
            return nullptr;
        }
    }

    return u;
}
//------------------------------------------------------------------------------

void ScriptError(Script* pScript)
{
    lua_State* pLua = pScript ? pScript->m_pLua : nullptr;
    if (!pLua)
    {
        return;
    }

    size_t szLen = 0;
    const char* stmp = lua_tolstring(pLua, -1, &szLen);

    const std::string sMsg(stmp, szLen);

    UdpDebug::m_Ptr->BroadcastFormat("[LUA] %s", sMsg.c_str());

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_LOG_SCRIPT_ERRORS)])
    {
        LogScript("{}", sMsg);
    }

    if (pScript && ((((pScript->m_ui16Functions & Script::ONERROR) == Script::ONERROR) && !ScriptOnError(pScript, stmp, szLen)) ||
                               SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_STOP_SCRIPT_ON_ERROR)]))
    {
        // PPK ... stop buggy script ;)
        EventQueue::m_Ptr->AddNormal(EventQueue::EventType::STOPSCRIPT, pScript->m_sName.c_str());
    }
}
//------------------------------------------------------------------------------

void ScriptOnTimer(uint64_t ui64ActualMillis)
{
    lua_State* pLuaState = nullptr;

    auto& timerList = ScriptManager::m_Ptr->m_TimerList;

    for (auto& timerWrapper : timerList)
    {
        ScriptTimer* pCurTmr = timerWrapper.get();

        while ((pCurTmr->m_ui64LastTick < ui64ActualMillis) && (ui64ActualMillis - pCurTmr->m_ui64LastTick) >= pCurTmr->m_ui64Interval)
        {
            pCurTmr->m_ui64LastTick += pCurTmr->m_ui64Interval;
            lua_pushcfunction(pCurTmr->m_pLua, ScriptTraceback);
            const int iTraceback = lua_gettop(pCurTmr->m_pLua);

            if (!pCurTmr->m_sFunctionName.empty())
            {
                lua_getglobal(pCurTmr->m_pLua, pCurTmr->m_sFunctionName.c_str());
                const int i = lua_gettop(pCurTmr->m_pLua);

                if (lua_isfunction(pCurTmr->m_pLua, i) == 0)
                {
                    lua_settop(pCurTmr->m_pLua, 0);
                    continue;
                }
            }
            else
            {
                lua_rawgeti(pCurTmr->m_pLua, LUA_REGISTRYINDEX, pCurTmr->m_iFunctionRef);

                if (lua_isfunction(pCurTmr->m_pLua, -1) == 0)
                {
                    Script* sc = ScriptManager::m_Ptr->FindScript(pCurTmr->m_pLua);
                    if (sc)
                    {
                        LogDbg("[LUA] Timer func ref {} is not a function in script {}", pCurTmr->m_iFunctionRef, sc->m_sName);
                    }
                    lua_settop(pCurTmr->m_pLua, 0);
                    continue;
                }
            }

            ScriptManager::m_Ptr->m_pActualUser = nullptr;

            lua_checkstack(pCurTmr->m_pLua, 1); // we need 1 empty slots in stack, check it to be sure

            lua_pushlightuserdata(pCurTmr->m_pLua, static_cast<void*>(pCurTmr));

            pLuaState = pCurTmr->m_pLua; // For case when timer will be removed in OnTimer

            // Snapshot the generation counter so we can detect the timer list
            // being modified (add/remove) from inside the Lua callback below.
            const uint64_t ui64GenBefore = ScriptManager::m_Ptr->m_ui64TimerListGen;

            // 1 passed parameters, 0 returned
            struct timespec ts_before, ts_after;
            clock_gettime(CLOCK_MONOTONIC, &ts_before);
            const int lua_res = lua_pcall(pCurTmr->m_pLua, 1, 0, iTraceback);
            clock_gettime(CLOCK_MONOTONIC, &ts_after);

            Script* sc = ScriptManager::m_Ptr->FindScript(pLuaState);
            if (sc)
            {
                sc->m_ui32LuaCallCount++;
                sc->m_ui64LuaTimeNsec += (ts_after.tv_sec - ts_before.tv_sec) * 1000000000ULL + (ts_after.tv_nsec - ts_before.tv_nsec);
            }

            if (lua_res != 0)
            {
                ScriptError(sc);
            }

            // clear the stack for sure
            lua_settop(pLuaState, 0);

            // If the Lua callback modified the timer list (e.g. removed this
            // timer or another one), our iterator/pCurTmr may be dangling —
            // stop iterating to stay safe. std::list node addresses are stable,
            // but an erased pCurTmr would be freed.
            if (ScriptManager::m_Ptr->m_ui64TimerListGen != ui64GenBefore)
            {
                return;
            }
        }
    }
}
//------------------------------------------------------------------------------

int ScriptTraceback(lua_State* pLua)
{
    const char* sMsg = lua_tostring(pLua, 1);
    if (sMsg)
    {
        luaL_traceback(pLua, pLua, sMsg, 1);
        return 1;
    }

    return 0;
}
//------------------------------------------------------------------------------
