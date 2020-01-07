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
#include "ProfilesDialog.h"
//---------------------------------------------------------------------------
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
#include "LineDialog.h"
//---------------------------------------------------------------------------
ProfilesDialog * ProfilesDialog::m_Ptr = nullptr;
//---------------------------------------------------------------------------
#define IDC_RENAME_PROFILE      1000
#define IDC_REMOVE_PROFILE      1001
//---------------------------------------------------------------------------
static ATOM atomProfilesDialog = 0;
//---------------------------------------------------------------------------

ProfilesDialog::ProfilesDialog() : m_bIgnoreItemChanged(false) {
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

ProfilesDialog::~ProfilesDialog() {
	ProfilesDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK ProfilesDialog::StaticProfilesDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	ProfilesDialog * pProfilesDialog = (ProfilesDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pProfilesDialog == nullptr) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pProfilesDialog->ProfilesDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void OnNewProfileOk(char * sLine, const int /*iLen*/) {
	int32_t iRet = ProfileManager::m_Ptr->AddProfile(sLine);

	if(iRet == -1) {
		::MessageBox(ProfilesDialog::m_Ptr->m_hWndWindowItems[ProfilesDialog::WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE_NAME_EXIST], g_sPtokaXTitle, MB_OK);
	} else if(iRet == -2) {
		::MessageBox(ProfilesDialog::m_Ptr->m_hWndWindowItems[ProfilesDialog::WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_CHARS_NOT_ALLOWED_IN_PROFILE], g_sPtokaXTitle, MB_OK);
	}
}
//---------------------------------------------------------------------------

LRESULT ProfilesDialog::ProfilesDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_WINDOWPOSCHANGED: {
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

		int iProfilesWidth = rcParent.right / 3;

		int iPermissionsWidth = rcParent.right - (iProfilesWidth + 17);

		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_ALL], nullptr, iProfilesWidth + 11 + (iPermissionsWidth / 2), rcParent.bottom - GuiSettingManager::m_iEditHeight - 10,
		               rcParent.right - (iProfilesWidth + 21 + (iPermissionsWidth / 2)), GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_SET_ALL], nullptr, iProfilesWidth + 11, rcParent.bottom - GuiSettingManager::m_iEditHeight - 10, (iPermissionsWidth / 2) - 3, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_PERMISSIONS], nullptr, iProfilesWidth + 12, rcParent.top + GuiSettingManager::m_iGroupBoxMargin,
		               rcParent.right - (iProfilesWidth + 23), rcParent.bottom - GuiSettingManager::m_iGroupBoxMargin - GuiSettingManager::m_iEditHeight - 14, SWP_NOZORDER);
		::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_SETCOLUMNWIDTH, 0, iPermissionsWidth-30);
		::SetWindowPos(m_hWndWindowItems[GB_PERMISSIONS], nullptr, iProfilesWidth + 4, rcParent.top, rcParent.right - (iProfilesWidth + 7), rcParent.bottom - 3,
		               SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_MOVE_DOWN], nullptr, (iProfilesWidth / 2) + 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2,
		               iProfilesWidth - (iProfilesWidth / 2), GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_MOVE_UP], nullptr, 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (iProfilesWidth / 2) - 1, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_PROFILES], nullptr, 0, 0, iProfilesWidth - 2, rcParent.bottom - (2 * GuiSettingManager::m_iEditHeight) - 12, SWP_NOMOVE | SWP_NOZORDER);
		::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_SETCOLUMNWIDTH, 0, iProfilesWidth - 6);
		::SetWindowPos(m_hWndWindowItems[BTN_ADD_PROFILE], nullptr, 0, 0, iProfilesWidth, GuiSettingManager::m_iEditHeight, SWP_NOMOVE | SWP_NOZORDER);

		return 0;
	}
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case (BTN_ADD_PROFILE+100): {
			LineDialog * pNewProfileDlg = new (std::nothrow) LineDialog(&OnNewProfileOk);

			if(pNewProfileDlg != nullptr) {
				pNewProfileDlg->DoModal(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_NEW_PROFILE_NAME], "");
			}

			return 0;
		}
		case IDC_RENAME_PROFILE: {
			int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

			if(iSel == -1) {
				return 0;
			}

			RenameProfile(iSel);

			return 0;
		}
		case IDC_REMOVE_PROFILE: {
			int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

			if(iSel == -1 || ::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
				return 0;
			}

			if(ProfileManager::m_Ptr->RemoveProfile((uint16_t)iSel) == false) {
				::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE_DEL_FAIL], g_sPtokaXTitle, MB_OK);
			}

			return 0;
		}
		case BTN_SET_ALL:
			ChangePermissionChecks(true);
			return 0;
		case BTN_CLEAR_ALL:
			ChangePermissionChecks(false);
			return 0;
		case BTN_MOVE_UP: {
			int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

			if(iSel == -1) {
				return 0;
			}

			ProfileManager::m_Ptr->MoveProfileUp((uint16_t)iSel);

			return 0;
		}
		case BTN_MOVE_DOWN: {
			int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

			if(iSel == -1) {
				return 0;
			}

			ProfileManager::m_Ptr->MoveProfileDown((uint16_t)iSel);

			return 0;
		}
		case IDOK: { // NM_RETURN
			if(::GetFocus() == m_hWndWindowItems[LV_PROFILES]) {
				int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

				if(iSel == -1) {
					return 0;
				}

				RenameProfile(iSel);

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
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_PROFILES]) {
			if(((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
				OnProfileChanged((LPNMLISTVIEW)lParam);
			} else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
				if(((LPNMITEMACTIVATE)lParam)->iItem == -1) {
					break;
				}

				RenameProfile(((LPNMITEMACTIVATE)lParam)->iItem);

				return 0;
			}
		} else if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_PERMISSIONS]) {
			if(((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
				OnPermissionChanged((LPNMLISTVIEW)lParam);
			}
		}

		break;
	case WM_GETMINMAXINFO: {
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_PROFILES_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_PROFILES_WINDOW_HEIGHT));

		return 0;
	}
	case WM_CLOSE: {
		RECT rcProfiles;
		::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcProfiles);

		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_PROFILES_WINDOW_WIDTH, rcProfiles.right - rcProfiles.left);
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_PROFILES_WINDOW_HEIGHT, rcProfiles.bottom - rcProfiles.top);

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
		::SetFocus(m_hWndWindowItems[LV_PROFILES]);
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

