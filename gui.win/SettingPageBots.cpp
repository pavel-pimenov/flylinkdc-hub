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

SettingPageBots::SettingPageBots() : bUpdateHubSec(false), bUpdateMOTD(false), bUpdateHubNameWelcome(false), bUpdateRegOnlyMessage(false), bUpdateShareLimitMessage(false), bUpdateSlotsLimitMessage(false),
	bUpdateHubSlotRatioMessage(false), bUpdateMaxHubsLimitMessage(false), bUpdateNoTagMessage(false), bUpdateNickLimitMessage(false), bUpdateBotsSameNick(false),
	bBotNickChanged(false), bUpdateBot(false), bOpChatNickChanged(false), bUpdateOpChat(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageBots::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case EDT_HUB_BOT_NICK:
		case EDT_OP_CHAT_BOT_NICK:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);
				
				bool bChanged = false;
				
				for (uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++)
				{
					if (buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ')
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
				
				return 0;
			}
			
			break;
		case EDT_HUB_BOT_DESCRIPTION:
		case EDT_HUB_BOT_EMAIL:
		case EDT_OP_CHAT_BOT_DESCRIPTION:
		case EDT_OP_CHAT_BOT_EMAIL:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemoveDollarsPipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case BTN_HUB_BOT_ENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnable = ::SendMessage(hWndPageItems[BTN_HUB_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
				::EnableWindow(hWndPageItems[EDT_HUB_BOT_NICK], bEnable);
				::EnableWindow(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], bEnable);
				::EnableWindow(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], bEnable);
				::EnableWindow(hWndPageItems[EDT_HUB_BOT_EMAIL], bEnable);
			}
			
			break;
		case BTN_OP_CHAT_BOT_ENABLE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnable = ::SendMessage(hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
				::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_NICK], bEnable);
				::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], bEnable);
				::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], bEnable);
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageBots::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	char sBot[65];
	int iBotLen = ::GetWindowText(hWndPageItems[EDT_HUB_BOT_NICK], sBot, 65);
	
	bool bBotHaveNewNick = (strcmp(clsSettingManager::mPtr->sTexts[SETTXT_BOT_NICK], sBot) != 0);
	bool bRegBot = ::SendMessage(hWndPageItems[BTN_HUB_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	bool bRegStateChange = false, bDescriptionChange = false, bEmailChange = false;
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] != bRegBot)
	{
		bRegStateChange = true;
	}
	
	if (strcmp(sBot, clsSettingManager::mPtr->sTexts[SETTXT_BOT_NICK]) != NULL)
	{
		bUpdateHubSec = true;
		bUpdateMOTD = true;
		bUpdateHubNameWelcome = true;
		bUpdateRegOnlyMessage = true;
		bUpdateShareLimitMessage = true;
		bUpdateSlotsLimitMessage = true;
		bUpdateHubSlotRatioMessage = true;
		bUpdateMaxHubsLimitMessage = true;
		bUpdateNoTagMessage = true;
		bUpdateNickLimitMessage = true;
		bUpdateBotsSameNick = true;
		bBotNickChanged = true;
		bUpdateBot = true;
	}
	
	if ((bUpdateBot == false || bBotNickChanged == false) && (bRegBot == true && clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false))
	{
		bUpdateBot = true;
		bBotNickChanged = true;
	}
	
	bool bHubBotIsHubSec = ::SendMessage(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	if (bHubBotIsHubSec != clsSettingManager::mPtr->bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC])
	{
		bUpdateHubSec = true;
		bUpdateMOTD = true;
		bUpdateHubNameWelcome = true;
		bUpdateRegOnlyMessage = true;
		bUpdateShareLimitMessage = true;
		bUpdateSlotsLimitMessage = true;
		bUpdateHubSlotRatioMessage = true;
		bUpdateMaxHubsLimitMessage = true;
		bUpdateNoTagMessage = true;
		bUpdateNickLimitMessage = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_USE_BOT_NICK_AS_HUB_SEC, bHubBotIsHubSec);
	
	char buf[65];
	int iLen = ::GetWindowText(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], buf, 65);
	
	if (bUpdateBot == false &&
	        ((clsSettingManager::mPtr->sTexts[SETTXT_BOT_DESCRIPTION] == NULL && iLen != 0) ||
	         (clsSettingManager::mPtr->sTexts[SETTXT_BOT_DESCRIPTION] != NULL &&
	          strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_BOT_DESCRIPTION]) != NULL)))
	{
		bUpdateBot = true;
	}
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_BOT_DESCRIPTION] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_BOT_DESCRIPTION] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_BOT_DESCRIPTION]) != NULL))
	{
		bDescriptionChange = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_BOT_DESCRIPTION, buf, iLen);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUB_BOT_EMAIL], buf, 65);
	
	if (bUpdateBot == false &&
	        (clsSettingManager::mPtr->sTexts[SETTXT_BOT_EMAIL] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_BOT_EMAIL] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_BOT_EMAIL]) != NULL))
	{
		bUpdateBot = true;
	}
	
	if ((clsSettingManager::mPtr->sTexts[SETTXT_BOT_EMAIL] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_BOT_EMAIL] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_BOT_EMAIL]) != NULL))
	{
		bEmailChange = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_BOT_EMAIL, buf, iLen);
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true)
	{
		clsSettingManager::mPtr->bUpdateLocked = false;
		clsSettingManager::mPtr->DisableBot((bBotHaveNewNick == true || bRegBot == false),
		                                    (bRegStateChange == true || bBotHaveNewNick == true || bDescriptionChange == true || bEmailChange == true) ? true : false);
		clsSettingManager::mPtr->bUpdateLocked = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_BOT_NICK, sBot, iBotLen);
	
	iBotLen = ::GetWindowText(hWndPageItems[EDT_OP_CHAT_BOT_NICK], sBot, 65);
	
	bool bRegOpChat = ::SendMessage(hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
	
	bBotHaveNewNick = (strcmp(clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK], sBot) != 0);
	
	if (bUpdateBotsSameNick == false && (bRegBot != clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] ||
	                                     bRegOpChat != clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] ||
	                                     strcmp(sBot, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]) != NULL))
	{
		bUpdateBotsSameNick = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_REG_BOT, bRegBot);
	
	if (strcmp(sBot, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK]) != NULL)
	{
		bOpChatNickChanged = true;
		bUpdateOpChat = true;
	}
	
	if ((bUpdateOpChat == false || bOpChatNickChanged == false) &&
	        (bRegOpChat == true && clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == false))
	{
		bUpdateOpChat = true;
		bOpChatNickChanged = true;
	}
	
	iLen = ::GetWindowText(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], buf, 65);
	
	if (bUpdateOpChat == false &&
	        ((clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_DESCRIPTION] == NULL && iLen != 0) ||
	         (clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_DESCRIPTION] != NULL &&
	          strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_DESCRIPTION]) != NULL)))
	{
		bUpdateOpChat = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_OP_CHAT_DESCRIPTION, buf, iLen);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], buf, 65);
	
	if (bUpdateOpChat == false &&
	        (clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_EMAIL] == NULL && iLen != 0) ||
	        (clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_EMAIL] != NULL &&
	         strcmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_EMAIL]) != NULL))
	{
		bUpdateOpChat = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_OP_CHAT_EMAIL, buf, iLen);
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true)
	{
		clsSettingManager::mPtr->bUpdateLocked = false;
		clsSettingManager::mPtr->DisableOpChat((bBotHaveNewNick == true || bRegOpChat == false));
		clsSettingManager::mPtr->bUpdateLocked = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_OP_CHAT_NICK, sBot, iBotLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_REG_OP_CHAT, bRegOpChat);
}
//------------------------------------------------------------------------------

