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
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#include "HubCommands.h"
//---------------------------------------------------------------------------
ChatCommand HubCommands::m_ChatCommand = {nullptr, nullptr, 0, false};
//---------------------------------------------------------------------------

bool HubCommands::DoCommand(User* pUser, char* sCommand, const uint32_t ui32CmdLen, bool bFromPM /* = false*/)
{
    m_ChatCommand.m_pUser = pUser;
    m_ChatCommand.m_sCommand = sCommand;
    m_ChatCommand.m_ui32CommandLen = ui32CmdLen;
    m_ChatCommand.m_bFromPM = bFromPM;

    if (!bFromPM)
    {
        // !me text
        if (strncasecmp(sCommand + pUser->m_sNick.size(), "me ", 3) == 0)
        {
            if (ui32CmdLen - (pUser->m_sNick.size() + 4) > 4)
            {
                sCommand[0] = '*';
                sCommand[1] = ' ';
                memcpy(sCommand + 2, pUser->m_sNick.data(), pUser->m_sNick.size());
                Users::m_Ptr->SendChat2All(pUser, sCommand, ui32CmdLen - 4, nullptr);

                return true;
            }

            return false;
        }

        // PPK ... optional reply commands in chat to PM
        m_ChatCommand.m_bFromPM = SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM)];

        m_ChatCommand.m_sCommand[ui32CmdLen - 5] = '\0'; // get rid of the pipe
        m_ChatCommand.m_sCommand += pUser->m_sNick.size();

        m_ChatCommand.m_ui32CommandLen = ui32CmdLen - (pUser->m_sNick.size() + 5);
    }
    else
    {
        m_ChatCommand.m_sCommand++;
        m_ChatCommand.m_ui32CommandLen = ui32CmdLen - (pUser->m_sNick.size() + 6);
    }

    switch (tolower(m_ChatCommand.m_sCommand[0]))
    {
    case 'g':
        // !getbans
        if (m_ChatCommand.m_ui32CommandLen == 7 && strncasecmp(m_ChatCommand.m_sCommand + 1, "etbans", 6) == 0)
        {
            return GetBans(&m_ChatCommand);
        }

        // !gag nick
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ag ", 3) == 0)
        {
            return Gag(&m_ChatCommand);
        }

        // !getinfo nick
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "etinfo ", 7) == 0)
        {
            return GetInfo(&m_ChatCommand);
        }

        // !getipinfo ip
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "etipinfo ", 9) == 0)
        {
            return GetIpInfo(&m_ChatCommand);
        }

        // !gettempbans
        if (m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand + 1, "ettempbans", 10) == 0)
        {
            return GetTempBans(&m_ChatCommand);
        }

        // !getscripts
        if (m_ChatCommand.m_ui32CommandLen == 10 && strncasecmp(m_ChatCommand.m_sCommand + 1, "etscripts", 9) == 0)
        {
            return GetScripts(&m_ChatCommand);
        }

        // !getpermbans
        if (m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand + 1, "etpermbans", 10) == 0)
        {
            return GetPermBans(&m_ChatCommand);
        }

        // !getrangebans
        if (m_ChatCommand.m_ui32CommandLen == 12 && strncasecmp(m_ChatCommand.m_sCommand + 1, "etrangebans", 11) == 0)
        {
            return GetRangeBans(&m_ChatCommand);
        }

        // !getrangepermbans
        if (m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand + 1, "etrangepermbans", 15) == 0)
        {
            return GetRangePermBans(&m_ChatCommand);
        }

        // !getrangetempbans
        if (m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand + 1, "etrangetempbans", 15) == 0)
        {
            return GetRangeTempBans(&m_ChatCommand);
        }

        return false;

    case 'n':
        // !nickban nick reason
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ickban ", 7) == 0)
        {
            return NickBan(&m_ChatCommand);
        }

        // !nicktempban nick time reason ...  m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "icktempban ", 11) == 0)
        {
            return NickTempBan(&m_ChatCommand);
        }

        return false;

    case 'b':
        // !ban nick reason
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "an ", 3) == 0)
        {
            return Ban(&m_ChatCommand);
        }

        // !banip ip reason
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "anip ", 5) == 0)
        {
            return BanIp(&m_ChatCommand);
        }

        return false;

    case 't':
        // !tempban nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "empban ", 7) == 0)
        {
            return TempBan(&m_ChatCommand);
        }

        // !tempbanip nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "empbanip ", 9) == 0)
        {
            return TempBanIp(&m_ChatCommand);
        }

        // !tempunban nick/ip
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "empunban ", 9) == 0)
        {
            return TempUnban(&m_ChatCommand);
        }

        // !topic text/off
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "opic ", 5) == 0)
        {
            return Topic(&m_ChatCommand);
        }

        return false;

    case 'm':
        // !myip
        if (m_ChatCommand.m_ui32CommandLen == 4 && strncasecmp(m_ChatCommand.m_sCommand + 1, "yip", 3) == 0)
        {
            return MyIp(&m_ChatCommand);
        }

        // !massmsg text
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "assmsg ", 7) == 0)
        {
            return MassMsg(&m_ChatCommand);
        }

        return false;

    case 'r':
        // !restartscripts
        if (m_ChatCommand.m_ui32CommandLen == 14 && strncasecmp(m_ChatCommand.m_sCommand + 1, "estartscripts", 13) == 0)
        {
            return RestartScripts(&m_ChatCommand);
        }

        // !restart
        if (m_ChatCommand.m_ui32CommandLen == 7 && strncasecmp(m_ChatCommand.m_sCommand + 1, "estart", 6) == 0)
        {
            return Restart(&m_ChatCommand);
        }

        // !reloadtxt
        if (m_ChatCommand.m_ui32CommandLen == 9 && strncasecmp(m_ChatCommand.m_sCommand + 1, "eloadtxt", 8) == 0)
        {
            return ReloadTxt(&m_ChatCommand);
        }

        // !restartscript scriptname
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "estartscript ", 13) == 0)
        {
            return RestartScript(&m_ChatCommand);
        }

        // !rangeban fromip toip reason
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "angeban ", 8) == 0)
        {
            return RangeBan(&m_ChatCommand);
        }

        // !rangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "angetempban ", 12) == 0)
        {
            return RangeTempBan(&m_ChatCommand);
        }

        // !rangeunban fromip toip
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "angeunban ", 10) == 0)
        {
            return RangeUnBan(&m_ChatCommand);
        }

        // !rangetempunban fromip toip
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "angetempunban ", 14) == 0)
        {
            return RangeTempUnBan(&m_ChatCommand);
        }

        // !rangepermunban fromip toip
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "angepermunban ", 14) == 0)
        {
            return RangePermUnBan(&m_ChatCommand);
        }

        // !reguser nick profile_name
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "eguser ", 7) == 0)
        {
            return RegNewUser(&m_ChatCommand);
        }

        return false;

    case 'u':
        // !unban nick/ip
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "nban ", 5) == 0)
        {
            return Unban(&m_ChatCommand);
        }

        // !ungag nick
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ngag ", 5) == 0)
        {
            return Ungag(&m_ChatCommand);
        }

        return false;

    case 'o':
        // !op nick
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "p ", 2) == 0)
        {
            return Op(&m_ChatCommand);
        }

        // !opmassmsg text
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "pmassmsg ", 9) == 0)
        {
            return OpMassMsg(&m_ChatCommand);
            ;
        }

        return false;

    case 'd':
        // !drop nick
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "rop ", 4) == 0)
        {
            return Drop(&m_ChatCommand);
        }

        // !delreguser nick
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "elreguser ", 10) == 0)
        {
            return DelRegUser(&m_ChatCommand);
        }

        // !debug port/off
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ebug ", 5) == 0)
        {
            return Debug(&m_ChatCommand);
        }

        return false;

    case 'c':
        // !clrtempbans
        if (m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand + 1, "lrtempbans", 10) == 0)
        {
            return ClrTempBans(&m_ChatCommand);
        }

        // !clrpermbans
        if (m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand + 1, "lrpermbans", 10) == 0)
        {
            return ClrPermBans(&m_ChatCommand);
        }

        // !clrrangetempbans
        if (m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand + 1, "lrrangetempbans", 15) == 0)
        {
            return ClrRangeTempBans(&m_ChatCommand);
        }

        // !clrrangepermbans
        if (m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand + 1, "lrrangepermbans", 15) == 0)
        {
            return ClrRangePermBans(&m_ChatCommand);
        }

        // !checknickban nick
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "hecknickban ", 12) == 0)
        {
            return CheckNickBan(&m_ChatCommand);
        }

        // !checkipban ip
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "heckipban ", 10) == 0)
        {
            return CheckIpBan(&m_ChatCommand);
        }

        // !checkrangeban ipfrom ipto
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "heckrangeban ", 13) == 0)
        {
            return CheckRangeBan(&m_ChatCommand);
        }

        return false;

    case 'a':
        // !addreguser nick pas> profile_name
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ddreguser ", 10) == 0)
        {
            return AddRegUser(&m_ChatCommand);
        }

        return false;

    case 'h':
        // !help
        if (m_ChatCommand.m_ui32CommandLen == 4 && strncasecmp(m_ChatCommand.m_sCommand + 1, "elp", 3) == 0)
        {
            return Help(&m_ChatCommand);
        }

        return false;

    case 's':
        // !stat !stats !statistic
        if ((m_ChatCommand.m_ui32CommandLen == 4 && strncasecmp(m_ChatCommand.m_sCommand + 1, "tat", 3) == 0) ||
            (m_ChatCommand.m_ui32CommandLen == 5 && strncasecmp(m_ChatCommand.m_sCommand + 1, "tats", 4) == 0) ||
            (m_ChatCommand.m_ui32CommandLen == 9 && strncasecmp(m_ChatCommand.m_sCommand + 1, "tatistic", 8) == 0))
        {
            return Stats(&m_ChatCommand);
        }

        // !stopscript scriptname
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "topscript ", 10) == 0)
        {
            return StopScript(&m_ChatCommand);
        }

        // !startscript scriptname
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "tartscript ", 11) == 0)
        {
            return StartScript(&m_ChatCommand);
        }

        return false;

    case 'p':
        // !passwd password
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "asswd ", 6) == 0)
        {
            return Passwd(&m_ChatCommand);
        }

        // !permunban what
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ermunban ", 9) == 0)
        {
            return PermUnban(&m_ChatCommand);
        }

        return false;

    case 'f':
        // !fullban nick reason
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ullban ", 7) == 0)
        {
            return FullBan(&m_ChatCommand);
        }

        // !fullbanip ip reason
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ullbanip ", 9) == 0)
        {
            return FullBanIp(&m_ChatCommand);
        }

        // Hub commands: !fulltempban nick time reason ... PPK m = minutes, h = hours, d = days, w = weeks, M = months , Y = years
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ulltempban ", 11) == 0)
        {
            return FullTempBan(&m_ChatCommand);
        }

        // !fulltempbanip ip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ulltempbanip ", 13) == 0)
        {
            return FullTempBanIp(&m_ChatCommand);
        }

        // !fullrangeban fromip toip reason
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ullrangeban ", 12) == 0)
        {
            return FullRangeBan(&m_ChatCommand);
        }

        // !fullrangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
        if (strncasecmp(m_ChatCommand.m_sCommand + 1, "ullrangetempban ", 16) == 0)
        {
            return FullRangeTempBan(&m_ChatCommand);
        }

        return false;
    default:
        break;
    }

    return false; // PPK ... and send to all as chat ;)
}
//---------------------------------------------------------------------------

