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
#include "../core/LuaInc.h"
//---------------------------------------------------------------------------
#include "ScriptEditorDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/UdpDebug.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "../core/LuaCoreLib.h"
#include "../core/LuaBanManLib.h"
#include "../core/LuaIP2CountryLib.h"
#include "../core/LuaProfManLib.h"
#include "../core/LuaRegManLib.h"
#include "../core/LuaScriptManLib.h"
#include "../core/LuaSetManLib.h"
#include "../core/LuaTmrManLib.h"
#include "../core/LuaUDPDbgLib.h"
#include "MainWindowPageScripts.h"
#include "Resources.h"
//---------------------------------------------------------------------------
static ATOM atomScriptEditorDialog = 0;
//---------------------------------------------------------------------------

static LRESULT CALLBACK MultiRichEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_GETDLGCODE && wParam == VK_TAB)
	{
		return 0;
	}
	
	return ::CallWindowProc(GuiSettingManager::m_wpOldMultiRichEditProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

ScriptEditorDialog::ScriptEditorDialog()
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
}
//---------------------------------------------------------------------------

ScriptEditorDialog::~ScriptEditorDialog()
{

}
//---------------------------------------------------------------------------

LRESULT CALLBACK ScriptEditorDialog::StaticScriptEditorDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ScriptEditorDialog * pScriptEditorDialog = (ScriptEditorDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if(pScriptEditorDialog == nullptr)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pScriptEditorDialog->ScriptEditorDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT ScriptEditorDialog::ScriptEditorDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_WINDOWPOSCHANGED:
	{
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		::SetWindowPos(m_hWndWindowItems[BTN_SAVE_SCRIPT], nullptr, (rcParent.right / 3) * 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2,
		               rcParent.right - ((rcParent.right / 3) * 2) - 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_CHECK_SYNTAX], nullptr, (rcParent.right / 3) + 1, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2,
		               (rcParent.right / 3) - 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_LOAD_SCRIPT], nullptr, 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (rcParent.right / 3) - 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[REDT_SCRIPT], nullptr, 0, 0, rcParent.right - ScaleGui(40), rcParent.bottom - GuiSettingManager::m_iEditHeight - 4, SWP_NOMOVE | SWP_NOZORDER);
		
		return 0;
	}
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case (REDT_SCRIPT+100):
			if(HIWORD(wParam) == EN_UPDATE)
			{
				OnUpdate();
			}
			break;
		case (BTN_LOAD_SCRIPT+100):
			OnLoadScript();
			return 0;
		case BTN_CHECK_SYNTAX:
			OnCheckSyntax();
			return 0;
		case BTN_SAVE_SCRIPT:
			OnSaveScript();
			return 0;
		case IDOK:
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		}
		
		if(RichEditCheckMenuCommands(m_hWndWindowItems[REDT_SCRIPT], LOWORD(wParam)) == true)
		{
			return 0;
		}
		
		break;
	case WM_CONTEXTMENU:
		OnContextMenu((HWND)wParam, lParam);
		break;
	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[REDT_SCRIPT] && ((LPNMHDR)lParam)->code == EN_LINK)
		{
			if(((ENLINK *)lParam)->msg == WM_LBUTTONUP)
			{
				RichEditOpenLink(m_hWndWindowItems[REDT_SCRIPT], (ENLINK *)lParam);
			}
		}
		
		break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = ScaleGui(443);
		mminfo->ptMinTrackSize.y = ScaleGui(454);
		
		return 0;
	}
	case WM_CLOSE:
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		ServerManager::m_hWndActiveDialog = nullptr;
		break;
	case WM_NCDESTROY:
	{
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_SETFOCUS:
	{
		CHARRANGE cr = { 0, 0 };
		::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_EXSETSEL, 0, (LPARAM)&cr);
		::SetFocus(m_hWndWindowItems[REDT_SCRIPT]);
		return 0;
	}
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void ScriptEditorDialog::DoModal(HWND hWndParent)
{
	if(atomScriptEditorDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_ScriptEditorDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomScriptEditorDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(443) / 2);
	int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(454) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomScriptEditorDialog), LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT_EDITOR],
	                                                    WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(443), ScaleGui(454),
	                                                    hWndParent, nullptr, ServerManager::m_hInstance, nullptr);
	                                                    
	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr)
	{
		return;
	}
	
	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticScriptEditorDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[REDT_SCRIPT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE |
	                                                  ES_WANTRETURN, ScaleGui(40), 0, rcParent.right - ScaleGui(40), rcParent.bottom - GuiSettingManager::m_iEditHeight - 4, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(REDT_SCRIPT+100), ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_EXLIMITTEXT, 0, (LPARAM)16777216);
	::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_AUTOURLDETECT, TRUE, 0);
	::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_GETEVENTMASK, 0, 0) | ENM_LINK);
	
	m_hWndWindowItems[BTN_LOAD_SCRIPT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_LOAD_SCRIPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                      2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (rcParent.right / 3) - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_LOAD_SCRIPT+100), ServerManager::m_hInstance, nullptr);
	                                                      
	{
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
		if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false || ServerManager::m_bServerRunning == false)
		{
			dwStyle |= WS_DISABLED;
		}
		
		m_hWndWindowItems[BTN_CHECK_SYNTAX] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CHECK_SYNTAX], dwStyle,
		                                                       (rcParent.right / 3) + 1, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, (rcParent.right / 3) - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CHECK_SYNTAX, ServerManager::m_hInstance, nullptr);
	}
	
	m_hWndWindowItems[BTN_SAVE_SCRIPT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SAVE_SCRIPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                      (rcParent.right / 3) * 2, rcParent.bottom - GuiSettingManager::m_iEditHeight - 2, rcParent.right - ((rcParent.right / 3) * 2) - 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_SAVE_SCRIPT,
	                                                      ServerManager::m_hInstance, nullptr);
	                                                      
	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		if(m_hWndWindowItems[ui8i] == nullptr)
		{
			return;
		}
		
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}
	
	GuiSettingManager::m_wpOldMultiRichEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndWindowItems[REDT_SCRIPT], GWLP_WNDPROC, (LONG_PTR)MultiRichEditProc);
	
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void ScriptEditorDialog::LoadScript(const char * sScript)
{
	FILE * pFile = fopen(sScript, "rb");
	
	if(pFile == nullptr)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (px_string(LanguageManager::m_Ptr->m_sTexts[LAN_FAILED_TO_OPEN], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FAILED_TO_OPEN]) + ": "+ sScript).c_str(),
		             LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK);
		return;
	}
	
	fseek(pFile, 0, SEEK_END);
	long lLenght = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	
	char * sFile = (char *)malloc(lLenght+1);
	
	if(sFile == nullptr)
	{
		fclose(pFile);
		
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (px_string(LanguageManager::m_Ptr->m_sTexts[LAN_FAILED_TO_OPEN], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FAILED_TO_OPEN]) + ": "+ sScript).c_str(),
		             LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK);
		             
		return;
	}
	
	fread(sFile, 1, lLenght, pFile);
	
	sFile[lLenght] = '\0';
	
	fclose(pFile);
	
	::SetWindowText(m_hWndWindowItems[REDT_SCRIPT], sFile);
	
	free(sFile);
	
	m_sScriptPath = sScript;
}
//------------------------------------------------------------------------------

