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

//---------------------------------------------------------------------------
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "SettingDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "MainWindow.h"
#include "Resources.h"
#include "SettingPageAdvanced.h"
#include "SettingPageBans.h"
#include "SettingPageBots.h"
#include "SettingPageDeflood.h"
#include "SettingPageDeflood2.h"
#include "SettingPageDeflood3.h"
#include "SettingPageGeneral.h"
#include "SettingPageGeneral2.h"
#include "SettingPageMOTD.h"
#include "SettingPageMyINFO.h"
#include "SettingPageRules.h"
#include "SettingPageRules2.h"
#include "../core/TextFileManager.h"
//---------------------------------------------------------------------------
SettingDialog * SettingDialog::m_Ptr = nullptr;
//---------------------------------------------------------------------------
static ATOM atomSettingDialog = 0;
//---------------------------------------------------------------------------

SettingDialog::SettingDialog()
{
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
	memset(&SettingPages, 0, sizeof(SettingPages));
	
	SettingPages[0] = new (std::nothrow) SettingPageGeneral();
	SettingPages[1] = new (std::nothrow) SettingPageMOTD();
	SettingPages[2] = new (std::nothrow) SettingPageBots();
	SettingPages[3] = new (std::nothrow) SettingPageGeneral2();
	SettingPages[4] = new (std::nothrow) SettingPageBans();
	SettingPages[5] = new (std::nothrow) SettingPageAdvanced();
	SettingPages[6] = new (std::nothrow) SettingPageMyINFO();
	SettingPages[7] = new (std::nothrow) SettingPageRules();
	SettingPages[8] = new (std::nothrow) SettingPageRules2();
	SettingPages[9] = new (std::nothrow) SettingPageDeflood();
	SettingPages[10] = new (std::nothrow) SettingPageDeflood2();
	SettingPages[11] = new (std::nothrow) SettingPageDeflood3();
	
	for (uint8_t ui8i = 0; ui8i < 12; ui8i++)
	{
		if (SettingPages[ui8i] == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot allocate SettingPage[%" PRIu8 "] in SettingDialog::SettingDialog\n", ui8i);
			exit(EXIT_FAILURE);
		}
	}
}
//---------------------------------------------------------------------------

