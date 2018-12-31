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
#include "RangeBanDialog.h"
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
#include "RangeBansDialog.h"
//---------------------------------------------------------------------------
static ATOM atomRangeBanDialog = 0;
//---------------------------------------------------------------------------

clsRangeBanDialog::clsRangeBanDialog() : pRangeBanToChange(NULL)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

clsRangeBanDialog::~clsRangeBanDialog()
{
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsRangeBanDialog::StaticRangeBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	clsRangeBanDialog * pRangeBanDialog = (clsRangeBanDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pRangeBanDialog == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pRangeBanDialog->RangeBanDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsRangeBanDialog::RangeBanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		::SetFocus(m_hWndWindowItems[EDT_FROM_IP]);
		return 0;
		
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void clsRangeBanDialog::DoModal(HWND hWndParent, RangeBanItem * pRangeBan/* = NULL*/)
{
	pRangeBanToChange = pRangeBan;
	
	if (atomRangeBanDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RangeBanDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomRangeBanDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (ScaleGui(300) / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (ScaleGui(307) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRangeBanDialog), clsLanguageManager::mPtr->sTexts[LAN_RANGE_BAN],
	                                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(300), ScaleGui(307),
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRangeBanDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	{
		int iHeight = clsGuiSettingManager::iOneLineOneChecksGB + (2 * clsGuiSettingManager::iOneLineGB) + (clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iOneLineGB + 5) + clsGuiSettingManager::iEditHeight + 6;
		
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
	
	m_hWndWindowItems[GB_RANGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_RANGE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               3, 0, rcParent.right - 6, clsGuiSettingManager::iOneLineOneChecksGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                               
	m_hWndWindowItems[EDT_FROM_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                  11, clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 13, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_FROM_IP], EM_SETLIMITTEXT, 39, 0);
	
	m_hWndWindowItems[EDT_TO_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                (rcParent.right / 2) + 3, clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 13, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_TO_IP], EM_SETLIMITTEXT, 39, 0);
	
	m_hWndWindowItems[BTN_FULL_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_FULL_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                   11, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 4, rcParent.right - 22, clsGuiSettingManager::iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                   
	int iPosY = clsGuiSettingManager::iOneLineOneChecksGB;
	
	m_hWndWindowItems[GB_REASON] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REASON], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, iPosY, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                
	m_hWndWindowItems[EDT_REASON] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, iPosY + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - 22, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
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
	
	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | DTS_SHORTDATECENTURYFORMAT,
	                                                              iThird + 16, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight, iThird - 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                              
	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | DTS_TIMEFORMAT | DTS_UPDOWN,
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
	
	if (pRangeBanToChange != NULL)
	{
		::SetWindowText(m_hWndWindowItems[EDT_FROM_IP], pRangeBanToChange->sIpFrom);
		::SetWindowText(m_hWndWindowItems[EDT_TO_IP], pRangeBanToChange->sIpTo);
		
		if (((pRangeBanToChange->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL) == true)
		{
			::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_SETCHECK, BST_CHECKED, 0);
		}
		
		if (pRangeBanToChange->sReason != NULL)
		{
			::SetWindowText(m_hWndWindowItems[EDT_REASON], pRangeBanToChange->sReason);
		}
		
		if (pRangeBanToChange->sBy != NULL)
		{
			::SetWindowText(m_hWndWindowItems[EDT_BY], pRangeBanToChange->sBy);
		}
		
		if (((pRangeBanToChange->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true)
		{
			::SendMessage(m_hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
			::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_CHECKED, 0);
			
			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);
			
			SYSTEMTIME stDateTime = { 0 };
			
			struct tm *tm = localtime(&pRangeBanToChange->tTempBanExpire);
			
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

bool clsRangeBanDialog::OnAccept()
{
	int iIpFromLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_FROM_IP]);
	
	char sFromIP[40];
	::GetWindowText(m_hWndWindowItems[EDT_FROM_IP], sFromIP, 40);
	
	uint8_t ui128FromIpHash[16];
	memset(ui128FromIpHash, 0, 16);
	
	if (iIpFromLen == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_IP_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	else if (HashIP(sFromIP, ui128FromIpHash) == false)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(sFromIP) + " " + clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	int iIpToLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_TO_IP]);
	
	char sToIP[40];
	::GetWindowText(m_hWndWindowItems[EDT_TO_IP], sToIP, 40);
	
	uint8_t ui128ToIpHash[16];
	memset(ui128ToIpHash, 0, 16);
	
	if (iIpToLen == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_IP_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	else if (HashIP(sToIP, ui128ToIpHash) == false)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(sToIP) + " " + clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	if (memcmp(ui128ToIpHash, ui128FromIpHash, 16) <= 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_NO_VALID_IP_RANGE_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
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
		
		if (ban_time <= acc_time || ban_time == (time_t) - 1)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_BAD_TIME_SPECIFIED_BAN_EXPIRED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
			
			return false;
		}
	}
	
	if (pRangeBanToChange == NULL)
	{
		RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
		if (pRangeBan == NULL)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in clsRangeBanDialog::OnAccept\n");
			return false;
		}
		
		if (bTempBan == true)
		{
			pRangeBan->ui8Bits |= clsBanManager::TEMP;
			pRangeBan->tTempBanExpire = ban_time;
		}
		else
		{
			pRangeBan->ui8Bits |= clsBanManager::PERM;
		}
		
		strcpy(pRangeBan->sIpFrom, sFromIP);
		memcpy(pRangeBan->ui128FromIpHash, ui128FromIpHash, 16);
		
		strcpy(pRangeBan->sIpTo, sToIP);
		memcpy(pRangeBan->ui128ToIpHash, ui128ToIpHash, 16);
		
		if (::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			pRangeBan->ui8Bits |= clsBanManager::FULL;
		}
		
		RangeBanItem * curBan = NULL,
		               * nxtBan = clsBanManager::mPtr->pRangeBanListS;
		               
		// PPK ... don't add range ban if is already here same range ban
		while (nxtBan != NULL)
		{
			curBan = nxtBan;
			nxtBan = curBan->pNext;
			
			if (((curBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true && acc_time > curBan->tTempBanExpire)
			{
				clsBanManager::mPtr->RemRange(curBan);
				delete curBan;
				
				continue;
			}
			
			if (memcmp(curBan->ui128FromIpHash, pRangeBan->ui128FromIpHash, 16) == 0 && memcmp(curBan->ui128ToIpHash, pRangeBan->ui128ToIpHash, 16) == 0)
			{
				delete pRangeBan;
				
				::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_SIMILAR_BAN_EXIST], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		}
		
		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);
		
		if (iReasonLen != 0)
		{
			pRangeBan->sReason = (char *)malloc(iReasonLen + 1);
			if (pRangeBan->sReason == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sReason in clsRangeBanDialog::OnAccept\n", iReasonLen + 1);
				
				delete pRangeBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_REASON], pRangeBan->sReason, iReasonLen + 1);
		}
		
		int iByLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_BY]);
		
		if (iByLen != 0)
		{
			pRangeBan->sBy = (char *)malloc(iByLen + 1);
			if (pRangeBan->sBy == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sBy in clsRangeBanDialog::OnAccept\n", iByLen + 1);
				
				delete pRangeBan;
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_BY], pRangeBan->sBy, iByLen + 1);
		}
		
		clsBanManager::mPtr->AddRange(pRangeBan);
		
		return true;
	}
	else
	{
		if (bTempBan == true)
		{
			pRangeBanToChange->ui8Bits &= ~clsBanManager::PERM;
			
			pRangeBanToChange->ui8Bits |= clsBanManager::TEMP;
			pRangeBanToChange->tTempBanExpire = ban_time;
		}
		else
		{
			pRangeBanToChange->ui8Bits &= ~clsBanManager::TEMP;
			
			pRangeBanToChange->ui8Bits |= clsBanManager::PERM;
		}
		
		strcpy(pRangeBanToChange->sIpFrom, sFromIP);
		memcpy(pRangeBanToChange->ui128FromIpHash, ui128FromIpHash, 16);
		
		strcpy(pRangeBanToChange->sIpTo, sToIP);
		memcpy(pRangeBanToChange->ui128ToIpHash, ui128ToIpHash, 16);
		
		if (::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			pRangeBanToChange->ui8Bits |= clsBanManager::FULL;
		}
		else
		{
			pRangeBanToChange->ui8Bits &= ~clsBanManager::FULL;
		}
		
		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);
		
		char * sReason = nullptr;
		if (iReasonLen != 0)
		{
			sReason = (char *)malloc(iReasonLen + 1);
			if (sReason == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sReason in clsRangeBanDialog::OnAccept\n", iReasonLen + 1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_REASON], sReason, iReasonLen + 1);
		}
		
		if (iReasonLen != 0)
		{
			if (pRangeBanToChange->sReason == NULL || strcmp(pRangeBanToChange->sReason, sReason) != NULL)
			{
				safe_free_and_init(pRangeBanToChange->sReason, sReason);
			}
		}
		else
		{
			safe_free(pRangeBanToChange->sReason);
		}
		
		if (sReason != NULL && pRangeBanToChange->sReason != sReason)
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
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sBy in clsRangeBanDialog::OnAccept\n", iByLen + 1);
				
				return false;
			}
			
			::GetWindowText(m_hWndWindowItems[EDT_BY], sBy, iByLen + 1);
		}
		
		if (iByLen != 0)
		{
			if (pRangeBanToChange->sBy == NULL || strcmp(pRangeBanToChange->sBy, sBy) != NULL)
			{
				safe_free_and_init(pRangeBanToChange->sBy, sBy);
			}
		}
		else
		{
			safe_free(pRangeBanToChange->sBy);
		}
		
		if (sBy != NULL && pRangeBanToChange->sBy != sBy)
		{
			free(sBy);
		}
		
		if (clsRangeBansDialog::mPtr != NULL)
		{
			clsRangeBansDialog::mPtr->RemoveRangeBan(pRangeBanToChange);
			clsRangeBansDialog::mPtr->AddRangeBan(pRangeBanToChange);
		}
		
		return true;
	}
}
//------------------------------------------------------------------------------

void clsRangeBanDialog::RangeBanDeleted(RangeBanItem * pRangeBan)
{
	if (pRangeBanToChange == NULL || pRangeBan != pRangeBanToChange)
	{
		return;
	}
	
	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], clsLanguageManager::mPtr->sTexts[LAN_RANGE_BAN_DELETED_ACCEPT_TO_NEW], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
