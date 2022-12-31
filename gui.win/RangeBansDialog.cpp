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
RangeBansDialog * RangeBansDialog::m_Ptr = nullptr;
//---------------------------------------------------------------------------
#define IDC_CHANGE_RANGE_BAN      1000
#define IDC_REMOVE_RANGE_BANS     1001
//---------------------------------------------------------------------------
static ATOM atomRangeBansDialog = 0;
//---------------------------------------------------------------------------

RangeBansDialog::RangeBansDialog() : m_iFilterColumn(0), m_iSortColumn(0), m_bSortAscending(true)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

RangeBansDialog::~RangeBansDialog()
{
	RangeBansDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RangeBansDialog::StaticRangeBansDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RangeBansDialog * pRangeBansDialog = (RangeBansDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if(pRangeBansDialog == nullptr)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pRangeBansDialog->RangeBansDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RangeBansDialog::RangeBansDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS], nullptr, (rcParent.right / 2) + 1, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2,
		               rcParent.right - (rcParent.right / 2) - 3, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS], nullptr, 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (rcParent.right / 2) - 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[CB_FILTER], nullptr, (rcParent.right / 2) + 3, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin,
		               rcParent.right - (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[EDT_FILTER], nullptr, 11, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[GB_FILTER], nullptr, 3, rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_RANGE_BANS], nullptr, 0, 0, rcParent.right - 6, rcParent.bottom - GuiSettingManager::m_iOneLineGB - (2 * GuiSettingManager::m_iEditHeight) - 14, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_ADD_RANGE_BAN], nullptr, 0, 0, rcParent.right - 4, GuiSettingManager::m_iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case (BTN_ADD_RANGE_BAN+100):
		{
		
			RangeBanDialog * pRangeBanDialog = new (std::nothrow) RangeBanDialog();
			
			if(pRangeBanDialog != nullptr)
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
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				if(::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]) != 0)
				{
					FilterRangeBans();
				}
			}
			
			break;
		case BTN_CLEAR_RANGE_TEMP_BANS:
			if(::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			BanManager::m_Ptr->ClearTempRange();
			AddAllRangeBans();
			
			return 0;
		case BTN_CLEAR_RANGE_PERM_BANS:
			if(::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			BanManager::m_Ptr->ClearPermRange();
			AddAllRangeBans();
			
			return 0;
		case IDOK:   // NM_RETURN
		{
			HWND hWndFocus = ::GetFocus();
			
			if(hWndFocus == m_hWndWindowItems[LV_RANGE_BANS])
			{
				ChangeRangeBan();
				return 0;
			}
			else if(hWndFocus == m_hWndWindowItems[EDT_FILTER])
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
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_RANGE_BANS])
		{
			if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK)
			{
				OnColumnClick((LPNMLISTVIEW)lParam);
			}
			else if(((LPNMHDR)lParam)->code == NM_DBLCLK)
			{
				if(((LPNMITEMACTIVATE)lParam)->iItem == -1)
				{
					break;
				}
				
				RangeBanItem * pRangeBan = reinterpret_cast<RangeBanItem *>(ListViewGetItem(m_hWndWindowItems[LV_RANGE_BANS], ((LPNMITEMACTIVATE)lParam)->iItem));
				
				RangeBanDialog * pRangeBanDialog = new (std::nothrow) RangeBanDialog();
				
				if(pRangeBanDialog != nullptr)
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
		mminfo->ptMinTrackSize.x = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_RANGE_BANS_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_RANGE_BANS_WINDOW_HEIGHT));
		
		return 0;
	}
	case WM_CLOSE:
	{
		RECT rcRangeBans;
		::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcRangeBans);
		
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_RANGE_BANS_WINDOW_WIDTH, rcRangeBans.right - rcRangeBans.left);
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_RANGE_BANS_WINDOW_HEIGHT, rcRangeBans.bottom - rcRangeBans.top);
		
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_RANGE_BANS_RANGE, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 0, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_RANGE_BANS_REASON, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 1, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_RANGE_BANS_EXPIRE, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 2, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_RANGE_BANS_BY, (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 3, 0));
		
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		ServerManager::m_hWndActiveDialog = nullptr;
		
		break;
	}
	case WM_NCDESTROY:
	{
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_SETFOCUS:
		if((UINT)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETSELECTEDCOUNT, 0, 0) != 0)
		{
			::SetFocus(m_hWndWindowItems[LV_RANGE_BANS]);
		}
		else
		{
			::SetFocus(m_hWndWindowItems[EDT_FILTER]);
		}
		
		return 0;
	case WM_ACTIVATE:
		if(LOWORD(wParam) != WA_INACTIVE)
		{
			ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
		}
		
		break;
		
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RangeBansDialog::DoModal(HWND hWndParent)
{
	if(atomRangeBansDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_RangeBansDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomRangeBansDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_WIDTH) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_HEIGHT) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRangeBansDialog), LanguageManager::m_Ptr->m_sTexts[LAN_RANGE_BANS],
	                                                    WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                                                    iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_HEIGHT),
	                                                    hWndParent, nullptr, ServerManager::m_hInstance, nullptr);
	                                                    
	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr)
	{
		return;
	}
	
	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRangeBansDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[BTN_ADD_RANGE_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ADD_NEW_RANGE_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                        2, 2, rcParent.right - 4, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_RANGE_BAN+100), ServerManager::m_hInstance, nullptr);
	                                                        
	m_hWndWindowItems[LV_RANGE_BANS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
	                                                    3, GuiSettingManager::m_iEditHeight + 6, rcParent.right - 6, rcParent.bottom - GuiSettingManager::m_iOneLineGB - (2 * GuiSettingManager::m_iEditHeight) - 14, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	
	m_hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_FILTER_RANGE_BANS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                
	m_hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);
	
	m_hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                (rcParent.right / 2) + 3, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight,
	                                                m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER,
	                                                ServerManager::m_hInstance, nullptr);
	                                                
	m_hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_TEMP_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                                2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (rcParent.right / 2) - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_RANGE_TEMP_BANS, ServerManager::m_hInstance, nullptr);
	                                                                
	m_hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_PERM_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                                (rcParent.right / 2) + 1, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, rcParent.right - (rcParent.right / 2) - 3, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE],
	                                                                (HMENU)BTN_CLEAR_RANGE_PERM_BANS, ServerManager::m_hInstance, nullptr);
	                                                                
	for(uint8_t ui8i = 1; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if(m_hWndWindowItems[ui8i] == nullptr)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}
	
	RECT rcRangeBans;
	::GetClientRect(m_hWndWindowItems[LV_RANGE_BANS], &rcRangeBans);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	
	const int iRangeBansStrings[] = { LAN_RANGE, LAN_REASON, LAN_EXPIRE, LAN_BANNED_BY };
	const int iRangeBansWidths[] = { GUISETINT_RANGE_BANS_RANGE, GUISETINT_RANGE_BANS_REASON, GUISETINT_RANGE_BANS_EXPIRE, GUISETINT_RANGE_BANS_BY };
	
	for(uint8_t ui8i = 0; ui8i < 4; ui8i++)
	{
		lvColumn.cx = GuiSettingManager::m_Ptr->m_i32Integers[iRangeBansWidths[ui8i]];
		lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[iRangeBansStrings[ui8i]];
		lvColumn.iSubItem = ui8i;
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_INSERTCOLUMN, ui8i, (LPARAM)&lvColumn);
		
		::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[iRangeBansStrings[ui8i]]);
	}
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_RANGE_BANS], m_bSortAscending, m_iSortColumn);
	
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);
	
	AddAllRangeBans();
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void RangeBansDialog::AddAllRangeBans()
{
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEALLITEMS, 0, 0);
	
	time_t acc_time;
	time(&acc_time);
	
	RangeBanItem * curRangeBan = nullptr,
	               * nextRangeBan = BanManager::m_Ptr->m_pRangeBanListS;
	               
	while(nextRangeBan != nullptr)
	{
		curRangeBan = nextRangeBan;
		nextRangeBan = curRangeBan->m_pNext;
		
		if((((curRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) && acc_time > curRangeBan->m_tTempBanExpire)
		{
			BanManager::m_Ptr->RemRange(curRangeBan);
			delete curRangeBan;
			
			continue;
		}
		
		AddRangeBan(curRangeBan);
	}
	
	ListViewSelectFirstItem(m_hWndWindowItems[LV_RANGE_BANS]);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RangeBansDialog::AddRangeBan(const RangeBanItem * pRangeBan)
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ListViewGetInsertPosition(m_hWndWindowItems[LV_RANGE_BANS], pRangeBan, m_bSortAscending, CompareRangeBans);
	
	string sTxt = string(pRangeBan->m_sIpFrom) + " - " + pRangeBan->m_sIpTo;
	if((pRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL)
	{
		sTxt += " (";
		sTxt += LanguageManager::m_Ptr->m_sTexts[LAN_FULL_BANNED];
		sTxt += ")";
	}
	lvItem.pszText = sTxt.c_str();
	lvItem.lParam = (LPARAM)pRangeBan;
	
	int i = (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	
	if(i == -1)
	{
		return;
	}
	
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = i;
	lvItem.iSubItem = 1;
	lvItem.pszText = (pRangeBan->m_sReason == nullptr ? "" : pRangeBan->m_sReason);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	
	if((pRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
	{
		char msg[256];
		struct tm * tm = localtime(&pRangeBan->m_tTempBanExpire);
		strftime(msg, 256, "%c", tm);
		
		lvItem.iSubItem = 2;
		lvItem.pszText = msg;
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
	
	lvItem.iSubItem = 3;
	lvItem.pszText = (pRangeBan->m_sBy == nullptr ? "" : pRangeBan->m_sBy);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

int RangeBansDialog::CompareRangeBans(const void * pItem, const void * pOtherItem)
{
	const RangeBanItem * pFirstRangeBan = reinterpret_cast<const RangeBanItem *>(pItem);
	const RangeBanItem * pSecondRangeBan = reinterpret_cast<const RangeBanItem *>(pOtherItem);
	
	switch(RangeBansDialog::m_Ptr->m_iSortColumn)
	{
	case 0:
		return (memcmp(pFirstRangeBan->m_ui128FromIpHash, pSecondRangeBan->m_ui128FromIpHash, 16) > 0) ? 1 :
		       ((memcmp(pFirstRangeBan->m_ui128FromIpHash, pSecondRangeBan->m_ui128FromIpHash, 16) == 0) ?
		        (memcmp(pFirstRangeBan->m_ui128ToIpHash, pSecondRangeBan->m_ui128ToIpHash, 16) > 0) ? 1 :
		        ((memcmp(pFirstRangeBan->m_ui128ToIpHash, pSecondRangeBan->m_ui128ToIpHash, 16) == 0) ? 0 : -1) : -1);
	case 1:
		return _stricmp(pFirstRangeBan->m_sReason == nullptr ? "" : pFirstRangeBan->m_sReason, pSecondRangeBan->m_sReason == nullptr ? "" : pSecondRangeBan->m_sReason);
	case 2:
		if((pFirstRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
		{
			if((pSecondRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else if((pSecondRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
		{
			return 1;
		}
		else
		{
			return (pFirstRangeBan->m_tTempBanExpire > pSecondRangeBan->m_tTempBanExpire) ? 1 : ((pFirstRangeBan->m_tTempBanExpire == pSecondRangeBan->m_tTempBanExpire) ? 0 : -1);
		}
	case 3:
		return _stricmp(pFirstRangeBan->m_sBy == nullptr ? "" : pFirstRangeBan->m_sBy, pSecondRangeBan->m_sBy == nullptr ? "" : pSecondRangeBan->m_sBy);
	default:
		return 0; // never happen, but we need to make compiler/complainer happy ;o)
	}
}
//------------------------------------------------------------------------------

void RangeBansDialog::OnColumnClick(const LPNMLISTVIEW pListView)
{
	if(pListView->iSubItem != m_iSortColumn)
	{
		m_bSortAscending = true;
		m_iSortColumn = pListView->iSubItem;
	}
	else
	{
		m_bSortAscending = !m_bSortAscending;
	}
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_RANGE_BANS], m_bSortAscending, m_iSortColumn);
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRangeBans);
}
//------------------------------------------------------------------------------

int CALLBACK RangeBansDialog::SortCompareRangeBans(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	int iResult = RangeBansDialog::m_Ptr->CompareRangeBans((void *)lParam1, (void *)lParam2);
	
	return (RangeBansDialog::m_Ptr->m_bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void RangeBansDialog::RemoveRangeBans()
{
	if(::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
	{
		return;
	}
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	RangeBanItem * pRangeBan = nullptr;
	int iSel = -1;
	
	while((iSel = (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED)) != -1)
	{
		pRangeBan = reinterpret_cast<RangeBanItem *>(ListViewGetItem(m_hWndWindowItems[LV_RANGE_BANS], iSel));
		
		BanManager::m_Ptr->RemRange(pRangeBan, true);
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEITEM, iSel, 0);
	}
	
	::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RangeBansDialog::FilterRangeBans()
{
	int iTextLength = ::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]);
	
	if(iTextLength == 0)
	{
		m_sFilterString.clear();
		
		AddAllRangeBans();
	}
	else
	{
		m_iFilterColumn = (int)::SendMessage(m_hWndWindowItems[CB_FILTER], CB_GETCURSEL, 0, 0);
		
		char buf[65];
		
		int iLen = ::GetWindowText(m_hWndWindowItems[EDT_FILTER], buf, 65);
		
		for(int i = 0; i < iLen; i++)
		{
			buf[i] = (char)tolower(buf[i]);
		}
		
		m_sFilterString = buf;
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEALLITEMS, 0, 0);
		
		time_t acc_time;
		time(&acc_time);
		
		RangeBanItem * curRangeBan = nullptr,
		               * nextRangeBan = BanManager::m_Ptr->m_pRangeBanListS;
		               
		while(nextRangeBan != nullptr)
		{
			curRangeBan = nextRangeBan;
			nextRangeBan = curRangeBan->m_pNext;
			
			if((((curRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) && acc_time > curRangeBan->m_tTempBanExpire)
			{
				BanManager::m_Ptr->RemRange(curRangeBan);
				delete curRangeBan;
				
				continue;
			}
			
			if(FilterRangeBan(curRangeBan) == false)
			{
				AddRangeBan(curRangeBan);
			}
		}
		
		ListViewSelectFirstItem(m_hWndWindowItems[LV_RANGE_BANS]);
		
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	}
}
//------------------------------------------------------------------------------

bool RangeBansDialog::FilterRangeBan(const RangeBanItem * pRangeBan)
{
	switch(m_iFilterColumn)
	{
	case 0:
	{
		char sTxt[82];
		snprintf(sTxt, 82, "%s - %s", pRangeBan->m_sIpFrom, pRangeBan->m_sIpTo);
		
		if(stristr2(sTxt, m_sFilterString.c_str()) != nullptr)
		{
			return false;
		}
		break;
	}
	case 1:
		if(pRangeBan->m_sReason != nullptr && stristr2(pRangeBan->m_sReason, m_sFilterString.c_str()) != nullptr)
		{
			return false;
		}
		break;
	case 2:
		if((pRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
		{
			char msg[256];
			struct tm * tm = localtime(&pRangeBan->m_tTempBanExpire);
			strftime(msg, 256, "%c", tm);
			
			if(stristr2(msg, m_sFilterString.c_str()) != nullptr)
			{
				return false;
			}
		}
		break;
	case 3:
		if(pRangeBan->m_sBy != nullptr && stristr2(pRangeBan->m_sBy, m_sFilterString.c_str()) != nullptr)
		{
			return false;
		}
		break;
	}
	
	return true;
}
//------------------------------------------------------------------------------

void RangeBansDialog::RemoveRangeBan(const RangeBanItem * pRangeBan)
{
	int iPos = ListViewGetItemPosition(m_hWndWindowItems[LV_RANGE_BANS], (void *)pRangeBan);
	
	if(iPos != -1)
	{
		::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_DELETEITEM, iPos, 0);
	}
}
//------------------------------------------------------------------------------

void RangeBansDialog::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if(hWindow != m_hWndWindowItems[LV_RANGE_BANS])
	{
		return;
	}
	
	UINT UISelectedCount = (UINT)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETSELECTEDCOUNT, 0, 0);
	
	if(UISelectedCount == 0)
	{
		return;
	}
	
	HMENU hMenu = ::CreatePopupMenu();
	
	if(UISelectedCount == 1)
	{
		::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_RANGE_BAN, LanguageManager::m_Ptr->m_sTexts[LAN_CHANGE]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
	}
	
	::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_RANGE_BANS, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE]);
	
	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);
	
	ListViewGetMenuPos(m_hWndWindowItems[LV_RANGE_BANS], iX, iY);
	
	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWndWindowItems[WINDOW_HANDLE], nullptr);
	
	::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RangeBansDialog::ChangeRangeBan()
{
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	
	if(iSel == -1)
	{
		return;
	}
	
	RangeBanItem * pRangeBan = reinterpret_cast<RangeBanItem *>(ListViewGetItem(m_hWndWindowItems[LV_RANGE_BANS], iSel));
	
	RangeBanDialog * pRangeBanDialog = new (std::nothrow) RangeBanDialog();
	
	if(pRangeBanDialog != nullptr)
	{
		pRangeBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pRangeBan);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
