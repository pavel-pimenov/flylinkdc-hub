/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2022  Petr Kozelka, PPK at PtokaX dot org

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
#include "RegisteredUserDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/ProfileManager.h"
#include "../core/ServerManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
RegisteredUserDialog * RegisteredUserDialog::m_Ptr = nullptr;
//---------------------------------------------------------------------------
static ATOM atomRegisteredUserDialog = 0;
//---------------------------------------------------------------------------

RegisteredUserDialog::RegisteredUserDialog() : m_pRegToChange(nullptr) {
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

RegisteredUserDialog::~RegisteredUserDialog() {
	RegisteredUserDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RegisteredUserDialog::StaticRegisteredUserDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	RegisteredUserDialog * pRegisteredUserDialog = (RegisteredUserDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pRegisteredUserDialog == nullptr) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pRegisteredUserDialog->RegisteredUserDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RegisteredUserDialog::RegisteredUserDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
			if(OnAccept() == false) {
				return 0;
			}
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		case (EDT_NICK+100):
			if(HIWORD(wParam) == EN_CHANGE) {
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);

				bool bChanged = false;

				for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
					if(buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ') {
						strcpy(buf+ui16i, buf+ui16i+1);
						bChanged = true;
						ui16i--;
					}
				}

				if(bChanged == true) {
					int iStart, iEnd;

					::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

					::SetWindowText((HWND)lParam, buf);

					::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
				}

				return 0;
			}

			break;
		case EDT_PASSWORD:
			if(HIWORD(wParam) == EN_CHANGE) {
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);

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

					::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

					::SetWindowText((HWND)lParam, buf);

					::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
				}

				return 0;
			}

			break;
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
	case WM_SETFOCUS:
		if(m_pRegToChange == nullptr) {
			::SetFocus(m_hWndWindowItems[EDT_NICK]);
		} else {
			::SetFocus(m_hWndWindowItems[EDT_PASSWORD]);
		}

		return 0;
	}

	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::DoModal(HWND hWndParent, RegUser * pReg/* = nullptr*/, char * sNick/* = nullptr*/) {
	m_pRegToChange = pReg;

	if(atomRegisteredUserDialog == 0) {
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RegisteredUserDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;

		atomRegisteredUserDialog = ::RegisterClassEx(&m_wc);
	}

	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);

	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(300) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(201) / 2);

	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRegisteredUserDialog), LanguageManager::m_Ptr->m_sTexts[LAN_REGISTERED_USER],
	                                   WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(300), ScaleGui(201),
	                                   hWndParent, nullptr, ServerManager::m_hInstance, nullptr);

	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr) {
		return;
	}

	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];

	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRegisteredUserDialogProc);

	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	{
		int iHeight = (3 * GuiSettingManager::m_iOneLineGB) + GuiSettingManager::m_iEditHeight + 6;

		int iDiff = rcParent.bottom - iHeight;

		if(iDiff != 0) {
			::GetWindowRect(hWndParent, &rcParent);

			iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - ((ScaleGui(196) - iDiff) / 2);

			::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

			::SetWindowPos(m_hWndWindowItems[WINDOW_HANDLE], nullptr, iX, iY, (rcParent.right-rcParent.left), (rcParent.bottom-rcParent.top) - iDiff, SWP_NOZORDER);
		}
	}

	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	m_hWndWindowItems[GB_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                             3, 0, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[EDT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                              11, GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(EDT_NICK+100), ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_NICK], EM_SETLIMITTEXT, 64, 0);

	int iPosY = GuiSettingManager::m_iOneLineGB;

	m_hWndWindowItems[GB_PASSWORD] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PASSWORD], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                 3, iPosY, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[EDT_PASSWORD] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                  11, iPosY + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right-rcParent.left)-22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_PASSWORD, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_PASSWORD], EM_SETLIMITTEXT, 64, 0);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndWindowItems[GB_PROFILE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                3, iPosY, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[CB_PROFILE] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                11, iPosY + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right-rcParent.left)-22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_PROFILE, ServerManager::m_hInstance, nullptr);

	iPosY += GuiSettingManager::m_iOneLineGB + 4;

	m_hWndWindowItems[BTN_ACCEPT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                2, iPosY, ((rcParent.right-rcParent.left)/2)-3, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[BTN_DISCARD] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                 ((rcParent.right-rcParent.left)/2)+2, iPosY, ((rcParent.right-rcParent.left)/2)-4, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, ServerManager::m_hInstance, nullptr);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++) {
		if(m_hWndWindowItems[ui8i] == nullptr) {
			return;
		}

		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	UpdateProfiles();

	if(m_pRegToChange != nullptr) {
		::SetWindowText(m_hWndWindowItems[EDT_NICK], m_pRegToChange->m_sNick.c_str());
		::EnableWindow(m_hWndWindowItems[EDT_NICK], FALSE);

		if(m_pRegToChange->m_bPassHash == false) {
			::SetWindowText(m_hWndWindowItems[EDT_PASSWORD], m_pRegToChange->m_sPass);
		} else {
			HWND hWndTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			                                  m_hWndWindowItems[EDT_PASSWORD], nullptr, ServerManager::m_hInstance, nullptr);

			TOOLINFO ti = { 0 };
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
			ti.hwnd = m_hWndWindowItems[WINDOW_HANDLE];
			ti.uId = (UINT_PTR)m_hWndWindowItems[EDT_PASSWORD];
			ti.hinst = ServerManager::m_hInstance;
			ti.lpszText = LanguageManager::m_Ptr->m_sTexts[LAN_PASSWORD_IS_HASHED];

			::SendMessage(hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
			::SendMessage(hWndTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM(30000, 0));
		}

		::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, m_pRegToChange->m_ui16Profile, 0);
	} else if(sNick != nullptr) {
		::SetWindowText(m_hWndWindowItems[EDT_NICK], sNick);
	}

	::EnableWindow(hWndParent, FALSE);

	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::UpdateProfiles() {
	int iSel = (int)::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_GETCURSEL, 0, 0);

	for(uint16_t ui16i = 0; ui16i < ProfileManager::m_Ptr->m_ui16ProfileCount; ui16i++) {
		::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_ADDSTRING, 0, (LPARAM)ProfileManager::m_Ptr->m_ppProfilesTable[ui16i]->m_sName);
	}

	if(m_pRegToChange != nullptr) {
		::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, m_pRegToChange->m_ui16Profile, 0);
	} else {
		iSel = (int)::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, iSel, 0);

		if(iSel == CB_ERR) {
			::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, 0, 0);
		}
	}
}
//------------------------------------------------------------------------------

