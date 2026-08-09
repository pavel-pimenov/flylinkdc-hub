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
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#include "DeFlood.h"
//---------------------------------------------------------------------------
#include "LanguageManager.h"

// Lookup table for DeFloodGetMessage: maps DefloodTypes × msgId → LangIds
struct DefloodMsgEntry
{
    int m_sWarning; // msgId 0
    int m_sAction;  // msgId 1
    int m_sReport;  // msgId 2
};

static constexpr DefloodMsgEntry g_defloodMessages[] = { // NOLINT(modernize-avoid-c-arrays)
    // GETNICKLIST
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_GetNickList), std::to_underlying(LangIds::LAN_GetNickList_FLOODING), std::to_underlying(LangIds::LAN_GetNickList_FLOODER)},
    // MYINFO
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_MyINFO), std::to_underlying(LangIds::LAN_MyINFO_FLOODING), std::to_underlying(LangIds::LAN_MyINFO_FLOODER)},
    // SEARCH
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_SEARCHES), std::to_underlying(LangIds::LAN_SEARCH_FLOODING), std::to_underlying(LangIds::LAN_SEARCH_FLOODER)},
    // CHAT
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_CHAT), std::to_underlying(LangIds::LAN_CHAT_FLOODING), std::to_underlying(LangIds::LAN_CHAT_FLOODER)},
    // PM
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_PM), std::to_underlying(LangIds::LAN_PM_FLOODING), std::to_underlying(LangIds::LAN_PM_FLOODER)},
    // SAME_SEARCH
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_SAME_SEARCHES), std::to_underlying(LangIds::LAN_SAME_SEARCH_FLOODING), std::to_underlying(LangIds::LAN_SAME_SEARCH_FLOODER)},
    // SAME_PM
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_SAME_PM), std::to_underlying(LangIds::LAN_SAME_PM_FLOODING), std::to_underlying(LangIds::LAN_SAME_PM_FLOODER)},
    // SAME_CHAT
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_SAME_CHAT), std::to_underlying(LangIds::LAN_SAME_CHAT_FLOODING), std::to_underlying(LangIds::LAN_SAME_CHAT_FLOODER)},
    // SAME_MULTI_PM
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_SAME_MULTI_PM), std::to_underlying(LangIds::LAN_SAME_MULTI_PM_FLOODING), std::to_underlying(LangIds::LAN_SAME_MULTI_PM_FLOODER)},
    // SAME_MULTI_CHAT
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_SAME_MULTI_CHAT), std::to_underlying(LangIds::LAN_SAME_MULTI_CHAT_FLOODING), std::to_underlying(LangIds::LAN_SAME_MULTI_CHAT_FLOODER)},
    // CTM
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_CTM), std::to_underlying(LangIds::LAN_CTM_FLOODING), std::to_underlying(LangIds::LAN_CTM_FLOODER)},
    // RCTM
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_RCTM), std::to_underlying(LangIds::LAN_RCTM_FLOODING), std::to_underlying(LangIds::LAN_RCTM_FLOODER)},
    // SR
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_SR), std::to_underlying(LangIds::LAN_SR_FLOODING), std::to_underlying(LangIds::LAN_SR_FLOODER)},
    // MAX_DOWN
    {std::to_underlying(LangIds::LAN_PLS_DONT_FLOOD_WITH_DATA), std::to_underlying(LangIds::LAN_DATA_FLOODING), std::to_underlying(LangIds::LAN_DATA_FLOODER)},
    // INTERVAL_CHAT (only msgId 0 used)
    {std::to_underlying(LangIds::LAN_SECONDS_BEFORE_NEXT_CHAT_MSG), {}, {}},
    // INTERVAL_PM (only msgId 0 used)
    {std::to_underlying(LangIds::LAN_SECONDS_BEFORE_NEXT_PM), {}, {}},
    // INTERVAL_SEARCH (only msgId 0 used)
    {std::to_underlying(LangIds::LAN_SECONDS_BEFORE_NEXT_SEARCH), {}, {}},
};

