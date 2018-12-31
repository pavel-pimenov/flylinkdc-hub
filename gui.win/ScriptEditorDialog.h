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
#ifndef ScriptEditorDialogH
#define ScriptEditorDialogH
//------------------------------------------------------------------------------

class ScriptEditorDialog
{
public:
	HWND m_hWndWindowItems[5];
	
	enum enmWindowItems
	{
		WINDOW_HANDLE,
		REDT_SCRIPT,
		BTN_LOAD_SCRIPT,
		BTN_CHECK_SYNTAX,
		BTN_SAVE_SCRIPT
	};
	
	ScriptEditorDialog();
	~ScriptEditorDialog();
	
	static LRESULT CALLBACK StaticScriptEditorDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void DoModal(HWND hWndParent);
	void LoadScript(const char * sScript);
private:
	string sScriptPath;
	
	ScriptEditorDialog(const ScriptEditorDialog&);
	const ScriptEditorDialog& operator=(const ScriptEditorDialog&);
	
	LRESULT ScriptEditorDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void OnContextMenu(HWND hWindow, LPARAM lParam);
	void OnUpdate();
	void OnLoadScript();
	void OnCheckSyntax();
	void OnSaveScript();
	string prepareLoadSaveScript(OPENFILENAME& OpenFileName, bool isSave); //[+]FlylinkDC++
	//------------------------------------------------------------------------------
};

#endif
