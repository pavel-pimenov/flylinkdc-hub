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
#include "RegisteredUsersDialog.h"
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
#include "RegisteredUserDialog.h"
//---------------------------------------------------------------------------
RegisteredUsersDialog * RegisteredUsersDialog::m_Ptr = nullptr;
//---------------------------------------------------------------------------
#define IDC_CHANGE_REG      1000
#define IDC_REMOVE_REGS     1001
//---------------------------------------------------------------------------
static ATOM atomRegisteredUsersDialog = 0;
//---------------------------------------------------------------------------

RegisteredUsersDialog::RegisteredUsersDialog() : m_iFilterColumn(0), m_iSortColumn(0), m_bSortAscending(true) {
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

RegisteredUsersDialog::~RegisteredUsersDialog() {
	RegisteredUsersDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RegisteredUsersDialog::StaticRegisteredUsersDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	RegisteredUsersDialog * pRegisteredUsersDialog = (RegisteredUsersDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pRegisteredUsersDialog == nullptr) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pRegisteredUsersDialog->RegisteredUsersDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RegisteredUsersDialog::RegisteredUsersDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_WINDOWPOSCHANGED: {
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

		::SetWindowPos(m_hWndWindowItems[CB_FILTER], nullptr, (rcParent.right / 2)+3, (rcParent.bottom - GuiSettingManager::m_iOneLineGB - 3) + GuiSettingManager::m_iGroupBoxMargin,
		               rcParent.right - (rcParent.right/2) - 14, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[EDT_FILTER], nullptr, 11, (rcParent.bottom - GuiSettingManager::m_iOneLineGB - 3) + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[GB_FILTER], nullptr, 3, rcParent.bottom - GuiSettingManager::m_iOneLineGB - 3, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_REGS], nullptr, 0, 0, rcParent.right - 6, rcParent.bottom - GuiSettingManager::m_iOneLineGB - GuiSettingManager::m_iEditHeight - 11, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_ADD_REG], nullptr, 0, 0, rcParent.right - 4, GuiSettingManager::m_iEditHeight, SWP_NOMOVE | SWP_NOZORDER);

		return 0;
	}
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case (BTN_ADD_REG+100): {
			RegisteredUserDialog::m_Ptr = new (std::nothrow) RegisteredUserDialog();

			if(RegisteredUserDialog::m_Ptr != nullptr) {
				RegisteredUserDialog::m_Ptr->DoModal(m_hWndWindowItems[WINDOW_HANDLE]);
			}

			return 0;
		}
		case IDC_CHANGE_REG: {
			ChangeReg();
			return 0;
		}
		case IDC_REMOVE_REGS:
			RemoveRegs();
			return 0;
		case CB_FILTER:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				if(::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]) != 0) {
					FilterRegs();
				}
			}

			break;
		case IDOK: { // NM_RETURN
			HWND hWndFocus = ::GetFocus();

			if(hWndFocus == m_hWndWindowItems[LV_REGS]) {
				ChangeReg();
				return 0;
			} else if(hWndFocus == m_hWndWindowItems[EDT_FILTER]) {
				FilterRegs();
				return 0;
			}

			break;
		}
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		}

		break;
	case WM_CONTEXTMENU:
		OnContextMenu((HWND)wParam, lParam);
		break;
	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_REGS]) {
			if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK) {
				OnColumnClick((LPNMLISTVIEW)lParam);
			} else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
				if(((LPNMITEMACTIVATE)lParam)->iItem == -1) {
					break;
				}

				RegUser * pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS],  ((LPNMITEMACTIVATE)lParam)->iItem));

				RegisteredUserDialog::m_Ptr = new (std::nothrow) RegisteredUserDialog();

				if(RegisteredUserDialog::m_Ptr != nullptr) {
					RegisteredUserDialog::m_Ptr->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pReg);
				}

				return 0;
			}
		}

		break;
	case WM_GETMINMAXINFO: {
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_REGS_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_REGS_WINDOW_HEIGHT));

		return 0;
	}
	case WM_CLOSE: {
		RECT rcRegs;
		::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcRegs);

		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_REGS_WINDOW_WIDTH, rcRegs.right - rcRegs.left);
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_REGS_WINDOW_HEIGHT, rcRegs.bottom - rcRegs.top);

		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_REGS_NICK, (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETCOLUMNWIDTH, 0, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_REGS_PASSWORD, (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETCOLUMNWIDTH, 1, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_REGS_PROFILE, (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETCOLUMNWIDTH, 2, 0));

		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		ServerManager::m_hWndActiveDialog = nullptr;

		break;
	}
	case WM_NCDESTROY: {
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_SETFOCUS:
		if((UINT)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETSELECTEDCOUNT, 0, 0) != 0) {
			::SetFocus(m_hWndWindowItems[LV_REGS]);
		} else {
			::SetFocus(m_hWndWindowItems[EDT_FILTER]);
		}

		return 0;
	case WM_ACTIVATE:
		if(LOWORD(wParam) != WA_INACTIVE) {
			ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
		}

		break;
	}

	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::DoModal(HWND hWndParent) {
	if(atomRegisteredUsersDialog == 0) {
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RegisteredUsersDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;

		atomRegisteredUsersDialog = ::RegisterClassEx(&m_wc);
	}

	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);

	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_WIDTH) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_HEIGHT) / 2);

	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRegisteredUsersDialog), LanguageManager::m_Ptr->m_sTexts[LAN_REG_USERS],
	                                   WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                                   iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_HEIGHT),
	                                   hWndParent, nullptr, ServerManager::m_hInstance, nullptr);

	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr) {
		return;
	}

	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];

	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRegisteredUsersDialogProc);

	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	m_hWndWindowItems[BTN_ADD_REG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ADD_NEW_REG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                 2, 2, (rcParent.right / 3) - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_REG+100), ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[LV_REGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
	                             3, GuiSettingManager::m_iEditHeight + 6, rcParent.right - 6, rcParent.bottom - GuiSettingManager::m_iOneLineGB - GuiSettingManager::m_iEditHeight - 11, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

	m_hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_FILTER_REGISTERED_USERS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                               3, rcParent.bottom - GuiSettingManager::m_iOneLineGB - 3, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                11, (rcParent.bottom - GuiSettingManager::m_iOneLineGB - 3) + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2)-14, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);

	m_hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                               (rcParent.right / 2) + 3, (rcParent.bottom - GuiSettingManager::m_iOneLineGB - 3) + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight,
	                               m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER, ServerManager::m_hInstance, nullptr);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++) {
		if(m_hWndWindowItems[ui8i] == nullptr) {
			return;
		}

		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_NICK]);
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PASSWORD]);
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE]);

	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);

	RECT rcRegs;
	::GetClientRect(m_hWndWindowItems[LV_REGS], &rcRegs);

	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_REGS_NICK];
	lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[LAN_NICK];
	lvColumn.iSubItem = 0;

	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

	lvColumn.fmt = LVCFMT_RIGHT;
	lvColumn.cx = GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_REGS_PASSWORD];
	lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[LAN_PASSWORD];
	lvColumn.iSubItem = 1;
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);

	lvColumn.cx = GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_REGS_PROFILE];
	lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE];
	lvColumn.iSubItem = 2;
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 2, (LPARAM)&lvColumn);

	ListViewUpdateArrow(m_hWndWindowItems[LV_REGS], m_bSortAscending, m_iSortColumn);

	AddAllRegs();

	::EnableWindow(hWndParent, FALSE);

	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::AddAllRegs() {
	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEALLITEMS, 0, 0);

	RegUser * curReg = nullptr,
	          * nextReg = RegManager::m_Ptr->m_pRegListS;

	while(nextReg != nullptr) {
		curReg = nextReg;
		nextReg = curReg->m_pNext;

		AddReg(curReg);
	}

	ListViewSelectFirstItem(m_hWndWindowItems[LV_REGS]);

	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::AddReg(const RegUser * pReg) {
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ListViewGetInsertPosition(m_hWndWindowItems[LV_REGS], pReg, m_bSortAscending, CompareRegs);
	lvItem.pszText = (char*) pReg->m_sNick.c_str();
	lvItem.lParam = (LPARAM)pReg;

	int i = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);

	if(i != -1) {
		char sHexaHash[129];

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = i;
		lvItem.iSubItem = 1;

		if(pReg->m_bPassHash == true) {
			memset(&sHexaHash, 0, 129);
			for(uint8_t ui8i = 0; ui8i < 64; ui8i++) {
				sprintf(sHexaHash+(ui8i*2), "%02X", pReg->m_ui8PassHash[ui8i]);
			}
			lvItem.pszText = sHexaHash;
		} else {
			lvItem.pszText = pReg->m_sPass;
		}

		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);

		lvItem.iSubItem = 2;
		lvItem.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[pReg->m_ui16Profile]->m_sName;

		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
}
//------------------------------------------------------------------------------

