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

//---------------------------------------------------------------------------
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "SettingPageGeneral.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------

SettingPageGeneral::SettingPageGeneral() : m_bUpdateHubNameWelcome(false), m_bUpdateHubName(false), m_bUpdateTCPPorts(false), m_bUpdateUDPPort(false), m_bUpdateAutoReg(false), m_bUpdateLanguage(false)
{
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageGeneral::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_COMMAND)
	{
		switch(LOWORD(wParam))
		{
		case EDT_HUB_NAME:
		case EDT_HUB_TOPIC:
		case EDT_HUB_DESCRIPTION:
		case EDT_HUB_ADDRESS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				RemoveDollarsPipes((HWND)lParam);

				return 0;
			}

			break;
		case EDT_TCP_PORTS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);

				bool bChanged = false;

				for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++)
				{
					if(isdigit(buf[ui16i]) == 0 && buf[ui16i] != ';')
					{
						strcpy(buf+ui16i, buf+ui16i+1);
						bChanged = true;
						ui16i--;
					}
				}

				if(bChanged == true)
				{
					int iStart, iEnd;

					::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

					::SetWindowText((HWND)lParam, buf);

					::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
				}

				return 0;
			}

			break;
		case EDT_UDP_PORT:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 65535);

				return 0;
			}

			break;
		case EDT_MAX_USERS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 32767);

				return 0;
			}

			break;
		case BTN_RESOLVE_ADDRESS:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnable = ::SendMessage(m_hWndPageItems[BTN_RESOLVE_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
				::SetWindowText(m_hWndPageItems[EDT_IPV4_ADDRESS], bEnable == FALSE ? ServerManager::m_sHubIP :
				                (SettingManager::m_Ptr->m_sTexts[SETTXT_IPV4_ADDRESS] != nullptr ? SettingManager::m_Ptr->m_sTexts[SETTXT_IPV4_ADDRESS] : ""));
				::EnableWindow(m_hWndPageItems[EDT_IPV4_ADDRESS], bEnable);
				::SetWindowText(m_hWndPageItems[EDT_IPV6_ADDRESS], bEnable == FALSE ? ServerManager::m_sHubIP6 :
				                (SettingManager::m_Ptr->m_sTexts[SETTXT_IPV6_ADDRESS] != nullptr ? SettingManager::m_Ptr->m_sTexts[SETTXT_IPV6_ADDRESS] : ""));
				::EnableWindow(m_hWndPageItems[EDT_IPV6_ADDRESS], bEnable);
			}

			break;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageGeneral::Save()
{
	if(m_bCreated == false)
	{
		return;
	}

	char sBuf[1025];
	int iLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_NAME], sBuf, 1025);

	if(strcmp(sBuf, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME]) != 0)
	{
		m_bUpdateHubNameWelcome = true;
		m_bUpdateHubName = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_HUB_NAME, sBuf, iLen);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_TOPIC], sBuf, 1025);

	if(m_bUpdateHubName == false && ((SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_TOPIC] == nullptr && iLen != 0) ||
	                                 (SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_TOPIC] != nullptr && strcmp(sBuf, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_TOPIC]) != 0)))
	{
		m_bUpdateHubNameWelcome = true;
		m_bUpdateHubName = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_HUB_TOPIC, sBuf, iLen);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_DESCRIPTION], sBuf, 1025);
	SettingManager::m_Ptr->SetText(SETTXT_HUB_DESCRIPTION, sBuf, iLen);

	SettingManager::m_Ptr->SetBool(SETBOOL_ANTI_MOGLO, ::SendMessage(m_hWndPageItems[BTN_ANTI_MOGLO], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	bool bResolveHubAddress = false;

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_ADDRESS], sBuf, 1025);

	if(strcmp(sBuf, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS]) != 0)
	{
		bResolveHubAddress = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_HUB_ADDRESS, sBuf, iLen);

	bool bOldResolve = SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP];

	SettingManager::m_Ptr->SetBool(SETBOOL_RESOLVE_TO_IP, ::SendMessage(m_hWndPageItems[BTN_RESOLVE_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	if(bOldResolve != SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP])
	{
		bResolveHubAddress = true;
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP] == false)
	{
		iLen = ::GetWindowText(m_hWndPageItems[EDT_IPV4_ADDRESS], sBuf, 1025);
		SettingManager::m_Ptr->SetText(SETTXT_IPV4_ADDRESS, sBuf, iLen);

		iLen = ::GetWindowText(m_hWndPageItems[EDT_IPV6_ADDRESS], sBuf, 1025);
		SettingManager::m_Ptr->SetText(SETTXT_IPV6_ADDRESS, sBuf, iLen);
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_BIND_ONLY_SINGLE_IP, ::SendMessage(m_hWndPageItems[BTN_BIND_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_TCP_PORTS], sBuf, 1025);

	if(strcmp(sBuf, SettingManager::m_Ptr->m_sTexts[SETTXT_TCP_PORTS]) != 0)
	{
		m_bUpdateTCPPorts = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_TCP_PORTS, sBuf, iLen);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_UDP_PORT], sBuf, 1025);

	if(strcmp(sBuf, SettingManager::m_Ptr->m_sTexts[SETTXT_UDP_PORT]) != 0)
	{
		m_bUpdateUDPPort = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_UDP_PORT, sBuf, iLen);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUB_LISTS], sBuf, 1025);
	SettingManager::m_Ptr->SetText(SETTXT_REGISTER_SERVERS, sBuf, iLen);

	if((::SendMessage(m_hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG])
	{
		m_bUpdateAutoReg = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_AUTO_REG, ::SendMessage(m_hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

	if(bResolveHubAddress == true)
	{
		ServerManager::ResolveHubAddress(true);
	}

	uint32_t ui32CurSel = (uint32_t)::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_GETCURSEL, 0, 0);

	if(ui32CurSel == 0)
	{
		if(SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE] != nullptr)
		{
			m_bUpdateLanguage = true;
			m_bUpdateHubNameWelcome = true;
		}

		SettingManager::m_Ptr->SetText(SETTXT_LANGUAGE, "", 0);
	}
	else
	{
		uint32_t ui32Len = (uint32_t)::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_GETLBTEXTLEN, ui32CurSel, 0);

		char * sTempBuf = (char *)malloc(ui32Len+1);

		if(sTempBuf == nullptr)
		{
			AppendDebugLogFormat("[MEM] Cannot allocate %u bytes for sTempBuf in SettingPageGeneral::Save\n", ui32Len+1);
			return;
		}

		if(SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE] == nullptr || strcmp(sTempBuf, SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE]) != 0)
		{
			m_bUpdateLanguage = true;
			m_bUpdateHubNameWelcome = true;
		}

		::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_GETLBTEXT, ui32CurSel, (LPARAM)sTempBuf);

		SettingManager::m_Ptr->SetText(SETTXT_LANGUAGE, sTempBuf, ui32Len);

		free(sTempBuf);
	}

	LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_MAX_USERS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_USERS, LOWORD(lResult));
	}
}
//------------------------------------------------------------------------------

