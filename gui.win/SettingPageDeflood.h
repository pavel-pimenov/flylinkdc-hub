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
#ifndef SettingPageDefloodH
#define SettingPageDefloodH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageDeflood : public SettingPage {
public:
    SettingPageDeflood();
    ~SettingPageDeflood() { };

    bool CreateSettingPage(HWND hOwner);

    void Save();
    void GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
        bool & /*bUpdatedAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
        bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
        bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
        bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
        bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);

    char * GetPageName();
    void FocusLastItem();
private:
    HWND m_hWndPageItems[88];
    
    enum enmPageItems {
        GB_GLOBAL_MAIN_CHAT,
        EDT_GLOBAL_MAIN_CHAT_MSGS,
        UD_GLOBAL_MAIN_CHAT_MSGS,
        LBL_GLOBAL_MAIN_CHAT_DIVIDER,
        EDT_GLOBAL_MAIN_CHAT_SECS,
        UD_GLOBAL_MAIN_CHAT_SECS,
        LBL_GLOBAL_MAIN_CHAT_SEC,
        CB_GLOBAL_MAIN_CHAT,
        LBL_GLOBAL_MAIN_CHAT_FOR,
        EDT_GLOBAL_MAIN_CHAT_SECS2,
        UD_GLOBAL_MAIN_CHAT_SECS2,
        LBL_GLOBAL_MAIN_CHAT_SEC2,
        GB_MAIN_CHAT,
        CB_MAIN_CHAT,
        EDT_MAIN_CHAT_MSGS,
        UD_MAIN_CHAT_MSGS,
        LBL_MAIN_CHAT_DIVIDER,
        EDT_MAIN_CHAT_SECS,
        UD_MAIN_CHAT_SECS,
        LBL_MAIN_CHAT_SECONDS,
        CB_MAIN_CHAT2,
        EDT_MAIN_CHAT_MSGS2,
        UD_MAIN_CHAT_MSGS2,
        LBL_MAIN_CHAT_DIVIDER2,
        EDT_MAIN_CHAT_SECS2,
        UD_MAIN_CHAT_SECS2,
        LBL_MAIN_CHAT_SECONDS2,
        LBL_MAIN_CHAT_INTERVAL,
        EDT_MAIN_CHAT_INTERVAL_MSGS,
        UD_MAIN_CHAT_INTERVAL_MSGS,
        LBL_MAIN_CHAT_INTERVAL_DIVIDER,
        EDT_MAIN_CHAT_INTERVAL_SECS,
        UD_MAIN_CHAT_INTERVAL_SECS,
        LBL_MAIN_CHAT_INTERVAL_SECONDS,
        GB_SAME_MAIN_CHAT,
        CB_SAME_MAIN_CHAT,
        EDT_SAME_MAIN_CHAT_MSGS,
        UD_SAME_MAIN_CHAT_MSGS,
        LBL_SAME_MAIN_CHAT_DIVIDER,
        EDT_SAME_MAIN_CHAT_SECS,
        UD_SAME_MAIN_CHAT_SECS,
        LBL_SAME_MAIN_CHAT_SECONDS,
        GB_SAME_MULTI_MAIN_CHAT,
        CB_SAME_MULTI_MAIN_CHAT,
        EDT_SAME_MULTI_MAIN_CHAT_MSGS,
        UD_SAME_MULTI_MAIN_CHAT_MSGS,
        LBL_SAME_MULTI_MAIN_CHAT_WITH,
        EDT_SAME_MULTI_MAIN_CHAT_LINES,
        UD_SAME_MULTI_MAIN_CHAT_LINES,
        LBL_SAME_MULTI_MAIN_CHAT_LINES,
        GB_CTM,
        CB_CTM,
        EDT_CTM_MSGS,
        UD_CTM_MSGS,
        LBL_CTM_DIVIDER,
        EDT_CTM_SECS,
        UD_CTM_SECS,
        LBL_CTM_SECONDS,
        CB_CTM2,
        EDT_CTM_MSGS2,
        UD_CTM_MSGS2,
        LBL_CTM_DIVIDER2,
        EDT_CTM_SECS2,
        UD_CTM_SECS2,
        LBL_CTM_SECONDS2,
        GB_RCTM,
        CB_RCTM,
        EDT_RCTM_MSGS,
        UD_RCTM_MSGS,
        LBL_RCTM_DIVIDER,
        EDT_RCTM_SECS,
        UD_RCTM_SECS,
        LBL_RCTM_SECONDS,
        CB_RCTM2,
        EDT_RCTM_MSGS2,
        UD_RCTM_MSGS2,
        LBL_RCTM_DIVIDER2,
        EDT_RCTM_SECS2,
        UD_RCTM_SECS2,
        LBL_RCTM_SECONDS2,
        GB_DEFLOOD_TEMP_BAN_TIME,
        EDT_DEFLOOD_TEMP_BAN_TIME,
        UD_DEFLOOD_TEMP_BAN_TIME,
        LBL_DEFLOOD_TEMP_BAN_TIME_MINUTES,
        GB_MAX_USERS_LOGINS,
        EDT_MAX_USERS_LOGINS,
        UD_MAX_USERS_LOGINS,
        LBL_MAX_USERS_LOGINS
    };

    SettingPageDeflood(const SettingPageDeflood&) = delete;
    const SettingPageDeflood& operator=(const SettingPageDeflood&) = delete;

    LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