void ProfilesDialog::DoModal(HWND hWndParent) {
	if(atomProfilesDialog == 0) {
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_ProfilesDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;

		atomProfilesDialog = ::RegisterClassEx(&m_wc);
	}

	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);

	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_WIDTH) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_HEIGHT) / 2);

	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomProfilesDialog), LanguageManager::m_Ptr->m_sTexts[LAN_PROFILES],
	                                   WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                                   iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_PROFILES_WINDOW_HEIGHT),
	                                   hWndParent, nullptr, ServerManager::m_hInstance, nullptr);

	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr) {
		return;
	}

	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];

	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticProfilesDialogProc);

	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	int iProfilesWidth = rcParent.right / 3;

	m_hWndWindowItems[BTN_ADD_PROFILE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ADD_NEW_PROFILE], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                     2, 2, iProfilesWidth, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_PROFILE+100), ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[LV_PROFILES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS |
	                                 LVS_SINGLESEL, 3, GuiSettingManager::m_iEditHeight + 6, iProfilesWidth - 2, rcParent.bottom - (2 * GuiSettingManager::m_iEditHeight) - 12, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	m_hWndWindowItems[BTN_MOVE_UP] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MOVE_UP], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (iProfilesWidth / 2) - 1, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_MOVE_UP, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[BTN_MOVE_DOWN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MOVE_DOWN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                   (iProfilesWidth / 2) + 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, iProfilesWidth - (iProfilesWidth / 2), GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_MOVE_DOWN, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[GB_PERMISSIONS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE_PERMISSIONS],
	                                    WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_GROUPBOX, iProfilesWidth + 4, 0, rcParent.right -(iProfilesWidth + 7), rcParent.bottom - 3,
	                                    m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[LV_PERMISSIONS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS |
	                                    LVS_SINGLESEL, iProfilesWidth + 12, GuiSettingManager::m_iGroupBoxMargin, rcParent.right - (iProfilesWidth + 23), rcParent.bottom - GuiSettingManager::m_iGroupBoxMargin - GuiSettingManager::m_iEditHeight - 14,
	                                    m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_CHECKBOXES);

	int iPermissionsWidth = rcParent.right - (iProfilesWidth + 17);

	m_hWndWindowItems[BTN_SET_ALL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SET_ALL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                 iProfilesWidth + 11, rcParent.bottom - GuiSettingManager::m_iEditHeight - 11, (iPermissionsWidth / 2) - 3, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_SET_ALL, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[BTN_CLEAR_ALL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_ALL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                   iProfilesWidth + 11 + (iPermissionsWidth / 2), rcParent.bottom - GuiSettingManager::m_iEditHeight - 11, rcParent.right - (iProfilesWidth + 21 + (iPermissionsWidth / 2)), GuiSettingManager::m_iEditHeight,
	                                   m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_ALL, ServerManager::m_hInstance, nullptr);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++) {
		if(m_hWndWindowItems[ui8i] == nullptr) {
			return;
		}

		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	RECT rcProfiles;
	::GetClientRect(m_hWndWindowItems[LV_PROFILES], &rcProfiles);

	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = rcProfiles.right-5;

	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

	RECT rcPermissions;
	::GetClientRect(m_hWndWindowItems[LV_PERMISSIONS], &rcPermissions);

	lvColumn.cx = rcPermissions.right;

	::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

	const int iPermissionsStrings[] = {
		LAN_ENT_FULL_HUB, LAN_ALLOW_LOGIN_BANNED_IP, LAN_CLOSE, LAN_DROP, LAN_KICK, LAN_GET_USER_INFO,
		LAN_REDIR, LAN_TEMPOR_BAN, LAN_TEMPOR_UNBAN, LAN_PERMANENT_BAN, LAN_UNBAN_PERM_UNBAN, LAN_CLEAR_TEMP_BANS,
		LAN_CLEAR_PERM_BANS, LAN_GET_BANS, LAN_RANGE_PERM_BAN, LAN_RANGE_UNBAN_RANGE_PERM_UNBAN, LAN_TEMP_RANGE_BAN,
		LAN_TEMP_RANGE_UNBAN, LAN_GET_RANGE_BANS, LAN_CLEAR_PERM_RANGE_BANS, LAN_CLEAR_TEMP_RANGE_BANS, LAN_GAG_UNGAG,
		LAN_TEMP_OP, LAN_START_STOP_RESTART_SCRIPTS, LAN_RESTART_HUB, LAN_RLD_TXT_FILES, LAN_NO_SHARE_LIMIT, LAN_NO_SLOT_CHECK,
		LAN_NO_RATIO_CHECK, LAN_NO_MAX_HUBS_CHECK, LAN_NO_CHAT_LIMITS, LAN_NO_TAG_CHECK, LAN_NO_MAIN_DFLD,
		LAN_NO_PM_DFLD, LAN_NO_SEARCH_LIMITS, LAN_NO_SEARCH_DFLD, LAN_NO_MYINFO_DFLD, LAN_NO_GETNICKLIST_DFLD,
		LAN_NO_CTM_DFLD, LAN_NO_RCTM_DFLD, LAN_NO_SR_DFLD, LAN_NO_RECV_DFLD, LAN_TOPIC,
		LAN_MASS_MSG, LAN_ADD_REG_USER, LAN_DEL_REG_USER, LAN_ALLOW_OPCHAT, LAN_SEND_LONG_MYINFOS,
		LAN_SEND_USER_IPS, LAN_DONT_CHECK_IP, LAN_HAVE_KEY_IS_OP, LAN_NO_CHAT_INTERVAL, LAN_NO_PM_INTERVAL,
		LAN_NO_SEARCH_INTERVAL, LAN_NO_MAX_USR_SAME_IP, LAN_NO_RECONN_TIME
	};

	const int iPermissionsIds[] = {
		ProfileManager::ENTERFULLHUB, ProfileManager::ENTERIFIPBAN, ProfileManager::CLOSE, ProfileManager::DROP, ProfileManager::KICK, ProfileManager::GETINFO,
		ProfileManager::REDIRECT, ProfileManager::TEMP_BAN, ProfileManager::TEMP_UNBAN, ProfileManager::BAN, ProfileManager::UNBAN, ProfileManager::CLRTEMPBAN,
		ProfileManager::CLRPERMBAN, ProfileManager::GETBANLIST, ProfileManager::RANGE_BAN, ProfileManager::RANGE_UNBAN, ProfileManager::RANGE_TBAN,
		ProfileManager::RANGE_TUNBAN, ProfileManager::GET_RANGE_BANS, ProfileManager::CLR_RANGE_BANS, ProfileManager::CLR_RANGE_TBANS, ProfileManager::GAG,
		ProfileManager::TEMPOP, ProfileManager::RSTSCRIPTS, ProfileManager::RSTHUB, ProfileManager::REFRESHTXT, ProfileManager::NOSHARELIMIT, ProfileManager::NOSLOTCHECK,
		ProfileManager::NOSLOTHUBRATIO, ProfileManager::NOMAXHUBCHECK, ProfileManager::NOCHATLIMITS, ProfileManager::NOTAGCHECK, ProfileManager::NODEFLOODMAINCHAT,
		ProfileManager::NODEFLOODPM, ProfileManager::NOSEARCHLIMITS, ProfileManager::NODEFLOODSEARCH, ProfileManager::NODEFLOODMYINFO, ProfileManager::NODEFLOODGETNICKLIST,
		ProfileManager::NODEFLOODCTM, ProfileManager::NODEFLOODRCTM, ProfileManager::NODEFLOODSR, ProfileManager::NODEFLOODRECV, ProfileManager::TOPIC,
		ProfileManager::MASSMSG, ProfileManager::ADDREGUSER, ProfileManager::DELREGUSER, ProfileManager::ALLOWEDOPCHAT, ProfileManager::SENDFULLMYINFOS,
		ProfileManager::SENDALLUSERIP, ProfileManager::NOIPCHECK, ProfileManager::HASKEYICON, ProfileManager::NOCHATINTERVAL, ProfileManager::NOPMINTERVAL,
		ProfileManager::NOSEARCHINTERVAL, ProfileManager::NOUSRSAMEIP, ProfileManager::NORECONNTIME
	};

	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;

	for(uint16_t ui16i = 0; ui16i < (sizeof(iPermissionsIds) / sizeof(iPermissionsIds[0])); ui16i++) {
		lvItem.iItem = ui16i;
		lvItem.lParam = (LPARAM)iPermissionsIds[ui16i];
		lvItem.pszText = LanguageManager::m_Ptr->m_sTexts[iPermissionsStrings[ui16i]];

		::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	}

	ListViewSelectFirstItem(m_hWndWindowItems[LV_PERMISSIONS]);

	AddAllProfiles();

	::EnableWindow(hWndParent, FALSE);

	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void ProfilesDialog::AddAllProfiles() {
	::SendMessage(m_hWndWindowItems[LV_PROFILES], WM_SETREDRAW, (WPARAM)FALSE, 0);

	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;

	for(uint16_t ui16i = 0; ui16i < ProfileManager::m_Ptr->m_ui16ProfileCount; ui16i++) {
		lvItem.iItem = ui16i;
		lvItem.lParam = (LPARAM)ProfileManager::m_Ptr->m_ppProfilesTable[ui16i];
		lvItem.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[ui16i]->m_sName;

		::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	}

	ListViewSelectFirstItem(m_hWndWindowItems[LV_PROFILES]);

	::SendMessage(m_hWndWindowItems[LV_PROFILES], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void ProfilesDialog::OnContextMenu(HWND hWindow, LPARAM lParam) {
	if(hWindow != m_hWndWindowItems[LV_PROFILES]) {
		return;
	}

	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if(iSel == -1) {
		return;
	}

	HMENU hMenu = ::CreatePopupMenu();

	::AppendMenu(hMenu, MF_STRING, IDC_RENAME_PROFILE, LanguageManager::m_Ptr->m_sTexts[LAN_RENAME]);
	::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
	::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_PROFILE, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE]);

	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);

	ListViewGetMenuPos(m_hWndWindowItems[LV_PROFILES], iX, iY);

	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWndWindowItems[WINDOW_HANDLE], nullptr);

	::DestroyMenu(hMenu);
}
//------------------------------------------------------------------------------