bool DeFloodCheckForFlood(User* pUser,
                          DefloodTypes eDefloodType,
                          const int16_t ui16Action,
                          uint16_t& ui16Count,
                          uint64_t& ui64LastOkTick,
                          const int16_t ui16DefloodCount,
                          const uint32_t ui32DefloodTime,
                          const char* sOtherNick /* = nullptr*/)
{
    if (ui16Count == 0)
    {
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
    }
    else if (ui16Count == ui16DefloodCount)
    {
        if ((ui64LastOkTick + ui32DefloodTime) > ServerManager::m_ui64ActualTick) [[unlikely]]
        {
            DeFloodDoAction(pUser, eDefloodType, ui16Action, ui16Count, sOtherNick);
            return true;
        }
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui16Count = 0;
    }
    else if (ui16Count > ui16DefloodCount)
    {
        if ((ui64LastOkTick + ui32DefloodTime) > ServerManager::m_ui64ActualTick) [[unlikely]]
        {
            if (ui16Action == 2 && ui16Count == (ui16DefloodCount * 2))
            {
                pUser->m_ui32DefloodWarnings++;

                if (DeFloodCheckForWarn(pUser, eDefloodType, sOtherNick))
                {
                    return true;
                }
                ui16Count -= ui16DefloodCount;
            }
            ui16Count++;
            return true;
        }
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui16Count = 0;
    }
    else if ((ui64LastOkTick + ui32DefloodTime) <= ServerManager::m_ui64ActualTick)
    {
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui16Count = 0;
    }

    ui16Count++;
    return false;
}
//---------------------------------------------------------------------------

bool DeFloodCheckForSameFlood(User* pUser,
                              DefloodTypes eDefloodType,
                              const int16_t ui16Action,
                              uint16_t& ui16Count,
                              uint64_t ui64LastOkTick,
                              const int16_t ui16DefloodCount,
                              const uint32_t ui32DefloodTime,
                              const char* sNewData,
                              const size_t ui32NewDataLen,
                              const char* sOldData,
                              const uint16_t ui16OldDataLen,
                              bool& bNewData,
                              const char* sOtherNick /* = nullptr*/)
{
    if (ui16OldDataLen == 0 && ui32NewDataLen == 0)
    {
        return false;
    }
    if (static_cast<uint32_t>(ui16OldDataLen) == ui32NewDataLen &&
        (ServerManager::m_ui64ActualTick >= ui64LastOkTick && (ui64LastOkTick + ui32DefloodTime) > ServerManager::m_ui64ActualTick) &&
        memcmp(sNewData, sOldData, ui16OldDataLen) == 0)
    {
        if (ui16Count < ui16DefloodCount)
        {
            ui16Count++;

            return false;
        }
        if (ui16Count == ui16DefloodCount)
        {
            DeFloodDoAction(pUser, eDefloodType, ui16Action, ui16Count, sOtherNick);
            if (std::to_underlying(pUser->m_ui8State) < std::to_underlying(User::UserStates::STATE_CLOSING))
            {
                ui16Count++;
            }

            return true;
        }
        if (ui16Action == 2 && ui16Count == (ui16DefloodCount * 2))
        {
            pUser->m_ui32DefloodWarnings++;

            if (DeFloodCheckForWarn(pUser, eDefloodType, sOtherNick))
            {
                return true;
            }
            ui16Count -= ui16DefloodCount;
        }
        ui16Count++;

        return true;
    }
    bNewData = true;
    return false;
}
//---------------------------------------------------------------------------

bool DeFloodCheckForDataFlood(User* pUser,
                              DefloodTypes eDefloodType,
                              const int16_t ui16Action,
                              uint32_t& ui32Count,
                              uint64_t& ui64LastOkTick,
                              const int16_t ui16DefloodCount,
                              const uint32_t ui32DefloodTime)
{
    if (static_cast<uint16_t>(ui32Count / 1024) >= ui16DefloodCount)
    {
        if ((ui64LastOkTick + ui32DefloodTime) > ServerManager::m_ui64ActualTick) [[unlikely]]
        {
            if ((pUser->m_ui32BoolBits & User::BIT_RECV_FLOODER) == User::BIT_RECV_FLOODER)
            {
                return true;
            }
            pUser->m_ui32BoolBits |= User::BIT_RECV_FLOODER;
            auto ui16Count = static_cast<uint16_t>(ui32Count);
            DeFloodDoAction(pUser, eDefloodType, ui16Action, ui16Count, nullptr);
            return true;
        }
        pUser->m_ui32BoolBits &= ~User::BIT_RECV_FLOODER;
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui32Count = 0;
        return false;
    }
    else if ((ui64LastOkTick + ui32DefloodTime) <= ServerManager::m_ui64ActualTick)
    {
        pUser->m_ui32BoolBits &= ~User::BIT_RECV_FLOODER;
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui32Count = 0;
        return false;
    }

    return false;
}
//---------------------------------------------------------------------------

