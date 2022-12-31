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
#include "MainWindowPageStats.h"
//---------------------------------------------------------------------------
#include "../core/colUsers.h"
#include "../core/GlobalDataQueue.h"
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/UdpDebug.h"
#include "../core/User.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "LineDialog.h"
#include "MainWindow.h"
//---------------------------------------------------------------------------

MainWindowPageStats::MainWindowPageStats()
{
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT MainWindowPageStats::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case BTN_START_STOP:
			if(ServerManager::m_bServerRunning == false)
			{
				if(ServerManager::Start() == false)
				{
					::SetWindowText(m_hWndPageItems[LBL_STATUS_VALUE],
					                (string(LanguageManager::m_Ptr->m_sTexts[LAN_READY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_READY])+".").c_str());
				}
				::SetFocus(m_hWndPageItems[BTN_START_STOP]);
			}
			else
			{
				ServerManager::Stop();
				::SetFocus(MainWindow::m_Ptr->m_hWndWindowItems[MainWindow::TC_TABS]);
			}
			
			return 0;
		case BTN_REDIRECT_ALL:
			OnRedirectAll();
			return 0;
		case BTN_MASS_MSG:
			OnMassMessage();
			return 0;
		}
		
		break;
	case WM_WINDOWPOSCHANGED:
	{
		int iX = ((WINDOWPOS*)lParam)->cx;
		
		::SetWindowPos(m_hWndPageItems[BTN_START_STOP], nullptr, 0, 0, iX - 8, ScaleGui(40), SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[GB_STATS], nullptr, 0, 0, iX-10, GuiSettingManager::m_iGroupBoxMargin + (8 * GuiSettingManager::m_iTextHeight) + 2, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_STATUS_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_JOINS_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_PARTS_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_ACTIVE_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_ONLINE_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_PEAK_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_RECEIVED_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[LBL_SENT_VALUE], nullptr, 0, 0, iX - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[BTN_REDIRECT_ALL], nullptr, 0, 0, iX - 8, GuiSettingManager::m_iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(m_hWndPageItems[BTN_MASS_MSG], nullptr, 0, 0, iX - 8, GuiSettingManager::m_iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_SETFOCUS:
		::SetFocus(m_hWndPageItems[BTN_START_STOP]);
		return 0;
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT CALLBACK SSButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if(uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			if(ServerManager::m_bServerRunning == true)
			{
				::SetFocus(::GetNextDlgTabItem(MainWindow::m_Ptr->m_hWnd, hWnd, FALSE));
				return 0;
			}
		}
		
		::SetFocus(MainWindow::m_Ptr->m_hWndWindowItems[MainWindow::TC_TABS]);
		return 0;
	}
	
	return ::CallWindowProc(GuiSettingManager::m_wpOldButtonProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

bool MainWindowPageStats::CreateMainWindowPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	RECT rcMain;
	::GetClientRect(m_hWnd, &rcMain);
	
	::SetWindowPos(m_hWnd, nullptr, 0, 0, rcMain.right, ScaleGui(40) + GuiSettingManager::m_iGroupBoxMargin + (8 * GuiSettingManager::m_iTextHeight) + (2 * GuiSettingManager::m_iEditHeight) + 13, SWP_NOMOVE | SWP_NOZORDER);
	
	m_hWndPageItems[BTN_START_STOP] = ::CreateWindowEx(0, WC_BUTTON, ServerManager::m_bServerRunning == false ? LanguageManager::m_Ptr->m_sTexts[LAN_START_HUB] : LanguageManager::m_Ptr->m_sTexts[LAN_STOP_HUB],
	                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 4, 3, rcMain.right-8, ScaleGui(40), m_hWnd, (HMENU)BTN_START_STOP, ServerManager::m_hInstance, nullptr);
	                                                   
	int iPosY = ScaleGui(40);
	
	m_hWndPageItems[GB_STATS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                             5, iPosY, rcMain.right-10, GuiSettingManager::m_iGroupBoxMargin + (8 * GuiSettingManager::m_iTextHeight) + 2, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                             
	m_hWndPageItems[LBL_STATUS] = ::CreateWindowEx(0, WC_STATIC, (string(LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_STATUS])+":").c_str(),
	                                               WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5), ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                               
	m_hWndPageItems[LBL_STATUS_VALUE] = ::CreateWindowEx(0, WC_STATIC, (string(LanguageManager::m_Ptr->m_sTexts[LAN_READY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_READY])+".").c_str(),
	                                                     WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5), rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                     
	m_hWndPageItems[LBL_JOINS] = ::CreateWindowEx(0, WC_STATIC,
	                                              (string(LanguageManager::m_Ptr->m_sTexts[LAN_ACCEPTED_CONNECTIONS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ACCEPTED_CONNECTIONS])+":").c_str(),
	                                              WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + GuiSettingManager::m_iTextHeight, ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                              
	m_hWndPageItems[LBL_JOINS_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                    ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + GuiSettingManager::m_iTextHeight, rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                    
	m_hWndPageItems[LBL_PARTS] = ::CreateWindowEx(0, WC_STATIC,
	                                              (string(LanguageManager::m_Ptr->m_sTexts[LAN_CLOSED_CONNECTIONS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CLOSED_CONNECTIONS])+":").c_str(),
	                                              WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (2 * GuiSettingManager::m_iTextHeight), ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                              
	m_hWndPageItems[LBL_PARTS_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                    ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (2 * GuiSettingManager::m_iTextHeight), rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                    
	m_hWndPageItems[LBL_ACTIVE] = ::CreateWindowEx(0, WC_STATIC,
	                                               (string(LanguageManager::m_Ptr->m_sTexts[LAN_ACTIVE_CONNECTIONS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ACTIVE_CONNECTIONS])+":").c_str(),
	                                               WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (3 * GuiSettingManager::m_iTextHeight), ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                               
	m_hWndPageItems[LBL_ACTIVE_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                     ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (3 * GuiSettingManager::m_iTextHeight), rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                     
	m_hWndPageItems[LBL_ONLINE] = ::CreateWindowEx(0, WC_STATIC,
	                                               (string(LanguageManager::m_Ptr->m_sTexts[LAN_USERS_ONLINE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_USERS_ONLINE])+":").c_str(),
	                                               WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (4 * GuiSettingManager::m_iTextHeight), ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                               
	m_hWndPageItems[LBL_ONLINE_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                     ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (4 * GuiSettingManager::m_iTextHeight), rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                     
	m_hWndPageItems[LBL_PEAK] = ::CreateWindowEx(0, WC_STATIC,
	                                             (string(LanguageManager::m_Ptr->m_sTexts[LAN_USERS_PEAK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_USERS_PEAK])+":").c_str(),
	                                             WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (5 * GuiSettingManager::m_iTextHeight), ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                             
	m_hWndPageItems[LBL_PEAK_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                   ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (5 * GuiSettingManager::m_iTextHeight), rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                   
	m_hWndPageItems[LBL_RECEIVED] = ::CreateWindowEx(0, WC_STATIC,
	                                                 (string(LanguageManager::m_Ptr->m_sTexts[LAN_RECEIVED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RECEIVED])+":").c_str(),
	                                                 WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (6 * GuiSettingManager::m_iTextHeight), ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                 
	m_hWndPageItems[LBL_RECEIVED_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0 B (0 B/s)", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                       ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (6 * GuiSettingManager::m_iTextHeight), rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                       
	m_hWndPageItems[LBL_SENT] = ::CreateWindowEx(0, WC_STATIC,
	                                             (string(LanguageManager::m_Ptr->m_sTexts[LAN_SENT], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SENT])+":").c_str(),
	                                             WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (7 * GuiSettingManager::m_iTextHeight), ScaleGui(150), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                             
	m_hWndPageItems[LBL_SENT_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0 B (0 B/s)", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                   ScaleGui(150) + 18, iPosY + (GuiSettingManager::m_iGroupBoxMargin - 5) + (7 * GuiSettingManager::m_iTextHeight), rcMain.right - (ScaleGui(150) + 31), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
	                                                   
	iPosY += GuiSettingManager::m_iGroupBoxMargin + (8 * GuiSettingManager::m_iTextHeight) + 1;
	
	m_hWndPageItems[BTN_REDIRECT_ALL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ALL_USERS], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                                     4, iPosY + 5, rcMain.right-8, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)BTN_REDIRECT_ALL, ServerManager::m_hInstance, nullptr);
	                                                     
	m_hWndPageItems[BTN_MASS_MSG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MASS_MSG], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                                 4, iPosY + GuiSettingManager::m_iEditHeight + 8, rcMain.right-8, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)BTN_MASS_MSG, ServerManager::m_hInstance, nullptr);
	                                                 
	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++)
	{
		if(m_hWndPageItems[ui8i] == nullptr)
		{
			return false;
		}
		
		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}
	
	GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_START_STOP], GWLP_WNDPROC, (LONG_PTR)SSButtonProc);
	GuiSettingManager::m_wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[BTN_MASS_MSG], GWLP_WNDPROC, (LONG_PTR)LastButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

void MainWindowPageStats::UpdateLanguage()
{
	if(ServerManager::m_bServerRunning == false)
	{
		::SetWindowText(m_hWndPageItems[BTN_START_STOP], LanguageManager::m_Ptr->m_sTexts[LAN_START_HUB]);
		::SetWindowText(m_hWndPageItems[LBL_STATUS_VALUE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_READY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_READY])+".").c_str());
	}
	else
	{
		::SetWindowText(m_hWndPageItems[BTN_START_STOP], LanguageManager::m_Ptr->m_sTexts[LAN_STOP_HUB]);
		::SetWindowText(m_hWndPageItems[LBL_STATUS_VALUE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_RUNNING], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RUNNING])+"...").c_str());
	}
	
	::SetWindowText(m_hWndPageItems[LBL_STATUS], (string(LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_STATUS])+":").c_str());
	::SetWindowText(m_hWndPageItems[LBL_JOINS], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ACCEPTED_CONNECTIONS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ACCEPTED_CONNECTIONS])+":").c_str());
	::SetWindowText(m_hWndPageItems[LBL_PARTS], (string(LanguageManager::m_Ptr->m_sTexts[LAN_CLOSED_CONNECTIONS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CLOSED_CONNECTIONS])+":").c_str());
	::SetWindowText(m_hWndPageItems[LBL_ACTIVE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_ACTIVE_CONNECTIONS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ACTIVE_CONNECTIONS])+":").c_str());
	::SetWindowText(m_hWndPageItems[LBL_ONLINE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_USERS_ONLINE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_USERS_ONLINE])+":").c_str());
	::SetWindowText(m_hWndPageItems[LBL_PEAK], (string(LanguageManager::m_Ptr->m_sTexts[LAN_USERS_PEAK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_USERS_PEAK])+":").c_str());
	::SetWindowText(m_hWndPageItems[LBL_RECEIVED], (string(LanguageManager::m_Ptr->m_sTexts[LAN_RECEIVED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RECEIVED])+":").c_str());
	::SetWindowText(m_hWndPageItems[LBL_SENT], (string(LanguageManager::m_Ptr->m_sTexts[LAN_SENT], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SENT])+":").c_str());
	
	::SetWindowText(m_hWndPageItems[BTN_REDIRECT_ALL], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ALL_USERS]);
	::SetWindowText(m_hWndPageItems[BTN_MASS_MSG], LanguageManager::m_Ptr->m_sTexts[LAN_MASS_MSG]);
}
//---------------------------------------------------------------------------

