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
#ifndef SettingPageMyINFOH
#define SettingPageMyINFOH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageMyINFO : public SettingPage {
public:
    bool m_bUpdateNoTagMessage;

    SettingPageMyINFO();
    ~SettingPageMyINFO() { };

    bool CreateSettingPage(HWND hOwner);

    void Save();
    void GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
        bool & /*bUpdatedAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
        bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & bUpdatedNoTagMessage,
        bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
        bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
        bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);

    char * GetPageName();
    void FocusLastItem();
private:
    HWND m_hWndPageItems[21];
    
    enum enmPageItems {
        GB_DESCRIPTION_TAG,
        BTN_REPORT_SUSPICIOUS_TAG,
        GB_NO_TAG_ACTION,
        CB_NO_TAG_ACTION,
        GB_NO_TAG_MESSAGE,
        EDT_NO_TAG_MESSAGE,
        GB_NO_TAG_REDIRECT,
        EDT_NO_TAG_REDIRECT,
        GB_MYINFO_PROCESSING,
        LBL_ORIGINAL_MYINFO,
        CB_ORIGINAL_MYINFO_ACTION,
        GB_MODIFIED_MYINFO_OPTIONS,
        BTN_REMOVE_DESCRIPTION,
        BTN_REMOVE_TAG,
        BTN_REMOVE_CONNECTION,
        BTN_REMOVE_EMAIL,
        BTN_MODE_TO_MYINFO,
        BTN_MODE_TO_DESCRIPTION,
        LBL_MINUTES_BEFORE_ACCEPT_NEW_MYINFO,
        EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO,
        UD_MINUTES_BEFORE_ACCEPT_NEW_MYINFO
    };

    SettingPageMyINFO(const SettingPageMyINFO&) = delete;
    const SettingPageMyINFO& operator=(const SettingPageMyINFO&) = delete;

    LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
