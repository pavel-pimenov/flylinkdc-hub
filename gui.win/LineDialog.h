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
#ifndef LineDialogH
#define LineDialogH
//------------------------------------------------------------------------------

class LineDialog
{
public:
	explicit LineDialog(void (*pOnOkFunction)(const char * Line, const int iLen));
	~LineDialog();
	
	static LRESULT CALLBACK StaticLineDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void DoModal(HWND hWndParent, const char * Caption, const char * Line);
private:
	HWND m_hWndWindowItems[5];
	
	enum enmWindowItems
	{
		WINDOW_HANDLE,
		GB_LINE,
		EDT_LINE,
		BTN_OK,
		BTN_CANCEL
	};
	
	LineDialog(const LineDialog&);
	const LineDialog& operator=(const LineDialog&);
	
	LRESULT LineDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
