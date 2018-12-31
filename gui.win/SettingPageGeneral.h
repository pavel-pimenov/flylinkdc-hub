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
#ifndef SettingPageGeneralH
#define SettingPageGeneralH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingPageGeneral : public SettingPage {
public:
    bool m_bUpdateHubNameWelcome, m_bUpdateHubName, m_bUpdateTCPPorts, m_bUpdateUDPPort, m_bUpdateAutoReg, m_bUpdateLanguage;

    SettingPageGeneral();
    ~SettingPageGeneral() { };

    bool CreateSettingPage(HWND hOwner);

    void Save();
    void GetUpdates(bool &bUpdatedHubNameWelcome, bool &bUpdatedHubName, bool &bUpdatedTCPPorts, bool &bUpdatedUDPPort, bool &bUpdatedAutoReg,
        bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
        bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
        bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
        bool & /*bUpdatedOpChat*/, bool &bUpdatedLanguage, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
        bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/);

    char * GetPageName();
    void FocusLastItem();
private:
    HWND m_hWndPageItems[27];
    
    enum enmPageItems {
        GB_LANGUAGE,
        CB_LANGUAGE,
        GB_MAX_USERS,
        EDT_MAX_USERS,
        UD_MAX_USERS,
        GB_HUB_NAME,
        EDT_HUB_NAME,
        GB_HUB_TOPIC,
        EDT_HUB_TOPIC,
        GB_HUB_DESCRIPTION,
        EDT_HUB_DESCRIPTION,
        BTN_ANTI_MOGLO,
        GB_HUB_ADDRESS,
        EDT_HUB_ADDRESS,
        BTN_RESOLVE_ADDRESS,
        GB_IPV4_ADDRESS,
        EDT_IPV4_ADDRESS,
        GB_IPV6_ADDRESS,
        EDT_IPV6_ADDRESS,
        BTN_BIND_ADDRESS,
        GB_TCP_PORTS,
        EDT_TCP_PORTS,
        GB_UDP_PORT,
        EDT_UDP_PORT,
        GB_HUB_LISTS,
        EDT_HUB_LISTS,
        BTN_HUBLIST_AUTO_REG
    };

    SettingPageGeneral(const SettingPageGeneral&) = delete;
    const SettingPageGeneral& operator=(const SettingPageGeneral&) = delete;

    LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