bool HubCommands::Ban(ChatCommand* pChatCommand, const bool bFull)
{
    char* sReason = strchr(pChatCommand->m_sCommand, ' ');
    if (sReason)
    {
        sReason[0] = '\0';

        if (sReason[1] == '\0')
        {
            pChatCommand->m_ui32CommandLen = static_cast<uint32_t>(sReason - pChatCommand->m_sCommand);

            sReason = nullptr;
        }
        else
        {
            sReason++;

            TruncateReason(sReason);

            pChatCommand->m_ui32CommandLen = static_cast<uint32_t>(sReason - pChatCommand->m_sCommand) - 1;
        }
    }

    if (pChatCommand->m_sCommand[0] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%sban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%sban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    // Self-ban ?
    if (!CheckSelfPermission(pChatCommand, std::to_underlying(LangIds::LAN_YOU_CANT_BAN_YOURSELF)))
    {
        return true;
    }

    User* pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen));
    if (pOtherUser)
    {
        // PPK don't ban user with higher profile
        if (!CheckHigherProfile(pChatCommand, pOtherUser, std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_BAN)))
        {
            return true;
        }

        UncountDeflood(pChatCommand);

        // Ban user
        BanManager::m_Ptr->Ban(pOtherUser, sReason, pChatCommand->m_pUser->m_sNick.c_str(), bFull);

        // Send user a message that he has been banned
        pOtherUser->SendFormat("HubCommands::Ban5",
                               false,
                               "<%s> %s: %s.|",
                               SettingManager::HubSec(),
                               LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_HAD_BEEN_BANNED_BCS)].c_str(),
                               !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);

        // Send message to all OPs that the user have been banned
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Ban6",
                                                        "<%s> *** %s %s %s %s %s%s %s %s %s: %s.|",
                                                        SettingManager::HubSec(),
                                                        pChatCommand->m_sCommand,
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                        pOtherUser->m_sIP.data(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                        bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                        pChatCommand->m_pUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                        !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);
        }

        if (ShouldReplyPM(pChatCommand))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban7",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %s %s %s %s%s.|",
                                                     SettingManager::HubSec(),
                                                     pChatCommand->m_sCommand,
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                     pOtherUser->m_sIP.data(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                     bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str());
        }

        // Finish him !
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) %sbanned by %s",
                                         pOtherUser->m_sNick.c_str(),
                                         pOtherUser->m_sIP.data(),
                                         bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                         pChatCommand->m_pUser->m_sNick.c_str());
        pOtherUser->Close();

        return true;
    }
    if (bFull)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban8",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_ONLINE)].c_str());
        return true;
    }
    return NickBan(pChatCommand, sReason);
}
//---------------------------------------------------------------------------

