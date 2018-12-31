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
#include "MainWindowPageUsersChat.h"
//---------------------------------------------------------------------------
#include "../core/colUsers.h"
#include "../core/GlobalDataQueue.h"
#include "../core/hashBanManager.h"
#include "../core/hashRegManager.h"
#include "../core/hashUsrManager.h"
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/UdpDebug.h"
#include "../core/User.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "LineDialog.h"
#include "MainWindow.h"
#include "RegisteredUserDialog.h"
#include "Resources.h"
//---------------------------------------------------------------------------
clsMainWindowPageUsersChat * clsMainWindowPageUsersChat::mPtr = nullptr;
//---------------------------------------------------------------------------
static WNDPROC wpOldMultiEditProc = nullptr;
//---------------------------------------------------------------------------

static LRESULT CALLBACK MultiRichEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return 0;
	}
	else if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)
	{
		return 0;
	}
	
	return ::CallWindowProc(clsGuiSettingManager::wpOldMultiRichEditProc, hWnd, uMsg, wParam, lParam);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsMainWindowPageUsersChat::clsMainWindowPageUsersChat()
{
	clsMainWindowPageUsersChat::mPtr = this;
	
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
	
	iPercentagePos = clsGuiSettingManager::mPtr->i32Integers[GUISETINT_USERS_CHAT_SPLITTER];
}
//---------------------------------------------------------------------------

