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
#ifndef MainWindowPageStatsH
#define MainWindowPageStatsH
//------------------------------------------------------------------------------
#include "MainWindowPage.h"
//------------------------------------------------------------------------------

class MainWindowPageStats : public MainWindowPage {
public:
    HWND m_hWndPageItems[20];

    enum enmPageItems {
        BTN_START_STOP,
        GB_STATS,
        LBL_STATUS,
        LBL_STATUS_VALUE,
        LBL_JOINS,
        LBL_JOINS_VALUE,
        LBL_PARTS,
        LBL_PARTS_VALUE,
        LBL_ACTIVE,
        LBL_ACTIVE_VALUE,
        LBL_ONLINE,
        LBL_ONLINE_VALUE,
        LBL_PEAK,
        LBL_PEAK_VALUE,
        LBL_RECEIVED,
        LBL_RECEIVED_VALUE,
        LBL_SENT,
        LBL_SENT_VALUE,
        BTN_REDIRECT_ALL,
        BTN_MASS_MSG
    };

    MainWindowPageStats();
    ~MainWindowPageStats() { };

    bool CreateMainWindowPage(HWND hOwner);
    void UpdateLanguage();
    char * GetPageName();
    void FocusFirstItem();
    void FocusLastItem();
private:
    MainWindowPageStats(const MainWindowPageStats&) = delete;
    const MainWindowPageStats& operator=(const MainWindowPageStats&) = delete;

    LRESULT MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnRedirectAll();
    void OnMassMessage();
};
//------------------------------------------------------------------------------

#endif
