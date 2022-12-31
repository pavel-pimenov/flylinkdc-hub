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
#include "MainWindow.h"
//---------------------------------------------------------------------------
#include "../core/hashBanManager.h"
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/LuaScriptManager.h"
#include "../core/ProfileManager.h"
#include "../core/ServerManager.h"
#include "../core/serviceLoop.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "AboutDialog.h"
#include "BansDialog.h"
#include "MainWindowPageScripts.h"
#include "MainWindowPageStats.h"
#include "MainWindowPageUsersChat.h"
#include "ProfilesDialog.h"
#include "RangeBansDialog.h"
#include "RegisteredUsersDialog.h"
#include "Resources.h"
#include "SettingDialog.h"
#include "UpdateDialog.h"
#include "../core/TextFileManager.h"
#include "../core/UpdateCheckThread.h"
//---------------------------------------------------------------------------
#define WM_TRAYICON (WM_USER+10)
//---------------------------------------------------------------------------
MainWindow * MainWindow::m_Ptr = nullptr;
//---------------------------------------------------------------------------
static const char sPtokaXDash[] = "PtokaX - ";
static const size_t szPtokaXDashLen = sizeof(sPtokaXDash)-1;
//---------------------------------------------------------------------------
#ifndef _WIN64
static uint64_t (*GetActualTick)();

uint64_t PXGetTickCount()
{
	return (uint64_t)(::GetTickCount() / 1000);
}

typedef ULONGLONG (WINAPI *GTC64)(void);
GTC64 pGTC64 = nullptr;

uint64_t PXGetTickCount64()
{
	return (pGTC64() / 1000);
}
#endif
//---------------------------------------------------------------------------

MainWindow::MainWindow() : m_hWnd(nullptr), m_ui64LastTrayMouseMove(0), m_uiTaskBarCreated(0)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
	memset(&m_MainWindowPages, 0, sizeof(m_MainWindowPages));
}
//---------------------------------------------------------------------------

