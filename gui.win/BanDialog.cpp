/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2022  Petr Kozelka, PPK at PtokaX dot org

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

BanDialog::BanDialog() : m_pBanToChange(nullptr)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

BanDialog::~BanDialog()
{
}
//---------------------------------------------------------------------------

LRESULT CALLBACK BanDialog::StaticBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BanDialog * pBanDialog = (BanDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if(pBanDialog == nullptr)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pBanDialog->BanDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT BanDialog::BanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			if(OnAccept() == false)
			{
				return 0;
			}
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		case (EDT_NICK+100):
			if(HIWORD(wParam) == EN_CHANGE)
			{
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);
				
				bool bChanged = false;
				
				for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++)
				{
					if(buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ')
					{
						strcpy(buf+ui16i, buf+ui16i+1);
						bChanged = true;
						ui16i--;
					}
				}
				
				if(bChanged == true)
				{
					int iStart = 0, iEnd = 0;
					
					::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
					
					::SetWindowText((HWND)lParam, buf);
					
					::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
				}
				
				return 0;
			}
			
			break;
		case BTN_IP_BAN:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				bool bChecked = ::SendMessage(m_hWndWindowItems[BTN_IP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				::EnableWindow(m_hWndWindowItems[BTN_FULL_BAN], bChecked == true ? TRUE : FALSE);
				
				return 0;
			}
			
			break;
		case RB_PERM_BAN:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], FALSE);
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], FALSE);
			}
			
			break;
		case RB_TEMP_BAN:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
				::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);
			}
			
			break;
		}
		
		break;
	case WM_CLOSE:
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		ServerManager::m_hWndActiveDialog = nullptr;
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