bool HubCommands::BanIp(ChatCommand* pChatCommand, const bool bFull)
{
    char* sReason = strchr(pChatCommand->m_sCommand, ' ');
    if (sReason)
    {
        sReason[0] = '\0';

        if (sReason[1] == '\0')
        {
            sReason = nullptr;
        }
        else
        {
            sReason++;

            TruncateReason(sReason);
        }
    }

    if (pChatCommand->m_sCommand[0] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%sbanip <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    // Check IP!
    if (!isIP(pChatCommand->m_sCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%sbanip <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_VALID_IP_SPECIFIED)].c_str());
        return true;
    }

    switch (BanManager::m_Ptr->BanIp(nullptr, pChatCommand->m_sCommand, sReason, pChatCommand->m_pUser->m_sNick.c_str(), bFull))
    {
    case 0:
    {
        UncountDeflood(pChatCommand);

        CloseIpBannedUsers(pChatCommand, sReason);

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::BanIp5",
                                                        "<%s> *** %s %s %s%s %s %s %s: %s.|",
                                                        SettingManager::HubSec(),
                                                        pChatCommand->m_sCommand,
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_LWR)].c_str(),
                                                        bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                        pChatCommand->m_pUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                        !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);
        }

        if (ShouldReplyPM(pChatCommand))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp6",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> %s %s %s%s.|",
                                                     SettingManager::HubSec(),
                                                     pChatCommand->m_sCommand,
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_LWR)].c_str(),
                                                     bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str());
        }

        return true;
    }
    case 1:
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp7",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%sbanip <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_VALID_IP_SPECIFIED)].c_str());

        return true;
    }
    case 2:
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp8",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s %s%s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREADY)].c_str(),
                                                 bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str());

        return true;
    }
    default:
        return true;
    }
}
//---------------------------------------------------------------------------

