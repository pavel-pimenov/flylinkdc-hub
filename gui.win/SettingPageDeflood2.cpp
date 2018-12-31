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
#include "SettingPageDeflood2.h"
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

SettingPageDeflood2::SettingPageDeflood2()
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageDeflood2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case EDT_PM_MSGS:
		case EDT_PM_SECS:
		case EDT_PM_MSGS2:
		case EDT_PM_SECS2:
		case EDT_PM_INTERVAL_MSGS:
		case EDT_PM_INTERVAL_SECS:
		case EDT_SAME_PM_MSGS:
		case EDT_SAME_PM_SECS:
		case EDT_WARN_COUNT:
		case EDT_NEW_CONNS_FROM_SAME_IP_COUNT:
		case EDT_NEW_CONNS_FROM_SAME_IP_TIME:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 999);
				
				return 0;
			}
			
			break;
		case EDT_SAME_MULTI_PM_MSGS:
		case EDT_SAME_MULTI_PM_LINES:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 2, 999);
				
				return 0;
			}
			
			break;
		case EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);
				
				return 0;
			}
			
			break;
		case EDT_RECEIVED_DATA_KB:
		case EDT_RECEIVED_DATA_SECS:
		case EDT_RECEIVED_DATA_KB2:
		case EDT_RECEIVED_DATA_SECS2:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 9999);
				
				return 0;
			}
			
			break;
		case EDT_MAX_USERS_FROM_SAME_IP:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 256);
				
				return 0;
			}
			
			break;
		case CB_PM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_PM], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_PM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_PM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_PM2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_PM2], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_PM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_PM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_PM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_PM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_PM_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_PM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_SAME_PM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_PM], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_SAME_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_PM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_SAME_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_PM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_SAME_MULTI_PM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_WITH], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_LINES], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_LINES], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_LINES], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_RECEIVED_DATA:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_KB], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_KB], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_SECS], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_SECONDS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case CB_RECEIVED_DATA2:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_KB2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_KB2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_SECS2], ui32Action == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageDeflood2::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_PM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_PM], CB_GETCURSEL, 0, 0));
	
	LRESULT lResult = ::SendMessage(hWndPageItems[UD_PM_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_PM_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_PM_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_PM_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_PM2], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_MSGS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_PM_MESSAGES2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_SECS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_PM_TIME2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_INTERVAL_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_PM_INTERVAL_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_INTERVAL_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_PM_INTERVAL_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_SAME_PM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_PM], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_PM_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_PM_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_PM_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_PM_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MULTI_PM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_PM_MSGS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MULTI_PM_MESSAGES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_PM_LINES], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_SAME_MULTI_PM_LINES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_PM_COUNT_TO_USER, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_MAX_DOWN_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_KB], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_DOWN_KB, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_SECS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_DOWN_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_MAX_DOWN_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_KB2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_DOWN_KB2, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_SECS2], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_DOWN_TIME2, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_DEFLOOD_WARNING_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_WARN_COUNT], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_DEFLOOD_WARNING_COUNT, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_COUNT], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_NEW_CONNECTIONS_COUNT, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_TIME], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_NEW_CONNECTIONS_TIME, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAX_USERS_FROM_SAME_IP], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_CONN_SAME_IP, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_DEFLOOD_REPORT, ::SendMessage(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageDeflood2::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                     bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                     bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                     bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                     bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                     bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
}
//------------------------------------------------------------------------------