char * MainWindowPageStats::GetPageName()
{
	return LanguageManager::m_Ptr->m_sTexts[LAN_STATS];
}
//------------------------------------------------------------------------------

void OnRedirectAllOk(char * sLine, const int iLen)
{
	char *sMSG = (char *)malloc(iLen+16);
	if(sMSG == nullptr)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sMSG in OnRedirectAllOk\n", iLen+16);
		return;
	}
	
	int iMsgLen = snprintf(sMSG, iLen+16, "$ForceMove %s|", sLine);
	if(iMsgLen <= 0)
	{
		return;
	}
	
	User * pCur = nullptr,
	       * pNext = Users::m_Ptr->m_pUserListS;
	       
	while(pNext != nullptr)
	{
		pCur = pNext;
		pNext = pCur->m_pNext;
		
		pCur->SendChar(sMSG, iMsgLen);
		// PPK ... close by hub needed !
		pCur->Close(true);
	}
	
	free(sMSG);
}
//---------------------------------------------------------------------------

void MainWindowPageStats::OnRedirectAll()
{
	const char sRedirectAll[] = "[SYS] Redirect All.";
	UdpDebug::m_Ptr->Broadcast(sRedirectAll, sizeof(sRedirectAll)-1);
	
	LineDialog * pRedirectAllDlg = new (std::nothrow) LineDialog(&OnRedirectAllOk);
	
	if(pRedirectAllDlg != nullptr)
	{
		pRedirectAllDlg->DoModal(::GetParent(m_hWnd), LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ALL_USERS_TO],
		                         SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS] == nullptr ? "" : SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS]);
	}
}
//---------------------------------------------------------------------------

