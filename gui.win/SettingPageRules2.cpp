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

//---------------------------------------------------------------------------
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "SettingPageRules2.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------

SettingPageRules2::SettingPageRules2() : m_bUpdateSlotsLimitMessage(false), m_bUpdateHubSlotRatioMessage(false), m_bUpdateMaxHubsLimitMessage(false)
{
	memset(&m_hWndPageItems, 0, sizeof(m_hWndPageItems));
}
//---------------------------------------------------------------------------

LRESULT SettingPageRules2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_COMMAND)
	{
		switch(LOWORD(wParam))
		{
		case EDT_SLOTS_MSG:
		case EDT_SLOTS_REDIR_ADDR:
		case EDT_HUBS_SLOTS_MSG:
		case EDT_HUBS_SLOTS_REDIR_ADDR:
		case EDT_HUBS_MSG:
		case EDT_HUBS_REDIR_ADDR:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				RemovePipes((HWND)lParam);

				return 0;
			}

			break;
		case EDT_SLOTS_MIN:
		case EDT_SLOTS_MAX:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);

				uint16_t ui16Min = 0, ui16Max = 0;

				LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_SLOTS_MIN], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0)
				{
					ui16Min = LOWORD(lResult);
				}

				lResult = ::SendMessage(m_hWndPageItems[UD_SLOTS_MAX], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0)
				{
					ui16Max = LOWORD(lResult);
				}

				if(ui16Min == 0 && ui16Max == 0)
				{
					::EnableWindow(m_hWndPageItems[EDT_SLOTS_MSG], FALSE);
					::EnableWindow(m_hWndPageItems[BTN_SLOTS_REDIR], FALSE);
					::EnableWindow(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(m_hWndPageItems[EDT_SLOTS_MSG], TRUE);
					::EnableWindow(m_hWndPageItems[BTN_SLOTS_REDIR], TRUE);
					::EnableWindow(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}

				return 0;
			}

			break;
		case BTN_SLOTS_REDIR:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}

			break;
		case EDT_HUBS:
		case EDT_SLOTS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);

				uint16_t ui16Hubs = 0, ui16Slots = 0;

				LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_HUBS], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0)
				{
					ui16Hubs = LOWORD(lResult);
				}

				lResult = ::SendMessage(m_hWndPageItems[UD_SLOTS], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0)
				{
					ui16Slots = LOWORD(lResult);
				}

				if(ui16Hubs == 0 || ui16Slots == 0)
				{
					::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_MSG], FALSE);
					::EnableWindow(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], FALSE);
					::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_MSG], TRUE);
					::EnableWindow(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], TRUE);
					::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}

				return 0;
			}

			break;
		case BTN_HUBS_SLOTS_REDIR:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}

			break;
		case EDT_MAX_HUBS:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 0, 999);

				uint16_t ui16Hubs = 0;

				LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_MAX_HUBS], UDM_GETPOS, 0, 0);
				if(HIWORD(lResult) == 0)
				{
					ui16Hubs = LOWORD(lResult);
				}

				if(ui16Hubs == 0)
				{
					::EnableWindow(m_hWndPageItems[EDT_HUBS_MSG], FALSE);
					::EnableWindow(m_hWndPageItems[BTN_HUBS_REDIR], FALSE);
					::EnableWindow(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], FALSE);
				}
				else
				{
					::EnableWindow(m_hWndPageItems[EDT_HUBS_MSG], TRUE);
					::EnableWindow(m_hWndPageItems[BTN_HUBS_REDIR], TRUE);
					::EnableWindow(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
				}

				return 0;
			}

			break;
		case BTN_HUBS_REDIR:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				::EnableWindow(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], ::SendMessage(m_hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
			}

			break;
		case EDT_CTM_LEN:
		case EDT_RCTM_LEN:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				MinMaxCheck((HWND)lParam, 1, 512);

				return 0;
			}

			break;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageRules2::Save()
{
	if(m_bCreated == false)
	{
		return;
	}

	LRESULT lResult = ::SendMessage(m_hWndPageItems[UD_SLOTS_MIN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SLOTS_LIMIT])
		{
			m_bUpdateSlotsLimitMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SLOTS_LIMIT, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_SLOTS_MAX], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SLOTS_LIMIT])
		{
			m_bUpdateSlotsLimitMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SLOTS_LIMIT, LOWORD(lResult));
	}

	char buf[257];
	int iLen = ::GetWindowText(m_hWndPageItems[EDT_SLOTS_MSG], buf, 257);

	if(strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_SLOTS_LIMIT_MSG]) != 0)
	{
		m_bUpdateSlotsLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_SLOTS_LIMIT_MSG, buf, iLen);

	bool bSlotsRedir = ::SendMessage(m_hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bSlotsRedir != SettingManager::m_Ptr->m_bBools[SETBOOL_SLOTS_LIMIT_REDIR])
	{
		m_bUpdateSlotsLimitMessage = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_SLOTS_LIMIT_REDIR, bSlotsRedir);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], buf, 257);

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS]) != 0))
	{
		m_bUpdateSlotsLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_SLOTS_LIMIT_REDIR_ADDRESS, buf, iLen);

	lResult = ::SendMessage(m_hWndPageItems[UD_HUBS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS])
		{
			m_bUpdateHubSlotRatioMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_HUB_SLOT_RATIO_HUBS, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_SLOTS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS])
		{
			m_bUpdateHubSlotRatioMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_HUB_SLOT_RATIO_SLOTS, LOWORD(lResult));
	}

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUBS_SLOTS_MSG], buf, 257);

	if(strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_SLOT_RATIO_MSG]) != 0)
	{
		m_bUpdateHubSlotRatioMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_HUB_SLOT_RATIO_MSG, buf, iLen);

	bool bHubsSlotsRedir = ::SendMessage(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bHubsSlotsRedir != SettingManager::m_Ptr->m_bBools[SETBOOL_HUB_SLOT_RATIO_REDIR])
	{
		m_bUpdateHubSlotRatioMessage = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_HUB_SLOT_RATIO_REDIR, bHubsSlotsRedir);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], buf, 257);

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS]) != 0))
	{
		m_bUpdateHubSlotRatioMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS, buf, iLen);

	lResult = ::SendMessage(m_hWndPageItems[UD_MAX_HUBS], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		if(LOWORD(lResult) != SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_HUBS_LIMIT])
		{
			m_bUpdateMaxHubsLimitMessage = true;
		}
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_HUBS_LIMIT, LOWORD(lResult));
	}

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUBS_MSG], buf, 257);

	if(strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]) != 0)
	{
		m_bUpdateMaxHubsLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_MAX_HUBS_LIMIT_MSG, buf, iLen);

	bool bHubsRedir = ::SendMessage(m_hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

	if(bHubsRedir != SettingManager::m_Ptr->m_bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR])
	{
		m_bUpdateMaxHubsLimitMessage = true;
	}

	SettingManager::m_Ptr->SetBool(SETBOOL_MAX_HUBS_LIMIT_REDIR, bHubsRedir);

	iLen = ::GetWindowText(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], buf, 257);

	if((SettingManager::m_Ptr->m_sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] == nullptr && iLen != 0) ||
	        (SettingManager::m_Ptr->m_sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] != nullptr &&
	         strcmp(buf, SettingManager::m_Ptr->m_sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS]) != 0))
	{
		m_bUpdateMaxHubsLimitMessage = true;
	}

	SettingManager::m_Ptr->SetText(SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS, buf, iLen);

	lResult = ::SendMessage(m_hWndPageItems[UD_CTM_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_CTM_LEN, LOWORD(lResult));
	}

	lResult = ::SendMessage(m_hWndPageItems[UD_RCTM_LEN], UDM_GETPOS, 0, 0);
	if(HIWORD(lResult) == 0)
	{
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_RCTM_LEN, LOWORD(lResult));
	}
}
//------------------------------------------------------------------------------