bool HubCommands::NickBan(ChatCommand* pChatCommand, const char* sReason)
{
    RegUser* pReg = RegManager::m_Ptr->Find(std::string_view(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen));

    // don't nickban user with higher profile
    if (pReg && pChatCommand->m_pUser->m_i32Profile > pReg->m_ui16Profile)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAN_LWR)].c_str(),
                                                 pChatCommand->m_sCommand);
        return true;
    }

    if (BanManager::m_Ptr->NickBan(nullptr, pChatCommand->m_sCommand, sReason, pChatCommand->m_pUser->m_sNick.c_str()))
    {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s nickbanned by %s", pChatCommand->m_sCommand, pChatCommand->m_pUser->m_sNick.c_str());
    }
    else
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREDY_BANNED)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::NickBan3",
                                                    "<%s> *** %s %s %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_sCommand,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN_BANNED_BY)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ADDED_TO_BANS)].c_str());
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempBan(ChatCommand* pChatCommand, const bool bFull)
{
    std::array<char*, 3> sCmdParts = {nullptr, nullptr, nullptr};
    std::array<uint16_t, 3> ui16CmdPartsLen = {0, 0, 0};

    (void)ParseCmdParts(pChatCommand, 0, sCmdParts.data(), ui16CmdPartsLen.data(), 3);

    if (ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0 || !sCmdParts[1])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (ui16CmdPartsLen[0] > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    // Self-ban ?
    if (strcasecmp(sCmdParts[0], pChatCommand->m_pUser->m_sNick.c_str()) == 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_CANT_BAN_YOURSELF)].c_str());
        return true;
    }

    User* pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(sCmdParts[0], ui16CmdPartsLen[0]));
    if (pOtherUser)
    {
        // PPK don't tempban user with higher profile
        if (!CheckHigherProfile(pChatCommand, pOtherUser, std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_TEMPBAN)))
        {
            return true;
        }

        const uint8_t ui8Time = sCmdParts[1][ui16CmdPartsLen[1] - 1];
        sCmdParts[1][ui16CmdPartsLen[1] - 1] = '\0';
        int iTime = 0;
        if (!safe_stoi(sCmdParts[1], iTime))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan5",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                     bFull ? "full" : "",
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
            return true;
        }
        time_t acc_time, ban_time;

        if (iTime <= 0 || !GenerateTempBanTime(ui8Time, static_cast<uint32_t>(iTime), acc_time, ban_time))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan5",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                     bFull ? "full" : "",
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
            return true;
        }

        BanManager::m_Ptr->TempBan(pOtherUser, sCmdParts[2], pChatCommand->m_pUser->m_sNick.c_str(), 0, ban_time, bFull);
        UncountDeflood(pChatCommand);

        const std::string sTime = formatTime((ban_time - acc_time) / 60);

        // Send user a message that he has been tempbanned
        pOtherUser->SendFormat("HubCommands::TempBan6",
                               false,
                               "<%s> %s: %s %s: %s.|",
                               SettingManager::HubSec(),
                               LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_HAD_BEEN_TEMP_BANNED_TO)].c_str(),
                               sTime.c_str(),
                               LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                               !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[2]);

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempBan7",
                                                        "<%s> *** %s %s %s %s %s%s %s %s %s: %s %s: %s.|",
                                                        SettingManager::HubSec(),
                                                        sCmdParts[0],
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                        pOtherUser->m_sIP.data(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                        bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                        pChatCommand->m_pUser->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                        sTime.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                        !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str()
                                                                                : sCmdParts[2]);
        }

        if (ShouldReplyPM(pChatCommand))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan8",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> %s %s %s %s %s%s %s: %s %s: %s.|",
                                                     SettingManager::HubSec(),
                                                     sCmdParts[0],
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                     pOtherUser->m_sIP.data(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                     bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                     sTime.c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                     !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str()
                                                                             : sCmdParts[2]);
        }

        // Finish him !
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) %stemp banned by %s",
                                         pOtherUser->m_sNick.c_str(),
                                         pOtherUser->m_sIP.data(),
                                         bFull ? "full " : "",
                                         pChatCommand->m_pUser->m_sNick.c_str());

        pOtherUser->Close();
        return true;
    }
    if (bFull)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan9",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_ONLINE)].c_str());
        return true;
    }
    return TempNickBan(pChatCommand, sCmdParts[0], sCmdParts[1], ui16CmdPartsLen[1], sCmdParts[2], true);
}
//---------------------------------------------------------------------------

