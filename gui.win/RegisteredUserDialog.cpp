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
#include "RegisteredUserDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/ProfileManager.h"
#include "../core/ServerManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
clsRegisteredUserDialog * clsRegisteredUserDialog::mPtr = nullptr;
//---------------------------------------------------------------------------
static ATOM atomRegisteredUserDialog = 0;
//---------------------------------------------------------------------------

clsRegisteredUserDialog::clsRegisteredUserDialog() : pRegToChange(NULL)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

clsRegisteredUserDialog::~clsRegisteredUserDialog()
{
	clsRegisteredUserDialog::mPtr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsRegisteredUserDialog::StaticRegisteredUserDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	clsRegisteredUserDialog * pRegisteredUserDialog = (clsRegisteredUserDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pRegisteredUserDialog == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pRegisteredUserDialog->RegisteredUserDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsRegisteredUserDialog::RegisteredUserDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (OnAccept() == false)
			{
				return 0;
			}
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		case (EDT_NICK+100):
			if (HIWORD(wParam) == EN_CHANGE)
			{
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);
				
				bool bChanged = false;
				
				for (uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++)
				{
					if (buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ')
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
				
				return 0;
			}
			
			break;
		case EDT_PASSWORD:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);
				
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
				
				return 0;
			}
			
			break;
		}
		
		break;
	case WM_CLOSE:
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		clsServerManager::hWndActiveDialog = nullptr;
		break;
	case WM_NCDESTROY:
	{
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_SETFOCUS:
		if (pRegToChange == NULL)
		{
			::SetFocus(m_hWndWindowItems[EDT_NICK]);
		}
		else
		{
			::SetFocus(m_hWndWindowItems[EDT_PASSWORD]);
		}
		
		return 0;
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void clsRegisteredUserDialog::DoModal(HWND hWndParent, RegUser * pReg/* = NULL*/, const char * sNick/* = NULL*/)
{
	pRegToChange = pReg;
	
	if (atomRegisteredUserDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RegisteredUserDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomRegisteredUserDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (ScaleGui(300) / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (ScaleGui(201) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRegisteredUserDialog), clsLanguageManager::mPtr->sTexts[LAN_REGISTERED_USER],
	                                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(300), ScaleGui(201),
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRegisteredUserDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	{
		int iHeight = (3 * clsGuiSettingManager::iOneLineGB) + clsGuiSettingManager::iEditHeight + 6;
		
		int iDiff = rcParent.bottom - iHeight;
		
		if (iDiff != 0)
		{
			::GetWindowRect(hWndParent, &rcParent);
			
			iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - ((ScaleGui(196) - iDiff) / 2);
			
			::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
			
			::SetWindowPos(m_hWndWindowItems[WINDOW_HANDLE], NULL, iX, iY, (rcParent.right - rcParent.left), (rcParent.bottom - rcParent.top) - iDiff, SWP_NOZORDER);
		}
	}
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[GB_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                              3, 0, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                              
	m_hWndWindowItems[EDT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                               11, clsGuiSettingManager::iGroupBoxMargin, rcParent.right - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(EDT_NICK + 100), clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_NICK], EM_SETLIMITTEXT, 64, 0);
	
	int iPosY = clsGuiSettingManager::iOneLineGB;
	
	m_hWndWindowItems[GB_PASSWORD] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PASSWORD], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  3, iPosY, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                  
	m_hWndWindowItems[EDT_PASSWORD] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                   11, iPosY + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right - rcParent.left) - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_PASSWORD, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_PASSWORD], EM_SETLIMITTEXT, 64, 0);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	m_hWndWindowItems[GB_PROFILE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PROFILE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 3, iPosY, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                 
	m_hWndWindowItems[CB_PROFILE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                 11, iPosY + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right - rcParent.left) - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_PROFILE, clsServerManager::hInstance, NULL);
	                                                 
	iPosY += clsGuiSettingManager::iOneLineGB + 4;
	
	m_hWndWindowItems[BTN_ACCEPT] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                 2, iPosY, ((rcParent.right - rcParent.left) / 2) - 3, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, clsServerManager::hInstance, NULL);
	                                                 
	m_hWndWindowItems[BTN_DISCARD] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                  ((rcParent.right - rcParent.left) / 2) + 2, iPosY, ((rcParent.right - rcParent.left) / 2) - 4, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, clsServerManager::hInstance, NULL);
	                                                  
	for (uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if (m_hWndWindowItems[ui8i] == NULL)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	UpdateProfiles();
	
	if (pRegToChange != NULL)
	{
		::SetWindowText(m_hWndWindowItems[EDT_NICK], pRegToChange->sNick);
		::EnableWindow(m_hWndWindowItems[EDT_NICK], FALSE);
		
		if (pRegToChange->bPassHash == false)
		{
			::SetWindowText(m_hWndWindowItems[EDT_PASSWORD], pRegToChange->sPass);
		}
		else
		{
			HWND hWndTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, "", TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			                                  m_hWndWindowItems[EDT_PASSWORD], NULL, clsServerManager::hInstance, NULL);
			                                  
			TOOLINFO ti = { 0 };
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
			ti.hwnd = m_hWndWindowItems[WINDOW_HANDLE];
			ti.uId = (UINT_PTR)m_hWndWindowItems[EDT_PASSWORD];
			ti.hinst = clsServerManager::hInstance;
			ti.lpszText = clsLanguageManager::mPtr->sTexts[LAN_PASSWORD_IS_HASHED];
			
			::SendMessage(hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
			::SendMessage(hWndTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM(30000, 0));
		}
		
		::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, pRegToChange->ui16Profile, 0);
	}
	else if (sNick != NULL)
	{
		::SetWindowText(m_hWndWindowItems[EDT_NICK], sNick);
	}
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void clsRegisteredUserDialog::UpdateProfiles()
{
	int iSel = (int)::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_GETCURSEL, 0, 0);
	
	for (uint16_t ui16i = 0; ui16i < clsProfileManager::mPtr->ui16ProfileCount; ui16i++)
	{
		::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_ADDSTRING, 0, (LPARAM)clsProfileManager::mPtr->ppProfilesTable[ui16i]->sName);
	}
	
	if (pRegToChange != NULL)
	{
		::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, pRegToChange->ui16Profile, 0);
	}
	else
	{
		iSel = (int)::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, iSel, 0);
		
		if (iSel == CB_ERR)
		{
			::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, 0, 0);
		}
	}
}
//------------------------------------------------------------------------------

