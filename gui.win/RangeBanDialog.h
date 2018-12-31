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
#ifndef RangeBanDialogH
#define RangeBanDialogH
//------------------------------------------------------------------------------
struct RangeBanItem;
//------------------------------------------------------------------------------

class clsRangeBanDialog
{
public:
	HWND m_hWndWindowItems[17];
	
	enum enmWindowItems
	{
		WINDOW_HANDLE,
		GB_RANGE,
		EDT_FROM_IP,
		EDT_TO_IP,
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
	
	clsRangeBanDialog();
	~clsRangeBanDialog();
	
	static LRESULT CALLBACK StaticRangeBanDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void DoModal(HWND hWndParent, RangeBanItem * pRangeBan = NULL);
	void RangeBanDeleted(RangeBanItem * pRangeBan);
private:
	RangeBanItem * pRangeBanToChange;
	
	LRESULT RangeBanDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	bool OnAccept();
	DISALLOW_COPY_AND_ASSIGN(clsRangeBanDialog);
};
//------------------------------------------------------------------------------

#endif
