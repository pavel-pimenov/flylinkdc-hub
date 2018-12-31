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
#ifndef SettingPageH
#define SettingPageH
//------------------------------------------------------------------------------

class SettingPage
{
public:
	HWND m_hWnd;
	
	static int iFullGB;
	static int iFullEDT;
	static int iGBinGB;
	static int iGBinGBEDT;
	static int iOneCheckGB;
	static int iTwoChecksGB;
	static int iOneLineTwoGroupGB;
	static int iTwoLineGB;
	static int iThreeLineGB;
	
	bool bCreated;
	
	SettingPage();
	virtual ~SettingPage() { };
	
	static LRESULT CALLBACK StaticSettingPageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	virtual bool CreateSettingPage(HWND hOwner) = 0;
	
	virtual void Save() = 0;
	virtual void GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
	                        bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
	                        bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
	                        bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
	                        bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
	                        bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) = 0;
	                        
	virtual char * GetPageName() = 0;
	virtual void FocusLastItem() = 0;
protected:
	void CreateHWND(HWND hOwner);
	static void RemoveDollarsPipes(HWND hWnd);
	static void RemovePipes(HWND hWnd);
	static void MinMaxCheck(HWND hWnd, const int iMin, const int iMax);
	void AddUpDown(HWND &hWnd, const int iX, const int iY, const int iWidth, const int iHeight, const LPARAM &lpRange, const WPARAM &wpBuddy, const LPARAM &lpPos);
	void AddToolTip(const HWND hWnd, char * sTipText) const;
private:
	SettingPage(const SettingPage&);
	const SettingPage& operator=(const SettingPage&);
	
	virtual LRESULT SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};
//------------------------------------------------------------------------------

LRESULT CALLBACK ButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NumberEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//------------------------------------------------------------------------------

#endif
