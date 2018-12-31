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
#ifndef SettingPageDeflood2H
#define SettingPageDeflood2H
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageDeflood2 : public SettingPage {
public:
    SettingPageDeflood2();
    ~SettingPageDeflood2() { };

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
    HWND m_hWndPageItems[74];
    
    enum enmPageItems {
        GB_PM,
        CB_PM,
        EDT_PM_MSGS,
        UD_PM_MSGS,
        LBL_PM_DIVIDER,
        EDT_PM_SECS,
        UD_PM_SECS,
        LBL_PM_SECONDS,
        CB_PM2,
        EDT_PM_MSGS2,
        UD_PM_MSGS2,
        LBL_PM_DIVIDER2,
        EDT_PM_SECS2,
        UD_PM_SECS2,
        LBL_PM_SECONDS2,
        LBL_PM_INTERVAL,
        EDT_PM_INTERVAL_MSGS,
        UD_PM_INTERVAL_MSGS,
        LBL_PM_INTERVAL_DIVIDER,
        EDT_PM_INTERVAL_SECS,
        UD_PM_INTERVAL_SECS,
        LBL_PM_INTERVAL_SECONDS,
        GB_SAME_PM,
        CB_SAME_PM,
        EDT_SAME_PM_MSGS,
        UD_SAME_PM_MSGS,
        LBL_SAME_PM_DIVIDER,
        EDT_SAME_PM_SECS,
        UD_SAME_PM_SECS,
        LBL_SAME_PM_SECONDS,
        GB_SAME_MULTI_PM,
        CB_SAME_MULTI_PM,
        EDT_SAME_MULTI_PM_MSGS,
        UD_SAME_MULTI_PM_MSGS,
        LBL_SAME_MULTI_PM_WITH,
        EDT_SAME_MULTI_PM_LINES,
        UD_SAME_MULTI_PM_LINES,
        LBL_SAME_MULTI_PM_LINES,
        GB_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS,
        LBL_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS,
        EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS,
        UD_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS,
        GB_RECEIVED_DATA,
        CB_RECEIVED_DATA,
        EDT_RECEIVED_DATA_KB,
        UD_RECEIVED_DATA_KB,
        LBL_RECEIVED_DATA_DIVIDER,
        EDT_RECEIVED_DATA_SECS,
        UD_RECEIVED_DATA_SECS,
        LBL_RECEIVED_DATA_SECONDS,
        CB_RECEIVED_DATA2,
        EDT_RECEIVED_DATA_KB2,
        UD_RECEIVED_DATA_KB2,
        LBL_RECEIVED_DATA_DIVIDER2,
        EDT_RECEIVED_DATA_SECS2,
        UD_RECEIVED_DATA_SECS2,
        LBL_RECEIVED_DATA_SECONDS2,
        GB_WARN_SETTING,
        CB_WARN_ACTION,
        LBL_WARN_AFTER,
        EDT_WARN_COUNT,
        UD_WARN_COUNT,
        LBL_WARN_WARNINGS,
        GB_NEW_CONNS_FROM_SAME_IP,
        EDT_NEW_CONNS_FROM_SAME_IP_COUNT,
        UD_NEW_CONNS_FROM_SAME_IP_COUNT,
        LBL_NEW_CONNS_FROM_SAME_IP_DIVIDER,
        EDT_NEW_CONNS_FROM_SAME_IP_TIME,
        UD_NEW_CONNS_FROM_SAME_IP_TIME,
        LBL_NEW_CONNS_FROM_SAME_IP_SECONDS,
        GB_MAX_USERS_FROM_SAME_IP,
        EDT_MAX_USERS_FROM_SAME_IP,
        UD_MAX_USERS_FROM_SAME_IP,
        BTN_REPORT_FLOOD_TO_OPS
    };

    SettingPageDeflood2(const SettingPageDeflood2&) = delete;
    const SettingPageDeflood2& operator=(const SettingPageDeflood2&) = delete;

    LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
