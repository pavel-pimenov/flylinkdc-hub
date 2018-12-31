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

//------------------------------------------------------------------------------
#ifndef SettingPageBotsH
#define SettingPageBotsH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageBots : public SettingPage
{
public:
	bool bUpdateHubSec, bUpdateMOTD, bUpdateHubNameWelcome, bUpdateRegOnlyMessage, bUpdateShareLimitMessage, bUpdateSlotsLimitMessage,
	     bUpdateHubSlotRatioMessage, bUpdateMaxHubsLimitMessage, bUpdateNoTagMessage, bUpdateNickLimitMessage, bUpdateBotsSameNick, bBotNickChanged,
	     bUpdateBot, bOpChatNickChanged, bUpdateOpChat;
	     
	SettingPageBots();
	~SettingPageBots() { };
	
	bool CreateSettingPage(HWND hOwner);
	
	void Save();
	void GetUpdates(bool &bUpdatedHubNameWelcome, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/, bool & /*bUpdatedAutoReg*/,
	                bool &bUpdatedMOTD, bool &bUpdatedHubSec, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
	                bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage,
	                bool &bUpdatedNickLimitMessage, bool &bUpdatedBotsSameNick, bool &bUpdatedBotNick, bool &bUpdatedBot, bool &bUpdatedOpChatNick,
	                bool &bUpdatedOpChat, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
	                bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);
	                
	char * GetPageName();
	void FocusLastItem();
private:
	HWND hWndPageItems[17];
	
	enum enmPageItems
	{
		GB_HUB_BOT,
		BTN_HUB_BOT_ENABLE,
		GB_HUB_BOT_NICK,
		EDT_HUB_BOT_NICK,
		BTN_HUB_BOT_IS_HUB_SEC,
		GB_HUB_BOT_DESCRIPTION,
		EDT_HUB_BOT_DESCRIPTION,
		GB_HUB_BOT_EMAIL,
		EDT_HUB_BOT_EMAIL,
		GB_OP_CHAT_BOT,
		BTN_OP_CHAT_BOT_ENABLE,
		GB_OP_CHAT_BOT_NICK,
		EDT_OP_CHAT_BOT_NICK,
		GB_OP_CHAT_BOT_DESCRIPTION,
		EDT_OP_CHAT_BOT_DESCRIPTION,
		GB_OP_CHAT_BOT_EMAIL,
		EDT_OP_CHAT_BOT_EMAIL
	};
	
	SettingPageBots(const SettingPageBots&);
	const SettingPageBots& operator=(const SettingPageBots&);
	
	LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
