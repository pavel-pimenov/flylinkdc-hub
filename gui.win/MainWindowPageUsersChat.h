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

class clsMainWindowPageUsersChat : public MainWindowPage, private BasicSplitter
{
public:
	static clsMainWindowPageUsersChat * mPtr;
	
	HWND hWndPageItems[7];
	
	enum enmPageItems
	{
		BTN_SHOW_CHAT,
		BTN_SHOW_COMMANDS,
		REDT_CHAT,
		EDT_CHAT,
		BTN_AUTO_UPDATE_USERLIST,
		LV_USERS,
		BTN_UPDATE_USERS
	};
	
	clsMainWindowPageUsersChat();
	~clsMainWindowPageUsersChat();
	
	bool CreateMainWindowPage(HWND hOwner);
	void UpdateLanguage();
	char * GetPageName();
	void FocusFirstItem();
	void FocusLastItem();
	
	bool OnEditEnter();
	void AddUser(const User * curUser);
	void RemoveUser(const User * curUser);
	User * GetUser();
private:

	LRESULT MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void UpdateUserList();
	void OnContextMenu(HWND hWindow, LPARAM lParam);
	void DisconnectUser();
	void KickUser();
	void BanUser();
	void RedirectUser();
	
	HWND GetWindowHandle();
	void UpdateSplitterParts();
	DISALLOW_COPY_AND_ASSIGN(clsMainWindowPageUsersChat);
};
//------------------------------------------------------------------------------

#endif
