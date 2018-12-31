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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GuiUtil.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/LanguageManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GuiSettingManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma hdrstop
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "Resources.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    enum enmMenuItems {
		IDC_COPY = 200,
		IDC_SELECT_ALL,
		IDC_CLEAR_ALL
    };

int ScaleGui(const int iValue) {
    return (int)(iValue * GuiSettingManager::m_fScaleFactor);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ScaleGuiDefaultsOnly(const int iValue) {
    if(GuiSettingManager::m_Ptr->m_i32Integers[iValue] == GuiSettingManager::m_Ptr->GetDefaultInteger(iValue)) {
        return (int)(GuiSettingManager::m_Ptr->m_i32Integers[iValue] * GuiSettingManager::m_fScaleFactor);
    } else {
        return GuiSettingManager::m_Ptr->m_i32Integers[iValue];
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RichEditOpenLink(const HWND hRichEdit, const ENLINK * pEnLink) {
    char * sURL = new (std::nothrow) char[(pEnLink->chrg.cpMax - pEnLink->chrg.cpMin)+1];

    if(sURL == nullptr) {
        return;
    }

    TEXTRANGE tr = { 0 };
    tr.chrg.cpMin = pEnLink->chrg.cpMin;
    tr.chrg.cpMax = pEnLink->chrg.cpMax;
    tr.lpstrText = sURL;

    ::SendMessage(hRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

    ::ShellExecute(nullptr, nullptr, sURL, nullptr, nullptr, SW_SHOWNORMAL);

    delete[] sURL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RichEditPopupMenu(const HWND hRichEdit, const HWND hParent, const LPARAM lParam) {
    HMENU hMenu = ::CreatePopupMenu();

    ::AppendMenu(hMenu, MF_STRING, IDC_COPY, LanguageManager::m_Ptr->m_sTexts[LAN_MENU_COPY]);
    ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenu(hMenu, MF_STRING, IDC_SELECT_ALL, LanguageManager::m_Ptr->m_sTexts[LAN_MENU_SELECT_ALL]);
    ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenu(hMenu, MF_STRING, IDC_CLEAR_ALL, LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_ALL]);

    ::SetMenuDefaultItem(hMenu, IDC_CLEAR_ALL, FALSE);

    int iX = GET_X_LPARAM(lParam);
    int iY = GET_Y_LPARAM(lParam);

    // -1, -1 is menu created by key. This need few tricks to show menu on correct position ;o)
    if(iX == -1 && iY == -1) {
        CHARRANGE cr = { 0, 0 };
        ::SendMessage(hRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);

        POINT pt = { 0 };
        ::SendMessage(hRichEdit, EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)cr.cpMax);

        RECT rcChat;
        ::GetClientRect(hRichEdit, &rcChat);

        if(pt.y < rcChat.top) {
            pt.y = rcChat.top;
        } else if(pt.y > rcChat.bottom) {
            pt.y = rcChat.bottom;
        }

        ::ClientToScreen(hRichEdit, &pt);
        iX = pt.x;
        iY = pt.y;
    }

    ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, hParent, nullptr);

    ::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool RichEditCheckMenuCommands(const HWND hRichEdit, const WORD wID) {
    switch(wID) {
        case IDC_COPY:
            ::SendMessage(hRichEdit, WM_COPY, 0, 0);
            return true;
        case IDC_SELECT_ALL: {
            CHARRANGE cr = { 0, -1 };
            ::SendMessage(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
            return true;
        }
        case IDC_CLEAR_ALL:
            ::SetWindowText(hRichEdit, "");
            return true;
        default:
            return false;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RichEditAppendText(const HWND hRichEdit, const char * sText, const bool bWithTime/* = true*/) {
    char msg[128];

    if(bWithTime == true) {
        time_t acc_time = time(nullptr);
        struct tm *tm = localtime(&acc_time);

        strftime(msg, 128, "\n[%X] ", tm);
    } else {
        msg[0] = '\n';
        msg[1] = '\0';
    }

    CHARRANGE cr = { 0, 0 };
    ::SendMessage(hRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    LONG lOldSelBegin = cr.cpMin;
    LONG lOldSelEnd = cr.cpMax;

    GETTEXTLENGTHEX gtle = { 0 };
    gtle.codepage = CP_ACP;
    gtle.flags = GTL_NUMCHARS;
    cr.cpMin = cr.cpMax = (LONG)::SendMessage(hRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
    ::SendMessage(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

    ::SendMessage(hRichEdit, EM_REPLACESEL, FALSE, ::GetWindowTextLength(hRichEdit) == 0 ? (LPARAM)msg+1 : (LPARAM)msg);

    ::SendMessage(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)sText);

    cr.cpMin = lOldSelBegin;
    cr.cpMax = lOldSelEnd;
    ::SendMessage(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

    ::SendMessage(hRichEdit, WM_VSCROLL, SB_BOTTOM, 0);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ListViewGetInsertPosition(const HWND hListView, const void * pItem, const bool bSortAscending, int (*pCompareFunc)(const void * pItem, const void * pOtherItem)) {
	int iHighLim = (int)::SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0) - 1;

	if(iHighLim == -1) {
		return 0;
    }

    int iCmpRes = 0, iLowLim = 0, iInsertPos = 0;

    void * pOtherItem = nullptr;

    while(iLowLim <= iHighLim) {
        iInsertPos = (iLowLim + iHighLim) / 2;
        pOtherItem = ListViewGetItem(hListView, iInsertPos);

        iCmpRes = (*pCompareFunc)(pItem, pOtherItem);

        if(iCmpRes == 0) {
            return iInsertPos;
        }

		if(bSortAscending == false) {
			iCmpRes = -iCmpRes;
        }

        if(iCmpRes < 0) {
            iHighLim = iInsertPos-1;
        } else {
            iLowLim = iInsertPos+1;
        }
    }

    if(iCmpRes > 0) {
        iInsertPos++;
    }

    return iInsertPos;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void * ListViewGetItem(const HWND hListView, const int iPos) {
	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = iPos;

	::SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&lvItem);

	return (void *)lvItem.lParam;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ListViewUpdateArrow(const HWND hListView, const bool bAscending, const int iSortColumn) {
	HWND hHeader = (HWND)::SendMessage(hListView, LVM_GETHEADER, 0, 0);
	const int iItemCount = (int)::SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);

	for(int i = 0; i < iItemCount; i++) {
		HDITEM hdItem = { 0 };
		hdItem.mask = HDI_FORMAT;

		::SendMessage(hHeader, HDM_GETITEM, i, (LPARAM)&hdItem);

		if(i == iSortColumn) {
            hdItem.fmt &= ~(bAscending ? HDF_SORTDOWN : HDF_SORTUP);
            hdItem.fmt |= (bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		} else {
            hdItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		}

		::SendMessage(hHeader, HDM_SETITEM, i, (LPARAM)&hdItem);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ListViewGetItemPosition(const HWND hListView, void * pItem) {
	LVFINDINFO lvFindInfo = { 0 };
    lvFindInfo.flags = LVFI_PARAM;
    lvFindInfo.lParam = (LPARAM)pItem;

	return (int)::SendMessage(hListView, LVM_FINDITEM, (WPARAM)-1, (LPARAM)&lvFindInfo);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ListViewGetMenuPos(const HWND hListView, int &iX, int &iY) {
    // -1, -1 is menu created by key. We need few tricks to show menu on correct position ;o)
    if(iX == -1 && iY == -1) {
        int iSel = (int)::SendMessage(hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

        POINT pt = { 0 };
        if((BOOL)::SendMessage(hListView, LVM_ISITEMVISIBLE, (WPARAM)iSel, 0) == FALSE) {
            RECT rcList;
            ::GetClientRect(hListView, &rcList);

            ::SendMessage(hListView, LVM_GETITEMPOSITION, (WPARAM)iSel, (LPARAM)&pt);

            pt.y = (pt.y < rcList.top) ? rcList.top : rcList.bottom;
        } else {
            RECT rcItem;
            rcItem.left = LVIR_LABEL;
            ::SendMessage(hListView, LVM_GETITEMRECT, (WPARAM)iSel, (LPARAM)&rcItem);

            pt.x = rcItem.left;
            pt.y = rcItem.top + ((rcItem.bottom - rcItem.top) / 2);
        }

        ::ClientToScreen(hListView, &pt);

        iX = pt.x;
        iY = pt.y;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ListViewSelectFirstItem(const HWND hListView) {
    if((int)::SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0) != 0) {
        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_STATE;
        lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
        lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        ::SendMessage(hListView, LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static LRESULT WantTabProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC wpOldProc) {
    if(uMsg == WM_GETDLGCODE && wParam == VK_TAB) {
        return DLGC_WANTTAB;
    }

    return ::CallWindowProc(wpOldProc, hWnd, uMsg, wParam, lParam);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK TabsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return WantTabProc(hWnd, uMsg, wParam, lParam, GuiSettingManager::m_wpOldTabsProc);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK TreeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return WantTabProc(hWnd, uMsg, wParam, lParam, GuiSettingManager::m_wpOldTreeProc);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
