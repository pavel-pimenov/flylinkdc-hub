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

SettingPageDeflood::SettingPageDeflood()
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageDeflood::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
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
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 999);
				
				return 0;
			}
			
			break;
		case EDT_SAME_MULTI_MAIN_CHAT_MSGS:
		case EDT_SAME_MULTI_MAIN_CHAT_LINES:
			if (HIWORD(wParam) == EN_CHANGE)
			{
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
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 9999);
				
				return 0;
			}
			
			break;
		case EDT_DEFLOOD_TEMP_BAN_TIME:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 32767);
				
				return 0;
			}
			
			break;
		case EDT_MAX_USERS_LOGINS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 1000);
				
				return 0;
			}
			
			break;
		case CB_GLOBAL_MAIN_CHAT:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_MAIN_CHAT:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_MAIN_CHAT2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_SAME_MAIN_CHAT:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_SAME_MULTI_MAIN_CHAT:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_CTM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_CTM], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_CTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_CTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_CTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_CTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_CTM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_CTM2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_CTM2], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_CTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_CTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_CTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_CTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_CTM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_RCTM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RCTM], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_RCTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RCTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_RCTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RCTM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_RCTM2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RCTM2], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_RCTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RCTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_RCTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RCTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageDeflood::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	LRESULT lResult = ::SendMessage(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAIN_CHAT_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_MAIN_CHAT_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_MSGS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAIN_CHAT_MESSAGES2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_SECS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAIN_CHAT_TIME2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_INTERVAL_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_CHAT_INTERVAL_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_INTERVAL_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_CHAT_INTERVAL_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MAIN_CHAT_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_LINES, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_CTM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_CTM], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_CTM_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_CTM_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_CTM_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_CTM_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_CTM_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_CTM2], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_CTM_MSGS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_CTM_MESSAGES2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_CTM_SECS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_CTM_TIME2, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_RCTM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_RCTM], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_RCTM_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_RCTM_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_RCTM_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_RCTM_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_RCTM_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_RCTM2], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_RCTM_MSGS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_RCTM_MESSAGES2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_RCTM_SECS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_RCTM_TIME2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_DEFLOOD_TEMP_BAN_TIME], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_DEFLOOD_TEMP_BAN_TIME, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAX_USERS_LOGINS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_SIMULTANEOUS_LOGINS, LOWORD(lResult));
	}
}
//------------------------------------------------------------------------------

void SettingPageDeflood::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
}
//------------------------------------------------------------------------------