void DeFloodDoAction(User* pUser, DefloodTypes eDefloodType, const int16_t ui16Action, uint16_t& ui16Count, const char* sOtherNick)
{
    switch (ui16Action)
    {
    case 1:
    {
        pUser->SendFormatCheckPM("DeFloodDoAction1", sOtherNick, true, "<%s> %s!|", SettingManager::HubSec(), DeFloodGetMessage(eDefloodType, 0));

        if (eDefloodType != DefloodTypes::MAX_DOWN)
        {
            ui16Count++;
        }
        return;
    }
    case 2:

        pUser->m_ui32DefloodWarnings++;

        if (!DeFloodCheckForWarn(pUser, eDefloodType, sOtherNick) && eDefloodType != DefloodTypes::MAX_DOWN)
        {
            ui16Count++;
        }

        return;
    case 3:
    {
        pUser->SendFormatCheckPM("DeFloodDoAction2", sOtherNick, false, "<%s> %s!|", SettingManager::HubSec(), DeFloodGetMessage(eDefloodType, 0));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_DISCONNECTED)].c_str());

        pUser->Close();
        return;
    }
    case 4:
    {
        BanManager::m_Ptr->TempBan(pUser, DeFloodGetMessage(eDefloodType, 1), nullptr, 0, 0, false);

        pUser->SendFormatCheckPM("DeFloodDoAction3",
                                 sOtherNick,
                                 false,
                                 "<%s> %s: %s!|",
                                 SettingManager::HubSec(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_BEING_KICKED_BCS)].c_str(),
                                 DeFloodGetMessage(eDefloodType, 1));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_KICKED)].c_str());

        pUser->Close();
        return;
    }
    case 5:
    {
        BanManager::m_Ptr->TempBan(
            pUser, DeFloodGetMessage(eDefloodType, 1), nullptr, SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_TEMP_BAN_TIME)], 0, false);

        pUser->SendFormatCheckPM("DeFloodDoAction4",
                                 sOtherNick,
                                 false,
                                 "<%s> %s: %s %s: %s!|",
                                 SettingManager::HubSec(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_HAD_BEEN_TEMP_BANNED_TO)].c_str(),
                                 formatTime(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_TEMP_BAN_TIME)]).c_str(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                 DeFloodGetMessage(eDefloodType, 1));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_TEMPORARY_BANNED)].c_str());

        pUser->Close();
        return;
    }
    case 6:
    {
        BanManager::m_Ptr->Ban(pUser, DeFloodGetMessage(eDefloodType, 1), nullptr, false);

        pUser->SendFormatCheckPM("DeFloodDoAction5",
                                 sOtherNick,
                                 false,
                                 "<%s> %s: %s!|",
                                 SettingManager::HubSec(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_BEING_BANNED_BECAUSE)].c_str(),
                                 DeFloodGetMessage(eDefloodType, 1));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_BANNED)].c_str());

        pUser->Close();
        return;
    }
    default:
        break;
    }
}
//---------------------------------------------------------------------------