bool SettingPageDeflood2::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	hWndPageItems[GB_PM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PRIVATE_MSGS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, iFullGB, iThreeLineGB,
	                                        m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                        
	hWndPageItems[CB_PM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                        8, clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_PM, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_PM], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION], 0);
	
	hWndPageItems[EDT_PM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                              ScaleGui(180) + 13, clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_PM_MSGS], ScaleGui(220) + 13, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_MSGS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_MESSAGES], 0));
	          
	hWndPageItems[LBL_PM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                 ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[EDT_PM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                              ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_SECS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_PM_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_SECS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_TIME], 0));
	          
	hWndPageItems[LBL_PM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                 ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                 (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[CB_PM2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                         8, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_PM2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_PM2], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2], 0);
	
	hWndPageItems[EDT_PM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                               ScaleGui(180) + 13, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_MSGS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_MSGS2], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_PM_MSGS2], ScaleGui(220) + 13, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_MSGS2],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_MESSAGES2], 0));
	          
	hWndPageItems[LBL_PM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                  ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_PM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                               ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_SECS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_SECS2], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_PM_SECS2], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_PM_SECS2], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_TIME2], 0));
	          
	hWndPageItems[LBL_PM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                  ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                  (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[LBL_PM_INTERVAL] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_INTERVAL], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                  8, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(180), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_PM_INTERVAL_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                       ScaleGui(180) + 13, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_INTERVAL_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_INTERVAL_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_PM_INTERVAL_MSGS], ScaleGui(220) + 13, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_PM_INTERVAL_MSGS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_INTERVAL_MESSAGES], 0));
	          
	hWndPageItems[LBL_PM_INTERVAL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                          ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                          
	hWndPageItems[EDT_PM_INTERVAL_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                       ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_INTERVAL_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_INTERVAL_SECS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_PM_INTERVAL_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_PM_INTERVAL_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_INTERVAL_TIME], 0));
	          
	hWndPageItems[LBL_PM_INTERVAL_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                          ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + 10 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                          (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                          
	int iPosY = iThreeLineGB;
	
	hWndPageItems[GB_SAME_PM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SAME_PRIVATE_MSGS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                             0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB,  m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                             
	hWndPageItems[CB_SAME_PM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                             8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_SAME_PM, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_PM], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION], 0);
	
	hWndPageItems[EDT_SAME_PM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                   ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_PM_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_PM_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_PM_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 2), (WPARAM)hWndPageItems[EDT_SAME_PM_MSGS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_MESSAGES], 0));
	          
	hWndPageItems[LBL_SAME_PM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                      ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[EDT_SAME_PM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                   ScaleGui(230) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_PM_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_PM_SECS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_PM_SECS], ScaleGui(270) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_SAME_PM_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_TIME], 0));
	          
	hWndPageItems[LBL_SAME_PM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                      ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                      (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_SAME_MULTI_PM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SAME_MULTI_PRIVATE_MSGS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                   0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[CB_SAME_MULTI_PM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                   8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_SAME_MULTI_PM, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION], 0);
	
	hWndPageItems[EDT_SAME_MULTI_PM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                         ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_MULTI_PM_MSGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_MULTI_PM_MSGS], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_MULTI_PM_MSGS], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 2),
	          (WPARAM)hWndPageItems[EDT_SAME_MULTI_PM_MSGS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_MESSAGES], 0));
	          
	hWndPageItems[LBL_SAME_MULTI_PM_WITH] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_WITH_LWR], WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                         ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(30), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                         
	hWndPageItems[EDT_SAME_MULTI_PM_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                          ScaleGui(250) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SAME_MULTI_PM_LINES, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SAME_MULTI_PM_LINES], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_SAME_MULTI_PM_LINES], ScaleGui(290) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 2),
	          (WPARAM)hWndPageItems[EDT_SAME_MULTI_PM_LINES], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_LINES], 0));
	          
	hWndPageItems[LBL_SAME_MULTI_PM_LINES] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_LINES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                          ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                          (rcThis.right - rcThis.left) - (ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                          
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PM_LIM_TO_URS_FRM_MULTI_NICKS],
	                                                                              WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                              
	hWndPageItems[LBL_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAX_MSGS_TO_ONE_USER_PER_MIN],
	                                                                               WS_CHILD | WS_VISIBLE | SS_LEFT, 8, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), (rcThis.right - rcThis.left) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 24, clsGuiSettingManager::iTextHeight,
	                                                                               m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                               
	hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                                               (rcThis.right - rcThis.left) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight,
	                                                                               m_hWnd, (HMENU)EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], (rcThis.right - rcThis.left) - clsGuiSettingManager::iUpDownWidth - 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight,
	          (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_COUNT_TO_USER], 0));
	          
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_RECEIVED_DATA] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_RECV_DATA_FROM_USER], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                   0, iPosY, iFullGB, iTwoLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[CB_RECEIVED_DATA] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                   8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_RECEIVED_DATA, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_ACTION], 0);
	
	hWndPageItems[EDT_RECEIVED_DATA_KB] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                       ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RECEIVED_DATA_KB, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_KB], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RECEIVED_DATA_KB], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_KB], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_KB], 0));
	          
	hWndPageItems[LBL_RECEIVED_DATA_DIVIDER] = ::CreateWindowEx(0, WC_STATIC,
	                                                            (string(clsLanguageManager::mPtr->sTexts[LAN_LWR_KILO_BYTES], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_LWR_KILO_BYTES]) + " /").c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                            ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(30), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                            
	hWndPageItems[EDT_RECEIVED_DATA_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                         ScaleGui(250) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RECEIVED_DATA_SECS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_SECS], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RECEIVED_DATA_SECS], ScaleGui(290) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_SECS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_TIME], 0));
	          
	hWndPageItems[LBL_RECEIVED_DATA_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                            ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                            (rcThis.right - rcThis.left) - (ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                            
	hWndPageItems[CB_RECEIVED_DATA2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                    8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_RECEIVED_DATA2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_IGNORE]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_WARN]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_ACTION2], 0);
	
	hWndPageItems[EDT_RECEIVED_DATA_KB2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                        ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RECEIVED_DATA_KB2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_KB2], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RECEIVED_DATA_KB2], ScaleGui(220) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_KB2], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_KB2], 0));
	          
	hWndPageItems[LBL_RECEIVED_DATA_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC,
	                                                             (string(clsLanguageManager::mPtr->sTexts[LAN_LWR_KILO_BYTES], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_LWR_KILO_BYTES]) + " /").c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                             ScaleGui(220) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(30), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                             
	hWndPageItems[EDT_RECEIVED_DATA_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                          ScaleGui(250) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RECEIVED_DATA_SECS2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_SECS2], EM_SETLIMITTEXT, 4, 0);
	
	AddUpDown(hWndPageItems[UD_RECEIVED_DATA_SECS2], ScaleGui(290) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 1),
	          (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_SECS2], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_DOWN_TIME2], 0));
	          
	hWndPageItems[LBL_RECEIVED_DATA_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                             ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 28, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                             (rcThis.right - rcThis.left) - (ScaleGui(290) + (2 * clsGuiSettingManager::iUpDownWidth) + 41), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                             
	iPosY += iTwoLineGB;
	
	hWndPageItems[GB_WARN_SETTING] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_WARN_SET], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[CB_WARN_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(180), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_RECEIVED_DATA2, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISCONNECT]);
	::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KICK]);
	::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFLOOD_WARNING_ACTION], 0);
	
	hWndPageItems[LBL_WARN_AFTER] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_AFTER], WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                 ScaleGui(180) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(40), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[EDT_WARN_COUNT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                 ScaleGui(220) + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_WARN_COUNT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_WARN_COUNT], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_WARN_COUNT], ScaleGui(260) + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_WARN_COUNT],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFLOOD_WARNING_COUNT], 0));
	          
	hWndPageItems[LBL_WARN_WARNINGS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_WARNINGS], WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                    ScaleGui(260) + clsGuiSettingManager::iUpDownWidth + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                    (rcThis.right - rcThis.left) - (ScaleGui(260) + clsGuiSettingManager::iUpDownWidth + 28), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_NEW_CONNS_FROM_SAME_IP] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NEW_CONNS_FRM_SINGLE_IP_LIM], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                            0, iPosY, ((rcThis.right - rcThis.left - 5) / 2) - 2, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                            
	hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_COUNT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                                   8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_NEW_CONNS_FROM_SAME_IP_COUNT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_COUNT], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_COUNT], ScaleGui(40) + 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_COUNT], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_NEW_CONNECTIONS_COUNT], 0));
	          
	hWndPageItems[LBL_NEW_CONNS_FROM_SAME_IP_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                                     ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                     
	hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                                  ScaleGui(50) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_NEW_CONNS_FROM_SAME_IP_TIME, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_TIME], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_TIME], ScaleGui(90) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 1),
	          (WPARAM)hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_TIME], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_NEW_CONNECTIONS_TIME], 0));
	          
	hWndPageItems[LBL_NEW_CONNS_FROM_SAME_IP_SECONDS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                                     ScaleGui(90) + (2 * clsGuiSettingManager::iUpDownWidth) + 23, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                                     ((rcThis.right - rcThis.left - 5) / 2) - (2 * clsGuiSettingManager::iUpDownWidth) - ScaleGui(90) - 33, clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                     
	hWndPageItems[GB_MAX_USERS_FROM_SAME_IP] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MAX_USR_SAME_IP], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                            ((rcThis.right - rcThis.left - 5) / 2) + 3, iPosY, (rcThis.right - rcThis.left) - (((rcThis.right - rcThis.left - 5) / 2) + 8), clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                            
	hWndPageItems[EDT_MAX_USERS_FROM_SAME_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                             (((rcThis.right - rcThis.left - 5) / 4) * 3) + 3 - ((ScaleGui(40) + clsGuiSettingManager::iUpDownWidth) / 2), iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight,
	                                                             m_hWnd, (HMENU)EDT_MAX_USERS_FROM_SAME_IP, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAX_USERS_FROM_SAME_IP], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MAX_USERS_FROM_SAME_IP], (((rcThis.right - rcThis.left - 5) / 4) * 3) + ScaleGui(40) + 3 - ((ScaleGui(40) + clsGuiSettingManager::iUpDownWidth) / 2), iPosY + clsGuiSettingManager::iGroupBoxMargin,
	          clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(256, 1), (WPARAM)hWndPageItems[EDT_MAX_USERS_FROM_SAME_IP], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CONN_SAME_IP], 0));
	          
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[BTN_REPORT_FLOOD_TO_OPS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REPORT_FLOOD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                          0, iPosY + 5, iFullGB, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_DEFLOOD_REPORT] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(hWndPageItems[EDT_PM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_PM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_PM_DIVIDER], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_PM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_PM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_PM_SECONDS], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_PM_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_PM_MSGS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_PM_DIVIDER2], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_PM_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_PM_SECS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_PM_SECONDS2], clsSettingManager::mPtr->i16Shorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_SAME_PM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_PM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_PM_DIVIDER], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_SAME_PM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_PM_SECS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_PM_SECONDS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_MSGS], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_WITH], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_LINES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_LINES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_LINES], clsSettingManager::mPtr->i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS], GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageDeflood2::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_MORE_DEFLOOD];
}
//------------------------------------------------------------------------------

void SettingPageDeflood2::FocusLastItem()
{
	::SetFocus(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS]);
}
//------------------------------------------------------------------------------
