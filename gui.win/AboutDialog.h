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

//------------------------------------------------------------------------------
#ifndef AboutDialogH
#define AboutDialogH
//------------------------------------------------------------------------------

class AboutDialog {
public:
    AboutDialog();
    ~AboutDialog();

    static LRESULT CALLBACK StaticAboutDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void DoModal(HWND hWndParent);
private:
    HICON m_hSpider, m_hLua;

    HFONT m_hBigFont;

    HWND m_hWndWindowItems[4];

    enum enmWindowItems {
        WINDOW_HANDLE,
        LBL_PTOKAX_VERSION,
        LBL_LUA_VERSION,
        REDT_ABOUT
    };

    AboutDialog(const AboutDialog&) = delete;
    const AboutDialog& operator=(const AboutDialog&) = delete;

    LRESULT AboutDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif
