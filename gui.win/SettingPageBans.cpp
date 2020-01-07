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
#include "SettingPageBans.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------

SettingPageBans::SettingPageBans() : m_bUpdateTempBanRedirAddress(false), m_bUpdatePermBanRedirAddress(false) {
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageBans::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == WM_COMMAND) {
		switch(LOWORD(wParam)) {
		case EDT_DEFAULT_TEMPBAN_TIME:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 1, 32767);

				return 0;
			}

			break;
		case EDT_ADD_MESSAGE:
		case EDT_TEMP_BAN_REDIR:
		case EDT_PERM_BAN_REDIR:
			if(HIWORD(wParam) == EN_CHANGE) {
				RemovePipes((HWND)lParam);

				return 0;
			}

			break;
		case BTN_TEMP_BAN_REDIR_ENABLE:
			if(HIWORD(wParam) == BN_CLICKED) {
				::EnableWindow(m_hWndPageItems[EDT_TEMP_BAN_REDIR],
				               ::SendMessage(m_hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}

			break;
		case BTN_PERM_BAN_REDIR_ENABLE:
			if(HIWORD(wParam) == BN_CLICKED) {
				::EnableWindow(m_hWndPageItems[EDT_PERM_BAN_REDIR],
				               ::SendMessage(m_hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}

			break;
		case CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[LBL_TEMP_BAN_TIME], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(m_hWndPageItems[EDT_TEMP_BAN_TIME], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(m_hWndPageItems[UD_TEMP_BAN_TIME], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(m_hWndPageItems[LBL_TEMP_BAN_TIME_HOURS], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(m_hWndPageItems[BTN_REPORT_3X_BAD_PASS], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case EDT_TEMP_BAN_TIME:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 1, 999);

				return 0;
			}

			break;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageBans::Save() {
	if(m_bCreated == false) {
		return;
	}

	LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_DEFAULT_TEMPBAN_TIME], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_DEFAULT_TEMP_BAN_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_BAN_MSG_SHOW_IP, ::SendMessage(m_hWndPageItems[BTN_SHOW_IP], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_BAN_MSG_SHOW_RANGE, ::SendMessage(m_hWndPageItems[BTN_SHOW_RANGE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_BAN_MSG_SHOW_NICK, ::SendMessage(m_hWndPageItems[BTN_SHOW_NICK], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_BAN_MSG_SHOW_REASON, ::SendMessage(m_hWndPageItems[BTN_SHOW_REASON], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_BAN_MSG_SHOW_BY, ::SendMessage(m_hWndPageItems[BTN_SHOW_BY], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	char buf[257];
	int iLen = ::GetWindowText(m_hWndPageItems[EDT_ADD_MESSAGE], buf, 257);
	SettingManager::m_Ptr->SetText(SETTXT_MSG_TO_ADD_TO_BAN_MSG, buf, iLen);

	SettingManager::m_Ptr->SetBool(SETBOOL_TEMP_BAN_REDIR, ::SendMessage(m_hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_TEMP_BAN_REDIR], buf, 257);

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS]) != 0)) {
		m_bUpdateTempBanRedirAddress = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_TEMP_BAN_REDIR_ADDRESS, buf, iLen);

	SettingManager::m_Ptr->SetBool(SETBOOL_PERM_BAN_REDIR, ::SendMessage(m_hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_PERM_BAN_REDIR], buf, 257);

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS]) != 0)) {
		m_bUpdatePermBanRedirAddress = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_PERM_BAN_REDIR_ADDRESS, buf, iLen);

	SettingManager::m_Ptr->SetBool(SETBOOL_ADVANCED_PASS_PROTECTION,
	                               ::SendMessage(m_hWndPageItems[BTN_ENABLE_ADVANCED_PASSWORD_PROTECTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	SettingManager::m_Ptr->SetShort(SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE, (int16_t)::SendMessage(m_hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_TEMP_BAN_TIME], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_REPORT_3X_BAD_PASS, ::SendMessage(m_hWndPageItems[BTN_REPORT_3X_BAD_PASS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageBans::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                 bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                 bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                 bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                 bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & bUpdatedTempBanRedirAddress,
                                 bool & bUpdatedPermBanRedirAddress, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
	if(bUpdatedTempBanRedirAddress == false) {
		bUpdatedTempBanRedirAddress = m_bUpdateTempBanRedirAddress;
	}
	if(bUpdatedPermBanRedirAddress == false) {
		bUpdatedPermBanRedirAddress = m_bUpdatePermBanRedirAddress;
	}
}

//------------------------------------------------------------------------------
bool SettingPageBans::CreateSettingPage(HWND hOwner) {
	CreateHWND(hOwner);

	if(m_bCreated == false) {
		return false;
	}

	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);

	m_hWndPageItems[GB_DEFAULT_TEMPBAN_TIME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_TIME_AFTER_ETC], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	        0, 0, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_DEFAULT_TEMPBAN_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        8, GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT / 2, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_DEFAULT_TEMPBAN_TIME, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_DEFAULT_TEMPBAN_TIME], EM_SETLIMITTEXT, 5, 0);

	AddUpDown(m_hWndPageItems[UD_DEFAULT_TEMPBAN_TIME], (m_iFullEDT / 2) + 8, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 1),
	          (WPARAM)m_hWndPageItems[EDT_DEFAULT_TEMPBAN_TIME], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME], 0));

	m_hWndPageItems[LBL_DEFAULT_TEMPBAN_TIME_MINUTES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MINUTES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        (m_iFullEDT / 2) + GuiSettingManager::m_iUpDownWidth + 13, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), (rcThis.right - rcThis.left) - ((m_iFullEDT / 2) + GuiSettingManager::m_iUpDownWidth + 18), GuiSettingManager::m_iTextHeight,
	        m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	int iPosY = GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_BAN_MESSAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_BAN_MSG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                  0, iPosY, m_iFullGB, GuiSettingManager::m_iGroupBoxMargin + (5 * GuiSettingManager::m_iCheckHeight) + 13 + GuiSettingManager::m_iOneLineGB + 5, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_SHOW_IP] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SHOW_BANNED_IP], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                               8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SHOW_IP], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_SHOW_RANGE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SHOW_BANNED_RANGE], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                  8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SHOW_RANGE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_RANGE] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_SHOW_NICK] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SHOW_BANNED_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                 8, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 6, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SHOW_NICK], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_NICK] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_SHOW_REASON] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SHOW_BAN_REASON], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                   8, iPosY + GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iCheckHeight) + 9, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SHOW_REASON], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_REASON] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_SHOW_BY] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SHOW_BAN_WHO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                               8, iPosY + GuiSettingManager::m_iGroupBoxMargin + (4 * GuiSettingManager::m_iCheckHeight) + 12, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SHOW_BY], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_BY] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[GB_ADD_MESSAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_BAN_MSG_ADD_MSG], WS_CHILD | WS_VISIBLE |
	                                  BS_GROUPBOX, 5, iPosY + GuiSettingManager::m_iGroupBoxMargin + (5 * GuiSettingManager::m_iCheckHeight) + 13, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_ADD_MESSAGE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG], WS_CHILD | WS_VISIBLE |
	                                   WS_TABSTOP | ES_AUTOHSCROLL, 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + (5 * GuiSettingManager::m_iCheckHeight) + 13 + GuiSettingManager::m_iGroupBoxMargin, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_ADD_MESSAGE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_ADD_MESSAGE], EM_SETLIMITTEXT, 256, 0);

	iPosY += GuiSettingManager::m_iGroupBoxMargin + (5 * GuiSettingManager::m_iCheckHeight) + 13 + GuiSettingManager::m_iOneLineGB + 5;

	m_hWndPageItems[GB_TEMP_BAN_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_REDIR_ADDRESS], WS_CHILD |
	                                     WS_VISIBLE | BS_GROUPBOX, 0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), ScaleGui(85), GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_TEMP_BAN_REDIR_ENABLE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_TEMP_BAN_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[EDT_TEMP_BAN_REDIR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
	                                      WS_TABSTOP | ES_AUTOHSCROLL, ScaleGui(85) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(85) + 26), GuiSettingManager::m_iEditHeight,
	                                      m_hWnd, (HMENU)EDT_TEMP_BAN_REDIR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_TEMP_BAN_REDIR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_TEMP_BAN_REDIR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_PERM_BAN_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN_REDIR_ADDRESS], WS_CHILD |
	                                     WS_VISIBLE | BS_GROUPBOX, 0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), ScaleGui(85), GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_PERM_BAN_REDIR_ENABLE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_PERM_BAN_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[EDT_PERM_BAN_REDIR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
	                                      WS_TABSTOP | ES_AUTOHSCROLL, ScaleGui(85) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(85) + 26), GuiSettingManager::m_iEditHeight,
	                                      m_hWnd, (HMENU)EDT_PERM_BAN_REDIR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_PERM_BAN_REDIR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_PERM_BAN_REDIR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_PASSWORD_PROTECTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PASSWORD_PROTECTION],
	        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, iPosY, m_iFullGB, (2 * GuiSettingManager::m_iGroupBoxMargin) + (2 * GuiSettingManager::m_iCheckHeight) + 10 + (2 * GuiSettingManager::m_iEditHeight) + 13, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_ENABLE_ADVANCED_PASSWORD_PROTECTION] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ADV_PASS_PRTCTN], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	        BS_AUTOCHECKBOX, 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_ENABLE_ADVANCED_PASSWORD_PROTECTION], BM_SETCHECK,
	              (SettingManager::m_Ptr->m_bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[GB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_BRUTE_FORCE_PASS_PRTC_ACT],
	        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGB, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + GuiSettingManager::m_iCheckHeight + 17, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE], 0);

	m_hWndPageItems[LBL_TEMP_BAN_TIME] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_TEMPORARY_BAN_TIME], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                     13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 6 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(225), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                     ScaleGui(225) + 18, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 6, ScaleGui(80), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_TEMP_BAN_TIME, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_TEMP_BAN_TIME], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_TEMP_BAN_TIME], ScaleGui(225) + 18 + ScaleGui(80), iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 6, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight,
	          (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_TEMP_BAN_TIME], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME], 0));

	m_hWndPageItems[LBL_TEMP_BAN_TIME_HOURS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_HOURS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        ScaleGui(225) + 18 + ScaleGui(80) + GuiSettingManager::m_iUpDownWidth + 5, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 6 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        (rcThis.right - rcThis.left) - (ScaleGui(225) + 18 + ScaleGui(80) + GuiSettingManager::m_iUpDownWidth + 18), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_REPORT_3X_BAD_PASS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REPORT_BAD_PASS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + (2 * GuiSettingManager::m_iEditHeight) + 10, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_REPORT_3X_BAD_PASS], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REPORT_3X_BAD_PASS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
		if(m_hWndPageItems[ui8i] == nullptr) {
			return false;
		}

		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	::EnableWindow(m_hWndPageItems[EDT_TEMP_BAN_REDIR], SettingManager::m_Ptr->m_bBools[SETBOOL_TEMP_BAN_REDIR] == true ? TRUE : FALSE);

	::EnableWindow(m_hWndPageItems[EDT_PERM_BAN_REDIR], SettingManager::m_Ptr->m_bBools[SETBOOL_PERM_BAN_REDIR] == true ? TRUE : FALSE);

	::EnableWindow(m_hWndPageItems[LBL_TEMP_BAN_TIME], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[EDT_TEMP_BAN_TIME], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[UD_TEMP_BAN_TIME], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[LBL_TEMP_BAN_TIME_HOURS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[BTN_REPORT_3X_BAD_PASS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 0 ? FALSE : TRUE);

	GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_REPORT_3X_BAD_PASS], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageBans::GetPageName() {
	return LanguageManager::m_Ptr->m_sTexts[LAN_BAN_HANDLING];
}
//------------------------------------------------------------------------------

void SettingPageBans::FocusLastItem() {
	::SetFocus(m_hWndPageItems[BTN_REPORT_3X_BAD_PASS]);
}
//------------------------------------------------------------------------------
