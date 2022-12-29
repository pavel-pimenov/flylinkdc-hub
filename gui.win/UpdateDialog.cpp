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
#include "../core/LuaInc.h"
//---------------------------------------------------------------------------
#include "UpdateDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
UpdateDialog * UpdateDialog::m_Ptr = nullptr;
//---------------------------------------------------------------------------
static ATOM atomUpdateDialog = 0;
//---------------------------------------------------------------------------

UpdateDialog::UpdateDialog() {
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

UpdateDialog::~UpdateDialog() {
	UpdateDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK UpdateDialog::StaticUpdateDialogProc(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return UpdateDialog::m_Ptr->UpdateDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT UpdateDialog::UpdateDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[REDT_UPDATE] && ((LPNMHDR)lParam)->code == EN_LINK) {
			if(((ENLINK *)lParam)->msg == WM_LBUTTONUP) {
				RichEditOpenLink(m_hWndWindowItems[REDT_UPDATE], (ENLINK *)lParam);
			}
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
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		}

		break;
	case WM_SETFOCUS:
		::SetFocus(m_hWndWindowItems[REDT_UPDATE]);
		return 0;
	}

	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void UpdateDialog::DoModal(HWND hWndParent) {
	if(atomUpdateDialog == 0) {
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_UpdateDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;

		atomUpdateDialog = ::RegisterClassEx(&m_wc);
	}

	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);

	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(500) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(460) / 2);

	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomUpdateDialog), LanguageManager::m_Ptr->m_sTexts[LAN_CHECKING_FOR_UPDATE],
	                                   WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(500), ScaleGui(460),
	                                   hWndParent, nullptr, ServerManager::m_hInstance, nullptr);

	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr) {
		return;
	}

	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];

	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticUpdateDialogProc);

	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	m_hWndWindowItems[REDT_UPDATE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
	                                 5, 5, rcParent.right - 10, rcParent.bottom - 10, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[REDT_UPDATE], EM_SETBKGNDCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
	::SendMessage(m_hWndWindowItems[REDT_UPDATE], EM_AUTOURLDETECT, TRUE, 0);
	::SendMessage(m_hWndWindowItems[REDT_UPDATE], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(m_hWndWindowItems[REDT_UPDATE], EM_GETEVENTMASK, 0, 0) | ENM_LINK);
	::SendMessage(m_hWndWindowItems[REDT_UPDATE], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));

	::EnableWindow(hWndParent, FALSE);

	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//---------------------------------------------------------------------------

void UpdateDialog::Message(char * sData) {
	RichEditAppendText(m_hWndWindowItems[REDT_UPDATE], sData);
}
//---------------------------------------------------------------------------
bool UpdateDialog::ParseData(char * sData, HWND hWndParent) {
	char * sVersion = nullptr;
	char * sBuildNumber = nullptr;
	char * sReleaseDate = nullptr;
	char * sChanges = nullptr;

	char * sBegin = sData;
	char * sTemp = strchr(sBegin, '=');

	size_t szLen = 0;

	while(sTemp != nullptr) {
		sTemp[0] = '\0';
		szLen = sTemp - sBegin;

		if(szLen == 7) {
			if(strcmp(sBegin, "Version") == 0) {
				sVersion = sTemp + 1;
			} else if(strcmp(sBegin, "Release") == 0) {
				sReleaseDate = sTemp + 1;
			} else if(strcmp(sBegin, "Changes") == 0) {
				sChanges = sTemp + 1;

				sTemp = strstr(sTemp + 1, "TestingVersion=");
				if(sTemp != nullptr) {
					sTemp[0] = '\0';
				}

				break;
			}
		} else if(szLen == 5 && strcmp(sBegin, "Build") == 0) {
			sBuildNumber = sTemp + 1;
		}

		sTemp = strchr(sTemp + 1, '\n');
		if(sTemp == nullptr) {
			break;
		}

		sTemp[0] = '\0';

		sBegin = sTemp + 1;

		sTemp--;

		if(sTemp[0] == '\r') {
			sTemp[0] = '\0';
		}

		sTemp = strchr(sBegin, '=');
	}

	if(sVersion == nullptr || sBuildNumber == nullptr || sReleaseDate == nullptr || sChanges == nullptr) {
		RichEditAppendText(m_hWndWindowItems[REDT_UPDATE], LanguageManager::m_Ptr->m_sTexts[LAN_UPDATE_CHECK_FAILED]);
		return false;
	}

	uint64_t ui64BuildNumber = _strtoui64(BUILD_NUMBER, nullptr, 10);
	uint64_t ui64NewBuildNumber = _strtoui64(sBuildNumber, nullptr, 10);

	if(ui64NewBuildNumber > ui64BuildNumber) {
		string sMsg = string(LanguageManager::m_Ptr->m_sTexts[LAN_NEW_VERSION], LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NEW_VERSION]) + " " + sVersion + " [build: " + sBuildNumber + "] " +
		              LanguageManager::m_Ptr->m_sTexts[LAN_RELEASED_ON] + " " + sReleaseDate + " " + LanguageManager::m_Ptr->m_sTexts[LAN_IS_AVAILABLE] + ".\r\n\r\n" + LanguageManager::m_Ptr->m_sTexts[LAN_CHANGES] + ":\r\n" +
		              sChanges;

		if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr) {
			DoModal(hWndParent);
			RichEditAppendText(m_hWndWindowItems[REDT_UPDATE], sMsg.c_str(), false);
		} else {
			RichEditAppendText(m_hWndWindowItems[REDT_UPDATE], sMsg.c_str());
		}

		return true;
	}

	if(m_hWndWindowItems[WINDOW_HANDLE] != nullptr) {
		RichEditAppendText(m_hWndWindowItems[REDT_UPDATE], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_NO_NEW_VERSION_AVAILABLE]);
	}

	return false;
}
//---------------------------------------------------------------------------
