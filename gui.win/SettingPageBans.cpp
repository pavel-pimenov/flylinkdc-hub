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

SettingPageBans::SettingPageBans() : bUpdateTempBanRedirAddress(false), bUpdatePermBanRedirAddress(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageBans::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case EDT_DEFAULT_TEMPBAN_TIME:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 32767);
				
				return 0;
			}
			
			break;
		case EDT_ADD_MESSAGE:
		case EDT_TEMP_BAN_REDIR:
		case EDT_PERM_BAN_REDIR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemovePipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case BTN_TEMP_BAN_REDIR_ENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_TEMP_BAN_REDIR],
				               ::SendMessage(hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case BTN_PERM_BAN_REDIR_ENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_PERM_BAN_REDIR],
				               ::SendMessage(hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[LBL_TEMP_BAN_TIME], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(hWndPageItems[EDT_TEMP_BAN_TIME], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(hWndPageItems[UD_TEMP_BAN_TIME], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(hWndPageItems[LBL_TEMP_BAN_TIME_HOURS], ui32Action == 2 ? TRUE : FALSE);
				::EnableWindow(hWndPageItems[BTN_REPORT_3X_BAD_PASS], ui32Action == 0 ? FALSE : TRUE);
			}
			
			break;
		case EDT_TEMP_BAN_TIME:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 999);
				
				return 0;
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageBans::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	LRESULT lResult = ::SendMessage(hWndPageItems[UD_DEFAULT_TEMPBAN_TIME], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_DEFAULT_TEMP_BAN_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_BAN_MSG_SHOW_IP, ::SendMessage(hWndPageItems[BTN_SHOW_IP], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_BAN_MSG_SHOW_RANGE, ::SendMessage(hWndPageItems[BTN_SHOW_RANGE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_BAN_MSG_SHOW_NICK, ::SendMessage(hWndPageItems[BTN_SHOW_NICK], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_BAN_MSG_SHOW_REASON, ::SendMessage(hWndPageItems[BTN_SHOW_REASON], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_BAN_MSG_SHOW_BY, ::SendMessage(hWndPageItems[BTN_SHOW_BY], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	char buf[257];
	int iLen = ::GetWindowText(hWndPageItems[EDT_ADD_MESSAGE], buf, 257);
	clsSettingManager::mPtr->SetText(SETTXT_MSG_TO_ADD_TO_BAN_MSG, buf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_TEMP_BAN_REDIR, ::SendMessage(hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_TEMP_BAN_REDIR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS]) != NULL))
	{
		bUpdateTempBanRedirAddress = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_TEMP_BAN_REDIR_ADDRESS, buf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_PERM_BAN_REDIR, ::SendMessage(hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_PERM_BAN_REDIR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS]) != NULL))
	{
		bUpdatePermBanRedirAddress = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_PERM_BAN_REDIR_ADDRESS, buf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_ADVANCED_PASS_PROTECTION,
	                                 ::SendMessage(hWndPageItems[BTN_ENABLE_ADVANCED_PASSWORD_PROTECTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	                                 
	clsSettingManager::mPtr->SetShort(SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE, (int16_t)::SendMessage(hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_GETCURSEL, 0, 0));
	
	lResult = ::SendMessage(hWndPageItems[UD_TEMP_BAN_TIME], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_REPORT_3X_BAD_PASS, ::SendMessage(hWndPageItems[BTN_REPORT_3X_BAD_PASS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageBans::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                 bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                 bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                 bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                 bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & bUpdatedTempBanRedirAddress,
                                 bool & bUpdatedPermBanRedirAddress, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if (bUpdatedTempBanRedirAddress == false)
	{
		bUpdatedTempBanRedirAddress = bUpdateTempBanRedirAddress;
	}
	if (bUpdatedPermBanRedirAddress == false)
	{
		bUpdatedPermBanRedirAddress = bUpdatePermBanRedirAddress;
	}
}

//------------------------------------------------------------------------------
bool SettingPageBans::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	hWndPageItems[GB_DEFAULT_TEMPBAN_TIME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_TIME_AFTER_ETC], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                          0, 0, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                          
	hWndPageItems[EDT_DEFAULT_TEMPBAN_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                           8, clsGuiSettingManager::iGroupBoxMargin, iFullEDT / 2, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_DEFAULT_TEMPBAN_TIME, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_DEFAULT_TEMPBAN_TIME], EM_SETLIMITTEXT, 5, 0);
	
	AddUpDown(hWndPageItems[UD_DEFAULT_TEMPBAN_TIME], (iFullEDT / 2) + 8, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(32767, 1),
	          (WPARAM)hWndPageItems[EDT_DEFAULT_TEMPBAN_TIME], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME], 0));
	          
	hWndPageItems[LBL_DEFAULT_TEMPBAN_TIME_MINUTES] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_MINUTES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                                   (iFullEDT / 2) + clsGuiSettingManager::iUpDownWidth + 13, clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), (rcThis.right - rcThis.left) - ((iFullEDT / 2) + clsGuiSettingManager::iUpDownWidth + 18), clsGuiSettingManager::iTextHeight,
	                                                                   m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                   
	int iPosY = clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_BAN_MESSAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_BAN_MSG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 0, iPosY, iFullGB, clsGuiSettingManager::iGroupBoxMargin + (5 * clsGuiSettingManager::iCheckHeight) + 13 + clsGuiSettingManager::iOneLineGB + 5, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[BTN_SHOW_IP] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SHOW_BANNED_IP], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                              8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SHOW_IP], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_BAN_MSG_SHOW_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_SHOW_RANGE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SHOW_BANNED_RANGE], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                 8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 3, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SHOW_RANGE], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_BAN_MSG_SHOW_RANGE] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_SHOW_NICK] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SHOW_BANNED_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                8, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 6, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SHOW_NICK], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_BAN_MSG_SHOW_NICK] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_SHOW_REASON] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SHOW_BAN_REASON], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                  8, iPosY + clsGuiSettingManager::iGroupBoxMargin + (3 * clsGuiSettingManager::iCheckHeight) + 9, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SHOW_REASON], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_BAN_MSG_SHOW_REASON] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_SHOW_BY] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SHOW_BAN_WHO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                              8, iPosY + clsGuiSettingManager::iGroupBoxMargin + (4 * clsGuiSettingManager::iCheckHeight) + 12, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SHOW_BY], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_BAN_MSG_SHOW_BY] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[GB_ADD_MESSAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_BAN_MSG_ADD_MSG], WS_CHILD | WS_VISIBLE |
	                                                 BS_GROUPBOX, 5, iPosY + clsGuiSettingManager::iGroupBoxMargin + (5 * clsGuiSettingManager::iCheckHeight) + 13, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[EDT_ADD_MESSAGE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG], WS_CHILD | WS_VISIBLE |
	                                                  WS_TABSTOP | ES_AUTOHSCROLL, 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + (5 * clsGuiSettingManager::iCheckHeight) + 13 + clsGuiSettingManager::iGroupBoxMargin, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_ADD_MESSAGE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_ADD_MESSAGE], EM_SETLIMITTEXT, 256, 0);
	
	iPosY += clsGuiSettingManager::iGroupBoxMargin + (5 * clsGuiSettingManager::iCheckHeight) + 13 + clsGuiSettingManager::iOneLineGB + 5;
	
	hWndPageItems[GB_TEMP_BAN_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN_REDIR_ADDRESS], WS_CHILD |
	                                                    WS_VISIBLE | BS_GROUPBOX, 0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                            8, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_TEMP_BAN_REDIR_ENABLE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_TEMP_BAN_REDIR_ENABLE], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_TEMP_BAN_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_TEMP_BAN_REDIR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
	                                                     WS_TABSTOP | ES_AUTOHSCROLL, ScaleGui(85) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(85) + 26), clsGuiSettingManager::iEditHeight,
	                                                     m_hWnd, (HMENU)EDT_TEMP_BAN_REDIR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_TEMP_BAN_REDIR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_TEMP_BAN_REDIR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_PERM_BAN_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN_REDIR_ADDRESS], WS_CHILD |
	                                                    WS_VISIBLE | BS_GROUPBOX, 0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                            8, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_PERM_BAN_REDIR_ENABLE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_PERM_BAN_REDIR_ENABLE], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_PERM_BAN_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_PERM_BAN_REDIR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
	                                                     WS_TABSTOP | ES_AUTOHSCROLL, ScaleGui(85) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(85) + 26), clsGuiSettingManager::iEditHeight,
	                                                     m_hWnd, (HMENU)EDT_PERM_BAN_REDIR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_PERM_BAN_REDIR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_PERM_BAN_REDIR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_PASSWORD_PROTECTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PASSWORD_PROTECTION],
	                                                         WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, iPosY, iFullGB, (2 * clsGuiSettingManager::iGroupBoxMargin) + (2 * clsGuiSettingManager::iCheckHeight) + 10 + (2 * clsGuiSettingManager::iEditHeight) + 13, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                         
	hWndPageItems[BTN_ENABLE_ADVANCED_PASSWORD_PROTECTION] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ADV_PASS_PRTCTN], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                                          BS_AUTOCHECKBOX, 8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_ENABLE_ADVANCED_PASSWORD_PROTECTION], BM_SETCHECK,
	              (clsSettingManager::mPtr->bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	              
	hWndPageItems[GB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_BRUTE_FORCE_PASS_PRTC_ACT],
	                                                                            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1, iGBinGB, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iEditHeight) + clsGuiSettingManager::iCheckHeight + 17, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                            
	hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                                            13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_DISABLED]);
	::SendMessage(hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PERM_BAN]);
	::SendMessage(hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_TEMP_BAN]);
	::SendMessage(hWndPageItems[CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE], 0);
	
	hWndPageItems[LBL_TEMP_BAN_TIME] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_TEMPORARY_BAN_TIME], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                    13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 6 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(225), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[EDT_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                    ScaleGui(225) + 18, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 6, ScaleGui(80), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_TEMP_BAN_TIME, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_TEMP_BAN_TIME], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_TEMP_BAN_TIME], ScaleGui(225) + 18 + ScaleGui(80), iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 6, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight,
	          (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_TEMP_BAN_TIME], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME], 0));
	          
	hWndPageItems[LBL_TEMP_BAN_TIME_HOURS] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_HOURS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                          ScaleGui(225) + 18 + ScaleGui(80) + clsGuiSettingManager::iUpDownWidth + 5, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 6 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2),
	                                                          (rcThis.right - rcThis.left) - (ScaleGui(225) + 18 + ScaleGui(80) + clsGuiSettingManager::iUpDownWidth + 18), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                          
	hWndPageItems[BTN_REPORT_3X_BAD_PASS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REPORT_BAD_PASS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                         13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + (2 * clsGuiSettingManager::iEditHeight) + 10, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REPORT_3X_BAD_PASS], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REPORT_3X_BAD_PASS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(hWndPageItems[EDT_TEMP_BAN_REDIR], clsSettingManager::mPtr->bBools[SETBOOL_TEMP_BAN_REDIR] == true ? TRUE : FALSE);
	
	::EnableWindow(hWndPageItems[EDT_PERM_BAN_REDIR], clsSettingManager::mPtr->bBools[SETBOOL_PERM_BAN_REDIR] == true ? TRUE : FALSE);
	
	::EnableWindow(hWndPageItems[LBL_TEMP_BAN_TIME], clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[EDT_TEMP_BAN_TIME], clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[UD_TEMP_BAN_TIME], clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[LBL_TEMP_BAN_TIME_HOURS], clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 2 ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[BTN_REPORT_3X_BAD_PASS], clsSettingManager::mPtr->i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 0 ? FALSE : TRUE);
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_REPORT_3X_BAD_PASS], GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageBans::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_BAN_HANDLING];
}
//------------------------------------------------------------------------------

void SettingPageBans::FocusLastItem()
{
	::SetFocus(hWndPageItems[BTN_REPORT_3X_BAD_PASS]);
}
//------------------------------------------------------------------------------
