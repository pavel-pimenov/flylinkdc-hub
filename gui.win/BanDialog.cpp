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
#include "BanDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashBanManager.h"
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "BansDialog.h"
//---------------------------------------------------------------------------
static ATOM atomBanDialog = 0;
//---------------------------------------------------------------------------

clsBanDialog::clsBanDialog() : pBanToChange(NULL)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

clsBanDialog::~clsBanDialog()
{
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsBanDialog::StaticBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	clsBanDialog * pBanDialog = (clsBanDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pBanDialog == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pBanDialog->BanDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsBanDialog::BanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		case BTN_IP_BAN:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				bool bChecked = ::SendMessage(m_hWndWindowItems[BTN_IP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				::EnableWindow(m_hWndWindowItems[BTN_FULL_BAN], bChecked == true ? TRUE : FALSE);
				
				return 0;
			}
			
			break;
		case RB_PERM_BAN:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], FALSE);
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], FALSE);
			}
			
			break;
		case RB_TEMP_BAN:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);
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
		::SetFocus(m_hWndWindowItems[EDT_NICK]);
		return 0;
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void clsBanDialog::DoModal(HWND hWndParent, BanItem * pBan/* = NULL*/)
{
	pBanToChange = pBan;
	
	if (atomBanDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_BanDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomBanDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (ScaleGui(300) / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (ScaleGui(394) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomBanDialog), clsLanguageManager::mPtr->sTexts[LAN_BAN],
	                                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(300), ScaleGui(394),
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticBanDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	{
		int iHeight = clsGuiSettingManager::iOneLineOneChecksGB + clsGuiSettingManager::iOneLineTwoChecksGB + (2 * clsGuiSettingManager::iOneLineGB) + (clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iOneLineGB + 5) + clsGuiSettingManager::iEditHeight + 6;
		
		int iDiff = rcParent.bottom - iHeight;
		
		if (iDiff != 0)
		{
			::GetWindowRect(hWndParent, &rcParent);
			
			iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - ((ScaleGui(307) - iDiff) / 2);
			
			::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
			
			::SetWindowPos(m_hWndWindowItems[WINDOW_HANDLE], NULL, iX, iY, (rcParent.right - rcParent.left), (rcParent.bottom - rcParent.top) - iDiff, SWP_NOZORDER);
		}
	}
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[GB_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                              3, 0, rcParent.right - 6, clsGuiSettingManager::iOneLineOneChecksGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                              
	m_hWndWindowItems[EDT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                               11, clsGuiSettingManager::iGroupBoxMargin, rcParent.right - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(EDT_NICK + 100), clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_NICK], EM_SETLIMITTEXT, 64, 0);
	
	m_hWndWindowItems[BTN_NICK_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NICK_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                   11, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 4, rcParent.right - 22, clsGuiSettingManager::iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                   
	int iPosY = clsGuiSettingManager::iOneLineOneChecksGB;
	
	m_hWndWindowItems[GB_IP] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_IP], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                            3, iPosY, rcParent.right - 6, clsGuiSettingManager::iOneLineTwoChecksGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                            
	m_hWndWindowItems[EDT_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                             11, iPosY + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_IP], EM_SETLIMITTEXT, 39, 0);
	
	m_hWndWindowItems[BTN_IP_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_IP_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                 11, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 4, rcParent.right - 22, clsGuiSettingManager::iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_IP_BAN, clsServerManager::hInstance, NULL);
	                                                 
	m_hWndWindowItems[BTN_FULL_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_FULL_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_AUTOCHECKBOX,
	                                                   11, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iCheckHeight + 7, rcParent.right - 22, clsGuiSettingManager::iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                   
	iPosY += clsGuiSettingManager::iOneLineTwoChecksGB;
	
	m_hWndWindowItems[GB_REASON] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REASON], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, iPosY, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                
	m_hWndWindowItems[EDT_REASON] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, iPosY + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_REASON, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_REASON], EM_SETLIMITTEXT, 511, 0);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	m_hWndWindowItems[GB_BY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CREATED_BY], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                            3, iPosY, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                            
	m_hWndWindowItems[EDT_BY] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                             11, iPosY + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_BY, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_BY], EM_SETLIMITTEXT, 64, 0);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	m_hWndWindowItems[GB_BAN_TYPE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  3, iPosY, rcParent.right - 6, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iOneLineGB + 5, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                  
	m_hWndWindowItems[GB_TEMP_BAN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight, rcParent.right - 16, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                  
	m_hWndWindowItems[RB_PERM_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_PERMANENT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
	                                                  16, iPosY + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - 32, clsGuiSettingManager::iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)RB_PERM_BAN, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_CHECKED, 0);
	
	int iThird = (rcParent.right - 32) / 3;
	
	m_hWndWindowItems[RB_TEMP_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_TEMPORARY], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
	                                                  16, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + ((clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight) / 2), iThird - 2, clsGuiSettingManager::iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)RB_TEMP_BAN, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
	
	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | DTS_SHORTDATECENTURYFORMAT,
	                                                              iThird + 16, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight, iThird - 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                              
	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | DTS_TIMEFORMAT | DTS_UPDOWN,
	                                                              (iThird * 2) + 19, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight, iThird - 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                              
	iPosY += clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iOneLineGB + 9;
	
	m_hWndWindowItems[BTN_ACCEPT] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                 2, iPosY, (rcParent.right / 2) - 3, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, clsServerManager::hInstance, NULL);
	                                                 
	m_hWndWindowItems[BTN_DISCARD] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                  (rcParent.right / 2) + 2, iPosY, (rcParent.right / 2) - 4, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, clsServerManager::hInstance, NULL);
	                                                  
	for (uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if (m_hWndWindowItems[ui8i] == NULL)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	if (pBanToChange != NULL)
	{
		if (pBanToChange->sNick != NULL)
		{
			::SetWindowText(m_hWndWindowItems[EDT_NICK], pBanToChange->sNick);
			
			if (((pBanToChange->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true)
			{
				::SendMessage(m_hWndWindowItems[BTN_NICK_BAN], BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		
		::SetWindowText(m_hWndWindowItems[EDT_IP], pBanToChange->sIp);
		
		if (((pBanToChange->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true)
		{
			::SendMessage(m_hWndWindowItems[BTN_IP_BAN], BM_SETCHECK, BST_CHECKED, 0);
			
			::EnableWindow(m_hWndWindowItems[BTN_FULL_BAN], TRUE);
			
			if (((pBanToChange->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true)
			{
				::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		
		if (pBanToChange->sReason != NULL)
		{
			::SetWindowText(m_hWndWindowItems[EDT_REASON], pBanToChange->sReason);
		}
		
		if (pBanToChange->sBy != NULL)
		{
			::SetWindowText(m_hWndWindowItems[EDT_BY], pBanToChange->sBy);
		}
		
		if (((pBanToChange->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true)
		{
			::SendMessage(m_hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
			::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_CHECKED, 0);
			
			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);
			
			SYSTEMTIME stDateTime = { 0 };
			
			struct tm *tm = localtime(&pBanToChange->tTempBanExpire);
			
			stDateTime.wDay = (WORD)tm->tm_mday;
			stDateTime.wMonth = (WORD)(tm->tm_mon + 1);
			stDateTime.wYear = (WORD)(tm->tm_year + 1900);
			
			stDateTime.wHour = (WORD)tm->tm_hour;
			stDateTime.wMinute = (WORD)tm->tm_min;
			stDateTime.wSecond = (WORD)tm->tm_sec;
			
			::SendMessage(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stDateTime);
			::SendMessage(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stDateTime);
		}
	}
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

bool clsBanDialog::OnAccept()
{
	bool bNickBan = ::SendMessage(m_hWndWindowItems[BTN_NICK_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	bool bIpBan = ::SendMessage(m_hWndWindowItems[BTN_IP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bNickBan == false && bIpBan == false)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_YOU_CANT_CREATE_BAN_WITHOUT_NICK_OR_IP], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	int iNickLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_NICK]);
	
	if (bNickBan == true && iNickLen == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_FOR_NICK_BAN_SPECIFY_NICK], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	int iIpLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_IP]);
	
	char sIP[40];
	::GetWindowText(m_hWndWindowItems[EDT_IP], sIP, 40);
	
	uint8_t ui128IpHash[16];
	memset(ui128IpHash, 0, 16);
	
	if (bIpBan == true)
	{
		if (iIpLen == 0)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_FOR_IP_BAN_SPECIFY_IP], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		else if (HashIP(sIP, ui128IpHash) == false)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(sIP) + " " + clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}
	
	time_t acc_time;
	time(&acc_time);
	
	time_t ban_time = 0;
	
	bool bTempBan = ::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bTempBan == true)
	{
		SYSTEMTIME stDate = { 0 };
		SYSTEMTIME stTime = { 0 };
		
		if (::SendMessage(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], DTM_GETSYSTEMTIME, 0, (LPARAM)&stDate) != GDT_VALID || ::SendMessage(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], DTM_GETSYSTEMTIME, 0, (LPARAM)&stTime) != GDT_VALID)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			
			return false;
		}
		
		struct tm *tm = localtime(&acc_time);
		
		tm->tm_mday = stDate.wDay;
		tm->tm_mon = (stDate.wMonth - 1);
		tm->tm_year = (stDate.wYear - 1900);
		
		tm->tm_hour = stTime.wHour;
		tm->tm_min = stTime.wMinute;
		tm->tm_sec = stTime.wSecond;
		
		tm->tm_isdst = -1;
		
		ban_time = mktime(tm);
		
		if (ban_time == (time_t) - 1 || ban_time <= acc_time)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED_BAN_EXPIRED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			
			return false;
		}
	}
	
	if (pBanToChange == NULL)
	{
		BanItem * pBan = new (std::nothrow) BanItem();
		if (pBan == NULL)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate Ban in clsBanDialog::OnAccept\n");
			return false;
		}
		
		if (iIpLen != 0)
		{
			strcpy(pBan->sIp, sIP);
			memcpy(pBan->ui128IpHash, ui128IpHash, 16);
			
			if (bIpBan == true)
			{
				pBan->ui8Bits |= clsBanManager::IP;
				
				if (::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					pBan->ui8Bits |= clsBanManager::FULL;
				}
			}
		}
		
		if (bTempBan == true)
		{
			pBan->ui8Bits |= clsBanManager::TEMP;
			pBan->tTempBanExpire = ban_time;
		}
		else
		{
			pBan->ui8Bits |= clsBanManager::PERM;
		}
		
		if (iNickLen != 0)
		{
			pBan->sNick = (char *)malloc(iNickLen + 1);
			if (pBan->sNick == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sNick in clsBanDialog::OnAccept\n", iNickLen + 1);
				delete pBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_NICK], pBan->sNick, iNickLen + 1);
			pBan->ui32NickHash = HashNick(pBan->sNick, iNickLen);
			
			if (bNickBan == true)
			{
				pBan->ui8Bits |= clsBanManager::NICK;
				
				// PPK ... not allow same nickbans !
				BanItem *nxtBan = clsBanManager::mPtr->FindNick(pBan->ui32NickHash, acc_time, pBan->sNick);
				
				if (nxtBan != NULL)
				{
					// PPK ... same ban exist, delete new
					delete pBan;
					
					::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_SIMILAR_BAN_EXIST], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
					return false;
				}
			}
		}
		
		if (bIpBan == true)
		{
			BanItem *nxtBan = clsBanManager::mPtr->FindIP(pBan->ui128IpHash, acc_time);
			
			if (nxtBan != NULL)
			{
				// PPK ... same ban exist, delete new
				delete pBan;
				
				::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_SIMILAR_BAN_EXIST], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		}
		
		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);
		
		if (iReasonLen != 0)
		{
			pBan->sReason = (char *)malloc(iReasonLen + 1);
			if (pBan->sReason == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sReason in clsBanDialog::OnAccept\n", iReasonLen + 1);
				
				delete pBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_REASON], pBan->sReason, iReasonLen + 1);
		}
		
		int iByLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_BY]);
		
		if (iByLen != 0)
		{
			pBan->sBy = (char *)malloc(iByLen + 1);
			if (pBan->sBy == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sBy in clsBanDialog::OnAccept\n", iByLen + 1);
				
				delete pBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_BY], pBan->sBy, iByLen + 1);
		}
		
		if (clsBanManager::mPtr->Add(pBan) == false)
		{
			delete pBan;
			return false;
		}
		
		return true;
	}
	else
	{
		if (pBanToChange->sIp[0] != '\0' && iIpLen == 0)
		{
			if (((pBanToChange->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true)
			{
				clsBanManager::mPtr->RemFromIpTable(pBanToChange);
			}
			
			pBanToChange->sIp[0] = '\0';
			memset(pBanToChange->ui128IpHash, 0, 16);
			
			pBanToChange->ui8Bits &= ~clsBanManager::IP;
			pBanToChange->ui8Bits &= ~clsBanManager::FULL;
		}
		else if ((pBanToChange->sIp[0] == '\0' && iIpLen != 0) ||
		         (pBanToChange->sIp[0] != '\0' && iIpLen != 0 && strcmp(pBanToChange->sIp, sIP) != NULL))
		{
			if (((pBanToChange->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true)
			{
				clsBanManager::mPtr->RemFromIpTable(pBanToChange);
			}
			
			strcpy(pBanToChange->sIp, sIP);
			memcpy(pBanToChange->ui128IpHash, ui128IpHash, 16);
			
			if (bIpBan == true)
			{
				pBanToChange->ui8Bits |= clsBanManager::IP;
				
				if (::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					pBanToChange->ui8Bits |= clsBanManager::FULL;
				}
				else
				{
					pBanToChange->ui8Bits &= ~clsBanManager::FULL;
				}
				
				if (clsBanManager::mPtr->Add2IpTable(pBanToChange) == false)
				{
					clsBanManager::mPtr->Rem(pBanToChange);
					
					delete pBanToChange;
					pBanToChange = nullptr;
					
					return false;
				}
			}
			else
			{
				pBanToChange->ui8Bits &= ~clsBanManager::IP;
				pBanToChange->ui8Bits &= ~clsBanManager::FULL;
			}
		}
		
		if (iIpLen != 0)
		{
			if (bIpBan == true)
			{
				if (((pBanToChange->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == false)
				{
					if (clsBanManager::mPtr->Add2IpTable(pBanToChange) == false)
					{
						clsBanManager::mPtr->Rem(pBanToChange);
						
						delete pBanToChange;
						pBanToChange = nullptr;
						
						return false;
					}
				}
				
				pBanToChange->ui8Bits |= clsBanManager::IP;
				
				if (::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					pBanToChange->ui8Bits |= clsBanManager::FULL;
				}
				else
				{
					pBanToChange->ui8Bits &= ~clsBanManager::FULL;
				}
			}
			else
			{
				if (((pBanToChange->ui8Bits & clsBanManager::IP) == clsBanManager::IP) == true)
				{
					clsBanManager::mPtr->RemFromIpTable(pBanToChange);
				}
				
				pBanToChange->ui8Bits &= ~clsBanManager::IP;
				pBanToChange->ui8Bits &= ~clsBanManager::FULL;
			}
		}
		
		if (bTempBan == true)
		{
			if (((pBanToChange->ui8Bits & clsBanManager::PERM) == clsBanManager::PERM) == true)
			{
				pBanToChange->ui8Bits &= ~clsBanManager::PERM;
				
				// remove from perm
				if (pBanToChange->pPrev == NULL)
				{
					if (pBanToChange->pNext == NULL)
					{
						clsBanManager::mPtr->pPermBanListS = nullptr;
						clsBanManager::mPtr->pPermBanListE = nullptr;
					}
					else
					{
						pBanToChange->pNext->pPrev = nullptr;
						clsBanManager::mPtr->pPermBanListS = pBanToChange->pNext;
					}
				}
				else if (pBanToChange->pNext == NULL)
				{
					pBanToChange->pPrev->pNext = nullptr;
					clsBanManager::mPtr->pPermBanListE = pBanToChange->pPrev;
				}
				else
				{
					pBanToChange->pPrev->pNext = pBanToChange->pNext;
					pBanToChange->pNext->pPrev = pBanToChange->pPrev;
				}
				
				pBanToChange->pPrev = nullptr;
				pBanToChange->pNext = nullptr;
				
				// add to temp
				if (clsBanManager::mPtr->pTempBanListE == NULL)
				{
					clsBanManager::mPtr->pTempBanListS = pBanToChange;
					clsBanManager::mPtr->pTempBanListE = pBanToChange;
				}
				else
				{
					clsBanManager::mPtr->pTempBanListE->pNext = pBanToChange;
					pBanToChange->pPrev = clsBanManager::mPtr->pTempBanListE;
					clsBanManager::mPtr->pTempBanListE = pBanToChange;
				}
			}
			
			pBanToChange->ui8Bits |= clsBanManager::TEMP;
			pBanToChange->tTempBanExpire = ban_time;
		}
		else
		{
			if (((pBanToChange->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true)
			{
				pBanToChange->ui8Bits &= ~clsBanManager::TEMP;
				
				// remove from temp
				if (pBanToChange->pPrev == NULL)
				{
					if (pBanToChange->pNext == NULL)
					{
						clsBanManager::mPtr->pTempBanListS = nullptr;
						clsBanManager::mPtr->pTempBanListE = nullptr;
					}
					else
					{
						pBanToChange->pNext->pPrev = nullptr;
						clsBanManager::mPtr->pTempBanListS = pBanToChange->pNext;
					}
				}
				else if (pBanToChange->pNext == NULL)
				{
					pBanToChange->pPrev->pNext = nullptr;
					clsBanManager::mPtr->pTempBanListE = pBanToChange->pPrev;
				}
				else
				{
					pBanToChange->pPrev->pNext = pBanToChange->pNext;
					pBanToChange->pNext->pPrev = pBanToChange->pPrev;
				}
				
				pBanToChange->pPrev = nullptr;
				pBanToChange->pNext = nullptr;
				
				// add to perm
				if (clsBanManager::mPtr->pPermBanListE == NULL)
				{
					clsBanManager::mPtr->pPermBanListS = pBanToChange;
					clsBanManager::mPtr->pPermBanListE = pBanToChange;
				}
				else
				{
					clsBanManager::mPtr->pPermBanListE->pNext = pBanToChange;
					pBanToChange->pPrev = clsBanManager::mPtr->pPermBanListE;
					clsBanManager::mPtr->pPermBanListE = pBanToChange;
				}
			}
			
			pBanToChange->ui8Bits |= clsBanManager::PERM;
			pBanToChange->tTempBanExpire = 0;
		}
		
		char * sNick = nullptr;
		if (iNickLen != 0)
		{
			sNick = (char *)malloc(iNickLen + 1);
			if (sNick == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sNick in clsBanDialog::OnAccept\n", iNickLen + 1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_NICK], sNick, iNickLen + 1);
		}
		
		if (pBanToChange->sNick != NULL && iNickLen == 0)
		{
			if (((pBanToChange->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true)
			{
				clsBanManager::mPtr->RemFromNickTable(pBanToChange);
			}
			
			safe_free(pBanToChange->sNick);
			
			pBanToChange->ui32NickHash = 0;
			
			pBanToChange->ui8Bits &= ~clsBanManager::NICK;
		}
		else if ((pBanToChange->sNick == NULL && iNickLen != 0) ||
		         (pBanToChange->sNick != NULL && iNickLen != 0 && strcmp(pBanToChange->sNick, sNick) != NULL))
		{
			if (((pBanToChange->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true)
			{
				clsBanManager::mPtr->RemFromNickTable(pBanToChange);
			}
			
			safe_free_and_init(pBanToChange->sNick, sNick);
			pBanToChange->ui32NickHash = HashNick(pBanToChange->sNick, iNickLen);
			
			if (bNickBan == true)
			{
				pBanToChange->ui8Bits |= clsBanManager::NICK;
				
				clsBanManager::mPtr->Add2NickTable(pBanToChange);
			}
			else
			{
				pBanToChange->ui8Bits &= ~clsBanManager::NICK;
			}
		}
		
		if (sNick != NULL && (pBanToChange->sNick != sNick))
		{
			free(sNick);
		}
		
		if (iNickLen != 0)
		{
			if (bNickBan == true)
			{
				if (((pBanToChange->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == false)
				{
					clsBanManager::mPtr->Add2NickTable(pBanToChange);
				}
				
				pBanToChange->ui8Bits |= clsBanManager::NICK;
			}
			else
			{
				if (((pBanToChange->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK) == true)
				{
					clsBanManager::mPtr->RemFromNickTable(pBanToChange);
				}
				
				pBanToChange->ui8Bits &= ~clsBanManager::NICK;
			}
		}
		
		
		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);
		
		char * sReason = nullptr;
		if (iReasonLen != 0)
		{
			sReason = (char *)malloc(iReasonLen + 1);
			if (sReason == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sReason in clsBanDialog::OnAccept\n", iReasonLen + 1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_REASON], sReason, iReasonLen + 1);
		}
		
		if (iReasonLen != 0)
		{
			if (pBanToChange->sReason == NULL || strcmp(pBanToChange->sReason, sReason) != NULL)
			{
				safe_free_and_init(pBanToChange->sReason, sReason);
			}
		}
		else
		{
			safe_free(pBanToChange->sReason);
		}
		
		if (sReason != NULL && pBanToChange->sReason != sReason)
		{
			free(sReason);
		}
		
		int iByLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_BY]);
		
		char * sBy = nullptr;
		if (iByLen != 0)
		{
			sBy = (char *)malloc(iByLen + 1);
			if (sBy == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sBy in clsBanDialog::OnAccept\n", iByLen + 1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_BY], sBy, iByLen + 1);
		}
		
		if (iByLen != 0)
		{
			if (pBanToChange->sBy == NULL || strcmp(pBanToChange->sBy, sBy) != NULL)
			{
				safe_free_and_init(pBanToChange->sBy, sBy);
			}
		}
		else
		{
			safe_free(pBanToChange->sBy);
		}
		
		if (sBy != NULL && pBanToChange->sBy != sBy)
		{
			free(sBy);
		}
		
		if (clsBansDialog::mPtr != NULL)
		{
			clsBansDialog::mPtr->RemoveBan(pBanToChange);
			clsBansDialog::mPtr->AddBan(pBanToChange);
		}
		
		return true;
	}
}
//------------------------------------------------------------------------------

void clsBanDialog::BanDeleted(BanItem * pBan)
{
	if (pBanToChange == NULL || pBan != pBanToChange)
	{
		return;
	}
	
	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_BAN_DELETED_ACCEPT_TO_NEW], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
