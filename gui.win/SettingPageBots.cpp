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
#include "SettingPageBots.h"
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

SettingPageBots::SettingPageBots() : m_bUpdateHubSec(false), m_bUpdateMOTD(false), m_bUpdateHubNameWelcome(false), m_bUpdateRegOnlyMessage(false), m_bUpdateShareLimitMessage(false), m_bUpdateSlotsLimitMessage(false),
	m_bUpdateHubSlotRatioMessage(false), m_bUpdateMaxHubsLimitMessage(false), m_bUpdateNoTagMessage(false), m_bUpdateNickLimitMessage(false), m_bUpdateBotsSameNick(false),
	m_bBotNickChanged(false), m_bUpdateBot(false), m_bOpChatNickChanged(false), m_bUpdateOpChat(false) {
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageBots::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == WM_COMMAND) {
		switch(LOWORD(wParam)) {
		case EDT_HUB_BOT_NICK:
		case EDT_OP_CHAT_BOT_NICK:
			if(HIWORD(wParam) == EN_CHANGE) {
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);

				bool bChanged = false;

				for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
					if(buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ') {
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

				return 0;
			}

			break;
		case EDT_HUB_BOT_DESCRIPTION:
		case EDT_HUB_BOT_EMAIL:
		case EDT_OP_CHAT_BOT_DESCRIPTION:
		case EDT_OP_CHAT_BOT_EMAIL:
			if(HIWORD(wParam) == EN_CHANGE) {
				RemoveDollarsPipes((HWND)lParam);

				return 0;
			}

			break;
		case BTN_HUB_BOT_ENABLE:
			if(HIWORD(wParam) == BN_CLICKED) {
				BOOL bEnable = ::SendMessage(m_hWndPageItems[BTN_HUB_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
				::EnableWindow(m_hWndPageItems[EDT_HUB_BOT_NICK], bEnable);
				::EnableWindow(m_hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], bEnable);
				::EnableWindow(m_hWndPageItems[EDT_HUB_BOT_DESCRIPTION], bEnable);
				::EnableWindow(m_hWndPageItems[EDT_HUB_BOT_EMAIL], bEnable);
			}

			break;
		case BTN_OP_CHAT_BOT_ENABLE:
			if(HIWORD(wParam) == BN_CLICKED) {
				BOOL bEnable = ::SendMessage(m_hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
				::EnableWindow(m_hWndPageItems[EDT_OP_CHAT_BOT_NICK], bEnable);
				::EnableWindow(m_hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], bEnable);
				::EnableWindow(m_hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], bEnable);
			}

			break;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageBots::Save() {
	if(m_bCreated == false) {
		return;
	}

	char sBot[65];
	int iBotLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_BOT_NICK], sBot, 65);

	bool bBotHaveNewNick = (strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK], sBot) != 0);
	bool bRegBot = ::SendMessage(m_hWndPageItems[BTN_HUB_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	bool bRegStateChange = false, bDescriptionChange = false, bEmailChange = false;

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] != bRegBot) {
		bRegStateChange = true;
	}

	if(strcmp(sBot, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK]) != 0) {
		m_bUpdateHubSec = true;
		m_bUpdateMOTD = true;
		m_bUpdateHubNameWelcome = true;
		m_bUpdateRegOnlyMessage = true;
		m_bUpdateShareLimitMessage = true;
		m_bUpdateSlotsLimitMessage = true;
		m_bUpdateHubSlotRatioMessage = true;
		m_bUpdateMaxHubsLimitMessage = true;
		m_bUpdateNoTagMessage = true;
		m_bUpdateNickLimitMessage = true;
		m_bUpdateBotsSameNick = true;
		m_bBotNickChanged = true;
		m_bUpdateBot = true;
	}

	if((m_bUpdateBot == false || m_bBotNickChanged == false) && (bRegBot == true && SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false)) {
		m_bUpdateBot = true;
		m_bBotNickChanged = true;
	}

	bool bHubBotIsHubSec = ::SendMessage(m_hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bHubBotIsHubSec != SettingManager::m_Ptr->m_bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC]) {
		m_bUpdateHubSec = true;
		m_bUpdateMOTD = true;
		m_bUpdateHubNameWelcome = true;
		m_bUpdateRegOnlyMessage = true;
		m_bUpdateShareLimitMessage = true;
		m_bUpdateSlotsLimitMessage = true;
		m_bUpdateHubSlotRatioMessage = true;
		m_bUpdateMaxHubsLimitMessage = true;
		m_bUpdateNoTagMessage = true;
		m_bUpdateNickLimitMessage = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_USE_BOT_NICK_AS_HUB_SEC, bHubBotIsHubSec);

	char buf[65];
	int iLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_BOT_DESCRIPTION], buf, 65);

	if(m_bUpdateBot == false &&
	        ((SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION] == nullptr && iLen != 0) ||
	         (SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION] != nullptr &&
	          strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION]) != 0))) {
		m_bUpdateBot = true;
	}

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION]) != 0)) {
		bDescriptionChange = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_BOT_DESCRIPTION, buf, iLen);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_BOT_EMAIL], buf, 65);

	if(m_bUpdateBot == false &&
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL]) != 0)) {
		m_bUpdateBot = true;
	}

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL]) != 0)) {
		bEmailChange = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_BOT_EMAIL, buf, iLen);

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true) {
		SettingManager::m_Ptr->m_bUpdateLocked = false;
		SettingManager::m_Ptr->DisableBot((bBotHaveNewNick == true || bRegBot == false),
		                                  (bRegStateChange == true || bBotHaveNewNick == true || bDescriptionChange == true || bEmailChange == true) ? true : false);
		SettingManager::m_Ptr->m_bUpdateLocked = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_BOT_NICK, sBot, iBotLen);

	iBotLen = ::GetWindowText(m_hWndPageItems[EDT_OP_CHAT_BOT_NICK], sBot, 65);

	bool bRegOpChat = ::SendMessage(m_hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	bBotHaveNewNick = (strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK], sBot) != 0);

	if(m_bUpdateBotsSameNick == false && (bRegBot != SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] ||
	                                      bRegOpChat != SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] ||
	                                      strcmp(sBot, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]) != 0)) {
		m_bUpdateBotsSameNick = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_REG_BOT, bRegBot);

	if(strcmp(sBot, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]) != 0) {
		m_bOpChatNickChanged = true;
		m_bUpdateOpChat = true;
	}

	if((m_bUpdateOpChat == false || m_bOpChatNickChanged == false) &&
	        (bRegOpChat == true && SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == false)) {
		m_bUpdateOpChat = true;
		m_bOpChatNickChanged = true;
	}

	iLen = ::GetWindowText(m_hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], buf, 65);

	if(m_bUpdateOpChat == false &&
	        ((SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION] == nullptr && iLen != 0) ||
	         (SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION] != nullptr &&
	          strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION]) != 0))) {
		m_bUpdateOpChat = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_OP_CHAT_DESCRIPTION, buf, iLen);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], buf, 65);

	if(m_bUpdateOpChat == false &&
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL]) != 0)) {
		m_bUpdateOpChat = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_OP_CHAT_EMAIL, buf, iLen);

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true) {
		SettingManager::m_Ptr->m_bUpdateLocked = false;
		SettingManager::m_Ptr->DisableOpChat((bBotHaveNewNick == true || bRegOpChat == false));
		SettingManager::m_Ptr->m_bUpdateLocked = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_OP_CHAT_NICK, sBot, iBotLen);

	SettingManager::m_Ptr->SetBool(SETBOOL_REG_OP_CHAT, bRegOpChat);
}
//------------------------------------------------------------------------------