void ScriptEditorDialog::OnContextMenu(HWND hWindow, LPARAM lParam)
{
	if(hWindow == m_hWndWindowItems[REDT_SCRIPT])
	{
		RichEditPopupMenu(m_hWndWindowItems[REDT_SCRIPT], m_hWndWindowItems[WINDOW_HANDLE], lParam);
	}
}
//------------------------------------------------------------------------------

void ScriptEditorDialog::OnUpdate()
{
	RECT rcScript;
	::GetClientRect(m_hWndWindowItems[REDT_SCRIPT], &rcScript);
	
	int iHeight = rcScript.bottom - rcScript.top;
	
	HDC hDC = ::GetDC(m_hWndWindowItems[WINDOW_HANDLE]);
	
	RECT rect = { 0, 0, ScaleGui(38), iHeight + 5 };
	
	::FillRect(hDC, &rect, ::GetSysColorBrush(COLOR_BTNFACE));
	rect.top = 2;
	
	::SetBkColor(hDC, ::GetSysColor(COLOR_BTNFACE));
	
	int iFirstVisibleLine = (int)::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_GETFIRSTVISIBLELINE, 0, 0);
	
	{
		int iFirstCharacterOnVisibleLine = (int)::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_LINEINDEX, (WPARAM)iFirstVisibleLine, 0);
		
		POINTL ptl = { 0 };
		::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_POSFROMCHAR, (WPARAM)&ptl, iFirstCharacterOnVisibleLine);
		
		rect.top += ptl.y;
	}
	
	POINTL ptl = { 0, iHeight };
	int iFirstCharacterOnLastLine = (long)::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_CHARFROMPOS, 0, (LPARAM)&ptl);
	int iLastVisibleLine = (long)::SendMessage(m_hWndWindowItems[REDT_SCRIPT], EM_EXLINEFROMCHAR, 0, iFirstCharacterOnLastLine);
	
	HGDIOBJ hOldObj = ::SelectObject(hDC, GuiSettingManager::m_hFont);
	
	SIZE sz = { 0 };
	::GetTextExtentPoint32(hDC, "1", 1, &sz);
	
	int iFontHeight = sz.cy;
	
	for(int iLine = iFirstVisibleLine; iLine <= iLastVisibleLine; iLine++)
	{
		rect.bottom = rect.top + iFontHeight;
		
		if(rect.bottom > (iHeight+5))
		{
			break;
		}
		
		px_string sLineNumber(iLine+1);
		::DrawText(hDC, sLineNumber.c_str(), (int)sLineNumber.size(), &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		
		rect.top = rect.top + iFontHeight;
	}
	
	::SelectObject(hDC, hOldObj);
	
	::ReleaseDC(m_hWndWindowItems[REDT_SCRIPT], hDC);
}
//------------------------------------------------------------------------------

