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
	memset(&hWndPageItems, 0, sizeof(hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT MainWindowPageStats::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case BTN_START_STOP:
			if (clsServerManager::bServerRunning == false)
			{
				if (clsServerManager::Start() == false)
				{
					::SetWindowText(hWndPageItems[LBL_STATUS_VALUE],
					                (string(clsLanguageManager::mPtr->sTexts[LAN_READY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_READY]) + ".").c_str());
				}
				::SetFocus(hWndPageItems[BTN_START_STOP]);
			}
			else
			{
				clsServerManager::Stop();
				::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
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
		
		::SetWindowPos(hWndPageItems[BTN_START_STOP], NULL, 0, 0, iX - 8, ScaleGui(40), SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[GB_STATS], NULL, 0, 0, iX - 10, clsGuiSettingManager::iGroupBoxMargin + (8 * clsGuiSettingManager::iTextHeight) + 2, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_STATUS_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_JOINS_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_PARTS_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_ACTIVE_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_ONLINE_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_PEAK_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_RECEIVED_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[LBL_SENT_VALUE], NULL, 0, 0, iX - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[BTN_REDIRECT_ALL], NULL, 0, 0, iX - 8, clsGuiSettingManager::iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		::SetWindowPos(hWndPageItems[BTN_MASS_MSG], NULL, 0, 0, iX - 8, clsGuiSettingManager::iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_SETFOCUS:
		::SetFocus(hWndPageItems[BTN_START_STOP]);
		return 0;
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT CALLBACK SSButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return DLGC_WANTTAB;
	}
	else if (uMsg == WM_CHAR && wParam == VK_TAB)
	{
		if ((::GetKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			if (clsServerManager::bServerRunning == true)
			{
				::SetFocus(::GetNextDlgTabItem(clsMainWindow::mPtr->m_hWnd, hWnd, FALSE));
				return 0;
			}
		}
		
		::SetFocus(clsMainWindow::SettingDialog::m_Ptr->m_hWndWindowItems[clsMainWindow::TC_TABS]);
		return 0;
	}
	
	return ::CallWindowProc(clsGuiSettingManager::wpOldButtonProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

bool MainWindowPageStats::CreateMainWindowPage(HWND hOwner)
{
	CreateHWND(hOwner);
	
	RECT rcMain;
	::GetClientRect(m_hWnd, &rcMain);
	
	::SetWindowPos(m_hWnd, NULL, 0, 0, rcMain.right, ScaleGui(40) + clsGuiSettingManager::iGroupBoxMargin + (8 * clsGuiSettingManager::iTextHeight) + (2 * clsGuiSettingManager::iEditHeight) + 13, SWP_NOMOVE | SWP_NOZORDER);
	
	hWndPageItems[BTN_START_STOP] = ::CreateWindowEx(0, WC_BUTTON, clsServerManager::bServerRunning == false ? clsLanguageManager::mPtr->sTexts[LAN_START_HUB] : clsLanguageManager::mPtr->sTexts[LAN_STOP_HUB],
	                                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 4, 3, rcMain.right - 8, ScaleGui(40), m_hWnd, (HMENU)BTN_START_STOP, clsServerManager::hInstance, NULL);
	                                                 
	int iPosY = ScaleGui(40);
	
	hWndPageItems[GB_STATS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                           5, iPosY, rcMain.right - 10, clsGuiSettingManager::iGroupBoxMargin + (8 * clsGuiSettingManager::iTextHeight) + 2, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                           
	hWndPageItems[LBL_STATUS] = ::CreateWindowEx(0, WC_STATIC, (string(clsLanguageManager::mPtr->sTexts[LAN_STATUS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_STATUS]) + ":").c_str(),
	                                             WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5), ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                             
	hWndPageItems[LBL_STATUS_VALUE] = ::CreateWindowEx(0, WC_STATIC, (string(clsLanguageManager::mPtr->sTexts[LAN_READY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_READY]) + ".").c_str(),
	                                                   WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5), rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[LBL_JOINS] = ::CreateWindowEx(0, WC_STATIC,
	                                            (string(clsLanguageManager::mPtr->sTexts[LAN_ACCEPTED_CONNECTIONS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ACCEPTED_CONNECTIONS]) + ":").c_str(),
	                                            WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + clsGuiSettingManager::iTextHeight, ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                            
	hWndPageItems[LBL_JOINS_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                  ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + clsGuiSettingManager::iTextHeight, rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[LBL_PARTS] = ::CreateWindowEx(0, WC_STATIC,
	                                            (string(clsLanguageManager::mPtr->sTexts[LAN_CLOSED_CONNECTIONS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CLOSED_CONNECTIONS]) + ":").c_str(),
	                                            WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (2 * clsGuiSettingManager::iTextHeight), ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                            
	hWndPageItems[LBL_PARTS_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                  ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (2 * clsGuiSettingManager::iTextHeight), rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                  
	hWndPageItems[LBL_ACTIVE] = ::CreateWindowEx(0, WC_STATIC,
	                                             (string(clsLanguageManager::mPtr->sTexts[LAN_ACTIVE_CONNECTIONS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ACTIVE_CONNECTIONS]) + ":").c_str(),
	                                             WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (3 * clsGuiSettingManager::iTextHeight), ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                             
	hWndPageItems[LBL_ACTIVE_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                   ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (3 * clsGuiSettingManager::iTextHeight), rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[LBL_ONLINE] = ::CreateWindowEx(0, WC_STATIC,
	                                             (string(clsLanguageManager::mPtr->sTexts[LAN_USERS_ONLINE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_USERS_ONLINE]) + ":").c_str(),
	                                             WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (4 * clsGuiSettingManager::iTextHeight), ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                             
	hWndPageItems[LBL_ONLINE_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                   ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (4 * clsGuiSettingManager::iTextHeight), rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[LBL_PEAK] = ::CreateWindowEx(0, WC_STATIC,
	                                           (string(clsLanguageManager::mPtr->sTexts[LAN_USERS_PEAK], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_USERS_PEAK]) + ":").c_str(),
	                                           WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (5 * clsGuiSettingManager::iTextHeight), ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                           
	hWndPageItems[LBL_PEAK_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                 ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (5 * clsGuiSettingManager::iTextHeight), rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	hWndPageItems[LBL_RECEIVED] = ::CreateWindowEx(0, WC_STATIC,
	                                               (string(clsLanguageManager::mPtr->sTexts[LAN_RECEIVED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RECEIVED]) + ":").c_str(),
	                                               WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (6 * clsGuiSettingManager::iTextHeight), ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                               
	hWndPageItems[LBL_RECEIVED_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0 B (0 B/s)", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                     ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (6 * clsGuiSettingManager::iTextHeight), rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                     
	hWndPageItems[LBL_SENT] = ::CreateWindowEx(0, WC_STATIC,
	                                           (string(clsLanguageManager::mPtr->sTexts[LAN_SENT], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SENT]) + ":").c_str(),
	                                           WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 13, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (7 * clsGuiSettingManager::iTextHeight), ScaleGui(150), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                           
	hWndPageItems[LBL_SENT_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0 B (0 B/s)", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
	                                                 ScaleGui(150) + 18, iPosY + (clsGuiSettingManager::iGroupBoxMargin - 5) + (7 * clsGuiSettingManager::iTextHeight), rcMain.right - (ScaleGui(150) + 31), clsGuiSettingManager::iTextHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
	                                                 
	iPosY += clsGuiSettingManager::iGroupBoxMargin + (8 * clsGuiSettingManager::iTextHeight) + 1;
	
	hWndPageItems[BTN_REDIRECT_ALL] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ALL_USERS], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                                   4, iPosY + 5, rcMain.right - 8, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)BTN_REDIRECT_ALL, clsServerManager::hInstance, NULL);
	                                                   
	hWndPageItems[BTN_MASS_MSG] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_MASS_MSG], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
	                                               4, iPosY + clsGuiSettingManager::iEditHeight + 8, rcMain.right - 8, clsGuiSettingManager::iEditHeight, m_hWnd, (HMENU)BTN_MASS_MSG, clsServerManager::hInstance, NULL);
	                                               
	for (uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++)
	{
		if (hWndPageItems[ui8i] == NULL)
		{
			return false;
		}
		
		::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_START_STOP], GWLP_WNDPROC, (LONG_PTR)SSButtonProc);
	clsGuiSettingManager::wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_MASS_MSG], GWLP_WNDPROC, (LONG_PTR)LastButtonProc);
	
	return true;
}
//------------------------------------------------------------------------------

void MainWindowPageStats::UpdateLanguage()
{
	if (clsServerManager::bServerRunning == false)
	{
		::SetWindowText(hWndPageItems[BTN_START_STOP], clsLanguageManager::mPtr->sTexts[LAN_START_HUB]);
		::SetWindowText(hWndPageItems[LBL_STATUS_VALUE], (string(clsLanguageManager::mPtr->sTexts[LAN_READY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_READY]) + ".").c_str());
	}
	else
	{
		::SetWindowText(hWndPageItems[BTN_START_STOP], clsLanguageManager::mPtr->sTexts[LAN_STOP_HUB]);
		::SetWindowText(hWndPageItems[LBL_STATUS_VALUE], (string(clsLanguageManager::mPtr->sTexts[LAN_RUNNING], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RUNNING]) + "...").c_str());
	}
	
	::SetWindowText(hWndPageItems[LBL_STATUS], (string(clsLanguageManager::mPtr->sTexts[LAN_STATUS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_STATUS]) + ":").c_str());
	::SetWindowText(hWndPageItems[LBL_JOINS], (string(clsLanguageManager::mPtr->sTexts[LAN_ACCEPTED_CONNECTIONS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ACCEPTED_CONNECTIONS]) + ":").c_str());
	::SetWindowText(hWndPageItems[LBL_PARTS], (string(clsLanguageManager::mPtr->sTexts[LAN_CLOSED_CONNECTIONS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CLOSED_CONNECTIONS]) + ":").c_str());
	::SetWindowText(hWndPageItems[LBL_ACTIVE], (string(clsLanguageManager::mPtr->sTexts[LAN_ACTIVE_CONNECTIONS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ACTIVE_CONNECTIONS]) + ":").c_str());
	::SetWindowText(hWndPageItems[LBL_ONLINE], (string(clsLanguageManager::mPtr->sTexts[LAN_USERS_ONLINE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_USERS_ONLINE]) + ":").c_str());
	::SetWindowText(hWndPageItems[LBL_PEAK], (string(clsLanguageManager::mPtr->sTexts[LAN_USERS_PEAK], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_USERS_PEAK]) + ":").c_str());
	::SetWindowText(hWndPageItems[LBL_RECEIVED], (string(clsLanguageManager::mPtr->sTexts[LAN_RECEIVED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RECEIVED]) + ":").c_str());
	::SetWindowText(hWndPageItems[LBL_SENT], (string(clsLanguageManager::mPtr->sTexts[LAN_SENT], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SENT]) + ":").c_str());
	
	::SetWindowText(hWndPageItems[BTN_REDIRECT_ALL], clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ALL_USERS]);
	::SetWindowText(hWndPageItems[BTN_MASS_MSG], clsLanguageManager::mPtr->sTexts[LAN_MASS_MSG]);
}
//---------------------------------------------------------------------------

char * MainWindowPageStats::GetPageName()
{
	return clsLanguageManager::mPtr->sTexts[LAN_STATS];
}
//------------------------------------------------------------------------------

void OnRedirectAllOk(const char * sLine, const int iLen)
{
	char *sMSG = (char *)malloc(iLen + 16);
	if (sMSG == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sMSG in OnRedirectAllOk\n", iLen + 16);
		return;
	}
	
	int imsgLen = sprintf(sMSG, "$ForceMove %s|", sLine);
	if (CheckSprintf(imsgLen, iLen + 16, "OnRedirectAllOk") == false)
	{
		free(sMSG);
		return;
	}
	
	User * pCur = NULL,
	       * pNext = clsUsers::mPtr->pListS;
	       
	while (pNext != NULL)
	{
		pCur = pNext;
		pNext = pCur->pNext;
		
		pCur->SendChar(sMSG, imsgLen);
		// PPK ... close by hub needed !
		pCur->Close(true);
	}
	
	free(sMSG);
}
//---------------------------------------------------------------------------

void MainWindowPageStats::OnRedirectAll()
{
	const char sRedirectAll[] = "[SYS] Redirect All.";
	clsUdpDebug::mPtr->Broadcast(sRedirectAll, sizeof(sRedirectAll) - 1);
	
	LineDialog * pRedirectAllDlg = new (std::nothrow) LineDialog(&OnRedirectAllOk);
	
	if (pRedirectAllDlg != NULL)
	{
		pRedirectAllDlg->DoModal(::GetParent(m_hWnd), clsLanguageManager::mPtr->sTexts[LAN_REDIRECT_ALL_USERS_TO],
		                         clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS] == NULL ? "" : clsSettingManager::mPtr->sTexts[SETTXT_REDIRECT_ADDRESS]);
	}
}
//---------------------------------------------------------------------------

void OnMassMessageOk(const char * sLine, const int iLen)
{
	char *sMSG = (char *)malloc(iLen + 256);
	if (sMSG == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %d bytes for sMSG in OnMassMessageOk\n", iLen + 256);
		return;
	}
	
	int imsgLen = sprintf(sMSG, "%s $<%s> %s|", clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false ? clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK] : clsSettingManager::mPtr->sTexts[SETTXT_BOT_NICK],
	                      clsSettingManager::mPtr->bBools[SETBOOL_REG_BOT] == false ? clsSettingManager::mPtr->sTexts[SETTXT_ADMIN_NICK] : clsSettingManager::mPtr->sTexts[SETTXT_BOT_NICK], sLine);
	if (CheckSprintf(imsgLen, iLen + 256, "OnMassMessageOk") == false)
	{
		free(sMSG);
		return;
	}
	
	clsGlobalDataQueue::mPtr->SingleItemStore(sMSG, imsgLen, NULL, 0, clsGlobalDataQueue::SI_PM2ALL);
	
	free(sMSG);
}

//---------------------------------------------------------------------------

void MainWindowPageStats::OnMassMessage()
{
	LineDialog * pMassMsgDlg = new (std::nothrow) LineDialog(&OnMassMessageOk);
	
	if (pMassMsgDlg != NULL)
	{
		pMassMsgDlg->DoModal(::GetParent(m_hWnd), clsLanguageManager::mPtr->sTexts[LAN_MASS_MSG], "");
	}
}
//---------------------------------------------------------------------------

void MainWindowPageStats::FocusFirstItem()
{
	::SetFocus(hWndPageItems[BTN_START_STOP]);
}
//------------------------------------------------------------------------------

void MainWindowPageStats::FocusLastItem()
{
	if (clsServerManager::bServerRunning == true)
	{
		::SetFocus(hWndPageItems[BTN_MASS_MSG]);
	}
	else
	{
		::SetFocus(hWndPageItems[BTN_START_STOP]);
	}
}
//------------------------------------------------------------------------------
