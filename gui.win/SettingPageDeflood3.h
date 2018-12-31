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
#ifndef SettingPageDeflood3H
#define SettingPageDeflood3H
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageDeflood3 : public SettingPage {
public:
    SettingPageDeflood3();
    ~SettingPageDeflood3() { };

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
    HWND m_hWndPageItems[84];
    
    enum enmPageItems {
        GB_SEARCH,
        CB_SEARCH,
        EDT_SEARCH_MSGS,
        UD_SEARCH_MSGS,
        LBL_SEARCH_DIVIDER,
        EDT_SEARCH_SECS,
        UD_SEARCH_SECS,
        LBL_SEARCH_SECONDS,
        CB_SEARCH2,
        EDT_SEARCH_MSGS2,
        UD_SEARCH_MSGS2,
        LBL_SEARCH_DIVIDER2,
        EDT_SEARCH_SECS2,
        UD_SEARCH_SECS2,
        LBL_SEARCH_SECONDS2,
        LBL_SEARCH_INTERVAL,
        EDT_SEARCH_INTERVAL_MSGS,
        UD_SEARCH_INTERVAL_MSGS,
        LBL_SEARCH_INTERVAL_DIVIDER,
        EDT_SEARCH_INTERVAL_SECS,
        UD_SEARCH_INTERVAL_SECS,
        LBL_SEARCH_INTERVAL_SECONDS,
        GB_SAME_SEARCH,
        CB_SAME_SEARCH,
        EDT_SAME_SEARCH_MSGS,
        UD_SAME_SEARCH_MSGS,
        LBL_SAME_SEARCH_DIVIDER,
        EDT_SAME_SEARCH_SECS,
        UD_SAME_SEARCH_SECS,
        LBL_SAME_SEARCH_SECONDS,
        GB_SR,
        CB_SR,
        EDT_SR_MSGS,
        UD_SR_MSGS,
        LBL_SR_DIVIDER,
        EDT_SR_SECS,
        UD_SR_SECS,
        LBL_SR_SECONDS,
        CB_SR2,
        EDT_SR_MSGS2,
        UD_SR_MSGS2,
        LBL_SR_DIVIDER2,
        EDT_SR_SECS2,
        UD_SR_SECS2,
        LBL_SR_SECONDS2,
        GB_SR_TO_PASSIVE_LIMIT,
        EDT_SR_TO_PASSIVE_LIMIT,
        UD_SR_TO_PASSIVE_LIMIT,
        GB_SR_LEN,
        LBL_SR_LEN,
        EDT_SR_LEN,
        UD_SR_LEN,
        GB_GETNICKLIST,
        CB_GETNICKLIST,
        EDT_GETNICKLIST_MSGS,
        UD_GETNICKLIST_MSGS,
        LBL_GETNICKLIST_DIVIDER,
        EDT_GETNICKLIST_MINS,
        UD_GETNICKLIST_MINS,
        LBL_GETNICKLIST_MINUTES,
        GB_MYINFO,
        CB_MYINFO,
        EDT_MYINFO_MSGS,
        UD_MYINFO_MSGS,
        LBL_MYINFO_DIVIDER,
        EDT_MYINFO_SECS,
        UD_MYINFO_SECS,
        LBL_MYINFO_SECONDS,
        CB_MYINFO2,
        EDT_MYINFO_MSGS2,
        UD_MYINFO_MSGS2,
        LBL_MYINFO_DIVIDER2,
        EDT_MYINFO_SECS2,
        UD_MYINFO_SECS2,
        LBL_MYINFO_SECONDS2,
        GB_MYINFO_LEN,
        LBL_MYINFO_LEN,
        EDT_MYINFO_LEN,
        UD_MYINFO_LEN,
        GB_RECONNECT_TIME,
        LBL_RECONNECT_TIME_MINIMAL,
        EDT_RECONNECT_TIME,
        UD_RECONNECT_TIME,
        LBL_RECONNECT_TIME_SECONDS,
    };

    SettingPageDeflood3(const SettingPageDeflood3&) = delete;
    const SettingPageDeflood3& operator=(const SettingPageDeflood3&) = delete;

    LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
