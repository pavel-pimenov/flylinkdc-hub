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
#include "LineDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "Resources.h"
//---------------------------------------------------------------------------
static ATOM atomLineDialog = 0;
//---------------------------------------------------------------------------
void (*pOnOk)(char * sLine, const int iLen) = nullptr;
//---------------------------------------------------------------------------

LineDialog::LineDialog(void (*pOnOkFunction)(char * sLine, const int iLen)) {
    memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));

	pOnOk = pOnOkFunction;
}
//---------------------------------------------------------------------------

LineDialog::~LineDialog() {
    pOnOk = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK LineDialog::StaticLineDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LineDialog * pLineDialog = (LineDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pLineDialog == nullptr) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pLineDialog->LineDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT LineDialog::LineDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_SETFOCUS:
            ::SetFocus(m_hWndWindowItems[EDT_LINE]);

            return 0;
        case WM_COMMAND:
           switch(LOWORD(wParam)) {
                case IDOK: {
                    int iLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_LINE]);
                    if(iLen != 0) {
                        char * sBuf = new (std::nothrow) char[iLen + 1];

                        if(sBuf != nullptr) {
                            ::GetWindowText(m_hWndWindowItems[EDT_LINE], sBuf, iLen + 1);
                            (*pOnOk)(sBuf, iLen);
                        }

                        delete [] sBuf;
                    }
                }
                case IDCANCEL:
                    ::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
					return 0;
            }

            break;
        case WM_CLOSE:
            ::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
            ServerManager::m_hWndActiveDialog = nullptr;
            break;
        case WM_NCDESTROY: {
            HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
            delete this;
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void LineDialog::DoModal(HWND hWndParent, char * sCaption, char * sLine) {
    if(atomLineDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_LineDialog";
        m_wc.hInstance = ServerManager::m_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomLineDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(306) / 2);
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(105) / 2);

	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomLineDialog), (string(sCaption) + ":").c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX, iY, ScaleGui(306), ScaleGui(105), hWndParent, nullptr, ServerManager::m_hInstance, nullptr);

    if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr) {
        return;
    }

    ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];

    ::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticLineDialogProc);

    {
        ::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

        int iHeight = GuiSettingManager::m_iOneLineGB + GuiSettingManager::m_iEditHeight + 6;

        int iDiff = rcParent.bottom - iHeight;

        if(iDiff != 0) {
            ::GetWindowRect(hWndParent, &rcParent);

            iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - ((ScaleGui(100) - iDiff) / 2);

            ::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

            ::SetWindowPos(m_hWndWindowItems[WINDOW_HANDLE], nullptr, iX, iY, (rcParent.right-rcParent.left), (rcParent.bottom-rcParent.top) - iDiff, SWP_NOZORDER);
        }
    }

    ::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	m_hWndWindowItems[GB_LINE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 0, rcParent.right - 10, GuiSettingManager::m_iOneLineGB,
		m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[EDT_LINE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, sLine, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 26, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndWindowItems[EDT_LINE], EM_SETSEL, 0, -1);

	m_hWndWindowItems[BTN_OK] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        4, GuiSettingManager::m_iOneLineGB + 4, (rcParent.right / 2) - 5, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[BTN_CANCEL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        rcParent.right - ((rcParent.right / 2) - 1), GuiSettingManager::m_iOneLineGB + 4, (rcParent.right / 2) - 5, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, ServerManager::m_hInstance, nullptr);

    for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++) {
        ::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//---------------------------------------------------------------------------
