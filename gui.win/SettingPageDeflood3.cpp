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
#include "SettingPageDeflood3.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------

SettingPageDeflood3::SettingPageDeflood3() {
    memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageDeflood3::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_SEARCH_MSGS:
            case EDT_SEARCH_SECS:
            case EDT_SEARCH_MSGS2:
            case EDT_SEARCH_SECS2:
            case EDT_SEARCH_INTERVAL_MSGS:
            case EDT_SEARCH_INTERVAL_SECS:
            case EDT_SAME_SEARCH_MSGS:
            case EDT_SAME_SEARCH_SECS:
            case EDT_GETNICKLIST_MSGS:
            case EDT_GETNICKLIST_MINS:
            case EDT_MYINFO_MSGS:
            case EDT_MYINFO_SECS:
            case EDT_MYINFO_MSGS2:
            case EDT_MYINFO_SECS2:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 999);

                    return 0;
                }

                break;
            case EDT_SR_MSGS:
            case EDT_SR_MSGS2:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 32767);

                    return 0;
                }

                break;
            case EDT_SR_SECS:
            case EDT_SR_SECS2:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 9999);

                    return 0;
                }

                break;
            case EDT_SR_TO_PASSIVE_LIMIT:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 0, 32767);

                    return 0;
                }

                break;
            case EDT_SR_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 8192);

                    return 0;
                }

                break;
            case EDT_MYINFO_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 64, 512);

                    return 0;
                }

                break;
            case EDT_RECONNECT_TIME:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 256);

                    return 0;
                }

                break;
            case CB_SEARCH:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_SEARCH], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SEARCH2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SEARCH_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SEARCH_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SAME_SEARCH:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_SAME_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SAME_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SAME_SEARCH_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_SAME_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SAME_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SAME_SEARCH_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SR:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_SR], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_SR_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SR_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SR_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_SR_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SR_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SR_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SR2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_SR2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_SR_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SR_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SR_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_SR_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_SR_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_SR_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_GETNICKLIST:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_GETNICKLIST_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_GETNICKLIST_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_GETNICKLIST_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_GETNICKLIST_MINS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_GETNICKLIST_MINS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_GETNICKLIST_MINUTES], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_MYINFO:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_MYINFO], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_MYINFO_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_MYINFO_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_MYINFO2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_MYINFO_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[UD_MYINFO_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageDeflood3::Save() {
    if(m_bCreated == false) {
        return;
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_SEARCH], CB_GETCURSEL, 0, 0));

    LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_TIME, LOWORD(lResult));
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_ACTION2, (int16_t)::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_INTERVAL_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_INTERVAL_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SEARCH_INTERVAL_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SEARCH_INTERVAL_TIME, LOWORD(lResult));
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_SAME_SEARCH_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(m_hWndPageItems[UD_SAME_SEARCH_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SAME_SEARCH_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SAME_SEARCH_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SAME_SEARCH_TIME, LOWORD(lResult));
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_SR_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_SR], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(m_hWndPageItems[UD_SR_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SR_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SR_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SR_TIME, LOWORD(lResult));
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_SR_ACTION2, (int16_t)::SendMessage(m_hWndPageItems[CB_SR2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(m_hWndPageItems[UD_SR_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SR_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SR_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_SR_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SR_TO_PASSIVE_LIMIT], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MAX_PASIVE_SR, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_SR_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SR_LEN, LOWORD(lResult));
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_GETNICKLIST_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(m_hWndPageItems[UD_GETNICKLIST_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_GETNICKLIST_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_GETNICKLIST_MINS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_GETNICKLIST_TIME, LOWORD(lResult));
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_MYINFO_ACTION, (int16_t)::SendMessage(m_hWndPageItems[CB_MYINFO], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(m_hWndPageItems[UD_MYINFO_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MYINFO_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_MYINFO_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MYINFO_TIME, LOWORD(lResult));
    }

    SettingManager::m_Ptr->SetShort(SETSHORT_MYINFO_ACTION2, (int16_t)::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(m_hWndPageItems[UD_MYINFO_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MYINFO_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_MYINFO_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MYINFO_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_MYINFO_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MAX_MYINFO_LEN, LOWORD(lResult));
    }

    lResult = ::SendMessage(m_hWndPageItems[UD_RECONNECT_TIME], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager::m_Ptr->SetShort(SETSHORT_MIN_RECONN_TIME, LOWORD(lResult));
    }
}
//------------------------------------------------------------------------------

void SettingPageDeflood3::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
}
//------------------------------------------------------------------------------

bool SettingPageDeflood3::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(m_bCreated == false) {
        return false;
    }

    RECT rcThis = { 0 };
    ::GetWindowRect(m_hWnd, &rcThis);

    m_hWndPageItems[GB_SEARCH] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SEARCH], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, m_iFullGB, m_iThreeLineGB,
        m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_SEARCH] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_SEARCH, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION], 0);

    m_hWndPageItems[EDT_SEARCH_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_MSGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SEARCH_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SEARCH_MSGS], ScaleGui(220) + 13, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_SEARCH_MSGS],
        (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_MESSAGES], 0));

    m_hWndPageItems[LBL_SEARCH_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SEARCH_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_SECS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SEARCH_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SEARCH_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_SEARCH_SECS],
        (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_TIME], 0));

    m_hWndPageItems[LBL_SEARCH_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_SEARCH2] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_SEARCH2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SEARCH2], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2], 0);

    m_hWndPageItems[EDT_SEARCH_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_MSGS2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SEARCH_MSGS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SEARCH_MSGS2], ScaleGui(220) + 13, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_SEARCH_MSGS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_MESSAGES2], 0));

    m_hWndPageItems[LBL_SEARCH_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SEARCH_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_SECS2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SEARCH_SECS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SEARCH_SECS2], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_SEARCH_SECS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_TIME2], 0));

    m_hWndPageItems[LBL_SEARCH_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[LBL_SEARCH_INTERVAL] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_INTERVAL], WS_CHILD | WS_VISIBLE | SS_RIGHT,
        8, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(180), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SEARCH_INTERVAL_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_INTERVAL_MSGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SEARCH_INTERVAL_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SEARCH_INTERVAL_MSGS], ScaleGui(220) + 13, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_SEARCH_INTERVAL_MSGS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_INTERVAL_MESSAGES], 0));

    m_hWndPageItems[LBL_SEARCH_INTERVAL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SEARCH_INTERVAL_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SEARCH_INTERVAL_SECS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SEARCH_INTERVAL_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SEARCH_INTERVAL_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_SEARCH_INTERVAL_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_INTERVAL_TIME], 0));

    m_hWndPageItems[LBL_SEARCH_INTERVAL_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iEditHeight) + 10 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    int iPosY = m_iThreeLineGB;

    m_hWndPageItems[GB_SAME_SEARCH] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SAME_SEARCH], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_SAME_SEARCH] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_SAME_SEARCH, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SAME_SEARCH], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION], 0);

    m_hWndPageItems[EDT_SAME_SEARCH_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SAME_SEARCH_MSGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SAME_SEARCH_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SAME_SEARCH_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 2),
        (WPARAM)m_hWndPageItems[EDT_SAME_SEARCH_MSGS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_MESSAGES], 0));

    m_hWndPageItems[LBL_SAME_SEARCH_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SAME_SEARCH_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SAME_SEARCH_SECS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SAME_SEARCH_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_SAME_SEARCH_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_SAME_SEARCH_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_TIME], 0));

    m_hWndPageItems[LBL_SAME_SEARCH_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    iPosY += GuiSettingManager::m_iOneLineGB;

    m_hWndPageItems[GB_SR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SR], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, iPosY, m_iFullGB, m_iTwoLineGB,
        m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_SR] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_SR, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SR], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION], 0);

    m_hWndPageItems[EDT_SR_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SR_MSGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SR_MSGS], EM_SETLIMITTEXT, 5, 0);

    AddUpDown(m_hWndPageItems[UD_SR_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 1), (WPARAM)m_hWndPageItems[EDT_SR_MSGS],
        (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_MESSAGES], 0));

    m_hWndPageItems[LBL_SR_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SR_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SR_SECS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SR_SECS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(m_hWndPageItems[UD_SR_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1), (WPARAM)m_hWndPageItems[EDT_SR_SECS],
        (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_TIME], 0));

    m_hWndPageItems[LBL_SR_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_SR2] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_SR2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_SR2], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2], 0);

    m_hWndPageItems[EDT_SR_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SR_MSGS2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SR_MSGS2], EM_SETLIMITTEXT, 5, 0);

    AddUpDown(m_hWndPageItems[UD_SR_MSGS2], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 1), (WPARAM)m_hWndPageItems[EDT_SR_MSGS2],
        (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_MESSAGES2], 0));

    m_hWndPageItems[LBL_SR_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SR_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SR_SECS2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SR_SECS2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(m_hWndPageItems[UD_SR_SECS2], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(9999, 1),
        (WPARAM)m_hWndPageItems[EDT_SR_SECS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_TIME2], 0));

    m_hWndPageItems[LBL_SR_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    iPosY += m_iTwoLineGB;

    m_hWndPageItems[GB_SR_TO_PASSIVE_LIMIT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PSR_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, ((rcThis.right - rcThis.left - 5) / 2) - 2, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ((rcThis.right - rcThis.left - 5) / 4) - 1 - ((ScaleGui(40) + GuiSettingManager::m_iUpDownWidth) / 2), iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight,
        m_hWnd, (HMENU)EDT_SR_TO_PASSIVE_LIMIT, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT], EM_SETLIMITTEXT, 5, 0);
    AddToolTip(m_hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(m_hWndPageItems[UD_SR_TO_PASSIVE_LIMIT], ((rcThis.right - rcThis.left - 5) / 4) + ScaleGui(40) - 1 - ((ScaleGui(40) + GuiSettingManager::m_iUpDownWidth) / 2), iPosY + GuiSettingManager::m_iGroupBoxMargin,
		GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 0), (WPARAM)m_hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PASIVE_SR], 0));

    m_hWndPageItems[GB_SR_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SR_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        ((rcThis.right - rcThis.left - 5) / 2) + 3, iPosY, (rcThis.right - rcThis.left) - (((rcThis.right - rcThis.left - 5) / 2) + 8), GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[LBL_SR_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ((rcThis.right - rcThis.left - 5) / 2) + 11, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 29, GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_SR_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        (rcThis.right - rcThis.left) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SR_LEN, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_SR_LEN], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(m_hWndPageItems[UD_SR_LEN], (rcThis.right - rcThis.left) - GuiSettingManager::m_iUpDownWidth - 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(8192, 1),
        (WPARAM)m_hWndPageItems[EDT_SR_LEN], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SR_LEN], 0));

    iPosY += GuiSettingManager::m_iOneLineGB;

    m_hWndPageItems[GB_GETNICKLIST] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_GETNICKLISTS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_GETNICKLIST] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_GETNICKLIST, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_GETNICKLIST], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION], 0);

    m_hWndPageItems[EDT_GETNICKLIST_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_GETNICKLIST_MSGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_GETNICKLIST_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_GETNICKLIST_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_GETNICKLIST_MSGS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_MESSAGES], 0));

    m_hWndPageItems[LBL_GETNICKLIST_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_GETNICKLIST_MINS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_GETNICKLIST_MINS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_GETNICKLIST_MINS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_GETNICKLIST_MINS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_GETNICKLIST_MINS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_TIME], 0));

    m_hWndPageItems[LBL_GETNICKLIST_MINUTES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MINUTES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    iPosY += GuiSettingManager::m_iOneLineGB;

    m_hWndPageItems[GB_MYINFO] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MYINFOS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, iPosY, m_iFullGB, m_iTwoLineGB,
        m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_MYINFO] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_MYINFO, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION], 0);

    m_hWndPageItems[EDT_MYINFO_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MYINFO_MSGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MYINFO_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_MYINFO_MSGS], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1), (WPARAM)m_hWndPageItems[EDT_MYINFO_MSGS],
        (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_MESSAGES], 0));

    m_hWndPageItems[LBL_MYINFO_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_MYINFO_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MYINFO_SECS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MYINFO_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_MYINFO_SECS], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_MYINFO_SECS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_TIME], 0));

    m_hWndPageItems[LBL_MYINFO_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[CB_MYINFO2] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(180), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)CB_MYINFO2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISABLED]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_IGNORE]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_WARN]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_KICK]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN]);
    ::SendMessage(m_hWndPageItems[CB_MYINFO2], CB_SETCURSEL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2], 0);

    m_hWndPageItems[EDT_MYINFO_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(180) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MYINFO_MSGS2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MYINFO_MSGS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_MYINFO_MSGS2], ScaleGui(220) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_MYINFO_MSGS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_MESSAGES2], 0));

    m_hWndPageItems[LBL_MYINFO_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
        ScaleGui(220) + GuiSettingManager::m_iUpDownWidth + 18, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_MYINFO_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(230) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MYINFO_SECS2, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MYINFO_SECS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_MYINFO_SECS2], ScaleGui(270) + GuiSettingManager::m_iUpDownWidth + 23, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 1),
        (WPARAM)m_hWndPageItems[EDT_MYINFO_SECS2], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_TIME2], 0));

    m_hWndPageItems[LBL_MYINFO_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 28, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 5 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(270) + (2 * GuiSettingManager::m_iUpDownWidth) + 41), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    iPosY += m_iTwoLineGB;

    m_hWndPageItems[GB_MYINFO_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MYINFO_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, ((rcThis.right - rcThis.left - 5) / 2) - 2, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[LBL_MYINFO_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 23, GuiSettingManager::m_iTextHeight,
        m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_MYINFO_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 10, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MYINFO_LEN, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MYINFO_LEN], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_MYINFO_LEN], ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - 10, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(512, 64),
        (WPARAM)m_hWndPageItems[EDT_MYINFO_LEN], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_MYINFO_LEN], 0));

    m_hWndPageItems[GB_RECONNECT_TIME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_RECONN_TIME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        ((rcThis.right - rcThis.left - 5) / 2) + 3, iPosY, (rcThis.right - rcThis.left) - (((rcThis.right - rcThis.left - 5) / 2) + 8), GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[LBL_RECONNECT_TIME_MINIMAL] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MINIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ((rcThis.right - rcThis.left - 5) / 2) + 11, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(70), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_RECONNECT_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ((rcThis.right - rcThis.left - 5) / 2) + ScaleGui(70) + 16, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_RECONNECT_TIME, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_RECONNECT_TIME], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(m_hWndPageItems[UD_RECONNECT_TIME], ((rcThis.right - rcThis.left - 5) / 2) + ScaleGui(110) + 16, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(256, 1),
        (WPARAM)m_hWndPageItems[EDT_RECONNECT_TIME], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_RECONN_TIME], 0));

    m_hWndPageItems[LBL_RECONNECT_TIME_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT,
        ((rcThis.right - rcThis.left - 5) / 2) + ScaleGui(110) + GuiSettingManager::m_iUpDownWidth + 21, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
        (rcThis.right - rcThis.left) - (ScaleGui(210) + (2 * GuiSettingManager::m_iUpDownWidth) + 37), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
        if(m_hWndPageItems[ui8i] == nullptr) {
            return false;
        }

        ::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SEARCH_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SEARCH_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SEARCH_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_DIVIDER2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_SEARCH_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SEARCH_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SEARCH_SECONDS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_SAME_SEARCH_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SAME_SEARCH_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SAME_SEARCH_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_SAME_SEARCH_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SAME_SEARCH_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SAME_SEARCH_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_SR_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SR_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SR_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_SR_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SR_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SR_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_SR_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SR_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SR_DIVIDER2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_SR_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_SR_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_SR_SECONDS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_GETNICKLIST_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_GETNICKLIST_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_GETNICKLIST_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_GETNICKLIST_MINS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_GETNICKLIST_MINS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_GETNICKLIST_MINUTES], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_MYINFO_MSGS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_DIVIDER], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_MYINFO_SECS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_SECONDS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_MYINFO_MSGS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_DIVIDER2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_MYINFO_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[UD_MYINFO_SECS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[LBL_MYINFO_SECONDS2], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);

    GuiSettingManager::m_wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_RECONNECT_TIME], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageDeflood3::GetPageName() {
    return LanguageManager::m_Ptr->m_sTexts[LAN_MORE_MORE_DEFLOOD];
}
//------------------------------------------------------------------------------

void SettingPageDeflood3::FocusLastItem() {
    ::SetFocus(m_hWndPageItems[EDT_RECONNECT_TIME]);
}
//------------------------------------------------------------------------------
