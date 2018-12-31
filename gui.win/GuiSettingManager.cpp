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
clsGuiSettingManager * clsGuiSettingManager::mPtr = nullptr;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float clsGuiSettingManager::fScaleFactor = 1.0;
int clsGuiSettingManager::iGroupBoxMargin = 17;
int clsGuiSettingManager::iCheckHeight = 16;
int clsGuiSettingManager::iEditHeight = 23;
int clsGuiSettingManager::iTextHeight = 15;
int clsGuiSettingManager::iUpDownWidth = 17;
int clsGuiSettingManager::iOneLineGB = 17 + 23 + 8;
int clsGuiSettingManager::iOneLineOneChecksGB = 17 + 16 + 23 + 12;
int clsGuiSettingManager::iOneLineTwoChecksGB = 17 + (2 * 16) + 23 + 15;
HFONT clsGuiSettingManager::hFont = nullptr;
HCURSOR clsGuiSettingManager::hArrowCursor = nullptr;
HCURSOR clsGuiSettingManager::hVerticalCursor = nullptr;
WNDPROC clsGuiSettingManager::wpOldButtonProc = nullptr;
WNDPROC clsGuiSettingManager::wpOldEditProc = nullptr;
WNDPROC clsGuiSettingManager::wpOldListViewProc = nullptr;
WNDPROC clsGuiSettingManager::wpOldMultiRichEditProc = nullptr;
WNDPROC clsGuiSettingManager::wpOldNumberEditProc = nullptr;
WNDPROC clsGuiSettingManager::wpOldTabsProc = nullptr;
WNDPROC clsGuiSettingManager::wpOldTreeProc = nullptr;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static const char sPtokaXGUISettings[] = "PtokaX GUI Settings";
static const size_t szPtokaXGUISettingsLen = sizeof("PtokaX GUI Settings") - 1;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsGuiSettingManager::clsGuiSettingManager(void)
{
	// Read default bools
	for (size_t szi = 0; szi < GUISETBOOL_IDS_END; szi++)
	{
		SetBool(szi, GuiSetBoolDef[szi]);
	}
	
	// Read default integers
	for (size_t szi = 0; szi < GUISETINT_IDS_END; szi++)
	{
		SetInteger(szi, GuiSetIntegerDef[szi]);
	}
	
	// Load settings
	Load();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsGuiSettingManager::~clsGuiSettingManager(void)
{
	// ...
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::Load()
{
	PXBReader pxbSettings;
	
	// Open setting file
	if (pxbSettings.OpenFileRead((clsServerManager::sPath + "\\cfg\\GuiSettigs.pxb").c_str(), 2) == false)
	{
		return;
	}
	
	// Read file header
	uint16_t ui16Identificators[2] = { *((uint16_t *)"FI"), *((uint16_t *)"FV") };
	
	if (pxbSettings.ReadNextItem(ui16Identificators, 2) == false)
	{
		return;
	}
	
	// Check header if we have correct file
	if (pxbSettings.ui16ItemLengths[0] != szPtokaXGUISettingsLen || strncmp((char *)pxbSettings.pItemDatas[0], sPtokaXGUISettings, szPtokaXGUISettingsLen) != 0)
	{
		return;
	}
	
	{
		uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbSettings.pItemDatas[1])));
		
		if (ui32FileVersion < 1)
		{
			return;
		}
	}
	
	// Read settings =)
	ui16Identificators[0] = *((uint16_t *)"SI");
	ui16Identificators[1] = *((uint16_t *)"SV");
	
	bool bSuccess = pxbSettings.ReadNextItem(ui16Identificators, 2);
	size_t szi = 0;
	
	while (bSuccess == true)
	{
		for (szi = 0; szi < GUISETBOOL_IDS_END; szi++)
		{
			if (pxbSettings.ui16ItemLengths[0] == strlen(GuiSetBoolStr[szi]) && strncmp((char *)pxbSettings.pItemDatas[0], GuiSetBoolStr[szi], pxbSettings.ui16ItemLengths[0]) == 0)
			{
				SetBool(szi, ((char *)pxbSettings.pItemDatas[1])[0] == '0' ? false : true);
			}
		}
		
		for (szi = 0; szi < GUISETINT_IDS_END; szi++)
		{
			if (pxbSettings.ui16ItemLengths[0] == strlen(GuiSetIntegerStr[szi]) && strncmp((char *)pxbSettings.pItemDatas[0], GuiSetIntegerStr[szi], pxbSettings.ui16ItemLengths[0]) == 0)
			{
				SetInteger(szi, ntohl(*((uint32_t *)(pxbSettings.pItemDatas[1]))));
			}
		}
		
		bSuccess = pxbSettings.ReadNextItem(ui16Identificators, 2);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::Save() const
{
	PXBReader pxbSettings;
	
	// Open setting file
	if (pxbSettings.OpenFileSave((clsServerManager::sPath + "\\cfg\\GuiSettigs.pxb").c_str(), 2) == false)
	{
		return;
	}
	
	// Write file header
	pxbSettings.sItemIdentifiers[0] = 'F';
	pxbSettings.sItemIdentifiers[1] = 'I';
	pxbSettings.ui16ItemLengths[0] = (uint16_t)szPtokaXGUISettingsLen;
	pxbSettings.pItemDatas[0] = (void *)sPtokaXGUISettings;
	pxbSettings.ui8ItemValues[0] = PXBReader::PXB_STRING;
	
	pxbSettings.sItemIdentifiers[2] = 'F';
	pxbSettings.sItemIdentifiers[3] = 'V';
	pxbSettings.ui16ItemLengths[1] = 4;
	uint32_t ui32Version = 1;
	pxbSettings.pItemDatas[1] = (void *)&ui32Version;
	pxbSettings.ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;
	
	if (pxbSettings.WriteNextItem(szPtokaXGUISettingsLen + 4, 2) == false)
	{
		return;
	}
	
	pxbSettings.sItemIdentifiers[0] = 'S';
	pxbSettings.sItemIdentifiers[1] = 'I';
	pxbSettings.sItemIdentifiers[2] = 'S';
	pxbSettings.sItemIdentifiers[3] = 'V';
	
	for (size_t szi = 0; szi < GUISETBOOL_IDS_END; szi++)
	{
		// Don't save setting with default value
		if (bBools[szi] == GuiSetBoolDef[szi])
		{
			continue;
		}
		
		pxbSettings.ui16ItemLengths[0] = (uint16_t)strlen(GuiSetBoolStr[szi]);
		pxbSettings.pItemDatas[0] = (void *)GuiSetBoolStr[szi];
		pxbSettings.ui8ItemValues[0] = PXBReader::PXB_STRING;
		
		pxbSettings.ui16ItemLengths[1] = 1;
		pxbSettings.pItemDatas[1] = (bBools[szi] == true ? (void *)1 : 0);
		pxbSettings.ui8ItemValues[1] = PXBReader::PXB_BYTE;
		
		if (pxbSettings.WriteNextItem(pxbSettings.ui16ItemLengths[0] + pxbSettings.ui16ItemLengths[1], 2) == false)
		{
			break;
		}
	}
	
	for (size_t szi = 0; szi < GUISETINT_IDS_END; szi++)
	{
		// Don't save setting with default value
		if (i32Integers[szi] == GuiSetIntegerDef[szi])
		{
			continue;
		}
		
		pxbSettings.ui16ItemLengths[0] = (uint16_t)strlen(GuiSetIntegerStr[szi]);
		pxbSettings.pItemDatas[0] = (void *)GuiSetIntegerStr[szi];
		pxbSettings.ui8ItemValues[0] = PXBReader::PXB_STRING;
		
		pxbSettings.ui16ItemLengths[1] = 4;
		pxbSettings.pItemDatas[1] = (void *)&i32Integers[szi];
		pxbSettings.ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;
		
		if (pxbSettings.WriteNextItem(pxbSettings.ui16ItemLengths[0] + pxbSettings.ui16ItemLengths[1], 2) == false)
		{
			break;
		}
	}
	
	pxbSettings.WriteRemaining();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool clsGuiSettingManager::GetDefaultBool(const size_t szBoolId)
{
	return GuiSetBoolDef[szBoolId];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int32_t clsGuiSettingManager::GetDefaultInteger(const size_t szIntegerId)
{
	return GuiSetIntegerDef[szIntegerId];;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::SetBool(const size_t szBoolId, const bool bValue)
{
	if (bBools[szBoolId] == bValue)
	{
		return;
	}
	
	bBools[szBoolId] = bValue;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGuiSettingManager::SetInteger(const size_t szIntegerId, const int32_t i32Value)
{
	if (i32Value < 0 || i32Integers[szIntegerId] == i32Value)
	{
		return;
	}
	
	i32Integers[szIntegerId] = i32Value;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