MainWindow::~MainWindow()
{
	delete GuiSettingManager::m_Ptr;
	
	for(uint8_t ui8i = 0; ui8i < (sizeof(m_MainWindowPages) / sizeof(m_MainWindowPages[0])); ui8i++)
	{
		delete m_MainWindowPages[ui8i];
	}
	
	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = 0;
	::Shell_NotifyIcon(NIM_DELETE, &nid);
	::DeleteObject(GuiSettingManager::m_hFont);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK MainWindow::StaticMainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_NCCREATE)
	{
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)MainWindow::m_Ptr);
		MainWindow::m_Ptr->m_hWnd = hWnd;
	}
	else
	{
		if(::GetWindowLongPtr(hWnd, GWLP_USERDATA) == 0)
		{
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	
	return MainWindow::m_Ptr->MainWindowProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT MainWindow::MainWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_PX_DO_LOOP:
	{
		ServiceLoop::m_Ptr->Looper();
		return 0;
	}
	case WM_CREATE:
	{
		{
			HWND hCombo = ::CreateWindowEx(0, WC_COMBOBOX, nullptr, WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr,
			                               ServerManager::m_hInstance, nullptr);
			                               
			if(hCombo != nullptr)
			{
				::SendMessage(hCombo, WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
				
				GuiSettingManager::m_iGroupBoxMargin = (int)::SendMessage(hCombo, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
				GuiSettingManager::m_iEditHeight = GuiSettingManager::m_iGroupBoxMargin + (::GetSystemMetrics(SM_CXFIXEDFRAME) * 2);
				GuiSettingManager::m_iTextHeight = GuiSettingManager::m_iGroupBoxMargin - 2;
				GuiSettingManager::m_iCheckHeight = max(GuiSettingManager::m_iTextHeight - 2, ::GetSystemMetrics(SM_CYSMICON));
				
				::DestroyWindow(hCombo);
			}
			
			HWND hUpDown = ::CreateWindowEx(0, UPDOWN_CLASS, nullptr, WS_CHILD | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT,
			                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, GuiSettingManager::m_iEditHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
			                                
			if(hUpDown != nullptr)
			{
				RECT rcUpDown;
				::GetWindowRect(hUpDown, &rcUpDown);
				
				GuiSettingManager::m_iUpDownWidth = rcUpDown.right - rcUpDown.left;
				
				::DestroyWindow(hUpDown);
			}
			
			GuiSettingManager::m_iOneLineGB = GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 8;
			GuiSettingManager::m_iOneLineOneChecksGB = GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + GuiSettingManager::m_iEditHeight + 12;
			GuiSettingManager::m_iOneLineTwoChecksGB = GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + GuiSettingManager::m_iEditHeight + 15;
			
			SettingPage::m_iOneCheckGB = GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iCheckHeight + 8;
			SettingPage::m_iTwoChecksGB = GuiSettingManager::m_iGroupBoxMargin + (2 * GuiSettingManager::m_iCheckHeight) + 3 + 8;
			SettingPage::m_iOneLineTwoGroupGB = GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + (2 * GuiSettingManager::m_iOneLineGB) + 7;
			SettingPage::m_iTwoLineGB = GuiSettingManager::m_iGroupBoxMargin  + (2 * GuiSettingManager::m_iEditHeight) + 13;
			SettingPage::m_iThreeLineGB = GuiSettingManager::m_iGroupBoxMargin + (3 * GuiSettingManager::m_iEditHeight) + 18;
		}
		
		RECT rcMain;
		::GetClientRect(m_hWnd, &rcMain);
		
		DWORD dwTabsStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP | TCS_TABS | TCS_FOCUSNEVER;
		
		BOOL bHotTrackEnabled = FALSE;
		::SystemParametersInfo(SPI_GETHOTTRACKING, 0, &bHotTrackEnabled, 0);
		
		if(bHotTrackEnabled == TRUE)
		{
			dwTabsStyle |= TCS_HOTTRACK;
		}
		
		m_hWndWindowItems[TC_TABS] = ::CreateWindowEx(0, WC_TABCONTROL, nullptr, dwTabsStyle, 0, 0, rcMain.right, GuiSettingManager::m_iEditHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);
		::SendMessage(m_hWndWindowItems[TC_TABS], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
		
		TCITEM tcItem = { 0 };
		tcItem.mask = TCIF_TEXT | TCIF_PARAM;
		
		for(uint8_t ui8i = 0; ui8i < (sizeof(m_MainWindowPages) / sizeof(m_MainWindowPages[0])); ui8i++)
		{
			if(m_MainWindowPages[ui8i] != nullptr)
			{
				if(m_MainWindowPages[ui8i]->CreateMainWindowPage(m_hWnd) == false)
				{
					::MessageBox(m_hWnd, "Main window creation failed!", g_sPtokaXTitle, MB_OK);
				}
				
				tcItem.pszText = m_MainWindowPages[ui8i]->GetPageName();
				tcItem.lParam = (LPARAM)m_MainWindowPages[ui8i];
				::SendMessage(m_hWndWindowItems[TC_TABS], TCM_INSERTITEM, ui8i, (LPARAM)&tcItem);
			}
			
			if(ui8i == 0 && GuiSettingManager::m_Ptr->m_i32Integers[GUISETINT_MAIN_WINDOW_HEIGHT] == GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_MAIN_WINDOW_HEIGHT))
			{
				RECT rcPage = { 0 };
				::GetWindowRect(m_MainWindowPages[0]->m_hWnd, &rcPage);
				
				int iDiff = (rcMain.bottom) - (rcPage.bottom-rcPage.top) - (GuiSettingManager::m_iEditHeight + 1);
				
				::GetWindowRect(m_hWnd, &rcMain);
				
				if(iDiff != 0)
				{
					::SetWindowPos(m_hWnd, nullptr, 0, 0, (rcMain.right-rcMain.left), (rcMain.bottom-rcMain.top) - iDiff, SWP_NOMOVE | SWP_NOZORDER);
				}
			}
		}
		
		GuiSettingManager::m_wpOldTabsProc = (WNDPROC)::SetWindowLongPtr(m_hWndWindowItems[TC_TABS], GWLP_WNDPROC, (LONG_PTR)TabsProc);
		
		if(SettingManager::m_Ptr->m_bBools[SETBOOL_CHECK_NEW_RELEASES] == true)
		{
			// Create update check thread
			UpdateCheckThread::m_Ptr = new (std::nothrow) UpdateCheckThread();
			if(UpdateCheckThread::m_Ptr == nullptr)
			{
				AppendDebugLog("%s - [MEM] Cannot allocate UpdateCheckThread::m_Ptr in MainWindow::MainWindowProc::WM_CREATE\n");
				exit(EXIT_FAILURE);
			}
			
			// Start the update check thread
			UpdateCheckThread::m_Ptr->Resume();
		}
		
		return 0;
	}
	case WM_CLOSE:
		if(::MessageBox(m_hWnd, (string(LanguageManager::m_Ptr->m_sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
		{
			ServerManager::m_bIsClose = true;
			
			RECT rcMain;
			::GetWindowRect(m_hWnd, &rcMain);
			
			GuiSettingManager::m_Ptr->SetInteger(GUISETINT_MAIN_WINDOW_WIDTH, rcMain.right - rcMain.left);
			GuiSettingManager::m_Ptr->SetInteger(GUISETINT_MAIN_WINDOW_HEIGHT, rcMain.bottom - rcMain.top);
			
			// stop server if running and save settings
			if(ServerManager::m_bServerRunning == true)
			{
				ServerManager::Stop();
				return 0;
			}
			else
			{
				ServerManager::FinalClose();
			}
			
			break;
		}
		
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(1);
		break;
	case WM_TRAYICON:
		if(lParam == WM_MOUSEMOVE)
		{
#ifndef _WIN64
			if(m_ui64LastTrayMouseMove != GetActualTick())
			{
				m_ui64LastTrayMouseMove = GetActualTick();
#else
			if(m_ui64LastTrayMouseMove != GetTickCount64())
			{
				m_ui64LastTrayMouseMove = GetTickCount64();
#endif
				NOTIFYICONDATA nid;
				memset(&nid, 0, sizeof(NOTIFYICONDATA));
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
				nid.hIcon = (HICON)::LoadImage(ServerManager::m_hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
				nid.uCallbackMessage = WM_TRAYICON;
				
				char msg[256];
				int iMsgLen = 0;
				if(ServerManager::m_bServerRunning == false)
				{
					msg[0] = '\0';
				}
				else
				{
					iMsgLen = snprintf(msg, 256, " (%s: %u)", LanguageManager::m_Ptr->m_sTexts[LAN_USERS], ServerManager::m_ui32Logged);
					if(iMsgLen <= 0)
					{
						return 0;
					}
				}
				
				const size_t szSize = sizeof(nid.szTip);
				
				if(szSize < (size_t)(SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME]+iMsgLen+10))
				{
					nid.szTip[szSize-1] = '\0';
					memcpy(nid.szTip+(szSize-(iMsgLen+1)), msg, iMsgLen);
					nid.szTip[szSize-(iMsgLen+2)] = '.';
					nid.szTip[szSize-(iMsgLen+3)] = '.';
					nid.szTip[szSize-(iMsgLen+4)] = '.';
					memcpy(nid.szTip+szPtokaXDashLen, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], szSize-(iMsgLen+13));
					memcpy(nid.szTip, sPtokaXDash, szPtokaXDashLen);
				}
				else
				{
					memcpy(nid.szTip, sPtokaXDash, szPtokaXDashLen);
					memcpy(nid.szTip+szPtokaXDashLen, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME]);
					memcpy(nid.szTip+szPtokaXDashLen+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME], msg, iMsgLen);
					nid.szTip[szPtokaXDashLen+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME]+iMsgLen] = '\0';
				}
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
		else if(lParam == WM_LBUTTONUP)
		{
			::ShowWindow(m_hWnd, SW_SHOW);
			::ShowWindow(m_hWnd, SW_RESTORE);
		}
		
		return 0;
	case WM_SIZE:
		if(wParam == SIZE_MINIMIZED && SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
		{
			::ShowWindow(m_hWnd, SW_HIDE);
		}
		else
		{
			RECT rc;
			::SetRect(&rc, 0, 0, GET_X_LPARAM(lParam), GuiSettingManager::m_iEditHeight);
			::SendMessage(m_hWndWindowItems[TC_TABS], TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);
			
			HDWP hdwp = ::BeginDeferWindowPos(3);
			
			::DeferWindowPos(hdwp, m_hWndWindowItems[TC_TABS], nullptr, 0, 0, GET_X_LPARAM(lParam), GuiSettingManager::m_iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
			
			for(uint8_t ui8i = 0; ui8i < (sizeof(m_MainWindowPages) / sizeof(m_MainWindowPages[0])); ui8i++)
			{
				if(m_MainWindowPages[ui8i] != nullptr)
				{
					::DeferWindowPos(hdwp, m_MainWindowPages[ui8i]->m_hWnd, nullptr, 0, GuiSettingManager::m_iEditHeight + 1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) - (GuiSettingManager::m_iEditHeight + 1),
					                 SWP_NOMOVE | SWP_NOZORDER);
				}
			}
			
			::EndDeferWindowPos(hdwp);
		}
		
		return 0;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_MAIN_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(GuiSettingManager::m_Ptr->GetDefaultInteger(GUISETINT_MAIN_WINDOW_HEIGHT));
		
		return 0;
	}
	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[TC_TABS])
		{
			if(((LPNMHDR)lParam)->code == TCN_SELCHANGE)
			{
				OnSelChanged();
				return 0;
			}
			else if(((LPNMHDR)lParam)->code == TCN_KEYDOWN )
			{
				NMTCKEYDOWN * ptckd = (NMTCKEYDOWN *)lParam;
				if(ptckd->wVKey == VK_TAB)
				{
					int iPage = (int)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETCURSEL, 0, 0);
					
					if(iPage == -1)
					{
						break;
					}
					
					TCITEM tcItem = { 0 };
					tcItem.mask = TCIF_PARAM;
					
					if((BOOL)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETITEM, iPage, (LPARAM)&tcItem) == FALSE)
					{
						break;
					}
					
					if(tcItem.lParam == 0)
					{
						::MessageBox(m_hWnd, "Not implemented!", g_sPtokaXTitle, MB_OK);
					}
					
					MainWindowPage * curMainWindowPage = reinterpret_cast<MainWindowPage *>(tcItem.lParam);
					
					if((::GetKeyState(VK_SHIFT) & 0x8000) > 0)
					{
						curMainWindowPage->FocusLastItem();
						return 0;
					}
					else
					{
						curMainWindowPage->FocusFirstItem();
						return 0;
					}
				}
			}
		}
		
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_EXIT:
			::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
			return 0;
		case IDC_SETTINGS:
		{
			SettingDialog::m_Ptr = new (std::nothrow) SettingDialog();
			
			if(SettingDialog::m_Ptr != nullptr)
			{
				SettingDialog::m_Ptr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_REG_USERS:
		{
			RegisteredUsersDialog::m_Ptr = new (std::nothrow) RegisteredUsersDialog();
			
			if(RegisteredUsersDialog::m_Ptr != nullptr)
			{
				RegisteredUsersDialog::m_Ptr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_PROFILES:
		{
			ProfilesDialog::m_Ptr = new (std::nothrow) ProfilesDialog();
			
			if(ProfilesDialog::m_Ptr != nullptr)
			{
				ProfilesDialog::m_Ptr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_BANS:
		{
			BansDialog::m_Ptr = new (std::nothrow) BansDialog();
			
			if(BansDialog::m_Ptr != nullptr)
			{
				BansDialog::m_Ptr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_RANGE_BANS:
		{
			RangeBansDialog::m_Ptr = new (std::nothrow) RangeBansDialog();
			
			if(RangeBansDialog::m_Ptr != nullptr)
			{
				RangeBansDialog::m_Ptr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_ABOUT:
		{
			AboutDialog * pAboutDlg = new (std::nothrow) AboutDialog();
			
			if(pAboutDlg != nullptr)
			{
				pAboutDlg->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_HOMEPAGE:
			::ShellExecute(nullptr, nullptr, "http://www.PtokaX.org", nullptr, nullptr, SW_SHOWNORMAL);
			return 0;
		case IDC_FORUM:
			::ShellExecute(nullptr, nullptr, "http://forum.PtokaX.org/", nullptr, nullptr, SW_SHOWNORMAL);
			return 0;
		case IDC_WIKI:
			::ShellExecute(nullptr, nullptr, "http://wiki.PtokaX.org", nullptr, nullptr, SW_SHOWNORMAL);
			return 0;
		case IDC_UPDATE_CHECK:
		{
			UpdateDialog::m_Ptr = new (std::nothrow) UpdateDialog();
			
			if(UpdateDialog::m_Ptr != nullptr)
			{
				UpdateDialog::m_Ptr->DoModal(m_hWnd);
				
				// First destroy old update check thread if any
				if(UpdateCheckThread::m_Ptr != nullptr)
				{
					UpdateCheckThread::m_Ptr->Close();
					UpdateCheckThread::m_Ptr->WaitFor();
				}
				
				delete UpdateCheckThread::m_Ptr;
				UpdateCheckThread::m_Ptr = nullptr;
				
				// Create update check thread
				UpdateCheckThread::m_Ptr = new (std::nothrow) UpdateCheckThread();
				if(UpdateCheckThread::m_Ptr == nullptr)
				{
					AppendDebugLog("%s - [MEM] Cannot allocate UpdateCheckThread::m_Ptr in MainWindow::MainWindowProc::IDC_UPDATE_CHECK\n");
					exit(EXIT_FAILURE);
				}
				
				// Start the update check thread
				UpdateCheckThread::m_Ptr->Resume();
			}
			
			return 0;
		}
		case IDC_SAVE_SETTINGS:
			BanManager::m_Ptr->Save(true);
			ProfileManager::m_Ptr->SaveProfiles();
			RegManager::m_Ptr->Save();
			ScriptManager::m_Ptr->SaveScripts();
			SettingManager::m_Ptr->Save();
			
			::MessageBox(m_hWnd, LanguageManager::m_Ptr->m_sTexts[LAN_SETTINGS_SAVED], g_sPtokaXTitle, MB_OK | MB_ICONINFORMATION);
			return 0;
		case IDC_RELOAD_TXTS:
			TextFilesManager::m_Ptr->RefreshTextFiles();
			
			::MessageBox(m_hWnd, (string(LanguageManager::m_Ptr->m_sTexts[LAN_TXT_FILES_RELOADED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_TXT_FILES_RELOADED])+".").c_str(), g_sPtokaXTitle, MB_OK | MB_ICONINFORMATION);
			
			return 0;
		}
		
		break;
	case WM_QUERYENDSESSION:
		BanManager::m_Ptr->Save(true);
		ProfileManager::m_Ptr->SaveProfiles();
		RegManager::m_Ptr->Save();
		ScriptManager::m_Ptr->SaveScripts();
		SettingManager::m_Ptr->Save();
		
		AppendLog("Serving stopped (Windows shutdown).");
		
		return TRUE;
	case WM_UPDATE_CHECK_MSG:
	{
		char * sMsg = (char *)lParam;
		if(UpdateDialog::m_Ptr != nullptr)
		{
			UpdateDialog::m_Ptr->Message(sMsg);
		}
		
		free(sMsg);
		
		return 0;
	}
	case WM_UPDATE_CHECK_DATA:
	{
		char * sMsg = (char *)lParam;
		
		if(UpdateDialog::m_Ptr == nullptr)
		{
			UpdateDialog::m_Ptr = new (std::nothrow) UpdateDialog();
			
			if(UpdateDialog::m_Ptr != nullptr)
			{
				if(UpdateDialog::m_Ptr->ParseData(sMsg, m_hWnd) == false)
				{
					delete UpdateDialog::m_Ptr;
				}
			}
		}
		else
		{
			UpdateDialog::m_Ptr->ParseData(sMsg, m_hWnd);
		}
		
		free(sMsg);
		
		return 0;
	}
	case WM_UPDATE_CHECK_TERMINATE:
		if(UpdateCheckThread::m_Ptr != nullptr)
		{
			UpdateCheckThread::m_Ptr->Close();
			UpdateCheckThread::m_Ptr->WaitFor();
		}
		
		delete UpdateCheckThread::m_Ptr;
		UpdateCheckThread::m_Ptr = nullptr;
		
		return 0;
	case WM_SETFOCUS:
		::SetFocus(m_hWndWindowItems[TC_TABS]);
		return 0;
	}
	
	if(uMsg == m_uiTaskBarCreated)
	{
		UpdateSysTray();
		return 0;
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

HWND MainWindow::CreateEx()
{
	GuiSettingManager::m_Ptr = new (std::nothrow) GuiSettingManager();
	
	if(GuiSettingManager::m_Ptr == nullptr)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate GuiSettingManager::m_Ptr in MainWindow::MainWindow\n");
		exit(EXIT_FAILURE);
	}
	
	INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES |
	                              ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS
	                            };
	InitCommonControlsEx(&iccx);
	
	m_MainWindowPages[0] = new (std::nothrow) MainWindowPageStats();
	m_MainWindowPages[1] = new (std::nothrow) MainWindowPageUsersChat();
	m_MainWindowPages[2] = new (std::nothrow) MainWindowPageScripts();
	
	for(uint8_t ui8i = 0; ui8i < 3; ui8i++)
	{
		if(m_MainWindowPages[ui8i] == nullptr)
		{
			AppendDebugLogFormat("[MEM] Cannot allocate MainWindowPage[%" PRIu8 "] in MainWindow::MainWindow\n", ui8i);
			exit(EXIT_FAILURE);
		}
	}
	
	m_uiTaskBarCreated = ::RegisterWindowMessage("TaskbarCreated");
	
	NONCLIENTMETRICS NCM = { 0 };
	NCM.cbSize = sizeof(NONCLIENTMETRICS);
	
	OSVERSIONINFOEX ver;
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	
#ifndef _WIN64
	if(::GetVersionEx((OSVERSIONINFO*)&ver) != 0 && ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion < 6)
	{
		NCM.cbSize -= sizeof(int);
	}
#endif
	
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NCM, 0);
	
	if(NCM.lfMessageFont.lfHeight > 0)
	{
		GuiSettingManager::m_fScaleFactor = (float)(NCM.lfMessageFont.lfHeight / 12.0);
	}
	else if(NCM.lfMessageFont.lfHeight < 0)
	{
		GuiSettingManager::m_fScaleFactor = float(NCM.lfMessageFont.lfHeight / -12.0);
	}
	
	GuiSettingManager::m_hFont = ::CreateFontIndirect(&NCM.lfMessageFont);
	
	GuiSettingManager::m_hArrowCursor = (HCURSOR)::LoadImage(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	GuiSettingManager::m_hVerticalCursor = (HCURSOR)::LoadImage(nullptr, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	
	HMENU hMainMenu = ::CreateMenu();
	
	HMENU hFileMenu = ::CreatePopupMenu();
	::AppendMenu(hFileMenu, MF_STRING, IDC_RELOAD_TXTS, (string(LanguageManager::m_Ptr->m_sTexts[LAN_RELOAD_TEXT_FILES], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RELOAD_TEXT_FILES]) + "...").c_str());
	::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hFileMenu, MF_STRING, IDC_SETTINGS, (string(LanguageManager::m_Ptr->m_sTexts[LAN_MENU_SETTINGS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_MENU_SETTINGS]) + "...").c_str());
	::AppendMenu(hFileMenu, MF_STRING, IDC_SAVE_SETTINGS, (string(LanguageManager::m_Ptr->m_sTexts[LAN_SAVE_SETTINGS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SAVE_SETTINGS]) + "...").c_str());
	::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hFileMenu, MF_STRING, IDC_EXIT, LanguageManager::m_Ptr->m_sTexts[LAN_EXIT]);
	
	::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hFileMenu, LanguageManager::m_Ptr->m_sTexts[LAN_FILE]);
	
	HMENU hViewMenu = ::CreatePopupMenu();
	::AppendMenu(hViewMenu, MF_STRING, IDC_REG_USERS, LanguageManager::m_Ptr->m_sTexts[LAN_REG_USERS]);
	::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hViewMenu, MF_STRING, IDC_PROFILES, LanguageManager::m_Ptr->m_sTexts[LAN_PROFILES]);
	::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hViewMenu, MF_STRING, IDC_BANS, LanguageManager::m_Ptr->m_sTexts[LAN_BANS]);
	::AppendMenu(hViewMenu, MF_STRING, IDC_RANGE_BANS, LanguageManager::m_Ptr->m_sTexts[LAN_RANGE_BANS]);
	
	::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hViewMenu, LanguageManager::m_Ptr->m_sTexts[LAN_VIEW]);
	
	HMENU hHelpMenu = ::CreatePopupMenu();
	::AppendMenu(hHelpMenu, MF_STRING, IDC_UPDATE_CHECK, (string(LanguageManager::m_Ptr->m_sTexts[LAN_CHECK_FOR_UPDATE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CHECK_FOR_UPDATE]) + "...").c_str());
	::AppendMenu(hHelpMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hHelpMenu, MF_STRING, IDC_HOMEPAGE, (string("PtokaX ") +LanguageManager::m_Ptr->m_sTexts[LAN_WEBSITE]).c_str());
	::AppendMenu(hHelpMenu, MF_STRING, IDC_FORUM, (string("PtokaX ") +LanguageManager::m_Ptr->m_sTexts[LAN_FORUM]).c_str());
	::AppendMenu(hHelpMenu, MF_STRING, IDC_WIKI, (string("PtokaX ") +LanguageManager::m_Ptr->m_sTexts[LAN_WIKI]).c_str());
	::AppendMenu(hHelpMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hHelpMenu, MF_STRING, IDC_ABOUT, (string(LanguageManager::m_Ptr->m_sTexts[LAN_MENU_ABOUT], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_MENU_ABOUT]) + " PtokaX").c_str());
	
	::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hHelpMenu, LanguageManager::m_Ptr->m_sTexts[LAN_HELP]);
	
	WNDCLASSEX m_wc;
	memset(&m_wc, 0, sizeof(WNDCLASSEX));
	m_wc.cbSize = sizeof(WNDCLASSEX);
	m_wc.lpfnWndProc = StaticMainWindowProc;
	m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	m_wc.lpszClassName = g_sPtokaXTitle;
	m_wc.hInstance = ServerManager::m_hInstance;
	m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
	m_wc.style = CS_HREDRAW | CS_VREDRAW;
	m_wc.hIcon = (HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(IDR_MAINICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR | LR_SHARED);
	m_wc.hIconSm = (HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
	
	ATOM atom = ::RegisterClassEx(&m_wc);
	
	m_hWnd = ::CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, MAKEINTATOM(atom), (string(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME]) + " | " + g_sPtokaXTitle).c_str(),
	                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, ScaleGuiDefaultsOnly(GUISETINT_MAIN_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_MAIN_WINDOW_HEIGHT), nullptr, hMainMenu, ServerManager::m_hInstance, nullptr);
	                          
#ifndef _WIN64
	if(::GetVersionEx((OSVERSIONINFO*)&ver) != 0 && ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion >= 6)
	{
		pGTC64 = (GTC64)::GetProcAddress(::GetModuleHandle("kernel32.dll"), "GetTickCount64");
		if(pGTC64 != nullptr)
		{
			GetActualTick = PXGetTickCount64;
			return m_hWnd;
		}
	}
	
	GetActualTick = PXGetTickCount;
#endif
	
	return m_hWnd;
}
//---------------------------------------------------------------------------

void MainWindow::UpdateSysTray() const
{
	if(ServerManager::m_bCmdNoTray == false)
	{
		NOTIFYICONDATA nid;
		memset(&nid, 0, sizeof(NOTIFYICONDATA));
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_ICON | NIF_MESSAGE;
		nid.hIcon = (HICON)::LoadImage(ServerManager::m_hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
		nid.uCallbackMessage = WM_TRAYICON;
		
		if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
		{
			::Shell_NotifyIcon(NIM_ADD, &nid);
		}
		else
		{
			::Shell_NotifyIcon(NIM_DELETE, &nid);
		}
	}
}
//---------------------------------------------------------------------------

void MainWindow::UpdateStats() const
{
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_JOINS_VALUE], string(ServerManager::m_ui32Joins).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_PARTS_VALUE], string(ServerManager::m_ui32Parts).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_ACTIVE_VALUE], string(ServerManager::m_ui32Joins - ServerManager::m_ui32Parts).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_ONLINE_VALUE], string(ServerManager::m_ui32Logged).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_PEAK_VALUE], string(ServerManager::m_ui32Peak).c_str());
	
	char msg[256];
	int iMsglen = snprintf(msg, 256, "%s (%s)", formatBytes(ServerManager::m_ui64BytesRead), formatBytesPerSecond(ServerManager::m_ui32ActualBytesRead));
	if(iMsglen > 0)
	{
		::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_RECEIVED_VALUE], msg);
	}
	
	iMsglen = snprintf(msg, 256, "%s (%s)", formatBytes(ServerManager::m_ui64BytesSent), formatBytesPerSecond(ServerManager::m_ui32ActualBytesSent));
	if(iMsglen > 0)
	{
		::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_SENT_VALUE], msg);
	}
}
//---------------------------------------------------------------------------

