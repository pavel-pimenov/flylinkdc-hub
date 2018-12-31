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

SettingPageRules::SettingPageRules() : bUpdateNickLimitMessage(false), bUpdateMinShare(false), bUpdateMaxShare(false), bUpdateShareLimitMessage(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageRules::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case EDT_NICK_LEN_MSG:
		case EDT_NICK_LEN_REDIR_ADDR:
		case EDT_SHARE_MSG:
		case EDT_SHARE_REDIR_ADDR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemovePipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case EDT_MIN_NICK_LEN:
		case EDT_MAX_NICK_LEN:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 64);
				
				uint16_t ui16Min = 0, ui16Max = 0;
				
				LRESULT lResult = ::SendMessage(hWndPageItems[UD_MIN_NICK_LEN], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Min = LOWORD(lResult);
				}
				
				lResult = ::SendMessage(hWndPageItems[UD_MAX_NICK_LEN], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Max = LOWORD(lResult);
				}
				
				if (ui16Min == 0 && ui16Max == 0)
				{
					::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], FALSE);
					::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], FALSE);
					::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], TRUE);
					::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], TRUE);
					::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}
				
				return 0;
			}
			
			break;
		case BTN_NICK_LEN_REDIR:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case EDT_MIN_SHARE:
		case EDT_MAX_SHARE:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 9999);
				
				uint16_t ui16Min = 0, ui16Max = 0;
				
				LRESULT lResult = ::SendMessage(hWndPageItems[UD_MIN_SHARE], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Min = LOWORD(lResult);
				}
				
				lResult = ::SendMessage(hWndPageItems[UD_MAX_SHARE], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Max = LOWORD(lResult);
				}
				
				if (ui16Min == 0 && ui16Max == 0)
				{
					::EnableWindow(hWndPageItems[EDT_SHARE_MSG], FALSE);
					::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], FALSE);
					::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(hWndPageItems[EDT_SHARE_MSG], TRUE);
					::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], TRUE);
					::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}
				
				return 0;
			}
			
			break;
		case BTN_SHARE_REDIR:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case UD_MAIN_CHAT_LEN:
		case UD_PM_LEN:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 32767);
				
				return 0;
			}
			
			break;
		case UD_MAIN_CHAT_LINES:
		case UD_PM_LINES:
		case UD_SEARCH_MIN_LEN:
		case UD_SEARCH_MAX_LEN:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);
				
				return 0;
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageRules::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	LRESULT lResult = ::SendMessage(hWndPageItems[UD_MIN_NICK_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_NICK_LEN])
		{
			bUpdateNickLimitMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_MIN_NICK_LEN, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAX_NICK_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_NICK_LEN])
		{
			bUpdateNickLimitMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_NICK_LEN, LOWORD(lResult));
	}
	
	char buf[257];
	int iLen = ::GetWindowText(hWndPageItems[EDT_NICK_LEN_MSG], buf, 257);
	
	if (strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_NICK_LIMIT_MSG]) != NULL)
	{
		bUpdateNickLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_NICK_LIMIT_MSG, buf, iLen);
	
	bool bNickRedir = ::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bNickRedir != clsSettingManager::mPtr->bBools[SETBOOL_NICK_LIMIT_REDIR])
	{
		bUpdateNickLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_NICK_LIMIT_REDIR, bNickRedir);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS]) != NULL))
	{
		bUpdateNickLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_NICK_LIMIT_REDIR_ADDRESS, buf, iLen);
	
	lResult = ::SendMessage(hWndPageItems[UD_MIN_SHARE], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SHARE_LIMIT])
		{
			bUpdateMinShare = true;
			bUpdateShareLimitMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_MIN_SHARE_LIMIT, LOWORD(lResult));
	}
	
	uint16_t ui16MinShareUnits = (int16_t)::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_GETCURSEL, 0, 0);
	
	if (ui16MinShareUnits != clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SHARE_UNITS])
	{
		bUpdateMinShare = true;
		bUpdateShareLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_MIN_SHARE_UNITS, ui16MinShareUnits);
	
	lResult = ::SendMessage(hWndPageItems[UD_MAX_SHARE], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SHARE_LIMIT])
		{
			bUpdateMaxShare = true;
			bUpdateShareLimitMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_SHARE_LIMIT, LOWORD(lResult));
	}
	
	uint16_t ui16MaxShareUnits = (int16_t)::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_GETCURSEL, 0, 0);
	
	if (ui16MaxShareUnits != clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SHARE_UNITS])
	{
		bUpdateMaxShare = true;
		bUpdateShareLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_MAX_SHARE_UNITS, ui16MaxShareUnits);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_SHARE_MSG], buf, 257);
	
	if (strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_SHARE_LIMIT_MSG]) != NULL)
	{
		bUpdateShareLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_SHARE_LIMIT_MSG, buf, iLen);
	
	bool bShareRedir = ::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bShareRedir != clsSettingManager::mPtr->bBools[SETBOOL_SHARE_LIMIT_REDIR])
	{
		bUpdateShareLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_SHARE_LIMIT_REDIR, bShareRedir);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_SHARE_REDIR_ADDR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS]) != NULL))
	{
		bUpdateShareLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_SHARE_LIMIT_REDIR_ADDRESS, buf, iLen);
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_CHAT_LEN, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_LINES], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_CHAT_LINES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_PM_LEN, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_PM_LINES], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_PM_LINES, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SEARCH_MIN_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MIN_SEARCH_LEN, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SEARCH_MAX_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_SEARCH_LEN, LOWORD(lResult));
	}
}
//------------------------------------------------------------------------------