void ProfilesDialog::OnProfileChanged(const LPNMLISTVIEW pListView) {
	UpdateUpDown();

	if(pListView->iItem == -1 || (pListView->uOldState & LVIS_SELECTED) == LVIS_SELECTED || (pListView->uNewState & LVIS_SELECTED) != LVIS_SELECTED) {
		return;
	}

	m_bIgnoreItemChanged = true;

	uint16_t iItemCount = (uint16_t)::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_GETITEMCOUNT, 0, 0);

	for(uint16_t ui16i = 0; ui16i < iItemCount; ui16i++) {
		LVITEM lvItem = { 0 };
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = ui16i;

		if((BOOL)::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
			continue;
		}

		lvItem.mask = LVIF_STATE;
		lvItem.state = INDEXTOSTATEIMAGEMASK(ProfileManager::m_Ptr->m_ppProfilesTable[pListView->iItem]->m_bPermissions[(int)lvItem.lParam] == true ? 2 : 1);
		lvItem.stateMask = LVIS_STATEIMAGEMASK;

		::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_SETITEMSTATE, ui16i, (LPARAM)&lvItem);
	}

	m_bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void ProfilesDialog::ChangePermissionChecks(const bool bCheck) {
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if(iSel == -1) {
		return;
	}

	m_bIgnoreItemChanged = true;

	uint16_t iItemCount = (uint16_t)::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_GETITEMCOUNT, 0, 0);

	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_STATE;
	lvItem.stateMask = LVIS_STATEIMAGEMASK;

	for(uint16_t ui16i = 0; ui16i < iItemCount; ui16i++) {
		ProfileManager::m_Ptr->m_ppProfilesTable[iSel]->m_bPermissions[ui16i] = bCheck;

		lvItem.iItem = ui16i;
		lvItem.state = INDEXTOSTATEIMAGEMASK(bCheck == true ? 2 : 1);

		::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_SETITEMSTATE, ui16i, (LPARAM)&lvItem);
	}

	m_bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void OnRenameProfileOk(char * sLine, const int iLen) {
	int iSel = (int)::SendMessage(ProfilesDialog::m_Ptr->m_hWndWindowItems[ProfilesDialog::LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if(iSel == -1) {
		return;
	}

	ProfileManager::m_Ptr->ChangeProfileName((uint16_t)iSel, sLine, iLen);

	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iSel;
	lvItem.pszText = sLine;

	::SendMessage(ProfilesDialog::m_Ptr->m_hWndWindowItems[ProfilesDialog::LV_PROFILES], LVM_SETITEMTEXT, iSel, (LPARAM)&lvItem);
}
//---------------------------------------------------------------------------

void ProfilesDialog::RenameProfile(const int iProfile) {
	LineDialog * pRenameProfileDlg = new (std::nothrow) LineDialog(&OnRenameProfileOk);

	if(pRenameProfileDlg != nullptr) {
		pRenameProfileDlg->DoModal(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_NEW_PROFILE_NAME], ProfileManager::m_Ptr->m_ppProfilesTable[iProfile]->m_sName);
	}
}
//------------------------------------------------------------------------------

void ProfilesDialog::RemoveProfile(const uint16_t iProfile) {
	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_DELETEITEM, iProfile, 0);

	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if(iSel == -1) {
		LVITEM lvItem = { 0 };
		lvItem.mask = LVIF_STATE;
		lvItem.state = LVIS_SELECTED;
		lvItem.stateMask = LVIS_SELECTED;

		::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);
	}

	UpdateUpDown();
}
//------------------------------------------------------------------------------

