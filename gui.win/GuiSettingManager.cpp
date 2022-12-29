/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2022  Petr Kozelka, PPK at PtokaX dot org

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
#include "GuiSettingStr.h"
#include "GuiSettingDefaults.h"
#include "GuiSettingManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/ServerManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma hdrstop
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/PXBReader.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
GuiSettingManager * GuiSettingManager::m_Ptr = nullptr;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float GuiSettingManager::m_fScaleFactor = 1.0;
int GuiSettingManager::m_iGroupBoxMargin = 17;
int GuiSettingManager::m_iCheckHeight = 16;
int GuiSettingManager::m_iEditHeight = 23;
int GuiSettingManager::m_iTextHeight = 15;
int GuiSettingManager::m_iUpDownWidth = 17;
int GuiSettingManager::m_iOneLineGB = 17 + 23 + 8;
int GuiSettingManager::m_iOneLineOneChecksGB = 17 + 16 + 23 + 12;
int GuiSettingManager::m_iOneLineTwoChecksGB = 17 + (2 * 16) + 23 + 15;
HFONT GuiSettingManager::m_hFont = nullptr;
HCURSOR GuiSettingManager::m_hArrowCursor = nullptr;
HCURSOR GuiSettingManager::m_hVerticalCursor = nullptr;
WNDPROC GuiSettingManager::m_wpOldButtonProc = nullptr;
WNDPROC GuiSettingManager::m_wpOldEditProc = nullptr;
WNDPROC GuiSettingManager::m_wpOldListViewProc = nullptr;
WNDPROC GuiSettingManager::m_wpOldMultiRichEditProc = nullptr;
WNDPROC GuiSettingManager::m_wpOldNumberEditProc = nullptr;
WNDPROC GuiSettingManager::m_wpOldTabsProc = nullptr;
WNDPROC GuiSettingManager::m_wpOldTreeProc = nullptr;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static const char sPtokaXGUISettings[] = "PtokaX GUI Settings";
static const size_t szPtokaXGUISettingsLen = sizeof("PtokaX GUI Settings")-1;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GuiSettingManager::GuiSettingManager(void) {
	// Read default bools
	for(size_t szi = 0; szi < GUISETBOOL_IDS_END; szi++) {
		SetBool(szi, GuiSetBoolDef[szi]);
	}

	// Read default integers
	for(size_t szi = 0; szi < GUISETINT_IDS_END; szi++) {
		SetInteger(szi, GuiSetIntegerDef[szi]);
	}

	// Load settings
	Load();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GuiSettingManager::~GuiSettingManager(void) {
	// ...
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GuiSettingManager::Load() {
	PXBReader pxbSettings;

	// Open setting file
	if(pxbSettings.OpenFileRead((ServerManager::m_sPath + "\\cfg\\GuiSettigs.pxb").c_str(), 2) == false) {
		return;
	}

	// Read file header
	uint16_t ui16Identificators[2] = { *((uint16_t *)"FI"), *((uint16_t *)"FV") };

	if(pxbSettings.ReadNextItem(ui16Identificators, 2) == false) {
		return;
	}

	// Check header if we have correct file
	if(pxbSettings.m_ui16ItemLengths[0] != szPtokaXGUISettingsLen || strncmp((char *)pxbSettings.m_pItemDatas[0], sPtokaXGUISettings, szPtokaXGUISettingsLen) != 0) {
		return;
	}

	{
		uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbSettings.m_pItemDatas[1])));

		if(ui32FileVersion < 1) {
			return;
		}
	}

	// Read settings =)
	ui16Identificators[0] = *((uint16_t *)"SI");
	ui16Identificators[1] = *((uint16_t *)"SV");

	bool bSuccess = pxbSettings.ReadNextItem(ui16Identificators, 2);
	size_t szi = 0;

	while(bSuccess == true) {
		for(szi = 0; szi < GUISETBOOL_IDS_END; szi++) {
			if(pxbSettings.m_ui16ItemLengths[0] == strlen(GuiSetBoolStr[szi]) && strncmp((char *)pxbSettings.m_pItemDatas[0], GuiSetBoolStr[szi], pxbSettings.m_ui16ItemLengths[0]) == 0) {
				SetBool(szi, ((char *)pxbSettings.m_pItemDatas[1])[0] == '0' ? false : true);
			}
		}

		for(szi = 0; szi < GUISETINT_IDS_END; szi++) {
			if(pxbSettings.m_ui16ItemLengths[0] == strlen(GuiSetIntegerStr[szi]) && strncmp((char *)pxbSettings.m_pItemDatas[0], GuiSetIntegerStr[szi], pxbSettings.m_ui16ItemLengths[0]) == 0) {
				SetInteger(szi, ntohl(*((uint32_t *)(pxbSettings.m_pItemDatas[1]))));
			}
		}

		bSuccess = pxbSettings.ReadNextItem(ui16Identificators, 2);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GuiSettingManager::Save() const {
	PXBReader pxbSettings;

	// Open setting file
	if(pxbSettings.OpenFileSave((ServerManager::m_sPath + "\\cfg\\GuiSettigs.pxb").c_str(), 2) == false) {
		return;
	}

	// Write file header
	pxbSettings.m_sItemIdentifiers[0] = 'F';
	pxbSettings.m_sItemIdentifiers[1] = 'I';
	pxbSettings.m_ui16ItemLengths[0] = (uint16_t)szPtokaXGUISettingsLen;
	pxbSettings.m_pItemDatas[0] = (void *)sPtokaXGUISettings;
	pxbSettings.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

	pxbSettings.m_sItemIdentifiers[2] = 'F';
	pxbSettings.m_sItemIdentifiers[3] = 'V';
	pxbSettings.m_ui16ItemLengths[1] = 4;
	uint32_t ui32Version = 1;
	pxbSettings.m_pItemDatas[1] = (void *)&ui32Version;
	pxbSettings.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

	if(pxbSettings.WriteNextItem(szPtokaXGUISettingsLen+4, 2) == false) {
		return;
	}

	pxbSettings.m_sItemIdentifiers[0] = 'S';
	pxbSettings.m_sItemIdentifiers[1] = 'I';
	pxbSettings.m_sItemIdentifiers[2] = 'S';
	pxbSettings.m_sItemIdentifiers[3] = 'V';

	for(size_t szi = 0; szi < GUISETBOOL_IDS_END; szi++) {
		// Don't save setting with default value
		if(m_bBools[szi] == GuiSetBoolDef[szi]) {
			continue;
		}

		pxbSettings.m_ui16ItemLengths[0] = (uint16_t)strlen(GuiSetBoolStr[szi]);
		pxbSettings.m_pItemDatas[0] = (void *)GuiSetBoolStr[szi];
		pxbSettings.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

		pxbSettings.m_ui16ItemLengths[1] = 1;
		pxbSettings.m_pItemDatas[1] = (m_bBools[szi] == true ? (void *)1 : 0);
		pxbSettings.m_ui8ItemValues[1] = PXBReader::PXB_BYTE;

		if(pxbSettings.WriteNextItem(pxbSettings.m_ui16ItemLengths[0] + pxbSettings.m_ui16ItemLengths[1], 2) == false) {
			break;
		}
	}

	for(size_t szi = 0; szi < GUISETINT_IDS_END; szi++) {
		// Don't save setting with default value
		if(m_i32Integers[szi] == GuiSetIntegerDef[szi]) {
			continue;
		}

		pxbSettings.m_ui16ItemLengths[0] = (uint16_t)strlen(GuiSetIntegerStr[szi]);
		pxbSettings.m_pItemDatas[0] = (void *)GuiSetIntegerStr[szi];
		pxbSettings.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

		pxbSettings.m_ui16ItemLengths[1] = 4;
		pxbSettings.m_pItemDatas[1] = (void *)&m_i32Integers[szi];
		pxbSettings.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

		if(pxbSettings.WriteNextItem(pxbSettings.m_ui16ItemLengths[0] + pxbSettings.m_ui16ItemLengths[1], 2) == false) {
			break;
		}
	}

	pxbSettings.WriteRemaining();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool GuiSettingManager::GetDefaultBool(const size_t szBoolId) {
	return GuiSetBoolDef[szBoolId];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int32_t GuiSettingManager::GetDefaultInteger(const size_t szIntegerId) {
	return GuiSetIntegerDef[szIntegerId];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GuiSettingManager::SetBool(const size_t szBoolId, const bool bValue) {
	if(m_bBools[szBoolId] == bValue) {
		return;
	}

	m_bBools[szBoolId] = bValue;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GuiSettingManager::SetInteger(const size_t szIntegerId, const int32_t i32Value) {
	if(i32Value < 0 || m_i32Integers[szIntegerId] == i32Value) {
		return;
	}

	m_i32Integers[szIntegerId] = i32Value;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
