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
#include "SettingPageDeflood.h"
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

SettingPageDeflood::SettingPageDeflood() {
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageDeflood::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == WM_COMMAND) {
		switch(LOWORD(wParam)) {
		case EDT_GLOBAL_MAIN_CHAT_MSGS:
		case EDT_GLOBAL_MAIN_CHAT_SECS:
		case EDT_GLOBAL_MAIN_CHAT_SECS2:
		case EDT_MAIN_CHAT_MSGS:
		case EDT_MAIN_CHAT_SECS:
		case EDT_MAIN_CHAT_MSGS2:
		case EDT_MAIN_CHAT_SECS2:
		case EDT_MAIN_CHAT_INTERVAL_MSGS:
		case EDT_MAIN_CHAT_INTERVAL_SECS:
		case EDT_SAME_MAIN_CHAT_MSGS:
		case EDT_SAME_MAIN_CHAT_SECS:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 1, 999);

				return 0;
			}

			break;
		case EDT_SAME_MULTI_MAIN_CHAT_MSGS:
		case EDT_SAME_MULTI_MAIN_CHAT_LINES:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 2, 999);

				return 0;
			}

			break;
		case EDT_CTM_MSGS:
		case EDT_CTM_SECS:
		case EDT_CTM_MSGS2:
		case EDT_CTM_SECS2:
		case EDT_RCTM_MSGS:
		case EDT_RCTM_SECS:
		case EDT_RCTM_MSGS2:
		case EDT_RCTM_SECS2:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 1, 9999);

				return 0;
			}

			break;
		case EDT_DEFLOOD_TEMP_BAN_TIME:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 1, 32767);

				return 0;
			}

			break;
		case EDT_MAX_USERS_LOGINS:
			if(HIWORD(wParam) == EN_CHANGE) {
				MinMaxCheck((HWND)lParam, 1, 1000);

				return 0;
			}

			break;
		case CB_GLOBAL_MAIN_CHAT:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_MAIN_CHAT:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_MAIN_CHAT2:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_SAME_MAIN_CHAT:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_SAME_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_SAME_MULTI_MAIN_CHAT:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_CTM:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_CTM], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_CTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_CTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_CTM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_CTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_CTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_CTM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_CTM2:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_CTM2], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_CTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_CTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_CTM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_CTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_CTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_CTM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_RCTM:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_RCTM], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_RCTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_RCTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_RCTM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_RCTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_RCTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_RCTM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		case CB_RCTM2:
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_RCTM2], CB_GETCURSEL, 0, 0);
				::EnableWindow(m_hWndPageItems[EDT_RCTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_RCTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_RCTM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[EDT_RCTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[UD_RCTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(m_hWndPageItems[LBL_RCTM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}

			break;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageDeflood::Save() {
	if(m_bCreated == false) {
		return;
	}

	LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_MAIN_CHAT_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAIN_CHAT_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_MAIN_CHAT_ACTION2, (int16_t)::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_MSGS2], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAIN_CHAT_MESSAGES2, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_SECS2], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAIN_CHAT_TIME2, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_INTERVAL_MSGS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_CHAT_INTERVAL_MESSAGES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_MAIN_CHAT_INTERVAL_SECS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_CHAT_INTERVAL_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_SAME_MAIN_CHAT_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_SAME_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_SAME_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_SAME_MAIN_CHAT_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_LINES, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_CTM_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_CTM], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_CTM_MSGS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_CTM_MESSAGES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_CTM_SECS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_CTM_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_CTM_ACTION2, (int16_t)::SendMessage(m_hWndPageItems[CB_CTM2], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_CTM_MSGS2], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_CTM_MESSAGES2, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_CTM_SECS2], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_CTM_TIME2, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_RCTM_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_RCTM], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_RCTM_MSGS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_RCTM_MESSAGES, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_RCTM_SECS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_RCTM_TIME, LOWORD(lResult));
	}

	SettingManager::m_Ptr->SetShort(SETSHORT_RCTM_ACTION2, (int16_t)::SendMessage(m_hWndPageItems[CB_RCTM2], CB_GETCURSEL, 0, 0));

	lResult = ::SendMessage(m_hWndPageItems[UD_RCTM_MSGS2], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_RCTM_MESSAGES2, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_RCTM_SECS2], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_RCTM_TIME2, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_DEFLOOD_TEMP_BAN_TIME], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_DEFLOOD_TEMP_BAN_TIME, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_MAX_USERS_LOGINS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0) {
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SIMULTANEOUS_LOGINS, LOWORD(lResult));
	}
}
//------------------------------------------------------------------------------