void SettingPageRules2::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
                                   bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
                                   bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool & /*bUpdatedNoTagMessage*/,
                                   bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
                                   bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
                                   bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/)
{
	if(bUpdatedSlotsLimitMessage == false)
	{
		bUpdatedSlotsLimitMessage = m_bUpdateSlotsLimitMessage;
	}
	if(bUpdatedHubSlotRatioMessage == false)
	{
		bUpdatedHubSlotRatioMessage = m_bUpdateHubSlotRatioMessage;
	}
	if(bUpdatedMaxHubsLimitMessage == false)
	{
		bUpdatedMaxHubsLimitMessage = m_bUpdateMaxHubsLimitMessage;
	}
}

//------------------------------------------------------------------------------

bool SettingPageRules2::CreateSettingPage(HWND hOwner)
{
	CreateHWND(hOwner);

	if(m_bCreated == false)
	{
		return false;
	}

	RECT rcThis = { 0 };
	::GetWindowRect(m_hWnd, &rcThis);

	m_hWndPageItems[GB_SLOTS_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_SLOTS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                   0, 0, m_iFullGB, m_iOneLineTwoGroupGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SLOTS_MIN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 8, GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SLOTS_MIN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SLOTS_MIN], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_SLOTS_MIN], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_SLOTS_MIN], ScaleGui(40) + 8, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 0), (WPARAM)m_hWndPageItems[EDT_SLOTS_MIN],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SLOTS_LIMIT], 0));

	m_hWndPageItems[LBL_SLOTS_MIN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MIN_SLOTS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                 ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                 ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 15), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_SLOTS_MAX] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAX_SLOTS], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                                 ((rcThis.right - rcThis.left - 16) / 2) + 2, GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                 ((rcThis.right - rcThis.left) - ScaleGui(40) - GuiSettingManager::m_iUpDownWidth - 18) - (((rcThis.right - rcThis.left - 16) / 2) + 2), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SLOTS_MAX] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                 (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth - ScaleGui(40), GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SLOTS_MAX, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SLOTS_MAX], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_SLOTS_MAX], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_SLOTS_MAX], (rcThis.right - rcThis.left) - 13 - GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 0),
	          (WPARAM)m_hWndPageItems[EDT_SLOTS_MAX], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SLOTS_LIMIT], 0));

	m_hWndPageItems[GB_SLOTS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SLOTS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_SLOTS_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                 13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + 2, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SLOTS_MSG, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SLOTS_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_SLOTS_MSG], LanguageManager::m_Ptr->m_sTexts[LAN_SLOT_LIMIT_MSG_HINT]);

	m_hWndPageItems[GB_SLOTS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                  5, GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_SLOTS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                   13, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), ScaleGui(85), GuiSettingManager::m_iCheckHeight, m_hWnd,
	                                   (HMENU)BTN_SLOTS_REDIR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_SLOTS_REDIR], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[EDT_SLOTS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                        ES_AUTOHSCROLL, ScaleGui(85) + 18, (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), GuiSettingManager::m_iEditHeight,
	                                        m_hWnd, (HMENU)EDT_SLOTS_REDIR_ADDR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	int iPosY = m_iOneLineTwoGroupGB;

	m_hWndPageItems[GB_HUBS_SLOTS_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_HUB_SLOT_RATIO_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                        0, iPosY, m_iFullGB, m_iOneLineTwoGroupGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_HUBS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_HUBS], WS_CHILD | WS_VISIBLE | SS_RIGHT,
	                            8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 18, GuiSettingManager::m_iTextHeight,
	                            m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUBS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                            (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 5, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight,
	                            m_hWnd, (HMENU)EDT_HUBS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUBS], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_HUBS], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_HUBS], (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2) - GuiSettingManager::m_iUpDownWidth - 5, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight,
	          (LPARAM)MAKELONG(999, 0), (WPARAM)m_hWndPageItems[EDT_HUBS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS], 0));

	m_hWndPageItems[LBL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER,
	                               (((rcThis.right - rcThis.left) - 21) / 2) - (ScaleGui(10) / 2), iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ScaleGui(10), GuiSettingManager::m_iTextHeight,
	                               m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_SLOTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                             (((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + 5, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_SLOTS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_SLOTS], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_SLOTS], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_SLOTS], (((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + ScaleGui(40) + 5, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight,
	          (LPARAM)MAKELONG(999, 0), (WPARAM)m_hWndPageItems[EDT_SLOTS], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS], 0));

	m_hWndPageItems[LBL_SLOTS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_SLOTS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                             (((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 10, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                             (rcThis.right - rcThis.left) - ((((rcThis.right - rcThis.left) - 21) / 2) + (ScaleGui(10) / 2) + ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 23), GuiSettingManager::m_iTextHeight,
	                             m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[GB_HUBS_SLOTS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                     5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUBS_SLOTS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_SLOT_RATIO_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                      13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + 2, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUBS_SLOTS_MSG, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUBS_SLOTS_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_HUBS_SLOTS_MSG], LanguageManager::m_Ptr->m_sTexts[LAN_HUB_SLOT_RATIO_MSG_HINT]);

	m_hWndPageItems[GB_HUBS_SLOTS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                       5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_HUBS_SLOTS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                        13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), ScaleGui(85), GuiSettingManager::m_iCheckHeight, m_hWnd,
	                                        (HMENU)BTN_HUBS_SLOTS_REDIR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	        ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), GuiSettingManager::m_iEditHeight,
	        m_hWnd, (HMENU)EDT_HUBS_SLOTS_REDIR_ADDR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	iPosY *= 2;

	m_hWndPageItems[GB_HUBS_LIMIT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MAX_HUBS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                 0, iPosY, m_iFullGB, m_iOneLineTwoGroupGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_MAX_HUBS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                8, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_MAX_HUBS, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_MAX_HUBS], EM_SETLIMITTEXT, 3, 0);
	AddToolTip(m_hWndPageItems[EDT_MAX_HUBS], LanguageManager::m_Ptr->m_sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

	AddUpDown(m_hWndPageItems[UD_MAX_HUBS], ScaleGui(40) + 8, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(999, 0), (WPARAM)m_hWndPageItems[EDT_MAX_HUBS],
	          (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_HUBS_LIMIT], 0));

	m_hWndPageItems[LBL_MAX_HUBS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAX_HUBS], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 13, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                ((rcThis.right - rcThis.left - 16) / 2) - (ScaleGui(40) + GuiSettingManager::m_iUpDownWidth + 15), GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[GB_HUBS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                               5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_HUBS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
	                                13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + 2, m_iGBinGBEDT, GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_HUBS_MSG, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUBS_MSG], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_HUBS_MSG], LanguageManager::m_Ptr->m_sTexts[LAN_HUB_LIMIT_MSG_HINT]);

	m_hWndPageItems[GB_HUBS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                                 5, iPosY + GuiSettingManager::m_iGroupBoxMargin + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, m_iGBinGB, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[BTN_HUBS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
	                                  13, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2 + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iCheckHeight) / 2), ScaleGui(85), GuiSettingManager::m_iCheckHeight, m_hWnd,
	                                  (HMENU)BTN_HUBS_REDIR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[BTN_HUBS_REDIR], BM_SETCHECK, (SettingManager::m_Ptr->m_bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

	m_hWndPageItems[EDT_HUBS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager::m_Ptr->m_sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
	                                       ES_AUTOHSCROLL, ScaleGui(85) + 18, iPosY + (2 * GuiSettingManager::m_iGroupBoxMargin) + GuiSettingManager::m_iEditHeight + GuiSettingManager::m_iOneLineGB + 2, (rcThis.right - rcThis.left) - (ScaleGui(85) + 36), GuiSettingManager::m_iEditHeight,
	                                       m_hWnd, (HMENU)EDT_HUBS_REDIR_ADDR, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
	AddToolTip(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], LanguageManager::m_Ptr->m_sTexts[LAN_REDIRECT_HINT]);

	iPosY += m_iOneLineTwoGroupGB;

	m_hWndPageItems[GB_CTM_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_CTM_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                              0, iPosY, ((rcThis.right - rcThis.left - 5) / 2) - 2, GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_CTM_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                               8, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2), ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 23, GuiSettingManager::m_iTextHeight,
	                               m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_CTM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                               ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 10, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_CTM_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_CTM_LEN], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_CTM_LEN], ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - 10, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(512, 1),
	          (WPARAM)m_hWndPageItems[EDT_CTM_LEN], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CTM_LEN], 0));

	m_hWndPageItems[GB_RCTM_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_RCTM_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
	                               ((rcThis.right - rcThis.left - 5) / 2) + 3, iPosY, (rcThis.right - rcThis.left) - (((rcThis.right - rcThis.left - 5) / 2) + 8), GuiSettingManager::m_iOneLineGB, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[LBL_RCTM_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager::m_Ptr->m_sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT,
	                                ((rcThis.right - rcThis.left - 5) / 2) + 11, iPosY + GuiSettingManager::m_iGroupBoxMargin + ((GuiSettingManager::m_iEditHeight - GuiSettingManager::m_iTextHeight) / 2),
	                                (rcThis.right - rcThis.left) - ((rcThis.right - rcThis.left - 5) / 2) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 29, GuiSettingManager::m_iTextHeight, m_hWnd, nullptr, ServerManager::m_hInstance, nullptr);

	m_hWndPageItems[EDT_RCTM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
	                                (rcThis.right - rcThis.left) - GuiSettingManager::m_iUpDownWidth - ScaleGui(40) - 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, ScaleGui(40), GuiSettingManager::m_iEditHeight, m_hWnd, (HMENU)EDT_RCTM_LEN, ServerManager::m_hInstance, nullptr);
	::SendMessage(m_hWndPageItems[EDT_RCTM_LEN], EM_SETLIMITTEXT, 3, 0);

	AddUpDown(m_hWndPageItems[UD_RCTM_LEN], (rcThis.right - rcThis.left) - GuiSettingManager::m_iUpDownWidth - 13, iPosY + GuiSettingManager::m_iGroupBoxMargin, GuiSettingManager::m_iUpDownWidth, GuiSettingManager::m_iEditHeight, (LPARAM)MAKELONG(512, 1),
	          (WPARAM)m_hWndPageItems[EDT_RCTM_LEN], (LPARAM)MAKELONG(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_RCTM_LEN], 0));

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndPageItems) / sizeof(m_hWndPageItems[0])); ui8i++)
	{
		if(m_hWndPageItems[ui8i] == nullptr)
		{
			return false;
		}

		::SendMessage(m_hWndPageItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SLOTS_LIMIT] == 0 && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SLOTS_LIMIT] == 0)
	{
		::EnableWindow(m_hWndPageItems[EDT_SLOTS_MSG], FALSE);
		::EnableWindow(m_hWndPageItems[BTN_SLOTS_REDIR], FALSE);
		::EnableWindow(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(m_hWndPageItems[EDT_SLOTS_MSG], TRUE);
		::EnableWindow(m_hWndPageItems[BTN_SLOTS_REDIR], TRUE);
		::EnableWindow(m_hWndPageItems[EDT_SLOTS_REDIR_ADDR], SettingManager::m_Ptr->m_bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true ? TRUE : FALSE);
	}

	if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS] == 0 || SettingManager::m_Ptr->m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] == 0)
	{
		::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_MSG], FALSE);
		::EnableWindow(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], FALSE);
		::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_MSG], TRUE);
		::EnableWindow(m_hWndPageItems[BTN_HUBS_SLOTS_REDIR], TRUE);
		::EnableWindow(m_hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], SettingManager::m_Ptr->m_bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true ? TRUE : FALSE);
	}

	if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_HUBS_LIMIT] == 0)
	{
		::EnableWindow(m_hWndPageItems[EDT_HUBS_MSG], FALSE);
		::EnableWindow(m_hWndPageItems[BTN_HUBS_REDIR], FALSE);
		::EnableWindow(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], FALSE);
	}
	else
	{
		::EnableWindow(m_hWndPageItems[EDT_HUBS_MSG], TRUE);
		::EnableWindow(m_hWndPageItems[BTN_HUBS_REDIR], TRUE);
		::EnableWindow(m_hWndPageItems[EDT_HUBS_REDIR_ADDR], SettingManager::m_Ptr->m_bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true ? TRUE : FALSE);
	}

	GuiSettingManager::m_wpOldNumberEditProc = (WNDPROC)::SetWindowLongPtr(m_hWndPageItems[EDT_RCTM_LEN], GWLP_WNDPROC, (LONG_PTR)NumberEditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageRules2::GetPageName()
{
	return LanguageManager::m_Ptr->m_sTexts[LAN_MORE_RULES];
}
//------------------------------------------------------------------------------

void SettingPageRules2::FocusLastItem()
{
	::SetFocus(m_hWndPageItems[EDT_RCTM_LEN]);
}
//------------------------------------------------------------------------------
