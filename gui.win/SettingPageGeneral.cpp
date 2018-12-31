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

SettingPageGeneral::SettingPageGeneral() : bUpdateHubNameWelcome(false), bUpdateHubName(false), bUpdateTCPPorts(false), bUpdateUDPPort(false), bUpdateAutoReg(false), bUpdateLanguage(false)
{
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageGeneral::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case EDT_HUB_NAME:
		case EDT_HUB_TOPIC:
		case EDT_HUB_DESCRIPTION:
		case EDT_HUB_ADDRESS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				RemoveDollarsPipes((HWND)lParam);
				
				return 0;
			}
			
			break;
		case EDT_TCP_PORTS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				char buf[65];
				::GetWindowText((HWND)lParam, buf, 65);
				
				bool bChanged = false;
				
				for (uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++)
				{
					if (isdigit(buf[ui16i]) == 0 && buf[ui16i] != ';')
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
		case EDT_UDP_PORT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 65535);
				
				return 0;
			}
			
			break;
		case EDT_MAX_USERS:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 32767);
				
				return 0;
			}
			
			break;
		case BTN_RESOLVE_ADDRESS:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnable = ::SendMessage(hWndPageItems[BTN_RESOLVE_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
				::SetWindowText(hWndPageItems[EDT_IPV4_ADDRESS], bEnable == FALSE ? clsServerManager::sHubIP :
				                (clsSettingManager::mPtr->sTexts[SETTXT_IPV4_ADDRESS] != NULL ? clsSettingManager::mPtr->sTexts[SETTXT_IPV4_ADDRESS] : ""));
				::EnableWindow(hWndPageItems[EDT_IPV4_ADDRESS], bEnable);
				::SetWindowText(hWndPageItems[EDT_IPV6_ADDRESS], bEnable == FALSE ? clsServerManager::sHubIP6 :
				                (clsSettingManager::mPtr->sTexts[SETTXT_IPV6_ADDRESS] != NULL ? clsSettingManager::mPtr->sTexts[SETTXT_IPV6_ADDRESS] : ""));
				::EnableWindow(hWndPageItems[EDT_IPV6_ADDRESS], bEnable);
			}
			
			break;
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageGeneral::Save()
{
	if (bCreated == false)
	{
		return;
	}
	
	char sBuf[1025];
	int iLen = ::GetWindowText(hWndPageItems[EDT_HUB_NAME], sBuf, 1025);
	
	if (strcmp(sBuf, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME]) != NULL)
	{
		bUpdateHubNameWelcome = true;
		bUpdateHubName = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_HUB_NAME, sBuf, iLen);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUB_TOPIC], sBuf, 1025);
	
	if (bUpdateHubName == false && ((clsSettingManager::mPtr->sTexts[SETTXT_HUB_TOPIC] == NULL && iLen != 0) ||
	                                (clsSettingManager::mPtr->sTexts[SETTXT_HUB_TOPIC] != NULL && strcmp(sBuf, clsSettingManager::mPtr->sTexts[SETTXT_HUB_TOPIC]) != NULL)))
	{
		bUpdateHubNameWelcome = true;
		bUpdateHubName = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_HUB_TOPIC, sBuf, iLen);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUB_DESCRIPTION], sBuf, 1025);
	clsSettingManager::mPtr->SetText(SETTXT_HUB_DESCRIPTION, sBuf, iLen);
	
	clsSettingManager::mPtr->SetBool(SETBOOL_ANTI_MOGLO, ::SendMessage(hWndPageItems[BTN_ANTI_MOGLO], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	bool bResolveHubAddress = false;
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUB_ADDRESS], sBuf, 1025);
	
	if (strcmp(sBuf, clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS]) != NULL)
	{
		bResolveHubAddress = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_HUB_ADDRESS, sBuf, iLen);
	
	bool bOldResolve = clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP];
	
	clsSettingManager::mPtr->SetBool(SETBOOL_RESOLVE_TO_IP, ::SendMessage(hWndPageItems[BTN_RESOLVE_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	if (bOldResolve != clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP])
	{
		bResolveHubAddress = true;
	}
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP] == false)
	{
		iLen = ::GetWindowText(hWndPageItems[EDT_IPV4_ADDRESS], sBuf, 1025);
		clsSettingManager::mPtr->SetText(SETTXT_IPV4_ADDRESS, sBuf, iLen);
		
		iLen = ::GetWindowText(hWndPageItems[EDT_IPV6_ADDRESS], sBuf, 1025);
		clsSettingManager::mPtr->SetText(SETTXT_IPV6_ADDRESS, sBuf, iLen);
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_BIND_ONLY_SINGLE_IP, ::SendMessage(hWndPageItems[BTN_BIND_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_TCP_PORTS], sBuf, 1025);
	
	if (strcmp(sBuf, clsSettingManager::mPtr->sTexts[SETTXT_TCP_PORTS]) != NULL)
	{
		bUpdateTCPPorts = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_TCP_PORTS, sBuf, iLen);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_UDP_PORT], sBuf, 1025);
	
	if (strcmp(sBuf, clsSettingManager::mPtr->sTexts[SETTXT_UDP_PORT]) != NULL)
	{
		bUpdateUDPPort = true;
	}
	
	clsSettingManager::mPtr->SetText(SETTXT_UDP_PORT, sBuf, iLen);
	
	iLen = ::GetWindowText(hWndPageItems[EDT_HUB_LISTS], sBuf, 1025);
	clsSettingManager::mPtr->SetText(SETTXT_REGISTER_SERVERS, sBuf, iLen);
	
	if ((::SendMessage(hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != clsSettingManager::mPtr->bBools[SETBOOL_AUTO_REG])
	{
		bUpdateAutoReg = true;
	}
	
	clsSettingManager::mPtr->SetBool(SETBOOL_AUTO_REG, ::SendMessage(hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	if (bResolveHubAddress == true)
	{
		clsServerManager::ResolveHubAddress(true);
	}
	
	uint32_t ui32CurSel = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETCURSEL, 0, 0);
	
	if (ui32CurSel == 0)
	{
		if (clsSettingManager::mPtr->sTexts[SETTXT_LANGUAGE] != NULL)
		{
			bUpdateLanguage = true;
			bUpdateHubNameWelcome = true;
		}
		
		clsSettingManager::mPtr->SetText(SETTXT_LANGUAGE, "", 0);
	}
	else
	{
		uint32_t ui32Len = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXTLEN, ui32CurSel, 0);
		
		char * sTempBuf = (char *)malloc(ui32Len + 1);
		
		if (sTempBuf == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot allocate %u bytes for sTempBuf in SettingPageGeneral::Save\n", ui32Len + 1);
			return;
		}
		
		if (clsSettingManager::mPtr->sTexts[SETTXT_LANGUAGE] == NULL || strcmp(sTempBuf, clsSettingManager::mPtr->sTexts[SETTXT_LANGUAGE]) != NULL)
		{
			bUpdateLanguage = true;
			bUpdateHubNameWelcome = true;
		}
		
		::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXT, ui32CurSel, (LPARAM)sTempBuf);
		
		clsSettingManager::mPtr->SetText(SETTXT_LANGUAGE, sTempBuf, ui32Len);
		
		free(sTempBuf);
	}
	
	LRESULT lResult = ::SendMessage(hWndPageItems[UD_MAX_USERS], UDM_GETPOS, 0, 0);
	if (HIWORD(lResult) == 0)
	{
		clsSettingManager::mPtr->SetShort(SETSHORT_MAX_USERS, LOWORD(lResult));
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
	if (bUpdatedHubNameWelcome == false)
	{
		bUpdatedHubNameWelcome = bUpdateHubNameWelcome;
	}
	if (bUpdatedHubName == false)
	{
		bUpdatedHubName = bUpdateHubName;
	}
	if (bUpdatedTCPPorts == false)
	{
		bUpdatedTCPPorts = bUpdateTCPPorts;
	}
	if (bUpdatedUDPPort == false)
	{
		bUpdatedUDPPort = bUpdateUDPPort;
	}
	if (bUpdatedAutoReg == false)
	{
		bUpdatedAutoReg = bUpdateAutoReg;
	}
	if (bUpdatedLanguage == false)
	{
		bUpdatedLanguage = bUpdateLanguage;
	}
}

//------------------------------------------------------------------------------
bool SettingPageGeneral::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if (bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	iFullGB = (rcThis.right - rcThis.left) - 5;
	iFullEDT = (rcThis.right - rcThis.left) - 21;
	iGBinGB = (rcThis.right - rcThis.left) - 15;
	iGBinGBEDT = (rcThis.right - rcThis.left) - 31;
	
	int iPosY = clsGuiSettingManager::iOneLineGB;
	
	int iHeight = iTwoChecksGB + clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 4 + clsGuiSettingManager::iOneLineGB + 5 + clsGuiSettingManager::iOneLineTwoChecksGB + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + (2 * clsGuiSettingManager::iOneLineGB) + 6 +
	              clsGuiSettingManager::iGroupBoxMargin + (3 * clsGuiSettingManager::iCheckHeight) + 14 + 3;
	              
	iHeight = iHeight > ((3 * iOneLineTwoGroupGB) + clsGuiSettingManager::iOneLineGB + 3) ? iHeight : ((3 * iOneLineTwoGroupGB) + clsGuiSettingManager::iOneLineGB + 3);
	
	::SetWindowPos(m_hWnd, NULL, 0, 0, rcThis.right, iHeight, SWP_NOMOVE | SWP_NOZORDER);
	
	hWndPageItems[GB_LANGUAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_LANGUAGE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, ScaleGui(297), clsGuiSettingManager::iOneLineGB,
	                                              m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[CB_LANGUAGE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST, 8, clsGuiSettingManager::iGroupBoxMargin, ScaleGui(297) - 16, clsGuiSettingManager::iEditHeight,
	                                              m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[GB_MAX_USERS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MAX_USERS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               ScaleGui(297) + 5, 0, (rcThis.right - rcThis.left) - (ScaleGui(297) + 10), clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_MAX_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                ScaleGui(297) + 13, clsGuiSettingManager::iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(297) + clsGuiSettingManager::iUpDownWidth + 26), clsGuiSettingManager::iEditHeight,
	                                                m_hWnd, (HMENU)EDT_MAX_USERS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_MAX_USERS], EM_SETLIMITTEXT, 5, 0);
	
	AddUpDown(hWndPageItems[UD_MAX_USERS], (rcThis.right - rcThis.left) - clsGuiSettingManager::iUpDownWidth - 13, clsGuiSettingManager::iGroupBoxMargin, clsGuiSettingManager::iUpDownWidth, clsGuiSettingManager::iEditHeight, (LPARAM)MAKELONG(32767, 1),
	          (WPARAM)hWndPageItems[EDT_MAX_USERS], (LPARAM)MAKELONG(clsSettingManager::mPtr->i16Shorts[SETSHORT_MAX_USERS], 0));
	          
	hWndPageItems[GB_HUB_NAME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_NAME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                              0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[EDT_HUB_NAME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                               8, clsGuiSettingManager::iOneLineGB + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUB_NAME, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_NAME], EM_SETLIMITTEXT, 256, 0);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_HUB_TOPIC] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_TOPIC], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               0, iPosY, iFullGB, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_HUB_TOPIC] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_HUB_TOPIC], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUB_TOPIC, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_TOPIC], EM_SETLIMITTEXT, 256, 0);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                     0, iPosY, iFullGB, clsGuiSettingManager::iOneLineOneChecksGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                     
	hWndPageItems[EDT_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_HUB_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                      8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUB_DESCRIPTION, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_DESCRIPTION], EM_SETLIMITTEXT, 256, 0);
	
	hWndPageItems[BTN_ANTI_MOGLO] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ANTI_MOGLO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                 8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 4, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_ANTI_MOGLO], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_ANTI_MOGLO] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	iPosY += clsGuiSettingManager::iOneLineOneChecksGB;
	
	hWndPageItems[GB_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 0, iPosY, iFullGB, clsGuiSettingManager::iOneLineTwoChecksGB + clsGuiSettingManager::iOneLineGB + 3, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[EDT_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_HUB_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                  8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_HUB_ADDRESS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_ADDRESS], EM_SETLIMITTEXT, 256, 0);
	
	hWndPageItems[BTN_RESOLVE_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_RESOLVE_HOSTNAME], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                      8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 4, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, (HMENU)BTN_RESOLVE_ADDRESS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_RESOLVE_ADDRESS], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	int iFourtyPercent = (int)(iGBinGB * 0.4);
	hWndPageItems[GB_IPV4_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_IPV4_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  5, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iCheckHeight + 5, iFourtyPercent - 3, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_IPV4_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP] == true ? clsServerManager::sHubIP :
	                                                   (clsSettingManager::mPtr->sTexts[SETTXT_IPV4_ADDRESS] != NULL ? clsSettingManager::mPtr->sTexts[SETTXT_IPV4_ADDRESS] : ""),
	                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 13, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iCheckHeight + 5 + clsGuiSettingManager::iGroupBoxMargin, iFourtyPercent - 19, clsGuiSettingManager::iEditHeight,
	                                                   m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_IPV4_ADDRESS], EM_SETLIMITTEXT, 15, 0);
	
	hWndPageItems[GB_IPV6_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_IPV6_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  10 + (iFourtyPercent - 3), iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iCheckHeight + 5, (iGBinGB - (iFourtyPercent - 3)) - 5, clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[EDT_IPV6_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP] == true ? clsServerManager::sHubIP6 :
	                                                   (clsSettingManager::mPtr->sTexts[SETTXT_IPV6_ADDRESS] != NULL ? clsSettingManager::mPtr->sTexts[SETTXT_IPV6_ADDRESS] : ""),
	                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 18 + (iFourtyPercent - 3), iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iCheckHeight + 5 + clsGuiSettingManager::iGroupBoxMargin,
	                                                   (iGBinGB - (iFourtyPercent - 3)) - 21, clsGuiSettingManager::iEditHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_IPV6_ADDRESS], EM_SETLIMITTEXT, 39, 0);
	
	hWndPageItems[BTN_BIND_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_BIND_ONLY_ADDRS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                   8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iOneLineGB + 10, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_BIND_ADDRESS], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	iPosY += clsGuiSettingManager::iOneLineTwoChecksGB + clsGuiSettingManager::iOneLineGB + 3;
	
	hWndPageItems[GB_TCP_PORTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_TCP_PORTS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               0, iPosY, ScaleGui(362), clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_TCP_PORTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_TCP_PORTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                8, iPosY + clsGuiSettingManager::iGroupBoxMargin, ScaleGui(362) - 16, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_TCP_PORTS, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_TCP_PORTS], EM_SETLIMITTEXT, 64, 0);
	AddToolTip(hWndPageItems[EDT_TCP_PORTS], clsLanguageManager::mPtr->sTexts[LAN_TCP_PORTS_HINT]);
	
	hWndPageItems[GB_UDP_PORT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_UDP_PORT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                              ScaleGui(362) + 5, iPosY, (rcThis.right - rcThis.left) - (ScaleGui(362) + 10), clsGuiSettingManager::iOneLineGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                              
	hWndPageItems[EDT_UDP_PORT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_UDP_PORT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
	                                               ScaleGui(362) + 13, iPosY + clsGuiSettingManager::iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(362) + 26), clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)EDT_UDP_PORT, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_UDP_PORT], EM_SETLIMITTEXT, 5, 0);
	AddToolTip(hWndPageItems[EDT_UDP_PORT], clsLanguageManager::mPtr->sTexts[LAN_ZERO_DISABLED]);
	
	iPosY += clsGuiSettingManager::iOneLineGB;
	
	hWndPageItems[GB_HUB_LISTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_HUB_REG_ADRS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                               0, iPosY, iFullGB, clsGuiSettingManager::iOneLineOneChecksGB, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[EDT_HUB_LISTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, clsSettingManager::mPtr->sTexts[SETTXT_REGISTER_SERVERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                8, iPosY + clsGuiSettingManager::iGroupBoxMargin, iFullEDT, clsGuiSettingManager::iEditHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[EDT_HUB_LISTS], EM_SETLIMITTEXT, 1024, 0);
	AddToolTip(hWndPageItems[EDT_HUB_LISTS], clsLanguageManager::mPtr->sTexts[LAN_HUB_LIST_REGS_HINT]);
	
	hWndPageItems[BTN_HUBLIST_AUTO_REG] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_AUTO_REG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                       8, iPosY + clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 4, iFullEDT, clsGuiSettingManager::iCheckHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	::SendMessage(hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_SETCHECK, (clsSettingManager::mPtr->bBools[SETBOOL_AUTO_REG] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	// add default language and select it
	::SendMessage(hWndPageItems[CB_LANGUAGE], CB_ADDSTRING, 0, (LPARAM)"Default English");
	::SendMessage(hWndPageItems[CB_LANGUAGE], CB_SETCURSEL, 0, 0);
	
	// add all languages found in language dir
	struct _finddata_t langfile;
	intptr_t hFile = _findfirst((clsServerManager::sPath + "\\language\\" + "*.xml").c_str(), &langfile);
	
	if (hFile != -1)
	{
		do
		{
			if (((langfile.attrib & _A_SUBDIR) == _A_SUBDIR) == true ||
			        stricmp(langfile.name + (strlen(langfile.name) - 4), ".xml") != NULL)
			{
				continue;
			}
			
			::SendMessage(hWndPageItems[CB_LANGUAGE], CB_ADDSTRING, 0, (LPARAM)string(langfile.name, strlen(langfile.name) - 4).c_str());
		}
		while (_findnext(hFile, &langfile) == 0);
		
		_findclose(hFile);
	}
	
	// Select actual language in combobox
	if (clsSettingManager::mPtr->sTexts[SETTXT_LANGUAGE] != NULL)
	{
		uint32_t ui32Count = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETCOUNT, 0, 0);
		for (uint32_t ui32i = 1; ui32i < ui32Count; ui32i++)
		{
			uint32_t ui32Len = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXTLEN, ui32i, 0);
			if (ui32Len == (int32_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_LANGUAGE])
			{
				char * buf = (char *)malloc(ui32Len + 1);
				
				if (buf == NULL)
				{
					AppendDebugLogFormat("[MEM] Cannot allocate %u bytes for buf in SettingPageGeneral::CreateSettingPage\n", ui32Len + 1);
					return false;
				}
				
				::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXT, ui32i, (LPARAM)buf);
				if (stricmp(buf, clsSettingManager::mPtr->sTexts[SETTXT_LANGUAGE]) == NULL)
				{
					::SendMessage(hWndPageItems[CB_LANGUAGE], CB_SETCURSEL, ui32i, 0);
					
					free(buf);
					
					break;
				}
				
				free(buf);
			}
		}
	}
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_RESOLVE_TO_IP] == true)
	{
		::EnableWindow(hWndPageItems[EDT_IPV4_ADDRESS], FALSE);
		::EnableWindow(hWndPageItems[EDT_IPV6_ADDRESS], FALSE);
	}
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_HUBLIST_AUTO_REG], GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageGeneral::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_GENERAL_SETTINGS];
}
//------------------------------------------------------------------------------

void SettingPageGeneral::FocusLastItem()
{
	::SetFocus(hWndPageItems[BTN_HUBLIST_AUTO_REG]);
}
//------------------------------------------------------------------------------
