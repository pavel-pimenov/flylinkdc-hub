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
MainWindowPageUsersChat * MainWindowPageUsersChat::m_Ptr = nullptr;
//---------------------------------------------------------------------------
static WNDPROC wpOldMultiEditProc = nullptr;
//---------------------------------------------------------------------------

static LRESULT CALLBACK MultiRichEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return 0;
    } else if(uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        return 0;
    }

    return ::CallWindowProc(GuiSettingManager::m_wpOldMultiRichEditProc, hWnd, uMsg, wParam, lParam);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

MainWindowPageUsersChat::MainWindowPageUsersChat() {
    MainWindowPageUsersChat::m_Ptr = this;

    memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));

	m_iPercentagePos = GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_USERS_CHAT_SPLITTER];
}
//---------------------------------------------------------------------------

MainWindowPageUsersChat::~MainWindowPageUsersChat() {
    MainWindowPageUsersChat::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT MainWindowPageUsersChat::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_SETFOCUS:
            ::SetFocus(m_hWndPageItems[BTN_SHOW_CHAT]);

            return 0;
        case WM_WINDOWPOSCHANGED: {
            RECT rcMain = { 0, GuiSettingManager::m_iCheckHeight, ((WINDOWPOS*)lParam)->cx, ((WINDOWPOS*)lParam)->cy };

            ::SetWindowPos(m_hWndPageItems[BTN_SHOW_CHAT], nullptr, 0, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, GuiSettingManager::m_iCheckHeight, SWP_NOMOVE | SWP_NOZORDER);
            ::SetWindowPos(m_hWndPageItems[BTN_SHOW_COMMANDS], nullptr, ((rcMain.right - ScaleGui(150)) / 2) + 1, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, GuiSettingManager::m_iCheckHeight, SWP_NOZORDER);
            ::SetWindowPos(m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST], nullptr, rcMain.right - (ScaleGui(150) - 4), 0, (rcMain.right - (rcMain.right - (ScaleGui(150) - 2))) - 4, GuiSettingManager::m_iCheckHeight,
                SWP_NOZORDER);

            SetSplitterRect(&rcMain);

            return 0;
        }
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == m_hWndPageItems[REDT_CHAT] && ((LPNMHDR)lParam)->code == EN_LINK) {
                if(((ENLINK *)lParam)->msg == WM_LBUTTONUP) {
                    RichEditOpenLink(m_hWndPageItems[REDT_CHAT], (ENLINK *)lParam);
                }
            } else if(((LPNMHDR)lParam)->hwndFrom == m_hWndPageItems[LV_USERS] && ((LPNMHDR)lParam)->code == LVN_GETINFOTIP) {
                NMLVGETINFOTIP * pGetInfoTip = (LPNMLVGETINFOTIP)lParam;

                char msg[1024];

                LVITEM lvItem = { 0 };
                lvItem.mask = LVIF_PARAM | LVIF_TEXT;
                lvItem.iItem = pGetInfoTip->iItem;
                lvItem.pszText = msg;
                lvItem.cchTextMax = 1024;

                if((BOOL)::SendMessage(m_hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
                    return 0;
                }

                User * curUser = reinterpret_cast<User *>(lvItem.lParam);

                if(::SendMessage(m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_UNCHECKED) {
                    User * testUser = HashManager::m_Ptr->FindUser(lvItem.pszText, strlen(lvItem.pszText));

                    if(testUser == nullptr || testUser != curUser) {
                        return 0;
                    }
                }

                px_string sInfoTip = px_string(LanguageManager::m_Ptr->m_sTexts[LAN_NICK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NICK]) + ": " + px_string(curUser->m_sNick, curUser->m_ui8NickLen) +
                    "\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_IP], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_IP]) + ": " + px_string(curUser->m_sIP);

                sInfoTip += "\n\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_CLIENT], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CLIENT]) + ": " +
                    px_string(curUser->m_sClient, (size_t)curUser->m_ui8ClientLen) +
                    "\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_VERSION], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_VERSION]) + ": " + px_string(curUser->m_sTagVersion, (size_t)curUser->m_ui8TagVersionLen);

                sInfoTip += "\n\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_MODE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_MODE]) + ": " + px_string(curUser->m_sModes) +
                    "\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_SLOTS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SLOTS]) + ": " + px_string(curUser->m_ui32Slots) +
                    "\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_HUBS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_HUBS]) + ": " + px_string(curUser->m_ui32Hubs);

                if(curUser->m_ui32OLimit != 0) {
                    sInfoTip += "\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_AUTO_OPEN_SLOT_WHEN_UP_UNDER], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_AUTO_OPEN_SLOT_WHEN_UP_UNDER]) + " " +
                        px_string(curUser->m_ui32OLimit)+" kB/s";
                }

                if(curUser->m_ui32DLimit != 0) {
                    sInfoTip += "\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_LIMITER], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_LIMITER])+ " D:" + px_string(curUser->m_ui32DLimit) + " kB/s";
                }

                if(curUser->m_ui32LLimit != 0) {
                    sInfoTip += "\n" + px_string(LanguageManager::m_Ptr->m_sTexts[LAN_LIMITER], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_LIMITER])+ " L:" + px_string(curUser->m_ui32LLimit) + " kB/s";
                }

                sInfoTip += "\n\nRecvBuf: " + px_string(curUser->m_ui32RecvBufLen) + " bytes";
                sInfoTip += "\nSendBuf: " + px_string(curUser->m_ui32SendBufLen) + " bytes";

                pGetInfoTip->cchTextMax = (int)(sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size());
                memcpy(pGetInfoTip->pszText, sInfoTip.c_str(), sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size());
                pGetInfoTip->pszText[(sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size()) - 1] = '\0';

                return 0;
            }
            break;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case BTN_AUTO_UPDATE_USERLIST:
                    if(HIWORD(wParam) == BN_CLICKED && ServerManager::m_bServerRunning == true) {
                        bool bChecked = ::SendMessage(m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
                        ::EnableWindow(m_hWndPageItems[BTN_UPDATE_USERS], bChecked == true ? FALSE : TRUE);
                        if(bChecked == true) {
                            UpdateUserList();
                        }

                        return 0;
                    }

                    break;
                case BTN_UPDATE_USERS:
                    UpdateUserList();
                    return 0;
                case EDT_CHAT:
                    if(HIWORD(wParam) == EN_CHANGE) {
                        int iLen = ::GetWindowTextLength((HWND)lParam);

                        char * buf = (char *)malloc(iLen+1);

                        if(buf == nullptr) {
                            AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in MainWindowPageUsersChat::MainWindowPageProc\n", iLen+1);
                            return 0;
                        }

                        ::GetWindowText((HWND)lParam, buf, iLen+1);

                        bool bChanged = false;

                        for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                            if(buf[ui16i] == '|') {
                                strcpy(buf+ui16i, buf+ui16i+1);
                                bChanged = true;
                                ui16i--;
                            }
                        }

                        if(bChanged == true) {
                            int iStart, iEnd;

                            ::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

                            ::SetWindowText((HWND)lParam, buf);

                            ::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
                        }

                        free(buf);

                        return 0;
                    }

                    break;
                case IDC_REG_USER: {
                    int iSel = (int)::SendMessage(m_hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

                    if(iSel == -1) {
                        return 0;
                    }

                    char sNick[65];

                    LVITEM lvItem = { 0 };
                    lvItem.mask = LVIF_TEXT;
                    lvItem.iItem = iSel;
                    lvItem.iSubItem = 0;
                    lvItem.pszText = sNick;
                    lvItem.cchTextMax = 65;

                    ::SendMessage(m_hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem);

                    RegisteredUserDialog::m_Ptr = new (std::nothrow) RegisteredUserDialog();

                    if(RegisteredUserDialog::m_Ptr != nullptr) {
                        RegisteredUserDialog::m_Ptr->DoModal(MainWindow::m_Ptr->m_hWnd, nullptr, sNick);
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

            if(RichEditCheckMenuCommands(m_hWndPageItems[REDT_CHAT], LOWORD(wParam)) == true) {
                return 0;
            }

            break;
        case WM_CONTEXTMENU:
            OnContextMenu((HWND)wParam, lParam);
            break;
        case WM_DESTROY:
            GuiSettingManager::m_Ptr->SetBool(GUISETBOOL_SHOW_CHAT, ::SendMessage(m_hWndPageItems[BTN_SHOW_CHAT], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
            GuiSettingManager::m_Ptr->SetBool(GUISETBOOL_SHOW_COMMANDS, ::SendMessage(m_hWndPageItems[BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
            GuiSettingManager::m_Ptr->SetBool(GUISETBOOL_AUTO_UPDATE_USERLIST, ::SendMessage(m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

            GuiSettingManager::m_Ptr->SetInteger(GUISETINT_USERS_CHAT_SPLITTER, m_iPercentagePos);

            break;
    }

    if(BasicSplitterProc(uMsg, wParam, lParam) == true) {
    	return 0;
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

static LRESULT CALLBACK MultiEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_KEYDOWN) {
        if(wParam == VK_RETURN && (::GetKeyState(VK_CONTROL) & 0x8000) == 0) {
            MainWindowPageUsersChat * pParent = (MainWindowPageUsersChat *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if(pParent != nullptr) {
                if(pParent->OnEditEnter() == true) {
                    return 0;
                }
            }
        } else if(wParam == '|') {
            return 0;
        }
    } else if(uMsg == WM_CHAR || uMsg == WM_KEYUP) {
        if(wParam == VK_RETURN && (::GetKeyState(VK_CONTROL) & 0x8000) == 0) {
            return 0;
        } else if(wParam == '|') {
            return 0;
        }
    } if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return 0;
    }

    return ::CallWindowProc(wpOldMultiEditProc, hWnd, uMsg, wParam, lParam);
}

//------------------------------------------------------------------------------

static LRESULT CALLBACK ListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return DLGC_WANTTAB;
    } else if(uMsg == WM_CHAR && wParam == VK_TAB) {
        if((::GetKeyState(VK_SHIFT) & 0x8000) == 0) {
            MainWindowPageUsersChat * pParent = (MainWindowPageUsersChat *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if(pParent != nullptr && ::IsWindowEnabled(pParent->m_hWndPageItems[MainWindowPageUsersChat::BTN_UPDATE_USERS])) {
                ::SetFocus(pParent->m_hWndPageItems[MainWindowPageUsersChat::BTN_UPDATE_USERS]);
            } else {
                ::SetFocus(MainWindow::m_Ptr->m_hWndWindowItems[MainWindow::TC_TABS]);
            }

            return 0;
        } else {
			::SetFocus(::GetNextDlgTabItem(MainWindow::m_Ptr->m_hWnd, hWnd, TRUE));
            return 0;
        }
    }

    return ::CallWindowProc(GuiSettingManager::m_wpOldListViewProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

bool MainWindowPageUsersChat::CreateMainWindowPage(HWND hOwner) {
    CreateHWND(hOwner);

    RECT rcMain;
    ::GetClientRect(m_hWnd, &rcMain);

	m_hWndPageItems[BTN_SHOW_CHAT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CHAT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        2, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_SHOW_COMMANDS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CMDS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        ((rcMain.right - ScaleGui(150)) / 2) + 1, 0, ((rcMain.right - ScaleGui(150)) / 2) - 3, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[REDT_CHAT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
        2, GuiSettingManager::m_iCheckHeight, rcMain.right - ScaleGui(150), rcMain.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight - 4, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[REDT_CHAT], EM_EXLIMITTEXT, 0, (LPARAM)262144);
    ::SendMessage(m_hWndPageItems[REDT_CHAT], EM_AUTOURLDETECT, TRUE, 0);
    ::SendMessage(m_hWndPageItems[REDT_CHAT], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(m_hWndPageItems[REDT_CHAT], EM_GETEVENTMASK, 0, 0) | ENM_LINK);

	m_hWndPageItems[EDT_CHAT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL | ES_MULTILINE,
        2, rcMain.bottom - GuiSettingManager::m_iEditHeight - 2, rcMain.right - ScaleGui(150), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_CHAT, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_CHAT], EM_SETLIMITTEXT, 8192, 0);

	m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_AUTO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        rcMain.right - ScaleGui(150) - 4, 0, (rcMain.right - (rcMain.right - ScaleGui(150) - 2)) - 4, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_AUTO_UPDATE_USERLIST, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LV_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL |
        LVS_SORTASCENDING, rcMain.right - ScaleGui(150) - 4, GuiSettingManager::m_iCheckHeight, (rcMain.right - (rcMain.right - ScaleGui(150) - 2)) - 4, rcMain.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight - 4,
        m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[LV_USERS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	m_hWndPageItems[BTN_UPDATE_USERS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REFRESH_USERLIST], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right - ScaleGui(150) - 3, rcMain.bottom - GuiSettingManager::m_iEditHeight - 2, (rcMain.right - (rcMain.right - ScaleGui(150) - 2)) - 2, GuiSettingManager::m_iEditHeight,
         m_hWnd, (HMENU)BTN_UPDATE_USERS, ServerManager::m_hInstance, nullptr);

    for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
        if(m_hWndPageItems[ui8i] == nullptr) {
            return false;
        }

        ::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
    }

    rcMain.top = GuiSettingManager::m_iCheckHeight;
    SetSplitterRect(&rcMain);

	RECT rcUsers;
	::GetClientRect(m_hWndPageItems[LV_USERS], &rcUsers);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = (rcUsers.right - rcUsers.left) - 5;

    ::SendMessage(m_hWndPageItems[LV_USERS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

    ::SendMessage(m_hWndPageItems[BTN_SHOW_CHAT], BM_SETCHECK, GuiSettingManager::m_Ptr->m_bBools[GUISETBOOL_SHOW_CHAT] == true ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessage(m_hWndPageItems[BTN_SHOW_COMMANDS], BM_SETCHECK, GuiSettingManager::m_Ptr->m_bBools[GUISETBOOL_SHOW_COMMANDS] == true ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessage(m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_SETCHECK, GuiSettingManager::m_Ptr->m_bBools[GUISETBOOL_AUTO_UPDATE_USERLIST] == true ? BST_CHECKED : BST_UNCHECKED, 0);

    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_SHOW_CHAT], GWLP_WNDPROC, (LONG_PTR)FirstButtonProc);

    GuiSettingManager::m_wpOldMultiRichEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[REDT_CHAT], GWLP_WNDPROC, (LONG_PTR)MultiRichEditProc);

    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_UPDATE_USERS], GWLP_WNDPROC, (LONG_PTR)LastButtonProc);

    ::SetWindowLongPtr(m_hWndPageItems[EDT_CHAT], GWLP_USERDATA, (LONG_PTR)this);
    wpOldMultiEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_CHAT], GWLP_WNDPROC, (LONG_PTR)MultiEditProc);

    ::SetWindowLongPtr(m_hWndPageItems[LV_USERS], GWLP_USERDATA, (LONG_PTR)this);
    GuiSettingManager::m_wpOldListViewProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[LV_USERS], GWLP_WNDPROC, (LONG_PTR)ListProc);

	return true;
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::UpdateLanguage() {
    ::SetWindowText(m_hWndPageItems[BTN_SHOW_CHAT], LanguageManager::m_Ptr->m_sTexts[LAN_CHAT]);
    ::SetWindowText(m_hWndPageItems[BTN_SHOW_COMMANDS], LanguageManager::m_Ptr->m_sTexts[LAN_CMDS]);
    ::SetWindowText(m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST], LanguageManager::m_Ptr->m_sTexts[LAN_AUTO]);
    ::SetWindowText(m_hWndPageItems[BTN_UPDATE_USERS], LanguageManager::m_Ptr->m_sTexts[LAN_REFRESH_USERLIST]);
}
//---------------------------------------------------------------------------

char * MainWindowPageUsersChat::GetPageName() {
    return LanguageManager::m_Ptr->m_sTexts[LAN_USERS_CHAT];
}
//------------------------------------------------------------------------------

bool MainWindowPageUsersChat::OnEditEnter() {
    if(ServerManager::m_bServerRunning == false) {
        return false;
    }

    int iAllocLen = ::GetWindowTextLength(m_hWndPageItems[EDT_CHAT]);

    if(iAllocLen == 0) {
        return false;
    }

    char * buf = (char *)malloc(iAllocLen+4+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]);

    if(buf == nullptr) {
        AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in MainWindowPageUsersChat::OnEditEnter\n", iAllocLen+4+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]);
        return false;
    }

    buf[0] = '<';
    memcpy(buf+1, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK], SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]);
    buf[SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]+1] = '>';
    buf[SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]+2] = ' ';
    ::GetWindowText(m_hWndPageItems[EDT_CHAT], buf+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]+3, iAllocLen+1);
    buf[iAllocLen+3+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]] = '|';

    GlobalDataQueue::m_Ptr->AddQueueItem(buf, iAllocLen+4+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK], nullptr, 0, GlobalDataQueue::CMD_CHAT);

    buf[iAllocLen+3+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_ADMIN_NICK]] = '\0';

    RichEditAppendText(m_hWndPageItems[REDT_CHAT], buf);

    free(buf);

    ::SetWindowText(m_hWndPageItems[EDT_CHAT], "");

    return true;
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::AddUser(const User * pUser) {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = 0;
    lvItem.lParam = (LPARAM)pUser;
    lvItem.pszText = pUser->m_sNick;

    ::SendMessage(m_hWndPageItems[LV_USERS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::RemoveUser(const User * pUser) {
    LVFINDINFO lvFindItem = { 0 };
    lvFindItem.flags = LVFI_PARAM;
    lvFindItem.lParam = (LPARAM)pUser;

    int iItem = (int)::SendMessage(m_hWndPageItems[LV_USERS], LVM_FINDITEM, (WPARAM)-1, (LPARAM)&lvFindItem);

    if(iItem != -1) {
        ::SendMessage(m_hWndPageItems[LV_USERS], LVM_DELETEITEM, iItem, 0);
    }
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::UpdateUserList() {
    ::SendMessage(m_hWndPageItems[LV_USERS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ::SendMessage(m_hWndPageItems[LV_USERS], LVM_DELETEALLITEMS, 0, 0);

    uint32_t ui32InClose = 0, ui32InLogin = 0, ui32LoggedIn = 0, ui32Total = 0;

	User * pCur = nullptr,
     * pNext = Users::m_Ptr->m_pUserListS;

	while(pNext != nullptr) {
        ui32Total++;

        pCur = pNext;
        pNext = pCur->m_pNext;

        switch(pCur->m_ui8State) {
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

    ListViewSelectFirstItem(m_hWndPageItems[LV_USERS]);

    ::SendMessage(m_hWndPageItems[LV_USERS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    RichEditAppendText(m_hWndPageItems[REDT_CHAT], (px_string(LanguageManager::m_Ptr->m_sTexts[LAN_TOTAL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_TOTAL]) + ": " + px_string(ui32Total) + ", " +
        px_string(LanguageManager::m_Ptr->m_sTexts[LAN_LOGGED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_LOGGED]) + ": " + px_string(ui32LoggedIn) + ", " +
        px_string(LanguageManager::m_Ptr->m_sTexts[LAN_CLOSING], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CLOSING]) + ": " + px_string(ui32InClose) + ", " +
        px_string(LanguageManager::m_Ptr->m_sTexts[LAN_LOGGING], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_LOGGING]) + ": " + px_string(ui32InLogin)).c_str());
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::OnContextMenu(HWND hWindow, LPARAM lParam) {
    if(hWindow == m_hWndPageItems[REDT_CHAT]) {
        RichEditPopupMenu(m_hWndPageItems[REDT_CHAT], m_hWnd, lParam);
    } else if(hWindow == m_hWndPageItems[LV_USERS]) {
        int iSel = (int)::SendMessage(m_hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

        if(iSel == -1) {
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

        ::SendMessage(m_hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem);

        if(RegManager::m_Ptr->Find(sNick, strlen(sNick)) == nullptr) {
            ::AppendMenu(hMenu, MF_STRING, IDC_REG_USER, LanguageManager::m_Ptr->m_sTexts[LAN_MENU_REG_USER]);
            ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        }

        ::AppendMenu(hMenu, MF_STRING, IDC_BAN_USER, LanguageManager::m_Ptr->m_sTexts[LAN_MENU_BAN_USER]);
        ::AppendMenu(hMenu, MF_STRING, IDC_KICK_USER, LanguageManager::m_Ptr->m_sTexts[LAN_MENU_KICK_USER]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        ::AppendMenu(hMenu, MF_STRING, IDC_DISCONNECT_USER, LanguageManager::m_Ptr->m_sTexts[LAN_MENU_DISCONNECT_USER]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        ::AppendMenu(hMenu, MF_STRING, IDC_REDIRECT_USER, LanguageManager::m_Ptr->m_sTexts[LAN_MENU_REDIRECT_USER]);

        int iX = GET_X_LPARAM(lParam);
        int iY = GET_Y_LPARAM(lParam);

        ListViewGetMenuPos(m_hWndPageItems[LV_USERS], iX, iY);

        ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWnd, nullptr);

        ::DestroyMenu(hMenu);
    }
}
//------------------------------------------------------------------------------

User * MainWindowPageUsersChat::GetUser() {
    int iSel = (int)::SendMessage(m_hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return nullptr;
    }

    char msg[1024];

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = iSel;
    lvItem.pszText = msg;
    lvItem.cchTextMax = 1024;

    if((BOOL)::SendMessage(m_hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
        return nullptr;
    }

    User * curUser = reinterpret_cast<User *>(lvItem.lParam);

    if(::SendMessage(m_hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_UNCHECKED) {
        User * testUser = HashManager::m_Ptr->FindUser(lvItem.pszText, strlen(lvItem.pszText));

        if(testUser == nullptr || testUser != curUser) {
            char buf[1024];
            int iMsgLen = snprintf(buf, 1024, "<%s> *** %s %s.", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], lvItem.pszText, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_ONLINE]);
            if(iMsgLen > 0) {
                RichEditAppendText(m_hWndPageItems[REDT_CHAT], buf);
            }

            return nullptr;
        }
    }

    return curUser;
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::DisconnectUser() {
    User * curUser = GetUser();

    if(curUser == nullptr) {
        return;
    }

    // disconnect the user
	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) closed by %s", curUser->m_sNick, curUser->m_sIP, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);

    curUser->Close();

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("MainWindowPageUsersChat::DisconnectUser", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], curUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], curUser->m_sIP, 
			LanguageManager::m_Ptr->m_sTexts[LAN_WAS_CLOSED_BY], SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);
    }

	char msg[1024];
    int iMsgLen = snprintf(msg, 1024, "<%s> *** %s %s %s %s.", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], curUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], curUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_CLOSED]);
    if(iMsgLen > 0) {
        RichEditAppendText(m_hWndPageItems[REDT_CHAT], msg);
    }
}
//------------------------------------------------------------------------------

void OnKickOk(char * sLine, const int iLen) {
    User * pUser = MainWindowPageUsersChat::m_Ptr->GetUser();

    if(pUser == nullptr) {
        return;
    }

    BanManager::m_Ptr->TempBan(pUser, iLen == 0 ? nullptr : sLine, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK], 0, 0, false);

    if(iLen == 0) {
        pUser->SendFormat("OnKickOk1", false, "<%s> %s...|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_BEING_KICKED]);
    } else {
        if(iLen > 512) {
            sLine[513] = '\0';
            sLine[512] = '.';
            sLine[511] = '.';
            sLine[510] = '.';
        }

        pUser->SendFormat("OnKickOk2", false, "<%s> %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_BEING_KICKED_BCS], sLine);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("OnKickOk", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_KICKED_BY], 
			SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);
    }

	char msg[1024];
    int iMsgLen = snprintf(msg, 1024, "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_KICKED]);
    if(iMsgLen > 0) {
        RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], msg);
    }

    // disconnect the user
	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", pUser->m_sNick, pUser->m_sIP, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);

    pUser->Close();
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::KickUser() {
    User * curUser = GetUser();

    if(curUser == nullptr) {
        return;
    }

	LineDialog * pKickDlg = new (std::nothrow) LineDialog(&OnKickOk);

	if(pKickDlg != nullptr) {
	   pKickDlg->DoModal(::GetParent(m_hWnd), LanguageManager::m_Ptr->m_sTexts[LAN_PLEASE_ENTER_REASON], "");
    }
}
//------------------------------------------------------------------------------

void OnBanOk(char * sLine, const int iLen) {
    User * pUser = MainWindowPageUsersChat::m_Ptr->GetUser();

    if(pUser == nullptr) {
        return;
    }

    BanManager::m_Ptr->Ban(pUser, iLen == 0 ? nullptr : sLine, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK], false);

    if(iLen == 0) {
        pUser->SendFormat("OnBanOk1", false, "<%s> %s...|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_BEING_KICKED]);
    } else {
        if(iLen > 512) {
            sLine[513] = '\0';
            sLine[512] = '.';
            sLine[511] = '.';
            sLine[510] = '.';
        }

        pUser->SendFormat("OnBanOk2", false, "<%s> %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_BEING_BANNED_BECAUSE], sLine);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("OnBanOk", "<%s> *** %s %s %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);
    }

	char msg[1024];
    int iMsgLen = snprintf(msg, 1024, "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR]);
    if(iMsgLen > 0) {
        RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], msg);
    }

    // disconnect the user
	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", pUser->m_sNick, pUser->m_sIP, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);

    pUser->Close();
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::BanUser() {
    User * curUser = GetUser();

    if(curUser == nullptr) {
        return;
    }

	LineDialog * pBanDlg = new (std::nothrow) LineDialog(&OnBanOk);

	if(pBanDlg != nullptr) {
	   pBanDlg->DoModal(::GetParent(m_hWnd), LanguageManager::m_Ptr->m_sTexts[LAN_PLEASE_ENTER_REASON], "");
    }
}
//------------------------------------------------------------------------------

void OnRedirectOk(char * sLine, const int iLen) {
    User * pUser = MainWindowPageUsersChat::m_Ptr->GetUser();

    if(pUser == nullptr || iLen == 0 || iLen > 512) {
        return;
    }

    pUser->SendFormat("OnRedirectOk", false, "<%s> %s %s %s %s.|$ForceMove %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_REDIRECTED_TO], sLine, LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK], sLine);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("OnRedirectOk", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_REDIRECTED_TO], sLine, LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], 
			SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);
    }

	char msg[2048];
    int iMsgLen = snprintf(msg, 2048, "<%s> *** %s %s %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_REDIRECTED_TO], sLine);
    if(iMsgLen > 0) {
        RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], msg);
    }

    // disconnect the user
	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) redirected by %s", pUser->m_sNick, pUser->m_sIP, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);

    pUser->Close();
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::RedirectUser() {
    User * curUser = GetUser();

    if(curUser == nullptr) {
        return;
    }

	LineDialog * pRedirectDlg = new (std::nothrow) LineDialog(&OnRedirectOk);

	if(pRedirectDlg != nullptr) {
        pRedirectDlg->DoModal(::GetParent(m_hWnd), LanguageManager::m_Ptr->m_sTexts[LAN_PLEASE_ENTER_REDIRECT_ADDRESS],
            SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS] == nullptr ? "" : SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS]);
    }
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::FocusFirstItem() {
    ::SetFocus(m_hWndPageItems[BTN_SHOW_CHAT]);
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::FocusLastItem() {
    if(::IsWindowEnabled(m_hWndPageItems[BTN_UPDATE_USERS])) {
        ::SetFocus(m_hWndPageItems[BTN_UPDATE_USERS]);
    } else {
        ::SetFocus(m_hWndPageItems[LV_USERS]);
    }
}
//------------------------------------------------------------------------------

HWND MainWindowPageUsersChat::GetWindowHandle() {
    return m_hWnd;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

void MainWindowPageUsersChat::UpdateSplitterParts() {
    ::SetWindowPos(m_hWndPageItems[REDT_CHAT], nullptr, 0, 0, m_iSplitterPos - 2, m_rcSplitter.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight - 4, SWP_NOMOVE | SWP_NOZORDER);
    ::SendMessage(m_hWndPageItems[REDT_CHAT], WM_VSCROLL, SB_BOTTOM, 0);
    ::SetWindowPos(m_hWndPageItems[LV_USERS], nullptr, m_iSplitterPos + 2, GuiSettingManager::m_iCheckHeight, m_rcSplitter.right - (m_iSplitterPos + 4), m_rcSplitter.bottom - GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight - 4, SWP_NOZORDER);
    ::SetWindowPos(m_hWndPageItems[EDT_CHAT], nullptr, 2, m_rcSplitter.bottom - GuiSettingManager::m_iEditHeight - 2, m_iSplitterPos - 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
    ::SetWindowPos(m_hWndPageItems[BTN_UPDATE_USERS], nullptr, m_iSplitterPos + 2, m_rcSplitter.bottom - GuiSettingManager::m_iEditHeight - 2, m_rcSplitter.right - (m_iSplitterPos + 4), GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
