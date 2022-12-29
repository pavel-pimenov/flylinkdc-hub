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

RangeBanDialog::RangeBanDialog() : m_pRangeBanToChange(nullptr)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

RangeBanDialog::~RangeBanDialog()
{
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RangeBanDialog::StaticRangeBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RangeBanDialog * pRangeBanDialog = (RangeBanDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pRangeBanDialog == nullptr)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pRangeBanDialog->RangeBanDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RangeBanDialog::RangeBanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		::SetFocus(m_hWndWindowItems[EDT_FROM_IP]);
		return 0;

	}

	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RangeBanDialog::DoModal(HWND hWndParent, RangeBanItem * pRangeBan/* = nullptr*/)
{
	m_pRangeBanToChange = pRangeBan;

	if(atomRangeBanDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RangeBanDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;

		atomRangeBanDialog = ::RegisterClassEx(&m_wc);
	}

	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);

	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(300) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(307) / 2);

	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRangeBanDialog), LanguageManager::m_Ptr->m_sTexts[LAN_RANGE_BAN],
	                                   WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(300), ScaleGui(307),
	                                   hWndParent, nullptr, ServerManager::m_hInstance, nullptr);

	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr)
	{
		return;
	}

	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];

	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRangeBanDialogProc);

	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	{
		int iHeight = GuiSettingManager::m_iOneLineOneChecksGB + (2 * GuiSettingManager::m_iOneLineGB) + (GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iOneLineGB + 5) + GuiSettingManager::m_iEditHeight + 6;

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

	m_hWndWindowItems[GB_RANGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                              3, 0, rcParent.right - 6, GuiSettingManager::m_iOneLineOneChecksGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[EDT_FROM_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                 11, GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2) - 13, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_FROM_IP], EM_SETLIMITTEXT, 39, 0);

	m_hWndWindowItems[EDT_TO_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                               (rcParent.right / 2) + 3, GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2) - 13, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_TO_IP], EM_SETLIMITTEXT, 39, 0);

	m_hWndWindowItems[BTN_FULL_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_FULL_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                  11, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, rcParent.right - 22, GuiSettingManager::m_iCheckHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	int iPosY = GuiSettingManager::m_iOneLineOneChecksGB;

	m_hWndWindowItems[GB_REASON] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REASON], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                               3, iPosY, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[EDT_REASON] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                11, iPosY + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - 22, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
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

	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_DISABLED | DTS_SHORTDATECENTURYFORMAT,
	        iThird + 16, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight, iThird - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME] = ::CreateWindowEx(0, DATETIMEPICK_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_DISABLED | DTS_TIMEFORMAT | DTS_UPDOWN,
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

	if(m_pRangeBanToChange != nullptr)
	{
		::SetWindowText(m_hWndWindowItems[EDT_FROM_IP], m_pRangeBanToChange->m_sIpFrom);
		::SetWindowText(m_hWndWindowItems[EDT_TO_IP], m_pRangeBanToChange->m_sIpTo);

		if(((m_pRangeBanToChange->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true)
		{
			::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_SETCHECK, BST_CHECKED, 0);
		}

		if(m_pRangeBanToChange->m_sReason != nullptr)
		{
			::SetWindowText(m_hWndWindowItems[EDT_REASON], m_pRangeBanToChange->m_sReason);
		}

		if(m_pRangeBanToChange->m_sBy != nullptr)
		{
			::SetWindowText(m_hWndWindowItems[EDT_BY], m_pRangeBanToChange->m_sBy);
		}

		if(((m_pRangeBanToChange->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
		{
			::SendMessage(m_hWndWindowItems[RB_PERM_BAN], BM_SETCHECK, BST_UNCHECKED, 0);
			::SendMessage(m_hWndWindowItems[RB_TEMP_BAN], BM_SETCHECK, BST_CHECKED, 0);

			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_DATE], TRUE);
			::EnableWindow(m_hWndWindowItems[DT_TEMP_BAN_EXPIRE_TIME], TRUE);

			SYSTEMTIME stDateTime = { 0 };

			struct tm *tm = localtime(&m_pRangeBanToChange->m_tTempBanExpire);

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

bool RangeBanDialog::OnAccept()
{
	int iIpFromLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_FROM_IP]);

	char sFromIP[40];
	::GetWindowText(m_hWndWindowItems[EDT_FROM_IP], sFromIP, 40);

	uint8_t ui128FromIpHash[16];
	memset(ui128FromIpHash, 0, 16);

	if(iIpFromLen == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	else if(HashIP(sFromIP, ui128FromIpHash) == false)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(sFromIP) + " " + LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	int iIpToLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_TO_IP]);

	char sToIP[40];
	::GetWindowText(m_hWndWindowItems[EDT_TO_IP], sToIP, 40);

	uint8_t ui128ToIpHash[16];
	memset(ui128ToIpHash, 0, 16);

	if(iIpToLen == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	else if(HashIP(sToIP, ui128ToIpHash) == false)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(sToIP) + " " + LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_VALID_IP_ADDRESS]).c_str(), g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if(memcmp(ui128ToIpHash, ui128FromIpHash, 16) <= 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_RANGE_SPECIFIED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
		return false;
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

		if(ban_time <= acc_time || ban_time == (time_t)-1)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED_BAN_EXPIRED], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);

			return false;
		}
	}

	if(m_pRangeBanToChange == nullptr)
	{
		RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
		if(pRangeBan == nullptr)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in RangeBanDialog::OnAccept\n");
			return false;
		}

		if(bTempBan == true)
		{
			pRangeBan->m_ui8Bits |= BanManager::TEMP;
			pRangeBan->m_tTempBanExpire = ban_time;
		}
		else
		{
			pRangeBan->m_ui8Bits |= BanManager::PERM;
		}

		strcpy(pRangeBan->m_sIpFrom, sFromIP);
		memcpy(pRangeBan->m_ui128FromIpHash, ui128FromIpHash, 16);

		strcpy(pRangeBan->m_sIpTo, sToIP);
		memcpy(pRangeBan->m_ui128ToIpHash, ui128ToIpHash, 16);

		if(::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			pRangeBan->m_ui8Bits |= BanManager::FULL;
		}

		RangeBanItem * curBan = nullptr,
		               * nxtBan = BanManager::m_Ptr->m_pRangeBanListS;

		// PPK ... don't add range ban if is already here same range ban
		while(nxtBan != nullptr)
		{
			curBan = nxtBan;
			nxtBan = curBan->m_pNext;

			if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true && acc_time > curBan->m_tTempBanExpire)
			{
				BanManager::m_Ptr->RemRange(curBan);
				delete curBan;

				continue;
			}

			if(memcmp(curBan->m_ui128FromIpHash, pRangeBan->m_ui128FromIpHash, 16) == 0 && memcmp(curBan->m_ui128ToIpHash, pRangeBan->m_ui128ToIpHash, 16) == 0)
			{
				delete pRangeBan;

				::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_SIMILAR_BAN_EXIST], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
		}

		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);

		if(iReasonLen != 0)
		{
			pRangeBan->m_sReason = (char *)malloc(iReasonLen+1);
			if(pRangeBan->m_sReason == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for m_sReason in RangeBanDialog::OnAccept\n", iReasonLen+1);

				delete pRangeBan;

				return false;
			}

			::GetWindowText(m_hWndWindowItems[EDT_REASON], pRangeBan->m_sReason, iReasonLen+1);
		}

		int iByLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_BY]);

		if(iByLen != 0)
		{
			pRangeBan->m_sBy = (char *)malloc(iByLen+1);
			if(pRangeBan->m_sBy == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for m_sBy in RangeBanDialog::OnAccept\n", iByLen+1);

				delete pRangeBan;

				return false;
			}

			::GetWindowText(m_hWndWindowItems[EDT_BY], pRangeBan->m_sBy, iByLen+1);
		}

		BanManager::m_Ptr->AddRange(pRangeBan);

		return true;
	}
	else
	{
		if(bTempBan == true)
		{
			m_pRangeBanToChange->m_ui8Bits &= ~BanManager::PERM;

			m_pRangeBanToChange->m_ui8Bits |= BanManager::TEMP;
			m_pRangeBanToChange->m_tTempBanExpire = ban_time;
		}
		else
		{
			m_pRangeBanToChange->m_ui8Bits &= ~BanManager::TEMP;

			m_pRangeBanToChange->m_ui8Bits |= BanManager::PERM;
		}

		strcpy(m_pRangeBanToChange->m_sIpFrom, sFromIP);
		memcpy(m_pRangeBanToChange->m_ui128FromIpHash, ui128FromIpHash, 16);

		strcpy(m_pRangeBanToChange->m_sIpTo, sToIP);
		memcpy(m_pRangeBanToChange->m_ui128ToIpHash, ui128ToIpHash, 16);

		if(::SendMessage(m_hWndWindowItems[BTN_FULL_BAN], BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			m_pRangeBanToChange->m_ui8Bits |= BanManager::FULL;
		}
		else
		{
			m_pRangeBanToChange->m_ui8Bits &= ~BanManager::FULL;
		}

		int iReasonLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_REASON]);

		char * sReason = nullptr;
		if(iReasonLen != 0)
		{
			sReason = (char *)malloc(iReasonLen+1);
			if(sReason == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sReason in RangeBanDialog::OnAccept\n", iReasonLen+1);

				return false;
			}

			::GetWindowText(m_hWndWindowItems[EDT_REASON], sReason, iReasonLen+1);
		}

		if(iReasonLen != 0)
		{
			if(m_pRangeBanToChange->m_sReason == nullptr || strcmp(m_pRangeBanToChange->m_sReason, sReason) != 0)
			{
				if(m_pRangeBanToChange->m_sReason != nullptr)
				{
					free(m_pRangeBanToChange->m_sReason);
					m_pRangeBanToChange->m_sReason = nullptr;
				}

				m_pRangeBanToChange->m_sReason = sReason;
			}
		}
		else if(m_pRangeBanToChange->m_sReason != nullptr)
		{
			free(m_pRangeBanToChange->m_sReason);
			m_pRangeBanToChange->m_sReason = nullptr;
		}

		if(sReason != nullptr && (m_pRangeBanToChange->m_sReason != sReason))
		{
			free(sReason);
		}

		int iByLen = ::GetWindowTextLength(m_hWndWindowItems[EDT_BY]);

		char * sBy = nullptr;
		if(iByLen != 0)
		{
			sBy = (char *)malloc(iByLen+1);
			if(sBy == nullptr)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sBy in RangeBanDialog::OnAccept\n", iByLen+1);

				return false;
			}

			::GetWindowText(m_hWndWindowItems[EDT_BY], sBy, iByLen+1);
		}

		if(iByLen != 0)
		{
			if(m_pRangeBanToChange->m_sBy == nullptr || strcmp(m_pRangeBanToChange->m_sBy, sBy) != 0)
			{
				if(m_pRangeBanToChange->m_sBy != nullptr)
				{
					free(m_pRangeBanToChange->m_sBy);
					m_pRangeBanToChange->m_sBy = nullptr;
				}

				m_pRangeBanToChange->m_sBy = sBy;
			}
		}
		else if(m_pRangeBanToChange->m_sBy != nullptr)
		{
			free(m_pRangeBanToChange->m_sBy);
			m_pRangeBanToChange->m_sBy = nullptr;
		}

		if(sBy != nullptr && (m_pRangeBanToChange->m_sBy != sBy))
		{
			free(sBy);
		}

		if(RangeBansDialog::m_Ptr != nullptr)
		{
			RangeBansDialog::m_Ptr->RemoveRangeBan(m_pRangeBanToChange);
			RangeBansDialog::m_Ptr->AddRangeBan(m_pRangeBanToChange);
		}

		return true;
	}
}
//------------------------------------------------------------------------------

void RangeBanDialog::RangeBanDeleted(RangeBanItem * pRangeBan)
{
	if(m_pRangeBanToChange == nullptr || pRangeBan != m_pRangeBanToChange)
	{
		return;
	}

	::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE_BAN_DELETED_ACCEPT_TO_NEW], g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------