void SettingPageBots::GetUpdates(bool &bUpdatedHubNameWelcome, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
                                 bool & /*bUpdatedAutoReg*/, bool &bUpdatedMOTD, bool &bUpdatedHubSec, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
                                 bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage,
                                 bool &bUpdatedNickLimitMessage, bool &bUpdatedBotsSameNick, bool &bUpdatedBotNick, bool &bUpdatedBot, bool &bUpdatedOpChatNick,
                                 bool &bUpdatedOpChat, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                 bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
	if(bUpdatedHubNameWelcome == false) {
		bUpdatedHubNameWelcome = m_bUpdateHubNameWelcome;
	}
	if(bUpdatedMOTD == false) {
		bUpdatedMOTD = m_bUpdateMOTD;
	}
	if(bUpdatedHubSec == false) {
		bUpdatedHubSec = m_bUpdateHubSec;
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
	if(bUpdatedNickLimitMessage == false) {
		bUpdatedNickLimitMessage = m_bUpdateNickLimitMessage;
	}
	if(bUpdatedBotsSameNick == false) {
		bUpdatedBotsSameNick = m_bUpdateBotsSameNick;
	}
	if(bUpdatedBotNick == false) {
		bUpdatedBotNick = m_bBotNickChanged;
	}
	if(bUpdatedBot == false) {
		bUpdatedBot = m_bUpdateBot;
	}
	if(bUpdatedOpChatNick == false) {
		bUpdatedOpChatNick = m_bOpChatNickChanged;
	}
	if(bUpdatedOpChat == false) {
		bUpdatedOpChat = m_bUpdateOpChat;
	}
}

//------------------------------------------------------------------------------
bool SettingPageBots::CreateSettingPage(HWND hOwner) {
	CreateHWND(hOwner);

	if(m_bCreated == false) {
		return false;
	}

	int iPosY = GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iOneLineOneChecksGB + (2 * GuiSettingManager::m_iOneLineGB) + 1 + 5;

	m_hWndPageItems[GB_HUB_BOT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_BOT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                              0, 0, m_iFullGB, iPosY, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_HUB_BOT_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_AND_REG_BOT_ON_HUB], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                      8, GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_HUB_BOT_ENABLE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_HUB_BOT_ENABLE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[GB_HUB_BOT_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                   5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGB, GuiSettingManager::m_iOneLineOneChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_BOT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                    13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_NICK, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_BOT_NICK], EM_SETLIMITTEXT, 64, 0);

	m_hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_USE_AS_ALIAS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iEditHeight + 4, m_iGBinGBEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[GB_HUB_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	        5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineOneChecksGB, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	        13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineOneChecksGB, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_DESCRIPTION, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_BOT_DESCRIPTION], EM_SETLIMITTEXT, 64, 0);

	m_hWndPageItems[GB_HUB_BOT_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                    5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineOneChecksGB + GuiSettingManager::m_iOneLineGB, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_BOT_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                     13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineOneChecksGB + GuiSettingManager::m_iOneLineGB, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_EMAIL, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_BOT_EMAIL], EM_SETLIMITTEXT, 64, 0);

	m_hWndPageItems[GB_OP_CHAT_BOT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_OP_CHAT_BOT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                  0, iPosY, m_iFullGB, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + (3 * GuiSettingManager::m_iOneLineGB) + 1 + 5, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_OP_CHAT_BOT_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_AND_REG_BOT_ON_HUB], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	        8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_OP_CHAT_BOT_ENABLE, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[GB_OP_CHAT_BOT_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                       5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_OP_CHAT_BOT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_NICK, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_OP_CHAT_BOT_NICK], EM_SETLIMITTEXT, 64, 0);

	m_hWndPageItems[GB_OP_CHAT_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	        5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineGB, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	        ES_AUTOHSCROLL, 13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + GuiSettingManager::m_iOneLineGB, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_DESCRIPTION, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], EM_SETLIMITTEXT, 64, 0);

	m_hWndPageItems[GB_OP_CHAT_BOT_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                        5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 1 + (2 * GuiSettingManager::m_iOneLineGB), m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_OP_CHAT_BOT_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iCheckHeight + 1 + (2 * GuiSettingManager::m_iOneLineGB), m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_EMAIL, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], EM_SETLIMITTEXT, 64, 0);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++) {
		if(m_hWndPageItems[ui8i] == nullptr) {
			return false;
		}

		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	::EnableWindow(m_hWndPageItems[EDT_HUB_BOT_NICK], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[EDT_HUB_BOT_DESCRIPTION], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[EDT_HUB_BOT_EMAIL], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);

	::EnableWindow(m_hWndPageItems[EDT_OP_CHAT_BOT_NICK], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);

	GuiSettingManager::m_wpOldEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], GWLP_WNDPROC, (LONG_PTR)EditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageBots::GetPageName() {
	return LanguageManager::m_Ptr->m_sTexts[LAN_DEFAULT_BOTS];
}
//------------------------------------------------------------------------------

void SettingPageBots::FocusLastItem() {
	::SetFocus(m_hWndPageItems[EDT_OP_CHAT_BOT_EMAIL]);
}
//------------------------------------------------------------------------------
