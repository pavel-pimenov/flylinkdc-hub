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
#ifndef MainWindowH
#define MainWindowH
//------------------------------------------------------------------------------
#include "MainWindowPage.h"
//------------------------------------------------------------------------------
#define WM_UPDATE_CHECK_MSG (WM_USER+11)
#define WM_UPDATE_CHECK_TERMINATE (WM_USER+12)
#define WM_UPDATE_CHECK_DATA (WM_USER+13)
//------------------------------------------------------------------------------

class clsMainWindow
{
public:
	static clsMainWindow * mPtr;
	
	HWND m_hWnd;
	
	HWND m_hWndWindowItems[1];
	
	enum enmWindowItems
	{
		TC_TABS
	};
	
	clsMainWindow();
	~clsMainWindow();
	
	static LRESULT CALLBACK StaticMainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	HWND CreateEx();
	
	void UpdateSysTray() const;
	void UpdateStats() const;
	void UpdateTitleBar();
	void UpdateLanguage();
	void EnableStartButton(const BOOL &bEnable) const;
	void SetStartButtonText(const char * sText) const;
	void SetStatusValue(const char * sText) const;
	void EnableGuiItems(const BOOL &bEnable) const;
	static void SaveGuiSettings();
private:
	uint64_t ui64LastTrayMouseMove;
	
	MainWindowPage * MainWindowPages[3];
	
	UINT uiTaskBarCreated;
	
	
	LRESULT MainWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void OnSelChanged();
	
	DISALLOW_COPY_AND_ASSIGN(clsMainWindow);
};
//------------------------------------------------------------------------------

#endif