bool HubCommands::TempBanIp(ChatCommand* pChatCommand, const bool bFull)
{
    std::array<char*, 3> sCmdParts = {nullptr, nullptr, nullptr};
    std::array<uint16_t, 3> ui16CmdPartsLen = {0, 0, 0};

    (void)ParseCmdParts(pChatCommand, 0, sCmdParts.data(), ui16CmdPartsLen.data(), 3);

    if (ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0 || !sCmdParts[1])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    const uint8_t ui8Time = sCmdParts[1][ui16CmdPartsLen[1] - 1];
    sCmdParts[1][ui16CmdPartsLen[1] - 1] = '\0';
    int iTime = 0;
    if (!safe_stoi(sCmdParts[1], iTime))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }
    time_t acc_time, ban_time;

    if (iTime <= 0 || !GenerateTempBanTime(ui8Time, static_cast<uint32_t>(iTime), acc_time, ban_time))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }

    // Check IP!
    if (!isIP(sCmdParts[0]))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp2-1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_VALID_IP_SPECIFIED)].c_str());
        return true;
    }

    switch (BanManager::m_Ptr->TempBanIp(nullptr, sCmdParts[0], sCmdParts[2], pChatCommand->m_pUser->m_sNick.c_str(), 0, ban_time, bFull))
    {
    case 0:
    {
        CloseIpBannedUsers(pChatCommand, sCmdParts[2]);
        break;
    }
    case 1:
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%sbanip <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_VALID_IP_SPECIFIED)].c_str());
        return true;
    }
    case 2:
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s %s%s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREADY)].c_str(),
                                                 bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LONGER_TIME)].c_str());
        return true;
    }
    default:
        return true;
    }

    UncountDeflood(pChatCommand);

    const std::string sTime = formatTime((ban_time - acc_time) / 60);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempBanIp",
                                                    "<%s> *** %s %s %s%s %s %s %s: %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_sCommand,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                    bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                    sTime.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[2]);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s %s%s %s: %s %s: %s.|",
                                                 SettingManager::HubSec(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN)].c_str(),
                                                 bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                 sTime.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                 !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[2]);
    }

    UdpDebug::m_Ptr->BroadcastFormat("[SYS] IP %s %stemp banned by %s", sCmdParts[0], bFull ? "full " : "", pChatCommand->m_pUser->m_sNick.c_str());

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::TempNickBan(
    ChatCommand* pChatCommand, const char* sNick, char* sTime, const uint16_t ui16TimeLen, const char* sReason, const bool bNotNickBan /* = false*/)
{
    RegUser* const pReg = RegManager::m_Ptr->Find(sNick);

    // don't nickban user with higher profile
    if (pReg && pChatCommand->m_pUser->m_i32Profile > pReg->m_ui16Profile)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAN_LWR)].c_str(),
                                                 sNick);
        return true;
    }

    const uint8_t ui8Time = sTime[ui16TimeLen - 1];
    sTime[ui16TimeLen - 1] = '\0';
    int iTime = 0;
    if (!safe_stoi(sTime, iTime))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 !bNotNickBan ? "nick" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }
    time_t acc_time, ban_time;

    if (iTime <= 0 || !GenerateTempBanTime(ui8Time, static_cast<uint32_t>(iTime), acc_time, ban_time))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 !bNotNickBan ? "nick" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }

    if (!BanManager::m_Ptr->NickTempBan(nullptr, sNick, sReason, pChatCommand->m_pUser->m_sNick.c_str(), 0, ban_time))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                                                 sNick,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALRD_BANNED_LONGER_TIME)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    const std::string sBanTime = formatTime((ban_time - acc_time) / 60);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempNickBan",
                                                    "<%s> *** %s %s %s %s: %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    sNick,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN_TMPBND_BY)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                    sBanTime.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s: %s %s: %s.|",
                                                 SettingManager::HubSec(),
                                                 sNick,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BEEN_TEMP_BANNED_TO)].c_str(),
                                                 sBanTime.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                 !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);
    }

    UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick %s tempbanned by %s", sNick, pChatCommand->m_pUser->m_sNick.c_str());

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeBan(ChatCommand* pChatCommand, const bool bFull)
{
    std::array<char*, 3> sCmdParts = {};
    std::array<uint16_t, 3> ui16CmdPartsLen = {};

    (void)ParseCmdParts(pChatCommand, 0, sCmdParts.data(), ui16CmdPartsLen.data(), 3);

    if (ui16CmdPartsLen[0] > 39 || ui16CmdPartsLen[1] > 39)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_IP_LEN_39_CHARS)].c_str());
        return true;
    }

    std::array<uint8_t, 16> ui128FromHash = {};
    std::array<uint8_t, 16> ui128ToHash = {};

    if (ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0 || !HashIP(sCmdParts[0], ui128FromHash.data()) || !HashIP(sCmdParts[1], ui128ToHash.data()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (memcmp(ui128ToHash.data(), ui128FromHash.data(), 16) <= 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_FROM)].c_str(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                 sCmdParts[1],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_VALID_RANGE)].c_str());
        return true;
    }

    if (!BanManager::m_Ptr->RangeBan(
            sCmdParts[0], ui128FromHash.data(), sCmdParts[1], ui128ToHash.data(), sCmdParts[2], pChatCommand->m_pUser->m_sNick.c_str(), bFull))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s-%s %s %s%s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 sCmdParts[0],
                                                 sCmdParts[1],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREADY)].c_str(),
                                                 bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeBan5",
                                                    "<%s> *** %s %s-%s %s %s%s %s %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                    sCmdParts[0],
                                                    sCmdParts[1],
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_LWR)].c_str(),
                                                    bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[2]);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s-%s %s %s%s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 sCmdParts[0],
                                                 sCmdParts[1],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_LWR)].c_str(),
                                                 bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str());
    }

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeTempBan(ChatCommand* pChatCommand, const bool bFull)
{
    std::array<char*, 4> sCmdParts = {nullptr, nullptr, nullptr, nullptr};
    std::array<uint16_t, 4> ui16CmdPartsLen = {0, 0, 0, 0};

    (void)ParseCmdParts(pChatCommand, 0, sCmdParts.data(), ui16CmdPartsLen.data(), 4);

    if (ui16CmdPartsLen[0] > 39 || ui16CmdPartsLen[1] > 39)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_IP_LEN_39_CHARS)].c_str());
        return true;
    }

    std::array<uint8_t, 16> ui128FromHash = {};
    std::array<uint8_t, 16> ui128ToHash = {};

    if (ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0 || ui16CmdPartsLen[2] == 0 || !sCmdParts[2] ||
        !HashIP(sCmdParts[0], ui128FromHash.data()) || !HashIP(sCmdParts[1], ui128ToHash.data()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (memcmp(ui128ToHash.data(), ui128FromHash.data(), 16) <= 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_FROM)].c_str(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                 sCmdParts[1],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_VALID_RANGE)].c_str());
        return true;
    }

    const uint8_t ui8Time = sCmdParts[2][ui16CmdPartsLen[2] - 1];
    sCmdParts[2][ui16CmdPartsLen[2] - 1] = '\0';
    int iTime = 0;
    if (!safe_stoi(sCmdParts[2], iTime))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }
    time_t acc_time, ban_time;

    if (iTime <= 0 || !GenerateTempBanTime(ui8Time, static_cast<uint32_t>(iTime), acc_time, ban_time))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 bFull ? "full" : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }

    if (!BanManager::m_Ptr->RangeTempBan(
            sCmdParts[0], ui128FromHash.data(), sCmdParts[1], ui128ToHash.data(), sCmdParts[3], pChatCommand->m_pUser->m_sNick.c_str(), 0, ban_time, bFull))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s-%s %s %s%s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 sCmdParts[0],
                                                 sCmdParts[1],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREADY)].c_str(),
                                                 bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LONGER_TIME)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    const std::string sTime = formatTime((ban_time - acc_time) / 60);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeTempBan",
                                                    "<%s> *** %s %s-%s %s %s%s %s %s %s: %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                    sCmdParts[0],
                                                    sCmdParts[1],
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_LWR)].c_str(),
                                                    bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY_LWR)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                    sTime.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    !sCmdParts[3] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[3]);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s-%s %s %s%s %s: %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 sCmdParts[0],
                                                 sCmdParts[1],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_LWR)].c_str(),
                                                 bFull ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_LWR)].c_str() : "",
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANNED)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                 sTime.c_str());
    }

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeUnban(ChatCommand* pChatCommand, const uint8_t ui8Type)
{
    char* sToIp = strchr(pChatCommand->m_sCommand, ' ');
    if (sToIp)
    {
        sToIp[0] = '\0';
        sToIp++;
    }

    std::array<uint8_t, 16> ui128FromHash = {};
    std::array<uint8_t, 16> ui128ToHash = {};

    if (!sToIp || pChatCommand->m_sCommand[0] == '\0' || sToIp[0] == '\0' || !HashIP(pChatCommand->m_sCommand, ui128FromHash.data()) ||
        !HashIP(sToIp, ui128ToHash.data()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (memcmp(ui128ToHash.data(), ui128FromHash.data(), 16) <= 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_FROM)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                 sToIp,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_VALID_RANGE)].c_str());
        return true;
    }

    if (!BanManager::m_Ptr->RangeUnban(ui128FromHash.data(), ui128ToHash.data(), ui8Type))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s-%s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 sToIp,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_IN_MY_RANGE)].c_str(),
                                                 ui8Type == 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANS_LWR)].c_str()
                                                              : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BANS_LWR)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeUnban",
                                                    "<%s> *** %s %s-%s %s %s by %s.|",
                                                    SettingManager::HubSec(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                    pChatCommand->m_sCommand,
                                                    sToIp,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_REMOVED_FROM_RANGE)].c_str(),
                                                    ui8Type == 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANS_LWR)].c_str()
                                                                 : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BANS_LWR)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s-%s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 sToIp,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_REMOVED_FROM_RANGE)].c_str(),
                                                 ui8Type == 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TEMP_BANS_LWR)].c_str()
                                                              : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PERM_BANS_LWR)].c_str());
    }

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeUnban(ChatCommand* pChatCommand)
{
    char* sToIp = strchr(pChatCommand->m_sCommand, ' ');
    if (sToIp)
    {
        sToIp[0] = '\0';
        sToIp++;
    }

    std::array<uint8_t, 16> ui128FromHash = {};
    std::array<uint8_t, 16> ui128ToHash = {};

    if (!sToIp || pChatCommand->m_sCommand[0] == '\0' || sToIp[0] == '\0' || !HashIP(pChatCommand->m_sCommand, ui128FromHash.data()) ||
        !HashIP(sToIp, ui128ToHash.data()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban21",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (memcmp(ui128ToHash.data(), ui128FromHash.data(), 16) <= 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban22",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR_FROM)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                 sToIp,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_VALID_RANGE)].c_str());
        return true;
    }

    if (!BanManager::m_Ptr->RangeUnban(ui128FromHash.data(), ui128ToHash.data()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban23",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s-%s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 sToIp,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_IN_MY_RANGE_BANS)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeUnban2",
                                                    "<%s> *** %s %s-%s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                    pChatCommand->m_sCommand,
                                                    sToIp,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_REMOVED_FROM_RANGE_BANS_BY)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban24",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s-%s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 sToIp,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_REMOVED_FROM_RANGE_BANS)].c_str());
    }

    return true;
}
//---------------------------------------------------------------------------