bool clsRegisteredUserDialog::OnAccept()
{
	if (::GetWindowTextLength(m_hWndWindowItems[EDT_NICK]) == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_NICK_MUST_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	if (pRegToChange == NULL && ::GetWindowTextLength(m_hWndWindowItems[EDT_PASSWORD]) == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_PASS_MUST_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	char sNick[65], sPassword[65];
	
	sPassword[0] = '\0';
	
	::GetWindowText(m_hWndWindowItems[EDT_NICK], sNick, 65);
	::GetWindowText(m_hWndWindowItems[EDT_PASSWORD], sPassword, 65);
	
	uint16_t ui16Profile = (uint16_t)::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_GETCURSEL, 0, 0);
	
	if (pRegToChange == NULL)
	{
		if (clsRegManager::mPtr->AddNew(sNick, sPassword, ui16Profile) == false)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_USER_IS_ALREDY_REG], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		
		return true;
	}
	else
	{
		RegUser * pReg = pRegToChange;
		pRegToChange = nullptr;
		
		clsRegManager::mPtr->ChangeReg(pReg, sPassword[0] == '\0' ? NULL : sPassword, ui16Profile);
		return true;
	}
}
//------------------------------------------------------------------------------

void clsRegisteredUserDialog::RegChanged(RegUser * pReg)
{
	if (pRegToChange == NULL || pReg != pRegToChange)
	{
		return;
	}
	
	::SetWindowText(m_hWndWindowItems[EDT_PASSWORD], pRegToChange->sPass);
	
	::SendMessage(m_hWndWindowItems[CB_PROFILE], CB_SETCURSEL, pRegToChange->ui16Profile, 0);
	
	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_USER_CHANGED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------

void clsRegisteredUserDialog::RegDeleted(RegUser * pReg)
{
	if (pRegToChange == NULL || pReg != pRegToChange)
	{
		return;
	}
	
	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_USER_DELETED_ACCEPT_TO_NEW], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
