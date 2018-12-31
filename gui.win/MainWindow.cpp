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
#include "MainWindow.h"
//---------------------------------------------------------------------------
#include "../core/hashBanManager.h"
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/LuaScriptManager.h"
#include "../core/ProfileManager.h"
#include "../core/ServerManager.h"
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
clsMainWindow * clsMainWindow::mPtr = nullptr;
//---------------------------------------------------------------------------
static const char sPtokaXDash[] = "PtokaX - ";
static const size_t szPtokaXDashLen = sizeof(sPtokaXDash) - 1;
//---------------------------------------------------------------------------
#ifndef _WIN64
static uint64_t (*GetActualTick)();

uint64_t PXGetTickCount()
{
	return (uint64_t)(::GetTickCount() / 1000);
}

typedef ULONGLONG(WINAPI *GTC64)(void);
GTC64 pGTC64 = nullptr;

uint64_t PXGetTickCount64()
{
	return (pGTC64() / 1000);
}
#endif
//---------------------------------------------------------------------------

clsMainWindow::clsMainWindow() : m_hWnd(NULL), ui64LastTrayMouseMove(0), uiTaskBarCreated(0)
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
	memset(&MainWindowPages, 0, sizeof(MainWindowPages));
}
//---------------------------------------------------------------------------

clsMainWindow::~clsMainWindow()
{
	safe_delete(clsGuiSettingManager::mPtr);
	
	for (uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++)
	{
		safe_delete(MainWindowPages[ui8i]);
	}
	
	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = 0;
	::Shell_NotifyIcon(NIM_DELETE, &nid);
	::DeleteObject(clsGuiSettingManager::hFont);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsMainWindow::StaticMainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_NCCREATE)
	{
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)clsMainWindow::mPtr);
		clsMainWindow::mPtr->m_hWnd = hWnd;
	}
	else
	{
		if (::GetWindowLongPtr(hWnd, GWLP_USERDATA) == NULL)
		{
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	
	return clsMainWindow::mPtr->MainWindowProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsMainWindow::MainWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		{
			HWND hCombo = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, NULL,
			                               clsServerManager::hInstance, NULL);
			                               
			if (hCombo != NULL)
			{
				::SendMessage(hCombo, WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
				
				clsGuiSettingManager::iGroupBoxMargin = (int)::SendMessage(hCombo, CB_GETITEMHEIGHT, (WPARAM) - 1, 0);
				clsGuiSettingManager::iEditHeight = clsGuiSettingManager::iGroupBoxMargin + (::GetSystemMetrics(SM_CXFIXEDFRAME) * 2);
				clsGuiSettingManager::iTextHeight = clsGuiSettingManager::iGroupBoxMargin - 2;
				clsGuiSettingManager::iCheckHeight = max(clsGuiSettingManager::iTextHeight - 2, ::GetSystemMetrics(SM_CYSMICON));
				
				::DestroyWindow(hCombo);
			}
			
			HWND hUpDown = ::CreateWindowEx(0, UPDOWN_CLASS, "", WS_CHILD | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT,
			                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, clsGuiSettingManager::iEditHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
			                                
			if (hUpDown != NULL)
			{
				RECT rcUpDown;
				::GetWindowRect(hUpDown, &rcUpDown);
				
				clsGuiSettingManager::iUpDownWidth = rcUpDown.right - rcUpDown.left;
				
				::DestroyWindow(hUpDown);
			}
			
			clsGuiSettingManager::iOneLineGB = clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + 8;
			clsGuiSettingManager::iOneLineOneChecksGB = clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + clsGuiSettingManager::iEditHeight + 12;
			clsGuiSettingManager::iOneLineTwoChecksGB = clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + clsGuiSettingManager::iEditHeight + 15;
			
			SettingPage::iOneCheckGB = clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iCheckHeight + 8;
			SettingPage::iTwoChecksGB = clsGuiSettingManager::iGroupBoxMargin + (2 * clsGuiSettingManager::iCheckHeight) + 3 + 8;
			SettingPage::iOneLineTwoGroupGB = clsGuiSettingManager::iGroupBoxMargin + clsGuiSettingManager::iEditHeight + (2 * clsGuiSettingManager::iOneLineGB) + 7;
			SettingPage::iTwoLineGB = clsGuiSettingManager::iGroupBoxMargin  + (2 * clsGuiSettingManager::iEditHeight) + 13;
			SettingPage::iThreeLineGB = clsGuiSettingManager::iGroupBoxMargin + (3 * clsGuiSettingManager::iEditHeight) + 18;
		}
		
		RECT rcMain;
		::GetClientRect(m_hWnd, &rcMain);
		
		DWORD dwTabsStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP | TCS_TABS | TCS_FOCUSNEVER;
		
		BOOL bHotTrackEnabled = FALSE;
		::SystemParametersInfo(SPI_GETHOTTRACKING, 0, &bHotTrackEnabled, 0);
		
		if (bHotTrackEnabled != FALSE)
		{
			dwTabsStyle |= TCS_HOTTRACK;
		}
		
		m_hWndWindowItems[TC_TABS] = ::CreateWindowEx(0, WC_TABCONTROL, "", dwTabsStyle, 0, 0, rcMain.right, clsGuiSettingManager::iEditHeight, m_hWnd, NULL, clsServerManager::hInstance, NULL);
		::SendMessage(m_hWndWindowItems[TC_TABS], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
		
		TCITEM tcItem = { 0 };
		tcItem.mask = TCIF_TEXT | TCIF_PARAM;
		
		for (uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++)
		{
			if (MainWindowPages[ui8i] != NULL)
			{
				if (MainWindowPages[ui8i]->CreateMainWindowPage(m_hWnd) == false)
				{
					::MessageBox(m_hWnd, "Main window creation failed!", g_sPtokaXTitle, MB_OK);
				}
				
				tcItem.pszText = MainWindowPages[ui8i]->GetPageName();
				tcItem.lParam = (LPARAM)MainWindowPages[ui8i];
				::SendMessage(m_hWndWindowItems[TC_TABS], TCM_INSERTITEM, ui8i, (LPARAM)&tcItem);
			}
			
			if (ui8i == 0 && clsGuiSettingManager::mPtr->i32Integers[GUISETINT_MAIN_WINDOW_HEIGHT] == clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_MAIN_WINDOW_HEIGHT))
			{
				RECT rcPage = { 0 };
				::GetWindowRect(MainWindowPages[0]->m_hWnd, &rcPage);
				
				int iDiff = (rcMain.bottom) - (rcPage.bottom - rcPage.top) - (clsGuiSettingManager::iEditHeight + 1);
				
				::GetWindowRect(m_hWnd, &rcMain);
				
				if (iDiff != 0)
				{
					::SetWindowPos(m_hWnd, NULL, 0, 0, (rcMain.right - rcMain.left), (rcMain.bottom - rcMain.top) - iDiff, SWP_NOMOVE | SWP_NOZORDER);
				}
			}
		}
		
		clsGuiSettingManager::wpOldTabsProc = (WNDPROC)::SetWindowLongPtr(m_hWndWindowItems[TC_TABS], GWLP_WNDPROC, (LONG_PTR)TabsProc);
		
#ifdef FLYLINKDC_USE_UPDATE_CHECKER_THREAD
		
		if (clsSettingManager::mPtr->bBools[SETBOOL_CHECK_NEW_RELEASES] == true)
		{
			// Create update check thread
			clsUpdateCheckThread::mPtr = new (std::nothrow) clsUpdateCheckThread();
			if (clsUpdateCheckThread::mPtr == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot allocate clsUpdateCheckThread::mPtr in MainWindow::MainWindowProc::WM_CREATE\n");
				exit(EXIT_FAILURE);
			}
			
			// Start the update check thread
			clsUpdateCheckThread::mPtr->Resume();
		}
#endif
		return 0;
	}
	case WM_CLOSE:
		if (::MessageBox(m_hWnd, (string(clsLanguageManager::mPtr->sTexts[LAN_ARE_YOU_SURE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_ARE_YOU_SURE]) + " ?").c_str(), g_sPtokaXTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
		{
			clsServerManager::bIsClose = true;
			
			RECT rcMain;
			::GetWindowRect(m_hWnd, &rcMain);
			
			clsGuiSettingManager::mPtr->SetInteger(GUISETINT_MAIN_WINDOW_WIDTH, rcMain.right - rcMain.left);
			clsGuiSettingManager::mPtr->SetInteger(GUISETINT_MAIN_WINDOW_HEIGHT, rcMain.bottom - rcMain.top);
			
			// stop server if running and save settings
			if (clsServerManager::bServerRunning == true)
			{
				clsServerManager::Stop();
				return 0;
			}
			else
			{
				clsServerManager::FinalClose();
			}
			
			break;
		}
		
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(1);
		break;
	case WM_TRAYICON:
		if (lParam == WM_MOUSEMOVE)
		{
#ifndef _WIN64
			if (ui64LastTrayMouseMove != GetActualTick())
			{
				ui64LastTrayMouseMove = GetActualTick();
#else
			if (ui64LastTrayMouseMove != GetTickCount64())
			{
				ui64LastTrayMouseMove = GetTickCount64();
#endif
				NOTIFYICONDATA nid;
				memset(&nid, 0, sizeof(NOTIFYICONDATA));
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
				nid.hIcon = (HICON)::LoadImage(clsServerManager::hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
				nid.uCallbackMessage = WM_TRAYICON;
				
				char msg[256];
				int iret = 0;
				if (clsServerManager::bServerRunning == false)
				{
					msg[0] = '\0';
				}
				else
				{
					iret = sprintf(msg, " (%s: %u)", clsLanguageManager::mPtr->sTexts[LAN_USERS], clsServerManager::ui32Logged);
					if (CheckSprintf(iret, 256, "MainWindow::MainWindowProc") == false)
					{
						return 0;
					}
				}
				
				const size_t szSize = sizeof(nid.szTip);
				
				if (szSize < (size_t)(clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME] + iret + 10))
				{
					nid.szTip[szSize - 1] = '\0';
					memcpy(nid.szTip + (szSize - (iret + 1)), msg, iret);
					nid.szTip[szSize - (iret + 2)] = '.';
					nid.szTip[szSize - (iret + 3)] = '.';
					nid.szTip[szSize - (iret + 4)] = '.';
					memcpy(nid.szTip + szPtokaXDashLen, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], szSize - (iret + 13));
					memcpy(nid.szTip, sPtokaXDash, szPtokaXDashLen);
				}
				else
				{
					memcpy(nid.szTip, sPtokaXDash, szPtokaXDashLen);
					memcpy(nid.szTip + szPtokaXDashLen, clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]);
					memcpy(nid.szTip + szPtokaXDashLen + clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME], msg, iret);
					nid.szTip[szPtokaXDashLen + clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME] + iret] = '\0';
				}
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
		else if (lParam == WM_LBUTTONUP)
		{
			::ShowWindow(m_hWnd, SW_SHOW);
			::ShowWindow(m_hWnd, SW_RESTORE);
		}
		
		return 0;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED && clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
		{
			::ShowWindow(m_hWnd, SW_HIDE);
		}
		else
		{
			RECT rc;
			::SetRect(&rc, 0, 0, GET_X_LPARAM(lParam), clsGuiSettingManager::iEditHeight);
			::SendMessage(m_hWndWindowItems[TC_TABS], TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);
			
			HDWP hdwp = ::BeginDeferWindowPos(3);
			
			::DeferWindowPos(hdwp, m_hWndWindowItems[TC_TABS], NULL, 0, 0, GET_X_LPARAM(lParam), clsGuiSettingManager::iEditHeight, SWP_NOMOVE | SWP_NOZORDER);
			
			for (uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++)
			{
				if (MainWindowPages[ui8i] != NULL)
				{
					::DeferWindowPos(hdwp, MainWindowPages[ui8i]->m_hWnd, NULL, 0, clsGuiSettingManager::iEditHeight + 1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) - (clsGuiSettingManager::iEditHeight + 1),
					                 SWP_NOMOVE | SWP_NOZORDER);
				}
			}
			
			::EndDeferWindowPos(hdwp);
		}
		
		return 0;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_MAIN_WINDOW_WIDTH));
		mminfo->ptMinTrackSize.y = ScaleGui(clsGuiSettingManager::mPtr->GetDefaultInteger(GUISETINT_MAIN_WINDOW_HEIGHT));
		
		return 0;
	}
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[TC_TABS])
		{
			if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
			{
				OnSelChanged();
				return 0;
			}
			else if (((LPNMHDR)lParam)->code == TCN_KEYDOWN)
			{
				NMTCKEYDOWN * ptckd = (NMTCKEYDOWN *)lParam;
				if (ptckd->wVKey == VK_TAB)
				{
					int iPage = (int)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETCURSEL, 0, 0);
					
					if (iPage == -1)
					{
						break;
					}
					
					TCITEM tcItem = { 0 };
					tcItem.mask = TCIF_PARAM;
					
					if ((BOOL)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETITEM, iPage, (LPARAM)&tcItem) == FALSE)
					{
						break;
					}
					
					if (tcItem.lParam == NULL)
					{
						::MessageBox(m_hWnd, "Not implemented!", g_sPtokaXTitle, MB_OK);
					}
					
					MainWindowPage * curMainWindowPage = reinterpret_cast<MainWindowPage *>(tcItem.lParam);
					
					if ((::GetKeyState(VK_SHIFT) & 0x8000) > 0)
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
		switch (LOWORD(wParam))
		{
		case IDC_EXIT:
			::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
			return 0;
		case IDC_SETTINGS:
		{
			SettingDialog::m_Ptr = new (std::nothrow) SettingDialog();
			
			if (SettingDialog::m_Ptr != nullptr)
			{
				SettingDialog::m_Ptr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_REG_USERS:
		{
			clsRegisteredUsersDialog::mPtr = new (std::nothrow) clsRegisteredUsersDialog();
			
			if (clsRegisteredUsersDialog::mPtr != NULL)
			{
				clsRegisteredUsersDialog::mPtr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_PROFILES:
		{
			clsProfilesDialog::mPtr = new (std::nothrow) clsProfilesDialog();
			
			if (clsProfilesDialog::mPtr != NULL)
			{
				clsProfilesDialog::mPtr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_BANS:
		{
			clsBansDialog::mPtr = new (std::nothrow) clsBansDialog();
			
			if (clsBansDialog::mPtr != NULL)
			{
				clsBansDialog::mPtr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_RANGE_BANS:
		{
			clsRangeBansDialog::mPtr = new (std::nothrow) clsRangeBansDialog();
			
			if (clsRangeBansDialog::mPtr != NULL)
			{
				clsRangeBansDialog::mPtr->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_ABOUT:
		{
			AboutDialog * pAboutDlg = new (std::nothrow) AboutDialog();
			
			if (pAboutDlg != NULL)
			{
				pAboutDlg->DoModal(m_hWnd);
			}
			
			return 0;
		}
		case IDC_HOMEPAGE:
			::ShellExecute(NULL, NULL, "https://github.com/pavel-pimenov/PtokaX", NULL, NULL, SW_SHOWNORMAL);
			return 0;
		case IDC_FORUM:
			::ShellExecute(NULL, NULL, "http://forum.PtokaX.org/", NULL, NULL, SW_SHOWNORMAL);
			return 0;
		case IDC_WIKI:
			::ShellExecute(NULL, NULL, "http://wiki.PtokaX.org", NULL, NULL, SW_SHOWNORMAL);
			return 0;
		case IDC_UPDATE_CHECK:
		{
#ifdef FLYLINKDC_USE_UPDATE_CHECKER_THREAD
			clsUpdateDialog::mPtr = new (std::nothrow) clsUpdateDialog();
			
			if (clsUpdateDialog::mPtr != NULL)
			{
				clsUpdateDialog::mPtr->DoModal(m_hWnd);
				
				// First destroy old update check thread if any
				if (clsUpdateCheckThread::mPtr != NULL)
				{
					clsUpdateCheckThread::mPtr->Close();
					clsUpdateCheckThread::mPtr->WaitFor();
				}
				
				safe_delete(clsUpdateCheckThread::mPtr);
				
				// Create update check thread
				clsUpdateCheckThread::mPtr = new (std::nothrow) clsUpdateCheckThread();
				if (clsUpdateCheckThread::mPtr == NULL)
				{
					AppendDebugLog("%s - [MEM] Cannot allocate clsUpdateCheckThread::mPtr in MainWindow::MainWindowProc::IDC_UPDATE_CHECK\n");
					exit(EXIT_FAILURE);
				}
				
				// Start the update check thread
				clsUpdateCheckThread::mPtr->Resume();
			}
#endif
			return 0;
		}
		case IDC_SAVE_SETTINGS:
			clsBanManager::mPtr->Save(true);
			clsProfileManager::mPtr->SaveProfiles();
			clsRegManager::mPtr->Save();
			clsScriptManager::mPtr->SaveScripts();
			clsSettingManager::mPtr->Save();
			
			::MessageBox(m_hWnd, clsLanguageManager::mPtr->sTexts[LAN_SETTINGS_SAVED], g_sPtokaXTitle, MB_OK | MB_ICONINFORMATION);
			return 0;
		case IDC_RELOAD_TXTS:
			clsTextFilesManager::mPtr->RefreshTextFiles();
			
			::MessageBox(m_hWnd, (string(clsLanguageManager::mPtr->sTexts[LAN_TXT_FILES_RELOADED], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_TXT_FILES_RELOADED]) + ".").c_str(), g_sPtokaXTitle, MB_OK | MB_ICONINFORMATION);
			
			return 0;
		}
		
		break;
	case WM_QUERYENDSESSION:
		clsBanManager::mPtr->Save(true);
		clsProfileManager::mPtr->SaveProfiles();
		clsRegManager::mPtr->Save();
		clsScriptManager::mPtr->SaveScripts();
		clsSettingManager::mPtr->Save();
		
		AppendLog("Serving stopped (Windows shutdown).");
		
		return TRUE;
	case WM_UPDATE_CHECK_MSG:
	{
		char * sMsg = (char *)lParam;
		if (clsUpdateDialog::mPtr != NULL)
		{
			clsUpdateDialog::mPtr->Message(sMsg);
		}
		
		free(sMsg);
		
		return 0;
	}
	case WM_UPDATE_CHECK_DATA:
	{
		char * sMsg = (char *)lParam;
		
		if (clsUpdateDialog::mPtr == NULL)
		{
			clsUpdateDialog::mPtr = new (std::nothrow) clsUpdateDialog();
			
			if (clsUpdateDialog::mPtr != NULL)
			{
				if (clsUpdateDialog::mPtr->ParseData(sMsg, m_hWnd) == false)
				{
					safe_delete(clsUpdateDialog::mPtr);
				}
			}
		}
		else
		{
			clsUpdateDialog::mPtr->ParseData(sMsg, m_hWnd);
		}
		
		free(sMsg);
		
		return 0;
	}
	case WM_UPDATE_CHECK_TERMINATE:
#ifdef FLYLINKDC_USE_UPDATE_CHECKER_THREAD
		if (clsUpdateCheckThread::mPtr != NULL)
		{
			clsUpdateCheckThread::mPtr->Close();
			clsUpdateCheckThread::mPtr->WaitFor();
		}
		
		safe_delete(clsUpdateCheckThread::mPtr);
#endif
		return 0;
	case WM_SETFOCUS:
		::SetFocus(m_hWndWindowItems[TC_TABS]);
		return 0;
	}
	
	if (uMsg == uiTaskBarCreated)
	{
		UpdateSysTray();
		return 0;
	}
	
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

HWND clsMainWindow::CreateEx()
{
	clsGuiSettingManager::mPtr = new (std::nothrow) clsGuiSettingManager();
	
	if (clsGuiSettingManager::mPtr == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate clsGuiSettingManager::mPtr in clsMainWindow::clsMainWindow\n");
		exit(EXIT_FAILURE);
	}
	
	INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES |
	                              ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS
	                            };
	InitCommonControlsEx(&iccx);
	
	MainWindowPages[0] = new (std::nothrow) MainWindowPageStats();
	MainWindowPages[1] = new (std::nothrow) clsMainWindowPageUsersChat();
	MainWindowPages[2] = new (std::nothrow) clsMainWindowPageScripts();
	
	for (uint8_t ui8i = 0; ui8i < 3; ui8i++)
	{
		if (MainWindowPages[ui8i] == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot allocate MainWindowPage[%" PRIu8 "] in clsMainWindow::clsMainWindow\n", ui8i);
			exit(EXIT_FAILURE);
		}
	}
	
	uiTaskBarCreated = ::RegisterWindowMessage("TaskbarCreated");
	
	NONCLIENTMETRICS NCM = { 0 };
	NCM.cbSize = sizeof(NONCLIENTMETRICS);
	
	OSVERSIONINFOEX ver;
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	
#ifndef _WIN64
	if (::GetVersionEx((OSVERSIONINFO*)&ver) != 0 && ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion < 6)
	{
		NCM.cbSize -= sizeof(int);
	}
#endif
	
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NCM, 0);
	
	if (NCM.lfMessageFont.lfHeight > 0)
	{
		clsGuiSettingManager::fScaleFactor = (float)(NCM.lfMessageFont.lfHeight / 12.0);
	}
	else if (NCM.lfMessageFont.lfHeight < 0)
	{
		clsGuiSettingManager::fScaleFactor = float(NCM.lfMessageFont.lfHeight / -12.0);
	}
	
	clsGuiSettingManager::hFont = ::CreateFontIndirect(&NCM.lfMessageFont);
	
	clsGuiSettingManager::hArrowCursor = (HCURSOR)::LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	clsGuiSettingManager::hVerticalCursor = (HCURSOR)::LoadImage(NULL, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	
	HMENU hMainMenu = ::CreateMenu();
	
	HMENU hFileMenu = ::CreatePopupMenu();
	::AppendMenu(hFileMenu, MF_STRING, IDC_RELOAD_TXTS, (string(clsLanguageManager::mPtr->sTexts[LAN_RELOAD_TEXT_FILES], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RELOAD_TEXT_FILES]) + "...").c_str());
	::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hFileMenu, MF_STRING, IDC_SETTINGS, (string(clsLanguageManager::mPtr->sTexts[LAN_MENU_SETTINGS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_MENU_SETTINGS]) + "...").c_str());
	::AppendMenu(hFileMenu, MF_STRING, IDC_SAVE_SETTINGS, (string(clsLanguageManager::mPtr->sTexts[LAN_SAVE_SETTINGS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SAVE_SETTINGS]) + "...").c_str());
	::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hFileMenu, MF_STRING, IDC_EXIT, clsLanguageManager::mPtr->sTexts[LAN_EXIT]);
	
	::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hFileMenu, clsLanguageManager::mPtr->sTexts[LAN_FILE]);
	
	HMENU hViewMenu = ::CreatePopupMenu();
	::AppendMenu(hViewMenu, MF_STRING, IDC_REG_USERS, clsLanguageManager::mPtr->sTexts[LAN_REG_USERS]);
	::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hViewMenu, MF_STRING, IDC_PROFILES, clsLanguageManager::mPtr->sTexts[LAN_PROFILES]);
	::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hViewMenu, MF_STRING, IDC_BANS, clsLanguageManager::mPtr->sTexts[LAN_BANS]);
	::AppendMenu(hViewMenu, MF_STRING, IDC_RANGE_BANS, clsLanguageManager::mPtr->sTexts[LAN_RANGE_BANS]);
	
	::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hViewMenu, clsLanguageManager::mPtr->sTexts[LAN_VIEW]);
	
	HMENU hHelpMenu = ::CreatePopupMenu();
	::AppendMenu(hHelpMenu, MF_STRING, IDC_UPDATE_CHECK, (string(clsLanguageManager::mPtr->sTexts[LAN_CHECK_FOR_UPDATE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CHECK_FOR_UPDATE]) + "...").c_str());
	::AppendMenu(hHelpMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hHelpMenu, MF_STRING, IDC_HOMEPAGE, (string("PtokaX ") + clsLanguageManager::mPtr->sTexts[LAN_WEBSITE]).c_str());
	::AppendMenu(hHelpMenu, MF_STRING, IDC_FORUM, (string("PtokaX ") + clsLanguageManager::mPtr->sTexts[LAN_FORUM]).c_str());
	::AppendMenu(hHelpMenu, MF_STRING, IDC_WIKI, (string("PtokaX ") + clsLanguageManager::mPtr->sTexts[LAN_WIKI]).c_str());
	::AppendMenu(hHelpMenu, MF_SEPARATOR, 0, 0);
	::AppendMenu(hHelpMenu, MF_STRING, IDC_ABOUT, (string(clsLanguageManager::mPtr->sTexts[LAN_MENU_ABOUT], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_MENU_ABOUT]) + " PtokaX").c_str());
	
	::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hHelpMenu, clsLanguageManager::mPtr->sTexts[LAN_HELP]);
	
	WNDCLASSEX m_wc;
	memset(&m_wc, 0, sizeof(WNDCLASSEX));
	m_wc.cbSize = sizeof(WNDCLASSEX);
	m_wc.lpfnWndProc = StaticMainWindowProc;
	m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	m_wc.lpszClassName = g_sPtokaXTitle;
	m_wc.hInstance = clsServerManager::hInstance;
	m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
	m_wc.style = CS_HREDRAW | CS_VREDRAW;
	m_wc.hIcon = (HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(IDR_MAINICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR | LR_SHARED);
	m_wc.hIconSm = (HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
	
	ATOM atom = ::RegisterClassEx(&m_wc);
	
	m_hWnd = ::CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, MAKEINTATOM(atom), (string(clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], (size_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]) + " | " + g_sPtokaXTitle).c_str(),
	                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, ScaleGuiDefaultsOnly(GUISETINT_MAIN_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_MAIN_WINDOW_HEIGHT), NULL, hMainMenu, clsServerManager::hInstance, NULL);
	                          