clsMainWindowPageUsersChat::~clsMainWindowPageUsersChat()
{
	clsMainWindowPageUsersChat::mPtr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT clsMainWindowPageUsersChat::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		::SetFocus(hWndPageItems[BTN_SHOW_CHAT]);
		
		return 0;
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcMain = { 0, clsGuiSettingManager::iCheckHeight, ((WINDOWPOS*)lParam)->cx, ((WINDOWPOS*)lParam)->cy };
		
		::SetWindowPos(hWndPageItems[BTN_SHOW_CHAT], NULL, 0, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, clsGuiSettingManager::iCheckHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[BTN_SHOW_COMMANDS], NULL, ((rcMain.right - ScaleGui(150)) / 2) + 1, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, clsGuiSettingManager::iCheckHeight, SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], NULL, rcMain.right - (ScaleGui(150) - 4), 0, (rcMain.right - (rcMain.right - (ScaleGui(150) - 2))) - 4, clsGuiSettingManager::iCheckHeight,
		               SWP_NOZORDER);
		               
		SetSplitterRect(&rcMain);
		
		return 0;
	}
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == hWndPageItems[REDT_CHAT] && ((LPNMHDR)lParam)->code == EN_LINK)
		{
			if (((ENLINK *)lParam)->msg == WM_LBUTTONUP)
			{
				RichEditOpenLink(hWndPageItems[REDT_CHAT], (ENLINK *)lParam);
			}
		}
		else if (((LPNMHDR)lParam)->hwndFrom == hWndPageItems[LV_USERS] && ((LPNMHDR)lParam)->code == LVN_GETINFOTIP)
		{
			NMLVGETINFOTIP * pGetInfoTip = (LPNMLVGETINFOTIP)lParam;
			
			char msg[1024];
			
			LVITEM lvItem = { 0 };
			lvItem.mask = LVIF_PARAM | LVIF_TEXT;
			lvItem.iItem = pGetInfoTip->iItem;
			lvItem.pszText = msg;
			lvItem.cchTextMax = 1024;
			
			if ((BOOL)::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE)
			{
				return 0;
			}
			
			User * curUser = reinterpret_cast<User *>(lvItem.lParam);
			
			if (::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_UNCHECKED)
			{
				User * testUser = clsHashManager::mPtr->FindUser(lvItem.pszText, strlen(lvItem.pszText));
				
				if (testUser == NULL || testUser != curUser)
				{
					return 0;
				}
			}
			
			string sInfoTip = string(clsLanguageManager::mPtr->sTexts[LAN_NICK], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_NICK]) + ": " + string(curUser->sNick, curUser->ui8NickLen) +
			                  "\n" + string(clsLanguageManager::mPtr->sTexts[LAN_IP], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_IP]) + ": " + string(curUser->sIP);
			                  
			sInfoTip += "\n\n" + string(clsLanguageManager::mPtr->sTexts[LAN_CLIENT], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CLIENT]) + ": " +
			            string(curUser->sClient, (size_t)curUser->ui8ClientLen) +
			            "\n" + string(clsLanguageManager::mPtr->sTexts[LAN_VERSION], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_VERSION]) + ": " + string(curUser->sTagVersion, (size_t)curUser->ui8TagVersionLen);
			            
			sInfoTip += "\n\n" + string(clsLanguageManager::mPtr->sTexts[LAN_MODE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_MODE]) + ": " + string(curUser->sModes) +
			            "\n" + string(clsLanguageManager::mPtr->sTexts[LAN_SLOTS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SLOTS]) + ": " + string(curUser->Slots) +
			            "\n" + string(clsLanguageManager::mPtr->sTexts[LAN_HUBS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_HUBS]) + ": " + string(curUser->Hubs);
			            
			if (curUser->OLimit != 0)
			{
				sInfoTip += "\n" + string(clsLanguageManager::mPtr->sTexts[LAN_AUTO_OPEN_SLOT_WHEN_UP_UNDER], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_AUTO_OPEN_SLOT_WHEN_UP_UNDER]) + " " +
				            string(curUser->OLimit) + " kB/s";
			}
			
			if (curUser->DLimit != 0)
			{
				sInfoTip += "\n" + string(clsLanguageManager::mPtr->sTexts[LAN_LIMITER], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_LIMITER]) + " D:" + string(curUser->DLimit) + " kB/s";
			}
			
			if (curUser->LLimit != 0)
			{
				sInfoTip += "\n" + string(clsLanguageManager::mPtr->sTexts[LAN_LIMITER], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_LIMITER]) + " L:" + string(curUser->LLimit) + " kB/s";
			}
			
			sInfoTip += "\n\nRecvBuf: " + string(curUser->ui32RecvBufLen) + " bytes";
			sInfoTip += "\nSendBuf: " + string(curUser->ui32SendBufLen) + " bytes";
			
			pGetInfoTip->cchTextMax = (int)(sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size());
			memcpy(pGetInfoTip->pszText, sInfoTip.c_str(), sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size());
			pGetInfoTip->pszText[(sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size()) - 1] = '\0';
			
			return 0;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case BTN_AUTO_UPDATE_USERLIST:
			if (HIWORD(wParam) == BN_CLICKED && clsServerManager::bServerRunning == true)
			{
				bool bChecked = ::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
				::EnableWindow(hWndPageItems[BTN_UPDATE_USERS], bChecked == true ? FALSE : TRUE);
				if (bChecked == true)
				{
					UpdateUserList();
				}
				
				return 0;
			}
			
			break;
		case BTN_UPDATE_USERS:
			UpdateUserList();
			return 0;
		case EDT_CHAT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				int iLen = ::GetWindowTextLength((HWND)lParam);
				
				char * buf = (char *)malloc(iLen + 1);
				
				if (buf == NULL)
				{
					AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in clsMainWindowPageUsersChat::MainWindowPageProc\n", iLen + 1);
					return 0;
				}
				
				::GetWindowText((HWND)lParam, buf, iLen + 1);
				
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
				
				free(buf);
				
				return 0;
			}
			
			break;
		case IDC_REG_USER:
		{
			int iSel = (int)::SendMessage(hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
			
			if (iSel == -1)
			{
				return 0;
			}
			
			char sNick[65];
			
			LVITEM lvItem = { 0 };
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = iSel;
			lvItem.iSubItem = 0;
			lvItem.pszText = sNick;
			lvItem.cchTextMax = 65;
			
			::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem);
			
			clsRegisteredUserDialog::mPtr = new (std::nothrow) clsRegisteredUserDialog();
			
			if (clsRegisteredUserDialog::mPtr != NULL)
			{
				clsRegisteredUserDialog::mPtr->DoModal(clsMainWindow::mPtr->m_hWnd, NULL, sNick);
			}
			
			return 0;
		}
		case IDC_DISCONNECT_USER:
			DisconnectUser();
			return 0;
		case IDC_KICK_USER:
			KickUser();
			return 0;
		case IDC_BAN_USER:
			BanUser();
			return 0;
		case IDC_REDIRECT_USER:
			RedirectUser();
			return 0;
		}
		
		if (RichEditCheckMenuCommands(hWndPageItems[REDT_CHAT], LOWORD(wParam)) == true)
		{
			return 0;
		}
		
		break;
	case WM_CONTEXTMENU:
		OnContextMenu((HWND)wParam, lParam);
		break;
	case WM_DESTROY:
		clsGuiSettingManager::mPtr->SetBool(GUISETBOOL_SHOW_CHAT, ::SendMessage(hWndPageItems[BTN_SHOW_CHAT], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
		clsGuiSettingManager::mPtr->SetBool(GUISETBOOL_SHOW_COMMANDS, ::SendMessage(hWndPageItems[BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
		clsGuiSettingManager::mPtr->SetBool(GUISETBOOL_AUTO_UPDATE_USERLIST, ::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
		
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_USERS_CHAT_SPLITTER, iPercentagePos);
		
		break;
	}
	
	if (BasicSplitterProc(uMsg, wParam, lParam) == true)
	{
		return 0;
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

static LRESULT CALLBACK MultiEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_RETURN && (::GetKeyState(VK_CONTROL) & 0x8000) == 0)
		{
			clsMainWindowPageUsersChat * pParent = (clsMainWindowPageUsersChat *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (pParent != NULL)
			{
				if (pParent->OnEditEnter() == true)
				{
					return 0;
				}
			}
		}
		else if (wParam == '|')
		{
			return 0;
		}
	}
	else if (uMsg == WM_CHAR || uMsg == WM_KEYUP)
	{
		if (wParam == VK_RETURN && (::GetKeyState(VK_CONTROL) & 0x8000) == 0)
		{
			return 0;
		}
		else if (wParam == '|')
		{
			return 0;
		}
	}
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return 0;
	}
	
	return ::CallWindowProc(wpOldMultiEditProc, hWnd, uMsg, wParam, lParam);
}

//------------------------------------------------------------------------------

static LRESULT CALLBACK ListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			clsMainWindowPageUsersChat * pParent = (clsMainWindowPageUsersChat *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			
			if (pParent != NULL && ::IsWindowEnabled(pParent->hWndPageItems[clsMainWindowPageUsersChat::BTN_UPDATE_USERS]))
			{
				::SetFocus(pParent->hWndPageItems[clsMainWindowPageUsersChat::BTN_UPDATE_USERS]);
			}
			else
			{
				::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
			}
			
			return 0;
		}
		else
		{
			::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, TRUE));
			return 0;
		}
	}
	
	return ::CallWindowProc(clsGuiSettingManager::wpOldListViewProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

bool clsMainWindowPageUsersChat::CreateMainWindowPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	RECT rcMain;
	::GetClientRect(m_hWnd, &rcMain);
	
	hWndPageItems[BTN_SHOW_CHAT] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CHAT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                2, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                
	hWndPageItems[BTN_SHOW_COMMANDS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_CMDS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                    ((rcMain.right - ScaleGui(150)) / 2) + 1, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[REDT_CHAT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
	                                            2, clsGuiSettingManager::iCheckHeight, rcMain.right - ScaleGui(150), rcMain.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight - 4, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[REDT_CHAT], EM_EXLIMITTEXT, 0, (LPARAM)262144);
	::SendMessage(hWndPageItems[REDT_CHAT], EM_AUTOURLDETECT, TRUE, 0);
	::SendMessage(hWndPageItems[REDT_CHAT], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(hWndPageItems[REDT_CHAT], EM_GETEVENTMASK, 0, 0) | ENM_LINK);
	
	hWndPageItems[EDT_CHAT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL | ES_MULTILINE,
	                                           2, rcMain.bottom - clsGuiSettingManager::iEditHeight - 2, rcMain.right - ScaleGui(150), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_CHAT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_CHAT], EM_SETLIMITTEXT, 8192, 0);
	
	hWndPageItems[BTN_AUTO_UPDATE_USERLIST] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_AUTO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                           rcMain.right - ScaleGui(150) - 4, 0, (rcMain.right - (rcMain.right - ScaleGui(150) - 2)) - 4, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_AUTO_UPDATE_USERLIST, clsServerManager::hInstance, NULL);
	                                                           
	hWndPageItems[LV_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL |
	                                           LVS_SORTASCENDING, rcMain.right - ScaleGui(150) - 4, clsGuiSettingManager::iCheckHeight, (rcMain.right - (rcMain.right - ScaleGui(150) - 2)) - 4, rcMain.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight - 4,
	                                           m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[LV_USERS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	
	hWndPageItems[BTN_UPDATE_USERS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REFRESH_USERLIST], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                                   rcMain.right - ScaleGui(150) - 3, rcMain.bottom - clsGuiSettingManager::iEditHeight - 2, (rcMain.right - (rcMain.right - ScaleGui(150) - 2)) - 2, clsGuiSettingManager::iEditHeight,
	                                                   m_hWnd, (HMENU)BTN_UPDATE_USERS, clsServerManager::hInstance, NULL);
	                                                   
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	rcMain.top = clsGuiSettingManager::iCheckHeight;
	SetSplitterRect(&rcMain);
	
	RECT rcUsers;
	::GetClientRect(hWndPageItems[LV_USERS], &rcUsers);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = (rcUsers.right - rcUsers.left) - 5;
	
	::SendMessage(hWndPageItems[LV_USERS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	
	::SendMessage(hWndPageItems[BTN_SHOW_CHAT], BM_SETCHECK, clsGuiSettingManager::mPtr->bBools[GUISETBOOL_SHOW_CHAT] == true ? BST_CHECKED : BST_UNCHECKED, 0);
	::SendMessage(hWndPageItems[BTN_SHOW_COMMANDS], BM_SETCHECK, clsGuiSettingManager::mPtr->bBools[GUISETBOOL_SHOW_COMMANDS] == true ? BST_CHECKED : BST_UNCHECKED, 0);
	::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_SETCHECK, clsGuiSettingManager::mPtr->bBools[GUISETBOOL_AUTO_UPDATE_USERLIST] == true ? BST_CHECKED : BST_UNCHECKED, 0);
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_SHOW_CHAT], GWLP_WNDPROC, (LONG_PTR)FirstButtonProc);
	
	clsGuiSettingManager::wpOldMultiRichEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[REDT_CHAT], GWLP_WNDPROC, (LONG_PTR)MultiRichEditProc);
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_UPDATE_USERS], GWLP_WNDPROC, (LONG_PTR)LastButtonProc);
	
	::SetWindowLongPtr(hWndPageItems[EDT_CHAT], GWLP_USERDATA, (LONG_PTR)this);
	wpOldMultiEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_CHAT], GWLP_WNDPROC, (LONG_PTR)MultiEditProc);
	
	::SetWindowLongPtr(hWndPageItems[LV_USERS], GWLP_USERDATA, (LONG_PTR)this);
	clsGuiSettingManager::wpOldListViewProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[LV_USERS], GWLP_WNDPROC, (LONG_PTR)ListProc);
	
	return true;
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::UpdateLanguage()
{
	::SetWindowText(hWndPageItems[BTN_SHOW_CHAT], clsLanguageManager::mPtr->sTexts[LAN_CHAT]);
	::SetWindowText(hWndPageItems[BTN_SHOW_COMMANDS], clsLanguageManager::mPtr->sTexts[LAN_CMDS]);
	::SetWindowText(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], clsLanguageManager::mPtr->sTexts[LAN_AUTO]);
	::SetWindowText(hWndPageItems[BTN_UPDATE_USERS], clsLanguageManager::mPtr->sTexts[LAN_REFRESH_USERLIST]);
}
//---------------------------------------------------------------------------