bool SettingPageDeflood::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	int iGMCWidth = (rcThis.right - rcThis.left) - 21 - (3 * clsGuiSettingManager::iUpDownWidth) - 35;
	int iGMCEditWidth = int(iGMCWidth * 0.08);
	int iGMCTriCharTxtWidth = int(iGMCWidth * 0.07);
	
	hWndPageItems[GB_GLOBAL_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_GLOBAL_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                      0, 0, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                            8, clsGuiSettingManager::iGroupBoxMargin, iGMCEditWidth, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], iGMCEditWidth + 8, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES], 0));
	          
	hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                               iGMCEditWidth + clsGuiSettingManager::iUpDownWidth + 13, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), int(iGMCWidth * 0.04), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                               
	hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                            iGMCEditWidth + int(iGMCWidth * 0.04) + clsGuiSettingManager::iUpDownWidth + 18, clsGuiSettingManager::iGroupBoxMargin, iGMCEditWidth, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) +  clsGuiSettingManager::iUpDownWidth + 18, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIME], 0));
	          
	hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SEC_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                           (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * clsGuiSettingManager::iUpDownWidth) + 23, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), iGMCTriCharTxtWidth, clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                           
	hWndPageItems[CB_GLOBAL_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                      (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + iGMCTriCharTxtWidth + (2 * clsGuiSettingManager::iUpDownWidth) + 28, clsGuiSettingManager::iGroupBoxMargin, int(iGMCWidth * 0.51), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_GLOBAL_MAIN_CHAT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_LOCK_CHAT]);
	::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_ONLY_TO_OPS_WITH_IP]);
	::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION], 0);
	
	hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_FOR], WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                           (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + iGMCTriCharTxtWidth + int(iGMCWidth * 0.51) + (2 * clsGuiSettingManager::iUpDownWidth) + 33, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), iGMCTriCharTxtWidth, clsGuiSettingManager::iTextHeight,
	                                                           m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                           
	hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                             (2 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (2 * clsGuiSettingManager::iUpDownWidth) + 38, clsGuiSettingManager::iGroupBoxMargin, iGMCEditWidth, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_SECS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], (3 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (2 * clsGuiSettingManager::iUpDownWidth) + 38, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT], 0));
	          
	hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SEC_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                            (3 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (3 * clsGuiSettingManager::iUpDownWidth) + 43, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                            iFullEDT - ((3 * iGMCEditWidth) + int(iGMCWidth * 0.04) + (2 * iGMCTriCharTxtWidth) + int(iGMCWidth * 0.51) + (3 * clsGuiSettingManager::iUpDownWidth) + 35), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                            
	int iPosY = clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               0, iPosY, iFullGB, iThreeLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[CB_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                               8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_MAIN_CHAT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION], 0);
	
	hWndPageItems[EDT_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                     ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_MSGS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES], 0));
	          
	hWndPageItems[LBL_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                        ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                        
	hWndPageItems[EDT_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                     ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_MAIN_CHAT_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_TIME], 0));
	          
	hWndPageItems[LBL_MAIN_CHAT_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                        ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                        
	hWndPageItems[CB_MAIN_CHAT2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_MAIN_CHAT2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2], 0);
	
	hWndPageItems[EDT_MAIN_CHAT_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                      ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_MSGS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_MSGS2], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_MSGS2], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_MSGS2],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES2], 0));
	          
	hWndPageItems[LBL_MAIN_CHAT_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                         ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                         
	hWndPageItems[EDT_MAIN_CHAT_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                      ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_SECS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_SECS2], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_SECS2], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_MAIN_CHAT_SECS2], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_TIME2], 0));
	          
	hWndPageItems[LBL_MAIN_CHAT_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                         ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                         (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                         
	hWndPageItems[LBL_MAIN_CHAT_INTERVAL] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_INTERVAL], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                         8, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(180), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                         
	hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                              ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_INTERVAL_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_INTERVAL_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_CHAT_INTERVAL_MESSAGES], 0));
	          
	hWndPageItems[LBL_MAIN_CHAT_INTERVAL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                                 ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                 
	hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                              ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_INTERVAL_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_INTERVAL_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight,
	          (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_CHAT_INTERVAL_TIME], 0));
	          
	hWndPageItems[LBL_MAIN_CHAT_INTERVAL_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                                 ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                                 (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                 
	iPosY += iThreeLineGB;
	
	hWndPageItems[GB_SAME_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SAME_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                    0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[CB_SAME_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                    8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_SAME_MAIN_CHAT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION], 0);
	
	hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                          ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_MAIN_CHAT_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 2), (WPARAM)hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_MESSAGES], 0));
	          
	hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                             ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                             
	hWndPageItems[EDT_SAME_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                          ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_MAIN_CHAT_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SAME_MAIN_CHAT_SECS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_TIME], 0));
	          
	hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                             ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                             (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                             
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_SAME_MULTI_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SAME_MULTI_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                          0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                          
	hWndPageItems[CB_SAME_MULTI_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                          8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_SAME_MULTI_MAIN_CHAT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION], 0);
	
	hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                                ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_MULTI_MAIN_CHAT_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 2),
	          (WPARAM)hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES], 0));
	          
	hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_WITH_LWR], WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                                ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(30), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                
	hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                                 ScaleGui(250) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_MULTI_MAIN_CHAT_LINES, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], ScaleGui(290) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 2),
	          (WPARAM)hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_LINES], 0));
	          
	hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_LINES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                                 ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                                 (rcThis.right - rcThis.left) - (ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                 
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_CTM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CONNECTTOME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                         0, iPosY, iFullGB, iTwoLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                         
	hWndPageItems[CB_CTM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                         8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_CTM, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_CTM], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION], 0);
	
	hWndPageItems[EDT_CTM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                               ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_CTM_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_CTM_MSGS], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_CTM_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_CTM_MSGS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_MESSAGES], 0));
	          
	hWndPageItems[LBL_CTM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                  ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_CTM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                               ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_CTM_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_CTM_SECS], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_CTM_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_CTM_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_TIME], 0));
	          
	hWndPageItems[LBL_CTM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                  ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                  (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[CB_CTM2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                          8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_CTM2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_CTM2], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2], 0);
	
	hWndPageItems[EDT_CTM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_CTM_MSGS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_CTM_MSGS2], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_CTM_MSGS2], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_CTM_MSGS2],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_MESSAGES2], 0));
	          
	hWndPageItems[LBL_CTM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                   ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[EDT_CTM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_CTM_SECS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_CTM_SECS2], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_CTM_SECS2], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_CTM_SECS2],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_TIME2], 0));
	          
	hWndPageItems[LBL_CTM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                   ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                   (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	iPosY += iTwoLineGB;
	
	hWndPageItems[GB_RCTM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REVCONNECTTOME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                          0, iPosY, iFullGB, iTwoLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                          
	hWndPageItems[CB_RCTM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                          8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_RCTM, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_RCTM], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION], 0);
	
	hWndPageItems[EDT_RCTM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RCTM_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RCTM_MSGS], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RCTM_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RCTM_MSGS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_MESSAGES], 0));
	          
	hWndPageItems[LBL_RCTM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                   ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[EDT_RCTM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RCTM_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RCTM_SECS], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RCTM_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_RCTM_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_TIME], 0));
	          
	hWndPageItems[LBL_RCTM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                   ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                   (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[CB_RCTM2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                           8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_RCTM2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_RCTM2], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2], 0);
	
	hWndPageItems[EDT_RCTM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                 ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RCTM_MSGS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RCTM_MSGS2], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RCTM_MSGS2], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_RCTM_MSGS2], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_MESSAGES2], 0));
	          
	hWndPageItems[LBL_RCTM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                    ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[EDT_RCTM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                 ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RCTM_SECS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RCTM_SECS2], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RCTM_SECS2], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_RCTM_SECS2], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_TIME2], 0));
	          
	hWndPageItems[LBL_RCTM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                    ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                    (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	iPosY += iTwoLineGB;
	
	hWndPageItems[GB_DEFLOOD_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DEFLOOD_TEMP_BAN_TIME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                           0, iPosY, ((rcThis.right - rcThis.left - 5) / 2) - 2, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                           
	hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                            8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_DEFLOOD_TEMP_BAN_TIME, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME], EM_SETLIMITTEXT, 5, 0);
	
	AddUpDown(hWndPageItems[UD_DEFLOOD_TEMP_BAN_TIME], ScaleGui(40) + 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(32767, 1),
	          (WPARAM)hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME], 0));
	          
	hWndPageItems[LBL_DEFLOOD_TEMP_BAN_TIME_MINUTES] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MINUTES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                                    ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                                    ((rcThis.right - rcThis.left - 5) / 2) - clsGuiSettingManager::iUpDownWidth - ScaleGui(40) - 23, clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                    
	hWndPageItems[GB_MAX_USERS_LOGINS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MAX_USERS_LOGINS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                      ((rcThis.right - rcThis.left - 5) / 2) + 3, iPosY, (rcThis.right - rcThis.left) - (((rcThis.right - rcThis.left - 5) / 2) + 8), clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[EDT_MAX_USERS_LOGINS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                       ((rcThis.right - rcThis.left - 5) / 2) + 11, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAX_USERS_LOGINS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAX_USERS_LOGINS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAX_USERS_LOGINS], ((rcThis.right - rcThis.left - 5) / 2) + ScaleGui(40) + 11, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(1000, 1),
	          (WPARAM)hWndPageItems[EDT_MAX_USERS_LOGINS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SIMULTANEOUS_LOGINS], 0));
	          
	hWndPageItems[LBL_MAX_USERS_LOGINS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_PER_10_SECONDS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                       ((rcThis.right - rcThis.left - 5) / 2) + ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 16, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                       (rcThis.right - rcThis.left) - ((rcThis.right - rcThis.left - 5) / 2) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 29, clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                       
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2], clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER2], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_CTM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_CTM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_CTM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_CTM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_CTM_SECONDS], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_CTM_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_CTM_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER2], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_CTM_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_CTM_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_CTM_SECONDS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_RCTM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_RCTM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_RCTM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_RCTM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_RCTM_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_RCTM_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER2], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_RCTM_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_RCTM_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
	
	clsGuiSettingManager::wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_MAX_USERS_LOGINS], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageDeflood::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_DEFLOOD];
}
//------------------------------------------------------------------------------

void SettingPageDeflood::FocusLastItem()
{
	::SetFocus(hWndPageItems[EDT_MAX_USERS_LOGINS]);
}
//------------------------------------------------------------------------------
