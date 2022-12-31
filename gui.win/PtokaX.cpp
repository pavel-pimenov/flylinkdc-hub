/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int iCmdShow)
{
	::SetDllDirectory("");
	
#ifndef _WIN64
	HINSTANCE hKernel32 = ::LoadLibrary("Kernel32.dll");
	
	typedef BOOL (WINAPI * SPDEPP)(DWORD);
	SPDEPP pSPDEPP = (SPDEPP)::GetProcAddress(hKernel32, "SetProcessDEPPolicy");
	
	if(pSPDEPP != nullptr)
	{
		pSPDEPP(PROCESS_DEP_ENABLE);
	}
	
	::FreeLibrary(hKernel32);
#endif
	
	ServerManager::m_hInstance = hInstance;
	
#ifdef _DEBUG
//    AllocConsole();
//    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//    Cout("PtokaX Debug console\n");
#endif

	char sBuf[MAX_PATH+1];
	::GetModuleFileName(nullptr, sBuf, MAX_PATH);
	char * sPath = strrchr(sBuf, '\\');
	if(sPath != nullptr)
	{
		ServerManager::m_sPath = string(sBuf, sPath-sBuf);
	}
	else
	{
		ServerManager::m_sPath = sBuf;
	}
	
	size_t szCmdLen = strlen(lpCmdLine);
	if(szCmdLen != 0)
	{
		char *sParam = lpCmdLine;
		size_t szParamLen = 0, szLen = 0;
		
		for(size_t szi = 0; szi < szCmdLen; szi++)
		{
			if(szi == szCmdLen-1)
			{
				if(lpCmdLine[szi] != ' ')
				{
					szi++;
				}
			}
			else if(lpCmdLine[szi] != ' ')
			{
				continue;
			}
			
			szParamLen = (lpCmdLine+szi)-sParam;
			
			switch(szParamLen)
			{
			case 7:
				if(strnicmp(sParam, "/notray", 7) == 0)
				{
					ServerManager::m_bCmdNoTray = true;
				}
				break;
			case 10:
				if(strnicmp(sParam, "/autostart", 10) == 0)
				{
					ServerManager::m_bCmdAutoStart = true;
				}
				break;
			case 12:
				if(strnicmp(sParam, "/noautostart", 12) == 0)
				{
					ServerManager::m_bCmdNoAutoStart = true;
				}
				break;
			case 20:
				if(strnicmp(sParam, "/generatexmllanguage", 20) == 0)
				{
					LanguageManager::GenerateXmlExample();
					return 0;
				}
				break;
			default:
				if(strnicmp(sParam, "-c ", 3) == 0)
				{
					szLen = strlen(sParam+3);
					if(szLen == 0)
					{
						::MessageBox(nullptr, "Missing config directory!", "Error!", MB_OK);
						return 0;
					}
					
					if(szLen >= 1 && sParam[0] != '\\' && sParam[0] != '/')
					{
						if(szLen < 4 || (sParam[1] != ':' || (sParam[2] != '\\' && sParam[2] != '/')))
						{
							::MessageBox(nullptr, "Config directory must be absolute path!", "Error!", MB_OK);
							return 0;
						}
					}
					
					if(sParam[szLen - 1] == '/' || sParam[szLen - 1] == '\\')
					{
						ServerManager::m_sPath = string(sParam, szLen - 1);
					}
					else
					{
						ServerManager::m_sPath = string(sParam, szLen);
					}
					
					if(DirExist(ServerManager::m_sPath.c_str()) == false)
					{
						if(CreateDirectory(ServerManager::m_sPath.c_str(), nullptr) == 0)
						{
							::MessageBox(nullptr, "Config directory not exist and can't be created!", "Error!", MB_OK);
							return 0;
						}
					}
				}
				break;
			}
			
			sParam = lpCmdLine+szi+1;
		}
	}
	
	HINSTANCE hRichEdit = ::LoadLibrary(/*"msftedit.dll"*/"riched20.dll");
	
	ExceptionHandlingInitialize(ServerManager::m_sPath, sBuf);
	
	ServerManager::Initialize();
	
	// systray icon on/off? added by Ptaczek 16.5.2003
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
	{
		MainWindow::m_Ptr->UpdateSysTray();
	}
	
	// If autostart is checked (or commandline /autostart given), then start the server
	if((SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_START] == true || ServerManager::m_bCmdAutoStart == true) && ServerManager::m_bCmdNoAutoStart == false)
	{
		if(ServerManager::Start() == false)
		{
			MainWindow::m_Ptr->SetStatusValue((string(LanguageManager::m_Ptr->m_sTexts[LAN_READY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_READY])+".").c_str());
		}
	}
	
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_START_MINIMIZED] == true && SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TRAY_ICON] == true)
	{
		::ShowWindow(MainWindow::m_Ptr->m_hWnd, SW_SHOWMINIMIZED);
	}
	else
	{
		::ShowWindow(MainWindow::m_Ptr->m_hWnd, iCmdShow);
	}
	
	MSG msg = { 0 };
	BOOL bRet = -1;
	
	while((bRet = ::GetMessage(&msg, nullptr, 0, 0)) != 0)
	{
		if(bRet == -1)
		{
			// handle the error and possibly exit
		}
		else
		{
			if(msg.message == WM_TIMER)
			{
				if (msg.wParam == ServerManager::m_upSecTimer)
				{
					ServerManager::OnSecTimer();
#ifdef FLYLINKDC_REMOVE_REGISTER_THREAD
				}
				else if(msg.wParam == ServerManager::m_upRegTimer)
				{
					ServerManager::OnRegTimer();
#endif
				}
				else
				{
					//Must be script timer
					ScriptOnTimer(msg.wParam);
				}
			}
			
			if(ServerManager::m_hWndActiveDialog == nullptr)
			{
				if(::IsDialogMessage(MainWindow::m_Ptr->m_hWnd, &msg) != 0)
				{
					continue;
				}
			}
			else if(::IsDialogMessage(ServerManager::m_hWndActiveDialog, &msg) != 0)
			{
				continue;
			}
			
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	
	delete MainWindow::m_Ptr;
	
	ExceptionHandlingUnitialize();
	
	::FreeLibrary(hRichEdit);
	
	return (int)msg.wParam;
}
//---------------------------------------------------------------------------
