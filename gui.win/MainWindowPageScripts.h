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

//------------------------------------------------------------------------------
#ifndef MainWindowPageScriptsH
#define MainWindowPageScriptsH
//------------------------------------------------------------------------------
#include "BasicSplitter.h"
#include "MainWindowPage.h"
//------------------------------------------------------------------------------
class ScriptEditorDialog;
//------------------------------------------------------------------------------

class clsMainWindowPageScripts : public MainWindowPage, private BasicSplitter
{
public:
	static clsMainWindowPageScripts * mPtr;
	
	HWND hWndPageItems[8];
	
	enum enmPageItems
	{
		GB_SCRIPTS_ERRORS,
		REDT_SCRIPTS_ERRORS,
		BTN_OPEN_SCRIPT_EDITOR,
		BTN_REFRESH_SCRIPTS,
		LV_SCRIPTS,
		BTN_MOVE_UP,
		BTN_MOVE_DOWN,
		BTN_RESTART_SCRIPTS
	};
	
	clsMainWindowPageScripts();
	~clsMainWindowPageScripts();
	
	bool CreateMainWindowPage(HWND hOwner);
	void UpdateLanguage();
	char * GetPageName();
	void FocusFirstItem();
	void FocusLastItem();
	
	void ClearMemUsageAll();
	void UpdateMemUsage();
	void MoveScript(uint8_t ui8ScriptId, const bool bUp);
	void AddScriptsToList(const bool bDelete);
	void ScriptToList(const uint8_t ui8ScriptId, const bool bInsert, const bool bSelected);
	void UpdateCheck(const uint8_t ui8ScriptId);
	void OpenInScriptEditor();
private:
	bool bIgnoreItemChanged;
	
	
	LRESULT MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void OnContextMenu(HWND hWindow, LPARAM lParam);
	static void OpenScriptEditor(const char * sScript = NULL);
	void RefreshScripts();
	void OnItemChanged(const LPNMLISTVIEW &pListView);
	void OnDoubleClick(const LPNMITEMACTIVATE &pItemActivate);
	void MoveUp();
	void MoveDown();
	static void RestartScripts();
	void UpdateUpDown();
	void OpenInExternalEditor();
	void DeleteScript();
	void ClearMemUsage(uint8_t ui8ScriptId);
	
	HWND GetWindowHandle();
	void UpdateSplitterParts();
	
	DISALLOW_COPY_AND_ASSIGN(clsMainWindowPageScripts);
};
//---------------------------------------------------------------------------

#endif
