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
#include "RegisteredUsersDialog.h"
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
#include "RegisteredUserDialog.h"
//---------------------------------------------------------------------------
clsRegisteredUsersDialog * clsRegisteredUsersDialog::mPtr = nullptr;
//---------------------------------------------------------------------------
#define IDC_CHANGE_REG      1000
#define IDC_REMOVE_REGS     1001
//---------------------------------------------------------------------------
static ATOM atomRegisteredUsersDialog = 0;
//---------------------------------------------------------------------------

clsRegisteredUsersDialog::clsRegisteredUsersDialog() : iFilterColumn(0), iSortColumn(0), bSortAscending(true)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

clsRegisteredUsersDialog::~clsRegisteredUsersDialog()
{
	clsRegisteredUsersDialog::mPtr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsRegisteredUsersDialog::StaticRegisteredUsersDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	clsRegisteredUsersDialog * pRegisteredUsersDialog = (clsRegisteredUsersDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pRegisteredUsersDialog == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pRegisteredUsersDialog->RegisteredUsersDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsRegisteredUsersDialog::RegisteredUsersDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		::SetWindowPos(m_hWndWindowItems[CB_FILTER], NULL, (rcParent.right / 2) + 3, (rcParent.bottom - clsGuiSettingManager::iOneLineGB - 3) + clsGuiSettingManager::iGroupBoxMargin,
		               rcParent.right - (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[EDT_FILTER], NULL, 11, (rcParent.bottom - clsGuiSettingManager::iOneLineGB - 3) + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[GB_FILTER], NULL, 3, rcParent.bottom - clsGuiSettingManager::iOneLineGB - 3, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_REGS], NULL, 0, 0, rcParent.right - 6, rcParent.bottom - clsGuiSettingManager::iOneLineGB - clsGuiSettingManager::iEditHeight - 11, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_ADD_REG], NULL, 0, 0, rcParent.right - 4, clsGuiSettingManager::iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case (BTN_ADD_REG+100):
		{
			clsRegisteredUserDialog::mPtr = new (std::nothrow) clsRegisteredUserDialog();
			
			if (clsRegisteredUserDialog::mPtr != NULL)
			{
				clsRegisteredUserDialog::mPtr->DoModal(m_hWndWindowItems[WINDOW_HANDLE]);
			}
			
			return 0;
		}
		case IDC_CHANGE_REG:
		{
			ChangeReg();
			return 0;
		}
		case IDC_REMOVE_REGS:
			RemoveRegs();
			return 0;
		case CB_FILTER:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]) != 0)
				{
					FilterRegs();
				}
			}
			
			break;
		case IDOK:   // NM_RETURN
		{
			HWND hWndFocus = ::GetFocus();
			
			if (hWndFocus == m_hWndWindowItems[LV_REGS])
			{
				ChangeReg();
				return 0;
			}
			else if (hWndFocus == m_hWndWindowItems[EDT_FILTER])
			{
				FilterRegs();
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
		if (((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_REGS])
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
				
				RegUser * pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS], ((LPNMITEMACTIVATE)lParam)->iItem));
				
				clsRegisteredUserDialog::mPtr = new (std::nothrow) clsRegisteredUserDialog();
				
				if (clsRegisteredUserDialog::mPtr != NULL)
				{
					clsRegisteredUserDialog::mPtr->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pReg);
				}
				
				return 0;
			}
		}
		
		break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_REGS_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_REGS_WINDOW_HEIGHT));
		
		return 0;
	}
	case WM_CLOSE:
	{
		RECT rcRegs;
		::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcRegs);
		
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_REGS_WINDOW_WIDTH, rcRegs.right - rcRegs.left);
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_REGS_WINDOW_HEIGHT, rcRegs.bottom - rcRegs.top);
		
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_REGS_NICK, (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETCOLUMNWIDTH, 0, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_REGS_PASSWORD, (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETCOLUMNWIDTH, 1, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_REGS_PROFILE, (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETCOLUMNWIDTH, 2, 0));
		
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
		if ((UINT)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETSELECTEDCOUNT, 0, 0) != 0)
		{
			::SetFocus(m_hWndWindowItems[LV_REGS]);
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

void clsRegisteredUsersDialog::DoModal(HWND hWndParent)
{
	if (atomRegisteredUsersDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RegisteredUsersDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomRegisteredUsersDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_WIDTH) / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_HEIGHT) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRegisteredUsersDialog), clsLanguageManager::mPtr->sTexts[LAN_REG_USERS],
	                                                    WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                                                    iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_REGS_WINDOW_HEIGHT),
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRegisteredUsersDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[BTN_ADD_REG] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ADD_NEW_REG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                  2, 2, (rcParent.right / 3) - 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_REG + 100), clsServerManager::hInstance, NULL);
	                                                  
	m_hWndWindowItems[LV_REGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
	                                              3, clsGuiSettingManager::iEditHeight + 6, rcParent.right - 6, rcParent.bottom - clsGuiSettingManager::iOneLineGB - clsGuiSettingManager::iEditHeight - 11, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	
	m_hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_FILTER_REGISTERED_USERS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, rcParent.bottom - clsGuiSettingManager::iOneLineGB - 3, rcParent.right - 6, clsGuiSettingManager::iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                                
	m_hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, (rcParent.bottom - clsGuiSettingManager::iOneLineGB - 3) + clsGuiSettingManager::iGroupBoxMargin, (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);
	
	m_hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                (rcParent.right / 2) + 3, (rcParent.bottom - clsGuiSettingManager::iOneLineGB - 3) + clsGuiSettingManager::iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, clsGuiSettingManager::iEditHeight,
	                                                m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER, clsServerManager::hInstance, NULL);
	                                                
	for (uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if (m_hWndWindowItems[ui8i] == NULL)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_NICK]);
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PASSWORD]);
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)clsLanguageManager::mPtr->sTexts[LAN_PROFILE]);
	
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);
	
	RECT rcRegs;
	::GetClientRect(m_hWndWindowItems[LV_REGS], &rcRegs);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = clsGuiSettingManager::mPtr->i32Integers[GUISETINT_REGS_NICK];
	lvColumn.pszText = clsLanguageManager::mPtr->sTexts[LAN_NICK];
	lvColumn.iSubItem = 0;
	
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	
	lvColumn.fmt = LVCFMT_RIGHT;
	lvColumn.cx = clsGuiSettingManager::mPtr->i32Integers[GUISETINT_REGS_PASSWORD];
	lvColumn.pszText = clsLanguageManager::mPtr->sTexts[LAN_PASSWORD];
	lvColumn.iSubItem = 1;
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
	
	lvColumn.cx = clsGuiSettingManager::mPtr->i32Integers[GUISETINT_REGS_PROFILE];
	lvColumn.pszText = clsLanguageManager::mPtr->sTexts[LAN_PROFILE];
	lvColumn.iSubItem = 2;
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 2, (LPARAM)&lvColumn);
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_REGS], bSortAscending, iSortColumn);
	
	AddAllRegs();
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::AddAllRegs()
{
	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEALLITEMS, 0, 0);
	
	RegUser * curReg = NULL,
	          * nextReg = clsRegManager::mPtr->pRegListS;
	          
	while (nextReg != NULL)
	{
		curReg = nextReg;
		nextReg = curReg->pNext;
		
		AddReg(curReg);
	}
	
	ListViewSelectFirstItem(m_hWndWindowItems[LV_REGS]);
	
	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::AddReg(const RegUser * pReg)
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ListViewGetInsertPosition(m_hWndWindowItems[LV_REGS], pReg, bSortAscending, CompareRegs);
	lvItem.pszText = pReg->sNick;
	lvItem.lParam = (LPARAM)pReg;
	
	int i = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	
	char sHexaHash[129];
	sHexaHash[0] = 0;
	
	if (i != -1)
	{
	
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = i;
		lvItem.iSubItem = 1;
		
		if (pReg->bPassHash == true)
		{
			memset(&sHexaHash, 0, 129);
			for (uint8_t ui8i = 0; ui8i < 64; ui8i++)
			{
				sprintf(sHexaHash + (ui8i * 2), "%02X", pReg->ui8PassHash[ui8i]);
			}
			lvItem.pszText = sHexaHash;
		}
		else
		{
			lvItem.pszText = pReg->sPass;
		}
		
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);
		
		lvItem.iSubItem = 2;
		lvItem.pszText = clsProfileManager::mPtr->ppProfilesTable[pReg->ui16Profile]->sName;
		
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
}
//------------------------------------------------------------------------------