void MainWindow::UpdateTitleBar()
{
	::SetWindowText(m_hWnd, (string(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME]) + " | " + g_sPtokaXTitle).c_str());
}
//---------------------------------------------------------------------------

void MainWindow::UpdateLanguage()
{
	for(uint8_t ui8i = 0; ui8i < (sizeof(m_MainWindowPages) / sizeof(m_MainWindowPages[0])); ui8i++)
	{
		if(m_MainWindowPages[ui8i] != nullptr)
		{
			m_MainWindowPages[ui8i]->UpdateLanguage();
			
			TCITEM tcItem = { 0 };
			tcItem.mask = TCIF_TEXT;
			
			tcItem.pszText = m_MainWindowPages[ui8i]->GetPageName();
			::SendMessage(m_hWndWindowItems[TC_TABS], TCM_SETITEM, ui8i, (LPARAM)&tcItem);
		}
	}
	
	HMENU hMenu = ::GetMenu(m_hWnd);
	
	::ModifyMenu(hMenu, 0, MF_BYPOSITION, 0, LanguageManager::m_Ptr->m_sTexts[LAN_FILE]);
	::ModifyMenu(hMenu, 1, MF_BYPOSITION, 0, LanguageManager::m_Ptr->m_sTexts[LAN_VIEW]);
	::ModifyMenu(hMenu, 2, MF_BYPOSITION, 0, LanguageManager::m_Ptr->m_sTexts[LAN_HELP]);
	
	::ModifyMenu(hMenu, IDC_EXIT, MF_BYCOMMAND, IDC_EXIT, LanguageManager::m_Ptr->m_sTexts[LAN_EXIT]);
	::ModifyMenu(hMenu, IDC_REG_USERS, MF_BYCOMMAND, IDC_REG_USERS, LanguageManager::m_Ptr->m_sTexts[LAN_REG_USERS]);
	::ModifyMenu(hMenu, IDC_PROFILES, MF_BYCOMMAND, IDC_PROFILES, LanguageManager::m_Ptr->m_sTexts[LAN_PROFILES]);
	::ModifyMenu(hMenu, IDC_BANS, MF_BYCOMMAND, IDC_BANS, LanguageManager::m_Ptr->m_sTexts[LAN_BANS]);
	::ModifyMenu(hMenu, IDC_RANGE_BANS, MF_BYCOMMAND, IDC_RANGE_BANS, LanguageManager::m_Ptr->m_sTexts[LAN_RANGE_BANS]);
	
	::ModifyMenu(hMenu, IDC_HOMEPAGE, MF_BYCOMMAND, IDC_HOMEPAGE, (string("PtokaX ") +LanguageManager::m_Ptr->m_sTexts[LAN_WEBSITE]).c_str());
	::ModifyMenu(hMenu, IDC_FORUM, MF_BYCOMMAND, IDC_FORUM, (string("PtokaX ") +LanguageManager::m_Ptr->m_sTexts[LAN_FORUM]).c_str());
	::ModifyMenu(hMenu, IDC_WIKI, MF_BYCOMMAND, IDC_WIKI, (string("PtokaX ") +LanguageManager::m_Ptr->m_sTexts[LAN_WIKI]).c_str());
	
	::ModifyMenu(hMenu, IDC_ABOUT, MF_BYCOMMAND, IDC_ABOUT,
	             (string(LanguageManager::m_Ptr->m_sTexts[LAN_MENU_ABOUT], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_MENU_ABOUT]) + " PtokaX").c_str());
	             
	::ModifyMenu(hMenu, IDC_UPDATE_CHECK, MF_BYCOMMAND, IDC_UPDATE_CHECK,
	             (string(LanguageManager::m_Ptr->m_sTexts[LAN_CHECK_FOR_UPDATE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CHECK_FOR_UPDATE]) + "...").c_str());
	::ModifyMenu(hMenu, IDC_RELOAD_TXTS, MF_BYCOMMAND, IDC_RELOAD_TXTS,
	             (string(LanguageManager::m_Ptr->m_sTexts[LAN_RELOAD_TEXT_FILES], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RELOAD_TEXT_FILES]) + "...").c_str());
	::ModifyMenu(hMenu, IDC_SETTINGS, MF_BYCOMMAND, IDC_SETTINGS,
	             (string(LanguageManager::m_Ptr->m_sTexts[LAN_MENU_SETTINGS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_MENU_SETTINGS]) + "...").c_str());
	::ModifyMenu(hMenu, IDC_SAVE_SETTINGS, MF_BYCOMMAND, IDC_SAVE_SETTINGS,
	             (string(LanguageManager::m_Ptr->m_sTexts[LAN_SAVE_SETTINGS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SAVE_SETTINGS]) + "...").c_str());
}
//---------------------------------------------------------------------------

