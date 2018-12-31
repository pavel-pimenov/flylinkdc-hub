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
#include "SettingPageRules2.h"
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

SettingPageRules2::SettingPageRules2() : bUpdateSlotsLimitMessage(false), bUpdateHubSlotRatioMessage(false), bUpdateMaxHubsLimitMessage(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageRules2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case EDT_SLOTS_MSG:
		case EDT_SLOTS_REDIR_ADDR:
		case EDT_HUBS_SLOTS_MSG:
		case EDT_HUBS_SLOTS_REDIR_ADDR:
		case EDT_HUBS_MSG:
		case EDT_HUBS_REDIR_ADDR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemovePipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case EDT_SLOTS_MIN:
		case EDT_SLOTS_MAX:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);
				
				uint16_t ui16Min = 0, ui16Max = 0;
				
				LRESULT lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MIN], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Min = LOWORD(lResult);
				}
				
				lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MAX], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Max = LOWORD(lResult);
				}
				
				if (ui16Min == 0 && ui16Max == 0)
				{
					::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], FALSE);
					::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], FALSE);
					::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], TRUE);
					::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], TRUE);
					::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}
				
				return 0;
			}
			
			break;
		case BTN_SLOTS_REDIR:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case EDT_HUBS:
		case EDT_SLOTS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);
				
				uint16_t ui16Hubs = 0, ui16Slots = 0;
				
				LRESULT lResult = ::SendMessage(hWndPageItems[UD_HUBS], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Hubs = LOWORD(lResult);
				}
				
				lResult = ::SendMessage(hWndPageItems[UD_SLOTS], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Slots = LOWORD(lResult);
				}
				
				if (ui16Hubs == 0 || ui16Slots == 0)
				{
					::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], FALSE);
					::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], FALSE);
					::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], TRUE);
					::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], TRUE);
					::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}
				
				return 0;
			}
			
			break;
		case BTN_HUBS_SLOTS_REDIR:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case EDT_MAX_HUBS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);
				
				uint16_t ui16Hubs = 0;
				
				LRESULT lResult = ::SendMessage(hWndPageItems[UD_MAX_HUBS], UDM_GETPOS, 0, 0);
				if (HIWORD(lResult) == 0)
				{
					ui16Hubs = LOWORD(lResult);
				}
				
				if (ui16Hubs == 0)
				{
					::EnableWindow(hWndPageItems[EDT_HUBS_MSG], FALSE);
					::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], FALSE);
					::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(hWndPageItems[EDT_HUBS_MSG], TRUE);
					::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], TRUE);
					::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}
				
				return 0;
			}
			
			break;
		case BTN_HUBS_REDIR:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case EDT_CTM_LEN:
		case EDT_RCTM_LEN:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 512);
				
				return 0;
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageRules2::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	LRESULT lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MIN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SLOTS_LIMIT])
		{
			bUpdateSlotsLimitMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_MIN_SLOTS_LIMIT, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MAX], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SLOTS_LIMIT])
		{
			bUpdateSlotsLimitMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_SLOTS_LIMIT, LOWORD(lResult));
	}
	
	char buf[257];
	int iLen = ::GetWindowText(hWndPageItems[EDT_SLOTS_MSG], buf, 257);
	
	if (strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_SLOTS_LIMIT_MSG]) != NULL)
	{
		bUpdateSlotsLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_SLOTS_LIMIT_MSG, buf, iLen);
	
	bool bSlotsRedir = ::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bSlotsRedir != clsSettingManager::mPtr->bBools[SETBOOL_SLOTS_LIMIT_REDIR])
	{
		bUpdateSlotsLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_SLOTS_LIMIT_REDIR, bSlotsRedir);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_SLOTS_REDIR_ADDR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS]) != NULL))
	{
		bUpdateSlotsLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_SLOTS_LIMIT_REDIR_ADDRESS, buf, iLen);
	
	lResult = ::SendMessage(hWndPageItems[UD_HUBS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS])
		{
			bUpdateHubSlotRatioMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_HUB_SLOT_RATIO_HUBS, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_SLOTS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS])
		{
			bUpdateHubSlotRatioMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_HUB_SLOT_RATIO_SLOTS, LOWORD(lResult));
	}
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_SLOTS_MSG], buf, 257);
	
	if (strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_HUB_SLOT_RATIO_MSG]) != NULL)
	{
		bUpdateHubSlotRatioMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_HUB_SLOT_RATIO_MSG, buf, iLen);
	
	bool bHubsSlotsRedir = ::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bHubsSlotsRedir != clsSettingManager::mPtr->bBools[SETBOOL_HUB_SLOT_RATIO_REDIR])
	{
		bUpdateHubSlotRatioMessage = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_HUB_SLOT_RATIO_REDIR, bHubsSlotsRedir);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS]) != NULL))
	{
		bUpdateHubSlotRatioMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS, buf, iLen);
	
	lResult = ::SendMessage(hWndPageItems[UD_MAX_HUBS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		if (LOWORD(lResult) != clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_HUBS_LIMIT])
		{
			bUpdateMaxHubsLimitMessage = true;
		}
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_HUBS_LIMIT, LOWORD(lResult));
	}
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_MSG], buf, 257);
	
	if (strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]) != NULL)
	{
		bUpdateMaxHubsLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_MAX_HUBS_LIMIT_MSG, buf, iLen);
	
	bool bHubsRedir = ::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bHubsRedir != clsSettingManager::mPtr->bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR])
	{
		bUpdateMaxHubsLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_MAX_HUBS_LIMIT_REDIR, bHubsRedir);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_REDIR_ADDR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS]) != NULL))
	{
		bUpdateMaxHubsLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS, buf, iLen);
	
	lResult = ::SendMessage(hWndPageItems[UD_CTM_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_CTM_LEN, LOWORD(lResult));
	}
	
	lResult = ::SendMessage(hWndPageItems[UD_RCTM_LEN], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_RCTM_LEN, LOWORD(lResult));
	}
}
//------------------------------------------------------------------------------

