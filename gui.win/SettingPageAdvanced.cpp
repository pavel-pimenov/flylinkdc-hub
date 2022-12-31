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
#include "SettingPageAdvanced.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
#include "SettingDialog.h"
//---------------------------------------------------------------------------

LRESULT CALLBACK EnableDbCheckProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if(uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			if(::SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				::SetFocus(::GetNextDlgTabItem(SettingDialog::m_Ptr->m_hWndWindowItems[SettingDialog::WINDOW_HANDLE], hWnd, FALSE));
			}
			else
			{
				::SetFocus(SettingDialog::m_Ptr->m_hWndWindowItems[SettingDialog::TV_TREE]);
			}
			
			return 0;
		}
		else
		{
			::SetFocus(::GetNextDlgTabItem(SettingDialog::m_Ptr->m_hWndWindowItems[SettingDialog::WINDOW_HANDLE], hWnd, TRUE));
			return 0;
		}
	}
	
	return ::CallWindowProc(GuiSettingManager::m_wpOldButtonProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

SettingPageAdvanced::SettingPageAdvanced() : m_bUpdateSysTray(false), m_bUpdateScripting(false)
{
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageAdvanced::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_COMMAND)
	{
		switch(LOWORD(wParam))
		{
		case BTN_ENABLE_TRAY_ICON:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndPageItems[BTN_MINIMIZE_ON_STARTUP],
				               ::SendMessage(m_hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case BTN_ENABLE_SCRIPTING:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnable = ::SendMessage(m_hWndPageItems[BTN_ENABLE_SCRIPTING], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
				::EnableWindow(m_hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], bEnable);
				::EnableWindow(m_hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], bEnable);
			}
			
			break;
		case BTN_FILTER_KICK_MESSAGES:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS],
				               ::SendMessage(m_hWndPageItems[BTN_FILTER_KICK_MESSAGES], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case BTN_SEND_STATUS_MESSAGES_TO_OPS:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM],
				               ::SendMessage(m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}
			
			break;
		case EDT_PREFIXES_FOR_HUB_COMMANDS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				char buf[6];
				::GetWindowText((HWND)lParam, buf, 6);
				
				bool bChanged = false;
				
				for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++)
				{
					if(buf[ui16i] == '|' || buf[ui16i] == ' ')
					{
						strcpy(buf+ui16i, buf+ui16i+1);
						bChanged = true;
						ui16i--;
						continue;
					}
					
					for(uint16_t ui16j = 0; buf[ui16j] != '\0'; ui16j++)
					{
						if(ui16j == ui16i)
						{
							continue;
						}
						
						if(buf[ui16j] == buf[ui16i])
						{
							strcpy(buf+ui16j, buf+ui16j+1);
							bChanged = true;
							if(ui16i > ui16j)
							{
								ui16i--;
							}
							ui16j--;
						}
					}
				}
				
				if(bChanged == true)
				{
					int iStart = 0, iEnd = 0;
					
					::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
					
					::SetWindowText((HWND)lParam, buf);
					
					::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
				}
				
				return 0;
			}
			
			break;
		case EDT_ADMIN_NICK:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				RemovePipes((HWND)lParam);
				
				return 0;
			}
			
			break;
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		case CHK_ENABLE_DATABASE:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bEnabled = ::SendMessage(m_hWndPageItems[CHK_ENABLE_DATABASE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
				
				::EnableWindow(m_hWndPageItems[LBL_REMOVE_OLD_RECORDS], bEnabled);
				::EnableWindow(m_hWndPageItems[EDT_REMOVE_OLD_RECORDS], bEnabled);
				::EnableWindow(m_hWndPageItems[UD_REMOVE_OLD_RECORDS], bEnabled);
			}
			
			break;
		case EDT_REMOVE_OLD_RECORDS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 32767);
				
				return 0;
			}
			
			break;