SettingDialog::~SettingDialog()
{
	for (uint8_t ui8i = 0; ui8i < 12; ui8i++)
	{
		delete SettingPages[ui8i];
	}
	
	SettingDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK SettingDialog::StaticSettingDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SettingDialog * pParent = (SettingDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	if (pParent == NULL)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return pParent->SettingDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT SettingDialog::SettingDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		::SetFocus(m_hWndWindowItems[TV_TREE]);
		
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			bool bUpdateHubNameWelcome = false, bUpdateHubName = false, bUpdateTCPPorts = false, bUpdateUDPPort = false, bUpdateAutoReg = false,
			     bUpdateMOTD = false, bUpdateHubSec = false, bUpdateRegOnlyMessage = false, bUpdateShareLimitMessage = false,
			     bUpdateSlotsLimitMessage = false, bUpdateHubSlotRatioMessage = false, bUpdateMaxHubsLimitMessage = false,
			     bUpdateNoTagMessage = false, bUpdateNickLimitMessage = false, bUpdateBotsSameNick = false, bUpdateBotNick = false,
			     bUpdateBot = false, bUpdateOpChatNick = false, bUpdateOpChat = false, bUpdateLanguage = false, bUpdateTextFiles = false,
			     bUpdateRedirectAddress = false, bUpdateTempBanRedirAddress = false, bUpdatePermBanRedirAddress = false, bUpdateSysTray = false,
			     bUpdateScripting = false, bUpdateMinShare = false, bUpdateMaxShare = false;
			     
			clsSettingManager::mPtr->bUpdateLocked = true;
			
			for (uint8_t ui8i = 0; ui8i < 12; ui8i++)
			{
				if (SettingPages[ui8i] != NULL)
				{
					SettingPages[ui8i]->Save();
					
					SettingPages[ui8i]->GetUpdates(bUpdateHubNameWelcome, bUpdateHubName, bUpdateTCPPorts, bUpdateUDPPort, bUpdateAutoReg,
					                               bUpdateMOTD, bUpdateHubSec, bUpdateRegOnlyMessage, bUpdateShareLimitMessage, bUpdateSlotsLimitMessage,
					                               bUpdateHubSlotRatioMessage, bUpdateMaxHubsLimitMessage, bUpdateNoTagMessage, bUpdateNickLimitMessage,
					                               bUpdateBotsSameNick, bUpdateBotNick, bUpdateBot, bUpdateOpChatNick, bUpdateOpChat, bUpdateLanguage, bUpdateTextFiles,
					                               bUpdateRedirectAddress, bUpdateTempBanRedirAddress, bUpdatePermBanRedirAddress, bUpdateSysTray, bUpdateScripting,
					                               bUpdateMinShare, bUpdateMaxShare);
				}
			}
			
			clsSettingManager::mPtr->bUpdateLocked = false;
			
			if (bUpdateHubSec == true)
			{
				clsSettingManager::mPtr->UpdateHubSec();
			}
			
			if (bUpdateMOTD == true)
			{
				clsSettingManager::mPtr->UpdateMOTD();
			}
			
			if (bUpdateLanguage == true)
			{
				clsSettingManager::mPtr->UpdateLanguage();
			}
			
			if (bUpdateHubNameWelcome == true)
			{
				clsSettingManager::mPtr->UpdateHubNameWelcome();
			}
			
			if (bUpdateHubName == true)
			{
				clsMainWindow::mPtr->UpdateTitleBar();
				clsSettingManager::mPtr->UpdateHubName();
			}
			
			if (bUpdateRedirectAddress == true)
			{
				clsSettingManager::mPtr->UpdateRedirectAddress();
			}
			
			if (bUpdateRegOnlyMessage == true)
			{
				clsSettingManager::mPtr->UpdateRegOnlyMessage();
			}
			
			if (bUpdateMinShare == true)
			{
				clsSettingManager::mPtr->UpdateMinShare();
			}
			
			if (bUpdateMaxShare == true)
			{
				clsSettingManager::mPtr->UpdateMaxShare();
			}
			
			if (bUpdateShareLimitMessage == true)
			{
				clsSettingManager::mPtr->UpdateShareLimitMessage();
			}
			
			if (bUpdateSlotsLimitMessage == true)
			{
				clsSettingManager::mPtr->UpdateSlotsLimitMessage();
			}
			
			if (bUpdateMaxHubsLimitMessage == true)
			{
				clsSettingManager::mPtr->UpdateMaxHubsLimitMessage();
			}
			
			if (bUpdateHubSlotRatioMessage == true)
			{
				clsSettingManager::mPtr->UpdateHubSlotRatioMessage();
			}
			
			if (bUpdateNoTagMessage == true)
			{
				clsSettingManager::mPtr->UpdateNoTagMessage();
			}
			
			if (bUpdateTempBanRedirAddress == true)
			{
				clsSettingManager::mPtr->UpdateTempBanRedirAddress();
			}
			
			if (bUpdatePermBanRedirAddress == true)
			{
				clsSettingManager::mPtr->UpdatePermBanRedirAddress();
			}
			
			if (bUpdateNickLimitMessage == true)
			{
				clsSettingManager::mPtr->UpdateNickLimitMessage();
			}
			
			if (bUpdateTCPPorts == true)
			{
				clsSettingManager::mPtr->UpdateTCPPorts();
			}
			
			if (bUpdateBotsSameNick == true)
			{
				clsSettingManager::mPtr->UpdateBotsSameNick();
			}
			
			if (bUpdateTextFiles == true)
			{
				clsTextFilesManager::mPtr->RefreshTextFiles();
			}
			
			if (bUpdateBot == true)
			{
				clsSettingManager::mPtr->UpdateBot(bUpdateBotNick);
			}
			
			if (bUpdateOpChat == true)
			{
				clsSettingManager::mPtr->UpdateOpChat(bUpdateOpChatNick);
			}
			
			if (bUpdateUDPPort == true)
			{
				clsSettingManager::mPtr->UpdateUDPPort();
			}
			
			if (bUpdateSysTray == true)
			{
				clsMainWindow::mPtr->UpdateSysTray();
			}
			
#ifdef FLYLINKDC_REMOVE_REGISTER_THREAD
			if (bUpdateAutoReg == true)
			{
				clsServerManager::UpdateAutoRegState();
			}
#endif
			
			if (bUpdateScripting == true)
			{
				clsSettingManager::mPtr->UpdateScripting();
			}
		}
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		}
		
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[TV_TREE])
		{
			if (((LPNMHDR)lParam)->code == TVN_SELCHANGED)
			{
				OnSelChanged();
				return 0;
			}
			else if (((LPNMHDR)lParam)->code == TVN_KEYDOWN)
			{
				NMTVKEYDOWN * ptvkd = (LPNMTVKEYDOWN)lParam;
				if (ptvkd->wVKey == VK_TAB)
				{
					if ((::GetKeyState(VK_SHIFT) & 0x8000) > 0)
					{
						HTREEITEM htiNode = (HTREEITEM)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETNEXTITEM, TVGN_CARET, 0);
						
						if (htiNode == NULL)
						{
							break;
						}
						
						TVITEM tvItem;
						memset(&tvItem, 0, sizeof(TVITEM));
						tvItem.hItem = htiNode;
						tvItem.mask = TVIF_PARAM;
						
						if ((BOOL)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETITEM, 0, (LPARAM)&tvItem) == FALSE)
						{
							break;
						}
						
						SettingPage * curSetPage = reinterpret_cast<SettingPage *>(tvItem.lParam);
						
						curSetPage->FocusLastItem();
						
						return 0;
					}
					else
					{
						::SetFocus(m_hWndWindowItems[BTN_OK]);
						
						return 0;
					}
				}
			}
		}
		
		break;
	case WM_CLOSE:
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		clsServerManager::hWndActiveDialog = nullptr;
		break;
	case WM_NCDESTROY:
	{
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	}
	
	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingDialog::DoModal(HWND hWndParent)
{
	if (atomSettingDialog == 0)
	{
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_SettingDialog";
		m_wc.hInstance = clsServerManager::hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;
		
		atomSettingDialog = ::RegisterClassEx(&m_wc);
	}
	
	int iWidth = ScaleGui(622);
	int iHeight = ScaleGui(494);
	
	RECT rcParent = { 0 };
	::GetWindowRect(hWndParent, &rcParent);
	
	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (iWidth / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top) / 2)) - (iHeight / 2);
	
	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomSettingDialog), clsLanguageManager::mPtr->sTexts[LAN_SETTINGS],
	                                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, iWidth, iHeight,
	                                                    hWndParent, NULL, clsServerManager::hInstance, NULL);
	                                                    
	if (m_hWndWindowItems[WINDOW_HANDLE] == NULL)
	{
		return;
	}
	
	clsServerManager::hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];
	
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticSettingDialogProc);
	
	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	m_hWndWindowItems[TV_TREE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS |
	                                              TVS_DISABLEDRAGDROP, 5, 5, ScaleGui(154), rcParent.bottom - (2 * clsGuiSettingManager::iEditHeight) - 16, m_hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
	                                              
	TVINSERTSTRUCT tvIS;
	memset(&tvIS, 0, sizeof(TVINSERTSTRUCT));
	tvIS.hInsertAfter = TVI_LAST;
	tvIS.item.mask = TVIF_PARAM | TVIF_TEXT;
	tvIS.itemex.mask |= TVIF_STATE;
	tvIS.itemex.state = TVIS_EXPANDED;
	tvIS.itemex.stateMask = TVIS_EXPANDED;
	
	tvIS.hParent = TVI_ROOT;
	
	for (uint8_t ui8i = 0; ui8i < 12; ui8i++)
	{
		if (ui8i == 5 || ui8i == 7 || ui8i == 9)
		{
			tvIS.hParent = TVI_ROOT;
		}
		
		tvIS.item.lParam = (LPARAM)SettingPages[ui8i];
		tvIS.item.pszText = SettingPages[ui8i]->GetPageName();
		if (ui8i == 0 || ui8i == 5 || ui8i == 7 || ui8i == 9)
		{
			tvIS.hParent = (HTREEITEM)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_INSERTITEM, 0, (LPARAM)&tvIS);
		}
		else
		{
			::SendMessage(m_hWndWindowItems[TV_TREE], TVM_INSERTITEM, 0, (LPARAM)&tvIS);
		}
	}
	
	m_hWndWindowItems[BTN_OK] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                             4, rcParent.bottom - (2 * clsGuiSettingManager::iEditHeight) - 7, ScaleGui(154) + 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, clsServerManager::hInstance, NULL);
	                                             
	m_hWndWindowItems[BTN_CANCEL] = ::CreateWindowEx(0, WC_BUTTON, clsLanguageManager::mPtr->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                                 4, rcParent.bottom - clsGuiSettingManager::iEditHeight - 4, ScaleGui(154) + 2, clsGuiSettingManager::iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, clsServerManager::hInstance, NULL);
	                                                 
	for (uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++)
	{
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));
	}
	
	clsGuiSettingManager::wpOldTreeProc = (WNDPROC)::SetWindowLongPtr(m_hWndWindowItems[TV_TREE], GWLP_WNDPROC, (LONG_PTR)TreeProc);
	
	::EnableWindow(hWndParent, FALSE);
	
	if (SettingPages[0]->CreateSettingPage(m_hWndWindowItems[WINDOW_HANDLE]) == false)
	{
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], "Setting page creation failed!", SettingPages[0]->GetPageName(), MB_OK);
		::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
	}
	
	RECT rcPage = { 0 };
	::GetWindowRect(SettingPages[0]->m_hWnd, &rcPage);
	
	int iDiff = rcParent.bottom - (rcPage.bottom - rcPage.top);
	
	::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
	
	if (iDiff != 0)
	{
		::SetWindowPos(m_hWndWindowItems[WINDOW_HANDLE], NULL, 0, 0, (rcParent.right - rcParent.left), (rcParent.bottom - rcParent.top) - iDiff, SWP_NOMOVE | SWP_NOZORDER);
		
		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);
		
		::SetWindowPos(m_hWndWindowItems[TV_TREE], NULL, 0, 0, ScaleGui(154), rcParent.bottom - (2 * clsGuiSettingManager::iEditHeight) - 16, SWP_NOMOVE | SWP_NOZORDER);
		
		::SetWindowPos(m_hWndWindowItems[BTN_OK], NULL, 4, rcParent.bottom - (2 * clsGuiSettingManager::iEditHeight) - 7, ScaleGui(154) + 2, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_CANCEL], NULL, 4, rcParent.bottom - clsGuiSettingManager::iEditHeight - 4, ScaleGui(154) + 2, clsGuiSettingManager::iEditHeight, SWP_NOZORDER);
	}
	
	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//---------------------------------------------------------------------------

void SettingDialog::OnSelChanged()
{
	HTREEITEM htiNode = (HTREEITEM)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETNEXTITEM, TVGN_CARET, 0);
	
	if (htiNode == NULL)
	{
		return;
	}
	
	char buf[256];
	
	TVITEM tvItem;
	memset(&tvItem, 0, sizeof(TVITEM));
	tvItem.hItem = htiNode;
	tvItem.mask = TVIF_PARAM | TVIF_TEXT;
	tvItem.pszText = buf;
	tvItem.cchTextMax = 256;
	
	if ((BOOL)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETITEM, 0, (LPARAM)&tvItem) == FALSE)
	{
		return;
	}
	
	SettingPage * curSetPage = reinterpret_cast<SettingPage *>(tvItem.lParam);
	
	if (curSetPage->bCreated == false)
	{
		if (curSetPage->CreateSettingPage(m_hWndWindowItems[WINDOW_HANDLE]) == false)
		{
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], "Setting page creation failed!", tvItem.pszText, MB_OK);
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
		}
	}
	
	::BringWindowToTop(curSetPage->m_hWnd);
}
//---------------------------------------------------------------------------