void HubCommands::SendNoPermission(ChatCommand* pChatCommand)
{
    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::SendNoPermission",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> %s!|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD)].c_str());
}
//---------------------------------------------------------------------------

int HubCommands::CheckFromPm(ChatCommand* pChatCommand)
{
    if (!pChatCommand->m_bFromPM)
    {
        return 0;
    }

    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                  ServerManager::m_szGlobalBufferSize,
                                  "$To: %s From: %s $",
                                  pChatCommand->m_pUser->m_sNick.c_str(),
                                  SettingManager::HubSec());
    if (iMsgLen <= 0)
    {
        return 0;
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

int HubCommands::PrepareReply(ChatCommand* pChatCommand)
{
    int iMsgLen = CheckFromPm(pChatCommand);
    if (!AppendHubSecPrefix(iMsgLen))
    {
        return -1;
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

int HubCommands::BeginBanList(ChatCommand* pChatCommand, const uint8_t ui8Profile)
{
    if (!CheckPermission(pChatCommand, ui8Profile))
    {
        return -1;
    }

    UncountDeflood(pChatCommand);

    return PrepareReply(pChatCommand);
}
//---------------------------------------------------------------------------

void HubCommands::UncountDeflood(ChatCommand* pChatCommand)
{
    if (!pChatCommand->m_bFromPM)
    {
        if (pChatCommand->m_pUser->m_ui16ChatMsgs != 0)
        {
            pChatCommand->m_pUser->m_ui16ChatMsgs--;
            pChatCommand->m_pUser->m_ui16ChatMsgs2--;
        }
    }
    else
    {
        if (pChatCommand->m_pUser->m_ui16PMs != 0)
        {
            pChatCommand->m_pUser->m_ui16PMs--;
            pChatCommand->m_pUser->m_ui16PMs2--;
        }
    }
}
//---------------------------------------------------------------------------

bool HubCommands::CheckPermission(ChatCommand* pChatCommand, const uint8_t ui8Profile)
{
    if (!ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ui8Profile))
    {
        SendNoPermission(pChatCommand);
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::CheckMinLength(ChatCommand* pChatCommand, const uint32_t ui32MinLen, const char* sFuncName, const char* sSyntax)
{
    if (pChatCommand->m_ui32CommandLen < ui32MinLen)
    {
        pChatCommand->m_pUser->SendFormatCheckPM(sFuncName,
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 sSyntax,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

void HubCommands::StripPrefix(ChatCommand* pChatCommand, const uint32_t ui32Len)
{
    pChatCommand->m_sCommand += ui32Len;
    pChatCommand->m_ui32CommandLen -= ui32Len;
}
//---------------------------------------------------------------------------

bool HubCommands::FullBanDelegate(ChatCommand* pChatCommand,
                                  const uint8_t ui8Profile,
                                  const uint32_t ui32MinLen,
                                  const uint32_t ui32PrefixLen,
                                  const char* sFuncName,
                                  const char* sSyntax,
                                  bool (*pDelegate)(ChatCommand*, const bool))
{
    if (!CheckPermission(pChatCommand, ui8Profile))
    {
        return true;
    }

    if (!CheckMinLength(pChatCommand, ui32MinLen, sFuncName, sSyntax))
    {
        return true;
    }

    StripPrefix(pChatCommand, ui32PrefixLen);

    return pDelegate(pChatCommand, true);
}
//---------------------------------------------------------------------------

bool HubCommands::ShouldReplyPM(ChatCommand* pChatCommand)
{
    return !SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)] ||
           !((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR);
}
//---------------------------------------------------------------------------

bool HubCommands::CheckSelfPermission(ChatCommand* pChatCommand, const int iLangId)
{
    if (strcasecmp(pChatCommand->m_sCommand, pChatCommand->m_pUser->m_sNick.c_str()) == 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::SelfPermission",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[iLangId].c_str());
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::CheckHigherProfile(ChatCommand* pChatCommand, const User* pOtherUser, const int iLangId1, const int iLangId2)
{
    if (pOtherUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pOtherUser->m_i32Profile)
    {
        if (iLangId2 == -1)
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::HigherProfile",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> %s %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[iLangId1].c_str(),
                                                     pOtherUser->m_sNick.c_str());
        }
        else
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::HigherProfile",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> %s %s %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[iLangId1].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[iLangId2].c_str(),
                                                     pOtherUser->m_sNick.c_str());
        }
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

const char* HubCommands::GetHubSecPM(const ChatCommand* p)
{
    return p->m_bFromPM ? SettingManager::HubSec() : nullptr;
}
//---------------------------------------------------------------------------

void HubCommands::FormatBanEntry(std::string& out, uint32_t& num, const BanItem* pBan, const bool bShowExpire)
{
    num++;
    out += "[ " + std::to_string(num) + " ]";

    if (pBan->m_sIp[0] != '\0')
    {
        if (((pBan->m_ui8Bits & BanManager::IP) == BanManager::IP))
        {
            out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED)];
        }
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)] + ": " + std::string(pBan->m_sIp.data());
        if (((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
        {
            out += " (" + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL)] + ")";
        }
    }

    if (!pBan->m_sNick.empty())
    {
        if (((pBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK))
        {
            out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED)];
        }
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)] + ": " + pBan->m_sNick;
    }

    if (!pBan->m_sBy.empty())
    {
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY)] + ": " + pBan->m_sBy;
    }

    if (!pBan->m_sReason.empty())
    {
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON)] + ": " + pBan->m_sReason;
    }

    if (bShowExpire)
    {
        const struct tm* tm = localtime(&pBan->m_tTempBanExpire);
        strftime(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%c\n", tm);
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXPIRE)] + ": " + std::string(ServerManager::m_pGlobalBuffer);
    }
    else
    {
        out += "\n";
    }
}
//---------------------------------------------------------------------------