char * clsMainWindowPageUsersChat::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_USERS_CHAT];
}
//------------------------------------------------------------------------------

bool clsMainWindowPageUsersChat::OnEditEnter()
{
	if (clsServerManager::bServerRunning == false)
	{
		return false;
	}
	
	int iAllocLen = ::GetWindowTextLength(hWndPageItems[EDT_CHAT]);
	
	if (iAllocLen == 0)
	{
		return false;
	}
	
	char * buf = (char *)malloc(iAllocLen + 4 + clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK]);
	
	if (buf == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in clsMainWindowPageUsersChat::OnEditEnter\n", iAllocLen + 4 + clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK]);
		return false;
	}
	
	buf[0] = '<';
	memcpy(buf + 1, clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK], clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK]);
	buf[clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK] + 1] = '>';
	buf[clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK] + 2] = ' ';
	::GetWindowText(hWndPageItems[EDT_CHAT], buf + clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK] + 3, iAllocLen + 1);
	buf[iAllocLen + 3 + clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK]] = '|';
	
	clsGlobalDataQueue::mPtr->AddQueueItem(buf, iAllocLen + 4 + clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK], NULL, 0, clsGlobalDataQueue::CMD_CHAT);
	
	buf[iAllocLen + 3 + clsSettingManager::mPtr->ui16TextsLens[SETTXT_ADMIN_NICK]] = '\0';
	
	RichEditAppendText(hWndPageItems[REDT_CHAT], buf);
	
	free(buf);
	
	::SetWindowText(hWndPageItems[EDT_CHAT], "");
	
	return true;
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::AddUser(const User * curUser)
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = 0;
	lvItem.lParam = (LPARAM)curUser;
	lvItem.pszText = curUser->sNick;
	
	::SendMessage(hWndPageItems[LV_USERS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::RemoveUser(const User * curUser)
{
	LVFINDINFO lvFindItem = { 0 };
	lvFindItem.flags = LVFI_PARAM;
	lvFindItem.lParam = (LPARAM)curUser;
	
	int iItem = (int)::SendMessage(hWndPageItems[LV_USERS], LVM_FINDITEM, (WPARAM) - 1, (LPARAM)&lvFindItem);
	
	if (iItem != -1)
	{
		::SendMessage(hWndPageItems[LV_USERS], LVM_DELETEITEM, iItem, 0);
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::UpdateUserList()
{
	::SendMessage(hWndPageItems[LV_USERS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	::SendMessage(hWndPageItems[LV_USERS], LVM_DELETEALLITEMS, 0, 0);
	
	uint32_t ui32InClose = 0, ui32InLogin = 0, ui32LoggedIn = 0, ui32Total = 0;
	
	User * pCur = NULL,
	       * pNext = clsUsers::mPtr->pListS;
	       
	while (pNext != NULL)
	{
		ui32Total++;
		
		pCur = pNext;
		pNext = pCur->pNext;
		
		switch (pCur->ui8State)
		{
		case User::STATE_ADDED:
			ui32LoggedIn++;
			
			AddUser(pCur);
			
			break;
		case User::STATE_CLOSING:
		case User::STATE_REMME:
			ui32InClose++;
			break;
		default:
			ui32InLogin++;
			break;
		}
	}
	
	ListViewSelectFirstItem(hWndPageItems[LV_USERS]);
	
	::SendMessage(hWndPageItems[LV_USERS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	
	RichEditAppendText(hWndPageItems[REDT_CHAT], (string(clsLanguageManager::mPtr->sTexts[LAN_TOTAL], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_TOTAL]) + ": " + string(ui32Total) + ", " +
	                                              string(clsLanguageManager::mPtr->sTexts[LAN_LOGGED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_LOGGED]) + ": " + string(ui32LoggedIn) + ", " +
	                                              string(clsLanguageManager::mPtr->sTexts[LAN_CLOSING], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CLOSING]) + ": " + string(ui32InClose) + ", " +
	                                              string(clsLanguageManager::mPtr->sTexts[LAN_LOGGING], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_LOGGING]) + ": " + string(ui32InLogin)).c_str());
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if (hWindow == hWndPageItems[REDT_CHAT])
	{
		RichEditPopupMenu(hWndPageItems[REDT_CHAT], m_hWnd, lParam);
	}
	else if (hWindow == hWndPageItems[LV_USERS])
	{
		int iSel = (int)::SendMessage(hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
		
		if (iSel == -1)
		{
			return;
		}
		
		HMENU hMenu = ::CreatePopupMenu();
		
		char sNick[65];
		
		LVITEM lvItem = { 0 };
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = iSel;
		lvItem.iSubItem = 0;
		lvItem.pszText = sNick;
		lvItem.cchTextMax = 65;
		
		::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem);
		
		if (clsRegManager::mPtr->Find(sNick, strlen(sNick)) == NULL)
		{
			::AppendMenu(hMenu, MF_STRING, IDC_REG_USER, clsLanguageManager::mPtr->sTexts[LAN_MENU_REG_USER]);
			::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		}
		
		::AppendMenu(hMenu, MF_STRING, IDC_BAN_USER, clsLanguageManager::mPtr->sTexts[LAN_MENU_BAN_USER]);
		::AppendMenu(hMenu, MF_STRING, IDC_KICK_USER, clsLanguageManager::mPtr->sTexts[LAN_MENU_KICK_USER]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		::AppendMenu(hMenu, MF_STRING, IDC_DISCONNECT_USER, clsLanguageManager::mPtr->sTexts[LAN_MENU_DISCONNECT_USER]);
		::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		::AppendMenu(hMenu, MF_STRING, IDC_REDIRECT_USER, clsLanguageManager::mPtr->sTexts[LAN_MENU_REDIRECT_USER]);
		
		int iX = GET_X_LPARAM(lParam);
		int iY = GET_Y_LPARAM(lParam);
		
		ListViewGetMenuPos(hWndPageItems[LV_USERS], iX, iY);
		
		::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWnd, NULL);
		
		::DestroyMenu(hMenu);
	}
}
//------------------------------------------------------------------------------

User * clsMainWindowPageUsersChat::GetUser()
{
	int iSel = (int)::SendMessage(hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return NULL;
	}
	
	char msg[1024];
	
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = iSel;
	lvItem.pszText = msg;
	lvItem.cchTextMax = 1024;
	
	if ((BOOL)::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE)
	{
		return NULL;
	}
	
	User * curUser = reinterpret_cast<User *>(lvItem.lParam);
	
	if (::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_UNCHECKED)
	{
		User * testUser = clsHashManager::mPtr->FindUser(lvItem.pszText, strlen(lvItem.pszText));
		
		if (testUser == NULL || testUser != curUser)
		{
			char buf[1024];
			int imsgLen = sprintf(buf, "<%s> *** %s %s.", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], lvItem.pszText, clsLanguageManager::mPtr->sTexts[LAN_IS_NOT_ONLINE]);
			if (CheckSprintf(imsgLen, 1024, "clsMainWindowPageUsersChat::DisconnectUser") == true)
			{
				RichEditAppendText(hWndPageItems[REDT_CHAT], buf);
			}
			
			return NULL;
		}
	}
	
	return curUser;
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::DisconnectUser()
{
	User * curUser = GetUser();
	
	if (curUser == NULL)
	{
		return;
	}
	
	char msg[1024];
	
	// disconnect the user
	clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) closed by %s", curUser->sNick, curUser->sIP, clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	
	curUser->Close();
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true)
	{
		clsGlobalDataQueue::mPtr->StatusMessageFormat("clsMainWindowPageUsersChat::DisconnectUser", "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], curUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], curUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_CLOSED_BY],
		                                              clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	}
	
	int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], curUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], curUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_CLOSED]);
	if (CheckSprintf(imsgLen, 1024, "clsMainWindowPageUsersChat::DisconnectUser4") == true)
	{
		RichEditAppendText(hWndPageItems[REDT_CHAT], msg);
	}
}
//------------------------------------------------------------------------------