void SettingPageRules::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                  bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool &bUpdatedShareLimitMessage,
                                  bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                  bool &bUpdatedNickLimitMessage, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                  bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                  bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool &bUpdatedMinShare, bool &bUpdatedMaxShare)
{
	if (bUpdatedNickLimitMessage == false)
	{
		bUpdatedNickLimitMessage = bUpdateNickLimitMessage;
	}
	if (bUpdatedShareLimitMessage == false)
	{
		bUpdatedShareLimitMessage = bUpdateShareLimitMessage;
	}
	if (bUpdatedMinShare == false)
	{
		bUpdatedMinShare = bUpdateMinShare;
	}
	if (bUpdatedMaxShare == false)
	{
		bUpdatedMaxShare = bUpdateMaxShare;
	}
}

//------------------------------------------------------------------------------

bool SettingPageRules::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	hWndPageItems[GB_NICK_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NICK_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 0, 0, iFullGB, iOneLineTwoGroupGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[EDT_MIN_NICK_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                   8, clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MIN_NICK_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MIN_NICK_LEN], EM_SETLIMITTEXT, 2, 0);
	AddToolTip(hWndPageItems[EDT_MIN_NICK_LEN], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_MIN_NICK_LEN], ScaleGui(40) + 8, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(64, 0), (WPARAM)hWndPageItems[EDT_MIN_NICK_LEN],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_NICK_LEN], 0));
	          
	hWndPageItems[LBL_MIN_NICK_LEN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MIN_LEN], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                   ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                   ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 15), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[LBL_MAX_NICK_LEN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAX_LEN], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                   ((rcThis.right - rcThis.left - 16) / 2) + 2, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                   ((rcThis.right - rcThis.left) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[EDT_MAX_NICK_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                   (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth - ScaleGui(40), clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAX_NICK_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAX_NICK_LEN], EM_SETLIMITTEXT, 2, 0);
	AddToolTip(hWndPageItems[EDT_MAX_NICK_LEN], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_MAX_NICK_LEN], (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(64, 0),
	          (WPARAM)hWndPageItems[EDT_MAX_NICK_LEN], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_NICK_LEN], 0));
	          
	hWndPageItems[GB_NICK_LEN_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_NICK_LEN_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_NICK_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                   13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + 2, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_NICK_LEN_MSG, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_NICK_LEN_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_NICK_LEN_MSG], clsLanguageManager::mPtr->sTexts[LAN_NICK_LIMIT_MSG_HINT]);
	
	hWndPageItems[GB_NICK_LEN_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                    5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[BTN_NICK_LEN_REDIR] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                     13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_NICK_LEN_REDIR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_NICK_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_NICK_LEN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                          ES_AUTOHSCROLL, ScaleGui(85) + 18, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), clsGuiSettingManager::iEditHeight,
	                                                          m_hWnd, (HMENU)EDT_NICK_LEN_REDIR_ADDR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	int iPosY = iOneLineTwoGroupGB;
	
	hWndPageItems[GB_SHARE_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SHARE_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  0, iPosY, iFullGB, iOneLineTwoGroupGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_MIN_SHARE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MIN_SHARE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MIN_SHARE], EM_SETLIMITTEXT, 4, 0);
	AddToolTip(hWndPageItems[EDT_MIN_SHARE], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_MIN_SHARE], ScaleGui(40) + 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 0), (WPARAM)hWndPageItems[EDT_MIN_SHARE],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SHARE_LIMIT], 0));
	          
	hWndPageItems[CB_MIN_SHARE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                               ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(50), clsGuiSettingManager::iEditHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_BYTES]);
	::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KILO_BYTES]);
	::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_MEGA_BYTES]);
	::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
	::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TERA_BYTES]);
	::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SHARE_UNITS], 0);
	
	hWndPageItems[LBL_MIN_SHARE] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MIN_SHARE], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                ScaleGui(90) + clsGuiSettingManager::iUpDownWidth + 18, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(90) + clsGuiSettingManager::iUpDownWidth + 20), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[LBL_MAX_SHARE] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAX_SHARE], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                ((rcThis.right - rcThis.left) - ScaleGui(90) - clsGuiSettingManager::iUpDownWidth - 23) - (((rcThis.right - rcThis.left - 16) / 2) + 2), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[EDT_MAX_SHARE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                (rcThis.right - rcThis.left) - clsGuiSettingManager::iUpDownWidth - ScaleGui(90) - 18, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAX_SHARE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAX_SHARE], EM_SETLIMITTEXT, 4, 0);
	AddToolTip(hWndPageItems[EDT_MAX_SHARE], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_MAX_SHARE], (rcThis.right - rcThis.left) - clsGuiSettingManager::iUpDownWidth - ScaleGui(50) - 18, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(9999, 0),
	          (WPARAM)hWndPageItems[EDT_MAX_SHARE], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SHARE_LIMIT], 0));
	          
	hWndPageItems[CB_MAX_SHARE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                               (rcThis.right - rcThis.left) - ScaleGui(50) - 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(50), clsGuiSettingManager::iEditHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_BYTES]);
	::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_KILO_BYTES]);
	::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_MEGA_BYTES]);
	::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_GIGA_BYTES]);
	::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TERA_BYTES]);
	::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SHARE_UNITS], 0);
	
	hWndPageItems[GB_SHARE_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_SHARE_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_SHARE_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + 2, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SHARE_MSG, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SHARE_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_SHARE_MSG], clsLanguageManager::mPtr->sTexts[LAN_SHARE_LIMIT_MSG_HINT]);
	
	hWndPageItems[GB_SHARE_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[BTN_SHARE_REDIR] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                  13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_SHARE_REDIR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_SHARE_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_SHARE_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                       ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), clsGuiSettingManager::iEditHeight,
	                                                       m_hWnd, (HMENU)EDT_NICK_LEN_REDIR_ADDR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SHARE_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_SHARE_REDIR_ADDR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	iPosY *= 2;
	
	hWndPageItems[GB_MAIN_CHAT_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MAIN_CHAT_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                      0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[EDT_MAIN_CHAT_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                    8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_LEN], EM_SETLIMITTEXT, 5, 0);
	AddToolTip(hWndPageItems[EDT_MAIN_CHAT_LEN], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_LEN], ScaleGui(40) + 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(32767, 0), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_LEN],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CHAT_LEN], 0));
	          
	hWndPageItems[LBL_MAIN_CHAT_LEN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_LENGTH], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                    ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                    ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 15), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[LBL_MAIN_CHAT_LINES] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_LINES], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                      ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                      ((rcThis.right - rcThis.left) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[EDT_MAIN_CHAT_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                      (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth - ScaleGui(40), iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_CHAT_LINES, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_CHAT_LINES], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_MAIN_CHAT_LINES], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_MAIN_CHAT_LINES], (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)hWndPageItems[EDT_MAIN_CHAT_LINES], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CHAT_LINES], 0));
	          
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_PM_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PM_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_PM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                             8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_LEN], EM_SETLIMITTEXT, 5, 0);
	AddToolTip(hWndPageItems[EDT_PM_LEN], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_PM_LEN], ScaleGui(40) + 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(32767, 0), (WPARAM)hWndPageItems[EDT_PM_LEN],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_LEN], 0));
	          
	hWndPageItems[LBL_PM_LEN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_LENGTH], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                             ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                             ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 15), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                             
	hWndPageItems[LBL_PM_LINES] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_LINES], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                               ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                               ((rcThis.right - rcThis.left) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_PM_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                               (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth - ScaleGui(40), iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_PM_LINES, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PM_LINES], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_PM_LINES], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_PM_LINES], (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)hWndPageItems[EDT_PM_LINES], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_PM_LINES], 0));
	          
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_SEARCH_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SEARCH_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                   0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[EDT_SEARCH_MIN_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                     8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_MIN_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SEARCH_MIN_LEN], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_SEARCH_MIN_LEN], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_SEARCH_MIN_LEN], ScaleGui(40) + 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SEARCH_MIN_LEN],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SEARCH_LEN], 0));
	          
	hWndPageItems[LBL_SEARCH_MIN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MINIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                 ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                 ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 15), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[LBL_SEARCH_MAX] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                 ((rcThis.right - rcThis.left - 16) / 2) + 2, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                 ((rcThis.right - rcThis.left) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[EDT_SEARCH_MAX_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                     (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth - ScaleGui(40), iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_MAX_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SEARCH_MAX_LEN], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_SEARCH_MAX_LEN], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_SEARCH_MAX_LEN], (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)hWndPageItems[EDT_SEARCH_MAX_LEN], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SEARCH_LEN], 0));
	          
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	if (clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SHARE_LIMIT] == 0 && clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SHARE_LIMIT] == 0)
	{
		::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], FALSE);
		::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], FALSE);
		::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], TRUE);
		::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], TRUE);
		::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], clsSettingManager::mPtr->bBools[SETBOOL_NICK_LIMIT_REDIR] == true ? TRUE : FALSE);
	}
	
	if (clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_NICK_LEN] == 0 && clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_NICK_LEN] == 0)
	{
		::EnableWindow(hWndPageItems[EDT_SHARE_MSG], FALSE);
		::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], FALSE);
		::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(hWndPageItems[EDT_SHARE_MSG], TRUE);
		::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], TRUE);
		::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], clsSettingManager::mPtr->bBools[SETBOOL_SHARE_LIMIT_REDIR] == true ? TRUE : FALSE);
	}
	
	clsGuiSettingManager::wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_SEARCH_MAX_LEN], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageRules::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_RULES];
}
//------------------------------------------------------------------------------

void SettingPageRules::FocusLastItem()
{
	::SetFocus(hWndPageItems[EDT_SEARCH_MAX_LEN]);
}
//------------------------------------------------------------------------------