#endif
		}
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageAdvanced::Save()
{
	if(m_bCreated == false)
	{
		return;
	}
	
	SettingManager::m_Ptr->SetBool(SETBOOL_AUTO_START, ::SendMessage(m_hWndPageItems[BTN_AUTO_START], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_CHECK_NEW_RELEASES, ::SendMessage(m_hWndPageItems[BTN_CHECK_FOR_UPDATE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	if((::SendMessage(m_hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TRAY_ICON])
	{
		m_bUpdateSysTray = true;
	}
	
	SettingManager::m_Ptr->SetBool(SETBOOL_ENABLE_TRAY_ICON, ::SendMessage(m_hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_START_MINIMIZED, ::SendMessage(m_hWndPageItems[BTN_MINIMIZE_ON_STARTUP], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	char buf[1025];
	int iLen = ::GetWindowText(m_hWndPageItems[EDT_PREFIXES_FOR_HUB_COMMANDS], buf, 6);
	SettingManager::m_Ptr->SetText(SETTXT_CHAT_COMMANDS_PREFIXES, buf, iLen);
	
	SettingManager::m_Ptr->SetBool(SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM, ::SendMessage(m_hWndPageItems[BTN_REPLY_TO_HUB_COMMANDS_IN_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	if((::SendMessage(m_hWndPageItems[BTN_ENABLE_SCRIPTING], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING])
	{
		m_bUpdateScripting = true;
	}
	
	SettingManager::m_Ptr->SetBool(SETBOOL_ENABLE_SCRIPTING, ::SendMessage(m_hWndPageItems[BTN_ENABLE_SCRIPTING], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_STOP_SCRIPT_ON_ERROR, ::SendMessage(m_hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_LOG_SCRIPT_ERRORS, ::SendMessage(m_hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	SettingManager::m_Ptr->SetBool(SETBOOL_FILTER_KICK_MESSAGES, ::SendMessage(m_hWndPageItems[BTN_FILTER_KICK_MESSAGES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_SEND_KICK_MESSAGES_TO_OPS, ::SendMessage(m_hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	SettingManager::m_Ptr->SetBool(SETBOOL_SEND_STATUS_MESSAGES, ::SendMessage(m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	SettingManager::m_Ptr->SetBool(SETBOOL_SEND_STATUS_MESSAGES_AS_PM, ::SendMessage(m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	iLen = ::GetWindowText(m_hWndPageItems[EDT_ADMIN_NICK], buf, 1025);
	SettingManager::m_Ptr->SetText(SETTXT_ADMIN_NICK, buf, iLen);
	
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
	bool bOldDbState = SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE];
	
	SettingManager::m_Ptr->SetBool(SETBOOL_ENABLE_DATABASE, ::SendMessage(m_hWndPageItems[CHK_ENABLE_DATABASE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
	
	if(bOldDbState != SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE])
	{
		SettingManager::m_Ptr->UpdateDatabase();
	}
	
	LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_REMOVE_OLD_RECORDS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		SettingManager::m_Ptr->SetShort(SETSHORT_DB_REMOVE_OLD_RECORDS, LOWORD(lResult));
	}
#endif
}
//------------------------------------------------------------------------------

void SettingPageAdvanced::GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
                                     bool & /*bUpdatedAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                     bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
                                     bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                     bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                     bool & /*bUpdatedPermBanRedirAddress*/, bool &bUpdatedSysTray, bool &bUpdatedScripting, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if(bUpdatedSysTray == false)
	{
		bUpdatedSysTray = m_bUpdateSysTray;
	}
	if(bUpdatedScripting == false)
	{
		bUpdatedScripting = m_bUpdateScripting;
	}
}

//------------------------------------------------------------------------------
bool SettingPageAdvanced::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	if(m_bCreated == false)
	{
		return false;
	}
	
	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);
	
	int iPosY = GuiSettingManager::m_iGroupBoxMargin + (4 * GuiSettingManager::m_iCheckHeight) + 17;
	
	m_hWndPageItems[GB_HUB_STARTUP_AND_TRAY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_STARTUP_AND_TRAY_ICON],
	                                                            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, m_iFullGB, iPosY, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                            
	m_hWndPageItems[BTN_AUTO_START] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_AUTO_START], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                   8, GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_AUTO_START], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_START] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[BTN_CHECK_FOR_UPDATE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_UPDATE_CHECK_ON_STARTUP], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                         8, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_CHECK_FOR_UPDATE], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_CHECK_NEW_RELEASES] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[BTN_ENABLE_TRAY_ICON] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_TRAY_ICON], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                         8, GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 6, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_ENABLE_TRAY_ICON, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TRAY_ICON] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[BTN_MINIMIZE_ON_STARTUP] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MINIMIZE_ON_STARTUP], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                            8, GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iCheckHeight) + 9, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_MINIMIZE_ON_STARTUP], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_START_MINIMIZED] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[GB_HUB_COMMANDS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_COMMANDS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                    0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineOneChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                    
	m_hWndPageItems[LBL_PREFIXES_FOR_HUB_COMMANDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_PREFIXES_FOR_HUB_CMDS],
	                                                                  WS_CHILD | WS_VISIBLE | SS_LEFT, 8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(350), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                                  
	m_hWndPageItems[EDT_PREFIXES_FOR_HUB_COMMANDS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES], WS_CHILD |
	                                                                  WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, ScaleGui(350) + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(350) + 26), GuiSettingManager::m_iEditHeight,
	                                                                  m_hWnd, (HMENU)EDT_PREFIXES_FOR_HUB_COMMANDS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_PREFIXES_FOR_HUB_COMMANDS], EM_SETLIMITTEXT, 5, 0);
	
	m_hWndPageItems[BTN_REPLY_TO_HUB_COMMANDS_IN_PM] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REPLY_HUB_CMDS_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                                    8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 4, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_REPLY_TO_HUB_COMMANDS_IN_PM], BM_SETCHECK,
	              (SettingManager::m_Ptr->m_bBools[SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	              
	iPosY += GuiSettingManager::m_iOneLineOneChecksGB;
	
	m_hWndPageItems[GB_SCRIPTING] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTING], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                 0, iPosY, m_iFullGB, GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iCheckHeight) + 14, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                 
	m_hWndPageItems[BTN_ENABLE_SCRIPTING] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_SCRIPTING], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                         8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_ENABLE_SCRIPTING, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_ENABLE_SCRIPTING], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_STOP_SCRIPT_ON_ERR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                             8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_STOP_SCRIPT_ON_ERROR] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_LOG_SCRIPT_ERRORS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                                  8, iPosY + GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 6, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	iPosY += GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iCheckHeight) + 14;
	
	m_hWndPageItems[GB_KICK_MESSAGES] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_KICK_MESSAGES], WS_CHILD | WS_VISIBLE |
	                                                     BS_GROUPBOX, 0, iPosY, m_iFullGB, m_iTwoChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                     
	m_hWndPageItems[BTN_FILTER_KICK_MESSAGES] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_FILTER_KICKS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                             8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_FILTER_KICK_MESSAGES, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_FILTER_KICK_MESSAGES], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_FILTER_KICK_MESSAGES] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_FILTERED_KICKS_TO_OPS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                                  8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_KICK_MESSAGES_TO_OPS] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	iPosY += m_iTwoChecksGB;
	
	m_hWndPageItems[GB_STATUS_MESSAGES] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_STATUS_MESSAGES], WS_CHILD | WS_VISIBLE |
	                                                       BS_GROUPBOX, 0, iPosY, m_iFullGB, m_iTwoChecksGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                       
	m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SEND_STATUS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                                    8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)BTN_SEND_STATUS_MESSAGES_TO_OPS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SEND_STATUS_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                                   8, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 3, m_iFullEDT, GuiSettingManager::m_iCheckHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);
	
	iPosY += m_iTwoChecksGB;
	
	m_hWndPageItems[GB_ADMIN_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ADMIN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                  0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                  
	m_hWndPageItems[EDT_ADMIN_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                                   8, iPosY + GuiSettingManager::m_iGroupBoxMargin, m_iFullEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_ADMIN_NICK, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_ADMIN_NICK], EM_SETLIMITTEXT, 64, 0);
	
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
	iPosY += GuiSettingManager::m_iOneLineGB;
	
	m_hWndPageItems[GB_DATABASE_SUPPORT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DATABASE_SUPPORT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                                        0, iPosY, m_iFullGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                        
	m_hWndPageItems[CHK_ENABLE_DATABASE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_DATABASE], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                                        8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), int(m_iFullEDT * 0.4) - 2, GuiSettingManager::m_iCheckHeight, m_hWnd, (HMENU)CHK_ENABLE_DATABASE, ServerManager::m_hInstance, nullptr);
	                                                        
	m_hWndPageItems[LBL_REMOVE_OLD_RECORDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE_OLD_RECORDS], WS_CHILD | WS_VISIBLE | SS_RIGHT, int(m_iFullEDT * 0.4)+10, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                                           ((m_iFullEDT - (int((m_iFullEDT/2)*0.2) + GuiSettingManager::m_iUpDownWidth)) + 8) - (int(m_iFullEDT * 0.4)+15), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                           
	m_hWndPageItems[EDT_REMOVE_OLD_RECORDS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                                           (m_iFullEDT - (int((m_iFullEDT/2)*0.2) + GuiSettingManager::m_iUpDownWidth)) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, int((m_iFullEDT/2) * 0.2), GuiSettingManager::m_iEditHeight,
	                                                           m_hWnd, (HMENU)EDT_REMOVE_OLD_RECORDS, ServerManager::m_hInstance, nullptr);
	                                                           
	AddUpDown(m_hWndPageItems[UD_REMOVE_OLD_RECORDS], (m_iFullEDT-GuiSettingManager::m_iUpDownWidth) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(32767, 0),
	          (WPARAM)m_hWndPageItems[EDT_REMOVE_OLD_RECORDS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS], 0));