void OnKickOk(const char * sLine, const int iLen)
{
	User * pUser = clsMainWindowPageUsersChat::mPtr->GetUser();
	
	if (pUser == NULL)
	{
		return;
	}
	
	clsBanManager::mPtr->TempBan(pUser, iLen == 0 ? NULL : sLine, clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK], 0, 0, false);
	
	char msg[1024];
	
	if (iLen == 0)
	{
		pUser->SendFormat("OnKickOk1", false, "<%s> %s...|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_BEING_KICKED]);
	}
	else
	{
#ifdef _FLYLINKDC_TODO
		if (iLen > 512)
		{
			sLine[513] = '\0';
			sLine[512] = '.';
			sLine[511] = '.';
			sLine[510] = '.';
		}
#endif
		
		pUser->SendFormat("OnKickOk2", false, "<%s> %s: %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_BEING_KICKED_BCS], sLine);
	}
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true)
	{
		clsGlobalDataQueue::mPtr->StatusMessageFormat("OnKickOk", "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_KICKED_BY], clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	}
	
	int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_WAS_KICKED]);
	if (CheckSprintf(imsgLen, 1024, "OnKickOk4") == true)
	{
		RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], msg);
	}
	
	// disconnect the user
	clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", pUser->sNick, pUser->sIP, clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	
	pUser->Close();
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::KickUser()
{
	User * curUser = GetUser();
	
	if (curUser == NULL)
	{
		return;
	}
	
	LineDialog * pKickDlg = new (std::nothrow) LineDialog(&OnKickOk);
	
	if (pKickDlg != NULL)
	{
		pKickDlg->DoModal(::GetParent(m_hWnd), clsLanguageManager::mPtr->sTexts[LAN_PLEASE_ENTER_REASON], "");
	}
}
//------------------------------------------------------------------------------