void ProfilesDialog::UpdateUpDown() {
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if(iSel == -1) {
		::EnableWindow(m_hWndWindowItems[BTN_MOVE_UP], FALSE);
		::EnableWindow(m_hWndWindowItems[BTN_MOVE_DOWN], FALSE);
	} else if(iSel == 0) {
		::EnableWindow(m_hWndWindowItems[BTN_MOVE_UP], FALSE);
		if(iSel == (ProfileManager::m_Ptr->m_ui16ProfileCount-1)) {
			::EnableWindow(m_hWndWindowItems[BTN_MOVE_DOWN], FALSE);
		} else {
			::EnableWindow(m_hWndWindowItems[BTN_MOVE_DOWN], TRUE);
		}
	} else if(iSel == (ProfileManager::m_Ptr->m_ui16ProfileCount-1)) {
		::EnableWindow(m_hWndWindowItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(m_hWndWindowItems[BTN_MOVE_DOWN], FALSE);
	} else {
		::EnableWindow(m_hWndWindowItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(m_hWndWindowItems[BTN_MOVE_DOWN], TRUE);
	}
}
//------------------------------------------------------------------------------

void ProfilesDialog::AddProfile() {
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ProfileManager::m_Ptr->m_ui16ProfileCount-1;
	lvItem.lParam = (LPARAM)ProfileManager::m_Ptr->m_ppProfilesTable[ProfileManager::m_Ptr->m_ui16ProfileCount-1];
	lvItem.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[ProfileManager::m_Ptr->m_ui16ProfileCount-1]->m_sName;

	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_INSERTITEM, 0, (LPARAM)&lvItem);

	UpdateUpDown();
}
//------------------------------------------------------------------------------

void ProfilesDialog::MoveDown(const uint16_t iProfile) {
	HWND hWndFocus = ::GetFocus();

	LVITEM lvItem1 = { 0 };
	lvItem1.mask = LVIF_PARAM | LVIF_STATE;
	lvItem1.iItem = iProfile;
	lvItem1.stateMask = LVIS_SELECTED;

	if((BOOL)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem1) == FALSE) {
		return;
	}

	LVITEM lvItem2 = { 0 };
	lvItem2.mask = LVIF_PARAM | LVIF_STATE;
	lvItem2.iItem = iProfile+1;
	lvItem2.stateMask = LVIS_SELECTED;

	if((BOOL)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem2) == FALSE) {
		return;
	}

	lvItem1.mask |= LVIF_TEXT;
	lvItem1.iItem++;
	lvItem1.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[lvItem1.iItem]->m_sName;

	lvItem2.mask |= LVIF_TEXT;
	lvItem2.iItem--;
	lvItem2.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[lvItem2.iItem]->m_sName;

	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem1);
	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem2);

	if(iProfile == ProfileManager::m_Ptr->m_ui16ProfileCount-2 && hWndFocus == m_hWndWindowItems[BTN_MOVE_DOWN]) {
		::SetFocus(m_hWndWindowItems[BTN_MOVE_UP]);
	} else if(hWndFocus == m_hWndWindowItems[BTN_MOVE_DOWN] || hWndFocus == m_hWndWindowItems[BTN_MOVE_UP]) {
		::SetFocus(hWndFocus);
	}
}
//------------------------------------------------------------------------------

