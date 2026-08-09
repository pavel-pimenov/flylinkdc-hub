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
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "HubCommands.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::MyIp(ChatCommand* pChatCommand) // !myip
{
    if (pChatCommand->m_pUser->m_sIPv4[0] != '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MyIp1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s: %s / %s|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_IP_IS)].c_str(),
                                                 pChatCommand->m_pUser->m_sIP.data(),
                                                 pChatCommand->m_pUser->m_sIPv4.data());
    }
    else
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MyIp2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s: %s|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_IP_IS)].c_str(),
                                                 pChatCommand->m_pUser->m_sIP.data());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::MassMsg(ChatCommand* pChatCommand) // !massmsg text
{
    if (!CheckPermission(pChatCommand, ProfileManager::MASSMSG))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 9)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MassMsg1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cmassmsg <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (pChatCommand->m_ui32CommandLen > 64000)
    {
        pChatCommand->m_sCommand[64000] = '\0';
    }

    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "%s $<%s> %s|",
                           SettingManager::HubSec(),
                           pChatCommand->m_pUser->m_sNick.c_str(),
                           pChatCommand->m_sCommand + 8);
    if (iMsgLen > 0)
    {
        GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, pChatCommand->m_pUser, 0, GlobalDataQueue::SendItem::PM2ALL);
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MassMsg2",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> *** %s.|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MASSMSG_TO_ALL_SENT)].c_str());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::NickBan(ChatCommand* pChatCommand) // !nickban nick reason
{
    if (!CheckPermission(pChatCommand, ProfileManager::BAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 9)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cnickban <%s> <%s>. %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 8;

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
    else
    {
        pChatCommand->m_ui32CommandLen -= 8;
    }

    if (pChatCommand->m_sCommand[0] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %cnickban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_NICK_SPECIFIED)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %cnickban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
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

    User* const pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen));
    if (pOtherUser)
    {
        // PPK don't nickban user with higher profile
        if (!CheckHigherProfile(pChatCommand, pOtherUser, std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO), std::to_underlying(LangIds::LAN_BAN_LWR)))
        {
            return true;
        }

        UncountDeflood(pChatCommand);

        pOtherUser->SendFormat("HubCommands::NickBan6",
                               false,
                               "<%s> %s: %s.|",
                               SettingManager::HubSec(),
                               LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_HAD_BEEN_BANNED_BCS)].c_str(),
                               !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);

        if (BanManager::m_Ptr->NickBan(pOtherUser, nullptr, sReason, pChatCommand->m_pUser->m_sNick.c_str()))
        {
            UdpDebug::m_Ptr->BroadcastFormat(
                "[SYS] User %s (%s) nickbanned by %s", pOtherUser->m_sNick.c_str(), pOtherUser->m_sIP.data(), pChatCommand->m_pUser->m_sNick.c_str());
            pOtherUser->Close();
        }
        else
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan7",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %s %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                                                     pOtherUser->m_sNick.c_str(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREDY_BANNED_DISCONNECT)].c_str());

            pOtherUser->Close();
            return true;
        }
    }
    else
    {
        return NickBan(pChatCommand, sReason);
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::NickBan8",
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
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan9",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ADDED_TO_BANS)].c_str());
    }
    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::NickTempBan(ChatCommand* pChatCommand) // !nicktempban nick time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
{
    if (!CheckPermission(pChatCommand, ProfileManager::TEMP_BAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 15)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    // Now in sCommand we have nick, time and maybe reason
    std::array<char*, 3> sCmdParts = {nullptr, nullptr, nullptr};
    std::array<uint16_t, 3> ui16CmdPartsLen = {0, 0, 0};

    (void)ParseCmdParts(pChatCommand, 12, sCmdParts.data(), ui16CmdPartsLen.data(), 3);

    if (ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0 || !sCmdParts[1])
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (ui16CmdPartsLen[0] > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    // Self-ban ?
    if (strcasecmp(sCmdParts[0], pChatCommand->m_pUser->m_sNick.c_str()) == 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_CANT_BAN_YOURSELF)].c_str());
        return true;
    }

    User* const pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(sCmdParts[0], ui16CmdPartsLen[0]));
    if (pOtherUser)
    {
        // PPK don't tempban user with higher profile
        if (!CheckHigherProfile(pChatCommand, pOtherUser, std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO), std::to_underlying(LangIds::LAN_TEMP_BAN_NICK)))
        {
            return true;
        }
    }
    else
    {
        return TempNickBan(pChatCommand, sCmdParts[0], sCmdParts[1], ui16CmdPartsLen[1], sCmdParts[2]);
    }

    const uint8_t ui8Time = sCmdParts[1][ui16CmdPartsLen[1] - 1];
    sCmdParts[1][ui16CmdPartsLen[1] - 1] = '\0';
    int iTime = 0;
    if (!safe_stoi(sCmdParts[1], iTime))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }
    time_t acc_time, ban_time;

    if (iTime <= 0 || !GenerateTempBanTime(ui8Time, static_cast<uint32_t>(iTime), acc_time, ban_time))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TIME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_TIME_SPECIFIED)].c_str());
        return true;
    }

    if (!BanManager::m_Ptr->NickTempBan(pOtherUser, nullptr, sCmdParts[2], pChatCommand->m_pUser->m_sNick.c_str(), 0, ban_time))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan7",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                                                 pOtherUser->m_sNick.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ALRD_BND_LNGR_TIME_DISCONNECTED)].c_str());
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Already temp banned user %s (%s) disconnected by %s",
                                         pOtherUser->m_sNick.c_str(),
                                         pOtherUser->m_sIP.data(),
                                         pChatCommand->m_pUser->m_sNick.c_str());

        // Disconnect user
        pOtherUser->Close();

        return true;
    }

    UncountDeflood(pChatCommand);

    const std::string sTime = formatTime((ban_time - acc_time) / 60);

    // Send user a message that he has been tempbanned
    pOtherUser->SendFormat("HubCommands::NickTempBan8",
                           false,
                           "<%s> %s: %s %s: %s.|",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_HAD_BEEN_TEMP_BANNED_TO)].c_str(),
                           sTime.c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                           !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[2]);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::NickTempBan9",
                                                    "<%s> *** %s %s %s %s: %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    sCmdParts[0],
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_BEEN_TMPBND_BY)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_LWR)].c_str(),
                                                    sTime.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[2]);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan10",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s: %s %s: %s.|",
                                                 SettingManager::HubSec(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BEEN_TEMP_BANNED_TO)].c_str(),
                                                 sTime.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                 !sCmdParts[2] ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sCmdParts[2]);
    }

    UdpDebug::m_Ptr->BroadcastFormat(
        "[SYS] User %s (%s) tempbanned by %s", pOtherUser->m_sNick.c_str(), pOtherUser->m_sIP.data(), pChatCommand->m_pUser->m_sNick.c_str());

    pOtherUser->Close();

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Op(ChatCommand* pChatCommand) // !op nick
{
    if (!ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMPOP) ||
        ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_TEMP_OPERATOR) == User::BIT_TEMP_OPERATOR))
    {
        SendNoPermission(pChatCommand);
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 4 || pChatCommand->m_sCommand[3] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cop <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 3;

    if (pChatCommand->m_ui32CommandLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cop <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    User* const pOtherUser = FindUserOrReply(pChatCommand, pChatCommand->m_ui32CommandLen - 3, "HubCommands::Op3", std::to_underlying(LangIds::LAN_IS_NOT_IN_USERLIST));
    if (!pOtherUser)
    {
        return true;
    }

    if (((pOtherUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pOtherUser->m_sNick.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ALREDY_IS_OP)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    const auto optProfileIndex = ProfileManager::m_Ptr->GetProfileIndex("Operator");
    if (!optProfileIndex)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s. %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_OPERATOR_PROFILE_MISSING)].c_str());
        return true;
    }

    pOtherUser->m_ui32BoolBits |= User::BIT_OPERATOR;
    const bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::ALLOWEDOPCHAT);
    pOtherUser->m_i32Profile = *optProfileIndex;
    pOtherUser->m_ui32BoolBits |= User::BIT_TEMP_OPERATOR; // to disallow adding more tempop by tempop user ;)
    // alex82 ... HideUserKey / ������ ���� �����
    if (!((pOtherUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
    {
        Users::m_Ptr->Add2OpList(pOtherUser);
    }

    if (!((pOtherUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST))
    {
        pOtherUser->SendFormat("HubCommands::Op6",
                               true,
                               "$LogedIn %s|<%s> *** %s.|",
                               pOtherUser->m_sNick.c_str(),
                               SettingManager::HubSec(),
                               LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_GOT_TEMP_OP)].c_str());
    }
    else
    {
        pOtherUser->SendFormat(
            "HubCommands::Op7", true, "<%s> *** %s.|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_GOT_TEMP_OP)].c_str());
    }
    // alex82 ... ������ ���� �����
    if (!((pOtherUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
    {
        GlobalDataQueue::m_Ptr->OpListStore(pOtherUser->m_sNick.c_str());
    }
    if (bAllowedOpChat != ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::ALLOWEDOPCHAT))
    {
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] &&
            (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] || !SettingManager::m_Ptr->m_bBotsSameNick))
        {
            if (!((pOtherUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO))
            {
                pOtherUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO)]);
            }
            pOtherUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO)]);
            pOtherUser->SendFormat("HubCommands::Op8", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
        }
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Op9",
                                                    "<%s> *** %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SETS_OP_MODE_TO)].c_str(),
                                                    pOtherUser->m_sNick.c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op10",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pOtherUser->m_sNick.c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_GOT_OP_STATUS)].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::OpMassMsg(ChatCommand* pChatCommand) // !opmassmsg text
{
    if (!CheckPermission(pChatCommand, ProfileManager::MASSMSG))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 11)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::OpMassMsg1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %copmassmsg <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "%s $<%s> %s|",
                           SettingManager::HubSec(),
                           pChatCommand->m_pUser->m_sNick.c_str(),
                           pChatCommand->m_sCommand + 10);
    if (iMsgLen > 0)
    {
        GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, pChatCommand->m_pUser, 0, GlobalDataQueue::SendItem::PM2OPS);
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::OpMassMsg2",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> *** %s.|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MASSMSG_TO_OPS_SND)].c_str());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Passwd(ChatCommand* pChatCommand) // !passwd password
{
    RegUser* const pReg = RegManager::m_Ptr->Find(pChatCommand->m_pUser);
    if (pChatCommand->m_pUser->m_i32Profile == -1 || !pReg)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_NOT_ALLOWED_TO_CHANGE_PASS)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 8)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cpasswd <%s>. %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NEW_PASSWORD)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PASS_MUST_SPECIFIED)].c_str());
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 71)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_PASS_LEN_64_CHARS)].c_str());
        return true;
    }

    if (strchr(pChatCommand->m_sCommand + 7, '|'))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PIPE_IN_PASS)].c_str());
        return true;
    }

    if (!pReg->UpdatePassword(std::string_view(pChatCommand->m_sCommand + 7, pChatCommand->m_ui32CommandLen - 7)))
    {
        return true;
    }

    RegManager::m_Ptr->Save(true);

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd5",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> *** %s.|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOUR_PASSWORD_UPDATE_SUCCESS)].c_str());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::PermUnban(ChatCommand* pChatCommand) // !permunban what
{
    if (!CheckPermission(pChatCommand, ProfileManager::UNBAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 11 || pChatCommand->m_sCommand[10] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cpermunban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 10;

    if (pChatCommand->m_ui32CommandLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cpermunban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP_OR_NICK)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    if (!BanManager::m_Ptr->PermUnban(pChatCommand->m_sCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban3",
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
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::PermUnban4",
                                                    "<%s> *** %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVED_LWR)].c_str(),
                                                    pChatCommand->m_sCommand,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROM_BANS)].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban5",
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