#endif
	          
	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++)
	{
		if(m_hWndPageItems[ui8i] == nullptr)
		{
			return false;
		}
		
		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}
	
	::EnableWindow(m_hWndPageItems[BTN_MINIMIZE_ON_STARTUP], SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TRAY_ICON] == true ? TRUE : FALSE);
	
	::EnableWindow(m_hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == true ? TRUE : FALSE);
	::EnableWindow(m_hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == true ? TRUE : FALSE);
	
	::EnableWindow(m_hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS], SettingManager::m_Ptr->m_bBools[SETBOOL_FILTER_KICK_MESSAGES] == true ? TRUE : FALSE);
	
	::EnableWindow(m_hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM], SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true ? TRUE : FALSE);
	
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE] == true)
	{
		::SendMessage(m_hWndPageItems[CHK_ENABLE_DATABASE], BM_SETCHECK, BST_CHECKED, 0);
		
		::EnableWindow(m_hWndPageItems[LBL_REMOVE_OLD_RECORDS], TRUE);
		::EnableWindow(m_hWndPageItems[EDT_REMOVE_OLD_RECORDS], TRUE);
		::EnableWindow(m_hWndPageItems[UD_REMOVE_OLD_RECORDS], TRUE);
	}
	else
	{
		::SendMessage(m_hWndPageItems[CHK_ENABLE_DATABASE], BM_SETCHECK, BST_UNCHECKED, 0);
		
		::EnableWindow(m_hWndPageItems[LBL_REMOVE_OLD_RECORDS], FALSE);
		::EnableWindow(m_hWndPageItems[EDT_REMOVE_OLD_RECORDS], FALSE);
		::EnableWindow(m_hWndPageItems[UD_REMOVE_OLD_RECORDS], FALSE);
	}
	
	::SendMessage(m_hWndPageItems[EDT_REMOVE_OLD_RECORDS], EM_SETLIMITTEXT, 5, 0);
	AddToolTip(m_hWndPageItems[EDT_REMOVE_OLD_RECORDS], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_DISABLED]);
#endif
	
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
	GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[CHK_ENABLE_DATABASE], GWLP_WNDPROC, (LONG_PTR)EnableDbCheckProc);
	GuiSettingManager::m_wpOldEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_REMOVE_OLD_RECORDS], GWLP_WNDPROC, (LONG_PTR)EditProc);
#else
	GuiSettingManager::m_wpOldEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_ADMIN_NICK], GWLP_WNDPROC, (LONG_PTR)EditProc);
#endif
	return true;
}
//------------------------------------------------------------------------------

char * SettingPageAdvanced::GetPageName()
{
	return LanguageManager::m_Ptr->m_sTexts[LAN_ADVANCED];
}
//------------------------------------------------------------------------------

void SettingPageAdvanced::FocusLastItem()
{
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
	if(::SendMessage(m_hWndPageItems[CHK_ENABLE_DATABASE], BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		::SetFocus(m_hWndPageItems[EDT_REMOVE_OLD_RECORDS]);
	}
	else
	{
		::SetFocus(m_hWndPageItems[CHK_ENABLE_DATABASE]);
	}
#else
	::SetFocus(m_hWndPageItems[EDT_ADMIN_NICK]);
#endif
}
//------------------------------------------------------------------------------