#ifndef _WIN64
	if (::GetVersionEx((OSVERSIONINFO*)&ver) != 0 && ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion >= 6)
	{
		pGTC64 = (GTC64)::GetProcAddress(::GetModuleHandle("kernel32.dll"), "GetTickCount64");
		if (pGTC64 != NULL)
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

void clsMainWindow::UpdateSysTray() const
{
	if (clsServerManager::bCmdNoTray == false)
	{
		NOTIFYICONDATA nid;
		memset(&nid, 0, sizeof(NOTIFYICONDATA));
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_ICON | NIF_MESSAGE;
		nid.hIcon = (HICON)::LoadImage(clsServerManager::hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
		nid.uCallbackMessage = WM_TRAYICON;
		
		if (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
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

void clsMainWindow::UpdateStats() const
{
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_JOINS_VALUE], string(clsServerManager::ui32Joins).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_PARTS_VALUE], string(clsServerManager::ui32Parts).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_ACTIVE_VALUE], string(clsServerManager::ui32Joins - clsServerManager::ui32Parts).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_ONLINE_VALUE], string(clsServerManager::ui32Logged).c_str());
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_PEAK_VALUE], string(clsServerManager::ui32Peak).c_str());
	
	char msg[256];
	
	int imsglen = sprintf(msg, "%s (%s)", formatBytes(clsServerManager::ui64BytesRead), formatBytesPerSecond(clsServerManager::ui32ActualBytesRead));
	if (CheckSprintf(imsglen, 256, "clsMainWindow::UpdateStats") == true)
	{
		::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_RECEIVED_VALUE], msg);
	}
	
	imsglen = sprintf(msg, "%s (%s)", formatBytes(clsServerManager::ui64BytesSent), formatBytesPerSecond(clsServerManager::ui32ActualBytesSent));
	if (CheckSprintf(imsglen, 256, "clsMainWindow::UpdateStats1") == true)
	{
		::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_SENT_VALUE], msg);
	}
}
//---------------------------------------------------------------------------