void ScriptEditorDialog::OnLoadScript()
{
	char buf[MAX_PATH+1];
	if(m_sScriptPath.size() != 0)
	{
		strncpy(buf, m_sScriptPath.c_str(), MAX_PATH);
		buf[MAX_PATH] = '\0';
	}
	else
	{
		buf[0] = '\0';
	}
	
	OPENFILENAME OpenFileName = { 0 };
	OpenFileName.lStructSize = sizeof(OPENFILENAME);
	OpenFileName.hwndOwner = m_hWndWindowItems[WINDOW_HANDLE];
	OpenFileName.lpstrFilter = "Lua Scripts\0*.lua\0All Files\0*.*\0\0";
	OpenFileName.nFilterIndex = 1;
	OpenFileName.lpstrFile = buf;
	OpenFileName.nMaxFile = MAX_PATH;
	OpenFileName.lpstrInitialDir = ServerManager::m_sScriptPath.c_str();
	OpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;//OFN_OVERWRITEPROMPT
	OpenFileName.lpstrDefExt = "lua";
	
	if(::GetOpenFileName(&OpenFileName) != 0)
	{
		LoadScript(buf);
	}
}
//------------------------------------------------------------------------------

void ScriptEditorDialog::OnCheckSyntax()
{
	int iAllocLen = ::GetWindowTextLength(m_hWndWindowItems[REDT_SCRIPT]);
	
	char * sBuf = (char *)malloc(iAllocLen+1);
	
	if(sBuf == nullptr)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_FAILED_TO_CHECK_SYNTAX], LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK);
		return;
	}
	
	::GetWindowText(m_hWndWindowItems[REDT_SCRIPT], sBuf, iAllocLen+1);
	
	lua_State * L = lua_newstate(LuaAlocator, nullptr);
	
	if(L == nullptr)
	{
		free(sBuf);
		
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_FAILED_TO_CHECK_SYNTAX], LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK);
		return;
	}
	
	luaL_openlibs(L);
	
	if(ServerManager::m_bServerRunning == true)
	{
#if LUA_VERSION_NUM > 501
		luaL_requiref(L, "Core", RegCore, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "SetMan", RegSetMan, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "RegMan", RegRegMan, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "BanMan", RegBanMan, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "ProfMan", RegProfMan, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "TmrMan", RegTmrMan, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "UDPDbg", RegUDPDbg, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "ScriptMan", RegScriptMan, 1);
		lua_pop(L, 1);
		
		luaL_requiref(L, "IP2Country", RegIP2Country, 1);
		lua_pop(L, 1);
#else
		RegCore(L);
		RegSetMan(L);
		RegRegMan(L);
		RegBanMan(L);
		RegProfMan(L);
		RegTmrMan(L);
		RegUDPDbg(L);
		RegScriptMan(L);
		RegIP2Country(L);
#endif
	}
	
	if(luaL_dostring(L, sBuf) == 0)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], LanguageManager::m_Ptr->m_sTexts[LAN_NO_SYNERR_IN_SCRIPT], g_sPtokaXTitle, MB_OK);
		lua_close(L);
	}
	else
	{
		size_t szLen = 0;
		char * stmp = (char*)lua_tolstring(L, -1, &szLen);
		
		string sTmp(LanguageManager::m_Ptr->m_sTexts[LAN_SYNTAX], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SYNTAX]);
		sTmp += " ";
		sTmp += stmp;
		
		RichEditAppendText(MainWindowPageScripts::m_Ptr->m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS], sTmp.c_str());
		
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], sTmp.c_str(), LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK);
		
		lua_close(L);
	}
	
	free(sBuf);
}
//------------------------------------------------------------------------------

