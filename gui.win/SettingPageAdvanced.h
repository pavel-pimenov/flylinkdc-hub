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
#ifndef SettingPageAdvancedH
#define SettingPageAdvancedH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageAdvanced : public SettingPage
{
public:
	bool bUpdateSysTray, bUpdateScripting;
	
	SettingPageAdvanced();
	~SettingPageAdvanced() { };
	
	bool CreateSettingPage(HWND hOwner);
	
	void Save();
	void GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/, bool & /*bUpdatedAutoReg*/,
	                bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
	                bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
	                bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
	                bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
	                bool & /*bUpdatedPermBanRedirAddress*/, bool &bUpdatedSysTray, bool &bUpdatedScripting, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);
	                
	char * GetPageName();
	void FocusLastItem();
private:
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
	HWND hWndPageItems[26];
#else
	HWND hWndPageItems[21];
#endif
	
	enum enmPageItems
	{
		GB_HUB_STARTUP_AND_TRAY,
		BTN_AUTO_START,
		BTN_CHECK_FOR_UPDATE,
		BTN_ENABLE_TRAY_ICON,
		BTN_MINIMIZE_ON_STARTUP,
		GB_HUB_COMMANDS,
		LBL_PREFIXES_FOR_HUB_COMMANDS,
		EDT_PREFIXES_FOR_HUB_COMMANDS,
		BTN_REPLY_TO_HUB_COMMANDS_IN_PM,
		GB_SCRIPTING,
		BTN_ENABLE_SCRIPTING,
		BTN_STOP_SCRIPT_ON_ERROR,
		BTN_SAVE_SCRIPT_ERRORS_TO_LOG,
		GB_KICK_MESSAGES,
		BTN_FILTER_KICK_MESSAGES,
		BTN_SEND_KICK_MESSAGES_TO_OPS,
		GB_STATUS_MESSAGES,
		BTN_SEND_STATUS_MESSAGES_TO_OPS,
		BTN_SEND_STATUS_MESSAGES_IN_PM,
		GB_ADMIN_NICK,
		EDT_ADMIN_NICK,
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		GB_DATABASE_SUPPORT,
		CHK_ENABLE_DATABASE,
		LBL_REMOVE_OLD_RECORDS,
		EDT_REMOVE_OLD_RECORDS,
		UD_REMOVE_OLD_RECORDS,
#endif
	};
	
	SettingPageAdvanced(const SettingPageAdvanced&);
	const SettingPageAdvanced& operator=(const SettingPageAdvanced&);
	
	LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