void clsMainWindow::UpdateTitleBar()
{
	::SetWindowText(m_hWnd, (string(clsSettingManager::mPtr->sTexts[SETTXT_HUB_NAME], (size_t)clsSettingManager::mPtr->ui16TextsLens[SETTXT_HUB_NAME]) + " | " + g_sPtokaXTitle).c_str());
}
//---------------------------------------------------------------------------

void clsMainWindow::UpdateLanguage()
{
	for (uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++)
	{
		if (MainWindowPages[ui8i] != NULL)
		{
			MainWindowPages[ui8i]->UpdateLanguage();
			
			TCITEM tcItem = { 0 };
			tcItem.mask = TCIF_TEXT;
			
			tcItem.pszText = MainWindowPages[ui8i]->GetPageName();
			::SendMessage(m_hWndWindowItems[TC_TABS], TCM_SETITEM, ui8i, (LPARAM)&tcItem);
		}
	}
	
	HMENU hMenu = ::GetMenu(m_hWnd);
	
	::ModifyMenu(hMenu, 0, MF_BYPOSITION, 0, clsLanguageManager::mPtr->sTexts[LAN_FILE]);
	::ModifyMenu(hMenu, 1, MF_BYPOSITION, 0, clsLanguageManager::mPtr->sTexts[LAN_VIEW]);
	::ModifyMenu(hMenu, 2, MF_BYPOSITION, 0, clsLanguageManager::mPtr->sTexts[LAN_HELP]);
	
	::ModifyMenu(hMenu, IDC_EXIT, MF_BYCOMMAND, IDC_EXIT, clsLanguageManager::mPtr->sTexts[LAN_EXIT]);
	::ModifyMenu(hMenu, IDC_REG_USERS, MF_BYCOMMAND, IDC_REG_USERS, clsLanguageManager::mPtr->sTexts[LAN_REG_USERS]);
	::ModifyMenu(hMenu, IDC_PROFILES, MF_BYCOMMAND, IDC_PROFILES, clsLanguageManager::mPtr->sTexts[LAN_PROFILES]);
	::ModifyMenu(hMenu, IDC_BANS, MF_BYCOMMAND, IDC_BANS, clsLanguageManager::mPtr->sTexts[LAN_BANS]);
	::ModifyMenu(hMenu, IDC_RANGE_BANS, MF_BYCOMMAND, IDC_RANGE_BANS, clsLanguageManager::mPtr->sTexts[LAN_RANGE_BANS]);
	
	::ModifyMenu(hMenu, IDC_HOMEPAGE, MF_BYCOMMAND, IDC_HOMEPAGE, (string("PtokaX ") + clsLanguageManager::mPtr->sTexts[LAN_WEBSITE]).c_str());
	::ModifyMenu(hMenu, IDC_FORUM, MF_BYCOMMAND, IDC_FORUM, (string("PtokaX ") + clsLanguageManager::mPtr->sTexts[LAN_FORUM]).c_str());
	::ModifyMenu(hMenu, IDC_WIKI, MF_BYCOMMAND, IDC_WIKI, (string("PtokaX ") + clsLanguageManager::mPtr->sTexts[LAN_WIKI]).c_str());
	
	::ModifyMenu(hMenu, IDC_ABOUT, MF_BYCOMMAND, IDC_ABOUT,
	             (string(clsLanguageManager::mPtr->sTexts[LAN_MENU_ABOUT], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_MENU_ABOUT]) + " PtokaX").c_str());
	             
	::ModifyMenu(hMenu, IDC_UPDATE_CHECK, MF_BYCOMMAND, IDC_UPDATE_CHECK,
	             (string(clsLanguageManager::mPtr->sTexts[LAN_CHECK_FOR_UPDATE], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_CHECK_FOR_UPDATE]) + "...").c_str());
	::ModifyMenu(hMenu, IDC_RELOAD_TXTS, MF_BYCOMMAND, IDC_RELOAD_TXTS,
	             (string(clsLanguageManager::mPtr->sTexts[LAN_RELOAD_TEXT_FILES], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_RELOAD_TEXT_FILES]) + "...").c_str());
	::ModifyMenu(hMenu, IDC_SETTINGS, MF_BYCOMMAND, IDC_SETTINGS,
	             (string(clsLanguageManager::mPtr->sTexts[LAN_MENU_SETTINGS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_MENU_SETTINGS]) + "...").c_str());
	::ModifyMenu(hMenu, IDC_SAVE_SETTINGS, MF_BYCOMMAND, IDC_SAVE_SETTINGS,
	             (string(clsLanguageManager::mPtr->sTexts[LAN_SAVE_SETTINGS], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_SAVE_SETTINGS]) + "...").c_str());
}
//---------------------------------------------------------------------------

