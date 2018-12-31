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
#ifndef GuiSettingManagerH
#define GuiSettingManagerH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GuiSettingIds.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class clsGuiSettingManager
{
private:

	void Load();
public:
	static clsGuiSettingManager * mPtr;
	
	static HFONT hFont;
	static HCURSOR hArrowCursor;
	static HCURSOR hVerticalCursor;
	static WNDPROC wpOldButtonProc;
	static WNDPROC wpOldEditProc;
	static WNDPROC wpOldListViewProc;
	static WNDPROC wpOldMultiRichEditProc;
	static WNDPROC wpOldNumberEditProc;
	static WNDPROC wpOldTabsProc;
	static WNDPROC wpOldTreeProc;
	
	static float fScaleFactor;
	
	static int iGroupBoxMargin;
	static int iCheckHeight;
	static int iEditHeight;
	static int iTextHeight;
	static int iUpDownWidth;
	static int iOneLineGB;
	static int iOneLineOneChecksGB;
	static int iOneLineTwoChecksGB;
	
	int32_t i32Integers[GUISETINT_IDS_END]; //clsGuiSettingManager::mPtr->iIntegers[]
	
	bool bBools[GUISETBOOL_IDS_END]; //clsGuiSettingManager::mPtr->bBools[]
	
	clsGuiSettingManager(void);
	~clsGuiSettingManager(void);
	
	static bool GetDefaultBool(const size_t szBoolId);
	static int32_t GetDefaultInteger(const size_t szIntegerId);
	
	void SetBool(const size_t szBoolId, const bool bValue); //clsGuiSettingManager::mPtr->SetBool()
	void SetInteger(const size_t szIntegerId, const int32_t i32Value); //clsGuiSettingManager::mPtr->SetInteger()
	
	void Save() const;
	
	DISALLOW_COPY_AND_ASSIGN(clsGuiSettingManager);
	
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
