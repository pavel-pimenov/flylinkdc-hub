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
MainWindowPageScripts * MainWindowPageScripts::m_Ptr = nullptr;
//---------------------------------------------------------------------------
#define IDC_OPEN_IN_EXT_EDITOR      500
#define IDC_OPEN_IN_SCRIPT_EDITOR   501
#define IDC_DELETE_SCRIPT           502
//---------------------------------------------------------------------------

MainWindowPageScripts::MainWindowPageScripts() : m_bIgnoreItemChanged(false) {
    MainWindowPageScripts::m_Ptr = this;

    memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));

	m_iPercentagePos = GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_SCRIPTS_SPLITTER];
}
//---------------------------------------------------------------------------

MainWindowPageScripts::~MainWindowPageScripts() {
    MainWindowPageScripts::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT MainWindowPageScripts::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_SETFOCUS: {
            CHARRANGE cr = { 0, 0 };
            ::SendMessage(m_hWndPageItems[REDT_SCRIPTS_ERRORS], EM_EXSETSEL, 0, (LPARAM)&cr);
            ::SetFocus(m_hWndPageItems[REDT_SCRIPTS_ERRORS]);
            return 0;
		}
        case WM_WINDOWPOSCHANGED: {
            RECT rcMain = { 0, 0, ((WINDOWPOS*)lParam)->cx, ((WINDOWPOS*)lParam)->cy };
            SetSplitterRect(&rcMain);

            return 0;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case BTN_OPEN_SCRIPT_EDITOR: {
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

            if(RichEditCheckMenuCommands(m_hWndPageItems[REDT_SCRIPTS_ERRORS], LOWORD(wParam)) == true) {
                return 0;
            }

            break;
        case WM_CONTEXTMENU:
            OnContextMenu((HWND)wParam, lParam);
            break;
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == m_hWndPageItems[LV_SCRIPTS]) {
                if(((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
                    OnItemChanged((LPNMLISTVIEW)lParam);
                } else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
                    if(((LPNMITEMACTIVATE)lParam)->iItem == -1) {
                        break;
                    }

                    OnDoubleClick((LPNMITEMACTIVATE)lParam);

                    return 0;
                }
            } else if(((LPNMHDR)lParam)->hwndFrom == m_hWndPageItems[REDT_SCRIPTS_ERRORS] && ((LPNMHDR)lParam)->code == EN_LINK) {
                if(((ENLINK *)lParam)->msg == WM_LBUTTONUP) {
                    RichEditOpenLink(m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS], (ENLINK *)lParam);
                    return 1;
                }
            }

            break;
        case WM_DESTROY:
            GuiSettingManager::m_Ptr->SetInteger(GUISETINT_SCRIPTS_SPLITTER, m_iPercentagePos);
            GuiSettingManager::m_Ptr->SetInteger(GUISETINT_SCRIPT_NAMES, (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETCOLUMNWIDTH, 0, 0));
            GuiSettingManager::m_Ptr->SetInteger(GUISETINT_SCRIPT_MEMORY_USAGES, (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETCOLUMNWIDTH, 1, 0));

            break;
    }

    if(BasicSplitterProc(uMsg, wParam, lParam) == true) {
    	return 0;
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

static LRESULT CALLBACK MultiRichEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return DLGC_WANTTAB;
    } else if(uMsg == WM_CHAR && wParam == VK_TAB) {
        if((::GetKeyState(VK_SHIFT) & 0x8000) == 0) {
            ::SetFocus(::GetNextDlgTabItem(MainWindow::m_Ptr->m_hWnd, hWnd, FALSE));
			return 0;
        }

		::SetFocus(MainWindow::m_Ptr->m_hWndWindowItems[MainWindow::TC_TABS]);
		return 0;
    } else if(uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        return 0;
    }

    return ::CallWindowProc(GuiSettingManager::m_wpOldMultiRichEditProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

static LRESULT CALLBACK ScriptsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_GETDLGCODE) {
        if(wParam == VK_TAB) {
            return DLGC_WANTTAB;
        } else if(wParam == VK_RETURN) {
            return DLGC_WANTALLKEYS;
        }
    } else if(uMsg == WM_CHAR && wParam == VK_TAB) {
        if((::GetKeyState(VK_SHIFT) & 0x8000) == 0) {
            MainWindowPageScripts * pParent = (MainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if(pParent != nullptr) {
                if(::IsWindowEnabled(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_UP])) {
                    ::SetFocus(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_UP]);
                    return 0;
                } else if(::IsWindowEnabled(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_DOWN])) {
                    ::SetFocus(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_DOWN]);
                    return 0;
                } else if(::IsWindowEnabled(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS])) {
                    ::SetFocus(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS]);
                    return 0;
                }
            }

            ::SetFocus(MainWindow::m_Ptr->m_hWndWindowItems[MainWindow::TC_TABS]);

            return 0;
        } else {
			::SetFocus(::GetNextDlgTabItem(MainWindow::m_Ptr->m_hWnd, hWnd, TRUE));
            return 0;
        }
    } else if(uMsg == WM_CHAR && wParam == VK_RETURN) {
        MainWindowPageScripts * pParent = (MainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if(pParent != nullptr) {
            pParent->OpenInScriptEditor();
            return 0;
        }
    }

    return ::CallWindowProc(GuiSettingManager::m_wpOldListViewProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

static LRESULT CALLBACK MoveUpProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return DLGC_WANTTAB;
    } else if(uMsg == WM_CHAR && wParam == VK_TAB) {
        if((::GetKeyState(VK_SHIFT) & 0x8000) == 0) {
            MainWindowPageScripts * pParent = (MainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if(pParent != nullptr) {
                if(::IsWindowEnabled(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_DOWN])) {
                    ::SetFocus(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_DOWN]);
                    return 0;
                } else if(::IsWindowEnabled(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS])) {
                    ::SetFocus(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS]);
                    return 0;
                }
            }

            ::SetFocus(MainWindow::m_Ptr->m_hWndWindowItems[MainWindow::TC_TABS]);

            return 0;
        } else {
			::SetFocus(::GetNextDlgTabItem(MainWindow::m_Ptr->m_hWnd, hWnd, TRUE));
            return 0;
        }
    }

    return ::CallWindowProc(GuiSettingManager::m_wpOldButtonProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

static LRESULT CALLBACK MoveDownProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return DLGC_WANTTAB;
    } else if(uMsg == WM_CHAR && wParam == VK_TAB) {
        if((::GetKeyState(VK_SHIFT) & 0x8000) == 0) {
            MainWindowPageScripts * pParent = (MainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if(pParent != nullptr) {
                if(::IsWindowEnabled(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS])) {
                    ::SetFocus(pParent->m_hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS]);
                    return 0;
                }
            }

            ::SetFocus(MainWindow::m_Ptr->m_hWndWindowItems[MainWindow::TC_TABS]);

            return 0;
        } else {
			::SetFocus(::GetNextDlgTabItem(MainWindow::m_Ptr->m_hWnd, hWnd, TRUE));
            return 0;
        }
    }

    return ::CallWindowProc(GuiSettingManager::m_wpOldButtonProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

bool MainWindowPageScripts::CreateMainWindowPage(HWND hOwner) {
    CreateHWND(hOwner);

    RECT rcMain;
    ::GetClientRect(m_hWnd, &rcMain);

	m_hWndPageItems[GB_SCRIPTS_ERRORS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTS_ERRORS],
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_GROUPBOX, 3, 0, rcMain.right - ScaleGui(145) - 9, rcMain.bottom - 3, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[REDT_SCRIPTS_ERRORS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
        11, GuiSettingManager::m_iGroupBoxMargin, rcMain.right - ScaleGui(145) - 25, rcMain.bottom - (GuiSettingManager::m_iGroupBoxMargin + 11), m_hWnd, (HMENU)REDT_SCRIPTS_ERRORS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[REDT_SCRIPTS_ERRORS], EM_EXLIMITTEXT, 0, (LPARAM)1048576);
    ::SendMessage(m_hWndPageItems[REDT_SCRIPTS_ERRORS], EM_AUTOURLDETECT, TRUE, 0);
    ::SendMessage(m_hWndPageItems[REDT_SCRIPTS_ERRORS], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(m_hWndPageItems[REDT_SCRIPTS_ERRORS], EM_GETEVENTMASK, 0, 0) | ENM_LINK);

	m_hWndPageItems[BTN_OPEN_SCRIPT_EDITOR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_OPEN_SCRIPT_EDITOR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right - ScaleGui(145) - 4, 1, ScaleGui(145) + 2, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)BTN_OPEN_SCRIPT_EDITOR, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_REFRESH_SCRIPTS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REFRESH_SCRIPTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right - ScaleGui(145) - 4, GuiSettingManager::m_iEditHeight + 4, ScaleGui(145) + 2, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)BTN_REFRESH_SCRIPTS, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LV_SCRIPTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        rcMain.right - ScaleGui(145) - 3, (2 * GuiSettingManager::m_iEditHeight) + 8, ScaleGui(145), rcMain.bottom - ((2 * GuiSettingManager::m_iEditHeight) + 8) - ((2 * GuiSettingManager::m_iEditHeight) - 5) - 14, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP | LVS_EX_CHECKBOXES);

	m_hWndPageItems[BTN_MOVE_UP] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MOVE_UP], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right - ScaleGui(145) - 4, rcMain.bottom - (2 * GuiSettingManager::m_iEditHeight) - 5, ScaleGui(72), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)BTN_MOVE_UP, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_MOVE_DOWN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MOVE_DOWN], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right - ScaleGui(72) - 2, rcMain.bottom - (2 * GuiSettingManager::m_iEditHeight) - 5, ScaleGui(72), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)BTN_MOVE_DOWN, ServerManager::m_hInstance, nullptr);

    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false || ServerManager::m_bServerRunning == false) {
            dwStyle |= WS_DISABLED;
        }

		m_hWndPageItems[BTN_RESTART_SCRIPTS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_RESTART_SCRIPTS], dwStyle,
            rcMain.right - ScaleGui(145) - 4, rcMain.bottom - GuiSettingManager::m_iEditHeight - 2, ScaleGui(145) + 2, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)BTN_RESTART_SCRIPTS, ServerManager::m_hInstance, nullptr);
    }

    for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
        if(m_hWndPageItems[ui8i] == nullptr) {
            return false;
        }

        ::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
    }

    SetSplitterRect(&rcMain);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_SCRIPT_NAMES];
    lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT_FILE];
    lvColumn.iSubItem = 0;

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

    lvColumn.fmt = LVCFMT_RIGHT;
    lvColumn.cx = GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_SCRIPT_MEMORY_USAGES];
    lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[LAN_MEM_USAGE];
    lvColumn.iSubItem = 1;
    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);

	AddScriptsToList(false);

    GuiSettingManager::m_wpOldMultiRichEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[REDT_SCRIPTS_ERRORS], GWLP_WNDPROC, (LONG_PTR)MultiRichEditProc);

    ::SetWindowLongPtr(m_hWndPageItems[LV_SCRIPTS], GWLP_USERDATA, (LONG_PTR)this);
    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[LV_SCRIPTS], GWLP_WNDPROC, (LONG_PTR)ScriptsProc);

    ::SetWindowLongPtr(m_hWndPageItems[BTN_MOVE_UP], GWLP_USERDATA, (LONG_PTR)this);
    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_MOVE_UP], GWLP_WNDPROC, (LONG_PTR)MoveUpProc);

    ::SetWindowLongPtr(m_hWndPageItems[BTN_MOVE_DOWN], GWLP_USERDATA, (LONG_PTR)this);
    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_MOVE_DOWN], GWLP_WNDPROC, (LONG_PTR)MoveDownProc);

    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_RESTART_SCRIPTS], GWLP_WNDPROC, (LONG_PTR)LastButtonProc);

	return true;
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateLanguage() {
    ::SetWindowText(m_hWndPageItems[GB_SCRIPTS_ERRORS], LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTS_ERRORS]);
    ::SetWindowText(m_hWndPageItems[BTN_OPEN_SCRIPT_EDITOR], LanguageManager::m_Ptr->m_sTexts[LAN_OPEN_SCRIPT_EDITOR]);
    ::SetWindowText(m_hWndPageItems[BTN_REFRESH_SCRIPTS], LanguageManager::m_Ptr->m_sTexts[LAN_REFRESH_SCRIPTS]);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_TEXT;
    lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT_FILE];

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_SETCOLUMN, 0, (LPARAM)&lvColumn);

    lvColumn.pszText = LanguageManager::m_Ptr->m_sTexts[LAN_MEM_USAGE];

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_SETCOLUMN, 1, (LPARAM)&lvColumn);

    ::SetWindowText(m_hWndPageItems[BTN_MOVE_UP], LanguageManager::m_Ptr->m_sTexts[LAN_MOVE_UP]);
    ::SetWindowText(m_hWndPageItems[BTN_MOVE_DOWN], LanguageManager::m_Ptr->m_sTexts[LAN_MOVE_DOWN]);
    ::SetWindowText(m_hWndPageItems[BTN_RESTART_SCRIPTS], LanguageManager::m_Ptr->m_sTexts[LAN_RESTART_SCRIPTS]);
}
//---------------------------------------------------------------------------

