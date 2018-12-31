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
#include "AboutDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#include "Resources.h"
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include <sqlite3.h>
#elif _WITH_POSTGRES
#include <libpq-fe.h>
#elif _WITH_MYSQL
#include <mysql.h>
#endif
//---------------------------------------------------------------------------
static ATOM atomAboutDialog = 0;
//---------------------------------------------------------------------------

AboutDialog::AboutDialog()
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
	
	hSpider = (HICON)::LoadImage(clsServerManager::hInstance, MAKEINTRESOURCE(IDR_MAINICONBIG), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	hLua = (HICON)::LoadImage(clsServerManager::hInstance, MAKEINTRESOURCE(IDR_LUAICON), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	
	LOGFONT lfFont;
	::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfFont);
	
	if (lfFont.lfHeight > 0)
	{
		lfFont.lfHeight = ScaleGui(20);
	}
	else
	{
		lfFont.lfHeight = ScaleGui(-20);
	}
	
	lfFont.lfWeight = FW_BOLD;
	
	hBigFont = ::CreateFontIndirect(&lfFont);
}
//---------------------------------------------------------------------------

AboutDialog::~AboutDialog()
{
	::DeleteObject(hSpider);
	::DeleteObject(hLua);
	
	::DeleteObject(hBigFont);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK AboutDialog::StaticAboutDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	AboutDialog * pAboutDialog = (AboutDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pAboutDialog == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pAboutDialog->AboutDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT AboutDialog::AboutDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		RECT rcParent;
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(m_hWndWindowItems[WINDOW_HANDLE], &ps);
		::DrawIconEx(hdc, 5, 5, hSpider, 0, 0, 0, NULL, DI_NORMAL);
		::DrawIconEx(hdc, rcParent.right - 69, 5, hLua, 0, 0, 0, NULL, DI_NORMAL);
		::EndPaint(m_hWndWindowItems[WINDOW_HANDLE], &ps);
		return 0;
	}
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[REDT_ABOUT] && ((LPNMHDR)lParam)->code == EN_LINK)
		{
			if (((ENLINK *)lParam)->msg == WM_LBUTTONUP)
			{
				RichEditOpenLink(m_hWndWindowItems[REDT_ABOUT], (ENLINK *)lParam);
			}
		}
		break;
	case WM_CLOSE:
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		clsServerManager::hWndActiveDialog = nullptr;
		break;
	case WM_NCDESTROY:
	{
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		}
		
		break;
	case WM_SETFOCUS:
	{
		CHARRANGE cr = { 0, 0 };
		::SendMessage(m_hWndWindowItems[REDT_ABOUT], EM_EXSETSEL, 0, (LPARAM)&cr);
		::SetFocus(m_hWndWindowItems[REDT_ABOUT]);
		return 0;
	}
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void AboutDialog::DoModal(HWND hWndParent)
{
	if (atomAboutDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_AboutDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomAboutDialog = ::RegisterClassEx(&m_wc);
	}
	
	RECT rcParent;
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (ScaleGui(443) / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (ScaleGui(454) / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomAboutDialog),
	                                                    (string(clsLanguageManager::mPtr->sTexts[LAN_ABOUT], clsLanguageManager::mPtr->ui16TextsLens[LAN_ABOUT]) + " PtokaX").c_str(),
	                                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(443), ScaleGui(454),
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticAboutDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[LBL_PTOKAX_VERSION] = ::CreateWindowEx(0, WC_STATIC, "PtokaX++ " PtokaXVersionString " [build " BUILD_NUMBER "]", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                                                         73, 10, ScaleGui(290), ScaleGui(25), m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[LBL_PTOKAX_VERSION], WM_SETFONT, (WPARAM)hBigFont, MAKELPARAM(TRUE, 0));
	
	m_hWndWindowItems[LBL_LUA_VERSION] = ::CreateWindowEx(0, WC_STATIC,
#ifdef _WITH_SQLITE
	                                                      LUA_RELEASE " / SQLite " SQLITE_VERSION
#elif _WITH_POSTGRES
	                                                      (LUA_RELEASE " / PostgreSQL " string(PQlibVersion())).c_str()
#elif _WITH_MYSQL
	                                                      LUA_RELEASE " / MySQL " MYSQL_SERVER_VERSION
#else
	                                                      LUA_RELEASE " / without SQL DB "
#endif
	                                                      , WS_CHILD | WS_VISIBLE | SS_CENTER, 73, ScaleGui(39), ScaleGui(290), ScaleGui(25),
	                                                      m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[LBL_LUA_VERSION], WM_SETFONT, (WPARAM)hBigFont, MAKELPARAM(TRUE, 0));
	
	m_hWndWindowItems[REDT_ABOUT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_CENTER | ES_READONLY,
	                                                 5, ScaleGui(74), rcParent.right - 10, rcParent.bottom - ScaleGui(74) - 5, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	::SendMessage(m_hWndWindowItems[REDT_ABOUT], EM_SETBKGNDCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
	::SendMessage(m_hWndWindowItems[REDT_ABOUT], EM_AUTOURLDETECT, TRUE, 0);
	::SendMessage(m_hWndWindowItems[REDT_ABOUT], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(m_hWndWindowItems[REDT_ABOUT], EM_GETEVENTMASK, 0, 0) | ENM_LINK);
	::SendMessage(m_hWndWindowItems[REDT_ABOUT], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	
	::SendMessage(m_hWndWindowItems[REDT_ABOUT], EM_REPLACESEL, FALSE, (LPARAM)"{\\rtf1\\ansi\\ansicpg1250{\\colortbl ;\\red0\\green0\\blue128;}"
	              "PtokaX is a server-software for the Direct Connect P2P Network.\\par\n"
	              "It's currently in developing by PPK and Ptaczek since May 2002.\\par\n"
	              "\\par\n"
	              "\\b Fork homepage:\\par\n"
	              "https://github.com/pavel-pimenov/PtokaX\\par\n"
	              "\\par\n"
	              "\\b Base homepage:\\par\n"
	              "http://www.PtokaX.org\\par\n"
	              "\\par\n"
	              "LUA Scripts forum:\\par\n"
	              "http://forum.PtokaX.org\\par\n"
	              "\\par\n"
	              "Wiki:\\par\n"
	              "http://wiki.PtokaX.org\\par\n"
	              "\\par\n"
	              "Designers:\\par\n"
	              "\\cf1 PPK\\cf0\\b0, PPK@PtokaX.org, ICQ: 122442343\\par\n"
	              "\\b\\cf1 Ptaczek\\cf0\\b0, ptaczek@PtokaX.org, ICQ: 2294472\\par\n"
	              "\\par\n"
	              "\\b Respect:\\b0\\par\n"
	              "aMutex, frontline3k, DirtyFinger, Yoshi, Nev.\\par\n"
	              "\\par\n"
	              "\\b Thanx to: (by Ptaczek)\\par\n"
	              "\\b\\cf1 Iceman\\cf0\\b0  for RAM and incredible and faithfull help with testing\\par\n"
	              "\\b\\cf1 PPK\\cf0\\b0  for incredible ability to find hidden bugs from very beginning of PtokaX\\par\n"
	              "\\b\\cf1 Beta-Team\\cf0\\b0  for quality work, ideas, suggestions and bugreports\\par\n"
	              "\\b\\cf1 Opium Volage\\cf0\\b0  for LUA forum hosting\\par\n"
	              "\\par\n"
	              "\\b Thanx to: (by PPK)\\par\n"
	              "\\b\\cf1 Sechmet\\cf0\\b0  for blue PtokaX icon\\par\n"
	              "\\par\n"
	              "}");
	              
	::EnableWindow(hWndParent, FALSE);
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//---------------------------------------------------------------------------
