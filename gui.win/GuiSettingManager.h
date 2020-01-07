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

class GuiSettingManager {
public:
	static GuiSettingManager * m_Ptr;

	static HFONT m_hFont;
	static HCURSOR m_hArrowCursor;
	static HCURSOR m_hVerticalCursor;
	static WNDPROC m_wpOldButtonProc;
	static WNDPROC m_wpOldEditProc;
	static WNDPROC m_wpOldListViewProc;
	static WNDPROC m_wpOldMultiRichEditProc;
	static WNDPROC m_wpOldNumberEditProc;
	static WNDPROC m_wpOldTabsProc;
	static WNDPROC m_wpOldTreeProc;

	static float m_fScaleFactor;

	static int m_iGroupBoxMargin;
	static int m_iCheckHeight;
	static int m_iEditHeight;
	static int m_iTextHeight;
	static int m_iUpDownWidth;
	static int m_iOneLineGB;
	static int m_iOneLineOneChecksGB;
	static int m_iOneLineTwoChecksGB;

	int32_t m_i32Integers[GUISETINT_IDS_END]; //GuiSettingManager::mPtr->iIntegers[]

	bool m_bBools[GUISETBOOL_IDS_END]; //GuiSettingManager::mPtr->bBools[]

	GuiSettingManager(void);
	~GuiSettingManager(void);

	static bool GetDefaultBool(const size_t szBoolId);
	static int32_t GetDefaultInteger(const size_t szIntegerId);

	void SetBool(const size_t szBoolId, const bool bValue); //GuiSettingManager::mPtr->SetBool()
	void SetInteger(const size_t szIntegerId, const int32_t i32Value); //GuiSettingManager::mPtr->SetInteger()

	void Save() const;
private:
	GuiSettingManager(const GuiSettingManager&) = delete;
	const GuiSettingManager& operator=(const GuiSettingManager&) = delete;

	void Load();
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
