/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2022  Petr Kozelka, PPK at PtokaX dot org

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
#ifndef SettingPageBansH
#define SettingPageBansH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageBans : public SettingPage
{
public:
	bool m_bUpdateTempBanRedirAddress, m_bUpdatePermBanRedirAddress;
	
	SettingPageBans();
	~SettingPageBans() { };
	
	bool CreateSettingPage(HWND hOwner) override;
	
	void Save() override;
	void GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
	                bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
	                bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
	                bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
	                bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & bUpdatedTempBanRedirAddress,
	                bool & bUpdatedPermBanRedirAddress, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) override;
	                
	char * GetPageName() override;
	void FocusLastItem() override;
private:
	HWND m_hWndPageItems[27];
	
	enum enmPageItems
	{
		GB_DEFAULT_TEMPBAN_TIME,
		EDT_DEFAULT_TEMPBAN_TIME,
		UD_DEFAULT_TEMPBAN_TIME,
		LBL_DEFAULT_TEMPBAN_TIME_MINUTES,
		GB_BAN_MESSAGE,
		BTN_SHOW_IP,
		BTN_SHOW_RANGE,
		BTN_SHOW_NICK,
		BTN_SHOW_REASON,
		BTN_SHOW_BY,
		GB_ADD_MESSAGE,
		EDT_ADD_MESSAGE,
		GB_TEMP_BAN_REDIR,
		BTN_TEMP_BAN_REDIR_ENABLE,
		EDT_TEMP_BAN_REDIR,
		GB_PERM_BAN_REDIR,
		BTN_PERM_BAN_REDIR_ENABLE,
		EDT_PERM_BAN_REDIR,
		GB_PASSWORD_PROTECTION,
		BTN_ENABLE_ADVANCED_PASSWORD_PROTECTION,
		GB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION,
		CB_BRUTE_FORCE_PASSWORD_PROTECTION_ACTION,
		LBL_TEMP_BAN_TIME,
		EDT_TEMP_BAN_TIME,
		UD_TEMP_BAN_TIME,
		LBL_TEMP_BAN_TIME_HOURS,
		BTN_REPORT_3X_BAD_PASS
	};
	
	SettingPageBans(const SettingPageBans&) = delete;
	const SettingPageBans& operator=(const SettingPageBans&) = delete;
	
	LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};
//------------------------------------------------------------------------------

#endif