int clsRegisteredUsersDialog::CompareRegs(const void * pItem, const void * pOtherItem)
{
	const RegUser * pFirstReg = reinterpret_cast<const RegUser *>(pItem);
	const RegUser * pSecondReg = reinterpret_cast<const RegUser *>(pOtherItem);
	
	switch (clsRegisteredUsersDialog::mPtr->iSortColumn)
	{
	case 0:
		return _stricmp(pFirstReg->sNick, pSecondReg->sNick);
	case 1:
		return _stricmp(pFirstReg->sPass, pSecondReg->sPass);
	case 2:
		return (pFirstReg->ui16Profile > pSecondReg->ui16Profile) ? 1 : ((pFirstReg->ui16Profile == pSecondReg->ui16Profile) ? 0 : -1);
	default:
		return 0; // never happen, but we need to make compiler/complainer happy ;o)
	}
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::OnColumnClick(const LPNMLISTVIEW &pListView)
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
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_REGS], bSortAscending, iSortColumn);
	
	::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRegs);
}
//------------------------------------------------------------------------------

int CALLBACK clsRegisteredUsersDialog::SortCompareRegs(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	int iResult = clsRegisteredUsersDialog::mPtr->CompareRegs((void *)lParam1, (void *)lParam2);
	
	return (clsRegisteredUsersDialog::mPtr->bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::RemoveRegs()
{
	if (::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
	{
		return;
	}
	
	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	RegUser * pReg = nullptr;
	int iSel = -1;
	
	while ((iSel = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED)) != -1)
	{
		pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS], iSel));
		
		clsRegManager::mPtr->Delete(pReg, true);
		
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEITEM, iSel, 0);
	}
	
	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::FilterRegs()
{
	int iTextLength = ::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]);
	
	if (iTextLength == 0)
	{
		sFilterString.clear();
		
		AddAllRegs();
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
		
		::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);
		
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEALLITEMS, 0, 0);
		
		RegUser * curReg = NULL,
		          * nextReg = clsRegManager::mPtr->pRegListS;
		          
		while (nextReg != NULL)
		{
			curReg = nextReg;
			nextReg = curReg->pNext;
			
			switch (iFilterColumn)
			{
			case 0:
				if (stristr2(curReg->sNick, sFilterString.c_str()) == NULL)
				{
					continue;
				}
				break;
			case 1:
				if (stristr2(curReg->sPass, sFilterString.c_str()) == NULL)
				{
					continue;
				}
				break;
			case 2:
				if (stristr2(clsProfileManager::mPtr->ppProfilesTable[curReg->ui16Profile]->sName, sFilterString.c_str()) == NULL)
				{
					continue;
				}
				break;
			}
			
			AddReg(curReg);
		}
		
		ListViewSelectFirstItem(m_hWndWindowItems[LV_REGS]);
		
		::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	}
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::RemoveReg(const RegUser * pReg)
{
	int iPos = ListViewGetItemPosition(m_hWndWindowItems[LV_REGS], (void *)pReg);
	
	if (iPos != -1)
	{
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_DELETEITEM, iPos, 0);
	}
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::UpdateProfiles()
{
	int iItemCount = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETITEMCOUNT, 0, 0);
	
	if (iItemCount == 0)
	{
		return;
	}
	
	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	RegUser * pReg = nullptr;
	
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 2;
	
	for (int i = 0; i < iItemCount; i++)
	{
		pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS], i));
		
		lvItem.iItem = i;
		lvItem.pszText = clsProfileManager::mPtr->ppProfilesTable[pReg->ui16Profile]->sName;
		
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
	
	::SendMessage(m_hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	
	if (iSortColumn == 2)
	{
		::SendMessage(m_hWndWindowItems[LV_REGS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRegs);
	}
}
//------------------------------------------------------------------------------

void clsRegisteredUsersDialog::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if (hWindow != m_hWndWindowItems[LV_REGS])
	{
		return;
	}
	
	UINT UISelectedCount = (UINT)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETSELECTEDCOUNT, 0, 0);
	
	if (UISelectedCount == 0)
	{
		return;
	}
	
	HMENU hMenu = ::CreatePopupMenu();
	
	if (UISelectedCount == 1)
	{
		::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_REG, clsLanguageManager::mPtr->sTexts[LAN_CHANGE]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	}
	
	::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_REGS, clsLanguageManager::mPtr->sTexts[LAN_REMOVE]);
	
	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);
	
	ListViewGetMenuPos(m_hWndWindowItems[LV_REGS], iX, iY);
	
	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWndWindowItems[WINDOW_HANDLE], NULL);
	
	::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsRegisteredUsersDialog::ChangeReg()
{
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_REGS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	RegUser * pReg = reinterpret_cast<RegUser *>(ListViewGetItem(m_hWndWindowItems[LV_REGS], iSel));
	
	clsRegisteredUserDialog::mPtr = new (std::nothrow) clsRegisteredUserDialog();
	
	if (clsRegisteredUserDialog::mPtr != NULL)
	{
		clsRegisteredUserDialog::mPtr->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pReg);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