void SettingPageGeneral::GetUpdates(bool &bUpdatedHubNameWelcome, bool &bUpdatedHubName, bool &bUpdatedTCPPorts, bool &bUpdatedUDPPort,
                                    bool &bUpdatedAutoReg, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                    bool & /*bUpdatedOpChat*/, bool &bUpdatedLanguage, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if(bUpdatedHubNameWelcome == false)
	{
		bUpdatedHubNameWelcome = m_bUpdateHubNameWelcome;
	}
	if(bUpdatedHubName == false)
	{
		bUpdatedHubName = m_bUpdateHubName;
	}
	if(bUpdatedTCPPorts == false)
	{
		bUpdatedTCPPorts = m_bUpdateTCPPorts;
	}
	if(bUpdatedUDPPort == false)
	{
		bUpdatedUDPPort = m_bUpdateUDPPort;
	}
	if(bUpdatedAutoReg == false)
	{
		bUpdatedAutoReg = m_bUpdateAutoReg;
	}
	if(bUpdatedLanguage == false)
	{
		bUpdatedLanguage = m_bUpdateLanguage;
	}
}

//------------------------------------------------------------------------------
bool SettingPageGeneral::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);

	if(m_bCreated == false)
	{
		return false;
	}

	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);

	m_iFullGB = (rcThis.right - rcThis.left) - 5;
	m_iFullEDT = (rcThis.right - rcThis.left) - 21;
	m_iGBinGB = (rcThis.right - rcThis.left) - 15;
	m_iGBinGBEDT = (rcThis.right - rcThis.left) - 31;

	int iPosY = GuiSettingManager::m_iOneLineGB;

	int iHeight = m_iTwoChecksGB + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 4 + GuiSettingManager::m_iOneLineGB + 5 + GuiSettingManager::m_iOneLineTwoChecksGB + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + (2 * GuiSettingManager::m_iOneLineGB) + 6 +
	              GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iCheckHeight) + 14 + 3;

	iHeight = iHeight > ((3 * m_iOneLineTwoGroupGB) + GuiSettingManager::m_iOneLineGB + 3) ? iHeight : ((3 * m_iOneLineTwoGroupGB) + GuiSettingManager::m_iOneLineGB + 3);

	::SetWindowPos(m_hWnd, nullptr, 0, 0, rcThis.right, iHeight, SWP_NOMOVE | SWP_NOZORDER);

	m_hWndPageItems[GB_LANGUAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_LANGUAGE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, ScaleGui(297), GuiSettingManager::m_iOneLineGB,
	                               m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[CB_LANGUAGE] = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST, 8, GuiSettingManager::m_iGroupBoxMargin, ScaleGui(297) - 16, GuiSettingManager::m_iEditHeight,
	                               m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[GB_MAX_USERS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MAX_USERS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                ScaleGui(297) + 5, 0, (rcThis.right - rcThis.left) - (ScaleGui(297) + 10), GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAX_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 ScaleGui(297) + 13, GuiSettingManager::m_iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(297) + GuiSettingManager::m_iUpDownWidth + 26), GuiSettingManager::m_iEditHeight,
	                                 m_hWnd, (HMENU)EDT_MAX_USERS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAX_USERS], EM_SETLIMITTEXT, 5, 0);

	AddUpDown(m_hWndPageItems[UD_MAX_USERS], (rcThis.right - rcThis.left)-GuiSettingManager::m_iUpDownWidth-13, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 1),
	          (WPARAM)m_hWndPageItems[EDT_MAX_USERS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS], 0));

	m_hWndPageItems[GB_HUB_NAME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_NAME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                               0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_NAME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                8, GuiSettingManager::m_iOneLineGB + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUB_NAME, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_NAME], EM_SETLIMITTEXT, 256, 0);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_HUB_TOPIC] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_TOPIC], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_TOPIC] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_TOPIC], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUB_TOPIC, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_TOPIC], EM_SETLIMITTEXT, 256, 0);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                      0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineOneChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                       8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUB_DESCRIPTION, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_DESCRIPTION], EM_SETLIMITTEXT, 256, 0);

	m_hWndPageItems[BTN_ANTI_MOGLO] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ANTI_MOGLO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                  8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_ANTI_MOGLO], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_ANTI_MOGLO] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	iPosY += GuiSettingManager::m_iOneLineOneChecksGB;

	m_hWndPageItems[GB_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                  0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineTwoChecksGB + GuiSettingManager::m_iOneLineGB + 3, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                   8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUB_ADDRESS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_ADDRESS], EM_SETLIMITTEXT, 256, 0);

	m_hWndPageItems[BTN_RESOLVE_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_RESOLVE_HOSTNAME], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                       8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_RESOLVE_ADDRESS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_RESOLVE_ADDRESS], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	int iFourtyPercent = (int)(m_iGBinGB * 0.4);
	m_hWndPageItems[GB_IPV4_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_IPV4_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                   5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iCheckHeight + 5, iFourtyPercent - 3, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_IPV4_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP] == true ? ServerManager::m_sHubIP :
	                                    (SettingManager::m_Ptr->m_sTexts[SETTXT_IPV4_ADDRESS] != nullptr ? SettingManager::m_Ptr->m_sTexts[SETTXT_IPV4_ADDRESS] : nullptr),
	                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iCheckHeight + 5 + GuiSettingManager::m_iGroupBoxMargin, iFourtyPercent - 19, GuiSettingManager::m_iEditHeight,
	                                    m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_IPV4_ADDRESS], EM_SETLIMITTEXT, 15, 0);

	m_hWndPageItems[GB_IPV6_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_IPV6_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                   10 + (iFourtyPercent - 3), iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iCheckHeight + 5, (m_iGBinGB - (iFourtyPercent - 3)) - 5, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_IPV6_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP] == true ? ServerManager::m_sHubIP6 :
	                                    (SettingManager::m_Ptr->m_sTexts[SETTXT_IPV6_ADDRESS] != nullptr ? SettingManager::m_Ptr->m_sTexts[SETTXT_IPV6_ADDRESS] : nullptr),
	                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 18 + (iFourtyPercent - 3), iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iCheckHeight + 5 + GuiSettingManager::m_iGroupBoxMargin,
	                                    (m_iGBinGB - (iFourtyPercent - 3)) - 21, GuiSettingManager::m_iEditHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_IPV6_ADDRESS], EM_SETLIMITTEXT, 39, 0);

	m_hWndPageItems[BTN_BIND_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_BIND_ONLY_ADDRS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                    8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iOneLineGB + 10, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_BIND_ADDRESS], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	iPosY += GuiSettingManager::m_iOneLineTwoChecksGB + GuiSettingManager::m_iOneLineGB + 3;

	m_hWndPageItems[GB_TCP_PORTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_TCP_PORTS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                0, iPosY, ScaleGui(362), GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_TCP_PORTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_TCP_PORTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(362) - 16, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_TCP_PORTS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_TCP_PORTS], EM_SETLIMITTEXT, 64, 0);
	AddToolTip(m_hWndPageItems[EDT_TCP_PORTS], LanguageManager::m_Ptr->m_sTexts[LAN_TCP_PORTS_HINT]);

	m_hWndPageItems[GB_UDP_PORT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_UDP_PORT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                               ScaleGui(362) + 5, iPosY, (rcThis.right - rcThis.left) - (ScaleGui(362) + 10), GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_UDP_PORT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_UDP_PORT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
	                                ScaleGui(362) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(362) + 26), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_UDP_PORT, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_UDP_PORT], EM_SETLIMITTEXT, 5, 0);
	AddToolTip(m_hWndPageItems[EDT_UDP_PORT], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_DISABLED]);

	iPosY += GuiSettingManager::m_iOneLineGB;

	m_hWndPageItems[GB_HUB_LISTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_REG_ADRS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineOneChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUB_LISTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_REGISTER_SERVERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iEditHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUB_LISTS], EM_SETLIMITTEXT, 1024, 0);
	AddToolTip(m_hWndPageItems[EDT_HUB_LISTS], LanguageManager::m_Ptr->m_sTexts[LAN_HUB_LIST_REGS_HINT]);

	m_hWndPageItems[BTN_HUBLIST_AUTO_REG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_AUTO_REG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++)
	{
		if(m_hWndPageItems[ui8i] == nullptr)
		{
			return false;
		}

		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	// add default language and select it
	::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_ADDSTRING, 0, (LPARAM)"Default English");
	::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_SETCURSEL, 0, 0);

	// add all languages found in language dir
	struct _finddata_t langfile;
	intptr_t hFile = _findfirst((ServerManager::m_sPath+"\\language\\"+"*.xml").c_str(), &langfile);

	if(hFile != -1)
	{
		do
		{
			if(((langfile.attrib & _A_SUBDIR) == _A_SUBDIR) == true ||
			        stricmp(langfile.name+(strlen(langfile.name)-4), ".xml") != 0)
			{
				continue;
			}

			::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_ADDSTRING, 0, (LPARAM)string(langfile.name, strlen(langfile.name)-4).c_str());
		}
		while(_findnext(hFile, &langfile) == 0);

		_findclose(hFile);
	}

	// Select actual language in combobox
	if(SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE] != nullptr)
	{
		uint32_t ui32Count = (uint32_t)::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_GETCOUNT, 0, 0);
		for(uint32_t ui32i = 1; ui32i < ui32Count; ui32i++)
		{
			uint32_t ui32Len = (uint32_t)::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_GETLBTEXTLEN, ui32i, 0);
			if(ui32Len == (int32_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_LANGUAGE])
			{
				char * buf = (char *)malloc(ui32Len+1);

				if(buf == nullptr)
				{
					AppendDebugLogFormat("[MEM] Cannot allocate %u bytes for buf in SettingPageGeneral::CreateSettingPage\n", ui32Len+1);
					return false;
				}

				::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_GETLBTEXT, ui32i, (LPARAM)buf);
				if(stricmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE]) == 0)
				{
					::SendMessage(m_hWndPageItems[CB_LANGUAGE], CB_SETCURSEL, ui32i, 0);

					free(buf);
					break;
				}

				free(buf);
			}
		}
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP] == true)
	{
		::EnableWindow(m_hWndPageItems[EDT_IPV4_ADDRESS], FALSE);
		::EnableWindow(m_hWndPageItems[EDT_IPV6_ADDRESS], FALSE);
	}

	GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_HUBLIST_AUTO_REG], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageGeneral::GetPageName()
{
	return LanguageManager::m_Ptr->m_sTexts[LAN_GENERAL_SETTINGS];
}
//------------------------------------------------------------------------------

void SettingPageGeneral::FocusLastItem()
{
	::SetFocus(m_hWndPageItems[BTN_HUBLIST_AUTO_REG]);
}
//------------------------------------------------------------------------------