void ScriptEditorDialog::OnSaveScript()
{
	char buf[MAX_PATH+1];
	if(m_sScriptPath.size() != 0)
	{
		strncpy(buf, m_sScriptPath.c_str(), MAX_PATH);
		buf[MAX_PATH] = '\0';
	}
	else
	{
		buf[0] = '\0';
	}
	
	OPENFILENAME OpenFileName = { 0 };
	OpenFileName.lStructSize = sizeof(OPENFILENAME);
	OpenFileName.hwndOwner = m_hWndWindowItems[WINDOW_HANDLE];
	OpenFileName.lpstrFilter = "Lua Scripts\0*.lua\0All Files\0*.*\0\0";
	OpenFileName.nFilterIndex = 1;
	OpenFileName.lpstrFile = buf;
	OpenFileName.nMaxFile = MAX_PATH;
	OpenFileName.lpstrInitialDir = ServerManager::m_sScriptPath.c_str();
	OpenFileName.Flags = OFN_OVERWRITEPROMPT;
	OpenFileName.lpstrDefExt = "lua";
	
	if(::GetSaveFileName(&OpenFileName) == 0)
	{
		return;
	}
	
	int iAllocLen = ::GetWindowTextLength(m_hWndWindowItems[REDT_SCRIPT]);
	
	char * sBuf = (char *)malloc(iAllocLen+1);
	
	if(sBuf == nullptr)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_FAILED_TO_SAVE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FAILED_TO_SAVE]) + ": "+ buf).c_str(),
		             LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK);
		return;
	}
	
	int iLen = ::GetWindowText(m_hWndWindowItems[REDT_SCRIPT], sBuf, iAllocLen+1);
	
	FILE * pFile = fopen(buf, "wb");
	
	if(pFile == nullptr)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager::m_Ptr->m_sTexts[LAN_FAILED_TO_SAVE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FAILED_TO_SAVE]) + ": "+ buf).c_str(),
		             LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK);
		free(sBuf);
		return;
	}
	
	fwrite(sBuf, 1, (size_t)iLen, pFile);
	
	fclose(pFile);
	
	free(sBuf);
	
	m_sScriptPath = buf;
}
//------------------------------------------------------------------------------
