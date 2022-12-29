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
#include "SettingPageRules.h"
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

SettingPageRules::SettingPageRules() : m_bUpdateNickLimitMessage(false), m_bUpdateMinShare(false), m_bUpdateMaxShare(false), m_bUpdateShareLimitMessage(false) {
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageRules::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == WM_COMMAND) {
		switch(LOWORD(wParam)) {
		case EDT_NICK_LEN_MSG:
		case EDT_NICK_LEN_REDIR_ADDR:
		case EDT_SHARE_MSG:
		case EDT_SHARE_REDIR_ADDR:
			if(HIWORD(wParam) == EN_CHANGE) {
				RemovePipes((HWND)lParam);

				return 0;
			}

			break;
		case EDT_MIN_NICK_LEN:
		case EDT_MAX_NICK_LEN:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 0, 64);

				uint16_t ui16Min = 0, ui16Max = 0;

				LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_MIN_NICK_LEN], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0) {
					ui16Min = LOWORD(lResult);
				}

				lResult = ::SendMessage(m_hWndPageItems[UD_MAX_NICK_LEN], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0) {
					ui16Max = LOWORD(lResult);
				}

				if(ui16Min == 0 && ui16Max == 0) {
					::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_MSG], FALSE);
					::EnableWindow(m_hWndPageItems[BTN_NICK_LEN_REDIR], FALSE);
					::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], FALSE);
				} else {
					::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_MSG], TRUE);
					::EnableWindow(m_hWndPageItems[BTN_NICK_LEN_REDIR], TRUE);
					::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}

				return 0;
			}

			break;
		case BTN_NICK_LEN_REDIR:
			if(HIWORD(wParam) == BN_CLICKED) {
				::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}

			break;
		case EDT_MIN_SHARE:
		case EDT_MAX_SHARE:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 0, 9999);

				uint16_t ui16Min = 0, ui16Max = 0;

				LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_MIN_SHARE], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0) {
					ui16Min = LOWORD(lResult);
				}

				lResult = ::SendMessage(m_hWndPageItems[UD_MAX_SHARE], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0) {
					ui16Max = LOWORD(lResult);
				}

				if(ui16Min == 0 && ui16Max == 0) {
					::EnableWindow(m_hWndPageItems[EDT_SHARE_MSG], FALSE);
					::EnableWindow(m_hWndPageItems[BTN_SHARE_REDIR], FALSE);
					::EnableWindow(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], FALSE);
				} else {
					::EnableWindow(m_hWndPageItems[EDT_SHARE_MSG], TRUE);
					::EnableWindow(m_hWndPageItems[BTN_SHARE_REDIR], TRUE);
					::EnableWindow(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}

				return 0;
			}

			break;
		case BTN_SHARE_REDIR:
			if(HIWORD(wParam) == BN_CLICKED) {
				::EnableWindow(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}

			break;
		case UD_MAIN_CHAT_LEN:
		case UD_PM_LEN:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 0, 32767);

				return 0;
			}

			break;
		case UD_MAIN_CHAT_LINES:
		case UD_PM_LINES:
		case UD_SEARCH_MIN_LEN:
		case UD_SEARCH_MAX_LEN:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 0, 999);

				return 0;
			}

			break;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageRules::Save() {
	if(m_bCreated == false) {
		return;
	}

	LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_MIN_NICK_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_NICK_LEN]) {
			m_bUpdateNickLimitMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_NICK_LEN, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_MAX_NICK_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_NICK_LEN]) {
			m_bUpdateNickLimitMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_NICK_LEN, LOWORD(lResult));
	}

	char buf[257];
	int iLen = ::GetWindowText(m_hWndPageItems[EDT_NICK_LEN_MSG], buf, 257);

	if(strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_NICK_LIMIT_MSG]) != 0) {
		m_bUpdateNickLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_NICK_LIMIT_MSG, buf, iLen);

	bool bNickRedir = ::SendMessage(m_hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bNickRedir != SettingManager::m_Ptr->m_bBools[SETBOOL_NICK_LIMIT_REDIR]) {
		m_bUpdateNickLimitMessage = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_NICK_LIMIT_REDIR, bNickRedir);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], buf, 257);

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS]) != 0))
	{
		m_bUpdateNickLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_NICK_LIMIT_REDIR_ADDRESS, buf, iLen);

	lResult = ::SendMessage(m_hWndPageItems[UD_MIN_SHARE], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SHARE_LIMIT]) {
			m_bUpdateMinShare = true;
			m_bUpdateShareLimitMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_LIMIT, LOWORD(lResult));
	}

	uint16_t ui16MinShareUnits = (int16_t)::SendMessage(m_hWndPageItems[CB_MIN_SHARE], CB_GETCURSEL, 0, 0);

	if(ui16MinShareUnits != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SHARE_UNITS]) {
		m_bUpdateMinShare = true;
		m_bUpdateShareLimitMessage = true;
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_UNITS, ui16MinShareUnits);

	lResult = ::SendMessage(m_hWndPageItems[UD_MAX_SHARE], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SHARE_LIMIT]) {
			m_bUpdateMaxShare = true;
			m_bUpdateShareLimitMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_LIMIT, LOWORD(lResult));
	}

	uint16_t ui16MaxShareUnits = (int16_t)::SendMessage(m_hWndPageItems[CB_MAX_SHARE], CB_GETCURSEL, 0, 0);

	if(ui16MaxShareUnits != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SHARE_UNITS]) {
		m_bUpdateMaxShare = true;
		m_bUpdateShareLimitMessage = true;
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_UNITS, ui16MaxShareUnits);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_SHARE_MSG], buf, 257);

	if(strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_SHARE_LIMIT_MSG]) != 0) {
		m_bUpdateShareLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_SHARE_LIMIT_MSG, buf, iLen);

	bool bShareRedir = ::SendMessage(m_hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bShareRedir != SettingManager::m_Ptr->m_bBools[SETBOOL_SHARE_LIMIT_REDIR]) {
		m_bUpdateShareLimitMessage = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_SHARE_LIMIT_REDIR, bShareRedir);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], buf, 257);

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS]) != 0))
	{
		m_bUpdateShareLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_SHARE_LIMIT_REDIR_ADDRESS, buf, iLen);

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_CHAT_LEN, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_LINES], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_CHAT_LINES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_PM_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_PM_LEN, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_PM_LINES], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_PM_LINES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_MIN_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SEARCH_LEN, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_MAX_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SEARCH_LEN, LOWORD(lResult));
	}
}
//------------------------------------------------------------------------------

