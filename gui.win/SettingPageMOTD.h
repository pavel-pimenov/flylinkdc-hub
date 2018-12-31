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
#ifndef SettingPageMOTDH
#define SettingPageMOTDH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageMOTD : public SettingPage
{
public:
	bool bUpdateMOTD;
	
	SettingPageMOTD();
	~SettingPageMOTD() { };
	
	bool CreateSettingPage(HWND hOwner);
	
	void Save();
	void GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
	                bool & /*bUpdatedAutoReg*/, bool &bUpdatedMOTD, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
	                bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
	                bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
	                bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
	                bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);
	                
	char * GetPageName();
	void FocusLastItem();
private:
	HWND hWndPageItems[4];
	
	enum enmPageItems
	{
		GB_MOTD,
		EDT_MOTD,
		BTN_MOTD_AS_PM,
		BTN_DISABLE_MOTD
	};
	
	SettingPageMOTD(const SettingPageMOTD&);
	const SettingPageMOTD& operator=(const SettingPageMOTD&);
	
	LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