void clsMainWindow::EnableStartButton(const BOOL &bEnable) const
{
	::EnableWindow((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::BTN_START_STOP], bEnable);
}
//---------------------------------------------------------------------------

void clsMainWindow::SetStartButtonText(const char * sText) const
{
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::BTN_START_STOP], sText);
}
//---------------------------------------------------------------------------

void clsMainWindow::SetStatusValue(const char * sText) const
{
	::SetWindowText((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[MainWindowPageStats::LBL_STATUS_VALUE], sText);
}
//---------------------------------------------------------------------------

void clsMainWindow::EnableGuiItems(const BOOL &bEnable) const
{
	for (uint8_t ui8i = 2; ui8i < 20; ui8i++)
	{
		::EnableWindow((reinterpret_cast<MainWindowPageStats *>(MainWindowPages[0]))->hWndPageItems[ui8i], bEnable);
	}
	
	if (bEnable == FALSE || ::SendMessage((reinterpret_cast<clsMainWindowPageUsersChat *>(MainWindowPages[1]))->hWndPageItems[clsMainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		::EnableWindow((reinterpret_cast<clsMainWindowPageUsersChat *>(MainWindowPages[1]))->hWndPageItems[clsMainWindowPageUsersChat::BTN_UPDATE_USERS], FALSE);
	}
	else
	{
		::EnableWindow((reinterpret_cast<clsMainWindowPageUsersChat *>(MainWindowPages[1]))->hWndPageItems[clsMainWindowPageUsersChat::BTN_UPDATE_USERS], TRUE);
	}
	
	if (bEnable == FALSE)
	{
		::SendMessage((reinterpret_cast<clsMainWindowPageUsersChat *>(MainWindowPages[1]))->hWndPageItems[clsMainWindowPageUsersChat::LV_USERS], LVM_DELETEALLITEMS, 0, 0);
		(reinterpret_cast<clsMainWindowPageScripts *>(MainWindowPages[2]))->ClearMemUsageAll();
	}
	
	::EnableWindow((reinterpret_cast<clsMainWindowPageScripts *>(MainWindowPages[2]))->hWndPageItems[clsMainWindowPageScripts::BTN_RESTART_SCRIPTS], bEnable);
}
//---------------------------------------------------------------------------

void clsMainWindow::OnSelChanged()
{
	int iPage = (int)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETCURSEL, 0, 0);
	
	if (iPage == -1)
	{
		return;
	}
	
	TCITEM tcItem = { 0 };
	tcItem.mask = TCIF_PARAM;
	
	if ((BOOL)::SendMessage(m_hWndWindowItems[TC_TABS], TCM_GETITEM, iPage, (LPARAM)&tcItem) == FALSE)
	{
		return;
	}
	
	if (tcItem.lParam == NULL)
	{
		::MessageBox(m_hWnd, "Not implemented!", g_sPtokaXTitle, MB_OK);
	}
	
	::BringWindowToTop((reinterpret_cast<MainWindowPage *>(tcItem.lParam))->m_hWnd);
}
//---------------------------------------------------------------------------

void clsMainWindow::SaveGuiSettings()
{
	clsGuiSettingManager::mPtr->Save();
};
//---------------------------------------------------------------------------