int RegisteredUsersDialog::CompareRegs(const void * pItem, const void * pOtherItem) {
	const RegUser * pFirstReg = reinterpret_cast<const RegUser *>(pItem);
	const RegUser * pSecondReg = reinterpret_cast<const RegUser *>(pOtherItem);

	switch(RegisteredUsersDialog::m_Ptr->m_iSortColumn) {
	case 0:
		return _stricmp(pFirstReg->m_sNick.c_str(), pSecondReg->m_sNick.c_str());
	case 1:
		return _stricmp(pFirstReg->m_sPass, pSecondReg->m_sPass);
	case 2:
		return (pFirstReg->m_ui16Profile > pSecondReg->m_ui16Profile) ? 1 : ((pFirstReg->m_ui16Profile == pSecondReg->m_ui16Profile) ? 0 : -1);
	default:
		return 0; // never happen, but we need to make compiler/complainer happy ;o)
	}
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::OnColumnClick(const LPNMLISTVIEW pListView) {
	if(pListView->iSubItem != m_iSortColumn) {
		m_bSortAscending = true;
		m_iSortColumn = pListView->iSubItem;
	} else {
		m_bSortAscending = !m_bSortAscending;
	}

	ListViewUpdateArrow(m_hWndWindowItems[LV_REGS], m_bSortAscending, m_iSortColumn);

	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRegs);
}
//------------------------------------------------------------------------------

