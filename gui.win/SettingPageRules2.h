/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2017  Petr Kozelka, PPK at PtokaX dot org

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

//------------------------------------------------------------------------------
#ifndef SettingPageRules2H
#define SettingPageRules2H
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageRules2 : public SettingPage {
public:
    bool m_bUpdateSlotsLimitMessage, m_bUpdateHubSlotRatioMessage, m_bUpdateMaxHubsLimitMessage;

    SettingPageRules2();
    ~SettingPageRules2() { };

    bool CreateSettingPage(HWND hOwner);

    void Save();
    void GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
        bool & /*bUpdatedAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
        bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool & /*bUpdatedNoTagMessage*/,
        bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
        bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
        bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);

    char * GetPageName();
    void FocusLastItem();
private:
    HWND m_hWndPageItems[42];
    
    enum enmPageItems {
        GB_SLOTS_LIMITS,
        EDT_SLOTS_MIN,
        UD_SLOTS_MIN,
        LBL_SLOTS_MIN,
        LBL_SLOTS_MAX,
        EDT_SLOTS_MAX,
        UD_SLOTS_MAX,
        GB_SLOTS_MSG,
        EDT_SLOTS_MSG,
        GB_SLOTS_REDIR,
        BTN_SLOTS_REDIR,
        EDT_SLOTS_REDIR_ADDR,
        GB_HUBS_SLOTS_LIMITS,
        LBL_HUBS,
        EDT_HUBS,
        UD_HUBS,
        LBL_DIVIDER,
        EDT_SLOTS,
        UD_SLOTS,
        LBL_SLOTS,
        GB_HUBS_SLOTS_MSG,
        EDT_HUBS_SLOTS_MSG,
        GB_HUBS_SLOTS_REDIR,
        BTN_HUBS_SLOTS_REDIR,
        EDT_HUBS_SLOTS_REDIR_ADDR,
        GB_HUBS_LIMIT,
        EDT_MAX_HUBS,
        UD_MAX_HUBS,
        LBL_MAX_HUBS,
        GB_HUBS_MSG,
        EDT_HUBS_MSG,
        GB_HUBS_REDIR,
        BTN_HUBS_REDIR,
        EDT_HUBS_REDIR_ADDR,
        GB_CTM_LEN,
        LBL_CTM_LEN,
        EDT_CTM_LEN,
        UD_CTM_LEN,
        GB_RCTM_LEN,
        LBL_RCTM_LEN,
        EDT_RCTM_LEN,
        UD_RCTM_LEN,
    };

    SettingPageRules2(const SettingPageRules2&) = delete;
    const SettingPageRules2& operator=(const SettingPageRules2&) = delete;

    LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
