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
#ifndef SettingPageRulesH
#define SettingPageRulesH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageRules : public SettingPage
{
public:
	bool bUpdateNickLimitMessage, bUpdateMinShare, bUpdateMaxShare, bUpdateShareLimitMessage;
	
	SettingPageRules();
	~SettingPageRules() { };
	
	bool CreateSettingPage(HWND hOwner);
	
	void Save();
	void GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
	                bool & /*bUpdatedAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool &bUpdatedShareLimitMessage,
	                bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
	                bool &bUpdatedNickLimitMessage, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
	                bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
	                bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool &bUpdatedMinShare, bool &bUpdatedMaxShare);
	                
	char * GetPageName();
	void FocusLastItem();
private:
	HWND hWndPageItems[47];
	
	enum enmPageItems
	{
		GB_NICK_LIMITS,
		EDT_MIN_NICK_LEN,
		UD_MIN_NICK_LEN,
		LBL_MIN_NICK_LEN,
		LBL_MAX_NICK_LEN,
		EDT_MAX_NICK_LEN,
		UD_MAX_NICK_LEN,
		GB_NICK_LEN_MSG,
		EDT_NICK_LEN_MSG,
		GB_NICK_LEN_REDIR,
		BTN_NICK_LEN_REDIR,
		EDT_NICK_LEN_REDIR_ADDR,
		GB_SHARE_LIMITS,
		EDT_MIN_SHARE,
		UD_MIN_SHARE,
		CB_MIN_SHARE,
		LBL_MIN_SHARE,
		LBL_MAX_SHARE,
		EDT_MAX_SHARE,
		UD_MAX_SHARE,
		CB_MAX_SHARE,
		GB_SHARE_MSG,
		EDT_SHARE_MSG,
		GB_SHARE_REDIR,
		BTN_SHARE_REDIR,
		EDT_SHARE_REDIR_ADDR,
		GB_MAIN_CHAT_LIMITS,
		EDT_MAIN_CHAT_LEN,
		UD_MAIN_CHAT_LEN,
		LBL_MAIN_CHAT_LEN,
		LBL_MAIN_CHAT_LINES,
		EDT_MAIN_CHAT_LINES,
		UD_MAIN_CHAT_LINES,
		GB_PM_LIMITS,
		EDT_PM_LEN,
		UD_PM_LEN,
		LBL_PM_LEN,
		LBL_PM_LINES,
		EDT_PM_LINES,
		UD_PM_LINES,
		GB_SEARCH_LIMITS,
		EDT_SEARCH_MIN_LEN,
		UD_SEARCH_MIN_LEN,
		LBL_SEARCH_MIN,
		LBL_SEARCH_MAX,
		EDT_SEARCH_MAX_LEN,
		UD_SEARCH_MAX_LEN
	};
	
	SettingPageRules(const SettingPageRules&);
	const SettingPageRules& operator=(const SettingPageRules&);
	
	LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