void HubCommands::FormatRangeBanEntry(std::string& out, uint32_t& num, const RangeBanItem* pBan, const bool bShowExpire)
{
    num++;
    out += "[ " + std::to_string(num) + " ]";
    out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)] + ": " + px_str(pBan->m_sIpFrom.data()) + "-" + px_str(pBan->m_sIpTo.data());

    if (((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
    {
        out += " (" + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL)] + ")";
    }

    if (!pBan->m_sBy.empty())
    {
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BY)] + ": " + pBan->m_sBy;
    }

    if (!pBan->m_sReason.empty())
    {
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON)] + ": " + pBan->m_sReason;
    }

    if (bShowExpire)
    {
        const struct tm* tm = localtime(&pBan->m_tTempBanExpire);
        strftime(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%c\n", tm);
        out += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXPIRE)] + ": " + std::string(ServerManager::m_pGlobalBuffer);
    }
    else
    {
        out += "\n";
    }
}
//---------------------------------------------------------------------------

User* HubCommands::FindUserOrReply(ChatCommand* pChatCommand, const uint32_t ui32NickLen, const char* sFunc, const int iLangId)
{
    User* pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(pChatCommand->m_sCommand, ui32NickLen));
    if (!pOtherUser)
    {
        pChatCommand->m_pUser->SendFormatCheckPM(sFunc,
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s: %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[iLangId].c_str());
        return nullptr;
    }
    return pOtherUser;
}
//---------------------------------------------------------------------------

void HubCommands::TruncateReason(char* s, const uint32_t ui32MaxLen)
{
    const auto ui32Len = static_cast<uint32_t>(strlen(s));
    if (ui32Len > ui32MaxLen)
    {
        s[ui32MaxLen - 3] = '.';
        s[ui32MaxLen - 2] = '.';
        s[ui32MaxLen - 1] = '.';
        s[ui32MaxLen] = '\0';
    }
}
//---------------------------------------------------------------------------

