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
#include "BansDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashBanManager.h"
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "BanDialog.h"
//---------------------------------------------------------------------------
clsBansDialog * clsBansDialog::mPtr = nullptr;
//---------------------------------------------------------------------------
#define IDC_CHANGE_BAN      1000
#define IDC_REMOVE_BANS     1001
//---------------------------------------------------------------------------
static ATOM atomBansDialog = 0;
//---------------------------------------------------------------------------

clsBansDialog::clsBansDialog() : iFilterColumn(0), iSortColumn(0), bSortAscending(true)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

clsBansDialog::~clsBansDialog()
{
	clsBansDialog::mPtr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsBansDialog::StaticBansDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	clsBansDialog * pBansDialog = (clsBansDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pBansDialog == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pBansDialog->BansDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsBansDialog::BansDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_PERM_BANS], NULL, (rcParent.right / 2) + 1, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2,
		               rcParent.right - (rcParent.right / 2) - 3, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_TEMP_BANS], NULL, 2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, (rcParent.right / 2) - 2, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[CB_FILTER], NULL, (rcParent.right / 2) + 3, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin,
		               rcParent.right - (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[EDT_FILTER], NULL, 11, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[GB_FILTER], NULL, 3, rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_BANS], NULL, 0, 0, rcParent.right - 6, rcParent.bottom - clsGuiSettingManager::iOneLineGB - (2 * clsGuiSettingManager::iEditHeight) - 14, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_ADD_BAN], NULL, 0, 0, rcParent.right - 4, clsGuiSettingManager::iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case (BTN_ADD_BAN+100):
		{
			clsBanDialog * pBanDialog = new (std::nothrow) clsBanDialog();
			
			if (pBanDialog != NULL)
			{
				pBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE]);
			}
			
			return 0;
		}
		case IDC_CHANGE_BAN:
		{
			ChangeBan();
			return 0;
		}
		case IDC_REMOVE_BANS:
			RemoveBans();
			return 0;
		case CB_FILTER:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]) != 0)
				{
					FilterBans();
				}
			}
			
			break;
		case BTN_CLEAR_TEMP_BANS:
			if (::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			clsBanManager::mPtr->ClearTemp();
			AddAllBans();
			
			return 0;
		case BTN_CLEAR_PERM_BANS:
			if (::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			clsBanManager::mPtr->ClearPerm();
			AddAllBans();
			
			return 0;
		case IDOK:   // NM_RETURN
		{
			HWND hWndFocus = ::GetFocus();
			
			if (hWndFocus == m_hWndWindowItems[LV_BANS])
			{
				ChangeBan();
				return 0;
			}
			else if (hWndFocus == m_hWndWindowItems[EDT_FILTER])
			{
				FilterBans();
				return 0;
			}
			
			break;
		}
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		}
		
		break;
	case WM_CONTEXTMENU:
		OnContextMenu((HWND)wParam, lParam);
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_BANS])
		{
			if (((LPNMHDR)lParam)->code == LVN_COLUMNCLICK)
			{
				OnColumnClick((LPNMLISTVIEW)lParam);
			}
			else if (((LPNMHDR)lParam)->code == NM_DBLCLK)
			{
				if (((LPNMITEMACTIVATE)lParam)->iItem == -1)
				{
					break;
				}
				
				BanItem * pBan = reinterpret_cast<BanItem *>(ListViewGetItem(m_hWndWindowItems[LV_BANS], ((LPNMITEMACTIVATE)lParam)->iItem));
				
				clsBanDialog * pBanDialog = new (std::nothrow) clsBanDialog();
				
				if (pBanDialog != NULL)
				{
					pBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pBan);
				}
				
				return 0;
			}
			else if (((LPNMHDR)lParam)->code == NM_RETURN)
			{
				ChangeBan();
				return 0;
			}
		}
		
		break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_BANS_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_BANS_WINDOW_HEIGHT));
		
		return 0;
	}
	case WM_CLOSE:
	{
		RECT rcBans;
		::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcBans);
		
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_BANS_WINDOW_WIDTH, rcBans.right - rcBans.left);
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_BANS_WINDOW_HEIGHT, rcBans.bottom - rcBans.top);
		
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_BANS_NICK, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 0, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_BANS_IP, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 1, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_BANS_REASON, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 2, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_BANS_EXPIRE, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 3, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_BANS_BY, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 4, 0));
		
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		clsServerManager::hWndActiveDialog = nullptr;
		
		break;
	}
	case WM_NCDESTROY:
	{
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_SETFOCUS:
		if ((UINT)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETSELECTEDCOUNT, 0, 0) != 0)
		{
			::SetFocus(m_hWndWindowItems[LV_BANS]);
		}
		else
		{
			::SetFocus(m_hWndWindowItems[EDT_FILTER]);
		}
		
		return 0;
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE)
		{
			clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
		}
		
		break;
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void clsBansDialog::DoModal(HWND hWndParent)
{
	if (atomBansDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_BansDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomBansDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_WIDTH) / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_HEIGHT) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomBansDialog), clsLanguageManager::mPtr->sTexts[LAN_BANS],
	                                                    WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                                                    iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_HEIGHT),
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticBansDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[BTN_ADD_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ADD_NEW_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                  2, 2, (rcParent.right / 3) - 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_BAN + 100), clsServerManager::hInstance, NULL);
	                                                  
	m_hWndWindowItems[LV_BANS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
	                                              3, clsGuiSettingManager::iEditHeight + 6, rcParent.right - 6, rcParent.bottom - clsGuiSettingManager::iOneLineGB - (2 * clsGuiSettingManager::iEditHeight) - 14, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	
	m_hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_FILTER_BANS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                
	m_hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);
	
	m_hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                (rcParent.right / 2) + 3, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight,
	                                                m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER, clsServerManager::hInstance, NULL);
	                                                
	m_hWndWindowItems[BTN_CLEAR_TEMP_BANS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CLEAR_TEMP_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                          2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, (rcParent.right / 2) - 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_TEMP_BANS, clsServerManager::hInstance, NULL);
	                                                          
	m_hWndWindowItems[BTN_CLEAR_PERM_BANS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CLEAR_PERM_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                          (rcParent.right / 2) + 1, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, rcParent.right - (rcParent.right / 2) - 3, clsGuiSettingManager::iEditHeight,
	                                                          m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_PERM_BANS, clsServerManager::hInstance, NULL);
	                                                          
	for (uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if (m_hWndWindowItems[ui8i] == NULL)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	RECT rcBans;
	::GetClientRect(m_hWndWindowItems[LV_BANS], &rcBans);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	
	const int iBansStrings[] = { LAN_NICK, LAN_IP, LAN_REASON, LAN_EXPIRE, LAN_BANNED_BY };
	const int iBansWidths[] = { GUISETINT_BANS_NICK, GUISETINT_BANS_IP, GUISETINT_BANS_REASON, GUISETINT_BANS_EXPIRE, GUISETINT_BANS_BY };
	
	for (uint8_t ui8i = 0; ui8i < 5; ui8i++)
	{
		lvColumn.cx = clsGuiSettingManager::mPtr->i32Integers[iBansWidths[ui8i]];
		lvColumn.pszText = clsLanguageManager::mPtr->sTexts[iBansStrings[ui8i]];
		lvColumn.iSubItem = ui8i;
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_INSERTCOLUMN, ui8i, (LPARAM)&lvColumn);
		
		::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[iBansStrings[ui8i]]);
	}
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_BANS], bSortAscending, iSortColumn);
	
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);
	
	AddAllBans();
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void clsBansDialog::AddAllBans()
{
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEALLITEMS, 0, 0);
	
	time_t acc_time;
	time(&acc_time);
	
	BanItem * curBan = NULL,
	          * nextBan = clsBanManager::mPtr->pTempBanListS;
	          
	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->pNext;
		
		if (acc_time > curBan->tTempBanExpire)
		{
			clsBanManager::mPtr->Rem(curBan);
			delete curBan;
			
			continue;
		}
		
		AddBan(curBan);
	}
	
	nextBan = clsBanManager::mPtr->pPermBanListS;
	
	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->pNext;
		
		AddBan(curBan);
	}
	
	ListViewSelectFirstItem(m_hWndWindowItems[LV_BANS]);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void clsBansDialog::AddBan(const BanItem * pBan)
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ListViewGetInsertPosition(m_hWndWindowItems[LV_BANS], pBan, bSortAscending, CompareBans);
	
	string sTxt = (pBan->sNick == NULL ? "" : pBan->sNick);
	if ((pBan->ui8Bits & clsBanManager::NICK) == clsBanManager::NICK)
	{
		sTxt += " (";
		sTxt += clsLanguageManager::mPtr->sTexts[LAN_BANNED];
		sTxt += ")";
	}
	lvItem.pszText = (char*) sTxt.c_str();
	lvItem.lParam = (LPARAM)pBan;
	
	int i = (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	
	if (i == -1)
	{
		return;
	}
	
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = i;
	lvItem.iSubItem = 1;
	
	sTxt = (pBan->sIp[0] == '\0' ? "" : pBan->sIp);
	if ((pBan->ui8Bits & clsBanManager::IP) == clsBanManager::IP)
	{
		sTxt += " (";
		if ((pBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL)
		{
			sTxt += clsLanguageManager::mPtr->sTexts[LAN_FULL_BANNED];
		}
		else
		{
			sTxt += clsLanguageManager::mPtr->sTexts[LAN_BANNED];
		}
		sTxt += ")";
	}
	lvItem.pszText = (char*) sTxt.c_str();
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	
	lvItem.iSubItem = 2;
	lvItem.pszText = (pBan->sReason == NULL ? "" : pBan->sReason);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	
	char msg[256];
	msg[0] = 0;
	if ((pBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
	{
		struct tm *tm = localtime(&pBan->tTempBanExpire);
		strftime(msg, 256, "%c", tm);
		
		lvItem.iSubItem = 3;
		lvItem.pszText = msg;
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
	
	lvItem.iSubItem = 4;
	lvItem.pszText = (pBan->sBy == NULL ? "" : pBan->sBy);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

int clsBansDialog::CompareBans(const void * pItem, const void * pOtherItem)
{
	const BanItem * pFirstBan = reinterpret_cast<const BanItem *>(pItem);
	const BanItem * pSecondBan = reinterpret_cast<const BanItem *>(pOtherItem);
	
	switch (clsBansDialog::mPtr->iSortColumn)
	{
	case 0:
		return _stricmp(pFirstBan->sNick == NULL ? "" : pFirstBan->sNick, pSecondBan->sNick == NULL ? "" : pSecondBan->sNick);
	case 1:
		return memcmp(pFirstBan->ui128IpHash, pSecondBan->ui128IpHash, 16);
	case 2:
		return _stricmp(pFirstBan->sReason == NULL ? "" : pFirstBan->sReason, pSecondBan->sReason == NULL ? "" : pSecondBan->sReason);
	case 3:
		if ((pFirstBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
		{
			if ((pSecondBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else if ((pSecondBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
		{
			return 1;
		}
		else
		{
			return (pFirstBan->tTempBanExpire > pSecondBan->tTempBanExpire) ? 1 : ((pFirstBan->tTempBanExpire == pSecondBan->tTempBanExpire) ? 0 : -1);
		}
	case 4:
		return _stricmp(pFirstBan->sBy == NULL ? "" : pFirstBan->sBy, pSecondBan->sBy == NULL ? "" : pSecondBan->sBy);
	default:
		return 0; // never happen, but we need to make compiler/complainer happy ;o)
	}
}
//------------------------------------------------------------------------------

void clsBansDialog::OnColumnClick(const LPNMLISTVIEW &pListView)
{
	if (pListView->iSubItem != iSortColumn)
	{
		bSortAscending = true;
		iSortColumn = pListView->iSubItem;
	}
	else
	{
		bSortAscending = !bSortAscending;
	}
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_BANS], bSortAscending, iSortColumn);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareBans);
}
//------------------------------------------------------------------------------

int CALLBACK clsBansDialog::SortCompareBans(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	int iResult = clsBansDialog::mPtr->CompareBans((void *)lParam1, (void *)lParam2);
	
	return (clsBansDialog::mPtr->bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void clsBansDialog::RemoveBans()
{
	if (::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
	{
		return;
	}
	
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	int iSel = -1;
	
	while ((iSel = (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED)) != -1)
	{
		BanItem *pBan = reinterpret_cast<BanItem *>(ListViewGetItem(m_hWndWindowItems[LV_BANS], iSel));
		
		clsBanManager::mPtr->Rem(pBan, true);
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEITEM, iSel, 0);
	}
	
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void clsBansDialog::FilterBans()
{
	int iTextLength = ::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]);
	
	if (iTextLength == 0)
	{
		sFilterString.clear();
		
		AddAllBans();
	}
	else
	{
		iFilterColumn = (int)::SendMessage(m_hWndWindowItems[CB_FILTER], CB_GETCURSEL, 0, 0);
		
		char buf[65];
		
		int iLen = ::GetWindowText(m_hWndWindowItems[EDT_FILTER], buf, 65);
		
		for (int i = 0; i < iLen; i++)
		{
			buf[i] = (char)tolower(buf[i]);
		}
		
		sFilterString = buf;
		
		::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEALLITEMS, 0, 0);
		
		time_t acc_time;
		time(&acc_time);
		
		BanItem * curBan = NULL,
		          * nextBan = clsBanManager::mPtr->pTempBanListS;
		          
		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->pNext;
			
			if (acc_time > curBan->tTempBanExpire)
			{
				clsBanManager::mPtr->Rem(curBan);
				delete curBan;
				
				continue;
			}
			
			if (FilterBan(curBan) == false)
			{
				AddBan(curBan);
			}
		}
		
		nextBan = clsBanManager::mPtr->pPermBanListS;
		
		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->pNext;
			
			if (FilterBan(curBan) == false)
			{
				AddBan(curBan);
			}
		}
		
		ListViewSelectFirstItem(m_hWndWindowItems[LV_BANS]);
		
		::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	}
}
//------------------------------------------------------------------------------

bool clsBansDialog::FilterBan(const BanItem * pBan)
{
	switch (iFilterColumn)
	{
	case 0:
		if (pBan->sNick != NULL && stristr2(pBan->sNick, sFilterString.c_str()) != NULL)
		{
			return false;
		}
		break;
	case 1:
		if (pBan->sIp[0] != '\0' && stristr2(pBan->sIp, sFilterString.c_str()) != NULL)
		{
			return false;
		}
		break;
	case 2:
		if (pBan->sReason != NULL && stristr2(pBan->sReason, sFilterString.c_str()) != NULL)
		{
			return false;
		}
		break;
	case 3:
		if ((pBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
		{
			char msg[256];
			struct tm *tm = localtime(&pBan->tTempBanExpire);
			strftime(msg, 256, "%c", tm);
			
			if (stristr2(msg, sFilterString.c_str()) != NULL)
			{
				return false;
			}
		}
		break;
	case 4:
		if (pBan->sBy != NULL && stristr2(pBan->sBy, sFilterString.c_str()) != NULL)
		{
			return false;
		}
		break;
	}
	
	return true;
}
//------------------------------------------------------------------------------

void clsBansDialog::RemoveBan(const BanItem * pBan)
{
	int iPos = ListViewGetItemPosition(m_hWndWindowItems[LV_BANS], (void *)pBan);
	
	if (iPos != -1)
	{
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEITEM, iPos, 0);
	}
}
//------------------------------------------------------------------------------

void clsBansDialog::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if (hWindow != m_hWndWindowItems[LV_BANS])
	{
		return;
	}
	
	UINT UISelectedCount = (UINT)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETSELECTEDCOUNT, 0, 0);
	
	if (UISelectedCount == 0)
	{
		return;
	}
	
	HMENU hMenu = ::CreatePopupMenu();
	
	if (UISelectedCount == 1)
	{
		::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_BAN, clsLanguageManager::mPtr->sTexts[LAN_CHANGE]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	}
	
	::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_BANS, clsLanguageManager::mPtr->sTexts[LAN_REMOVE]);
	
	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);
	
	ListViewGetMenuPos(m_hWndWindowItems[LV_BANS], iX, iY);
	
	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWndWindowItems[WINDOW_HANDLE], NULL);
	
	::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsBansDialog::ChangeBan()
{
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	BanItem * pBan = (BanItem *)ListViewGetItem(m_hWndWindowItems[LV_BANS], iSel);
	
	clsBanDialog * pBanDialog = new (std::nothrow) clsBanDialog();
	
	if (pBanDialog != NULL)
	{
		pBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pBan);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