void SettingPageBots::GetUpdates(bool &bUpdatedHubNameWelcome, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
                                 bool & /*bUpdatedAutoReg*/, bool &bUpdatedMOTD, bool &bUpdatedHubSec, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
                                 bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage,
                                 bool &bUpdatedNickLimitMessage, bool &bUpdatedBotsSameNick, bool &bUpdatedBotNick, bool &bUpdatedBot, bool &bUpdatedOpChatNick,
                                 bool &bUpdatedOpChat, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                 bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if (bUpdatedHubNameWelcome == false)
	{
		bUpdatedHubNameWelcome = bUpdateHubNameWelcome;
	}
	if (bUpdatedMOTD == false)
	{
		bUpdatedMOTD = bUpdateMOTD;
	}
	if (bUpdatedHubSec == false)
	{
		bUpdatedHubSec = bUpdateHubSec;
	}
	if (bUpdatedRegOnlyMessage == false)
	{
		bUpdatedRegOnlyMessage = bUpdateRegOnlyMessage;
	}
	if (bUpdatedShareLimitMessage == false)
	{
		bUpdatedShareLimitMessage = bUpdateShareLimitMessage;
	}
	if (bUpdatedSlotsLimitMessage == false)
	{
		bUpdatedSlotsLimitMessage = bUpdateSlotsLimitMessage;
	}
	if (bUpdatedHubSlotRatioMessage == false)
	{
		bUpdatedHubSlotRatioMessage = bUpdateHubSlotRatioMessage;
	}
	if (bUpdatedMaxHubsLimitMessage == false)
	{
		bUpdatedMaxHubsLimitMessage = bUpdateMaxHubsLimitMessage;
	}
	if (bUpdatedNoTagMessage == false)
	{
		bUpdatedNoTagMessage = bUpdateNoTagMessage;
	}
	if (bUpdatedNickLimitMessage == false)
	{
		bUpdatedNickLimitMessage = bUpdateNickLimitMessage;
	}
	if (bUpdatedBotsSameNick == false)
	{
		bUpdatedBotsSameNick = bUpdateBotsSameNick;
	}
	if (bUpdatedBotNick == false)
	{
		bUpdatedBotNick = bBotNickChanged;
	}
	if (bUpdatedBot == false)
	{
		bUpdatedBot = bUpdateBot;
	}
	if (bUpdatedOpChatNick == false)
	{
		bUpdatedOpChatNick = bOpChatNickChanged;
	}
	if (bUpdatedOpChat == false)
	{
		bUpdatedOpChat = bUpdateOpChat;
	}
}

