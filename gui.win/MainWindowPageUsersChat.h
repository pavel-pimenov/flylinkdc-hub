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
#ifndef MainWindowPageUsersChatH
#define MainWindowPageUsersChatH
//------------------------------------------------------------------------------
#include "BasicSplitter.h"
#include "MainWindowPage.h"
//------------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class MainWindowPageUsersChat : public MainWindowPage, private BasicSplitter {
public:
    static MainWindowPageUsersChat * m_Ptr;

    HWND m_hWndPageItems[7];

    enum enmPageItems {
        BTN_SHOW_CHAT,
        BTN_SHOW_COMMANDS,
        REDT_CHAT,
        EDT_CHAT,
        BTN_AUTO_UPDATE_USERLIST,
        LV_USERS,
        BTN_UPDATE_USERS
    };

    enum enmMenuItems {
		IDC_REG_USER = 100,
		IDC_DISCONNECT_USER,
		IDC_KICK_USER,
		IDC_BAN_USER,
		IDC_REDIRECT_USER
    };

    MainWindowPageUsersChat();
    ~MainWindowPageUsersChat();

    bool CreateMainWindowPage(HWND hOwner);
    void UpdateLanguage();
    char * GetPageName();
    void FocusFirstItem();
    void FocusLastItem();

    bool OnEditEnter();
    void AddUser(const User * pUser);
    void RemoveUser(const User * pUser);
    User * GetUser();
private:
    MainWindowPageUsersChat(const MainWindowPageUsersChat&) = delete;
    const MainWindowPageUsersChat& operator=(const MainWindowPageUsersChat&) = delete;

    LRESULT MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void UpdateUserList();
    void OnContextMenu(HWND hWindow, LPARAM lParam);
    void DisconnectUser();
    void KickUser();
    void BanUser();
    void RedirectUser();

    HWND GetWindowHandle();
    void UpdateSplitterParts();
};
//------------------------------------------------------------------------------

#endif
