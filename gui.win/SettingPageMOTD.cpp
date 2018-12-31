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
#include "SettingPageMOTD.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
static WNDPROC wpOldMultiEditProc = nullptr;
//---------------------------------------------------------------------------

LRESULT CALLBACK MultiEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return 0;
    } else if(uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        ::SendMessage(::GetParent(::GetParent(hWnd)), WM_COMMAND, IDCANCEL, 0);
        return 0;
    }

    return ::CallWindowProc(wpOldMultiEditProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

SettingPageMOTD::SettingPageMOTD() : m_bUpdateMOTD(false) {
    memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageMOTD::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        if(LOWORD(wParam) == EDT_MOTD) {
            if(HIWORD(wParam) == EN_CHANGE) {
                int iLen = ::GetWindowTextLength((HWND)lParam);

                char * buf = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);

                if(buf == nullptr) {
                    AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in SettingPageMOTD::PageMOTDProc\n", iLen+1);
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

                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)buf) == 0) {
                    AppendDebugLog("%s - [MEM] Cannot deallocate buf in SettingPageMOTD::PageMOTDProc\n");
                }

                return 0;
            }
        } else if(LOWORD(wParam) == BTN_DISABLE_MOTD) {
            if(HIWORD(wParam) == BN_CLICKED) {
                BOOL bDisableMOTD = ::SendMessage(m_hWndPageItems[BTN_DISABLE_MOTD], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
                ::EnableWindow(m_hWndPageItems[EDT_MOTD], bDisableMOTD);
                ::EnableWindow(m_hWndPageItems[BTN_MOTD_AS_PM], bDisableMOTD);
            }
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageMOTD::Save() {
    if(m_bCreated == false) {
        return;
    }

    bool bMOTDAsPM = ::SendMessage(m_hWndPageItems[BTN_MOTD_AS_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
    bool bDisableMOTD = ::SendMessage(m_hWndPageItems[BTN_DISABLE_MOTD], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    int iAllocLen = ::GetWindowTextLength(m_hWndPageItems[EDT_MOTD]);

    char * buf = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iAllocLen+1);

    if(buf == nullptr) {
        AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for buf in SettingPageMOTD::Save\n", iAllocLen+1);
        return;
    }

    int iLen = ::GetWindowText(m_hWndPageItems[EDT_MOTD], buf, iAllocLen+1);

	if((bDisableMOTD != SettingManager::m_Ptr->m_bBools[SETBOOL_DISABLE_MOTD] || bMOTDAsPM != SettingManager::m_Ptr->m_bBools[SETBOOL_MOTD_AS_PM]) || 
		(SettingManager::m_Ptr->m_sMOTD == nullptr && iLen != 0) || (SettingManager::m_Ptr->m_sMOTD != nullptr && strcmp(buf, SettingManager::m_Ptr->m_sMOTD) != 0))
	{
		m_bUpdateMOTD = true;
	}

    SettingManager::m_Ptr->SetMOTD(buf, iLen);

    if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)buf) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate buf in SettingPageMOTD::Save\n");
    }

    SettingManager::m_Ptr->SetBool(SETBOOL_MOTD_AS_PM, bMOTDAsPM);

    SettingManager::m_Ptr->SetBool(SETBOOL_DISABLE_MOTD, bDisableMOTD);
}
//------------------------------------------------------------------------------

void SettingPageMOTD::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool &bUpdatedMOTD, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
    if(bUpdatedMOTD == false) {
        bUpdatedMOTD = m_bUpdateMOTD;
    }
}

//------------------------------------------------------------------------------

bool SettingPageMOTD::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(m_bCreated == false) {
        return false;
    }

    RECT rcThis = { 0 };
    ::GetWindowRect(m_hWnd, &rcThis);

    m_hWndPageItems[GB_MOTD] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MOTD], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, m_iFullGB, (rcThis.bottom - rcThis.top) - 3, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_MOTD] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sMOTD != nullptr ? SettingManager::m_Ptr->m_sMOTD : nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN, 8, GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, (rcThis.bottom - rcThis.top) - GuiSettingManager::m_iGroupBoxMargin - (2 * GuiSettingManager::m_iCheckHeight) - 18,
        m_hWnd, (HMENU)EDT_MOTD, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MOTD], EM_SETLIMITTEXT, 64000, 0);

    m_hWndPageItems[BTN_MOTD_AS_PM] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MOTD_IN_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, (rcThis.bottom - rcThis.top) - (2 * GuiSettingManager::m_iCheckHeight) - 14, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_MOTD_AS_PM], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_MOTD_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[BTN_DISABLE_MOTD] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DISABLE_MOTD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, (rcThis.bottom - rcThis.top) - GuiSettingManager::m_iCheckHeight - 11, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_DISABLE_MOTD, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_DISABLE_MOTD], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_DISABLE_MOTD] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
        if(m_hWndPageItems[ui8i] == nullptr) {
            return false;
        }

        ::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(m_hWndPageItems[EDT_MOTD], SettingManager::m_Ptr->m_bBools[SETBOOL_DISABLE_MOTD] == true ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[BTN_MOTD_AS_PM], SettingManager::m_Ptr->m_bBools[SETBOOL_DISABLE_MOTD] == true ? FALSE : TRUE);

    wpOldMultiEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_MOTD], GWLP_WNDPROC, (LONG_PTR)MultiEditProc);

    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_DISABLE_MOTD], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageMOTD::GetPageName() {
    return LanguageManager::m_Ptr->m_sTexts[LAN_MOTD_ONLY];
}
//------------------------------------------------------------------------------

void SettingPageMOTD::FocusLastItem() {
    ::SetFocus(m_hWndPageItems[BTN_DISABLE_MOTD]);
}
//------------------------------------------------------------------------------
