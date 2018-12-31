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
#ifndef GuiUtilH
#define GuiUtilH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ScaleGui(const int iValue);
int ScaleGuiDefaultsOnly(const int iValue);
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RichEditOpenLink(const HWND hRichEdit, const ENLINK * pEnLink);
void RichEditPopupMenu(const HWND hRichEdit, const HWND hParent, const LPARAM lParam);
bool RichEditCheckMenuCommands(const HWND hRichEdit, const WORD wID);
void RichEditAppendText(const HWND hRichEdit, const char * sText, const bool bWithTime = true);
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ListViewGetInsertPosition(const HWND hListView, const void * pItem, const bool bSortAscending, int (*pCompareFunc)(const void * pItem, const void * pOtherItem));
void * ListViewGetItem(const HWND hListView, const int iPos);
void ListViewUpdateArrow(const HWND hListView, const bool bAscending, const int iSortColumn);
int ListViewGetItemPosition(const HWND hListView, void * pItem);
void ListViewGetMenuPos(const HWND hListView, int &iX, int &iY);
void ListViewSelectFirstItem(const HWND hListView);
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK TabsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TreeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