char * MainWindowPageScripts::GetPageName() {
    return LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTS];
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OnContextMenu(HWND hWindow, LPARAM lParam) {
    if(hWindow == m_hWndPageItems[REDT_SCRIPTS_ERRORS]) {
        RichEditPopupMenu(m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS], m_hWnd, lParam);
        return;
    }

    if(hWindow != m_hWndPageItems[LV_SCRIPTS]) {
        return;
    }

    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    HMENU hMenu = ::CreatePopupMenu();

    ::AppendMenu(hMenu, MF_STRING, IDC_OPEN_IN_EXT_EDITOR, LanguageManager::m_Ptr->m_sTexts[LAN_OPEN_EXT_EDIT]);
    ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenu(hMenu, MF_STRING, IDC_OPEN_IN_SCRIPT_EDITOR, LanguageManager::m_Ptr->m_sTexts[LAN_OPEN_IN_SCRIPT_EDITOR]);
    ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenu(hMenu, MF_STRING, IDC_DELETE_SCRIPT, LanguageManager::m_Ptr->m_sTexts[LAN_DELETE_SCRIPT]);

    ::SetMenuDefaultItem(hMenu, IDC_OPEN_IN_SCRIPT_EDITOR, FALSE);

    int iX = GET_X_LPARAM(lParam);
    int iY = GET_Y_LPARAM(lParam);

    ListViewGetMenuPos(m_hWndPageItems[LV_SCRIPTS], iX, iY);

    ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWnd, nullptr);

    ::DestroyMenu(hMenu);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OpenScriptEditor(char * sScript/* = nullptr*/) {
    ScriptEditorDialog * pScriptEditorDialog = new (std::nothrow) ScriptEditorDialog();

    if(pScriptEditorDialog != nullptr) {
        pScriptEditorDialog->DoModal(MainWindow::m_Ptr->m_hWnd);

        if(sScript != nullptr) {
            pScriptEditorDialog->LoadScript(sScript);
        }
    }
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::RefreshScripts() {
    ScriptManager::m_Ptr->CheckForDeletedScripts();
	ScriptManager::m_Ptr->CheckForNewScripts();

	AddScriptsToList(true);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::AddScriptsToList(const bool bDelete) {
    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    if(bDelete == true) {
        ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_DELETEALLITEMS, 0, 0);
    }

	for(uint8_t ui8i = 0; ui8i < ScriptManager::m_Ptr->m_ui8ScriptCount; ui8i++) {
        ScriptToList(ui8i, true, false);
	}

    ListViewSelectFirstItem(m_hWndPageItems[LV_SCRIPTS]);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::ScriptToList(const uint8_t ui8ScriptId, const bool bInsert, const bool bSelected) {
    m_bIgnoreItemChanged = true;

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = ui8ScriptId;
    lvItem.pszText = ScriptManager::m_Ptr->m_ppScriptTable[ui8ScriptId]->m_sName;
    lvItem.lParam = (LPARAM)ScriptManager::m_Ptr->m_ppScriptTable[ui8ScriptId];

    if(bSelected == true) {
        lvItem.mask |= LVIF_STATE;
        lvItem.state = LVIS_SELECTED;
        lvItem.stateMask = LVIS_SELECTED;
    }

    int i = -1;
    
    if(bInsert == true) {
        i = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
    } else {
        i = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
    }

    if(i != -1 || bInsert == false) {
        lvItem.mask = LVIF_STATE;
        lvItem.state = INDEXTOSTATEIMAGEMASK(ScriptManager::m_Ptr->m_ppScriptTable[ui8ScriptId]->m_bEnabled == true ? 2 : 1);
        lvItem.stateMask = LVIS_STATEIMAGEMASK;

        ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_SETITEMSTATE, ui8ScriptId, (LPARAM)&lvItem);
    }

	m_bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OnItemChanged(const LPNMLISTVIEW pListView) {
    UpdateUpDown();

    if(m_bIgnoreItemChanged == true || pListView->iItem == -1 || (pListView->uNewState & LVIS_STATEIMAGEMASK) == (pListView->uOldState & LVIS_STATEIMAGEMASK)) {
        return;
    }

    if((((pListView->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) == 0) {
        if(ScriptManager::m_Ptr->m_ppScriptTable[pListView->iItem]->m_bEnabled == false) {
            return;
        }

        ScriptManager::m_Ptr->m_ppScriptTable[pListView->iItem]->m_bEnabled = false;

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false || ServerManager::m_bServerRunning == false) {
			return;
        }

		ScriptManager::m_Ptr->StopScript(ScriptManager::m_Ptr->m_ppScriptTable[pListView->iItem], false);
		ClearMemUsage((uint8_t)pListView->iItem);

		RichEditAppendText(m_hWndPageItems[REDT_SCRIPTS_ERRORS], (string(LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT_STOPPED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SCRIPT_STOPPED])+".").c_str());
    } else {
        if(ScriptManager::m_Ptr->m_ppScriptTable[pListView->iItem]->m_bEnabled == true) {
            return;
        }

        ScriptManager::m_Ptr->m_ppScriptTable[pListView->iItem]->m_bEnabled = true;

		if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false || ServerManager::m_bServerRunning == false) {
            return;
        }

		if(ScriptManager::m_Ptr->StartScript(ScriptManager::m_Ptr->m_ppScriptTable[pListView->iItem], false) == true) {
			RichEditAppendText(m_hWndPageItems[REDT_SCRIPTS_ERRORS], (string(LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT_STARTED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SCRIPT_STARTED])+".").c_str());
		}
    }
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OnDoubleClick(const LPNMITEMACTIVATE pItemActivate) {
    RECT rc = { LVIR_ICON, 0, 0, 0 };

    if(::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETITEMRECT, pItemActivate->iItem, (LPARAM)&rc) == FALSE || pItemActivate->ptAction.x > rc.left) {
        string sScript = ServerManager::m_sScriptPath + ScriptManager::m_Ptr->m_ppScriptTable[pItemActivate->iItem]->m_sName;
        OpenScriptEditor(sScript.c_str());
    }
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::ClearMemUsageAll() {
	for(uint8_t ui8i = 0; ui8i < ScriptManager::m_Ptr->m_ui8ScriptCount; ui8i++) {
        ClearMemUsage(ui8i);
	}
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateMemUsage() {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;
    lvItem.iSubItem = 1;

	for(uint8_t ui8i = 0; ui8i < ScriptManager::m_Ptr->m_ui8ScriptCount; ui8i++) {
        if(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_bEnabled == false) {
            continue;
        }

        string tmp(lua_gc(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_pLua, LUA_GCCOUNT, 0));

        lvItem.iItem = ui8i;

        string sMemUsage(lua_gc(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_pLua, LUA_GCCOUNT, 0));
        lvItem.pszText = sMemUsage.c_str();

        ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::MoveUp() {
    HWND hWndFocus = ::GetFocus();

    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

	ScriptManager::m_Ptr->MoveScript((uint8_t)iSel, true);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ScriptToList((uint8_t)iSel, false, false);

    iSel--;

    ScriptToList((uint8_t)iSel, false, true);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, iSel, FALSE);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();

    if(hWndFocus == m_hWndPageItems[BTN_MOVE_UP]) {
        if(::IsWindowEnabled(m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_UP])) {
            ::SetFocus(m_hWndPageItems[BTN_MOVE_UP]);
        } else {
            ::SetFocus(m_hWndPageItems[BTN_MOVE_DOWN]);
        }
    }
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::MoveDown() {
    HWND hWndFocus = ::GetFocus();

    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

	ScriptManager::m_Ptr->MoveScript((uint8_t)iSel, false);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ScriptToList((uint8_t)iSel, false, false);

    iSel++;

    ScriptToList((uint8_t)iSel, false, true);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, iSel, FALSE);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();

    if(hWndFocus == m_hWndPageItems[BTN_MOVE_DOWN]) {
        if(::IsWindowEnabled(m_hWndPageItems[MainWindowPageScripts::BTN_MOVE_DOWN])) {
            ::SetFocus(m_hWndPageItems[BTN_MOVE_DOWN]);
        } else {
            ::SetFocus(m_hWndPageItems[BTN_MOVE_UP]);
        }
    }
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::RestartScripts() {
    ScriptManager::m_Ptr->Restart();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateUpDown() {
    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
		::EnableWindow(m_hWndPageItems[BTN_MOVE_UP], FALSE);
		::EnableWindow(m_hWndPageItems[BTN_MOVE_DOWN], FALSE);
	} else if(iSel == 0) {
		::EnableWindow(m_hWndPageItems[BTN_MOVE_UP], FALSE);
		if(iSel == (ScriptManager::m_Ptr->m_ui8ScriptCount-1)) {
			::EnableWindow(m_hWndPageItems[BTN_MOVE_DOWN], FALSE);
		} else {
            ::EnableWindow(m_hWndPageItems[BTN_MOVE_DOWN], TRUE);
		}
	} else if(iSel == (ScriptManager::m_Ptr->m_ui8ScriptCount-1)) {
		::EnableWindow(m_hWndPageItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(m_hWndPageItems[BTN_MOVE_DOWN], FALSE);
	} else {
		::EnableWindow(m_hWndPageItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(m_hWndPageItems[BTN_MOVE_DOWN], TRUE);
	}
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OpenInExternalEditor() {
    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    ::ShellExecute(nullptr, nullptr, (ServerManager::m_sScriptPath + ScriptManager::m_Ptr->m_ppScriptTable[iSel]->m_sName).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OpenInScriptEditor() {
    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    string sScript = ServerManager::m_sScriptPath + ScriptManager::m_Ptr->m_ppScriptTable[iSel]->m_sName;
    OpenScriptEditor(sScript.c_str());
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::DeleteScript() {
    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    if(::MessageBox(m_hWnd, (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
		return;
	}

	ScriptManager::m_Ptr->DeleteScript((uint8_t)iSel);
	::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_DELETEITEM, iSel, 0);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::MoveScript(uint8_t ui8ScriptId, const bool bUp) {
    int iSel = (int)::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ScriptToList(ui8ScriptId, false, (iSel != -1 && ((bUp == true && iSel == (int)ui8ScriptId-1) || (bUp == false && iSel == (int)ui8ScriptId+1))) ? true : false);

    if(bUp == true) {
        ui8ScriptId--;
    } else {
        ui8ScriptId++;
    }

    ScriptToList(ui8ScriptId, false, (iSel != -1 && iSel == (int)ui8ScriptId) ? true : false);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, ui8ScriptId, FALSE);

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateCheck(const uint8_t ui8ScriptId) {
	m_bIgnoreItemChanged = true;

    ListView_SetItemState(m_hWndPageItems[LV_SCRIPTS], ui8ScriptId, INDEXTOSTATEIMAGEMASK(ScriptManager::m_Ptr->m_ppScriptTable[ui8ScriptId]->m_bEnabled == true ? 2 : 1), LVIS_STATEIMAGEMASK);

    if(ScriptManager::m_Ptr->m_ppScriptTable[ui8ScriptId]->m_bEnabled == false) {
        ClearMemUsage(ui8ScriptId);
    }

	m_bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::ClearMemUsage(const uint8_t ui8ScriptId) {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = ui8ScriptId;
    lvItem.iSubItem = 1;
    lvItem.pszText = "";

    ::SendMessage(m_hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::FocusFirstItem() {
    ::SetFocus(m_hWndPageItems[REDT_SCRIPTS_ERRORS]);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::FocusLastItem() {
    if(::IsWindowEnabled(m_hWndPageItems[BTN_RESTART_SCRIPTS])) {
        ::SetFocus(m_hWndPageItems[BTN_RESTART_SCRIPTS]);
    } else if(::IsWindowEnabled(m_hWndPageItems[BTN_MOVE_DOWN])) {
        ::SetFocus(m_hWndPageItems[BTN_MOVE_DOWN]);
    } else if(::IsWindowEnabled(m_hWndPageItems[BTN_MOVE_UP])) {
        ::SetFocus(m_hWndPageItems[BTN_MOVE_UP]);
    } else if(::IsWindowEnabled(m_hWndPageItems[LV_SCRIPTS])) {
        ::SetFocus(m_hWndPageItems[LV_SCRIPTS]);
    }
}
//------------------------------------------------------------------------------

HWND MainWindowPageScripts::GetWindowHandle() {
    return m_hWnd;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateSplitterParts() {
    ::SetWindowPos(m_hWndPageItems[BTN_RESTART_SCRIPTS], nullptr, m_iSplitterPos + 2, m_rcSplitter.bottom - GuiSettingManager::m_iEditHeight - 2, m_rcSplitter.right - (m_iSplitterPos + 4), GuiSettingManager::m_iEditHeight,
        SWP_NOZORDER);

    int iButtonWidth = (m_rcSplitter.right - (m_iSplitterPos + 7)) / 2;
    ::SetWindowPos(m_hWndPageItems[BTN_MOVE_DOWN], nullptr, m_rcSplitter.right - iButtonWidth - 2, m_rcSplitter.bottom - (2 * GuiSettingManager::m_iEditHeight) - 5, iButtonWidth, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
    ::SetWindowPos(m_hWndPageItems[BTN_MOVE_UP], nullptr, m_iSplitterPos + 2, m_rcSplitter.bottom - (2 * GuiSettingManager::m_iEditHeight) - 5, iButtonWidth, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);

    ::SetWindowPos(m_hWndPageItems[LV_SCRIPTS], nullptr, m_iSplitterPos + 2, (2 * GuiSettingManager::m_iEditHeight) + 8, m_rcSplitter.right - (m_iSplitterPos + 4),
		m_rcSplitter.bottom - ((2 * GuiSettingManager::m_iEditHeight) + 8) - ((2 * GuiSettingManager::m_iEditHeight) - 5) - 14, SWP_NOZORDER);
    ::SetWindowPos(m_hWndPageItems[BTN_REFRESH_SCRIPTS], nullptr, m_iSplitterPos + 2, GuiSettingManager::m_iEditHeight + 4, m_rcSplitter.right - (m_iSplitterPos + 4), GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
    ::SetWindowPos(m_hWndPageItems[BTN_OPEN_SCRIPT_EDITOR], nullptr, m_iSplitterPos + 2, 1, m_rcSplitter.right - (m_iSplitterPos + 4), GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
    ::SetWindowPos(m_hWndPageItems[REDT_SCRIPTS_ERRORS], nullptr, 0, 0, m_iSplitterPos - 19, m_rcSplitter.bottom - (GuiSettingManager::m_iGroupBoxMargin + 11), SWP_NOMOVE | SWP_NOZORDER);
    ::SendMessage(m_hWndPageItems[REDT_SCRIPTS_ERRORS], WM_VSCROLL, SB_BOTTOM, 0);
    ::SetWindowPos(m_hWndPageItems[GB_SCRIPTS_ERRORS], nullptr, 0, 0, m_iSplitterPos - 3, m_rcSplitter.bottom - 3, SWP_NOMOVE | SWP_NOZORDER);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