void BanDialog::DoModal(HWND hWndParent, BanItem * pBan/* = nullptr*/)
{
	m_pBanToChange = pBan;
	
	if(atomBanDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_BanDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomBanDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(300) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(394) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomBanDialog), LanguageManager::m_Ptr->m_sTexts[LAN_BAN],
	                                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(300), ScaleGui(394),
	                                                    hWndParent, nullptr, ServerManager::m_hInstance, nullptr);
	                                                    
	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr)
	{
		return;
	}
	
	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticBanDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	{
		int iHeight = GuiSettingManager::m_iOneLineOneChecksGB + GuiSettingManager::m_iOneLineTwoChecksGB + (2 * GuiSettingManager::m_iOneLineGB) + (GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iOneLineGB + 5) + GuiSettingManager::m_iEditHeight + 6;
		
		int iDiff = rcParent.bottom - iHeight;
		
		if(iDiff != 0)
		{
			::GetWindowRect(hWndParent, &rcParent);
			
			iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - ((ScaleGui(307) - iDiff) / 2);
			
			::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
			
			::SetWindowPos(m_hWndWindowItems[WINDOW_HANDLE], nullptr, iX, iY, (rcParent.right-rcParent.left), (rcParent.bottom-rcParent.top) - iDiff, SWP_NOZORDER);
		}
	}
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[GB_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                              3, 0, rcParent.right - 6, GuiSettingManager::m_iOneLineOneChecksGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                              
	m_hWndWindowItems[EDT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                               11, GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(EDT_NICK+100), ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_NICK], EM_SETLIMITTEXT, 64, 0);
	
	m_hWndWindowItems[BTN_NICK_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_NICK_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                   11, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, rcParent.right - 22, GuiSettingManager::m_iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                   
	int iPosY = GuiSettingManager::m_iOneLineOneChecksGB;
	
	m_hWndWindowItems[GB_IP] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_IP], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                            3, iPosY, rcParent.right - 6, GuiSettingManager::m_iOneLineTwoChecksGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                            
	m_hWndWindowItems[EDT_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                             11, iPosY + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_IP], EM_SETLIMITTEXT, 39, 0);
	
	m_hWndWindowItems[BTN_IP_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_IP_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                 11, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, rcParent.right - 22, GuiSettingManager::m_iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_IP_BAN, ServerManager::m_hInstance, nullptr);
	                                                 
	m_hWndWindowItems[BTN_FULL_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_FULL_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_AUTOCHECKBOX,
	                                                   11, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iCheckHeight + 7, rcParent.right - 22, GuiSettingManager::m_iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                   
	iPosY += GuiSettingManager::m_iOneLineTwoChecksGB;
	
	m_hWndWindowItems[GB_REASON] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REASON], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, iPosY, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                
	m_hWndWindowItems[EDT_REASON] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, iPosY + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_REASON, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_REASON], EM_SETLIMITTEXT, 511, 0);
	
	iPosY += GuiSettingManager::m_iOneLineGB;
	
	m_hWndWindowItems[GB_BY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CREATED_BY], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                            3, iPosY, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                            
	m_hWndWindowItems[EDT_BY] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                             11, iPosY + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_BY, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_BY], EM_SETLIMITTEXT, 64, 0);
	
	iPosY += GuiSettingManager::m_iOneLineGB;
	
	m_hWndWindowItems[GB_BAN_TYPE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  3, iPosY, rcParent.right - 6, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iOneLineGB + 5, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                  
	m_hWndWindowItems[GB_TEMP_BAN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight, rcParent.right - 16, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                  
	m_hWndWindowItems[RB_PERM_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PERMANENT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
	                                                  16, iPosY + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 32, GuiSettingManager::m_iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)RB_PERM_BAN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_CHECKED, 0);
	
	int iThird = (rcParent.right - 32) / 3;
	
	m_hWndWindowItems[RB_TEMP_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_TEMPORARY], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
	                                                  16, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), iThird - 2, GuiSettingManager::m_iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)RB_TEMP_BAN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
	
	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | DTS_SHORTDATECENTURYFORMAT,
	                                                              iThird + 16, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight, iThird - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                              
	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED | DTS_TIMEFORMAT | DTS_UPDOWN,
	                                                              (iThird * 2) + 19, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight, iThird - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                              
	iPosY += GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iOneLineGB + 9;
	
	m_hWndWindowItems[BTN_ACCEPT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                 2, iPosY, (rcParent.right / 2) - 3, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, ServerManager::m_hInstance, nullptr);
	                                                 
	m_hWndWindowItems[BTN_DISCARD] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                  (rcParent.right / 2) + 2, iPosY, (rcParent.right / 2) - 4, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, ServerManager::m_hInstance, nullptr);
	                                                  
	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if(m_hWndWindowItems[ui8i] == nullptr)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}
	
	if(m_pBanToChange != nullptr)
	{
		if(m_pBanToChange->m_sNick != nullptr)
		{
			::SetWindowText(m_hWndWindowItems[EDT_NICK], m_pBanToChange->m_sNick);
			
			if(((m_pBanToChange->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true)
			{
				::SendMessage(m_hWndWindowItems[BTN_NICK_BAN], BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		
		::SetWindowText(m_hWndWindowItems[EDT_IP], m_pBanToChange->m_sIp);
		
		if(((m_pBanToChange->m_ui8Bits & BanManager::IP) == BanManager::IP) == true)
		{
			::SendMessage(m_hWndWindowItems[BTN_IP_BAN], BM_SETCHECK, BST_CHECKED, 0);
			
			::EnableWindow(m_hWndWindowItems[BTN_FULL_BAN], TRUE);
			
			if(((m_pBanToChange->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true)
			{
				::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		
		if(m_pBanToChange->m_sReason != nullptr)
		{
			::SetWindowText(m_hWndWindowItems[EDT_REASON], m_pBanToChange->m_sReason);
		}
		
		if(m_pBanToChange->m_sBy != nullptr)
		{
			::SetWindowText(m_hWndWindowItems[EDT_BY], m_pBanToChange->m_sBy);
		}
		
		if(((m_pBanToChange->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
		{
			::SendMessage(m_hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
			::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_CHECKED, 0);
			
			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);
			
			SYSTEMTIME stDateTime = { 0 };
			
			struct tm *tm = localtime(&m_pBanToChange->m_tTempBanExpire);
			
			stDateTime.wDay = (WORD)tm->tm_mday;
			stDateTime.wMonth = (WORD)(tm->tm_mon+1);
			stDateTime.wYear = (WORD)(tm->tm_year+1900);
			
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

bool BanDialog::OnAccept()
{
	bool bNickBan = ::SendMessage(m_hWndWindowItems[BTN_NICK_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	bool bIpBan = ::SendMessage(m_hWndWindowItems[BTN_IP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if(bNickBan == false && bIpBan == false)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_CREATE_BAN_WITHOUT_NICK_OR_IP], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	int iNickLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_NICK]);
	
	if(bNickBan == true && iNickLen == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_FOR_NICK_BAN_SPECIFY_NICK], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	int iIpLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_IP]);
	
	char sIP[40];
	::GetWindowText(m_hWndWindowItems[EDT_IP], sIP, 40);
	
	uint8_t ui128IpHash[16];
	memset(ui128IpHash, 0, 16);
	
	if(bIpBan == true)
	{
		if(iIpLen == 0)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_FOR_IP_BAN_SPECIFY_IP], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		else if(HashIP(sIP, ui128IpHash) == false)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(sIP) + " " + LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}
	
	time_t acc_time;
	time(&acc_time);
	
	time_t ban_time = 0;
	
	bool bTempBan = ::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if(bTempBan == true)
	{
		SYSTEMTIME stDate = { 0 };
		SYSTEMTIME stTime = { 0 };
		
		if(::SendMessage(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], DTM_GETSYSTEMTIME, 0, (LPARAM)&stDate) != GDT_VALID || ::SendMessage(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], DTM_GETSYSTEMTIME, 0, (LPARAM)&stTime) != GDT_VALID)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			
			return false;
		}
		
		struct tm *tm = localtime(&acc_time);
		
		tm->tm_mday = stDate.wDay;
		tm->tm_mon = (stDate.wMonth-1);
		tm->tm_year = (stDate.wYear-1900);
		
		tm->tm_hour = stTime.wHour;
		tm->tm_min = stTime.wMinute;
		tm->tm_sec = stTime.wSecond;
		
		tm->tm_isdst = -1;
		
		ban_time = mktime(tm);
		
		if(ban_time == (time_t)-1 || ban_time <= acc_time)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED_BAN_EXPIRED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			
			return false;
		}
	}
	
	if(m_pBanToChange == nullptr)
	{
		BanItem * pBan = new (std::nothrow) BanItem();
		if(pBan == nullptr)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate Ban in BanDialog::OnAccept\n");
			return false;
		}
		
		if(iIpLen != 0)
		{
			strcpy(pBan->m_sIp, sIP);
			memcpy(pBan->m_ui128IpHash, ui128IpHash, 16);
			
			if(bIpBan == true)
			{
				pBan->m_ui8Bits |= BanManager::IP;
				
				if(::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					pBan->m_ui8Bits |= BanManager::FULL;
				}
			}
		}
		
		if(bTempBan == true)
		{
			pBan->m_ui8Bits |= BanManager::TEMP;
			pBan->m_tTempBanExpire = ban_time;
		}
		else
		{
			pBan->m_ui8Bits |= BanManager::PERM;
		}
		
		if(iNickLen != 0)
		{
			pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
			if(pBan->m_sNick == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sNick in BanDialog::OnAccept\n", iNickLen+1);
				delete pBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_NICK], pBan->m_sNick, iNickLen+1);
			pBan->m_ui32NickHash = HashNick(pBan->m_sNick, iNickLen);
			
			if(bNickBan == true)
			{
				pBan->m_ui8Bits |= BanManager::NICK;
				
				// PPK ... not allow same nickbans !
				BanItem *nxtBan = BanManager::m_Ptr->FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);
				
				if(nxtBan != nullptr)
				{
					// PPK ... same ban exist, delete new
					delete pBan;
					
					::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_SIMILAR_BAN_EXIST], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
					return false;
				}
			}
		}
		
		if(bIpBan == true)
		{
			BanItem *nxtBan = BanManager::m_Ptr->FindIP(pBan->m_ui128IpHash, acc_time);
			
			if(nxtBan != nullptr)
			{
				// PPK ... same ban exist, delete new
				delete pBan;
				
				::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_SIMILAR_BAN_EXIST], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		}
		
		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);
		
		if(iReasonLen != 0)
		{
			pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
			if(pBan->m_sReason == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sReason in BanDialog::OnAccept\n", iReasonLen+1);
				
				delete pBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_REASON], pBan->m_sReason, iReasonLen+1);
		}
		
		int iByLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_BY]);
		
		if(iByLen != 0)
		{
			pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
			if(pBan->m_sBy == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sBy in BanDialog::OnAccept\n", iByLen+1);
				
				delete pBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_BY], pBan->m_sBy, iByLen+1);
		}
		
		if(BanManager::m_Ptr->Add(pBan) == false)
		{
			delete pBan;
			return false;
		}
		
		return true;
	}
	else
	{
		if(m_pBanToChange->m_sIp[0] != '\0' && iIpLen == 0)
		{
			if(((m_pBanToChange->m_ui8Bits & BanManager::IP) == BanManager::IP) == true)
			{
				BanManager::m_Ptr->RemFromIpTable(m_pBanToChange);
			}
			
			m_pBanToChange->m_sIp[0] = '\0';
			memset(m_pBanToChange->m_ui128IpHash, 0, 16);
			
			m_pBanToChange->m_ui8Bits &= ~BanManager::IP;
			m_pBanToChange->m_ui8Bits &= ~BanManager::FULL;
		}
		else if((m_pBanToChange->m_sIp[0] == '\0' && iIpLen != 0) ||
		        (m_pBanToChange->m_sIp[0] != '\0' && iIpLen != 0 && strcmp(m_pBanToChange->m_sIp, sIP) != 0))
		{
			if(((m_pBanToChange->m_ui8Bits & BanManager::IP) == BanManager::IP) == true)
			{
				BanManager::m_Ptr->RemFromIpTable(m_pBanToChange);
			}
			
			strcpy(m_pBanToChange->m_sIp, sIP);
			memcpy(m_pBanToChange->m_ui128IpHash, ui128IpHash, 16);
			
			if(bIpBan == true)
			{
				m_pBanToChange->m_ui8Bits |= BanManager::IP;
				
				if(::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					m_pBanToChange->m_ui8Bits |= BanManager::FULL;
				}
				else
				{
					m_pBanToChange->m_ui8Bits &= ~BanManager::FULL;
				}
				
				if(BanManager::m_Ptr->Add2IpTable(m_pBanToChange) == false)
				{
					BanManager::m_Ptr->Rem(m_pBanToChange);
					
					delete m_pBanToChange;
					m_pBanToChange = nullptr;
					
					return false;
				}
			}
			else
			{
				m_pBanToChange->m_ui8Bits &= ~BanManager::IP;
				m_pBanToChange->m_ui8Bits &= ~BanManager::FULL;
			}
		}
		
		if(iIpLen != 0)
		{
			if(bIpBan == true)
			{
				if(((m_pBanToChange->m_ui8Bits & BanManager::IP) == BanManager::IP) == false)
				{
					if(BanManager::m_Ptr->Add2IpTable(m_pBanToChange) == false)
					{
						BanManager::m_Ptr->Rem(m_pBanToChange);
						
						delete m_pBanToChange;
						m_pBanToChange = nullptr;
						
						return false;
					}
				}
				
				m_pBanToChange->m_ui8Bits |= BanManager::IP;
				
				if(::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					m_pBanToChange->m_ui8Bits |= BanManager::FULL;
				}
				else
				{
					m_pBanToChange->m_ui8Bits &= ~BanManager::FULL;
				}
			}
			else
			{
				if(((m_pBanToChange->m_ui8Bits & BanManager::IP) == BanManager::IP) == true)
				{
					BanManager::m_Ptr->RemFromIpTable(m_pBanToChange);
				}
				
				m_pBanToChange->m_ui8Bits &= ~BanManager::IP;
				m_pBanToChange->m_ui8Bits &= ~BanManager::FULL;
			}
		}
		
		if(bTempBan == true)
		{
			if(((m_pBanToChange->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true)
			{
				m_pBanToChange->m_ui8Bits &= ~BanManager::PERM;
				
				// remove from perm
				if(m_pBanToChange->m_pPrev == nullptr)
				{
					if(m_pBanToChange->m_pNext == nullptr)
					{
						BanManager::m_Ptr->m_pPermBanListS = nullptr;
						BanManager::m_Ptr->m_pPermBanListE = nullptr;
					}
					else
					{
						m_pBanToChange->m_pNext->m_pPrev = nullptr;
						BanManager::m_Ptr->m_pPermBanListS = m_pBanToChange->m_pNext;
					}
				}
				else if(m_pBanToChange->m_pNext == nullptr)
				{
					m_pBanToChange->m_pPrev->m_pNext = nullptr;
					BanManager::m_Ptr->m_pPermBanListE = m_pBanToChange->m_pPrev;
				}
				else
				{
					m_pBanToChange->m_pPrev->m_pNext = m_pBanToChange->m_pNext;
					m_pBanToChange->m_pNext->m_pPrev = m_pBanToChange->m_pPrev;
				}
				
				m_pBanToChange->m_pPrev = nullptr;
				m_pBanToChange->m_pNext = nullptr;
				
				// add to temp
				if(BanManager::m_Ptr->m_pTempBanListE == nullptr)
				{
					BanManager::m_Ptr->m_pTempBanListS = m_pBanToChange;
					BanManager::m_Ptr->m_pTempBanListE = m_pBanToChange;
				}
				else
				{
					BanManager::m_Ptr->m_pTempBanListE->m_pNext = m_pBanToChange;
					m_pBanToChange->m_pPrev = BanManager::m_Ptr->m_pTempBanListE;
					BanManager::m_Ptr->m_pTempBanListE = m_pBanToChange;
				}
			}
			
			m_pBanToChange->m_ui8Bits |= BanManager::TEMP;
			m_pBanToChange->m_tTempBanExpire = ban_time;
		}
		else
		{
			if(((m_pBanToChange->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
			{
				m_pBanToChange->m_ui8Bits &= ~BanManager::TEMP;
				
				// remove from temp
				if(m_pBanToChange->m_pPrev == nullptr)
				{
					if(m_pBanToChange->m_pNext == nullptr)
					{
						BanManager::m_Ptr->m_pTempBanListS = nullptr;
						BanManager::m_Ptr->m_pTempBanListE = nullptr;
					}
					else
					{
						m_pBanToChange->m_pNext->m_pPrev = nullptr;
						BanManager::m_Ptr->m_pTempBanListS = m_pBanToChange->m_pNext;
					}
				}
				else if(m_pBanToChange->m_pNext == nullptr)
				{
					m_pBanToChange->m_pPrev->m_pNext = nullptr;
					BanManager::m_Ptr->m_pTempBanListE = m_pBanToChange->m_pPrev;
				}
				else
				{
					m_pBanToChange->m_pPrev->m_pNext = m_pBanToChange->m_pNext;
					m_pBanToChange->m_pNext->m_pPrev = m_pBanToChange->m_pPrev;
				}
				
				m_pBanToChange->m_pPrev = nullptr;
				m_pBanToChange->m_pNext = nullptr;
				
				// add to perm
				if(BanManager::m_Ptr->m_pPermBanListE == nullptr)
				{
					BanManager::m_Ptr->m_pPermBanListS = m_pBanToChange;
					BanManager::m_Ptr->m_pPermBanListE = m_pBanToChange;
				}
				else
				{
					BanManager::m_Ptr->m_pPermBanListE->m_pNext = m_pBanToChange;
					m_pBanToChange->m_pPrev = BanManager::m_Ptr->m_pPermBanListE;
					BanManager::m_Ptr->m_pPermBanListE = m_pBanToChange;
				}
			}
			
			m_pBanToChange->m_ui8Bits |= BanManager::PERM;
			m_pBanToChange->m_tTempBanExpire = 0;
		}
		
		char * sNick = nullptr;
		if(iNickLen != 0)
		{
			sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iNickLen+1);
			if(sNick == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sNick in BanDialog::OnAccept\n", iNickLen+1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_NICK], sNick, iNickLen+1);
		}
		
		if(m_pBanToChange->m_sNick != nullptr && iNickLen == 0)
		{
			if(((m_pBanToChange->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true)
			{
				BanManager::m_Ptr->RemFromNickTable(m_pBanToChange);
			}
			
			if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pBanToChange->m_sNick) == 0)
			{
				AppendDebugLog("%s - [MEM] Cannot deallocate sNick in BanDialog::OnAccept\n");
			}
			m_pBanToChange->m_sNick = nullptr;
			
			m_pBanToChange->m_ui32NickHash = 0;
			
			m_pBanToChange->m_ui8Bits &= ~BanManager::NICK;
		}
		else if((m_pBanToChange->m_sNick == nullptr && iNickLen != 0) ||
		        (m_pBanToChange->m_sNick != nullptr && iNickLen != 0 && strcmp(m_pBanToChange->m_sNick, sNick) != 0))
		{
			if(((m_pBanToChange->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true)
			{
				BanManager::m_Ptr->RemFromNickTable(m_pBanToChange);
			}
			
			if(m_pBanToChange->m_sNick != nullptr)
			{
				if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pBanToChange->m_sNick) == 0)
				{
					AppendDebugLog("%s - [MEM] Cannot deallocate sNick in BanDialog::OnAccept\n");
				}
				m_pBanToChange->m_sNick = nullptr;
			}
			
			m_pBanToChange->m_sNick = sNick;
			
			m_pBanToChange->m_ui32NickHash = HashNick(m_pBanToChange->m_sNick, iNickLen);
			
			if(bNickBan == true)
			{
				m_pBanToChange->m_ui8Bits |= BanManager::NICK;
				
				BanManager::m_Ptr->Add2NickTable(m_pBanToChange);
			}
			else
			{
				m_pBanToChange->m_ui8Bits &= ~BanManager::NICK;
			}
		}
		
		if(sNick != nullptr && (m_pBanToChange->m_sNick != sNick))
		{
			if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sNick) == 0)
			{
				AppendDebugLog("%s - [MEM] Cannot deallocate sNick in BanDialog::OnAccept\n");
			}
		}
		
		if(iNickLen != 0)
		{
			if(bNickBan == true)
			{
				if(((m_pBanToChange->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == false)
				{
					BanManager::m_Ptr->Add2NickTable(m_pBanToChange);
				}
				
				m_pBanToChange->m_ui8Bits |= BanManager::NICK;
			}
			else
			{
				if(((m_pBanToChange->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true)
				{
					BanManager::m_Ptr->RemFromNickTable(m_pBanToChange);
				}
				
				m_pBanToChange->m_ui8Bits &= ~BanManager::NICK;
			}
		}
		
		
		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);
		
		char * sReason = nullptr;
		if(iReasonLen != 0)
		{
			sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iReasonLen+1);
			if(sReason == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sReason in BanDialog::OnAccept\n", iReasonLen+1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_REASON], sReason, iReasonLen+1);
			
			if(m_pBanToChange->m_sReason == nullptr || strcmp(m_pBanToChange->m_sReason, sReason) != 0)
			{
				if(m_pBanToChange->m_sReason != nullptr)
				{
					if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pBanToChange->m_sReason) == 0)
					{
						AppendDebugLog("%s - [MEM] Cannot deallocate sReason in BanDialog::OnAccept\n");
					}
					m_pBanToChange->m_sReason = nullptr;
				}
				
				m_pBanToChange->m_sReason = sReason;
			}
		}
		else if(m_pBanToChange->m_sReason != nullptr)
		{
			if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pBanToChange->m_sReason) == 0)
			{
				AppendDebugLog("%s - [MEM] Cannot deallocate sReason in BanDialog::OnAccept\n");
			}
			
			m_pBanToChange->m_sReason = nullptr;
		}
		
		if(sReason != nullptr && (m_pBanToChange->m_sReason != sReason))
		{
			if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sReason) == 0)
			{
				AppendDebugLog("%s - [MEM] Cannot deallocate sReason in BanDialog::OnAccept\n");
			}
		}
		
		int iByLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_BY]);
		
		char * sBy = nullptr;
		if(iByLen != 0)
		{
			sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iByLen+1);
			if(sBy == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sBy in BanDialog::OnAccept\n", iByLen+1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_BY], sBy, iByLen+1);
			
			if(m_pBanToChange->m_sBy == nullptr || strcmp(m_pBanToChange->m_sBy, sBy) != 0)
			{
				if(m_pBanToChange->m_sBy != nullptr)
				{
					if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pBanToChange->m_sBy) == 0)
					{
						AppendDebugLog("%s - [MEM] Cannot deallocate sBy in BanDialog::OnAccept\n");
					}
					m_pBanToChange->m_sBy = nullptr;
				}
				
				m_pBanToChange->m_sBy = sBy;
			}
		}
		else if(m_pBanToChange->m_sBy != nullptr)
		{
			if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pBanToChange->m_sBy) == 0)
			{
				AppendDebugLog("%s - [MEM] Cannot deallocate sBy in BanDialog::OnAccept\n");
			}
			m_pBanToChange->m_sBy = nullptr;
		}
		
		if(sBy != nullptr && (m_pBanToChange->m_sBy != sBy))
		{
			if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sBy) == 0)
			{
				AppendDebugLog("%s - [MEM] Cannot deallocate sBy in BanDialog::OnAccept\n");
			}
		}
		
		if(BansDialog::m_Ptr != nullptr)
		{
			BansDialog::m_Ptr->RemoveBan(m_pBanToChange);
			BansDialog::m_Ptr->AddBan(m_pBanToChange);
		}
		
		return true;
	}
}
//------------------------------------------------------------------------------

void BanDialog::BanDeleted(BanItem * pBan)
{
	if(m_pBanToChange == nullptr || pBan != m_pBanToChange)
	{
		return;
	}
	
	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_BAN_DELETED_ACCEPT_TO_NEW], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
