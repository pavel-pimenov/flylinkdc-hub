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

//---------------------------------------------------------------------------
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "MainWindowPage.h"
//---------------------------------------------------------------------------
#include "../core/ServerManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "MainWindow.h"
//---------------------------------------------------------------------------
static ATOM atomMainWindowPage = 0;
//---------------------------------------------------------------------------

MainWindowPage::MainWindowPage() : m_hWnd(NULL)
{
	// ...
}
//---------------------------------------------------------------------------

LRESULT CALLBACK MainWindowPage::StaticMainWindowPageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MainWindowPage * pMainWindowPage = (MainWindowPage *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pMainWindowPage == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pMainWindowPage->MainWindowPageProc(uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

void MainWindowPage::CreateHWND(HWND hOwner)
{
	if (atomMainWindowPage == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_MainWindowPage";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomMainWindowPage = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcMain;
	::GetClientRect(hOwner, &rcMain);
	
	m_hWnd = ::CreateWindowEx(WS_EX_CONTROLPARENT, MAKEINTATOM(atomMainWindowPage), "", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                          0, clsGuiSettingManager::iEditHeight + 1, rcMain.right, rcMain.bottom - (clsGuiSettingManager::iEditHeight + 1), hOwner, NULL, clsServerManager::hInstance, NULL);
	                          
	if (m_hWnd != NULL)
	{
		::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
		::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)StaticMainWindowPageProc);
	}
}
//---------------------------------------------------------------------------

LRESULT FirstItemProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC wpOldProc)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, FALSE));
			return 0;
		}
		else
		{
			::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
			return 0;
		}
	}
	
	return ::CallWindowProc(wpOldProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK FirstButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return FirstItemProc(hWnd, uMsg, wParam, lParam, clsGuiSettingManager::wpOldButtonProc);
}
//------------------------------------------------------------------------------

LRESULT LastItemProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC wpOldProc)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
			return 0;
		}
		else
		{
			::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, TRUE));
			return 0;
		}
	}
	
	return ::CallWindowProc(wpOldProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK LastButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return LastItemProc(hWnd, uMsg, wParam, lParam, clsGuiSettingManager::wpOldButtonProc);
}

//------------------------------------------------------------------------------