void OnBanOk(const char * sLine, const int iLen)
{
	User * pUser = clsMainWindowPageUsersChat::mPtr->GetUser();
	
	if (pUser == NULL)
	{
		return;
	}
	
	clsBanManager::mPtr->Ban(pUser, iLen == 0 ? NULL : sLine, clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK], false);
	
	char msg[1024];
	
	if (iLen == 0)
	{
		pUser->SendFormat("OnBanOk1", false, "<%s> %s...|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_BEING_KICKED]);
	}
	else
	{
#ifdef _FLYLINKDC_TODO
		if (iLen > 512)
		{
			sLine[513] = '\0';
			sLine[512] = '.';
			sLine[511] = '.';
			sLine[510] = '.';
		}
#endif
		
		pUser->SendFormat("OnBanOk2", false, "<%s> %s: %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_BEING_BANNED_BECAUSE], sLine);
	}
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true)
	{
		clsGlobalDataQueue::mPtr->StatusMessageFormat("OnBanOk", "<%s> *** %s %s %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR],
		                                              clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	}
	
	int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WITH_IP], pUser->sIP, clsLanguageManager::mPtr->sTexts[LAN_HAS_BEEN], clsLanguageManager::mPtr->sTexts[LAN_BANNED_LWR]);
	if (CheckSprintf(imsgLen, 1024, "OnBanOk4") == true)
	{
		RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], msg);
	}
	
	// disconnect the user
	clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", pUser->sNick, pUser->sIP, clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	
	pUser->Close();
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::BanUser()
{
	User * curUser = GetUser();
	
	if (curUser == NULL)
	{
		return;
	}
	
	LineDialog * pBanDlg = new (std::nothrow) LineDialog(&OnBanOk);
	
	if (pBanDlg != NULL)
	{
		pBanDlg->DoModal(::GetParent(m_hWnd), clsLanguageManager::mPtr->sTexts[LAN_PLEASE_ENTER_REASON], "");
	}
}
//------------------------------------------------------------------------------