bool RegisteredUserDialog::OnAccept() {
	if(::GetWindowTextLength(m_hWndWindowItems[EDT_NICK]) == 0) {
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_MUST_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if(m_pRegToChange == nullptr && ::GetWindowTextLength(m_hWndWindowItems[EDT_PASSWORD]) == 0) {
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_PASS_MUST_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	char sNick[65], sPassword[65];

	sPassword[0] = '\0';

	::GetWindowText(m_hWndWindowItems[EDT_NICK], sNick, 65);
	::GetWindowText(m_hWndWindowItems[EDT_PASSWORD], sPassword, 65);

	uint16_t ui16Profile = (uint16_t)::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_GETCURSEL, 0, 0);

	if(m_pRegToChange == nullptr) {
		if(RegManager::m_Ptr->AddNew(sNick, sPassword, ui16Profile) == false) {
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_USER_IS_ALREDY_REG], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		return true;
	} else {
		RegUser * pReg = m_pRegToChange;
		m_pRegToChange = nullptr;

		RegManager::m_Ptr->ChangeReg(pReg, sPassword[0] == '\0' ? nullptr : sPassword, ui16Profile);
		return true;
	}
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::RegChanged(RegUser * pReg) {
	if(m_pRegToChange == nullptr || pReg != m_pRegToChange) {
		return;
	}

	::SetWindowText(m_hWndWindowItems[EDT_PASSWORD], m_pRegToChange->m_sPass);

	::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, m_pRegToChange->m_ui16Profile, 0);

	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_USER_CHANGED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::RegDeleted(RegUser * pReg) {
	if(m_pRegToChange == nullptr || pReg != m_pRegToChange) {
		return;
	}

	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_USER_DELETED_ACCEPT_TO_NEW], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
