/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
#include "../core/LuaInc.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/serviceLoop.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "../core/ExceptionHandling.h"
#include "../core/LuaScript.h"
#include "MainWindow.h"
//---------------------------------------------------------------------------
#ifndef _DEBUG
#include "DbgHelp.h"
#include "../doctor-dump/CrashRpt.h"
static void crash_test_doctor_dump()
{
#ifndef _DEBUG
	*((int*)0) = 0;
#endif
}

template<typename T> static T getFilePath(const T& path)
{
	const auto i = path.rfind('\\');
	return (i != string_t::npos) ? path.substr(0, i + 1) : path;
}

crash_rpt::ApplicationInfo* GetApplicationInfo()
{
	static crash_rpt::ApplicationInfo appInfo;
	appInfo.ApplicationInfoSize = sizeof(appInfo);
	appInfo.ApplicationGUID = "70D71CA7-8FDF-41C4-9AA1-4ADB30527A54";
	
	
#ifdef _WIN64
	appInfo.Prefix = "ptokax-gui-x64";             // Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
	appInfo.AppName = L"PtokaX++ GUI x64";         // Application name that will be used in message box.
#else
	appInfo.Prefix = "ptokax-gui-x86";             // Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
	appInfo.AppName = L"PtokaX++ GUI x86";             // Application name that will be used in message box.
#endif
	appInfo.Company = L"PtokaX++ GUI developers";  // Company name that will be used in message box.
	appInfo.V[0] = 0;
	appInfo.V[1] = 5;
	appInfo.V[2] = 1;
	appInfo.V[3] = USHORT(atoi(BUILD_NUMBER));
	return &appInfo;
}

crash_rpt::HandlerSettings* GetHandlerSettings()
{
	static wchar_t g_path_sender[MAX_PATH] = {0};
	static wchar_t g_path_dbhelp[MAX_PATH] = {0};
	::GetModuleFileNameW(NULL, g_path_sender, MAX_PATH);
	wcscpy(g_path_dbhelp, g_path_sender);
	auto l_tslash = wcsrchr(g_path_sender, '\\');
	if (l_tslash)
	{
		l_tslash++;
#ifdef _WIN64
		wcscpy(l_tslash, L"sendrpt-x64.exe");
#else
		wcscpy(l_tslash, L"sendrpt-x86.exe");
#endif
		l_tslash = wcsrchr(g_path_dbhelp, '\\');
		l_tslash++;
#ifdef _WIN64
		wcscpy(l_tslash, L"dbghelp-x64.dll");
#else
		wcscpy(l_tslash, L"dbghelp-x86.dll");
#endif
	}
	static crash_rpt::HandlerSettings g_handlerSettings;
	g_handlerSettings.HandlerSettingsSize = sizeof(g_handlerSettings);
	g_handlerSettings.OpenProblemInBrowser = TRUE;
	g_handlerSettings.SendRptPath = g_path_sender;
	g_handlerSettings.DbgHelpPath = g_path_dbhelp;
	return &g_handlerSettings;
}

crash_rpt::CrashRpt g_crashRpt(
#ifdef _WIN64
    L"crashrpt-x64.dll",
#else
    L"crashrpt-x86.dll",
#endif
    GetApplicationInfo(),
    GetHandlerSettings());