uint8_t
HubCommands::ParseCmdParts(ChatCommand* pChatCommand, const uint32_t ui32StartOffset, char* sParts[], uint16_t ui16PartLens[], const uint8_t ui8MaxParts) // NOLINT(modernize-avoid-c-arrays) variable-size call-site arrays
{
    uint8_t ui8Part = 0;
    sParts[0] = pChatCommand->m_sCommand + ui32StartOffset;

    for (uint32_t ui32i = ui32StartOffset; ui32i < pChatCommand->m_ui32CommandLen; ui32i++)
    {
        if (pChatCommand->m_sCommand[ui32i] == ' ')
        {
            pChatCommand->m_sCommand[ui32i] = '\0';
            ui16PartLens[ui8Part] = static_cast<uint16_t>((pChatCommand->m_sCommand + ui32i) - sParts[ui8Part]);

            ui8Part++;
            if (ui8Part >= ui8MaxParts - 1)
            {
                sParts[ui8Part] = pChatCommand->m_sCommand + ui32i + 1;
                ui16PartLens[ui8Part] = static_cast<uint16_t>(pChatCommand->m_ui32CommandLen - ui32i - 1);
                break;
            }
            sParts[ui8Part] = pChatCommand->m_sCommand + ui32i + 1;
        }
    }

    // Set length for last part if it wasn't terminated by a space
    if (ui16PartLens[ui8Part] == 0)
    {
        ui16PartLens[ui8Part] = static_cast<uint16_t>(pChatCommand->m_ui32CommandLen - (sParts[ui8Part] - pChatCommand->m_sCommand));
    }

    // Post-processing: auto-calc single-part length, empty→nullptr, truncate reason
    const uint8_t ui8LastPart = ui8Part;

    // If only one part found and second part exists but has no content, calc its length from remaining
    if (ui8LastPart >= 1 && ui16PartLens[ui8LastPart] == 0 && ui16PartLens[ui8LastPart - 1] == 0 && sParts[ui8LastPart])
    {
        ui16PartLens[ui8LastPart - 1] = static_cast<uint16_t>(pChatCommand->m_ui32CommandLen - (sParts[ui8LastPart - 1] - pChatCommand->m_sCommand));
    }

    // Empty last part → nullptr
    if (sParts[ui8LastPart] && ui16PartLens[ui8LastPart] == 0)
    {
        sParts[ui8LastPart] = nullptr;
    }

    // Truncate long reason
    if (sParts[ui8LastPart] && ui16PartLens[ui8LastPart] > 511)
    {
        TruncateReason(sParts[ui8LastPart]);
    }

    return ui8Part + 1;
}
//---------------------------------------------------------------------------

void HubCommands::CloseIpBannedUsers(ChatCommand* pChatCommand, const char* sReason)
{
    std::array<uint8_t, 16> ui128Hash = {};

    (void)HashIP(pChatCommand->m_sCommand, ui128Hash.data());

    User *pCurUser = nullptr, *pNextUser = HashManager::m_Ptr->FindUser(ui128Hash.data());

    while (pNextUser)
    {
        pCurUser = pNextUser;
        pNextUser = pCurUser->m_pHashIpTableNext;

        if (pCurUser == pChatCommand->m_pUser || (pCurUser->m_i32Profile != -1 && ProfileManager::m_Ptr->IsAllowed(pCurUser, ProfileManager::ENTERIFIPBAN)))
        {
            continue;
        }

        // PPK don't nickban user with higher profile
        if (pCurUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pCurUser->m_i32Profile)
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CloseIpBannedUsers1",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> %s %s %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO)].c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAN_LWR)].c_str(),
                                                     pCurUser->m_sNick.c_str());
            continue;
        }

        pCurUser->SendFormat("HubCommands::CloseIpBannedUsers2",
                             false,
                             "<%s> %s: %s.|",
                             SettingManager::HubSec(),
                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_HAD_BEEN_BANNED_BCS)].c_str(),
                             !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);

        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] User %s (%s) ipbanned by %s", pCurUser->m_sNick.c_str(), pCurUser->m_sIP.data(), pChatCommand->m_pUser->m_sNick.c_str());

        pCurUser->Close();
    }
}
//---------------------------------------------------------------------------

void HubCommands::AppendMatchingRangeBans(std::string& Bans, uint32_t& iBanNum, const uint8_t* ui128IpHash, const time_t acc_time)
{
    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    auto itRangeBan = rangeList.begin();
    while (itRangeBan != rangeList.end())
    {
        RangeBanItem* pCurRangeBan = itRangeBan->get();

        if (((pCurRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
        {
            if (acc_time >= pCurRangeBan->m_tTempBanExpire)
            {
                itRangeBan = rangeList.erase(itRangeBan);
                continue;
            }
        }

        if (memcmp(pCurRangeBan->m_ui128FromIpHash, ui128IpHash, 16) <= 0 && memcmp(pCurRangeBan->m_ui128ToIpHash, ui128IpHash, 16) >= 0)
        {
            iBanNum++;
            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                   ServerManager::m_szGlobalBufferSize,
                                   "\n[%u] %s: %s-%s",
                                   iBanNum,
                                   LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                                   pCurRangeBan->m_sIpFrom.data(),
                                   pCurRangeBan->m_sIpTo.data());
            if (iMsgLen <= 0)
            {
                return;
            }

            if (((pCurRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
            {
                if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                    iMsgLen,
                                    ServerManager::m_szGlobalBufferSize,
                                    " (%s)",
                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL)].c_str()))
                {
                    return;
                }
            }

            Bans += std::string(ServerManager::m_pGlobalBuffer, iMsgLen);

            if (!pCurRangeBan->m_sReason.empty())
            {
                iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                   ServerManager::m_szGlobalBufferSize,
                                   " %s: %s",
                                   LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON)].c_str(),
                                   pCurRangeBan->m_sReason.c_str());
                if (iMsgLen > 0)
                {
                    Bans += ServerManager::m_pGlobalBuffer;
                }
            }

            if (!pCurRangeBan->m_sBy.empty())
            {
                iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                   ServerManager::m_szGlobalBufferSize,
                                   " %s: %s",
                                   LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_BY)].c_str(),
                                   pCurRangeBan->m_sBy.c_str());
                if (iMsgLen > 0)
                {
                    Bans += ServerManager::m_pGlobalBuffer;
                }
            }

            if (((pCurRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
            {
                const struct tm* tm = localtime(&pCurRangeBan->m_tTempBanExpire);
                if (strftime(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%c", tm) > 0)
                {
                    Bans += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXPIRE)] + ": " + std::string(ServerManager::m_pGlobalBuffer);
                }
            }
        }

        ++itRangeBan;
    }
}
//---------------------------------------------------------------------------
