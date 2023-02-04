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
BansDialog * BansDialog::m_Ptr = nullptr;
//---------------------------------------------------------------------------
#define IDC_CHANGE_BAN      1000
#define IDC_REMOVE_BANS     1001
//---------------------------------------------------------------------------
static ATOM atomBansDialog = 0;
//---------------------------------------------------------------------------

BansDialog::BansDialog() : m_iFilterColumn(0), m_iSortColumn(0), m_bSortAscending(true)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

BansDialog::~BansDialog()
{
	BansDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK BansDialog::StaticBansDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BansDialog * pBansDialog = (BansDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if(pBansDialog == nullptr)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pBansDialog->BansDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT BansDialog::BansDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_PERM_BANS], nullptr, (rcParent.right / 2) + 1, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2,
		               rcParent.right - (rcParent.right / 2) - 3, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_CLEAR_TEMP_BANS], nullptr, 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (rcParent.right / 2) - 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[CB_FILTER], nullptr, (rcParent.right / 2) + 3, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin,
		               rcParent.right - (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[EDT_FILTER], nullptr, 11, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[GB_FILTER], nullptr, 3, rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[LV_BANS], nullptr, 0, 0, rcParent.right - 6, rcParent.bottom - GuiSettingManager::m_iOneLineGB - (2 * GuiSettingManager::m_iEditHeight) - 14, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_ADD_BAN], nullptr, 0, 0, rcParent.right - 4, GuiSettingManager::m_iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case (BTN_ADD_BAN+100):
		{
			BanDialog * pBanDialog = new (std::nothrow) BanDialog();
			
			if(pBanDialog != nullptr)
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
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				if(::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]) != 0)
				{
					FilterBans();
				}
			}
			
			break;
		case BTN_CLEAR_TEMP_BANS:
			if(::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			BanManager::m_Ptr->ClearTemp();
			AddAllBans();
			
			return 0;
		case BTN_CLEAR_PERM_BANS:
			if(::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
			{
				return 0;
			}
			
			BanManager::m_Ptr->ClearPerm();
			AddAllBans();
			
			return 0;
		case IDOK:   // NM_RETURN
		{
			HWND hWndFocus = ::GetFocus();
			
			if(hWndFocus == m_hWndWindowItems[LV_BANS])
			{
				ChangeBan();
				return 0;
			}
			else if(hWndFocus == m_hWndWindowItems[EDT_FILTER])
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
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[LV_BANS])
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
				
				BanItem * pBan = reinterpret_cast<BanItem *>(ListViewGetItem(m_hWndWindowItems[LV_BANS], ((LPNMITEMACTIVATE)lParam)->iItem));
				
				BanDialog * pBanDialog = new (std::nothrow) BanDialog();
				
				if(pBanDialog != nullptr)
				{
					pBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pBan);
				}
				
				return 0;
			}
			else if(((LPNMHDR)lParam)->code == NM_RETURN)
			{
				ChangeBan();
				return 0;
			}
		}
		
		break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_BANS_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_BANS_WINDOW_HEIGHT));
		
		return 0;
	}
	case WM_CLOSE:
	{
		RECT rcBans;
		::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcBans);
		
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_BANS_WINDOW_WIDTH, rcBans.right - rcBans.left);
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_BANS_WINDOW_HEIGHT, rcBans.bottom - rcBans.top);
		
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_BANS_NICK, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 0, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_BANS_IP, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 1, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_BANS_REASON, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 2, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_BANS_EXPIRE, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 3, 0));
		GuiSettingManager::m_Ptr->SetInteger(GUISETINT_BANS_BY, (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETCOLUMNWIDTH, 4, 0));
		
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
		if((UINT)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETSELECTEDCOUNT, 0, 0) != 0)
		{
			::SetFocus(m_hWndWindowItems[LV_BANS]);
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

void BansDialog::DoModal(HWND hWndParent)
{
	if(atomBansDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_BansDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomBansDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_WIDTH) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_HEIGHT) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomBansDialog), LanguageManager::m_Ptr->m_sTexts[LAN_BANS],
	                                                    WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
	                                                    iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_BANS_WINDOW_HEIGHT),
	                                                    hWndParent, nullptr, ServerManager::m_hInstance, nullptr);
	                                                    
	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr)
	{
		return;
	}
	
	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticBansDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[BTN_ADD_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ADD_NEW_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                  2, 2, (rcParent.right / 3) - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_BAN+100), ServerManager::m_hInstance, nullptr);
	                                                  
	m_hWndWindowItems[LV_BANS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
	                                              3, GuiSettingManager::m_iEditHeight + 6, rcParent.right - 6, rcParent.bottom - GuiSettingManager::m_iOneLineGB - (2 * GuiSettingManager::m_iEditHeight) - 14, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	
	m_hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_FILTER_BANS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                3, rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6, rcParent.right - 6, GuiSettingManager::m_iOneLineGB, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);
	                                                
	m_hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                 11, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin, (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);
	
	m_hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
	                                                (rcParent.right / 2) + 3, (rcParent.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iOneLineGB - 6) + GuiSettingManager::m_iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, GuiSettingManager::m_iEditHeight,
	                                                m_hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER, ServerManager::m_hInstance, nullptr);
	                                                
	m_hWndWindowItems[BTN_CLEAR_TEMP_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_TEMP_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                          2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (rcParent.right / 2) - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_TEMP_BANS, ServerManager::m_hInstance, nullptr);
	                                                          
	m_hWndWindowItems[BTN_CLEAR_PERM_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_PERM_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                          (rcParent.right / 2) + 1, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, rcParent.right - (rcParent.right / 2) - 3, GuiSettingManager::m_iEditHeight,
	                                                          m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_PERM_BANS, ServerManager::m_hInstance, nullptr);
	                                                          
	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if(m_hWndWindowItems[ui8i] == nullptr)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}
	
	RECT rcBans;
	::GetClientRect(m_hWndWindowItems[LV_BANS], &rcBans);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	
	const int iBansStrings[] = { LAN_NICK, LAN_IP, LAN_REASON, LAN_EXPIRE, LAN_BANNED_BY };
	const int iBansWidths[] = { GUISETINT_BANS_NICK, GUISETINT_BANS_IP, GUISETINT_BANS_REASON, GUISETINT_BANS_EXPIRE, GUISETINT_BANS_BY };
	
	for(uint8_t ui8i = 0; ui8i < 5; ui8i++)
	{
		lvColumn.cx = GuiSettingManager::m_Ptr->m_i32Integers[iBansWidths[ui8i]];
		lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[iBansStrings[ui8i]];
		lvColumn.iSubItem = ui8i;
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_INSERTCOLUMN, ui8i, (LPARAM)&lvColumn);
		
		::SendMessage(m_hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[iBansStrings[ui8i]]);
	}
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_BANS], m_bSortAscending, m_iSortColumn);
	
	::SendMessage(m_hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);
	
	AddAllBans();
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void BansDialog::AddAllBans()
{
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEALLITEMS, 0, 0);
	
	time_t acc_time;
	time(&acc_time);
	
	BanItem * curBan = nullptr,
	          * nextBan = BanManager::m_Ptr->m_pTempBanListS;
	          
	while(nextBan != nullptr)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;
		
		if(acc_time > curBan->m_tTempBanExpire)
		{
			BanManager::m_Ptr->Rem(curBan);
			delete curBan;
			
			continue;
		}
		
		AddBan(curBan);
	}
	
	nextBan = BanManager::m_Ptr->m_pPermBanListS;
	
	while(nextBan != nullptr)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;
		
		AddBan(curBan);
	}
	
	ListViewSelectFirstItem(m_hWndWindowItems[LV_BANS]);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void BansDialog::AddBan(const BanItem * pBan)
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ListViewGetInsertPosition(m_hWndWindowItems[LV_BANS], pBan, m_bSortAscending, CompareBans);
	
	std::string sTxt(pBan->m_sNick == nullptr ? "" : pBan->m_sNick);
	if((pBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK)
	{
		sTxt += " (";
		sTxt += LanguageManager::m_Ptr->m_sTexts[LAN_BANNED];
		sTxt += ")";
	}
	lvItem.pszText = (LPSTR)sTxt.c_str();
	lvItem.lParam = (LPARAM)pBan;
	
	int i = (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	
	if(i == -1)
	{
		return;
	}
	
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = i;
	lvItem.iSubItem = 1;
	
	sTxt = (pBan->m_sIp[0] == '\0' ? "" : pBan->m_sIp);
	if((pBan->m_ui8Bits & BanManager::IP) == BanManager::IP)
	{
		sTxt += " (";
		if((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL)
		{
			sTxt += LanguageManager::m_Ptr->m_sTexts[LAN_FULL_BANNED];
		}
		else
		{
			sTxt += LanguageManager::m_Ptr->m_sTexts[LAN_BANNED];
		}
		sTxt += ")";
	}
	lvItem.pszText = (LPSTR)sTxt.c_str();
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	
	lvItem.iSubItem = 2;
	lvItem.pszText = (pBan->m_sReason == nullptr ? "" : pBan->m_sReason);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	
	if((pBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
	{
		char msg[256];
		struct tm *tm = localtime(&pBan->m_tTempBanExpire);
		strftime(msg, 256, "%c", tm);
		
		lvItem.iSubItem = 3;
		lvItem.pszText = msg;
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
	
	lvItem.iSubItem = 4;
	lvItem.pszText = (pBan->m_sBy == nullptr ? "" : pBan->m_sBy);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

int BansDialog::CompareBans(const void * pItem, const void * pOtherItem)
{
	const BanItem * pFirstBan = reinterpret_cast<const BanItem *>(pItem);
	const BanItem * pSecondBan = reinterpret_cast<const BanItem *>(pOtherItem);
	
	switch(BansDialog::m_Ptr->m_iSortColumn)
	{
	case 0:
		return _stricmp(pFirstBan->m_sNick == nullptr ? "" : pFirstBan->m_sNick, pSecondBan->m_sNick == nullptr ? "" : pSecondBan->m_sNick);
	case 1:
		return memcmp(pFirstBan->m_ui128IpHash, pSecondBan->m_ui128IpHash, 16);
	case 2:
		return _stricmp(pFirstBan->m_sReason == nullptr ? "" : pFirstBan->m_sReason, pSecondBan->m_sReason == nullptr ? "" : pSecondBan->m_sReason);
	case 3:
		if((pFirstBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
		{
			if((pSecondBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else if((pSecondBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
		{
			return 1;
		}
		else
		{
			return (pFirstBan->m_tTempBanExpire > pSecondBan->m_tTempBanExpire) ? 1 : ((pFirstBan->m_tTempBanExpire == pSecondBan->m_tTempBanExpire) ? 0 : -1);
		}
	case 4:
		return _stricmp(pFirstBan->m_sBy == nullptr ? "" : pFirstBan->m_sBy, pSecondBan->m_sBy == nullptr ? "" : pSecondBan->m_sBy);
	default:
		return 0; // never happen, but we need to make compiler/complainer happy ;o)
	}
}
//------------------------------------------------------------------------------

void BansDialog::OnColumnClick(const LPNMLISTVIEW pListView)
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
	
	ListViewUpdateArrow(m_hWndWindowItems[LV_BANS], m_bSortAscending, m_iSortColumn);
	
	::SendMessage(m_hWndWindowItems[LV_BANS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareBans);
}
//------------------------------------------------------------------------------

int CALLBACK BansDialog::SortCompareBans(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	int iResult = BansDialog::m_Ptr->CompareBans((void *)lParam1, (void *)lParam2);
	
	return (BansDialog::m_Ptr->m_bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void BansDialog::RemoveBans()
{
	if(::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
	{
		return;
	}
	
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	BanItem * pBan = nullptr;
	int iSel = -1;
	
	while((iSel = (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED)) != -1)
	{
		pBan = reinterpret_cast<BanItem *>(ListViewGetItem(m_hWndWindowItems[LV_BANS], iSel));
		
		BanManager::m_Ptr->Rem(pBan, true);
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEITEM, iSel, 0);
	}
	
	::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void BansDialog::FilterBans()
{
	int iTextLength = ::GetWindowTextLength(m_hWndWindowItems[EDT_FILTER]);
	
	if(iTextLength == 0)
	{
		m_sFilterString.clear();
		
		AddAllBans();
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
		
		::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);
		
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEALLITEMS, 0, 0);
		
		time_t acc_time;
		time(&acc_time);
		
		BanItem * curBan = nullptr,
		          * nextBan = BanManager::m_Ptr->m_pTempBanListS;
		          
		while(nextBan != nullptr)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;
			
			if(acc_time > curBan->m_tTempBanExpire)
			{
				BanManager::m_Ptr->Rem(curBan);
				delete curBan;
				
				continue;
			}
			
			if(FilterBan(curBan) == false)
			{
				AddBan(curBan);
			}
		}
		
		nextBan = BanManager::m_Ptr->m_pPermBanListS;
		
		while(nextBan != nullptr)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;
			
			if(FilterBan(curBan) == false)
			{
				AddBan(curBan);
			}
		}
		
		ListViewSelectFirstItem(m_hWndWindowItems[LV_BANS]);
		
		::SendMessage(m_hWndWindowItems[LV_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	}
}
//------------------------------------------------------------------------------

bool BansDialog::FilterBan(const BanItem * pBan)
{
	switch(m_iFilterColumn)
	{
	case 0:
		if(pBan->m_sNick != nullptr && stristr2(pBan->m_sNick, m_sFilterString.c_str()) != nullptr)
		{
			return false;
		}
		break;
	case 1:
		if(pBan->m_sIp[0] != '\0' && stristr2(pBan->m_sIp, m_sFilterString.c_str()) != nullptr)
		{
			return false;
		}
		break;
	case 2:
		if(pBan->m_sReason != nullptr && stristr2(pBan->m_sReason, m_sFilterString.c_str()) != nullptr)
		{
			return false;
		}
		break;
	case 3:
		if((pBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)
		{
			char msg[256];
			struct tm *tm = localtime(&pBan->m_tTempBanExpire);
			strftime(msg, 256, "%c", tm);
			
			if(stristr2(msg, m_sFilterString.c_str()) != nullptr)
			{
				return false;
			}
		}
		break;
	case 4:
		if(pBan->m_sBy != nullptr && stristr2(pBan->m_sBy, m_sFilterString.c_str()) != nullptr)
		{
			return false;
		}
		break;
	}
	
	return true;
}
//------------------------------------------------------------------------------

void BansDialog::RemoveBan(const BanItem * pBan)
{
	int iPos = ListViewGetItemPosition(m_hWndWindowItems[LV_BANS], (void *)pBan);
	
	if(iPos != -1)
	{
		::SendMessage(m_hWndWindowItems[LV_BANS], LVM_DELETEITEM, iPos, 0);
	}
}
//------------------------------------------------------------------------------

void BansDialog::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if(hWindow != m_hWndWindowItems[LV_BANS])
	{
		return;
	}
	
	UINT UISelectedCount = (UINT)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETSELECTEDCOUNT, 0, 0);
	
	if(UISelectedCount == 0)
	{
		return;
	}
	
	HMENU hMenu = ::CreatePopupMenu();
	
	if(UISelectedCount == 1)
	{
		::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_BAN, LanguageManager::m_Ptr->m_sTexts[LAN_CHANGE]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
	}
	
	::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_BANS, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE]);
	
	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);
	
	ListViewGetMenuPos(m_hWndWindowItems[LV_BANS], iX, iY);
	
	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWndWindowItems[WINDOW_HANDLE], nullptr);
	
	::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void BansDialog::ChangeBan()
{
	int iSel = (int)::SendMessage(m_hWndWindowItems[LV_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	
	if(iSel == -1)
	{
		return;
	}
	
	BanItem * pBan = reinterpret_cast<BanItem *>(ListViewGetItem(m_hWndWindowItems[LV_BANS], iSel));
	
	BanDialog * pBanDialog = new (std::nothrow) BanDialog();
	
	if(pBanDialog != nullptr)
	{
		pBanDialog->DoModal(m_hWndWindowItems[WINDOW_HANDLE], pBan);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
