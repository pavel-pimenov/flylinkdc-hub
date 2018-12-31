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
#include "SettingPageGeneral2.h"
//---------------------------------------------------------------------------
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------

SettingPageGeneral2::SettingPageGeneral2() : bUpdateTextFiles(false), bUpdateRedirectAddress(false), bUpdateRegOnlyMessage(false), bUpdateShareLimitMessage(false), bUpdateSlotsLimitMessage(false),
	bUpdateHubSlotRatioMessage(false), bUpdateMaxHubsLimitMessage(false), bUpdateNoTagMessage(false), bUpdateTempBanRedirAddress(false), bUpdatePermBanRedirAddress(false),
	bUpdateNickLimitMessage(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageGeneral2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case EDT_OWNER_EMAIL:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemoveDollarsPipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case EDT_MAIN_REDIR_ADDR:
		case EDT_MSG_TO_NON_REGS:
		case EDT_NON_REG_REDIR_ADDR:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemovePipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case BTN_ENABLE_TEXT_FILES:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM],
				               ::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case BTN_DONT_ALLOW_PINGER:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnable = ::SendMessage(hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
				::EnableWindow(hWndPageItems[BTN_REPORT_PINGER], bEnable);
				::EnableWindow(hWndPageItems[EDT_OWNER_EMAIL], bEnable);
			}
			
			break;
		case BTN_REDIR_ALL:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[BTN_REDIR_HUB_FULL],
				               ::SendMessage(hWndPageItems[BTN_REDIR_ALL], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE);
			}
			
			break;
		case BTN_ALLOW_ONLY_REGS:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnable = ::SendMessage(hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
				::EnableWindow(hWndPageItems[EDT_MSG_TO_NON_REGS], bEnable);
				::EnableWindow(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], bEnable);
				::EnableWindow(hWndPageItems[EDT_NON_REG_REDIR_ADDR],
				               (bEnable != FALSE && ::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE);
			}
			
			break;
		case BTN_NON_REG_REDIR_ENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(hWndPageItems[EDT_NON_REG_REDIR_ADDR],
				               ::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		}
	}
	
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	if ((::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TEXT_FILES])
	{
		bUpdateTextFiles = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_ENABLE_TEXT_FILES, ::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_SEND_TEXT_FILES_AS_PM, ::SendMessage(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_DONT_ALLOW_PINGERS, ::SendMessage(hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_REPORT_PINGERS, ::SendMessage(hWndPageItems[BTN_REPORT_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	char buf[257];
	int iLen = ::GetWindowText(hWndPageItems[EDT_OWNER_EMAIL], buf, 257);
	clsSettingManager::mPtr->SetText(SETTXT_HUB_OWNER_EMAIL, buf, iLen);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_MAIN_REDIR_ADDR], buf, 257);
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS] == NULL && iLen != 0) || (clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS] != NULL && strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS]) != NULL))
	{
		bUpdateRedirectAddress = true;
		bUpdateRegOnlyMessage = true;
		bUpdateShareLimitMessage = true;
		bUpdateSlotsLimitMessage = true;
		bUpdateHubSlotRatioMessage = true;
		bUpdateMaxHubsLimitMessage = true;
		bUpdateNoTagMessage = true;
		bUpdateTempBanRedirAddress = true;
		bUpdatePermBanRedirAddress = true;
		bUpdateNickLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_REDIRECT_ADDRESS, buf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_REDIRECT_ALL, ::SendMessage(hWndPageItems[BTN_REDIR_ALL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_REDIRECT_WHEN_HUB_FULL, ::SendMessage(hWndPageItems[BTN_REDIR_HUB_FULL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_REG_ONLY, ::SendMessage(hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_MSG_TO_NON_REGS], buf, 257);
	
	if (bUpdateRegOnlyMessage == false &&
	        ((::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY_REDIR] ||
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_REG_ONLY_MSG]) != NULL))
	{
		bUpdateRegOnlyMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_REG_ONLY_MSG, buf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_REG_ONLY_REDIR, ::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_NON_REG_REDIR_ADDR], buf, 257);
	
	if (bUpdateRegOnlyMessage == false &&
	        (clsSettingManager::mPtr->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS]) != NULL))
	{
		bUpdateRegOnlyMessage = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_REG_ONLY_REDIR_ADDRESS, buf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_KEEP_SLOW_USERS, ::SendMessage(hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_HASH_PASSWORDS, ::SendMessage(hWndPageItems[BTN_HASH_PASSWORDS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	clsSettingManager::mPtr->SetBool(SETBOOL_NO_QUACK_SUPPORTS, ::SendMessage(hWndPageItems[BTN_KILL_THAT_DUCK], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_HASH_PASSWORDS] == true)
	{
		clsRegManager::mPtr->HashPasswords();
	}
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
                                     bool & /*bUpdatedAutoReg*/, bool &/*bUpdatedMOTD*/, bool &/*bUpdatedHubSec*/, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
                                     bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage,
                                     bool &bUpdatedNickLimitMessage, bool &/*bUpdatedBotsSameNick*/, bool &/*bUpdatedBotNick*/, bool &/*bUpdatedBot*/, bool &/*bUpdatedOpChatNick*/,
                                     bool &/*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool &bUpdatedTextFiles, bool &bUpdatedRedirectAddress, bool &bUpdatedTempBanRedirAddress,
                                     bool &bUpdatedPermBanRedirAddress, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if (bUpdatedTextFiles == false)
	{
		bUpdatedTextFiles = bUpdateTextFiles;
	}
	if (bUpdatedRedirectAddress == false)
	{
		bUpdatedRedirectAddress = bUpdateRedirectAddress;
	}
	if (bUpdatedRegOnlyMessage == false)
	{
		bUpdatedRegOnlyMessage = bUpdateRegOnlyMessage;
	}
	if (bUpdatedShareLimitMessage == false)
	{
		bUpdatedShareLimitMessage = bUpdateShareLimitMessage;
	}
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
	if (bUpdatedNoTagMessage == false)
	{
		bUpdatedNoTagMessage = bUpdateNoTagMessage;
	}
	if (bUpdatedTempBanRedirAddress == false)
	{
		bUpdatedTempBanRedirAddress = bUpdateTempBanRedirAddress;
	}
	if (bUpdatedPermBanRedirAddress == false)
	{
		bUpdatedPermBanRedirAddress = bUpdatePermBanRedirAddress;
	}
	if (bUpdatedNickLimitMessage == false)
	{
		bUpdatedNickLimitMessage = bUpdateNickLimitMessage;
	}
}

//------------------------------------------------------------------------------
bool SettingPageGeneral2::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	hWndPageItems[GB_TEXT_FILES] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_TEXT_FILES], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                0, 0, iFullGB, iTwoChecksGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[BTN_ENABLE_TEXT_FILES] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_TEXT_FILES], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                        8, clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_ENABLE_TEXT_FILES, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TEXT_FILES] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_TEXT_FILES_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                            8, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 3, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_SEND_TEXT_FILES_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	int iPosY = iTwoChecksGB;
	
	hWndPageItems[GB_PINGER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PINGER], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                            0, iPosY, iFullGB, clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 4 + clsGuiSettingManager::iOneLineGB + 5, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                            
	hWndPageItems[BTN_DONT_ALLOW_PINGER] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DISALLOW_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                        8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_DONT_ALLOW_PINGER, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_REPORT_PINGER] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REPORT_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                    8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 3, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REPORT_PINGER], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REPORT_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[GB_OWNER_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_OWNER_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 5, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 4, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[EDT_OWNER_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_HUB_OWNER_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                  13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + (2 * clsGuiSettingManager::iCheckHeight) + 4, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_OWNER_EMAIL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_OWNER_EMAIL], EM_SETLIMITTEXT, 64, 0);
	
	iPosY += clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 4 + clsGuiSettingManager::iOneLineGB + 5;
	
	hWndPageItems[GB_MAIN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MAIN_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                     0, iPosY, iFullGB, clsGuiSettingManager::iOneLineTwoChecksGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                     
	hWndPageItems[EDT_MAIN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                      8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MAIN_REDIR_ADDR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAIN_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	
	hWndPageItems[BTN_REDIR_ALL] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ALL_CONN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 4, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_REDIR_ALL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REDIR_ALL], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REDIRECT_ALL] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_REDIR_HUB_FULL] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_FULL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                     8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iCheckHeight + 7, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_REDIR_HUB_FULL], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	iPosY += clsGuiSettingManager::iOneLineTwoChecksGB;
	
	hWndPageItems[GB_REG_ONLY_HUB] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REG_ONLY_HUB], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  0, iPosY, iFullGB, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + (2 * clsGuiSettingManager::iOneLineGB) + 6, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[BTN_ALLOW_ONLY_REGS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ALLOW_ONLY_REGS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                      8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_ALLOW_ONLY_REGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[GB_MSG_TO_NON_REGS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REG_ONLY_MSG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                     5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                     
	hWndPageItems[EDT_MSG_TO_NON_REGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_REG_ONLY_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                      13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_MSG_TO_NON_REGS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MSG_TO_NON_REGS], EM_SETLIMITTEXT, 256, 0);
	
	hWndPageItems[GB_NON_REG_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                   5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineGB, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[BTN_NON_REG_REDIR_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                           13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineGB + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), ScaleGui(85), clsGuiSettingManager::iCheckHeight,
	                                                           m_hWnd, (HMENU)BTN_NON_REG_REDIR_ENABLE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[EDT_NON_REG_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                         ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineGB, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), clsGuiSettingManager::iEditHeight,
	                                                         m_hWnd, (HMENU)EDT_NON_REG_REDIR_ADDR, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_NON_REG_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(hWndPageItems[EDT_NON_REG_REDIR_ADDR], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_HINT]);
	
	iPosY +=  clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + (2 * clsGuiSettingManager::iOneLineGB) + 6;
	
	hWndPageItems[GB_EXPERTS_ONLY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_EXPERTS_ONLY], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  0, iPosY, iFullGB, clsGuiSettingManager::iGroupBoxMargin + (3 * clsGuiSettingManager::iCheckHeight) + 14, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_KEEP_SLOW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                               8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_KEEP_SLOW_USERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_HASH_PASSWORDS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_STORE_REG_PASS_HASHED], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                     8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 3, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_HASH_PASSWORDS], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_HASH_PASSWORDS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_KILL_THAT_DUCK] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DISALLOW_BUGGY_SUPPORTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                     8, iPosY + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 6, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_KILL_THAT_DUCK], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_NO_QUACK_SUPPORTS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TEXT_FILES] == true ? TRUE : FALSE);
	
	::EnableWindow(hWndPageItems[BTN_REPORT_PINGER], clsSettingManager::mPtr->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[EDT_OWNER_EMAIL], clsSettingManager::mPtr->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[BTN_REDIR_HUB_FULL], clsSettingManager::mPtr->bBools[SETBOOL_REDIRECT_ALL] == true ? FALSE : TRUE);
	
	::EnableWindow(hWndPageItems[EDT_MSG_TO_NON_REGS], clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY] == true ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[BTN_NON_REG_REDIR_ENABLE], clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY] == true ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[EDT_NON_REG_REDIR_ADDR],
	               (clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY] == true && clsSettingManager::mPtr->bBools[SETBOOL_REG_ONLY_REDIR] == true) ? TRUE : FALSE);
	               
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_KILL_THAT_DUCK], GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageGeneral2::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_MORE_GENERAL];
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::FocusLastItem()
{
	::SetFocus(hWndPageItems[BTN_KILL_THAT_DUCK]);
}
//------------------------------------------------------------------------------