int CALLBACK RegisteredUsersDialog::SortCompareRegs(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/) {
	int iResult = RegisteredUsersDialog::m_Ptr->CompareRegs((void *)lParam1, (void *)lParam2);

	return (RegisteredUsersDialog::m_Ptr->m_bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::RemoveRegs() {
	if(::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
		return;
	}

	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

	RegUser * pReg = nullptr;
	int iSel = -1;

	while((iSel = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED)) != -1) {
		pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS], iSel));

		RegManager::m_Ptr->Delete(pReg, true);

		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEITEM, iSel, 0);
	}

	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::FilterRegs() {
	int iTextLength = ::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]);

	if(iTextLength == 0) {
		m_sFilterString.clear();

		AddAllRegs();
	} else {
		m_iFilterColumn = (int)::SendMessage(m_hWndWindowItems[CB_FILTER], CB_GETCURSEL, 0, 0);

		char buf[65];

		int iLen = ::GetWindowText(m_hWndWindowItems[EDT_FILTER], buf, 65);

		for(int i = 0; i < iLen; i++) {
			buf[i] = (char)tolower(buf[i]);
		}

		m_sFilterString = buf;

		::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEALLITEMS, 0, 0);

		RegUser * curReg = nullptr,
		          * nextReg = RegManager::m_Ptr->m_pRegListS;

		while(nextReg != nullptr) {
			curReg = nextReg;
			nextReg = curReg->m_pNext;

			switch(m_iFilterColumn) {
			case 0:
				if(stristr2(curReg->m_sNick.c_str(), m_sFilterString.c_str()) == nullptr) {
					continue;
				}
				break;
			case 1:
				if(stristr2(curReg->m_sPass, m_sFilterString.c_str()) == nullptr) {
					continue;
				}
				break;
			case 2:
				if(stristr2(ProfileManager::m_Ptr->m_ppProfilesTable[curReg->m_ui16Profile]->m_sName, m_sFilterString.c_str()) == nullptr) {
					continue;
				}
				break;
			}

			AddReg(curReg);
		}

		ListViewSelectFirstItem(m_hWndWindowItems[LV_REGS]);

		::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	}
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::RemoveReg(const RegUser * pReg) {
	int iPos = ListViewGetItemPosition(m_hWndWindowItems[LV_REGS], (void *)pReg);

	if(iPos != -1) {
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEITEM, iPos, 0);
	}
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::UpdateProfiles() {
	int iItemCount = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETITEMCOUNT, 0, 0);

	if(iItemCount == 0) {
		return;
	}

	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

	RegUser * pReg = nullptr;

	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 2;

	for(int i = 0; i < iItemCount; i++) {
		pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS], i));

		lvItem.iItem = i;
		lvItem.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[pReg->m_ui16Profile]->m_sName;

		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}

	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);

	if(m_iSortColumn == 2) {
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRegs);
	}
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::OnContextMenu(HWND hWindow, LPARAM lParam) {
	if(hWindow != m_hWndWindowItems[LV_REGS]) {
		return;
	}

	UINT UISelectedCount = (UINT)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETSELECTEDCOUNT, 0, 0);

	if(UISelectedCount == 0) {
		return;
	}

	HMENU hMenu = ::CreatePopupMenu();

	if(UISelectedCount == 1) {
		::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_REG, LanguageManager::m_Ptr->m_sTexts[LAN_CHANGE]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
	}

	::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_REGS, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE]);

	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);

	ListViewGetMenuPos(m_hWndWindowItems[LV_REGS], iX, iY);

	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWndWindowItems[WINDOW_HANDLE], nullptr);

	::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RegisteredUsersDialog::ChangeReg() {
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if(iSel == -1) {
		return;
	}

	RegUser * pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS], iSel));

	RegisteredUserDialog::m_Ptr = new (std::nothrow) RegisteredUserDialog();

	if(RegisteredUserDialog::m_Ptr != nullptr) {
		RegisteredUserDialog::m_Ptr->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pReg);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
