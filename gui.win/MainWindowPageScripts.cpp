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
#include "../core/LuaInc.h"
//---------------------------------------------------------------------------
#include "MainWindowPageScripts.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/LuaScript.h"
#include "../core/LuaScriptManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/UdpDebug.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "MainWindow.h"
#include "Resources.h"
#include "ScriptEditorDialog.h"
//---------------------------------------------------------------------------
clsMainWindowPageScripts * clsMainWindowPageScripts::mPtr = nullptr;
//---------------------------------------------------------------------------
#define IDC_OPEN_IN_EXT_EDITOR      500
#define IDC_OPEN_IN_SCRIPT_EDITOR   501
#define IDC_DELETE_SCRIPT           502
//---------------------------------------------------------------------------

clsMainWindowPageScripts::clsMainWindowPageScripts() : bIgnoreItemChanged(false)
{
	clsMainWindowPageScripts::mPtr = this;
	
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
	
	iPercentagePos = clsGuiSettingManager::mPtr->i32Integers[GUISETINT_SCRIPTS_SPLITTER];
}
//---------------------------------------------------------------------------

clsMainWindowPageScripts::~clsMainWindowPageScripts()
{
	clsMainWindowPageScripts::mPtr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT clsMainWindowPageScripts::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
	{
		CHARRANGE cr = { 0, 0 };
		::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_EXSETSEL, 0, (LPARAM)&cr);
		::SetFocus(hWndPageItems[REDT_SCRIPTS_ERRORS]);
		return 0;
	}
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcMain = { 0, 0, ((WINDOWPOS*)lParam)->cx, ((WINDOWPOS*)lParam)->cy };
		SetSplitterRect(&rcMain);
		
		return 0;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case BTN_OPEN_SCRIPT_EDITOR:
		{
			OpenScriptEditor();
			return 0;
		}
		case BTN_REFRESH_SCRIPTS:
			RefreshScripts();
			return 0;
		case BTN_MOVE_UP:
			MoveUp();
			return 0;
		case BTN_MOVE_DOWN:
			MoveDown();
			return 0;
		case BTN_RESTART_SCRIPTS:
			RestartScripts();
			return 0;
		case IDC_OPEN_IN_EXT_EDITOR:
			OpenInExternalEditor();
			return 0;
		case IDC_OPEN_IN_SCRIPT_EDITOR:
			OpenInScriptEditor();
			return 0;
		case IDC_DELETE_SCRIPT:
			DeleteScript();
			return 0;
		}
		
		if (RichEditCheckMenuCommands(hWndPageItems[REDT_SCRIPTS_ERRORS], LOWORD(wParam)) == true)
		{
			return 0;
		}
		
		break;
	case WM_CONTEXTMENU:
		OnContextMenu((HWND)wParam, lParam);
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == hWndPageItems[LV_SCRIPTS])
		{
			if (((LPNMHDR)lParam)->code == LVN_ITEMCHANGED)
			{
				OnItemChanged((LPNMLISTVIEW)lParam);
			}
			else if (((LPNMHDR)lParam)->code == NM_DBLCLK)
			{
				if (((LPNMITEMACTIVATE)lParam)->iItem == -1)
				{
					break;
				}
				
				OnDoubleClick((LPNMITEMACTIVATE)lParam);
				
				return 0;
			}
		}
		else if (((LPNMHDR)lParam)->hwndFrom == hWndPageItems[REDT_SCRIPTS_ERRORS] && ((LPNMHDR)lParam)->code == EN_LINK)
		{
			if (((ENLINK *)lParam)->msg == WM_LBUTTONUP)
			{
				RichEditOpenLink(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS], (ENLINK *)lParam);
				return 1;
			}
		}
		
		break;
	case WM_DESTROY:
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_SCRIPTS_SPLITTER, iPercentagePos);
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_SCRIPT_NAMES, (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETCOLUMNWIDTH, 0, 0));
		clsGuiSettingManager::mPtr->SetInteger(GUISETINT_SCRIPT_MEMORY_USAGES, (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETCOLUMNWIDTH, 1, 0));
		
		break;
	}
	
	if (BasicSplitterProc(uMsg, wParam, lParam) == true)
	{
		return 0;
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

static LRESULT CALLBACK MultiRichEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, FALSE));
			return 0;
		}
		
		::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
		return 0;
	}
	else if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)
	{
		return 0;
	}
	
	return ::CallWindowProc(clsGuiSettingManager::wpOldMultiRichEditProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

static LRESULT CALLBACK ScriptsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE)
	{
		if (wParam == VK_TAB)
		{
			return DLGC_WANTTAB;
		}
		else if (wParam == VK_RETURN)
		{
			return DLGC_WANTALLKEYS;
		}
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			clsMainWindowPageScripts * pParent = (clsMainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			
			if (pParent != NULL)
			{
				if (::IsWindowEnabled(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_UP]))
				{
					::SetFocus(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_UP]);
					return 0;
				}
				else if (::IsWindowEnabled(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_DOWN]))
				{
					::SetFocus(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_DOWN]);
					return 0;
				}
				else if (::IsWindowEnabled(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_RESTART_SCRIPTS]))
				{
					::SetFocus(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_RESTART_SCRIPTS]);
					return 0;
				}
			}
			
			::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
			
			return 0;
		}
		else
		{
			::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, TRUE));
			return 0;
		}
	}
	else if (uMsg == WM_CHAR && wParam == VK_RETURN)
	{
		clsMainWindowPageScripts * pParent = (clsMainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (pParent != NULL)
		{
			pParent->OpenInScriptEditor();
			return 0;
		}
	}
	
	return ::CallWindowProc(clsGuiSettingManager::wpOldListViewProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

static LRESULT CALLBACK MoveUpProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			clsMainWindowPageScripts * pParent = (clsMainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			
			if (pParent != NULL)
			{
				if (::IsWindowEnabled(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_DOWN]))
				{
					::SetFocus(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_DOWN]);
					return 0;
				}
				else if (::IsWindowEnabled(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_RESTART_SCRIPTS]))
				{
					::SetFocus(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_RESTART_SCRIPTS]);
					return 0;
				}
			}
			
			::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
			
			return 0;
		}
		else
		{
			::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, TRUE));
			return 0;
		}
	}
	
	return ::CallWindowProc(clsGuiSettingManager::wpOldButtonProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

static LRESULT CALLBACK MoveDownProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			clsMainWindowPageScripts * pParent = (clsMainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			
			if (pParent != NULL)
			{
				if (::IsWindowEnabled(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_RESTART_SCRIPTS]))
				{
					::SetFocus(pParent->hWndPageItems[clsMainWindowPageScripts::BTN_RESTART_SCRIPTS]);
					return 0;
				}
			}
			
			::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
			
			return 0;
		}
		else
		{
			::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, TRUE));
			return 0;
		}
	}
	
	return ::CallWindowProc(clsGuiSettingManager::wpOldButtonProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

bool clsMainWindowPageScripts::CreateMainWindowPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	RECT rcMain;
	::GetClientRect(m_hWnd, &rcMain);
	
	hWndPageItems[GB_SCRIPTS_ERRORS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_SCRIPTS_ERRORS],
	                                                    WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_GROUPBOX, 3, 0, rcMain.right - ScaleGui(145) - 9, rcMain.bottom - 3, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                    
	hWndPageItems[REDT_SCRIPTS_ERRORS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
	                                                      11, clsGuiSettingManager::iGroupBoxMargin, rcMain.right - ScaleGui(145) - 25, rcMain.bottom - (clsGuiSettingManager::iGroupBoxMargin + 11), m_hWnd, (HMENU)REDT_SCRIPTS_ERRORS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_EXLIMITTEXT, 0, (LPARAM)1048576);
	::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_AUTOURLDETECT, TRUE, 0);
	::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_GETEVENTMASK, 0, 0) | ENM_LINK);
	
	hWndPageItems[BTN_OPEN_SCRIPT_EDITOR] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_OPEN_SCRIPT_EDITOR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                         rcMain.right - ScaleGui(145) - 4, 1, ScaleGui(145) + 2, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)BTN_OPEN_SCRIPT_EDITOR, clsServerManager::hInstance, NULL);
	                                                         
	hWndPageItems[BTN_REFRESH_SCRIPTS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REFRESH_SCRIPTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                      rcMain.right - ScaleGui(145) - 4, clsGuiSettingManager::iEditHeight + 4, ScaleGui(145) + 2, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)BTN_REFRESH_SCRIPTS, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[LV_SCRIPTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
	                                             rcMain.right - ScaleGui(145) - 3, (2 * clsGuiSettingManager::iEditHeight) + 8, ScaleGui(145), rcMain.bottom - ((2 * clsGuiSettingManager::iEditHeight) + 8) - ((2 * clsGuiSettingManager::iEditHeight) - 5) - 14, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP | LVS_EX_CHECKBOXES);
	
	hWndPageItems[BTN_MOVE_UP] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MOVE_UP], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                              rcMain.right - ScaleGui(145) - 4, rcMain.bottom - (2 * clsGuiSettingManager::iEditHeight) - 5, ScaleGui(72), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)BTN_MOVE_UP, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[BTN_MOVE_DOWN] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MOVE_DOWN], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                                rcMain.right - ScaleGui(72) - 2, rcMain.bottom - (2 * clsGuiSettingManager::iEditHeight) - 5, ScaleGui(72), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)BTN_MOVE_DOWN, clsServerManager::hInstance, NULL);
	                                                
	{
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
		if (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false || clsServerManager::bServerRunning == false)
		{
			dwStyle |= WS_DISABLED;
		}
		
		hWndPageItems[BTN_RESTART_SCRIPTS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_RESTART_SCRIPTS], dwStyle,
		                                                      rcMain.right - ScaleGui(145) - 4, rcMain.bottom - clsGuiSettingManager::iEditHeight - 2, ScaleGui(145) + 2, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)BTN_RESTART_SCRIPTS, clsServerManager::hInstance, NULL);
	}
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	SetSplitterRect(&rcMain);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = clsGuiSettingManager::mPtr->i32Integers[GUISETINT_SCRIPT_NAMES];
	lvColumn.pszText = clsLanguageManager::mPtr->sTexts[LAN_SCRIPT_FILE];
	lvColumn.iSubItem = 0;
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	
	lvColumn.fmt = LVCFMT_RIGHT;
	lvColumn.cx = clsGuiSettingManager::mPtr->i32Integers[GUISETINT_SCRIPT_MEMORY_USAGES];
	lvColumn.pszText = clsLanguageManager::mPtr->sTexts[LAN_MEM_USAGE];
	lvColumn.iSubItem = 1;
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
	
	AddScriptsToList(false);
	
	clsGuiSettingManager::wpOldMultiRichEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[REDT_SCRIPTS_ERRORS], GWLP_WNDPROC, (LONG_PTR)MultiRichEditProc);
	
	::SetWindowLongPtr(hWndPageItems[LV_SCRIPTS], GWLP_USERDATA, (LONG_PTR)this);
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[LV_SCRIPTS], GWLP_WNDPROC, (LONG_PTR)ScriptsProc);
	
	::SetWindowLongPtr(hWndPageItems[BTN_MOVE_UP], GWLP_USERDATA, (LONG_PTR)this);
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_MOVE_UP], GWLP_WNDPROC, (LONG_PTR)MoveUpProc);
	
	::SetWindowLongPtr(hWndPageItems[BTN_MOVE_DOWN], GWLP_USERDATA, (LONG_PTR)this);
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_MOVE_DOWN], GWLP_WNDPROC, (LONG_PTR)MoveDownProc);
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_RESTART_SCRIPTS], GWLP_WNDPROC, (LONG_PTR)LastButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::UpdateLanguage()
{
	::SetWindowText(hWndPageItems[GB_SCRIPTS_ERRORS], clsLanguageManager::mPtr->sTexts[LAN_SCRIPTS_ERRORS]);
	::SetWindowText(hWndPageItems[BTN_OPEN_SCRIPT_EDITOR], clsLanguageManager::mPtr->sTexts[LAN_OPEN_SCRIPT_EDITOR]);
	::SetWindowText(hWndPageItems[BTN_REFRESH_SCRIPTS], clsLanguageManager::mPtr->sTexts[LAN_REFRESH_SCRIPTS]);
	
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = clsLanguageManager::mPtr->sTexts[LAN_SCRIPT_FILE];
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETCOLUMN, 0, (LPARAM)&lvColumn);
	
	lvColumn.pszText = clsLanguageManager::mPtr->sTexts[LAN_MEM_USAGE];
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETCOLUMN, 1, (LPARAM)&lvColumn);
	
	::SetWindowText(hWndPageItems[BTN_MOVE_UP], clsLanguageManager::mPtr->sTexts[LAN_MOVE_UP]);
	::SetWindowText(hWndPageItems[BTN_MOVE_DOWN], clsLanguageManager::mPtr->sTexts[LAN_MOVE_DOWN]);
	::SetWindowText(hWndPageItems[BTN_RESTART_SCRIPTS], clsLanguageManager::mPtr->sTexts[LAN_RESTART_SCRIPTS]);
}
//---------------------------------------------------------------------------

