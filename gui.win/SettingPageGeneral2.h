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
#ifndef SettingPageGeneral2H
#define SettingPageGeneral2H
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageGeneral2 : public SettingPage {
public:
	bool m_bUpdateTextFiles, m_bUpdateRedirectAddress, m_bUpdateRegOnlyMessage, m_bUpdateShareLimitMessage, m_bUpdateSlotsLimitMessage, m_bUpdateHubSlotRatioMessage,
	     m_bUpdateMaxHubsLimitMessage, m_bUpdateNoTagMessage, m_bUpdateTempBanRedirAddress, m_bUpdatePermBanRedirAddress, m_bUpdateNickLimitMessage;

	SettingPageGeneral2();
	~SettingPageGeneral2() { };

	bool CreateSettingPage(HWND hOwner);

	void Save();
	void GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/, bool & /*bUpdatedAutoReg*/,
	                bool &/*bUpdatedMOTD*/, bool &/*bUpdatedHubSec*/, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
	                bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage,
	                bool &bUpdatedNickLimitMessage, bool &/*bUpdatedBotsSameNick*/, bool &/*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool &/*bUpdatedOpChatNick*/,
	                bool &/*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool &bUpdatedTextFiles, bool &bUpdatedRedirectAddress, bool &bUpdatedTempBanRedirAddress,
	                bool &bUpdatedPermBanRedirAddress, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);

	char * GetPageName();
	void FocusLastItem();
private:
	HWND m_hWndPageItems[23];

	enum enmPageItems {
		GB_TEXT_FILES,
		BTN_ENABLE_TEXT_FILES,
		BTN_SEND_TEXT_FILES_AS_PM,
		GB_PINGER,
		BTN_DONT_ALLOW_PINGER,
		BTN_REPORT_PINGER,
		GB_OWNER_EMAIL,
		EDT_OWNER_EMAIL,
		GB_MAIN_REDIR_ADDR,
		EDT_MAIN_REDIR_ADDR,
		BTN_REDIR_ALL,
		BTN_REDIR_HUB_FULL,
		GB_REG_ONLY_HUB,
		BTN_ALLOW_ONLY_REGS,
		GB_MSG_TO_NON_REGS,
		EDT_MSG_TO_NON_REGS,
		GB_NON_REG_REDIR,
		BTN_NON_REG_REDIR_ENABLE,
		EDT_NON_REG_REDIR_ADDR,
		GB_EXPERTS_ONLY,
		BTN_KEEP_SLOW_CLIENTS_ONLINE,
		BTN_HASH_PASSWORDS,
		BTN_KILL_THAT_DUCK,
	};

	SettingPageGeneral2(const SettingPageGeneral2&) = delete;
	const SettingPageGeneral2& operator=(const SettingPageGeneral2&) = delete;

	LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