void SettingPageRules::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                  bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool &bUpdatedShareLimitMessage,
                                  bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                  bool &bUpdatedNickLimitMessage, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                  bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                  bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool &bUpdatedMinShare, bool &bUpdatedMaxShare) {
	if(bUpdatedNickLimitMessage == false) {
		bUpdatedNickLimitMessage = m_bUpdateNickLimitMessage;
	}
	if(bUpdatedShareLimitMessage == false) {
		bUpdatedShareLimitMessage = m_bUpdateShareLimitMessage;
	}
	if(bUpdatedMinShare == false) {
		bUpdatedMinShare = m_bUpdateMinShare;
	}
	if(bUpdatedMaxShare == false) {
		bUpdatedMaxShare = m_bUpdateMaxShare;
	}
}

//------------------------------------------------------------------------------

bool SettingPageRules::CreateSettingPage(HWND hOwner) {
	CreateHWND(hOwner);

	if(m_bCreated == false) {
		return false;
	}

	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);

	m_hWndPageItems[GB_NICK_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                  0, 0, m_iFullGB, m_iOneLineTwoGroupGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MIN_NICK_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                    8, GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MIN_NICK_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MIN_NICK_LEN], EM_SETLIMITTEXT, 2, 0);
	AddToolTip(m_hWndPageItems[EDT_MIN_NICK_LEN], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_MIN_NICK_LEN], ScaleGui(40) + 8, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(64, 0), (WPARAM)m_hWndPageItems[EDT_MIN_NICK_LEN],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_NICK_LEN], 0));

	m_hWndPageItems[LBL_MIN_NICK_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MIN_LEN], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                    ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                    ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 15), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_MAX_NICK_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAX_LEN], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                    ((rcThis.right - rcThis.left - 16) / 2) + 2, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                    ((rcThis.right - rcThis.left) - ScaleGui(40) - GuiSettingManager::m_iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAX_NICK_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                    (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth - ScaleGui(40), GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAX_NICK_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAX_NICK_LEN], EM_SETLIMITTEXT, 2, 0);
	AddToolTip(m_hWndPageItems[EDT_MAX_NICK_LEN], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_MAX_NICK_LEN], (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(64, 0),
	          (WPARAM)m_hWndPageItems[EDT_MAX_NICK_LEN], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_NICK_LEN], 0));

	m_hWndPageItems[GB_NICK_LEN_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                   5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_NICK_LEN_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_NICK_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                    13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + 2, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_NICK_LEN_MSG, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_NICK_LEN_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_NICK_LEN_MSG], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LIMIT_MSG_HINT]);

	m_hWndPageItems[GB_NICK_LEN_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                     5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_NICK_LEN_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                      13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), ScaleGui(85), GuiSettingManager::m_iCheckHeight, m_hWnd,
	                                      (HMENU)BTN_NICK_LEN_REDIR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_NICK_LEN_REDIR], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_NICK_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	        ES_AUTOHSCROLL, ScaleGui(85) + 18, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), GuiSettingManager::m_iEditHeight,
	        m_hWnd, (HMENU)EDT_NICK_LEN_REDIR_ADDR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	int iPosY = m_iOneLineTwoGroupGB;

	m_hWndPageItems[GB_SHARE_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                   0, iPosY, m_iFullGB, m_iOneLineTwoGroupGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MIN_SHARE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MIN_SHARE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MIN_SHARE], EM_SETLIMITTEXT, 4, 0);
	AddToolTip(m_hWndPageItems[EDT_MIN_SHARE], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_MIN_SHARE], ScaleGui(40) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 0), (WPARAM)m_hWndPageItems[EDT_MIN_SHARE],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SHARE_LIMIT], 0));

	m_hWndPageItems[CB_MIN_SHARE] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(50), GuiSettingManager::m_iEditHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KILO_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_MEGA_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TERA_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MIN_SHARE], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SHARE_UNITS], 0);

	m_hWndPageItems[LBL_MIN_SHARE] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MIN_SHARE], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                 ScaleGui(90) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                 ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(90) + GuiSettingManager::m_iUpDownWidth + 20), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_MAX_SHARE] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAX_SHARE], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                 ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                 ((rcThis.right - rcThis.left) - ScaleGui(90) - GuiSettingManager::m_iUpDownWidth - 23) - (((rcThis.right - rcThis.left - 16) / 2) + 2), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAX_SHARE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 (rcThis.right - rcThis.left) - GuiSettingManager::m_iUpDownWidth - ScaleGui(90) - 18, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAX_SHARE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAX_SHARE], EM_SETLIMITTEXT, 4, 0);
	AddToolTip(m_hWndPageItems[EDT_MAX_SHARE], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_MAX_SHARE], (rcThis.right - rcThis.left) - GuiSettingManager::m_iUpDownWidth - ScaleGui(50) - 18, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 0),
	          (WPARAM)m_hWndPageItems[EDT_MAX_SHARE], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SHARE_LIMIT], 0));

	m_hWndPageItems[CB_MAX_SHARE] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                (rcThis.right - rcThis.left) - ScaleGui(50) - 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(50), GuiSettingManager::m_iEditHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KILO_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_MEGA_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TERA_BYTES]);
	::SendMessage(m_hWndPageItems[CB_MAX_SHARE], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SHARE_UNITS], 0);

	m_hWndPageItems[GB_SHARE_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SHARE_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_SHARE_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                 13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + 2, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SHARE_MSG, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SHARE_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_SHARE_MSG], LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_LIMIT_MSG_HINT]);

	m_hWndPageItems[GB_SHARE_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                  5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_SHARE_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                   13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), ScaleGui(85), GuiSettingManager::m_iCheckHeight, m_hWnd,
	                                   (HMENU)BTN_SHARE_REDIR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SHARE_REDIR], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_SHARE_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[EDT_SHARE_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                        ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), GuiSettingManager::m_iEditHeight,
	                                        m_hWnd, (HMENU)EDT_NICK_LEN_REDIR_ADDR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	iPosY *= 2;

	m_hWndPageItems[GB_MAIN_CHAT_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MAIN_CHAT_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                       0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAIN_CHAT_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                     8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_LEN], EM_SETLIMITTEXT, 5, 0);
	AddToolTip(m_hWndPageItems[EDT_MAIN_CHAT_LEN], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_LEN], ScaleGui(40) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 0), (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_LEN],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CHAT_LEN], 0));

	m_hWndPageItems[LBL_MAIN_CHAT_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_LENGTH], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                     ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, iPosY +GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                     ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 15), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_MAIN_CHAT_LINES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_LINES], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                       ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY +GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                       ((rcThis.right - rcThis.left) - ScaleGui(40) - GuiSettingManager::m_iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAIN_CHAT_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                       (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth - ScaleGui(40), iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_LINES, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_LINES], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_MAIN_CHAT_LINES], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_LINES], (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_LINES], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CHAT_LINES], 0));

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_PM_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PM_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_PM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                              8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_PM_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_PM_LEN], EM_SETLIMITTEXT, 5, 0);
	AddToolTip(m_hWndPageItems[EDT_PM_LEN], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_PM_LEN], ScaleGui(40) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 0), (WPARAM)m_hWndPageItems[EDT_PM_LEN],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_LEN], 0));

	m_hWndPageItems[LBL_PM_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_LENGTH], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                              ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, iPosY +GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                              ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 15), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_PM_LINES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_LINES], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY +GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                ((rcThis.right - rcThis.left) - ScaleGui(40) - GuiSettingManager::m_iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_PM_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth - ScaleGui(40), iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_PM_LINES, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_PM_LINES], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_PM_LINES], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_PM_LINES], (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)m_hWndPageItems[EDT_PM_LINES], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_LINES], 0));

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_SEARCH_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SEARCH_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                    0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SEARCH_MIN_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                      8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_MIN_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SEARCH_MIN_LEN], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_SEARCH_MIN_LEN], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_SEARCH_MIN_LEN], ScaleGui(40) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 0), (WPARAM)m_hWndPageItems[EDT_SEARCH_MIN_LEN],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SEARCH_LEN], 0));

	m_hWndPageItems[LBL_SEARCH_MIN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MINIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                  ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, iPosY +GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                  ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 15), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_SEARCH_MAX] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                  ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY +GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                  ((rcThis.right - rcThis.left) - ScaleGui(40) - GuiSettingManager::m_iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SEARCH_MAX_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                      (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth - ScaleGui(40), iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_MAX_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SEARCH_MAX_LEN], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_SEARCH_MAX_LEN], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_SEARCH_MAX_LEN], (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)m_hWndPageItems[EDT_SEARCH_MAX_LEN], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN], 0));

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
		if(m_hWndPageItems[ui8i] == nullptr) {
			return false;
		}

		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SHARE_LIMIT] == 0 && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SHARE_LIMIT] == 0) {
		::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_MSG], FALSE);
		::EnableWindow(m_hWndPageItems[BTN_NICK_LEN_REDIR], FALSE);
		::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], FALSE);
	} else {
		::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_MSG], TRUE);
		::EnableWindow(m_hWndPageItems[BTN_NICK_LEN_REDIR], TRUE);
		::EnableWindow(m_hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], SettingManager::m_Ptr->m_bBools[SETBOOL_NICK_LIMIT_REDIR] == true ? TRUE : FALSE);
	}

	if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_NICK_LEN] == 0 && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_NICK_LEN] == 0) {
		::EnableWindow(m_hWndPageItems[EDT_SHARE_MSG], FALSE);
		::EnableWindow(m_hWndPageItems[BTN_SHARE_REDIR], FALSE);
		::EnableWindow(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], FALSE);
	} else {
		::EnableWindow(m_hWndPageItems[EDT_SHARE_MSG], TRUE);
		::EnableWindow(m_hWndPageItems[BTN_SHARE_REDIR], TRUE);
		::EnableWindow(m_hWndPageItems[EDT_SHARE_REDIR_ADDR], SettingManager::m_Ptr->m_bBools[SETBOOL_SHARE_LIMIT_REDIR] == true ? TRUE : FALSE);
	}

	GuiSettingManager::m_wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_SEARCH_MAX_LEN], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageRules::GetPageName() {
	return LanguageManager::m_Ptr->m_sTexts[LAN_RULES];
}
//------------------------------------------------------------------------------

void SettingPageRules::FocusLastItem() {
	::SetFocus(m_hWndPageItems[EDT_SEARCH_MAX_LEN]);
}
//------------------------------------------------------------------------------
