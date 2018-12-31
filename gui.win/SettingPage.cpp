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
#include "SettingPage.h"
//---------------------------------------------------------------------------
#include "../core/ServerManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "SettingDialog.h"
//---------------------------------------------------------------------------
static ATOM atomSettingPage = 0;
//---------------------------------------------------------------------------
int SettingPage::m_iFullGB = 462 - 5;
int SettingPage::m_iFullEDT = 462 - 21;
int SettingPage::m_iGBinGB = 462 - 15;
int SettingPage::m_iGBinGBEDT = 462 - 31;
int SettingPage::m_iOneCheckGB = 17 + 16 + 8;
int SettingPage::m_iTwoChecksGB = 17 + (2 * 16) + 3 + 8;
int SettingPage::m_iOneLineTwoGroupGB = 17 + 23 + (2 * GuiSettingManager::m_iOneLineGB) + 7;
int SettingPage::m_iTwoLineGB = 17 + (2 * 23) + 13;
int SettingPage::m_iThreeLineGB = 17 + (3 * 23) + 18;
//---------------------------------------------------------------------------

SettingPage::SettingPage() : m_hWnd(nullptr), m_bCreated(false) {
    // ...
}
//---------------------------------------------------------------------------

LRESULT CALLBACK SettingPage::StaticSettingPageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SettingPage * pSettingPage = (SettingPage *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pSettingPage == nullptr) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pSettingPage->SettingPageProc(uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

void SettingPage::CreateHWND(HWND hOwner) {
    if(atomSettingPage == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_SettingPage";
        m_wc.hInstance = ServerManager::m_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomSettingPage = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent = { 0 };
    ::GetClientRect(hOwner, &rcParent);

    m_hWnd = ::CreateWindowEx(WS_EX_CONTROLPARENT, MAKEINTATOM(atomSettingPage), nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        ScaleGui(154) + 10, 0, rcParent.right-(ScaleGui(154) + 10), rcParent.bottom, hOwner, nullptr, ServerManager::m_hInstance, nullptr);

    if(m_hWnd != nullptr) {
		m_bCreated = true;

        ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
        ::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)StaticSettingPageProc);
    }
}
//---------------------------------------------------------------------------

void SettingPage::RemoveDollarsPipes(HWND hWnd) {
    char buf[257];
    ::GetWindowText(hWnd, buf, 257);

    bool bChanged = false;

    for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
        if(buf[ui16i] == '$' || buf[ui16i] == '|') {
            strcpy(buf+ui16i, buf+ui16i+1);
            bChanged = true;
            ui16i--;
        }
    }

    if(bChanged == true) {
        int iStart, iEnd;

        ::SendMessage(hWnd, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

        ::SetWindowText(hWnd, buf);

        ::SendMessage(hWnd, EM_SETSEL, iStart, iEnd);
    }
}
//---------------------------------------------------------------------------

void SettingPage::RemovePipes(HWND hWnd) {
    char buf[257];
    ::GetWindowText(hWnd, buf, 257);

    bool bChanged = false;

    for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
        if(buf[ui16i] == '|') {
            strcpy(buf+ui16i, buf+ui16i+1);
            bChanged = true;
            ui16i--;
        }
    }

    if(bChanged == true) {
        int iStart, iEnd;

        ::SendMessage(hWnd, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

        ::SetWindowText(hWnd, buf);

        ::SendMessage(hWnd, EM_SETSEL, iStart, iEnd);
    }
}
//---------------------------------------------------------------------------

void SettingPage::MinMaxCheck(HWND hWnd, const int iMin, const int iMax) {
    char buf[6];
    ::GetWindowText(hWnd, buf, 6);

    int iValue = atoi(buf);

    int iStart, iEnd;

    ::SendMessage(hWnd, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

    if(iValue > iMax) {
        _itoa(iMax, buf, 10);
        ::SetWindowText(hWnd, buf);
    } else if(iValue < iMin || ::GetWindowTextLength(hWnd) == 0) {
        _itoa(iMin, buf, 10);
        ::SetWindowText(hWnd, buf);
    }

    ::SendMessage(hWnd, EM_SETSEL, iStart, iEnd);
}
//---------------------------------------------------------------------------

void SettingPage::AddUpDown(HWND &hWnd, const int iX, const int iY, const int iWidth, const int iHeight, const LPARAM lpRange, const WPARAM wpBuddy, const LPARAM lpPos) {
    hWnd = ::CreateWindowEx(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT, iX, iY, iWidth, iHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(hWnd, UDM_SETRANGE, 0, lpRange);
    ::SendMessage(hWnd, UDM_SETBUDDY, wpBuddy, 0);
    ::SendMessage(hWnd, UDM_SETPOS, 0, lpPos);
}
//---------------------------------------------------------------------------

void SettingPage::AddToolTip(HWND &hWnd, char * sTipText) const {
    HWND hWndTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    TOOLINFO ti = { 0 };
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd = m_hWnd;
	ti.uId = (UINT_PTR)hWnd;
	ti.hinst = ServerManager::m_hInstance;
	ti.lpszText = sTipText;

    ::SendMessage(hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    ::SendMessage(hWndTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM(30000, 0));
}
//---------------------------------------------------------------------------

LRESULT SettingWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC wpOldProc) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return DLGC_WANTTAB;
    } else if(uMsg == WM_CHAR && wParam == VK_TAB) {
        if((::GetKeyState(VK_SHIFT) & 0x8000) == 0) {
            ::SetFocus(SettingDialog::m_Ptr->m_hWndWindowItems[SettingDialog::TV_TREE]);
            return 0;
        } else {
			::SetFocus(::GetNextDlgTabItem(SettingDialog::m_Ptr->m_hWndWindowItems[SettingDialog::WINDOW_HANDLE], hWnd, TRUE));
            return 0;
        }
    }

    return ::CallWindowProc(wpOldProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK ButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return SettingWindowProc(hWnd, uMsg, wParam, lParam, GuiSettingManager::m_wpOldButtonProc);
}

//------------------------------------------------------------------------------

LRESULT CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return SettingWindowProc(hWnd, uMsg, wParam, lParam, GuiSettingManager::m_wpOldEditProc);
}

//------------------------------------------------------------------------------

LRESULT CALLBACK NumberEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return SettingWindowProc(hWnd, uMsg, wParam, lParam, GuiSettingManager::m_wpOldNumberEditProc);
}

//------------------------------------------------------------------------------