void OnRedirectOk(const char * sLine, const int iLen)
{
	User * pUser = clsMainWindowPageUsersChat::mPtr->GetUser();
	
	if (pUser == NULL || iLen == 0 || iLen > 512)
	{
		return;
	}
	
	char msg[2048];
	
	pUser->SendFormat("OnRedirectOk", false, "<%s> %s %s %s %s.|$ForceMove %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_YOU_ARE_REDIRECTED_TO], sLine, clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK], sLine);
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true)
	{
		clsGlobalDataQueue::mPtr->StatusMessageFormat("OnRedirectOk", "<%s> *** %s %s %s %s %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WAS_REDIRECTED_TO], sLine, clsLanguageManager::mPtr->sTexts[LAN_BY_LWR], clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	}
	
	int imsgLen = sprintf(msg, "<%s> *** %s %s %s|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], pUser->sNick, clsLanguageManager::mPtr->sTexts[LAN_WAS_REDIRECTED_TO], sLine);
	if (CheckSprintf(imsgLen, 2048, "OnRedirectOk3") == true)
	{
		RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], msg);
	}
	
	// disconnect the user
	clsUdpDebug::mPtr->BroadcastFormat("[SYS] User %s (%s) redirected by %s", pUser->sNick, pUser->sIP, clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK]);
	
	pUser->Close();
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::RedirectUser()
{
	User * curUser = GetUser();
	
	if (curUser == NULL)
	{
		return;
	}
	
	LineDialog * pRedirectDlg = new (std::nothrow) LineDialog(&OnRedirectOk);
	
	if (pRedirectDlg != NULL)
	{
		pRedirectDlg->DoModal(::GetParent(m_hWnd), clsLanguageManager::mPtr->sTexts[LAN_PLEASE_ENTER_REDIRECT_ADDRESS],
		                      clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS] == NULL ? "" : clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS]);
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::FocusFirstItem()
{
	::SetFocus(hWndPageItems[BTN_SHOW_CHAT]);
}
//------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::FocusLastItem()
{
	if (::IsWindowEnabled(hWndPageItems[BTN_UPDATE_USERS]))
	{
		::SetFocus(hWndPageItems[BTN_UPDATE_USERS]);
	}
	else
	{
		::SetFocus(hWndPageItems[LV_USERS]);
	}
}
//------------------------------------------------------------------------------

HWND clsMainWindowPageUsersChat::GetWindowHandle()
{
	return m_hWnd;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

void clsMainWindowPageUsersChat::UpdateSplitterParts()
{
	::SetWindowPos(hWndPageItems[REDT_CHAT], NULL, 0, 0, iSplitterPos - 2, rcSplitter.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight - 4, SWP_NOMOVE | SWP_NOZORDER);
	::SendMessage(hWndPageItems[REDT_CHAT], WM_VSCROLL, SB_BOTTOM, NULL);
	::SetWindowPos(hWndPageItems[LV_USERS], NULL, iSplitterPos + 2, clsGuiSettingManager::iCheckHeight, rcSplitter.right - (iSplitterPos + 4), rcSplitter.bottom - clsGuiSettingManager::iEditHeight - clsGuiSettingManager::iCheckHeight - 4, SWP_NOZORDER);
	::SetWindowPos(hWndPageItems[EDT_CHAT], NULL, 2, rcSplitter.bottom - clsGuiSettingManager::iEditHeight - 2, iSplitterPos - 2, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
	::SetWindowPos(hWndPageItems[BTN_UPDATE_USERS], NULL, iSplitterPos + 2, rcSplitter.bottom - clsGuiSettingManager::iEditHeight - 2, rcSplitter.right - (iSplitterPos + 4), clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
