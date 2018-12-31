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
#include "SettingPageGeneral2.h"
//---------------------------------------------------------------------------
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------

SettingPageGeneral2::SettingPageGeneral2() : m_bUpdateTextFiles(false), m_bUpdateRedirectAddress(false), m_bUpdateRegOnlyMessage(false), m_bUpdateShareLimitMessage(false), m_bUpdateSlotsLimitMessage(false),
    m_bUpdateHubSlotRatioMessage(false), m_bUpdateMaxHubsLimitMessage(false), m_bUpdateNoTagMessage(false), m_bUpdateTempBanRedirAddress(false), m_bUpdatePermBanRedirAddress(false),
    m_bUpdateNickLimitMessage(false) {
    memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageGeneral2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_OWNER_EMAIL:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemoveDollarsPipes((HWND)lParam);

                    return 0;
                }

                break;
            case EDT_MAIN_REDIR_ADDR:
            case EDT_MSG_TO_NON_REGS:
            case EDT_NON_REG_REDIR_ADDR:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }

                break;
            case BTN_ENABLE_TEXT_FILES:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(m_hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM],
                        ::SendMessage(m_hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
            case BTN_DONT_ALLOW_PINGER:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(m_hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
                    ::EnableWindow(m_hWndPageItems[BTN_REPORT_PINGER], bEnable);
                    ::EnableWindow(m_hWndPageItems[EDT_OWNER_EMAIL], bEnable);
                }

                break;
            case BTN_REDIR_ALL:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(m_hWndPageItems[BTN_REDIR_HUB_FULL],
                        ::SendMessage(m_hWndPageItems[BTN_REDIR_ALL], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE);
                }

                break;
            case BTN_ALLOW_ONLY_REGS:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(m_hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
                    ::EnableWindow(m_hWndPageItems[EDT_MSG_TO_NON_REGS], bEnable);
                    ::EnableWindow(m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE], bEnable);
                    ::EnableWindow(m_hWndPageItems[EDT_NON_REG_REDIR_ADDR],
                        (bEnable == TRUE && ::SendMessage(m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE);
                }

                break;
            case BTN_NON_REG_REDIR_ENABLE:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(m_hWndPageItems[EDT_NON_REG_REDIR_ADDR],
                        ::SendMessage(m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
        }
    }


	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::Save() {
    if(m_bCreated == false) {
        return;
    }

    if((::SendMessage(m_hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TEXT_FILES]) {
		m_bUpdateTextFiles = true;
    }

    SettingManager::m_Ptr->SetBool(SETBOOL_ENABLE_TEXT_FILES, ::SendMessage(m_hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager::m_Ptr->SetBool(SETBOOL_SEND_TEXT_FILES_AS_PM, ::SendMessage(m_hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager::m_Ptr->SetBool(SETBOOL_DONT_ALLOW_PINGERS, ::SendMessage(m_hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager::m_Ptr->SetBool(SETBOOL_REPORT_PINGERS, ::SendMessage(m_hWndPageItems[BTN_REPORT_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    char buf[257];
    int iLen = ::GetWindowText(m_hWndPageItems[EDT_OWNER_EMAIL], buf, 257);
    SettingManager::m_Ptr->SetText(SETTXT_HUB_OWNER_EMAIL, buf, iLen);

    iLen = ::GetWindowText(m_hWndPageItems[EDT_MAIN_REDIR_ADDR], buf, 257);

    if((SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS] == nullptr && iLen != 0) || (SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS] != nullptr && strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS]) != 0)) {
		m_bUpdateRedirectAddress = true;
		m_bUpdateRegOnlyMessage = true;
		m_bUpdateShareLimitMessage = true;
		m_bUpdateSlotsLimitMessage = true;
		m_bUpdateHubSlotRatioMessage = true;
		m_bUpdateMaxHubsLimitMessage = true;
		m_bUpdateNoTagMessage = true;
		m_bUpdateTempBanRedirAddress = true;
		m_bUpdatePermBanRedirAddress = true;
		m_bUpdateNickLimitMessage = true;
	}

    SettingManager::m_Ptr->SetText(SETTXT_REDIRECT_ADDRESS, buf, iLen);

    SettingManager::m_Ptr->SetBool(SETBOOL_REDIRECT_ALL, ::SendMessage(m_hWndPageItems[BTN_REDIR_ALL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager::m_Ptr->SetBool(SETBOOL_REDIRECT_WHEN_HUB_FULL, ::SendMessage(m_hWndPageItems[BTN_REDIR_HUB_FULL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager::m_Ptr->SetBool(SETBOOL_REG_ONLY, ::SendMessage(m_hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    iLen = ::GetWindowText(m_hWndPageItems[EDT_MSG_TO_NON_REGS], buf, 257);

	if(m_bUpdateRegOnlyMessage == false && ((::SendMessage(m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY_REDIR] ||
		strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_REG_ONLY_MSG]) != 0))
	{
		m_bUpdateRegOnlyMessage = true;
	}

    SettingManager::m_Ptr->SetText(SETTXT_REG_ONLY_MSG, buf, iLen);

    SettingManager::m_Ptr->SetBool(SETBOOL_REG_ONLY_REDIR, ::SendMessage(m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    iLen = ::GetWindowText(m_hWndPageItems[EDT_NON_REG_REDIR_ADDR], buf, 257);

    if(m_bUpdateRegOnlyMessage == false &&
        (SettingManager::m_Ptr->m_sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] == nullptr && iLen != 0) ||
        (SettingManager::m_Ptr->m_sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != nullptr &&
        strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS]) != 0))
	{
		m_bUpdateRegOnlyMessage = true;
	}

    SettingManager::m_Ptr->SetText(SETTXT_REG_ONLY_REDIR_ADDRESS, buf, iLen);

    SettingManager::m_Ptr->SetBool(SETBOOL_KEEP_SLOW_USERS, ::SendMessage(m_hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager::m_Ptr->SetBool(SETBOOL_HASH_PASSWORDS, ::SendMessage(m_hWndPageItems[BTN_HASH_PASSWORDS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager::m_Ptr->SetBool(SETBOOL_NO_QUACK_SUPPORTS, ::SendMessage(m_hWndPageItems[BTN_KILL_THAT_DUCK], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_HASH_PASSWORDS] == true) {
        RegManager::m_Ptr->HashPasswords();
    }
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
    bool & /*bUpdatedAutoReg*/, bool &/*bUpdatedMOTD*/, bool &/*bUpdatedHubSec*/, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
    bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage,
    bool &bUpdatedNickLimitMessage, bool &/*bUpdatedBotsSameNick*/, bool &/*bUpdatedBotNick*/, bool &/*bUpdatedBot*/, bool &/*bUpdatedOpChatNick*/,
    bool &/*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool &bUpdatedTextFiles, bool &bUpdatedRedirectAddress, bool &bUpdatedTempBanRedirAddress,
    bool &bUpdatedPermBanRedirAddress, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
    if(bUpdatedTextFiles == false) {
        bUpdatedTextFiles = m_bUpdateTextFiles;
    }
    if(bUpdatedRedirectAddress == false) {
        bUpdatedRedirectAddress = m_bUpdateRedirectAddress;
    }
    if(bUpdatedRegOnlyMessage == false) {
        bUpdatedRegOnlyMessage = m_bUpdateRegOnlyMessage;
    }
    if(bUpdatedShareLimitMessage == false) {
        bUpdatedShareLimitMessage = m_bUpdateShareLimitMessage;
    }
    if(bUpdatedSlotsLimitMessage == false) {
        bUpdatedSlotsLimitMessage = m_bUpdateSlotsLimitMessage;
    }
    if(bUpdatedHubSlotRatioMessage == false) {
        bUpdatedHubSlotRatioMessage = m_bUpdateHubSlotRatioMessage;
    }
    if(bUpdatedMaxHubsLimitMessage == false) {
        bUpdatedMaxHubsLimitMessage = m_bUpdateMaxHubsLimitMessage;
    }
    if(bUpdatedNoTagMessage == false) {
        bUpdatedNoTagMessage = m_bUpdateNoTagMessage;
    }
    if(bUpdatedTempBanRedirAddress == false) {
        bUpdatedTempBanRedirAddress = m_bUpdateTempBanRedirAddress;
    }
    if(bUpdatedPermBanRedirAddress == false) {
        bUpdatedPermBanRedirAddress = m_bUpdatePermBanRedirAddress;
    }
    if(bUpdatedNickLimitMessage == false) {
        bUpdatedNickLimitMessage = m_bUpdateNickLimitMessage;
    }
}

//------------------------------------------------------------------------------
bool SettingPageGeneral2::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(m_bCreated == false) {
        return false;
    }

    RECT rcThis = { 0 };
    ::GetWindowRect(m_hWnd, &rcThis);

    m_hWndPageItems[GB_TEXT_FILES] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_TEXT_FILES], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, m_iFullGB, m_iTwoChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[BTN_ENABLE_TEXT_FILES] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_TEXT_FILES], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_ENABLE_TEXT_FILES, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_ENABLE_TEXT_FILES], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TEXT_FILES] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_TEXT_FILES_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_TEXT_FILES_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    int iPosY = m_iTwoChecksGB;

    m_hWndPageItems[GB_PINGER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_PINGER], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, m_iFullGB, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 4 + GuiSettingManager::m_iOneLineGB + 5, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[BTN_DONT_ALLOW_PINGER] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DISALLOW_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_DONT_ALLOW_PINGER, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_DONT_ALLOW_PINGER], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[BTN_REPORT_PINGER] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REPORT_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_REPORT_PINGER], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REPORT_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[GB_OWNER_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_OWNER_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 4, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_OWNER_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_OWNER_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + (2 * GuiSettingManager::m_iCheckHeight) + 4, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_OWNER_EMAIL, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_OWNER_EMAIL], EM_SETLIMITTEXT, 64, 0);

    iPosY += GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 4 + GuiSettingManager::m_iOneLineGB + 5;

    m_hWndPageItems[GB_MAIN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MAIN_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineTwoChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_MAIN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAIN_REDIR_ADDR, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MAIN_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);

    m_hWndPageItems[BTN_REDIR_ALL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ALL_CONN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_REDIR_ALL, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_REDIR_ALL], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REDIRECT_ALL] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[BTN_REDIR_HUB_FULL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_FULL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iCheckHeight + 7, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_REDIR_HUB_FULL], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    iPosY += GuiSettingManager::m_iOneLineTwoChecksGB;

    m_hWndPageItems[GB_REG_ONLY_HUB] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REG_ONLY_HUB], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, m_iFullGB, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + (2 * GuiSettingManager::m_iOneLineGB) + 6, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[BTN_ALLOW_ONLY_REGS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ALLOW_ONLY_REGS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_ALLOW_ONLY_REGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_ALLOW_ONLY_REGS], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[GB_MSG_TO_NON_REGS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REG_ONLY_MSG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[EDT_MSG_TO_NON_REGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_REG_ONLY_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MSG_TO_NON_REGS, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_MSG_TO_NON_REGS], EM_SETLIMITTEXT, 256, 0);

    m_hWndPageItems[GB_NON_REG_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineGB, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineGB + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight)/2), ScaleGui(85), GuiSettingManager::m_iCheckHeight,
        m_hWnd, (HMENU)BTN_NON_REG_REDIR_ENABLE, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[EDT_NON_REG_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineGB, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), GuiSettingManager::m_iEditHeight,
        m_hWnd, (HMENU)EDT_NON_REG_REDIR_ADDR, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[EDT_NON_REG_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(m_hWndPageItems[EDT_NON_REG_REDIR_ADDR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

    iPosY +=  GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + (2 * GuiSettingManager::m_iOneLineGB) + 6;

    m_hWndPageItems[GB_EXPERTS_ONLY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_EXPERTS_ONLY], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosY, m_iFullGB, GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iCheckHeight) + 14, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

    m_hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_KEEP_SLOW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_KEEP_SLOW_USERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[BTN_HASH_PASSWORDS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_STORE_REG_PASS_HASHED], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_HASH_PASSWORDS], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_HASH_PASSWORDS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    m_hWndPageItems[BTN_KILL_THAT_DUCK] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DISALLOW_BUGGY_SUPPORTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 6, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
    ::SendMessage(m_hWndPageItems[BTN_KILL_THAT_DUCK], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_NO_QUACK_SUPPORTS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
        if(m_hWndPageItems[ui8i] == nullptr) {
            return false;
        }

        ::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(m_hWndPageItems[BTN_SEND_TEXT_FILES_AS_PM], SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TEXT_FILES] == true ? TRUE : FALSE);

    ::EnableWindow(m_hWndPageItems[BTN_REPORT_PINGER], SettingManager::m_Ptr->m_bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? FALSE : TRUE);
    ::EnableWindow(m_hWndPageItems[EDT_OWNER_EMAIL], SettingManager::m_Ptr->m_bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[BTN_REDIR_HUB_FULL], SettingManager::m_Ptr->m_bBools[SETBOOL_REDIRECT_ALL] == true ? FALSE : TRUE);

    ::EnableWindow(m_hWndPageItems[EDT_MSG_TO_NON_REGS], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY] == true ? TRUE : FALSE);
    ::EnableWindow(m_hWndPageItems[BTN_NON_REG_REDIR_ENABLE], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY] == true ? TRUE : FALSE);
    ::EnableWindow(m_hWndPageItems[EDT_NON_REG_REDIR_ADDR],
        (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY] == true && SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY_REDIR] == true) ? TRUE : FALSE);

    GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_KILL_THAT_DUCK], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageGeneral2::GetPageName() {
    return LanguageManager::m_Ptr->m_sTexts[LAN_MORE_GENERAL];
}
//------------------------------------------------------------------------------

void SettingPageGeneral2::FocusLastItem() {
    ::SetFocus(m_hWndPageItems[BTN_KILL_THAT_DUCK]);
}
//------------------------------------------------------------------------------
