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

bool HubCommands::AddRegUser(ChatCommand* pChatCommand)
{
    // !addreguser nick pass profile_name

    if (!CheckPermission(pChatCommand, ProfileManager::ADDREGUSER))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 255)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CMD_TOO_LONG)].c_str());
        return true;
    }

    std::array<char*, 3> sCmdParts = {nullptr, nullptr, nullptr};
    std::array<uint16_t, 3> iCmdPartsLen = {0, 0, 0};

    uint8_t cPart = 0;

    sCmdParts[cPart] = pChatCommand->m_sCommand + 11; // nick start

    for (uint32_t ui32i = 11; ui32i < pChatCommand->m_ui32CommandLen; ++ui32i)
    {
        if (pChatCommand->m_sCommand[ui32i] == ' ')
        {
            pChatCommand->m_sCommand[ui32i] = '\0';
            iCmdPartsLen[cPart] = static_cast<uint16_t>((pChatCommand->m_sCommand + ui32i) - sCmdParts[cPart]);

            // are we on last space ???
            if (cPart == 1)
            {
                sCmdParts[2] = pChatCommand->m_sCommand + ui32i + 1;
                iCmdPartsLen[2] = static_cast<uint16_t>(pChatCommand->m_ui32CommandLen - ui32i - 1);
                for (uint32_t ui32j = pChatCommand->m_ui32CommandLen; ui32j > ui32i; --ui32j)
                {
                    if (pChatCommand->m_sCommand[ui32j] == ' ')
                    {
                        pChatCommand->m_sCommand[ui32j] = '\0';

                        sCmdParts[2] = pChatCommand->m_sCommand + ui32j + 1;
                        iCmdPartsLen[2] = static_cast<uint16_t>(pChatCommand->m_ui32CommandLen - ui32j - 1);

                        pChatCommand->m_sCommand[ui32i] = ' ';
                        iCmdPartsLen[1] = static_cast<uint16_t>((pChatCommand->m_sCommand + ui32j) - sCmdParts[1]);

                        break;
                    }
                }
                break;
            }

            cPart++;
            sCmdParts[cPart] = pChatCommand->m_sCommand + ui32i + 1;
        }
    }

    if (iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0 || iCmdPartsLen[2] == 0)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %caddreguser <%s> <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PASSWORD_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILENAME_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    if (iCmdPartsLen[0] > 65)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    if (strpbrk(sCmdParts[0], " $|"))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_BAD_CHARS_IN_NICK)].c_str());
        return true;
    }

    if (iCmdPartsLen[1] > 65)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_PASS_LEN_64_CHARS)].c_str());
        return true;
    }

    if (strchr(sCmdParts[1], '|'))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PIPE_IN_PASS)].c_str());
        return true;
    }

    const auto optProfileIdx = ProfileManager::m_Ptr->GetProfileIndex(sCmdParts[2]);
    if (!optProfileIdx)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser7",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST)].c_str());
        return true;
    }

    // check hierarchy
    // deny if pUser is not Master and tries add equal or higher profile
    if (pChatCommand->m_pUser->m_i32Profile > 0 && *optProfileIdx <= pChatCommand->m_pUser->m_i32Profile)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser8",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE)].c_str());
        return true;
    }

    // try to add the user
    if (!RegManager::m_Ptr->AddNew(sCmdParts[0], sCmdParts[1], *optProfileIdx))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser9",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USER)].c_str(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREDY_REGISTERED)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::AddRegUser10",
                                                    "<%s> *** %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SUCCESSFULLY_ADDED)].c_str(),
                                                    sCmdParts[0],
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_REGISTERED_USERS)].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser11",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 sCmdParts[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SUCCESSFULLY_ADDED_TO_REGISTERED_USERS)].c_str());
    }

    User* const pOtherUser = HashManager::m_Ptr->FindUser(std::string_view(sCmdParts[0], iCmdPartsLen[0]));
    if (pOtherUser)
    {
        const bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::ALLOWEDOPCHAT);
        pOtherUser->m_i32Profile = *optProfileIdx;
        if (!((pOtherUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            if (ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::HASKEYICON))
            {
                pOtherUser->m_ui32BoolBits |= User::BIT_OPERATOR;
            }
            else
            {
                pOtherUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
            }

            if (((pOtherUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
            {
                // alex82 ... HideUserKey / ������ ���� �����
                if (!((pOtherUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
                {
                    Users::m_Ptr->Add2OpList(pOtherUser);
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
                        pOtherUser->SendFormat(
                            "HubCommands::AddRegUser12", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
                    }
                }
            }
        }
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::BanIp(ChatCommand* pChatCommand) // !banip ip reason
{
    if (!CheckPermission(pChatCommand, ProfileManager::BAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 12)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %cbanip <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 6;
    pChatCommand->m_ui32CommandLen -= 6;

    return BanIp(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Ban(ChatCommand* pChatCommand) // !ban nick reason
{
    if (!CheckPermission(pChatCommand, ProfileManager::BAN))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 5)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 4;
    pChatCommand->m_ui32CommandLen -= 4;

    return Ban(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrBans(ChatCommand* pChatCommand, const uint8_t ui8Profile, BanClearType eClearType, const int iStatusLang, const int iReplyLang)
{
    if (!CheckPermission(pChatCommand, ui8Profile))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    switch (eClearType)
    {
    case BanClearType::Temp:
        BanManager::m_Ptr->ClearTemp();
        break;
    case BanClearType::Perm:
        BanManager::m_Ptr->ClearPerm();
        break;
    case BanClearType::TempRange:
        BanManager::m_Ptr->ClearTempRange();
        break;
    case BanClearType::PermRange:
        BanManager::m_Ptr->ClearPermRange();
        break;
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::ClrBans",
                                                    "<%s> *** %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[iStatusLang].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ClrBans",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[iReplyLang].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrTempBans(ChatCommand* pChatCommand) // !clrtempbans
{
    return ClrBans(pChatCommand, ProfileManager::CLRTEMPBAN, BanClearType::Temp, std::to_underlying(LangIds::LAN_HAS_CLEARED_TEMPBANS), std::to_underlying(LangIds::LAN_TEMP_BANS_CLEARED));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrPermBans(ChatCommand* pChatCommand) // !clrpermbans
{
    return ClrBans(pChatCommand, ProfileManager::CLRPERMBAN, BanClearType::Perm, std::to_underlying(LangIds::LAN_HAS_CLEARED_PERMBANS), std::to_underlying(LangIds::LAN_PERM_BANS_CLEARED));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrRangeTempBans(ChatCommand* pChatCommand) // !clrrangetempbans
{
    return ClrBans(pChatCommand, ProfileManager::CLR_RANGE_TBANS, BanClearType::TempRange, std::to_underlying(LangIds::LAN_HAS_CLEARED_TEMP_RANGEBANS), std::to_underlying(LangIds::LAN_TEMP_RANGE_BANS_CLEARED));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrRangePermBans(ChatCommand* pChatCommand) // !clrrangepermbans
{
    return ClrBans(pChatCommand, ProfileManager::CLR_RANGE_BANS, BanClearType::PermRange, std::to_underlying(LangIds::LAN_HAS_CLEARED_PERM_RANGEBANS), std::to_underlying(LangIds::LAN_PERM_RANGE_BANS_CLEARED));
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::CheckNickBan(ChatCommand* pChatCommand) // !checknickban nick
{
    if (!CheckPermission(pChatCommand, ProfileManager::GETBANLIST))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    if (pChatCommand->m_ui32CommandLen < 14 || pChatCommand->m_sCommand[13] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckNickBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> ***%s %cchecknickban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 13;

    BanItem* pBan = BanManager::m_Ptr->FindNick(std::string_view(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen - 13));
    if (!pBan)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckNickBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_BAN_FOUND)].c_str());
        return true;
    }

    int iMsgLen = CheckFromPm(pChatCommand);

    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                        iMsgLen,
                        ServerManager::m_szGlobalBufferSize,
                        "<%s> %s: %s",
                        SettingManager::HubSec(),
                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_NICK)].c_str(),
                        pBan->m_sNick.c_str()))
    {
        return true;
    }

    if (pBan->m_sIp[0] != '\0')
    {
        if (((pBan->m_ui8Bits & BanManager::IP) == BanManager::IP))
        {
            if (!SnprintfAppend(
                    ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, " %s", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED)].c_str()))
            {
                return true;
            }
        }
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            " %s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                            pBan->m_sIp.data()))
        {
            return true;
        }

        if (((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
        {
            if (!SnprintfAppend(
                    ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, " (%s)", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL)].c_str()))
            {
                return true;
            }
        }
    }

    if (!pBan->m_sReason.empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            " %s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON)].c_str(),
                            pBan->m_sReason.c_str()))
        {
            return true;
        }
    }

    if (!pBan->m_sBy.empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            " %s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_BY)].c_str(),
                            pBan->m_sBy.c_str()))
        {
            return true;
        }
    }

    if (((pBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
    {
        if (!SnprintfAppend(
                ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, " %s: ", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXPIRE)].c_str()))
        {
            return true;
        }

        const struct tm* tm = localtime(&pBan->m_tTempBanExpire);
        const size_t szRet = strftime(ServerManager::m_pGlobalBuffer + iMsgLen, ServerManager::m_szGlobalBufferSize - iMsgLen, "%c.|", tm);
        if (szRet > 0)
        {
            iMsgLen += static_cast<int>(szRet);
        }
        else
        {
            ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
            ServerManager::m_pGlobalBuffer[iMsgLen + 1] = '|';
            ServerManager::m_pGlobalBuffer[iMsgLen + 2] = '\0';
            iMsgLen += 2;
        }
    }
    else
    {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
        ServerManager::m_pGlobalBuffer[iMsgLen + 1] = '|';
        ServerManager::m_pGlobalBuffer[iMsgLen + 2] = '\0';
        iMsgLen += 2;
    }

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::CheckIpBan(ChatCommand* pChatCommand) // !checkipban ip
{
    if (!CheckPermission(pChatCommand, ProfileManager::GETBANLIST))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    if (pChatCommand->m_ui32CommandLen < 15 || pChatCommand->m_sCommand[11] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckIpBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ccheckipban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 11;

    std::array<uint8_t, 16> ui128Hash = {};

    // check ip
    if (!HashIP(pChatCommand->m_sCommand, ui128Hash.data()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckIpBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ccheckipban <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_VALID_IP_SPECIFIED)].c_str());
        return true;
    }

    time_t acc_time;
    time(&acc_time);

    BanItem* pNextBan = BanManager::m_Ptr->FindIP(ui128Hash.data(), acc_time);

    if (pNextBan)
    {
        int iMsgLen = PrepareReply(pChatCommand);
        if (iMsgLen == -1)
        {
            return true;
        }

        std::string Bans(ServerManager::m_pGlobalBuffer, iMsgLen);

        uint32_t iBanNum = 0;
        BanItem* pCurBan = nullptr;

        while (pNextBan)
        {
            pCurBan = pNextBan;
            pNextBan = pCurBan->m_pHashIpTableNext;

            if (((pCurBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
            {
                // PPK ... check if temban expired
                if (acc_time >= pCurBan->m_tTempBanExpire)
                {
                    BanManager::m_Ptr->Rem(pCurBan);
                    std::unique_ptr<BanItem> guard(pCurBan);

                    continue;
                }
            }

            iBanNum++;
            iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                               ServerManager::m_szGlobalBufferSize,
                               "\n[%u] %s: %s",
                               iBanNum,
                               LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_IP)].c_str(),
                               pCurBan->m_sIp.data());
            if (iMsgLen <= 0)
            {
                return true;
            }

            if (((pCurBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
            {
                if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                    iMsgLen,
                                    ServerManager::m_szGlobalBufferSize,
                                    " (%s)",
                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL)].c_str()))
                {
                    return true;
                }
            }

            Bans += std::string(ServerManager::m_pGlobalBuffer, iMsgLen);

            if (!pCurBan->m_sNick.empty())
            {
                iMsgLen = 0;
                if (((pCurBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK))
                {
                    iMsgLen = snprintf(
                        ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED)].c_str());
                    if (iMsgLen <= 0)
                    {
                        return true;
                    }
                }

                if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                    iMsgLen,
                                    ServerManager::m_szGlobalBufferSize,
                                    " %s: %s",
                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                                    pCurBan->m_sNick.c_str()))
                {
                    return true;
                }
                Bans += ServerManager::m_pGlobalBuffer;
            }

            if (!pCurBan->m_sReason.empty())
            {
                iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                   ServerManager::m_szGlobalBufferSize,
                                   " %s: %s",
                                   LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON)].c_str(),
                                   pCurBan->m_sReason.c_str());
                if (iMsgLen <= 0)
                {
                    return true;
                }
                Bans += ServerManager::m_pGlobalBuffer;
            }

            if (!pCurBan->m_sBy.empty())
            {
                iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                   ServerManager::m_szGlobalBufferSize,
                                   " %s: %s",
                                   LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_BY)].c_str(),
                                   pCurBan->m_sBy.c_str());
                if (iMsgLen <= 0)
                {
                    return true;
                }
                Bans += ServerManager::m_pGlobalBuffer;
            }

            if (((pCurBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
            {
                const struct tm* tm = localtime(&pCurBan->m_tTempBanExpire);
                if (strftime(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%c", tm) > 0)
                {
                    Bans += " " + LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXPIRE)] + ": " + std::string(ServerManager::m_pGlobalBuffer);
                }
            }
        }

        if (!ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS))
        {
            Bans += "|";

            pChatCommand->m_pUser->SendCharDelayed(Bans);
            return true;
        }

        AppendMatchingRangeBans(Bans, iBanNum, ui128Hash.data(), acc_time);

        Bans += "|";

        pChatCommand->m_pUser->SendCharDelayed(Bans);
        return true;
    }
    if (ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS))
    {
        const int iMsgLen = PrepareReply(pChatCommand);
        if (iMsgLen == -1)
        {
            return true;
        }

        std::string Bans(ServerManager::m_pGlobalBuffer, iMsgLen);

        uint32_t iBanNum = 0;

        AppendMatchingRangeBans(Bans, iBanNum, ui128Hash.data(), acc_time);

        Bans += "|";

        pChatCommand->m_pUser->SendCharDelayed(Bans);

        return true;
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckIpBan3",
                                             GetHubSecPM(pChatCommand),
                                             true,
                                             "<%s> *** %s.|",
                                             SettingManager::HubSec(),
                                             LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_BAN_FOUND)].c_str());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::CheckRangeBan(ChatCommand* pChatCommand) // !checkrangeban ipfrom ipto
{
    if (!CheckPermission(pChatCommand, ProfileManager::GET_RANGE_BANS))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    if (pChatCommand->m_ui32CommandLen < 26)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 14;

    char* sIpTo = strchr(pChatCommand->m_sCommand, ' ');
    if (!sIpTo)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    sIpTo[0] = '\0';
    sIpTo++;

    // check ipfrom
    if (pChatCommand->m_sCommand[0] == '\0' || sIpTo[0] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BAD_PARAMS_GIVEN)].c_str());
        return true;
    }

    std::array<uint8_t, 16> ui128FromHash = {};
    std::array<uint8_t, 16> ui128ToHash = {};

    if (!HashIP(pChatCommand->m_sCommand, ui128FromHash.data()) || !HashIP(sIpTo, ui128ToHash.data()))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROMIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TOIP)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_VALID_IP_RANGE_SPECIFIED)].c_str());
        return true;
    }

    time_t acc_time;
    time(&acc_time);

    RangeBanItem* pRangeBan = BanManager::m_Ptr->FindRange(ui128FromHash.data(), ui128ToHash.data(), acc_time);
    if (!pRangeBan)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan5",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_RANGE_BAN_FOUND)].c_str());
        return true;
    }

    int iMsgLen = CheckFromPm(pChatCommand);

    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                        iMsgLen,
                        ServerManager::m_szGlobalBufferSize,
                        "<%s> %s: %s-%s",
                        SettingManager::HubSec(),
                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                        pRangeBan->m_sIpFrom.data(),
                        pRangeBan->m_sIpTo.data()))
    {
        return true;
    }

    if (((pRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL))
    {
        if (!SnprintfAppend(
                ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, " (%s)", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL)].c_str()))
        {
            return true;
        }
    }

    if (!pRangeBan->m_sReason.empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            " %s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON)].c_str(),
                            pRangeBan->m_sReason.c_str()))
        {
            return true;
        }
    }

    if (!pRangeBan->m_sBy.empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            " %s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_BY)].c_str(),
                            pRangeBan->m_sBy.c_str()))
        {
            return true;
        }
    }

    if (((pRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
    {
        if (!SnprintfAppend(
                ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, " %s: ", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_EXPIRE)].c_str()))
        {
            return true;
        }

        const struct tm* tm = localtime(&pRangeBan->m_tTempBanExpire);
        const size_t szRet = strftime(ServerManager::m_pGlobalBuffer + iMsgLen, ServerManager::m_szGlobalBufferSize - iMsgLen, "%c.|", tm);
        if (szRet > 0)
        {
            iMsgLen += static_cast<int>(szRet);
        }
        else
        {
            ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
            ServerManager::m_pGlobalBuffer[iMsgLen + 1] = '|';
            ServerManager::m_pGlobalBuffer[iMsgLen + 2] = '\0';
            iMsgLen += 2;
        }
    }
    else
    {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
        ServerManager::m_pGlobalBuffer[iMsgLen + 1] = '|';
        ServerManager::m_pGlobalBuffer[iMsgLen + 2] = '\0';
        iMsgLen += 2;
    }

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Drop(ChatCommand* pChatCommand) //! drop nick
{
    if (!CheckPermission(pChatCommand, ProfileManager::DROP))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 6)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cdrop <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 5;
    pChatCommand->m_ui32CommandLen -= 5;

    uint32_t ui32NickLen = 0;

    char* sReason = strchr(pChatCommand->m_sCommand, ' ');
    if (sReason)
    {
        ui32NickLen = static_cast<uint32_t>(sReason - pChatCommand->m_sCommand);

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
    else
    {
        ui32NickLen = pChatCommand->m_ui32CommandLen;
    }

    if (ui32NickLen > 100)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cdrop <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MAX_ALWD_NICK_LEN_64_CHARS)].c_str());
        return true;
    }

    if (pChatCommand->m_sCommand[0] == '\0')
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cdrop <%s>. %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    // Self-drop ?
    if (!CheckSelfPermission(pChatCommand, std::to_underlying(LangIds::LAN_YOU_CANT_DROP_YOURSELF)))
    {
        return true;
    }

    User* const pOtherUser = FindUserOrReply(pChatCommand, ui32NickLen, "HubCommands::Drop5", std::to_underlying(LangIds::LAN_IS_NOT_IN_USERLIST));
    if (!pOtherUser)
    {
        return true;
    }

    // PPK don't drop user with higher profile
    if (!CheckHigherProfile(pChatCommand, pOtherUser, std::to_underlying(LangIds::LAN_YOU_NOT_ALLOWED_TO), std::to_underlying(LangIds::LAN_DROP_LWR)))
    {
        return true;
    }

    UncountDeflood(pChatCommand);

    BanManager::m_Ptr->TempBan(pOtherUser, sReason, pChatCommand->m_pUser->m_sNick.c_str(), 0, 0, false);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Drop7",
                                                    "<%s> *** %s %s %s %s %s %s: %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_sCommand,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                    pOtherUser->m_sIP.data(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DROP_ADDED_TEMPBAN_BY)].c_str(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop8",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s %s %s: %s.|",
                                                 SettingManager::HubSec(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                 pOtherUser->m_sIP.data(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DROP_ADDED_TEMPBAN_BCS)].c_str(),
                                                 !sReason ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_REASON_SPECIFIED)].c_str() : sReason);
    }

    pOtherUser->Close();

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::DelRegUser(ChatCommand* pChatCommand) // !delreguser nick
{
    if (!CheckPermission(pChatCommand, ProfileManager::DELREGUSER))
    {
        return true;
    }

    if (pChatCommand->m_ui32CommandLen > 255)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CMD_TOO_LONG)].c_str());
        return true;
    }
    if (pChatCommand->m_ui32CommandLen < 13)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser2",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cdelreguser <%s>. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK_LWR)].c_str(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
        return true;
    }

    pChatCommand->m_sCommand += 11;
    pChatCommand->m_ui32CommandLen -= 11;

    // find him
    RegUser* pReg = RegManager::m_Ptr->Find(std::string_view(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen));
    if (!pReg)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser3",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %s %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ERROR)].c_str(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_NOT_IN_REGS)].c_str());
        return true;
    }

    // check hierarchy
    // deny if pUser is not Master and tries delete equal or higher profile
    if (pChatCommand->m_pUser->m_i32Profile > 0 && pReg->m_ui16Profile <= pChatCommand->m_pUser->m_i32Profile)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOURE_NOT_ALWD_TO_DLT_USER_THIS_PRFL)].c_str());
        return true;
    }

    UncountDeflood(pChatCommand);

    RegManager::m_Ptr->Delete(pReg);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::DelRegUser5",
                                                    "<%s> *** %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    pChatCommand->m_pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVED_LWR)].c_str(),
                                                    pChatCommand->m_sCommand,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FROM_REGS)].c_str());
    }

    if (ShouldReplyPM(pChatCommand))
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser6",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> %s %s.|",
                                                 SettingManager::HubSec(),
                                                 pChatCommand->m_sCommand,
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REMOVED_FROM_REGS)].c_str());
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Debug(ChatCommand* pChatCommand) // !debug port/off
{
    if (!((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
    {
        SendNoPermission(pChatCommand);
        return true;
    }

    if (pChatCommand->m_ui32CommandLen < 7)
    {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug1",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s %cdebug <%s>, %cdebug off. %s!|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PORT_LWR)].c_str(),
                                                 SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NO_PARAM_GIVEN)].c_str());
    }

    pChatCommand->m_sCommand += 6;

    if (strcasecmp(pChatCommand->m_sCommand, "off") == 0)
    {
        if (UdpDebug::m_Ptr->CheckUdpSub(pChatCommand->m_pUser))
        {
            if (UdpDebug::m_Ptr->Remove(pChatCommand->m_pUser))
            {
                UncountDeflood(pChatCommand);

                pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug2",
                                                         GetHubSecPM(pChatCommand),
                                                         true,
                                                         "<%s> *** %s.|",
                                                         SettingManager::HubSec(),
                                                         LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNSUB_FROM_UDP_DBG)].c_str());

                UdpDebug::m_Ptr->BroadcastFormat(
                    "[SYS] Debug subscription cancelled: %s (%s)", pChatCommand->m_pUser->m_sNick.c_str(), pChatCommand->m_pUser->m_sIP.data());

                return true;
            }

            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug3",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s!|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNABLE_FIND_UDP_DBG_INTER)].c_str());
            return true;
        }
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug4",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NOT_UDP_DEBUG_SUBSCRIB)].c_str());
        return true;
    }
    else
    {
        if (UdpDebug::m_Ptr->CheckUdpSub(pChatCommand->m_pUser))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug5",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %cdebug off %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ALRD_UDP_SUBSCRIP_TO_UNSUB)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IN_MAINCHAT)].c_str());
            return true;
        }

        int iDbgPort = 0;
        if (!safe_stoi(pChatCommand->m_sCommand, iDbgPort))
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug6",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %cdebug <%s>, %cdebug off.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PORT_LWR)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0]);
            return true;
        }
        if (iDbgPort <= 0 || iDbgPort > 65535)
        {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug6",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %cdebug <%s>, %cdebug off.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SNTX_ERR_IN_CMD)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PORT_LWR)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0]);
            return true;
        }

        if (UdpDebug::m_Ptr->New(pChatCommand->m_pUser, static_cast<uint16_t>(iDbgPort)))
        {
            UncountDeflood(pChatCommand);

            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug7",
                                                     GetHubSecPM(pChatCommand),
                                                     true,
                                                     "<%s> *** %s %d. %s %cdebug off %s.|",
                                                     SettingManager::HubSec(),
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SUBSCIB_UDP_DEBUG_ON_PORT)].c_str(),
                                                     iDbgPort,
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_TO_UNSUB_TYPE)].c_str(),
                                                     SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES)].c_str()[0],
                                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IN_MAINCHAT)].c_str());

            UdpDebug::m_Ptr->BroadcastFormat(
                "[SYS] New Debug subscriber: %s (%s)", pChatCommand->m_pUser->m_sNick.c_str(), pChatCommand->m_pUser->m_sIP.data());

            return true;
        }

        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug8",
                                                 GetHubSecPM(pChatCommand),
                                                 true,
                                                 "<%s> *** %s.|",
                                                 SettingManager::HubSec(),
                                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UDP_DEBUG_SUBSCRIB_FAILED)].c_str());
        return true;
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