bool DeFloodCheckForWarn(User* pUser, DefloodTypes eDefloodType, const char* sOtherNick)
{
    if (pUser->m_ui32DefloodWarnings < static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_COUNT)]))
    {
        pUser->SendFormat("DeFloodCheckForWarn", true, "<%s> %s!|", SettingManager::HubSec(), DeFloodGetMessage(eDefloodType, 0));
        return false;
    }

    switch (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_ACTION)])
    {
    case 0:
    {
        pUser->SendFormatCheckPM("DeFloodCheckForWarn1",
                                 sOtherNick,
                                 false,
                                 "<%s> %s: %s!|",
                                 SettingManager::HubSec(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_BEING_DISCONNECTED_BECAUSE)].c_str(),
                                 DeFloodGetMessage(eDefloodType, 1));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_DISCONNECTED)].c_str());

        break;
    }
    case 1:
    {
        BanManager::m_Ptr->TempBan(pUser, DeFloodGetMessage(eDefloodType, 1), nullptr, 0, 0, false);

        pUser->SendFormatCheckPM("DeFloodCheckForWarn2",
                                 sOtherNick,
                                 false,
                                 "<%s> %s: %s!|",
                                 SettingManager::HubSec(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_BEING_KICKED_BCS)].c_str(),
                                 DeFloodGetMessage(eDefloodType, 1));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_KICKED)].c_str());

        break;
    }
    case 2:
    {
        BanManager::m_Ptr->TempBan(
            pUser, DeFloodGetMessage(eDefloodType, 1), nullptr, SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_TEMP_BAN_TIME)], 0, false);

        pUser->SendFormatCheckPM("DeFloodCheckForWarn3",
                                 sOtherNick,
                                 false,
                                 "<%s> %s: %s %s: %s!|",
                                 SettingManager::HubSec(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_HAD_BEEN_TEMP_BANNED_TO)].c_str(),
                                 formatTime(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_TEMP_BAN_TIME)]).c_str(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                 DeFloodGetMessage(eDefloodType, 1));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_TEMPORARY_BANNED)].c_str());

        break;
    }
    case 3:
    {
        BanManager::m_Ptr->Ban(pUser, DeFloodGetMessage(eDefloodType, 1), nullptr, false);

        pUser->SendFormatCheckPM("DeFloodCheckForWarn4",
                                 sOtherNick,
                                 false,
                                 "<%s> %s: %s!|",
                                 SettingManager::HubSec(),
                                 LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_ARE_BEING_BANNED_BECAUSE)].c_str(),
                                 DeFloodGetMessage(eDefloodType, 1));

        DeFloodReport(pUser, eDefloodType, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_BANNED)].c_str());

        break;
    }
    default:
        break;
    }

    pUser->Close();
    return true;
}
//---------------------------------------------------------------------------

const char* DeFloodGetMessage(DefloodTypes eDefloodType, const uint8_t ui8MsgId)
{
    const auto idx = std::to_underlying(eDefloodType);
    if (idx >= std::size(g_defloodMessages) || ui8MsgId > 2)
    {
        return "";
    }

    const auto& entry = g_defloodMessages[idx];
    switch (ui8MsgId)
    {
    case 0:
        return LanguageManager::m_Ptr->m_sTexts[entry.m_sWarning].c_str();
    case 1:
        return LanguageManager::m_Ptr->m_sTexts[entry.m_sAction].c_str();
    case 2:
        return LanguageManager::m_Ptr->m_sTexts[entry.m_sReport].c_str();
    default:
        return "";
    }
}
//---------------------------------------------------------------------------

void DeFloodReport(User* pUser, DefloodTypes eDefloodType, const char* sAction)
{
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_DEFLOOD_REPORT)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("DeFloodReport",
                                                    "<%s> *** %s %s %s %s %s.|",
                                                    SettingManager::HubSec(),
                                                    DeFloodGetMessage(eDefloodType, 2),
                                                    pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                    pUser->m_sIP.data(),
                                                    sAction);
    }

    UdpDebug::m_Ptr->BroadcastFormat(
        "[SYS] Flood type %hu from %s (%s) - user closed.", std::to_underlying(eDefloodType), pUser->m_sNick.c_str(), pUser->m_sIP.data());
}
//---------------------------------------------------------------------------

bool DeFloodCheckInterval(User* pUser,
                          DefloodTypes eDefloodType,
                          uint16_t& ui16Count,
                          uint64_t& ui64LastOkTick,
                          const int16_t ui16DefloodCount,
                          const uint32_t ui32DefloodTime,
                          const char* sOtherNick /* = nullptr*/)
{
    if (ui16Count == 0)
    {
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
    }
    else if (ui16Count >= ui16DefloodCount)
    {
        if ((ui64LastOkTick + ui32DefloodTime) > ServerManager::m_ui64ActualTick) [[unlikely]]
        {
            ui16Count++;

            pUser->SendFormatCheckPM("DeFloodCheckInterval",
                                     sOtherNick,
                                     true,
                                     "<%s> %s %" PRIu64 " %s.|",
                                     SettingManager::HubSec(),
                                     LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PLEASE_WAIT)].c_str(),
                                     (ui64LastOkTick + ui32DefloodTime) - ServerManager::m_ui64ActualTick,
                                     DeFloodGetMessage(eDefloodType, 0));

            return true;
        }
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui16Count = 0;
    }
    else if ((ui64LastOkTick + ui32DefloodTime) <= ServerManager::m_ui64ActualTick)
    {
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui16Count = 0;
    }

    ui16Count++;
    return false;
}
//---------------------------------------------------------------------------