void SettingPageRules2::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                   bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                   bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool & /*bUpdatedNoTagMessage*/,
                                   bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                   bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                   bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if (bUpdatedSlotsLimitMessage == false)
	{
		bUpdatedSlotsLimitMessage = bUpdateSlotsLimitMessage;
	}
	if (bUpdatedHubSlotRatioMessage == false)
	{
		bUpdatedHubSlotRatioMessage = bUpdateHubSlotRatioMessage;
	}
	if (bUpdatedMaxHubsLimitMessage == false)
	{
		bUpdatedMaxHubsLimitMessage = bUpdateMaxHubsLimitMessage;
	}
}

//------------------------------------------------------------------------------

bool SettingPageRules2::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	hWndPageItems[GB_SLOTS_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SLOTS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  0, 0, iFullGB, iOneLineTwoGroupGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_SLOTS_MIN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                8, clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SLOTS_MIN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SLOTS_MIN], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_SLOTS_MIN], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_SLOTS_MIN], ScaleGui(40) + 8, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SLOTS_MIN],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SLOTS_LIMIT], 0));
	          
	hWndPageItems[LBL_SLOTS_MIN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MIN_SLOTS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 15), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[LBL_SLOTS_MAX] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAX_SLOTS], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                                ((rcThis.right - rcThis.left - 16) / 2) + 2, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                ((rcThis.right - rcThis.left) - ScaleGui(40) - clsGuiSettingManager::iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[EDT_SLOTS_MAX] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth - ScaleGui(40), clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SLOTS_MAX, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SLOTS_MAX], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_SLOTS_MAX], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_SLOTS_MAX], (rcThis.right - rcThis.left) - 13 - clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)hWndPageItems[EDT_SLOTS_MAX], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SLOTS_LIMIT], 0));
	          
	hWndPageItems[GB_SLOTS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_SLOTS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_SLOTS_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + 2, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SLOTS_MSG, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SLOTS_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_SLOTS_MSG], clsLanguageManager::mPtr->sTexts[LAN_SLOT_LIMIT_MSG_HINT]);
	
	hWndPageItems[GB_SLOTS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[BTN_SLOTS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                  13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_SLOTS_REDIR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_SLOTS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                       ES_AUTOHSCROLL, ScaleGui(85) + 18, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), clsGuiSettingManager::iEditHeight,
	                                                       m_hWnd, (HMENU)EDT_SLOTS_REDIR_ADDR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SLOTS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_SLOTS_REDIR_ADDR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	int iPosY = iOneLineTwoGroupGB;
	
	hWndPageItems[GB_HUBS_SLOTS_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_SLOT_RATIO_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                       0, iPosY, iFullGB, iOneLineTwoGroupGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                       
	hWndPageItems[LBL_HUBS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_HUBS], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                           8, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2) - clsGuiSettingManager::iUpDownWidth - ScaleGui(40) - 18, clsGuiSettingManager::iTextHeight,
	                                           m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                           
	hWndPageItems[EDT_HUBS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                           (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2) - clsGuiSettingManager::iUpDownWidth - ScaleGui(40) - 5, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight,
	                                           m_hWnd, (HMENU)EDT_HUBS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUBS], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_HUBS], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_HUBS], (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2) - clsGuiSettingManager::iUpDownWidth - 5, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight,
	          (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_HUBS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS], 0));
	          
	hWndPageItems[LBL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                              (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2), iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(10), clsGuiSettingManager::iTextHeight,
	                                              m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[EDT_SLOTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                            (((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + 5, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_SLOTS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_SLOTS], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_SLOTS], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_SLOTS], (((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + ScaleGui(40) + 5, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight,
	          (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SLOTS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS], 0));
	          
	hWndPageItems[LBL_SLOTS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SLOTS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                            (((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 10, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                            (rcThis.right - rcThis.left) - ((((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 23), clsGuiSettingManager::iTextHeight,
	                                            m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                            
	hWndPageItems[GB_HUBS_SLOTS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                    5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[EDT_HUBS_SLOTS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_HUB_SLOT_RATIO_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                     13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + 2, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUBS_SLOTS_MSG, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUBS_SLOTS_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_HUBS_SLOTS_MSG], clsLanguageManager::mPtr->sTexts[LAN_HUB_SLOT_RATIO_MSG_HINT]);
	
	hWndPageItems[GB_HUBS_SLOTS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                      5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[BTN_HUBS_SLOTS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                       13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_HUBS_SLOTS_REDIR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                            ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), clsGuiSettingManager::iEditHeight,
	                                                            m_hWnd, (HMENU)EDT_HUBS_SLOTS_REDIR_ADDR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	iPosY *= 2;
	
	hWndPageItems[GB_HUBS_LIMIT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MAX_HUBS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                0, iPosY, iFullGB, iOneLineTwoGroupGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[EDT_MAX_HUBS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                               8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAX_HUBS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAX_HUBS], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(hWndPageItems[EDT_MAX_HUBS], clsLanguageManager::mPtr->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);
	
	AddUpDown(hWndPageItems[UD_MAX_HUBS], ScaleGui(40) + 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_MAX_HUBS],
	          (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_HUBS_LIMIT], 0));
	          
	hWndPageItems[LBL_MAX_HUBS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAX_HUBS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                               ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                               ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + clsGuiSettingManager::iUpDownWidth + 15), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[GB_HUBS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                              5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[EDT_HUBS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                               13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + 2, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUBS_MSG, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUBS_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_HUBS_MSG], clsLanguageManager::mPtr->sTexts[LAN_HUB_LIMIT_MSG_HINT]);
	
	hWndPageItems[GB_HUBS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[BTN_HUBS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                 13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_HUBS_REDIR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_HUBS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                      ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), clsGuiSettingManager::iEditHeight,
	                                                      m_hWnd, (HMENU)EDT_HUBS_REDIR_ADDR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUBS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_HUBS_REDIR_ADDR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	iPosY += iOneLineTwoGroupGB;
	
	hWndPageItems[GB_CTM_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CTM_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                             0, iPosY, ((rcThis.right - rcThis.left - 5) / 2) - 2, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                             
	hWndPageItems[LBL_CTM_LEN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                              8, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ((rcThis.right - rcThis.left - 5) / 2) - clsGuiSettingManager::iUpDownWidth - ScaleGui(40) - 23, clsGuiSettingManager::iTextHeight,
	                                              m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[EDT_CTM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                              ((rcThis.right - rcThis.left - 5) / 2) - clsGuiSettingManager::iUpDownWidth - ScaleGui(40) - 10, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_CTM_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_CTM_LEN], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_CTM_LEN], ((rcThis.right - rcThis.left - 5) / 2) - clsGuiSettingManager::iUpDownWidth - 10, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(512, 1),
	          (WPARAM)hWndPageItems[EDT_CTM_LEN], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_CTM_LEN], 0));
	          
	hWndPageItems[GB_RCTM_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_RCTM_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                              ((rcThis.right - rcThis.left - 5) / 2) + 3, iPosY, (rcThis.right - rcThis.left) - (((rcThis.right - rcThis.left - 5) / 2) + 8), clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[LBL_RCTM_LEN] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                               ((rcThis.right - rcThis.left - 5) / 2) + 11, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                               (rcThis.right - rcThis.left) - ((rcThis.right - rcThis.left - 5) / 2) - clsGuiSettingManager::iUpDownWidth - ScaleGui(40) - 29, clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_RCTM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                               (rcThis.right - rcThis.left) - clsGuiSettingManager::iUpDownWidth - ScaleGui(40) - 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(40), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_RCTM_LEN, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_RCTM_LEN], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_RCTM_LEN], (rcThis.right - rcThis.left) - clsGuiSettingManager::iUpDownWidth - 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(512, 1),
	          (WPARAM)hWndPageItems[EDT_RCTM_LEN], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_RCTM_LEN], 0));
	          
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	if (clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_SLOTS_LIMIT] == 0 && clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_SLOTS_LIMIT] == 0)
	{
		::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], FALSE);
		::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], FALSE);
		::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], TRUE);
		::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], TRUE);
		::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], clsSettingManager::mPtr->bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true ? TRUE : FALSE);
	}
	
	if (clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS] == 0 || clsSettingManager::mPtr->i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] == 0)
	{
		::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], FALSE);
		::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], FALSE);
		::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], TRUE);
		::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], TRUE);
		::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], clsSettingManager::mPtr->bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true ? TRUE : FALSE);
	}
	
	if (clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_HUBS_LIMIT] == 0)
	{
		::EnableWindow(hWndPageItems[EDT_HUBS_MSG], FALSE);
		::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], FALSE);
		::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(hWndPageItems[EDT_HUBS_MSG], TRUE);
		::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], TRUE);
		::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], clsSettingManager::mPtr->bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true ? TRUE : FALSE);
	}
	
	clsGuiSettingManager::wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_RCTM_LEN], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageRules2::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_MORE_RULES];
}
//------------------------------------------------------------------------------

void SettingPageRules2::FocusLastItem()
{
	::SetFocus(hWndPageItems[EDT_RCTM_LEN]);
}
//------------------------------------------------------------------------------