#endif


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int nCmdShow)
{
	::SetDllDirectory("");
	
#ifndef _WIN64
	HINSTANCE hKernel32 = ::LoadLibrary("Kernel32.dll");
	
	typedef BOOL (WINAPI * SPDEPP)(DWORD);
	SPDEPP pSPDEPP = (SPDEPP)::GetProcAddress(hKernel32, "SetProcessDEPPolicy");
	
	if (pSPDEPP != NULL)
	{
		pSPDEPP(PROCESS_DEP_ENABLE);
	}
	
	::FreeLibrary(hKernel32);
#endif
	
	clsServerManager::hInstance = hInstance;
	
#ifdef _DEBUG
//    AllocConsole();
//    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//    Cout("PtokaX Debug console\n");
#endif

	char sBuf[MAX_PATH + 1];
	::GetModuleFileName(NULL, sBuf, MAX_PATH);
	char * sPath = strrchr(sBuf, '\\');
	if (sPath != NULL)
	{
		clsServerManager::sPath = string(sBuf, sPath - sBuf);
	}
	else
	{
		clsServerManager::sPath = sBuf;
	}
	
	size_t szCmdLen = strlen(lpCmdLine);
	if (szCmdLen != 0)
	{
		char *sParam = lpCmdLine;
		size_t szParamLen = 0, szLen = 0;
		
		for (size_t szi = 0; szi < szCmdLen; szi++)
		{
			if (szi == szCmdLen - 1)
			{
				if (lpCmdLine[szi] != ' ')
				{
					szi++;
				}
			}
			else if (lpCmdLine[szi] != ' ')
			{
				continue;
			}
			
			szParamLen = (lpCmdLine + szi) - sParam;
			
			if (strnicmp(sParam, "/crash-test-doctor-dump", 23) == NULL)
			{
				crash_test_doctor_dump();
			}
			
			switch (szParamLen)
			{
			case 7:
				if (strnicmp(sParam, "/notray", 7) == NULL)
				{
					clsServerManager::bCmdNoTray = true;
				}
				break;
			case 10:
				if (strnicmp(sParam, "/autostart", 10) == NULL)
				{
					clsServerManager::bCmdAutoStart = true;
				}
				break;
			case 12:
				if (strnicmp(sParam, "/noautostart", 12) == NULL)
				{
					clsServerManager::bCmdNoAutoStart = true;
				}
				break;
			case 20:
				if (strnicmp(sParam, "/generatexmllanguage", 20) == NULL)
				{
					clsLanguageManager::GenerateXmlExample();
					return 0;
				}
				break;
			default:
				if (strnicmp(sParam, "-c ", 3) == NULL)
				{
					szLen = strlen(sParam + 3);
					if (szLen == 0)
					{
						::MessageBox(NULL, "Missing config directory!", "Error!", MB_OK);
						return 0;
					}
					
					if (szLen >= 1 && sParam[0] != '\\' && sParam[0] != '/')
					{
						if (szLen < 4 || (sParam[1] != ':' || (sParam[2] != '\\' && sParam[2] != '/')))
						{
							::MessageBox(NULL, "Config directory must be absolute path!", "Error!", MB_OK);
							return 0;
						}
					}
					
					if (sParam[szLen - 1] == '/' || sParam[szLen - 1] == '\\')
					{
						clsServerManager::sPath = string(sParam, szLen - 1);
					}
					else
					{
						clsServerManager::sPath = string(sParam, szLen);
					}
					
					if (DirExist(clsServerManager::sPath.c_str()) == false)
					{
						if (CreateDirectory(clsServerManager::sPath.c_str(), NULL) == 0)
						{
							::MessageBox(NULL, "Config directory not exist and can't be created!", "Error!", MB_OK);
							return 0;
						}
					}
				}
				break;
			}
			
			sParam = lpCmdLine + szi + 1;
		}
	}
	
	HINSTANCE hRichEdit = ::LoadLibrary(/*"msftedit.dll"*/"riched20.dll");
	
	ExceptionHandlingInitialize(clsServerManager::sPath, sBuf);
	
	clsServerManager::Initialize();
	
	// systray icon on/off? added by Ptaczek 16.5.2003
	if (clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
	{
		clsMainWindow::mPtr->UpdateSysTray();
	}
	
	// If autostart is checked (or commandline /autostart given), then start the server
	if ((clsSettingManager::mPtr->bBools[SETBOOL_AUTO_START] == true || clsServerManager::bCmdAutoStart == true) && clsServerManager::bCmdNoAutoStart == false)
	{
		if (clsServerManager::Start() == false)
		{
			clsMainWindow::mPtr->SetStatusValue((string(clsLanguageManager::mPtr->sTexts[LAN_READY], (size_t)clsLanguageManager::mPtr->ui16TextsLens[LAN_READY]) + ".").c_str());
		}
	}
	
	if (clsSettingManager::mPtr->bBools[SETBOOL_START_MINIMIZED] == true && clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
	{
		::ShowWindow(clsMainWindow::mPtr->m_hWnd, SW_SHOWMINIMIZED);
	}
	else
	{
		::ShowWindow(clsMainWindow::mPtr->m_hWnd, nCmdShow);
	}
	
	MSG msg = { 0 };
	BOOL bRet = -1;
	
	while ((bRet = ::GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else
		{
			if (msg.message == WM_USER + 1)
			{
				break;
			}
			else if (msg.message == WM_TIMER)
			{
				if (msg.wParam == clsServerManager::sectimer)
				{
					clsServerManager::OnSecTimer();
				}
				else if (msg.wParam == clsServiceLoop::srvLoopTimer)
				{
					clsServiceLoop::mPtr->Looper();
				}
#ifdef FLYLINKDC_REMOVE_REGISTER_THREAD
				else if (msg.wParam == clsServerManager::regtimer)
				{
					clsServerManager::OnRegTimer();
				}
#endif
				else
				{
					//Must be script timer
					ScriptOnTimer(msg.wParam);
				}
			}
			
			if (clsServerManager::hWndActiveDialog == NULL)
			{
				if (::IsDialogMessage(clsMainWindow::mPtr->m_hWnd, &msg) != 0)
				{
					continue;
				}
			}
			else if (::IsDialogMessage(clsServerManager::hWndActiveDialog, &msg) != 0)
			{
				continue;
			}
			
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	
	safe_delete(clsMainWindow::mPtr);
	
	ExceptionHandlingUnitialize();
	
	::FreeLibrary(hRichEdit);
	
	return (int)msg.wParam;
}
//---------------------------------------------------------------------------
