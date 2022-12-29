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
#include "SettingPageMyINFO.h"
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

SettingPageMyINFO::SettingPageMyINFO() : m_bUpdateNoTagMessage(false) {
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageMyINFO::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == WM_COMMAND) {
		switch(LOWORD(wParam)) {
		case CB_NO_TAG_ACTION:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32NoTagAction = (uint32_t)::SendMessage(m_hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_NO_TAG_MESSAGE], ui32NoTagAction != 0 ? TRUE : FALSE);
				::EnableWindow(m_hWndPageItems[EDT_NO_TAG_REDIRECT], ui32NoTagAction == 2 ? TRUE : FALSE);
				::EnableWindow(m_hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], ui32NoTagAction == 0 ? TRUE : FALSE);
			}

			break;
		case EDT_NO_TAG_MESSAGE:
		case EDT_NO_TAG_REDIRECT:
			if(HIWORD(wParam) == EN_CHANGE) {
				RemovePipes((HWND)lParam);

				return 0;
			}

			break;
		case EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 0, 999);

				return 0;
			}

			break;
		case CB_ORIGINAL_MYINFO_ACTION:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				BOOL bEnable = ((uint32_t)::SendMessage(m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_GETCURSEL, 0, 0) == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[BTN_REMOVE_DESCRIPTION], bEnable);
				::EnableWindow(m_hWndPageItems[BTN_REMOVE_TAG], bEnable);
				::EnableWindow(m_hWndPageItems[BTN_REMOVE_CONNECTION], bEnable);
				::EnableWindow(m_hWndPageItems[BTN_REMOVE_EMAIL], bEnable);
				::EnableWindow(m_hWndPageItems[BTN_MODE_TO_MYINFO], bEnable);
				::EnableWindow(m_hWndPageItems[BTN_MODE_TO_DESCRIPTION], bEnable);
			}

			break;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::Save() {
	if(m_bCreated == false) {
		return;
	}

	char buf[257];
	int iLen = ::GetWindowText(m_hWndPageItems[EDT_NO_TAG_MESSAGE], buf, 257);

	if((int16_t)::SendMessage(m_hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_NO_TAG_OPTION] ||
	        strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_NO_TAG_MSG]) != 0)
	{
		m_bUpdateNoTagMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_NO_TAG_MSG, buf, iLen);

	SettingManager::m_Ptr->SetShort(SETSHORT_NO_TAG_OPTION, (int16_t)::SendMessage(m_hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0));

	iLen = ::GetWindowText(m_hWndPageItems[EDT_NO_TAG_REDIRECT], buf, 257);

	if(m_bUpdateNoTagMessage == false && ((SettingManager::m_Ptr->m_sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	                                      (SettingManager::m_Ptr->m_sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != nullptr &&
	                                       strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_NO_TAG_REDIR_ADDRESS]) != 0)))
	{
		m_bUpdateNoTagMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_NO_TAG_REDIR_ADDRESS, buf, iLen);

	SettingManager::m_Ptr->SetBool(SETBOOL_REPORT_SUSPICIOUS_TAG, ::SendMessage(m_hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MYINFO_DELAY, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_FULL_MYINFO_OPTION, (int16_t)::SendMessage(m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_GETCURSEL, 0, 0));

	SettingManager::m_Ptr->SetBool(SETBOOL_STRIP_DESCRIPTION, ::SendMessage(m_hWndPageItems[BTN_REMOVE_DESCRIPTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_STRIP_TAG, ::SendMessage(m_hWndPageItems[BTN_REMOVE_TAG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_STRIP_CONNECTION, ::SendMessage(m_hWndPageItems[BTN_REMOVE_CONNECTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_STRIP_EMAIL, ::SendMessage(m_hWndPageItems[BTN_REMOVE_EMAIL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_MODE_TO_MYINFO, ::SendMessage(m_hWndPageItems[BTN_MODE_TO_MYINFO], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_MODE_TO_DESCRIPTION, ::SendMessage(m_hWndPageItems[BTN_MODE_TO_DESCRIPTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                   bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                   bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & bUpdatedNoTagMessage,
                                   bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                   bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                   bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
	if(bUpdatedNoTagMessage == false) {
		bUpdatedNoTagMessage = m_bUpdateNoTagMessage;
	}
}

//------------------------------------------------------------------------------
bool SettingPageMyINFO::CreateSettingPage(HWND hOwner) {
	CreateHWND(hOwner);

	if(m_bCreated == false) {
		return false;
	}

	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);

	int iGBinGBinGB = (rcThis.right - rcThis.left) - 25;
	int iGBinGBinGBEDT = (rcThis.right - rcThis.left) - 41;

	int iPosY = (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iEditHeight + (2 * GuiSettingManager::m_iOneLineGB) + 12;

	m_hWndPageItems[GB_DESCRIPTION_TAG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION_TAG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                      0, 0, m_iFullGB, iPosY, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REPORT_BAD_TAGS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        8, GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REPORT_SUSPICIOUS_TAG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[GB_NO_TAG_ACTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_NO_TAG_ACTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                    5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGB, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + (2 * GuiSettingManager::m_iOneLineGB) + 7, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_NO_TAG_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                    13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_NO_TAG_ACTION, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_ACCEPT]);
	::SendMessage(m_hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_REJECT]);
	::SendMessage(m_hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_REDIR]);
	::SendMessage(m_hWndPageItems[CB_NO_TAG_ACTION], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_NO_TAG_OPTION], 0);

	m_hWndPageItems[GB_NO_TAG_MESSAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                     10, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 3, iGBinGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_NO_TAG_MESSAGE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_NO_TAG_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                      18, (3 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 3, iGBinGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_NO_TAG_MESSAGE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_NO_TAG_MESSAGE], EM_SETLIMITTEXT, 256, 0);

	m_hWndPageItems[GB_NO_TAG_REDIRECT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                      10, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 3 + GuiSettingManager::m_iOneLineGB, iGBinGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_NO_TAG_REDIRECT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_NO_TAG_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                       18, (3 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 3 + GuiSettingManager::m_iOneLineGB, iGBinGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_NO_TAG_REDIRECT, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_NO_TAG_REDIRECT], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_NO_TAG_REDIRECT], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	m_hWndPageItems[GB_MYINFO_PROCESSING] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MYINFO_PROCESSING], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                        0, iPosY, m_iFullGB, (2 * GuiSettingManager::m_iGroupBoxMargin) + (2 * GuiSettingManager::m_iEditHeight) + 10 + (6 * GuiSettingManager::m_iCheckHeight) + 28, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_ORIGINAL_MYINFO] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SEND_LONG_MYINFO], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                       8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ((rcThis.right - rcThis.left) - 21) / 2, GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	        (((rcThis.right - rcThis.left) - 21) / 2) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, (rcThis.right - rcThis.left) - ((((rcThis.right - rcThis.left) - 21) / 2) + 26), GuiSettingManager::m_iEditHeight,
	        m_hWnd, (HMENU)CB_ORIGINAL_MYINFO_ACTION, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_FULL_MYINFO_ALL]);
	::SendMessage(m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_FULL_MYINFO_PROFILE]);
	::SendMessage(m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_FULL_MYINFO_NONE]);
	::SendMessage(m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION], 0);
	AddToolTip(m_hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], LanguageManager::m_Ptr->m_sTexts[LAN_MYINFO_TO_HINT]);

	iPosY += GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 2;

	m_hWndPageItems[GB_MODIFIED_MYINFO_OPTIONS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_OPTIONS_FOR_SHORT_MYINFO], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	        5, iPosY, m_iGBinGB, GuiSettingManager::m_iGroupBoxMargin + (6 * GuiSettingManager::m_iCheckHeight) + 15 + 8, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_REMOVE_DESCRIPTION] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        13, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_REMOVE_DESCRIPTION], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_DESCRIPTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_REMOVE_TAG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE_TAG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                  13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_REMOVE_TAG], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_TAG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_REMOVE_CONNECTION] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE_CONNECTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        13, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 6, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_REMOVE_CONNECTION], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_CONNECTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_REMOVE_EMAIL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                    13, iPosY + GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iCheckHeight) + 9, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_REMOVE_EMAIL], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_STRIP_EMAIL] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_MODE_TO_MYINFO] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MODE_TO_MYINFO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                      13, iPosY + GuiSettingManager::m_iGroupBoxMargin + (4 * GuiSettingManager::m_iCheckHeight) + 12, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_MODE_TO_MYINFO], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_MODE_TO_MYINFO] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[BTN_MODE_TO_DESCRIPTION] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MODE_TO_DESCR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        13, iPosY + GuiSettingManager::m_iGroupBoxMargin + (5 * GuiSettingManager::m_iCheckHeight) + 15, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_MODE_TO_DESCRIPTION], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_MODE_TO_DESCRIPTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	iPosY += GuiSettingManager::m_iGroupBoxMargin + (6 * GuiSettingManager::m_iCheckHeight) + 15 + 8;

	m_hWndPageItems[LBL_MINUTES_BEFORE_ACCEPT_NEW_MYINFO] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_DONT_SEND_MYINFO_CHANGES], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        8, iPosY + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(320), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        ScaleGui(320) + 13, iPosY + 5, (rcThis.right - rcThis.left) - (ScaleGui(320) + GuiSettingManager::m_iUpDownWidth + 26), GuiSettingManager::m_iEditHeight,
	        m_hWnd, (HMENU)EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], (rcThis.right - rcThis.left) - GuiSettingManager::m_iUpDownWidth - 13, iPosY + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight,
	          (LPARAM)MAKELONG(999, 0), (WPARAM)m_hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_DELAY], 0));

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
		if(m_hWndPageItems[ui8i] == nullptr) {
			return false;
		}

		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	::EnableWindow(m_hWndPageItems[EDT_NO_TAG_MESSAGE], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_NO_TAG_OPTION] != 0 ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[EDT_NO_TAG_REDIRECT], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_NO_TAG_OPTION] == 2 ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_NO_TAG_OPTION] == 0 ? TRUE : FALSE);

	::EnableWindow(m_hWndPageItems[BTN_REMOVE_DESCRIPTION], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[BTN_REMOVE_TAG], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[BTN_REMOVE_CONNECTION], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[BTN_REMOVE_EMAIL], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[BTN_MODE_TO_MYINFO], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[BTN_MODE_TO_DESCRIPTION], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);

	GuiSettingManager::m_wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageMyINFO::GetPageName() {
	return LanguageManager::m_Ptr->m_sTexts[LAN_MYINFO_PROCESSING];
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::FocusLastItem() {
	::SetFocus(m_hWndPageItems[BTN_MODE_TO_DESCRIPTION]);
}
//------------------------------------------------------------------------------
