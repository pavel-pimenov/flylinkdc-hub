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
#include "SettingPageMOTD.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
static WNDPROC wpOldMultiEditProc = nullptr;
//---------------------------------------------------------------------------

LRESULT CALLBACK MultiEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return 0;
	}
	else if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)
	{
		::SendMessage(::GetParent(::GetParent(hWnd)), WM_COMMAND, IDCANCEL, 0);
		return 0;
	}
	
	return ::CallWindowProc(wpOldMultiEditProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

SettingPageMOTD::SettingPageMOTD() : bUpdateMOTD(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageMOTD::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		if (LOWORD(wParam) == EDT_MOTD)
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				int iLen = ::GetWindowTextLength((HWND)lParam);
				
				char * buf = (char *)malloc(iLen + 1);
				
				if (buf == NULL)
				{
					AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in SettingPageMOTD::PageMOTDProc\n", iLen + 1);
					return 0;
				}
				
				::GetWindowText((HWND)lParam, buf, iLen + 1);
				
				bool bChanged = false;
				
				for (uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++)
				{
					if (buf[ui16i] == '|')
					{
						strcpy(buf + ui16i, buf + ui16i + 1);
						bChanged = true;
						ui16i--;
					}
				}
				
				if (bChanged == true)
				{
					int iStart, iEnd;
					
					::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
					
					::SetWindowText((HWND)lParam, buf);
					
					::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
				}
				
				free(buf);
				
				return 0;
			}
		}
		else if (LOWORD(wParam) == BTN_DISABLE_MOTD)
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bDisableMOTD = ::SendMessage(hWndPageItems[BTN_DISABLE_MOTD], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
				::EnableWindow(hWndPageItems[EDT_MOTD], bDisableMOTD);
				::EnableWindow(hWndPageItems[BTN_MOTD_AS_PM], bDisableMOTD);
			}
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageMOTD::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	bool bMOTDAsPM = ::SendMessage(hWndPageItems[BTN_MOTD_AS_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	bool bDisableMOTD = ::SendMessage(hWndPageItems[BTN_DISABLE_MOTD], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	int iAllocLen = ::GetWindowTextLength(hWndPageItems[EDT_MOTD]);
	
	char * buf = (char *)malloc(iAllocLen + 1);
	
	if (buf == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in SettingPageMOTD::Save\n", iAllocLen + 1);
		return;
	}
	
	int iLen = ::GetWindowText(hWndPageItems[EDT_MOTD], buf, iAllocLen + 1);
	
	if ((bDisableMOTD != clsSettingManager::mPtr->bBools[SETBOOL_DISABLE_MOTD] || bMOTDAsPM != clsSettingManager::mPtr->bBools[SETBOOL_MOTD_AS_PM]) ||
	        (clsSettingManager::mPtr->sMOTD == NULL && iLen != 0) || (clsSettingManager::mPtr->sMOTD != NULL && strcmp(buf, clsSettingManager::mPtr->sMOTD) != NULL))
	{
		bUpdateMOTD = true;
	}
	
	clsSettingManager::mPtr->SetMOTD(buf, iLen);
	
	free(buf);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_MOTD_AS_PM, bMOTDAsPM);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_DISABLE_MOTD, bDisableMOTD);
}
//------------------------------------------------------------------------------

void SettingPageMOTD::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                 bool & /*bUpdateAutoReg*/, bool &bUpdatedMOTD, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                 bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                 bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                 bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                 bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if (bUpdatedMOTD == false)
	{
		bUpdatedMOTD = bUpdateMOTD;
	}
}

//------------------------------------------------------------------------------

bool SettingPageMOTD::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	hWndPageItems[GB_MOTD] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MOTD], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                          0, 0, iFullGB, (rcThis.bottom - rcThis.top) - 3, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                          
	hWndPageItems[EDT_MOTD] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sMOTD != NULL ? clsSettingManager::mPtr->sMOTD : "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
	                                           ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN, 8, clsGuiSettingManager::iGroupBoxMargin, iFullEDT, (rcThis.bottom - rcThis.top) - clsGuiSettingManager::iGroupBoxMargin - (2 * clsGuiSettingManager::iCheckHeight) - 18,
	                                           m_hWnd, (HMENU)EDT_MOTD, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MOTD], EM_SETLIMITTEXT, 64000, 0);
	
	hWndPageItems[BTN_MOTD_AS_PM] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MOTD_IN_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                 8, (rcThis.bottom - rcThis.top) - (2 * clsGuiSettingManager::iCheckHeight) - 14, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_MOTD_AS_PM], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_MOTD_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[BTN_DISABLE_MOTD] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DISABLE_MOTD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                   8, (rcThis.bottom - rcThis.top) - clsGuiSettingManager::iCheckHeight - 11, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_DISABLE_MOTD, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_DISABLE_MOTD], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_DISABLE_MOTD] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(hWndPageItems[EDT_MOTD], clsSettingManager::mPtr->bBools[SETBOOL_DISABLE_MOTD] == true ? FALSE : TRUE);
	::EnableWindow(hWndPageItems[BTN_MOTD_AS_PM], clsSettingManager::mPtr->bBools[SETBOOL_DISABLE_MOTD] == true ? FALSE : TRUE);
	
	wpOldMultiEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_MOTD], GWLP_WNDPROC, (LONG_PTR)MultiEditProc);
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_DISABLE_MOTD], GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageMOTD::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_MOTD_ONLY];
}
//------------------------------------------------------------------------------

void SettingPageMOTD::FocusLastItem()
{
	::SetFocus(hWndPageItems[BTN_DISABLE_MOTD]);
}
//------------------------------------------------------------------------------
