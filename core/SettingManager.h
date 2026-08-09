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
#ifndef SetManH
#define SetManH
//---------------------------------------------------------------------------
#include <array>
#include <string>
#include <string_view>
#include "SettingIds.h"
#include "CriticalSection.h"
//---------------------------------------------------------------------------

class SettingManager
{
private:
    mutable CriticalSection m_csSetting;

    void CreateDefaultMOTD();
    void LoadMOTD();
    void SaveMOTD();
    void CheckMOTD();

    void CheckAndSet(const char* sName, const char* sValue);
    void Load();
    void LoadXML();

    // Helpers to reduce duplication in Update*Message methods
    [[nodiscard]] auto AppendRedirectAddress(char* sDest, size_t szDestSize, int iMsgLen, size_t szBoolId, size_t szTxtRedirId) const -> int;
    void CommitGlobalBufferToPreText(size_t szPreTxtId, int iMsgLen, const char* sFuncName);

public:
    static std::unique_ptr<SettingManager> m_Ptr;
    static time_t m_tConfigLoadTime;

    enum class SetPreTxtIds : uint8_t
    {
        SETPRETXT_HUB_SEC,
        SETPRETXT_MOTD,
        SETPRETXT_HUB_NAME_WLCM,
        SETPRETXT_HUB_NAME,
        SETPRETXT_REDIRECT_ADDRESS,
        SETPRETXT_REG_ONLY_MSG,
        SETPRETXT_SHARE_LIMIT_MSG,
        SETPRETXT_SLOTS_LIMIT_MSG,
        SETPRETXT_HUB_SLOT_RATIO_MSG,
        SETPRETXT_MAX_HUBS_LIMIT_MSG,
        SETPRETXT_NO_TAG_MSG,
        SETPRETXT_TEMP_BAN_REDIR_ADDRESS,
        SETPRETXT_PERM_BAN_REDIR_ADDRESS,
        SETPRETXT_NICK_LIMIT_MSG,
        SETPRETXT_HUB_BOT_MYINFO,
        SETPRETXT_OP_CHAT_HELLO,
        SETPRETXT_OP_CHAT_MYINFO,
        SETPRETXT_IDS_END
    }; // SETPRETXT_,

    uint64_t m_ui64MinShare = 0; // SettingManager->ui64MinShare
    uint64_t m_ui64MaxShare = 0; // SettingManager->ui64MaxShare

    std::string m_sMOTD;

    std::array<std::string, std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_IDS_END)> m_sPreTexts{}; // SettingManager->sPreTexts[]
    std::array<std::string, std::to_underlying(SetTxtIds::SETTXT_IDS_END)> m_sTexts{};       // SettingManager->m_sTexts[]

    std::array<int16_t, std::to_underlying(SetShortIds::SETSHORT_IDS_END)> m_i16Shorts{}; // SettingManager->i16Shorts[]

    std::array<uint16_t, 25> m_ui16PortNumbers{}; // SettingManager->ui16PortNumbers[0]

    std::array<bool, std::to_underlying(SetBoolIds::SETBOOL_IDS_END)> m_bBools{}; // SettingManager->bBools[]

    // PPK ... same nick for bot and opchat bool
    bool m_bBotsSameNick = false; // SettingManager->m_bBotsSameNick

    bool m_bUpdateLocked = true; // SettingManager->m_bUpdateLocked

    bool m_bFirstRun = false;

    uint8_t m_ui8FullMyINFOOption = 0; // SettingManager->ui8FullMyINFOOption;

    SettingManager(const SettingManager&) = delete;
    auto operator=(const SettingManager&) -> SettingManager& = delete;

    SettingManager();
    ~SettingManager();

    [[nodiscard]] auto GetBool(size_t szBoolId) const -> bool;
    [[nodiscard]] auto GetFirstPort() const -> uint16_t;
    [[nodiscard]] auto GetShort(size_t szShortId) const -> int16_t;
    [[nodiscard]] auto GetText(size_t szTxtId) const -> std::string;

    void SetBool(size_t szBoolId, bool bValue); // SettingManager->SetBool()
    void SetMOTD(std::string_view sTxt);
    void SetShort(size_t szShortId, int16_t i16Value);
    void SetText(size_t szTxtId, std::string_view sTxt);

    void UpdateBot(bool bNickChanged = true);
    void DisableBot(bool bNickChanged = true, bool bRemoveMyINFO = true);
    void UpdateOpChat(bool bNickChanged = true);
    void DisableOpChat(bool bNickChanged = true);

    void Save();
    void UpdateAll();

    void UpdateHubSec();
    void UpdateMOTD();
    void UpdateHubNameWelcome();
    void UpdateHubName();
    void UpdateRedirectAddress();
    void UpdateRegOnlyMessage();
    void UpdateShareLimitMessage();
    void UpdateSlotsLimitMessage();
    void UpdateHubSlotRatioMessage();
    void UpdateMaxHubsLimitMessage();
    void UpdateNoTagMessage();
    void UpdateTempBanRedirAddress();
    void UpdatePermBanRedirAddress();
    void UpdateNickLimitMessage();
    void UpdateMinShare();
    void UpdateMaxShare();

    // Helpers for limit message builder boilerplate
    [[nodiscard]] auto BeginLimitMessage() const -> int;
    void EndLimitMessage(int iMsgLen, size_t preTxtId, size_t boolRedirId, size_t txtRedirId, const char* funcName);
    void UpdateTCPPorts();
    void UpdateBotsSameNick();
    void UpdateLanguage();
    void UpdateUDPPort();
    void UpdateScripting() const;
    static void UpdateDatabase();

    void CmdLineBasicSetup();
    void CmdLineCompleteSetup();

    // Shortcuts for most commonly accessed pre-text
    [[nodiscard]] static auto HubSec() -> const char*
    {
        return m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str();
    }
    [[nodiscard]] static auto HubSecStr() -> const std::string&
    {
        return m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)];
    }
};
//---------------------------------------------------------------------------

#endif