//------------------------------------------------------------------------------
bool SettingPageBots::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	int iPosY = clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iOneLineOneChecksGB + (2 * clsGuiSettingManager::iOneLineGB) + 1 + 5;
	
	hWndPageItems[GB_HUB_BOT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_BOT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                             0, 0, iFullGB, iPosY, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                             
	hWndPageItems[BTN_HUB_BOT_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_AND_REG_BOT_ON_HUB], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                     8, clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_HUB_BOT_ENABLE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_HUB_BOT_ENABLE], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[GB_HUB_BOT_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1, iGBinGB, clsGuiSettingManager::iOneLineOneChecksGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_HUB_BOT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_BOT_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                   13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_NICK, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_BOT_NICK], EM_SETLIMITTEXT, 64, 0);
	
	hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_USE_AS_ALIAS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                         13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iEditHeight + 4, iGBinGBEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[GB_HUB_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                         5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineOneChecksGB, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                         
	hWndPageItems[EDT_HUB_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_BOT_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                          13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineOneChecksGB, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_DESCRIPTION, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], EM_SETLIMITTEXT, 64, 0);
	
	hWndPageItems[GB_HUB_BOT_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                   5, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineOneChecksGB + clsGuiSettingManager::iOneLineGB, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[EDT_HUB_BOT_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_BOT_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                    13, (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineOneChecksGB + clsGuiSettingManager::iOneLineGB, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_EMAIL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_BOT_EMAIL], EM_SETLIMITTEXT, 64, 0);
	
	hWndPageItems[GB_OP_CHAT_BOT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_OP_CHAT_BOT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 0, iPosY, iFullGB, clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + (3 * clsGuiSettingManager::iOneLineGB) + 1 + 5, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[BTN_OP_CHAT_BOT_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ENABLE_AND_REG_BOT_ON_HUB], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                         8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_OP_CHAT_BOT_ENABLE, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	hWndPageItems[GB_OP_CHAT_BOT_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                      5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                      
	hWndPageItems[EDT_OP_CHAT_BOT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                       13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_NICK, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_OP_CHAT_BOT_NICK], EM_SETLIMITTEXT, 64, 0);
	
	hWndPageItems[GB_OP_CHAT_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                             5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineGB, iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                             
	hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                                              ES_AUTOHSCROLL, 13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + clsGuiSettingManager::iOneLineGB, iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_DESCRIPTION, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], EM_SETLIMITTEXT, 64, 0);
	
	hWndPageItems[GB_OP_CHAT_BOT_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                       5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 1 + (2 * clsGuiSettingManager::iOneLineGB), iGBinGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                       
	hWndPageItems[EDT_OP_CHAT_BOT_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_OP_CHAT_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                        13, iPosY + (2 * clsGuiSettingManager::iGroupBoxMargin) + clsGuiSettingManager::iCheckHeight + 1 + (2 * clsGuiSettingManager::iOneLineGB), iGBinGBEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_EMAIL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], EM_SETLIMITTEXT, 64, 0);
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(hWndPageItems[EDT_HUB_BOT_NICK], clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[EDT_HUB_BOT_EMAIL], clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
	
	::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_NICK], clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);
	::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], clsSettingManager::mPtr->bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);
	
	clsGuiSettingManager::wpOldEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], GWLP_WNDPROC, (LONG_PTR)EditProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageBots::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_DEFAULT_BOTS];
}
//------------------------------------------------------------------------------

void SettingPageBots::FocusLastItem()
{
	::SetFocus(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL]);
}
//------------------------------------------------------------------------------
