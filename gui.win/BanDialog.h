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
#ifndef BanDialogH
#define BanDialogH
//------------------------------------------------------------------------------
struct BanItem;
//------------------------------------------------------------------------------

class BanDialog {
public:
    HWND m_hWndWindowItems[20];

    enum enmWindowItems {
        WINDOW_HANDLE,
        GB_NICK,
        EDT_NICK,
        BTN_NICK_BAN,
        GB_IP,
        EDT_IP,
        BTN_IP_BAN,
        BTN_FULL_BAN,
        GB_REASON,
        EDT_REASON,
        GB_BY,
        EDT_BY,
        GB_BAN_TYPE,
        RB_PERM_BAN,
        GB_TEMP_BAN,
        RB_TEMP_BAN,
        DT_TEMP_BAN_EXPIRE_DATE,
        DT_TEMP_BAN_EXPIRE_TIME,
        BTN_ACCEPT,
        BTN_DISCARD
    };

    BanDialog();
    ~BanDialog();

    static LRESULT CALLBACK StaticBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void DoModal(HWND hWndParent, BanItem * pBan = nullptr);
	void BanDeleted(BanItem * pBan);
private:
    BanItem * m_pBanToChange;

    BanDialog(const BanDialog&) = delete;
    const BanDialog& operator=(const BanDialog&) = delete;

    LRESULT BanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    bool OnAccept();
};
//------------------------------------------------------------------------------

#endif