void MainWindow::EnableStartButton(const BOOL bEnable) const
{
	::EnableWindow((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::BTN_START_STOP], bEnable);
}
//---------------------------------------------------------------------------

void MainWindow::SetStartButtonText(const char * sText) const
{
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::BTN_START_STOP], sText);
}
//---------------------------------------------------------------------------

void MainWindow::SetStatusValue(const char * sText) const
{
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[MainWindowPageStats::LBL_STATUS_VALUE], sText);
}
//---------------------------------------------------------------------------

void MainWindow::EnableGuiItems(const BOOL bEnable) const
{
	for(uint8_t ui8i = 2; ui8i < 20; ui8i++)
	{
		::EnableWindow((reinterpret_cast<MainWindowPageStats *>(m_MainWindowPages[0]))->m_hWndPageItems[ui8i], bEnable);
	}
	
	if(bEnable == FALSE || ::SendMessage((reinterpret_cast<MainWindowPageUsersChat *>(m_MainWindowPages[1]))->m_hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		::EnableWindow((reinterpret_cast<MainWindowPageUsersChat *>(m_MainWindowPages[1]))->m_hWndPageItems[MainWindowPageUsersChat::BTN_UPDATE_USERS], FALSE);
	}
	else
	{
		::EnableWindow((reinterpret_cast<MainWindowPageUsersChat *>(m_MainWindowPages[1]))->m_hWndPageItems[MainWindowPageUsersChat::BTN_UPDATE_USERS], TRUE);
	}
	
	if(bEnable == FALSE)
	{
		::SendMessage((reinterpret_cast<MainWindowPageUsersChat *>(m_MainWindowPages[1]))->m_hWndPageItems[MainWindowPageUsersChat::LV_USERS], LVM_DELETEALLITEMS, 0, 0);
		(reinterpret_cast<MainWindowPageScripts *>(m_MainWindowPages[2]))->ClearMemUsageAll();
	}
	
	::EnableWindow((reinterpret_cast<MainWindowPageScripts *>(m_MainWindowPages[2]))->m_hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS], bEnable);
}
//---------------------------------------------------------------------------

void MainWindow::OnSelChanged()
{
	int iPage = (int)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETCURSEL, 0, 0);
	
	if(iPage == -1)
	{
		return;
	}
	
	TCITEM tcItem = { 0 };
	tcItem.mask = TCIF_PARAM;
	
	if((BOOL)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETITEM, iPage, (LPARAM)&tcItem) == FALSE)
	{
		return;
	}
	
	if(tcItem.lParam == 0)
	{
		::MessageBox(m_hWnd, "Not implemented!", g_sPtokaXTitle, MB_OK);
	}
	
	::BringWindowToTop((reinterpret_cast<MainWindowPage *>(tcItem.lParam))->m_hWnd);
}
//---------------------------------------------------------------------------

void MainWindow::SaveGuiSettings()
{
	GuiSettingManager::m_Ptr->Save();
};
//---------------------------------------------------------------------------