void OnMassMessageOk(char * sLine, const int iLen)
{
	char *sMSG = (char *)malloc(iLen+256);
	if(sMSG == nullptr)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sMSG in OnMassMessageOk\n", iLen+256);
		return;
	}
	
	int iMsgLen = snprintf(sMSG, iLen+256, "%s $<%s> %s|", SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false ? SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK] : SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK],
	                       SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false ? SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK] : SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK], sLine);
	if(iMsgLen <= 0)
	{
		return;
	}
	
	GlobalDataQueue::m_Ptr->SingleItemStore(sMSG, iMsgLen, nullptr, 0, GlobalDataQueue::SI_PM2ALL);
	
	free(sMSG);
}

//---------------------------------------------------------------------------

void MainWindowPageStats::OnMassMessage()
{
	LineDialog * pMassMsgDlg = new (std::nothrow) LineDialog(&OnMassMessageOk);
	
	if(pMassMsgDlg != nullptr)
	{
		pMassMsgDlg->DoModal(::GetParent(m_hWnd), LanguageManager::m_Ptr->m_sTexts[LAN_MASS_MSG], "");
	}
}
//---------------------------------------------------------------------------

void MainWindowPageStats::FocusFirstItem()
{
	::SetFocus(m_hWndPageItems[BTN_START_STOP]);
}
//------------------------------------------------------------------------------

void MainWindowPageStats::FocusLastItem()
{
	if(ServerManager::m_bServerRunning == true)
	{
		::SetFocus(m_hWndPageItems[BTN_MASS_MSG]);
	}
	else
	{
		::SetFocus(m_hWndPageItems[BTN_START_STOP]);
	}
}
//------------------------------------------------------------------------------
