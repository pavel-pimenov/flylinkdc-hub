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
#include "RangeBansDialog.h"
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
#include "RangeBanDialog.h"
//---------------------------------------------------------------------------
clsRangeBansDialog * clsRangeBansDialog::mPtr = nullptr;
//---------------------------------------------------------------------------
#define IDC_CHANGE_RANGE_BAN      1000
#define IDC_REMOVE_RANGE_BANS     1001
//---------------------------------------------------------------------------
static ATOM atomRangeBansDialog = 0;
//---------------------------------------------------------------------------

clsRangeBansDialog::clsRangeBansDialog() : iFilterColumn(0), iSortColumn(0), bSortAscending(true)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

clsRangeBansDialog::~clsRangeBansDialog()
{
	clsRangeBansDialog::mPtr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsRangeBansDialog::StaticRangeBansDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	clsRangeBansDialog * pRangeBansDialog = (clsRangeBansDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pRangeBansDialog == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pRangeBansDialog->RangeBansDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsRangeBansDialog::RangeBansDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS], NULL, (rcParent.right / 2) + 1, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2,
		               rcParent.right - (rcParent.right / 2) - 3, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS], NULL, 2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, (rcParent.right / 2) - 2, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[CB_FILTER], NULL, (rcParent.right / 2) + 3, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin,
		               rcParent.right - (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[EDT_FILTER], NULL, 11, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[GB_FILTER], NULL, 3, rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_RANGE_BANS], NULL, 0, 0, rcParent.right - 6, rcParent.bottom - clsGuiSettingManager::iOneLineGB - (2 * clsGuiSettingManager::iEditHeight) - 14, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_ADD_RANGE_BAN], NULL, 0, 0, rcParent.right - 4, clsGuiSettingManager::iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case (BTN_ADD_RANGE_BAN+100):
		{
		
			clsRangeBanDialog * pRangeBanDialog = new (std::nothrow) clsRangeBanDialog();
			
			if (pRangeBanDialog != NULL)
			{
				pRangeBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE]);
			}
			
			return 0;
		}
		case IDC_CHANGE_RANGE_BAN:
		{
			ChangeRangeBan();
			return 0;
		}
		case IDC_REMOVE_RANGE_BANS:
			RemoveRangeBans();
			return 0;
		case CB_FILTER:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]) != 0)
				{
					FilterRangeBans();
				}
			}
			
			break;
		case BTN_CLEAR_RANGE_TEMP_BANS:
			if (::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			clsBanManager::mPtr->ClearTempRange();
			AddAllRangeBans();
			
			return 0;
		case BTN_CLEAR_RANGE_PERM_BANS:
			if (::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			clsBanManager::mPtr->ClearPermRange();
			AddAllRangeBans();
			
			return 0;
		case IDOK:   // NM_RETURN
		{
			HWND hWndFocus = ::GetFocus();
			
			if (hWndFocus == m_hWndWindowItems[LV_RANGE_BANS])
			{
				ChangeRangeBan();
				return 0;
			}
			else if (hWndFocus == m_hWndWindowItems[EDT_FILTER])
			{
				FilterRangeBans();
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
		if (((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_RANGE_BANS])
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
				
				RangeBanItem * pRangeBan = reinterpret_cast<RangeBanItem *>(ListViewGetItem(m_hWndWindowItems[LV_RANGE_BANS], ((LPNMITEMACTIVATE)lParam)->iItem));
				
				clsRangeBanDialog * pRangeBanDialog = new (std::nothrow) clsRangeBanDialog();
				
				if (pRangeBanDialog != NULL)
				{
					pRangeBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pRangeBan);
				}
				
				return 0;
			}
		}
		
		break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_RANGE_BANS_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_RANGE_BANS_WINDOW_HEIGHT));
		
		return 0;
	}
	case WM_CLOSE:
	{
		RECT rcRangeBans;
		::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcRangeBans);
		
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_RANGE_BANS_WINDOW_WIDTH, rcRangeBans.right - rcRangeBans.left);
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_RANGE_BANS_WINDOW_HEIGHT, rcRangeBans.bottom - rcRangeBans.top);
		
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_RANGE_BANS_RANGE, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 0, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_RANGE_BANS_REASON, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 1, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_RANGE_BANS_EXPIRE, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 2, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_RANGE_BANS_BY, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 3, 0));
		
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
		if ((UINT)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETSELECTEDCOUNT, 0, 0) != 0)
		{
			::SetFocus(m_hWndWindowItems[LV_RANGE_BANS]);
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

void clsRangeBansDialog::DoModal(HWND hWndParent)
{
	if (atomRangeBansDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RangeBansDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomRangeBansDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_WIDTH) / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_HEIGHT) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRangeBansDialog), clsLanguageManager::mPtr->sTexts[LAN_RANGE_BANS],
	                                                    WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                                                    iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_HEIGHT),
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRangeBansDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[BTN_ADD_RANGE_BAN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ADD_NEW_RANGE_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                        2, 2, rcParent.right - 4, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_RANGE_BAN + 100), clsServerManager::hInstance, NULL);
	                                                        
	m_hWndWindowItems[LV_RANGE_BANS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
	                                                    3, clsGuiSettingManager::iEditHeight + 6, rcParent.right - 6, rcParent.bottom - clsGuiSettingManager::iOneLineGB - (2 * clsGuiSettingManager::iEditHeight) - 14, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	
	m_hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_FILTER_RANGE_BANS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                
	m_hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);
	
	m_hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                (rcParent.right / 2) + 3, (rcParent.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iOneLineGB - 6) + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight,
	                                                m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER,
	                                                clsServerManager::hInstance, NULL);
	                                                
	m_hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CLEAR_TEMP_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                                2, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, (rcParent.right / 2) - 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_RANGE_TEMP_BANS, clsServerManager::hInstance, NULL);
	                                                                
	m_hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CLEAR_PERM_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                                (rcParent.right / 2) + 1, rcParent.bottom - clsGuiSettingManager::iEditHeight - 2, rcParent.right - (rcParent.right / 2) - 3, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE],
	                                                                (HMENU)BTN_CLEAR_RANGE_PERM_BANS, clsServerManager::hInstance, NULL);
	                                                                
	for (uint8_t ui8i = 1; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if (m_hWndWindowItems[ui8i] == NULL)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	RECT rcRangeBans;
	::GetClientRect(m_hWndWindowItems[LV_RANGE_BANS], &rcRangeBans);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	
	const int iRangeBansStrings[] = { LAN_RANGE, LAN_REASON, LAN_EXPIRE, LAN_BANNED_BY };
	const int iRangeBansWidths[] = { GUISETINT_RANGE_BANS_RANGE, GUISETINT_RANGE_BANS_REASON, GUISETINT_RANGE_BANS_EXPIRE, GUISETINT_RANGE_BANS_BY };
	
	for (uint8_t ui8i = 0; ui8i < 4; ui8i++)
	{
		lvColumn.cx = clsGuiSettingManager::mPtr->i32Integers[iRangeBansWidths[ui8i]];
		lvColumn.pszText = clsLanguageManager::mPtr->sTexts[iRangeBansStrings[ui8i]];
		lvColumn.iSubItem = ui8i;
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_INSERTCOLUMN, ui8i, (LPARAM)&lvColumn);
		
		::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[iRangeBansStrings[ui8i]]);
	}
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_RANGE_BANS], bSortAscending, iSortColumn);
	
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);
	
	AddAllRangeBans();
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void clsRangeBansDialog::AddAllRangeBans()
{
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEALLITEMS, 0, 0);
	
	time_t acc_time;
	time(&acc_time);
	
	RangeBanItem * curRangeBan = NULL,
	               * nextRangeBan = clsBanManager::mPtr->pRangeBanListS;
	               
	while (nextRangeBan != NULL)
	{
		curRangeBan = nextRangeBan;
		nextRangeBan = curRangeBan->pNext;
		
		if ((((curRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) && acc_time > curRangeBan->tTempBanExpire)
		{
			clsBanManager::mPtr->RemRange(curRangeBan);
			delete curRangeBan;
			
			continue;
		}
		
		AddRangeBan(curRangeBan);
	}
	
	ListViewSelectFirstItem(m_hWndWindowItems[LV_RANGE_BANS]);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void clsRangeBansDialog::AddRangeBan(const RangeBanItem * pRangeBan)
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ListViewGetInsertPosition(m_hWndWindowItems[LV_RANGE_BANS], pRangeBan, bSortAscending, CompareRangeBans);
	
	string sTxt = string(pRangeBan->sIpFrom) + " - " + pRangeBan->sIpTo;
	if ((pRangeBan->ui8Bits & clsBanManager::FULL) == clsBanManager::FULL)
	{
		sTxt += " (";
		sTxt += clsLanguageManager::mPtr->sTexts[LAN_FULL_BANNED];
		sTxt += ")";
	}
	lvItem.pszText = (char*) sTxt.c_str();
	lvItem.lParam = (LPARAM)pRangeBan;
	
	int i = (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	
	if (i == -1)
	{
		return;
	}
	
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = i;
	lvItem.iSubItem = 1;
	lvItem.pszText = (pRangeBan->sReason == NULL ? "" : pRangeBan->sReason);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	
	char msg[256];
	msg[0] = 0;
	
	if ((pRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
	{
		struct tm * tm = localtime(&pRangeBan->tTempBanExpire);
		strftime(msg, 256, "%c", tm);
		
		lvItem.iSubItem = 2;
		lvItem.pszText = msg;
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
	
	lvItem.iSubItem = 3;
	lvItem.pszText = (pRangeBan->sBy == NULL ? "" : pRangeBan->sBy);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

int clsRangeBansDialog::CompareRangeBans(const void * pItem, const void * pOtherItem)
{
	const RangeBanItem * pFirstRangeBan = reinterpret_cast<const RangeBanItem *>(pItem);
	const RangeBanItem * pSecondRangeBan = reinterpret_cast<const RangeBanItem *>(pOtherItem);
	
	switch (clsRangeBansDialog::mPtr->iSortColumn)
	{
	case 0:
		return (memcmp(pFirstRangeBan->ui128FromIpHash.data(), pSecondRangeBan->ui128FromIpHash.data(), 16) > 0) ? 1 :
		       ((memcmp(pFirstRangeBan->ui128FromIpHash.data(), pSecondRangeBan->ui128FromIpHash.data(), 16) == 0) ?
		        (memcmp(pFirstRangeBan->ui128ToIpHash.data(), pSecondRangeBan->ui128ToIpHash.data(), 16) > 0) ? 1 :
		        ((memcmp(pFirstRangeBan->ui128ToIpHash.data(), pSecondRangeBan->ui128ToIpHash.data(), 16) == 0) ? 0 : -1) : -1);
	case 1:
		return _stricmp(pFirstRangeBan->sReason == NULL ? "" : pFirstRangeBan->sReason, pSecondRangeBan->sReason == NULL ? "" : pSecondRangeBan->sReason);
	case 2:
		if ((pFirstRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
		{
			if ((pSecondRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else if ((pSecondRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
		{
			return 1;
		}
		else
		{
			return (pFirstRangeBan->tTempBanExpire > pSecondRangeBan->tTempBanExpire) ? 1 : ((pFirstRangeBan->tTempBanExpire == pSecondRangeBan->tTempBanExpire) ? 0 : -1);
		}
	case 3:
		return _stricmp(pFirstRangeBan->sBy == NULL ? "" : pFirstRangeBan->sBy, pSecondRangeBan->sBy == NULL ? "" : pSecondRangeBan->sBy);
	default:
		return 0; // never happen, but we need to make compiler/complainer happy ;o)
	}
}
//------------------------------------------------------------------------------

void clsRangeBansDialog::OnColumnClick(const LPNMLISTVIEW &pListView)
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
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_RANGE_BANS], bSortAscending, iSortColumn);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRangeBans);
}
//------------------------------------------------------------------------------

int CALLBACK clsRangeBansDialog::SortCompareRangeBans(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	int iResult = clsRangeBansDialog::mPtr->CompareRangeBans((void *)lParam1, (void *)lParam2);
	
	return (clsRangeBansDialog::mPtr->bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void clsRangeBansDialog::RemoveRangeBans()
{
	if (::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
	{
		return;
	}
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	int iSel = -1;
	
	while ((iSel = (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED)) != -1)
	{
		RangeBanItem *pRangeBan = reinterpret_cast<RangeBanItem *>(ListViewGetItem(m_hWndWindowItems[LV_RANGE_BANS], iSel));
		
		clsBanManager::mPtr->RemRange(pRangeBan, true);
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEITEM, iSel, 0);
	}
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void clsRangeBansDialog::FilterRangeBans()
{
	int iTextLength = ::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]);
	
	if (iTextLength == 0)
	{
		sFilterString.clear();
		
		AddAllRangeBans();
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
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEALLITEMS, 0, 0);
		
		time_t acc_time;
		time(&acc_time);
		
		RangeBanItem * curRangeBan = NULL,
		               * nextRangeBan = clsBanManager::mPtr->pRangeBanListS;
		               
		while (nextRangeBan != NULL)
		{
			curRangeBan = nextRangeBan;
			nextRangeBan = curRangeBan->pNext;
			
			if ((((curRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP) == true) && acc_time > curRangeBan->tTempBanExpire)
			{
				clsBanManager::mPtr->RemRange(curRangeBan);
				delete curRangeBan;
				
				continue;
			}
			
			if (FilterRangeBan(curRangeBan) == false)
			{
				AddRangeBan(curRangeBan);
			}
		}
		
		ListViewSelectFirstItem(m_hWndWindowItems[LV_RANGE_BANS]);
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	}
}
//------------------------------------------------------------------------------

bool clsRangeBansDialog::FilterRangeBan(const RangeBanItem * pRangeBan)
{
	switch (iFilterColumn)
	{
	case 0:
	{
		char sTxt[82];
		sprintf(sTxt, "%s - %s", pRangeBan->sIpFrom, pRangeBan->sIpTo);
		
		if (stristr2(sTxt, sFilterString.c_str()) != NULL)
		{
			return false;
		}
		break;
	}
	case 1:
		if (pRangeBan->sReason != NULL && stristr2(pRangeBan->sReason, sFilterString.c_str()) != NULL)
		{
			return false;
		}
		break;
	case 2:
		if ((pRangeBan->ui8Bits & clsBanManager::TEMP) == clsBanManager::TEMP)
		{
			char msg[256];
			struct tm * tm = localtime(&pRangeBan->tTempBanExpire);
			strftime(msg, 256, "%c", tm);
			
			if (stristr2(msg, sFilterString.c_str()) != NULL)
			{
				return false;
			}
		}
		break;
	case 3:
		if (pRangeBan->sBy != NULL && stristr2(pRangeBan->sBy, sFilterString.c_str()) != NULL)
		{
			return false;
		}
		break;
	}
	
	return true;
}
//------------------------------------------------------------------------------

void clsRangeBansDialog::RemoveRangeBan(const RangeBanItem * pRangeBan)
{
	int iPos = ListViewGetItemPosition(m_hWndWindowItems[LV_RANGE_BANS], (void *)pRangeBan);
	
	if (iPos != -1)
	{
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEITEM, iPos, 0);
	}
}
//------------------------------------------------------------------------------

void clsRangeBansDialog::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if (hWindow != m_hWndWindowItems[LV_RANGE_BANS])
	{
		return;
	}
	
	UINT UISelectedCount = (UINT)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETSELECTEDCOUNT, 0, 0);
	
	if (UISelectedCount == 0)
	{
		return;
	}
	
	HMENU hMenu = ::CreatePopupMenu();
	
	if (UISelectedCount == 1)
	{
		::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_RANGE_BAN, clsLanguageManager::mPtr->sTexts[LAN_CHANGE]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	}
	
	::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_RANGE_BANS, clsLanguageManager::mPtr->sTexts[LAN_REMOVE]);
	
	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);
	
	ListViewGetMenuPos(m_hWndWindowItems[LV_RANGE_BANS], iX, iY);
	
	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWndWindowItems[WINDOW_HANDLE], NULL);
	
	::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsRangeBansDialog::ChangeRangeBan()
{
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	RangeBanItem * pRangeBan = reinterpret_cast<RangeBanItem *>(ListViewGetItem(m_hWndWindowItems[LV_RANGE_BANS], iSel));
	
	clsRangeBanDialog * pRangeBanDialog = new (std::nothrow) clsRangeBanDialog();
	
	if (pRangeBanDialog != NULL)
	{
		pRangeBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pRangeBan);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