void ProfilesDialog::MoveUp(const uint16_t iProfile) {
	HWND hWndFocus = ::GetFocus();

	LVITEM lvItem1 = { 0 };
	lvItem1.mask = LVIF_PARAM | LVIF_STATE;
	lvItem1.iItem = iProfile;
	lvItem1.stateMask = LVIS_SELECTED;

	if((BOOL)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem1) == FALSE) {
		return;
	}

	LVITEM lvItem2 = { 0 };
	lvItem2.mask = LVIF_PARAM | LVIF_STATE;
	lvItem2.iItem = iProfile-1;
	lvItem2.stateMask = LVIS_SELECTED;

	if((BOOL)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETITEM, 0, (LPARAM)&lvItem2) == FALSE) {
		return;
	}

	lvItem1.mask |= LVIF_TEXT;
	lvItem1.iItem--;
	lvItem1.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[lvItem1.iItem]->m_sName;

	lvItem2.mask |= LVIF_TEXT;
	lvItem2.iItem++;
	lvItem2.pszText = ProfileManager::m_Ptr->m_ppProfilesTable[lvItem2.iItem]->m_sName;

	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem1);
	::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_SETITEM, 0, (LPARAM)&lvItem2);

	if(iProfile == 1 && hWndFocus == m_hWndWindowItems[BTN_MOVE_UP]) {
		::SetFocus(m_hWndWindowItems[BTN_MOVE_DOWN]);
	} else if(hWndFocus == m_hWndWindowItems[BTN_MOVE_DOWN] || hWndFocus == m_hWndWindowItems[BTN_MOVE_UP]) {
		::SetFocus(hWndFocus);
	}
}
//------------------------------------------------------------------------------

void ProfilesDialog::OnPermissionChanged(const LPNMLISTVIEW pListView) {
	if(m_bIgnoreItemChanged == true || pListView->iItem == -1 || (pListView->uNewState & LVIS_STATEIMAGEMASK) == (pListView->uOldState & LVIS_STATEIMAGEMASK)) {
		return;
	}

	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_PROFILES], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if(iSel == -1) {
		return;
	}

	bool bEnabled = ((((pListView->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) == 0 ? false : true);

	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = pListView->iItem;

	if((BOOL)::SendMessage(m_hWndWindowItems[LV_PERMISSIONS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
		return;
	}

	ProfileManager::m_Ptr->m_ppProfilesTable[iSel]->m_bPermissions[(int)lvItem.lParam] = bEnabled;
}
//------------------------------------------------------------------------------
