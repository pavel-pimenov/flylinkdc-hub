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
#ifndef UpdateDialogH
#define UpdateDialogH
//------------------------------------------------------------------------------

class UpdateDialog {
public:
    static UpdateDialog * m_Ptr;

    UpdateDialog();
    ~UpdateDialog();

    static LRESULT CALLBACK StaticUpdateDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void DoModal(HWND hWndParent);
	void Message(char * sData);
	bool ParseData(char * sData, HWND hWndParent);
private:
    HWND m_hWndWindowItems[2];

    enum enmWindowItems {
        WINDOW_HANDLE,
        REDT_UPDATE
    };

    UpdateDialog(const UpdateDialog&) = delete;
    const UpdateDialog& operator=(const UpdateDialog&) = delete;

    LRESULT UpdateDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