void SettingPageDeflood::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
}
//------------------------------------------------------------------------------

bool SettingPageDeflood::CreateSettingPage(HWND hOwner) {
	CreateHWND(hOwner);

	if(m_bCreated == false) {
		return false;
	}

	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);

	int iGMCWidth = (rcThis.right - rcThis.left) - 21 - (3 * GuiSettingManager::m_iUpDownWidth) - 35;
	int iGMCEditWidth = int(iGMCWidth * 0.08);
	int iGMCTriCharTxtWidth = int(iGMCWidth * 0.07);

	m_hWndPageItems[GB_GLOBAL_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_GLOBAL_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                       0, 0, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        8, GuiSettingManager::m_iGroupBoxMargin, iGMCEditWidth, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_MSGS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], iGMCEditWidth + 8, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES], 0));

	m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	        iGMCEditWidth + GuiSettingManager::m_iUpDownWidth + 13, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), int(iGMCWidth * 0.04), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        iGMCEditWidth + int(iGMCWidth * 0.04) + GuiSettingManager::m_iUpDownWidth + 18, GuiSettingManager::m_iGroupBoxMargin, iGMCEditWidth, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_SECS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) +  GuiSettingManager::m_iUpDownWidth + 18, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIME], 0));

	m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SEC_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * GuiSettingManager::m_iUpDownWidth) + 23, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), iGMCTriCharTxtWidth, GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_GLOBAL_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                       (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + iGMCTriCharTxtWidth + (2 * GuiSettingManager::m_iUpDownWidth) + 28, GuiSettingManager::m_iGroupBoxMargin, int(iGMCWidth * 0.51), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_GLOBAL_MAIN_CHAT, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_LOCK_CHAT]);
	::SendMessage(m_hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_ONLY_TO_OPS_WITH_IP]);
	::SendMessage(m_hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION], 0);

	m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_FOR], WS_CHILD | WS_VISIBLE | SS_CENTER,
	        (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + iGMCTriCharTxtWidth + int(iGMCWidth * 0.51) + (2 * GuiSettingManager::m_iUpDownWidth) + 33, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), iGMCTriCharTxtWidth, GuiSettingManager::m_iTextHeight,
	        m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (2 * GuiSettingManager::m_iUpDownWidth) + 38, GuiSettingManager::m_iGroupBoxMargin, iGMCEditWidth, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_SECS2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], (3 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (2 * GuiSettingManager::m_iUpDownWidth) + 38, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT], 0));

	m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SEC_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        (3 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (3 * GuiSettingManager::m_iUpDownWidth) + 43, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        m_iFullEDT - ((3 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (3 * GuiSettingManager::m_iUpDownWidth) + 35), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	int iPosY = GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                0, iPosY, m_iFullGB, m_iThreeLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_MAIN_CHAT, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION], 0);

	m_hWndPageItems[EDT_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                      ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_MSGS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_MSGS],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES], 0));

	m_hWndPageItems[LBL_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                      ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_SECS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_TIME], 0));

	m_hWndPageItems[LBL_MAIN_CHAT_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_MAIN_CHAT2] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                 8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_MAIN_CHAT2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_MAIN_CHAT2], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2], 0);

	m_hWndPageItems[EDT_MAIN_CHAT_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                       ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_MSGS2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_MSGS2], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_MSGS2], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_MSGS2],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES2], 0));

	m_hWndPageItems[LBL_MAIN_CHAT_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAIN_CHAT_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                       ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_SECS2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_SECS2], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_SECS2], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_SECS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_TIME2], 0));

	m_hWndPageItems[LBL_MAIN_CHAT_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_MAIN_CHAT_INTERVAL] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_INTERVAL], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(180), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_INTERVAL_MSGS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_INTERVAL_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CHAT_INTERVAL_MESSAGES], 0));

	m_hWndPageItems[LBL_MAIN_CHAT_INTERVAL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_INTERVAL_SECS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MAIN_CHAT_INTERVAL_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight,
	          (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CHAT_INTERVAL_TIME], 0));

	m_hWndPageItems[LBL_MAIN_CHAT_INTERVAL_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	iPosY += m_iThreeLineGB;

	m_hWndPageItems[GB_SAME_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SAME_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                     0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_SAME_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                     8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_SAME_MAIN_CHAT, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_SAME_MAIN_CHAT], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION], 0);

	m_hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SAME_MAIN_CHAT_MSGS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 2), (WPARAM)m_hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_MESSAGES], 0));

	m_hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SAME_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SAME_MAIN_CHAT_SECS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_SAME_MAIN_CHAT_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_SAME_MAIN_CHAT_SECS],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_TIME], 0));

	m_hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_SAME_MULTI_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SAME_MULTI_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	        0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_SAME_MULTI_MAIN_CHAT, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION], 0);

	m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SAME_MULTI_MAIN_CHAT_MSGS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 2),
	          (WPARAM)m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES], 0));

	m_hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_LWR], WS_CHILD | WS_VISIBLE | SS_CENTER,
	        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(30), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        ScaleGui(250) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SAME_MULTI_MAIN_CHAT_LINES, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], ScaleGui(290) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 2),
	          (WPARAM)m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_LINES], 0));

	m_hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_LINES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        ScaleGui(290) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        (rcThis.right - rcThis.left) - (ScaleGui(290) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_CTM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CONNECTTOME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                          0, iPosY, m_iFullGB, m_iTwoLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_CTM] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                          8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_CTM, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_CTM], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION], 0);

	m_hWndPageItems[EDT_CTM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_CTM_MSGS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_CTM_MSGS], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_CTM_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)m_hWndPageItems[EDT_CTM_MSGS],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_MESSAGES], 0));

	m_hWndPageItems[LBL_CTM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                   ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_CTM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_CTM_SECS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_CTM_SECS], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_CTM_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)m_hWndPageItems[EDT_CTM_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_TIME], 0));

	m_hWndPageItems[LBL_CTM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                   ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                   (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_CTM2] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                           8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_CTM2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_CTM2], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2], 0);

	m_hWndPageItems[EDT_CTM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_CTM_MSGS2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_CTM_MSGS2], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_CTM_MSGS2], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)m_hWndPageItems[EDT_CTM_MSGS2],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_MESSAGES2], 0));

	m_hWndPageItems[LBL_CTM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                    ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_CTM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_CTM_SECS2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_CTM_SECS2], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_CTM_SECS2], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)m_hWndPageItems[EDT_CTM_SECS2],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_TIME2], 0));

	m_hWndPageItems[LBL_CTM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                    ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                    (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	iPosY += m_iTwoLineGB;

	m_hWndPageItems[GB_RCTM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REVCONNECTTOME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                           0, iPosY, m_iFullGB, m_iTwoLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_RCTM] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                           8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_RCTM, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_RCTM], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION], 0);

	m_hWndPageItems[EDT_RCTM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_RCTM_MSGS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_RCTM_MSGS], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_RCTM_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)m_hWndPageItems[EDT_RCTM_MSGS],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_MESSAGES], 0));

	m_hWndPageItems[LBL_RCTM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                    ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_RCTM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_RCTM_SECS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_RCTM_SECS], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_RCTM_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)m_hWndPageItems[EDT_RCTM_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_TIME], 0));

	m_hWndPageItems[LBL_RCTM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                    ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                    (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_RCTM2] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                            8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_RCTM2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
	::SendMessage(m_hWndPageItems[CB_RCTM2], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2], 0);

	m_hWndPageItems[EDT_RCTM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                  ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_RCTM_MSGS2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_RCTM_MSGS2], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_RCTM_MSGS2], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)m_hWndPageItems[EDT_RCTM_MSGS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_MESSAGES2], 0));

	m_hWndPageItems[LBL_RCTM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                     ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_RCTM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                  ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_RCTM_SECS2, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_RCTM_SECS2], EM_SETLIMITTEXT, 4, 0);

	AddUpDown(m_hWndPageItems[UD_RCTM_SECS2], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)m_hWndPageItems[EDT_RCTM_SECS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_TIME2], 0));

	m_hWndPageItems[LBL_RCTM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                     ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                     (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	iPosY += m_iTwoLineGB;

	m_hWndPageItems[GB_DEFLOOD_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DEFLOOD_TEMP_BAN_TIME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	        0, iPosY, ((rcThis.right - rcThis.left - 5) / 2) - 2, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_DEFLOOD_TEMP_BAN_TIME, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME], EM_SETLIMITTEXT, 5, 0);

	AddUpDown(m_hWndPageItems[UD_DEFLOOD_TEMP_BAN_TIME], ScaleGui(40) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 1),
	          (WPARAM)m_hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME], 0));

	m_hWndPageItems[LBL_DEFLOOD_TEMP_BAN_TIME_MINUTES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MINUTES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	        ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	        ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 23, GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[GB_MAX_USERS_LOGINS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MAX_USERS_LOGINS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                       ((rcThis.right - rcThis.left - 5) / 2) + 3, iPosY, (rcThis.right - rcThis.left) - (((rcThis.right - rcThis.left - 5) / 2) + 8), GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAX_USERS_LOGINS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                        ((rcThis.right - rcThis.left - 5) / 2) + 11, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAX_USERS_LOGINS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAX_USERS_LOGINS], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_MAX_USERS_LOGINS], ((rcThis.right - rcThis.left - 5) / 2) + ScaleGui(40) + 11, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(1000, 1),
	          (WPARAM)m_hWndPageItems[EDT_MAX_USERS_LOGINS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SIMULTANEOUS_LOGINS], 0));

	m_hWndPageItems[LBL_MAX_USERS_LOGINS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_PER_10_SECONDS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                        ((rcThis.right - rcThis.left - 5) / 2) + ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 16, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                        (rcThis.right - rcThis.left) - ((rcThis.right - rcThis.left - 5) / 2) - ScaleGui(40) - GuiSettingManager::m_iUpDownWidth - 29, GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
		if(m_hWndPageItems[ui8i] == nullptr) {
			return false;
		}

		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	::EnableWindow(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_DIVIDER2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_MAIN_CHAT_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_MAIN_CHAT_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_MAIN_CHAT_SECONDS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_SAME_MAIN_CHAT_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_CTM_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_CTM_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_CTM_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_CTM_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_CTM_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_CTM_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_CTM_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_CTM_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_CTM_DIVIDER2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_CTM_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_CTM_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_CTM_SECONDS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_RCTM_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_RCTM_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_RCTM_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_RCTM_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_RCTM_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_RCTM_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);

	::EnableWindow(m_hWndPageItems[EDT_RCTM_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_RCTM_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_RCTM_DIVIDER2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[EDT_RCTM_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[UD_RCTM_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(m_hWndPageItems[LBL_RCTM_SECONDS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);

	GuiSettingManager::m_wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_MAX_USERS_LOGINS], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageDeflood::GetPageName() {
	return LanguageManager::m_Ptr->m_sTexts[LAN_DEFLOOD];
}
//------------------------------------------------------------------------------

void SettingPageDeflood::FocusLastItem() {
	::SetFocus(m_hWndPageItems[EDT_MAX_USERS_LOGINS]);
}
//------------------------------------------------------------------------------