char * clsMainWindowPageScripts::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_SCRIPTS];
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if (hWindow == hWndPageItems[REDT_SCRIPTS_ERRORS])
	{
		RichEditPopupMenu(clsMainWindowPageScripts::mPtr->hWndPageItems[clsMainWindowPageScripts::REDT_SCRIPTS_ERRORS], clsMainWindowPageScripts::mPtr->m_hWnd, lParam);
		return;
	}
	
	if (hWindow != hWndPageItems[LV_SCRIPTS])
	{
		return;
	}
	
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	HMENU hMenu = ::CreatePopupMenu();
	
	::AppendMenu(hMenu, MF_STRING, IDC_OPEN_IN_EXT_EDITOR, clsLanguageManager::mPtr->sTexts[LAN_OPEN_EXT_EDIT]);
	::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	::AppendMenu(hMenu, MF_STRING, IDC_OPEN_IN_SCRIPT_EDITOR, clsLanguageManager::mPtr->sTexts[LAN_OPEN_IN_SCRIPT_EDITOR]);
	::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	::AppendMenu(hMenu, MF_STRING, IDC_DELETE_SCRIPT, clsLanguageManager::mPtr->sTexts[LAN_DELETE_SCRIPT]);
	
	::SetMenuDefaultItem(hMenu, IDC_OPEN_IN_SCRIPT_EDITOR, FALSE);
	
	int iX = GET_X_LPARAM(lParam);
	int iY = GET_Y_LPARAM(lParam);
	
	ListViewGetMenuPos(hWndPageItems[LV_SCRIPTS], iX, iY);
	
	::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWnd, NULL);
	
	::DestroyMenu(hMenu);
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::OpenScriptEditor(const char * sScript/* = NULL*/)
{
	ScriptEditorDialog * pScriptEditorDialog = new (std::nothrow) ScriptEditorDialog();
	
	if (pScriptEditorDialog != NULL)
	{
		pScriptEditorDialog->DoModal(clsMainWindow::mPtr->m_hWnd);
		
		if (sScript != NULL)
		{
			pScriptEditorDialog->LoadScript(sScript);
		}
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::RefreshScripts()
{
	clsScriptManager::mPtr->CheckForDeletedScripts();
	clsScriptManager::mPtr->CheckForNewScripts();
	
	AddScriptsToList(true);
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::AddScriptsToList(const bool bDelete)
{
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	if (bDelete == true)
	{
		::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_DELETEALLITEMS, 0, 0);
	}
	
	for (uint8_t ui8i = 0; ui8i < clsScriptManager::mPtr->ui8ScriptCount; ui8i++)
	{
		ScriptToList(ui8i, true, false);
	}
	
	ListViewSelectFirstItem(hWndPageItems[LV_SCRIPTS]);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	
	UpdateUpDown();
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::ScriptToList(const uint8_t ui8ScriptId, const bool bInsert, const bool bSelected)
{
	bIgnoreItemChanged = true;
	
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = ui8ScriptId;
	lvItem.pszText = clsScriptManager::mPtr->ppScriptTable[ui8ScriptId]->sName;
	lvItem.lParam = (LPARAM)clsScriptManager::mPtr->ppScriptTable[ui8ScriptId];
	
	if (bSelected == true)
	{
		lvItem.mask |= LVIF_STATE;
		lvItem.state = LVIS_SELECTED;
		lvItem.stateMask = LVIS_SELECTED;
	}
	
	int i = -1;
	
	if (bInsert == true)
	{
		i = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
	}
	else
	{
		i = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
	
	if (i != -1 || bInsert == false)
	{
		lvItem.mask = LVIF_STATE;
		lvItem.state = INDEXTOSTATEIMAGEMASK(clsScriptManager::mPtr->ppScriptTable[ui8ScriptId]->bEnabled == true ? 2 : 1);
		lvItem.stateMask = LVIS_STATEIMAGEMASK;
		
		::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEMSTATE, ui8ScriptId, (LPARAM)&lvItem);
	}
	
	bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::OnItemChanged(const LPNMLISTVIEW &pListView)
{
	UpdateUpDown();
	
	if (bIgnoreItemChanged == true || pListView->iItem == -1 || (pListView->uNewState & LVIS_STATEIMAGEMASK) == (pListView->uOldState & LVIS_STATEIMAGEMASK))
	{
		return;
	}
	
	if ((((pListView->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) == 0)
	{
		if (clsScriptManager::mPtr->ppScriptTable[pListView->iItem]->bEnabled == false)
		{
			return;
		}
		
		clsScriptManager::mPtr->ppScriptTable[pListView->iItem]->bEnabled = false;
		
		if (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false || clsServerManager::bServerRunning == false)
		{
			return;
		}
		
		clsScriptManager::mPtr->StopScript(clsScriptManager::mPtr->ppScriptTable[pListView->iItem], false);
		ClearMemUsage((uint8_t)pListView->iItem);
		
		RichEditAppendText(hWndPageItems[REDT_SCRIPTS_ERRORS], (string(clsLanguageManager::mPtr->sTexts[LAN_SCRIPT_STOPPED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SCRIPT_STOPPED]) + ".").c_str());
	}
	else
	{
		if (clsScriptManager::mPtr->ppScriptTable[pListView->iItem]->bEnabled == true)
		{
			return;
		}
		
		clsScriptManager::mPtr->ppScriptTable[pListView->iItem]->bEnabled = true;
		
		if (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == false || clsServerManager::bServerRunning == false)
		{
			return;
		}
		
		if (clsScriptManager::mPtr->StartScript(clsScriptManager::mPtr->ppScriptTable[pListView->iItem], false) == true)
		{
			RichEditAppendText(hWndPageItems[REDT_SCRIPTS_ERRORS], (string(clsLanguageManager::mPtr->sTexts[LAN_SCRIPT_STARTED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SCRIPT_STARTED]) + ".").c_str());
		}
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::OnDoubleClick(const LPNMITEMACTIVATE &pItemActivate)
{
	RECT rc = { LVIR_ICON, 0, 0, 0 };
	
	if (::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETITEMRECT, pItemActivate->iItem, (LPARAM)&rc) == FALSE || pItemActivate->ptAction.x > rc.left)
	{
		string sScript = clsServerManager::sScriptPath + clsScriptManager::mPtr->ppScriptTable[pItemActivate->iItem]->sName;
		OpenScriptEditor(sScript.c_str());
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::ClearMemUsageAll()
{
	for (uint8_t ui8i = 0; ui8i < clsScriptManager::mPtr->ui8ScriptCount; ui8i++)
	{
		ClearMemUsage(ui8i);
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::UpdateMemUsage()
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 1;
	
	for (uint8_t ui8i = 0; ui8i < clsScriptManager::mPtr->ui8ScriptCount; ui8i++)
	{
		if (clsScriptManager::mPtr->ppScriptTable[ui8i]->bEnabled == false)
		{
			continue;
		}
		
		string tmp(lua_gc(clsScriptManager::mPtr->ppScriptTable[ui8i]->pLUA, LUA_GCCOUNT, 0));
		
		lvItem.iItem = ui8i;
		
		string sMemUsage(lua_gc(clsScriptManager::mPtr->ppScriptTable[ui8i]->pLUA, LUA_GCCOUNT, 0));
		lvItem.pszText = (char*) sMemUsage.c_str();
		
		::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::MoveUp()
{
	HWND hWndFocus = ::GetFocus();
	
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	clsScriptManager::mPtr->MoveScript((uint8_t)iSel, true);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	ScriptToList((uint8_t)iSel, false, false);
	
	iSel--;
	
	ScriptToList((uint8_t)iSel, false, true);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, iSel, FALSE);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	
	UpdateUpDown();
	
	if (hWndFocus == hWndPageItems[BTN_MOVE_UP])
	{
		if (::IsWindowEnabled(hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_UP]))
		{
			::SetFocus(hWndPageItems[BTN_MOVE_UP]);
		}
		else
		{
			::SetFocus(hWndPageItems[BTN_MOVE_DOWN]);
		}
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::MoveDown()
{
	HWND hWndFocus = ::GetFocus();
	
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	clsScriptManager::mPtr->MoveScript((uint8_t)iSel, false);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	ScriptToList((uint8_t)iSel, false, false);
	
	iSel++;
	
	ScriptToList((uint8_t)iSel, false, true);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, iSel, FALSE);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	
	UpdateUpDown();
	
	if (hWndFocus == hWndPageItems[BTN_MOVE_DOWN])
	{
		if (::IsWindowEnabled(hWndPageItems[clsMainWindowPageScripts::BTN_MOVE_DOWN]))
		{
			::SetFocus(hWndPageItems[BTN_MOVE_DOWN]);
		}
		else
		{
			::SetFocus(hWndPageItems[BTN_MOVE_UP]);
		}
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::RestartScripts()
{
	clsScriptManager::mPtr->Restart();
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::UpdateUpDown()
{
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], FALSE);
		::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], FALSE);
	}
	else if (iSel == 0)
	{
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], FALSE);
		if (iSel == (clsScriptManager::mPtr->ui8ScriptCount - 1))
		{
			::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], FALSE);
		}
		else
		{
			::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], TRUE);
		}
	}
	else if (iSel == (clsScriptManager::mPtr->ui8ScriptCount - 1))
	{
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], FALSE);
	}
	else
	{
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], TRUE);
	}
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::OpenInExternalEditor()
{
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	::ShellExecute(NULL, NULL, (clsServerManager::sScriptPath + clsScriptManager::mPtr->ppScriptTable[iSel]->sName).c_str(), NULL, NULL, SW_SHOWNORMAL);
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::OpenInScriptEditor()
{
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	string sScript = clsServerManager::sScriptPath + clsScriptManager::mPtr->ppScriptTable[iSel]->sName;
	OpenScriptEditor(sScript.c_str());
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::DeleteScript()
{
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	if (iSel == -1)
	{
		return;
	}
	
	if (::MessageBox(m_hWnd, (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
	{
		return;
	}
	
	clsScriptManager::mPtr->DeleteScript((uint8_t)iSel);
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_DELETEITEM, iSel, 0);
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::MoveScript(uint8_t ui8ScriptId, const bool bUp)
{
	int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM) - 1, LVNI_SELECTED);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);
	
	ScriptToList(ui8ScriptId, false, (iSel != -1 && ((bUp == true && iSel == (int)ui8ScriptId - 1) || (bUp == false && iSel == (int)ui8ScriptId + 1))) ? true : false);
	
	if (bUp == true)
	{
		ui8ScriptId--;
	}
	else
	{
		ui8ScriptId++;
	}
	
	ScriptToList(ui8ScriptId, false, (iSel != -1 && iSel == (int)ui8ScriptId) ? true : false);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, ui8ScriptId, FALSE);
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);
	
	UpdateUpDown();
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::UpdateCheck(const uint8_t ui8ScriptId)
{
	bIgnoreItemChanged = true;
	
	ListView_SetItemState(hWndPageItems[LV_SCRIPTS], ui8ScriptId, INDEXTOSTATEIMAGEMASK(clsScriptManager::mPtr->ppScriptTable[ui8ScriptId]->bEnabled == true ? 2 : 1), LVIS_STATEIMAGEMASK);
	
	if (clsScriptManager::mPtr->ppScriptTable[ui8ScriptId]->bEnabled == false)
	{
		ClearMemUsage(ui8ScriptId);
	}
	
	bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::ClearMemUsage(uint8_t ui8ScriptId)
{
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = ui8ScriptId;
	lvItem.iSubItem = 1;
	lvItem.pszText = "";
	
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::FocusFirstItem()
{
	::SetFocus(hWndPageItems[REDT_SCRIPTS_ERRORS]);
}
//------------------------------------------------------------------------------

void clsMainWindowPageScripts::FocusLastItem()
{
	if (::IsWindowEnabled(hWndPageItems[BTN_RESTART_SCRIPTS]))
	{
		::SetFocus(hWndPageItems[BTN_RESTART_SCRIPTS]);
	}
	else if (::IsWindowEnabled(hWndPageItems[BTN_MOVE_DOWN]))
	{
		::SetFocus(hWndPageItems[BTN_MOVE_DOWN]);
	}
	else if (::IsWindowEnabled(hWndPageItems[BTN_MOVE_UP]))
	{
		::SetFocus(hWndPageItems[BTN_MOVE_UP]);
	}
	else if (::IsWindowEnabled(hWndPageItems[LV_SCRIPTS]))
	{
		::SetFocus(hWndPageItems[LV_SCRIPTS]);
	}
}
//------------------------------------------------------------------------------

HWND clsMainWindowPageScripts::GetWindowHandle()
{
	return m_hWnd;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

void clsMainWindowPageScripts::UpdateSplitterParts()
{
	::SetWindowPos(hWndPageItems[BTN_RESTART_SCRIPTS], NULL, iSplitterPos + 2, rcSplitter.bottom - clsGuiSettingManager::iEditHeight - 2, rcSplitter.right - (iSplitterPos + 4), clsGuiSettingManager::iEditHeight,
	               SWP_NOZORDER);
	               
	int iButtonWidth = (rcSplitter.right - (iSplitterPos + 7)) / 2;
	::SetWindowPos(hWndPageItems[BTN_MOVE_DOWN], NULL, rcSplitter.right - iButtonWidth - 2, rcSplitter.bottom - (2 * clsGuiSettingManager::iEditHeight) - 5, iButtonWidth, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
	::SetWindowPos(hWndPageItems[BTN_MOVE_UP], NULL, iSplitterPos + 2, rcSplitter.bottom - (2 * clsGuiSettingManager::iEditHeight) - 5, iButtonWidth, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
	
	::SetWindowPos(hWndPageItems[LV_SCRIPTS], NULL, iSplitterPos + 2, (2 * clsGuiSettingManager::iEditHeight) + 8, rcSplitter.right - (iSplitterPos + 4),
	               rcSplitter.bottom - ((2 * clsGuiSettingManager::iEditHeight) + 8) - ((2 * clsGuiSettingManager::iEditHeight) - 5) - 14, SWP_NOZORDER);
	::SetWindowPos(hWndPageItems[BTN_REFRESH_SCRIPTS], NULL, iSplitterPos + 2, clsGuiSettingManager::iEditHeight + 4, rcSplitter.right - (iSplitterPos + 4), clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
	::SetWindowPos(hWndPageItems[BTN_OPEN_SCRIPT_EDITOR], NULL, iSplitterPos + 2, 1, rcSplitter.right - (iSplitterPos + 4), clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
	::SetWindowPos(hWndPageItems[REDT_SCRIPTS_ERRORS], NULL, 0, 0, iSplitterPos - 19, rcSplitter.bottom - (clsGuiSettingManager::iGroupBoxMargin + 11), SWP_NOMOVE | SWP_NOZORDER);
	::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], WM_VSCROLL, SB_BOTTOM, NULL);
	::SetWindowPos(hWndPageItems[GB_SCRIPTS_ERRORS], NULL, 0, 0, iSplitterPos - 3, rcSplitter.bottom - 3, SWP_NOMOVE | SWP_NOZORDER);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
