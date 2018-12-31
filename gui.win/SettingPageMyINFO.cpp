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

SettingPageMyINFO::SettingPageMyINFO() : bUpdateNoTagMessage(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageMyINFO::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case CB_NO_TAG_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t ui32NoTagAction = (uint32_t)::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0);
				::EnableWindow(hWndPageItems[EDT_NO_TAG_MESSAGE], ui32NoTagAction != 0 ? TRUE : FALSE);
				::EnableWindow(hWndPageItems[EDT_NO_TAG_REDIRECT], ui32NoTagAction == 2 ? TRUE : FALSE);
				::EnableWindow(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], ui32NoTagAction == 0 ? TRUE : FALSE);
			}
			
			break;
		case EDT_NO_TAG_MESSAGE:
		case EDT_NO_TAG_REDIRECT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemovePipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);
				
				return 0;
			}
			
			break;
		case CB_ORIGINAL_MYINFO_ACTION:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				BOOL bEnable = ((uint32_t)::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_GETCURSEL, 0, 0) == 0 ? FALSE : TRUE);
				::EnableWindow(hWndPageItems[BTN_REMOVE_DESCRIPTION], bEnable);
				::EnableWindow(hWndPageItems[BTN_REMOVE_TAG], bEnable);
				::EnableWindow(hWndPageItems[BTN_REMOVE_CONNECTION], bEnable);
				::EnableWindow(hWndPageItems[BTN_REMOVE_EMAIL], bEnable);
				::EnableWindow(hWndPageItems[BTN_MODE_TO_MYINFO], bEnable);
				::EnableWindow(hWndPageItems[BTN_MODE_TO_DESCRIPTION], bEnable);
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	char buf[257];
	int iLen = ::GetWindowText(hWndPageItems[EDT_NO_TAG_MESSAGE], buf, 257);
	
	if ((int16_t)::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0) != clsSettingManager::mPtr->i16Shorts[SETSHORT_NO_TAG_OPTION] ||
	        strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_NO_TAG_MSG]) != NULL)
	{
		bUpdateNoTagMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_NO_TAG_MSG, buf, iLen);
	
	clsSettingManager::mPtr->SetShort(SETSHORT_NO_TAG_OPTION, (int16_t)::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0));
	
	iLen = ::GetWindowText(hWndPageItems[EDT_NO_TAG_REDIRECT], buf, 257);
	
	if (bUpdateNoTagMessage == false && ((clsSettingManager::mPtr->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] == NULL && iLen != 0) ||
	                                     (clsSettingManager::mPtr->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != NULL &&
	                                      strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS]) != NULL)))
	{
		bUpdateNoTagMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_NO_TAG_REDIR_ADDRESS, buf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_REPORT_SUSPICIOUS_TAG, ::SendMessage(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	LRESULT lResult = ::SendMessage(hWndPageItems[UD_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MYINFO_DELAY, LOWORD(lResult));
	}
	
	clsSettingManager::mPtr->SetShort(SETSHORT_FULL_MYINFO_OPTION, (int16_t)::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_GETCURSEL, 0, 0));
	
	clsSettingManager::mPtr->SetBool(SETBOOL_STRIP_DESCRIPTION, ::SendMessage(hWndPageItems[BTN_REMOVE_DESCRIPTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_STRIP_TAG, ::SendMessage(hWndPageItems[BTN_REMOVE_TAG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_STRIP_CONNECTION, ::SendMessage(hWndPageItems[BTN_REMOVE_CONNECTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_STRIP_EMAIL, ::SendMessage(hWndPageItems[BTN_REMOVE_EMAIL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_MODE_TO_MYINFO, ::SendMessage(hWndPageItems[BTN_MODE_TO_MYINFO], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_MODE_TO_DESCRIPTION, ::SendMessage(hWndPageItems[BTN_MODE_TO_DESCRIPTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                   bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                   bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & bUpdatedNoTagMessage,
                                   bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                   bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                   bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if (bUpdatedNoTagMessage == false)
	{
		bUpdatedNoTagMessage = bUpdateNoTagMessage;
	}
}

//------------------------------------------------------------------------------
bool SettingPageMyINFO::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	int iGBinGBinGB = (rcThis.right - rcThis.left) - 25;
	int iGBinGBinGBEDT = (rcThis.right - rcThis.left) - 41;
	
	int iPosY = (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iEditHeight + (2 * clsGuiSettingManager::iOneLineGB) + 12;
	
	hWndPageItems[GB_DESCRIPTION_TAG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION_TAG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                     0, 0, iFullGB, iPosY, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                     
	hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REPORT_BAD_TAGS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                            8, clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REPORT_SUSPICIOUS_TAG] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[GB_NO_TAG_ACTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NO_TAG_ACTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                   5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1, iGBinGB, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + (2 * clsGuiSettingManager::iOneLineGB) + 7, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[CB_NO_TAG_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                   13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)CB_NO_TAG_ACTION, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_ACCEPT]);
	::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_REJECT]);
	::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_REDIR]);
	::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_NO_TAG_OPTION], 0);
	
	hWndPageItems[GB_NO_TAG_MESSAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                    10, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 3, iGBinGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[EDT_NO_TAG_MESSAGE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_NO_TAG_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                     18, (3 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 3, iGBinGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_NO_TAG_MESSAGE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_NO_TAG_MESSAGE], EM_SETLIMITTEXT, 256, 0);
	
	hWndPageItems[GB_NO_TAG_REDIRECT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                     10, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 3 + clsGuiSettingManager::iOneLineGB, iGBinGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                     
	hWndPageItems[EDT_NO_TAG_REDIRECT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                      18, (3 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 3 + clsGuiSettingManager::iOneLineGB, iGBinGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_NO_TAG_REDIRECT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_NO_TAG_REDIRECT], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_NO_TAG_REDIRECT], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	hWndPageItems[GB_MYINFO_PROCESSING] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MYINFO_PROCESSING], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                       0, iPosY, iFullGB, (2 * clsGuiSettingManager::iGroupBoxMargin) + (2 * clsGuiSettingManager::iEditHeight) + 10 + (6 * clsGuiSettingManager::iCheckHeight) + 28, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                       
	hWndPageItems[LBL_ORIGINAL_MYINFO] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_SEND_LONG_MYINFO], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                      8, iPosY + clsGuiSettingManager::iGroupBoxMargin + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ((rcThis.right - rcThis.left) - 21) / 2, clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[CB_ORIGINAL_MYINFO_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                            (((rcThis.right - rcThis.left) - 21) / 2) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, (rcThis.right - rcThis.left) - ((((rcThis.right - rcThis.left) - 21) / 2) + 26), clsGuiSettingManager::iEditHeight,
	                                                            m_hWnd, (HMENU)CB_ORIGINAL_MYINFO_ACTION, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_FULL_MYINFO_ALL]);
	::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_FULL_MYINFO_PROFILE]);
	::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_FULL_MYINFO_NONE]);
	::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_SETCURSEL, clsSettingManager::mPtr->i16Shorts[SETSHORT_FULL_MYINFO_OPTION], 0);
	AddToolTip(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], clsLanguageManager::mPtr->sTexts[LAN_MYINFO_TO_HINT]);
	
	iPosY += clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 2;
	
	hWndPageItems[GB_MODIFIED_MYINFO_OPTIONS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_OPTIONS_FOR_SHORT_MYINFO], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                             5, iPosY, iGBinGB, clsGuiSettingManager::iGroupBoxMargin + (6 * clsGuiSettingManager::iCheckHeight) + 15 + 8, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                             
	hWndPageItems[BTN_REMOVE_DESCRIPTION] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REMOVE_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                         13, iPosY + clsGuiSettingManager::iGroupBoxMargin, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REMOVE_DESCRIPTION], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_STRIP_DESCRIPTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_REMOVE_TAG] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REMOVE_TAG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 3, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REMOVE_TAG], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_STRIP_TAG] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_REMOVE_CONNECTION] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REMOVE_CONNECTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                        13, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 6, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REMOVE_CONNECTION], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_STRIP_CONNECTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_REMOVE_EMAIL] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REMOVE_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                   13, iPosY + clsGuiSettingManager::iGroupBoxMargin + (3 * clsGuiSettingManager::iCheckHeight) + 9, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REMOVE_EMAIL], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_STRIP_EMAIL] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_MODE_TO_MYINFO] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MODE_TO_MYINFO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                     13, iPosY + clsGuiSettingManager::iGroupBoxMargin + (4 * clsGuiSettingManager::iCheckHeight) + 12, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_MODE_TO_MYINFO], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_MODE_TO_MYINFO] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_MODE_TO_DESCRIPTION] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MODE_TO_DESCR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                          13, iPosY + clsGuiSettingManager::iGroupBoxMargin + (5 * clsGuiSettingManager::iCheckHeight) + 15, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_MODE_TO_DESCRIPTION], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_MODE_TO_DESCRIPTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	iPosY += clsGuiSettingManager::iGroupBoxMargin + (6 * clsGuiSettingManager::iCheckHeight) + 15 + 8;
	
	hWndPageItems[LBL_MINUTES_BEFORE_ACCEPT_NEW_MYINFO] = ::CreateWindowEx(0, WC_STATIC, clsLanguageManager::mPtr->sTexts[LAN_DONT_SEND_MYINFO_CHANGES], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                                                       8, iPosY + 5 + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iTextHeight) / 2), ScaleGui(320), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                                       
	hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                                       ScaleGui(320) + 13, iPosY + 5, (rcThis.right - rcThis.left) - (ScaleGui(320) + clsGuiSettingManager::iUpDownWidth + 26), clsGuiSettingManager::iEditHeight,
	                                                                       m_hWnd, (HMENU)EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], EM_SETLIMITTEXT, 3, 0);
	
	AddUpDown(hWndPageItems[UD_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], (rcThis.right - rcThis.left) - clsGuiSettingManager::iUpDownWidth - 13, iPosY + 5, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight,
	          (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MYINFO_DELAY], 0));
	          
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(hWndPageItems[EDT_NO_TAG_MESSAGE], clsSettingManager::mPtr->i16Shorts[SETSHORT_NO_TAG_OPTION] != 0 ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[EDT_NO_TAG_REDIRECT], clsSettingManager::mPtr->i16Shorts[SETSHORT_NO_TAG_OPTION] == 2 ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], clsSettingManager::mPtr->i16Shorts[SETSHORT_NO_TAG_OPTION] == 0 ? TRUE : FALSE);
	
	::EnableWindow(hWndPageItems[BTN_REMOVE_DESCRIPTION], clsSettingManager::mPtr->i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[BTN_REMOVE_TAG], clsSettingManager::mPtr->i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[BTN_REMOVE_CONNECTION], clsSettingManager::mPtr->i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[BTN_REMOVE_EMAIL], clsSettingManager::mPtr->i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[BTN_MODE_TO_MYINFO], clsSettingManager::mPtr->i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[BTN_MODE_TO_DESCRIPTION], clsSettingManager::mPtr->i16Shorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
	
	clsGuiSettingManager::wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageMyINFO::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_MYINFO_PROCESSING];
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::FocusLastItem()
{
	::SetFocus(hWndPageItems[BTN_MODE_TO_DESCRIPTION]);
}
//------------------------------------------------------------------------------
