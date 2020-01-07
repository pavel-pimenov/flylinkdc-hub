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

SettingDialog::SettingDialog() {
	memset(&m_hWndWindowItems, 0, sizeof(m_hWndWindowItems));
	memset(&m_SettingPages, 0, sizeof(m_SettingPages));

	m_SettingPages[0] = new (std::nothrow) SettingPageGeneral();
	m_SettingPages[1] = new (std::nothrow) SettingPageMOTD();
	m_SettingPages[2] = new (std::nothrow) SettingPageBots();
	m_SettingPages[3] = new (std::nothrow) SettingPageGeneral2();
	m_SettingPages[4] = new (std::nothrow) SettingPageBans();
	m_SettingPages[5] = new (std::nothrow) SettingPageAdvanced();
	m_SettingPages[6] = new (std::nothrow) SettingPageMyINFO();
	m_SettingPages[7] = new (std::nothrow) SettingPageRules();
	m_SettingPages[8] = new (std::nothrow) SettingPageRules2();
	m_SettingPages[9] = new (std::nothrow) SettingPageDeflood();
	m_SettingPages[10] = new (std::nothrow) SettingPageDeflood2();
	m_SettingPages[11] = new (std::nothrow) SettingPageDeflood3();

	for(uint8_t ui8i = 0; ui8i < 12; ui8i++) {
		if(m_SettingPages[ui8i] == nullptr) {
			AppendDebugLogFormat("[MEM] Cannot allocate SettingPage[%" PRIu8 "] in SettingDialog::SettingDialog\n", ui8i);
			exit(EXIT_FAILURE);
		}
	}
}
//---------------------------------------------------------------------------

SettingDialog::~SettingDialog() {
	for(uint8_t ui8i = 0; ui8i < 12; ui8i++) {
		delete m_SettingPages[ui8i];
	}

	SettingDialog::m_Ptr = nullptr;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK SettingDialog::StaticSettingDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	SettingDialog * pParent = (SettingDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pParent == nullptr) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pParent->SettingDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT SettingDialog::SettingDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_SETFOCUS:
		::SetFocus(m_hWndWindowItems[TV_TREE]);

		return 0;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK: {
			bool bUpdateHubNameWelcome = false, bUpdateHubName = false, bUpdateTCPPorts = false, bUpdateUDPPort = false, bUpdateAutoReg = false,
			     bUpdateMOTD = false, bUpdateHubSec = false, bUpdateRegOnlyMessage = false, bUpdateShareLimitMessage = false,
			     bUpdateSlotsLimitMessage = false, bUpdateHubSlotRatioMessage = false, bUpdateMaxHubsLimitMessage = false,
			     bUpdateNoTagMessage = false, bUpdateNickLimitMessage = false, bUpdateBotsSameNick = false, bUpdateBotNick = false,
			     bUpdateBot = false, bUpdateOpChatNick = false, bUpdateOpChat = false, bUpdateLanguage = false, bUpdateTextFiles = false,
			     bUpdateRedirectAddress = false, bUpdateTempBanRedirAddress = false, bUpdatePermBanRedirAddress = false, bUpdateSysTray = false,
			     bUpdateScripting = false, bUpdateMinShare = false, bUpdateMaxShare = false;

			SettingManager::m_Ptr->m_bUpdateLocked = true;

			for(uint8_t ui8i = 0; ui8i < 12; ui8i++) {
				if(m_SettingPages[ui8i] != nullptr) {
					m_SettingPages[ui8i]->Save();

					m_SettingPages[ui8i]->GetUpdates(bUpdateHubNameWelcome, bUpdateHubName, bUpdateTCPPorts, bUpdateUDPPort, bUpdateAutoReg,
					                                 bUpdateMOTD, bUpdateHubSec, bUpdateRegOnlyMessage, bUpdateShareLimitMessage, bUpdateSlotsLimitMessage,
					                                 bUpdateHubSlotRatioMessage, bUpdateMaxHubsLimitMessage, bUpdateNoTagMessage, bUpdateNickLimitMessage,
					                                 bUpdateBotsSameNick, bUpdateBotNick, bUpdateBot, bUpdateOpChatNick, bUpdateOpChat, bUpdateLanguage, bUpdateTextFiles,
					                                 bUpdateRedirectAddress, bUpdateTempBanRedirAddress, bUpdatePermBanRedirAddress, bUpdateSysTray, bUpdateScripting,
					                                 bUpdateMinShare, bUpdateMaxShare);
				}
			}

			SettingManager::m_Ptr->m_bUpdateLocked = false;

			if(bUpdateHubSec == true) {
				SettingManager::m_Ptr->UpdateHubSec();
			}

			if(bUpdateMOTD == true) {
				SettingManager::m_Ptr->UpdateMOTD();
			}

			if(bUpdateLanguage == true) {
				SettingManager::m_Ptr->UpdateLanguage();
			}

			if(bUpdateHubNameWelcome == true) {
				SettingManager::m_Ptr->UpdateHubNameWelcome();
			}

			if(bUpdateHubName == true) {
				MainWindow::m_Ptr->UpdateTitleBar();
				SettingManager::m_Ptr->UpdateHubName();
			}

			if(bUpdateRedirectAddress == true) {
				SettingManager::m_Ptr->UpdateRedirectAddress();
			}

			if(bUpdateRegOnlyMessage == true) {
				SettingManager::m_Ptr->UpdateRegOnlyMessage();
			}

			if(bUpdateMinShare == true) {
				SettingManager::m_Ptr->UpdateMinShare();
			}

			if(bUpdateMaxShare == true) {
				SettingManager::m_Ptr->UpdateMaxShare();
			}

			if(bUpdateShareLimitMessage == true) {
				SettingManager::m_Ptr->UpdateShareLimitMessage();
			}

			if(bUpdateSlotsLimitMessage == true) {
				SettingManager::m_Ptr->UpdateSlotsLimitMessage();
			}

			if(bUpdateMaxHubsLimitMessage == true) {
				SettingManager::m_Ptr->UpdateMaxHubsLimitMessage();
			}

			if(bUpdateHubSlotRatioMessage == true) {
				SettingManager::m_Ptr->UpdateHubSlotRatioMessage();
			}

			if(bUpdateNoTagMessage == true) {
				SettingManager::m_Ptr->UpdateNoTagMessage();
			}

			if(bUpdateTempBanRedirAddress == true) {
				SettingManager::m_Ptr->UpdateTempBanRedirAddress();
			}

			if(bUpdatePermBanRedirAddress == true) {
				SettingManager::m_Ptr->UpdatePermBanRedirAddress();
			}

			if(bUpdateNickLimitMessage == true) {
				SettingManager::m_Ptr->UpdateNickLimitMessage();
			}

			if(bUpdateTCPPorts == true) {
				SettingManager::m_Ptr->UpdateTCPPorts();
			}

			if(bUpdateBotsSameNick == true) {
				SettingManager::m_Ptr->UpdateBotsSameNick();
			}

			if(bUpdateTextFiles == true) {
				TextFilesManager::m_Ptr->RefreshTextFiles();
			}

			if(bUpdateBot == true) {
				SettingManager::m_Ptr->UpdateBot(bUpdateBotNick);
			}

			if(bUpdateOpChat == true) {
				SettingManager::m_Ptr->UpdateOpChat(bUpdateOpChatNick);
			}

			if(bUpdateUDPPort == true) {
				SettingManager::m_Ptr->UpdateUDPPort();
			}

			if(bUpdateSysTray == true) {
				MainWindow::m_Ptr->UpdateSysTray();
			}

#ifdef FLYLINKDC_REMOVE_REGISTER_THREAD
			if(bUpdateAutoReg == true) {
				ServerManager::UpdateAutoRegState();
			}
#endif
			if(bUpdateScripting == true) {
				SettingManager::m_Ptr->UpdateScripting();
			}
		}
		case IDCANCEL:
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
			return 0;
		}

		break;
	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->hwndFrom == m_hWndWindowItems[TV_TREE]) {
			if(((LPNMHDR)lParam)->code == TVN_SELCHANGED) {
				OnSelChanged();
				return 0;
			} else if(((LPNMHDR)lParam)->code == TVN_KEYDOWN) {
				NMTVKEYDOWN * ptvkd = (LPNMTVKEYDOWN)lParam;
				if(ptvkd->wVKey == VK_TAB) {
					if((::GetKeyState(VK_SHIFT) & 0x8000) > 0) {
						HTREEITEM htiNode = (HTREEITEM)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETNEXTITEM, TVGN_CARET, 0);

						if(htiNode == nullptr) {
							break;
						}

						TVITEM tvItem;
						memset(&tvItem, 0, sizeof(TVITEM));
						tvItem.hItem = htiNode;
						tvItem.mask = TVIF_PARAM;

						if((BOOL)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETITEM, 0, (LPARAM)&tvItem) == FALSE) {
							break;
						}

						SettingPage * curSetPage = reinterpret_cast<SettingPage *>(tvItem.lParam);

						curSetPage->FocusLastItem();

						return 0;
					} else {
						::SetFocus(m_hWndWindowItems[BTN_OK]);

						return 0;
					}
				}
			}
		}

		break;
	case WM_CLOSE:
		::EnableWindow(::GetParent(m_hWndWindowItems[WINDOW_HANDLE]), TRUE);
		ServerManager::m_hWndActiveDialog = nullptr;
		break;
	case WM_NCDESTROY: {
		HWND hWnd = m_hWndWindowItems[WINDOW_HANDLE];
		delete this;
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	}

	return ::DefWindowProc(m_hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingDialog::DoModal(HWND hWndParent) {
	if(atomSettingDialog == 0) {
		WNDCLASSEX m_wc;
		memset(&m_wc, 0, sizeof(WNDCLASSEX));
		m_wc.cbSize = sizeof(WNDCLASSEX);
		m_wc.lpfnWndProc = ::DefWindowProc;
		m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		m_wc.lpszClassName = "PtokaX_SettingDialog";
		m_wc.hInstance = ServerManager::m_hInstance;
		m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
		m_wc.style = CS_HREDRAW | CS_VREDRAW;

		atomSettingDialog = ::RegisterClassEx(&m_wc);
	}

	int iWidth = ScaleGui(622);
	int iHeight = ScaleGui(494);

	RECT rcParent = { 0 };
	::GetWindowRect(hWndParent, &rcParent);

	int iX = (rcParent.left + (((rcParent.right - rcParent.left)) / 2)) - (iWidth / 2);
	int iY = (rcParent.top + ((rcParent.bottom - rcParent.top)/ 2 )) - (iHeight / 2);

	m_hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomSettingDialog), LanguageManager::m_Ptr->m_sTexts[LAN_SETTINGS],
	                                   WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, iWidth, iHeight,
	                                   hWndParent, nullptr, ServerManager::m_hInstance, nullptr);

	if(m_hWndWindowItems[WINDOW_HANDLE] == nullptr) {
		return;
	}

	ServerManager::m_hWndActiveDialog = m_hWndWindowItems[WINDOW_HANDLE];

	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticSettingDialogProc);

	::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	m_hWndWindowItems[TV_TREE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS |
	                             TVS_DISABLEDRAGDROP, 5, 5, ScaleGui(154), rcParent.bottom - ( 2 * GuiSettingManager::m_iEditHeight) - 16, m_hWndWindowItems[WINDOW_HANDLE], nullptr, ServerManager::m_hInstance, nullptr);

	TVINSERTSTRUCT tvIS;
	memset(&tvIS, 0, sizeof(TVINSERTSTRUCT));
	tvIS.hInsertAfter = TVI_LAST;
	tvIS.item.mask = TVIF_PARAM | TVIF_TEXT;
	tvIS.itemex.mask |= TVIF_STATE;
	tvIS.itemex.state = TVIS_EXPANDED;
	tvIS.itemex.stateMask = TVIS_EXPANDED;

	tvIS.hParent = TVI_ROOT;

	for(uint8_t ui8i = 0; ui8i < 12; ui8i++) {
		if(ui8i == 5 || ui8i == 7 || ui8i == 9) {
			tvIS.hParent = TVI_ROOT;
		}

		tvIS.item.lParam = (LPARAM)m_SettingPages[ui8i];
		tvIS.item.pszText = m_SettingPages[ui8i]->GetPageName();
		if(ui8i == 0 || ui8i == 5 || ui8i == 7 || ui8i == 9) {
			tvIS.hParent = (HTREEITEM)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_INSERTITEM, 0, (LPARAM)&tvIS);
		} else {
			::SendMessage(m_hWndWindowItems[TV_TREE], TVM_INSERTITEM, 0, (LPARAM)&tvIS);
		}
	}

	m_hWndWindowItems[BTN_OK] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                            4, rcParent.bottom - ( 2 * GuiSettingManager::m_iEditHeight) - 7, ScaleGui(154) + 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDOK, ServerManager::m_hInstance, nullptr);

	m_hWndWindowItems[BTN_CANCEL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager::m_Ptr->m_sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
	                                4, rcParent.bottom - GuiSettingManager::m_iEditHeight - 4, ScaleGui(154) + 2, GuiSettingManager::m_iEditHeight, m_hWndWindowItems[WINDOW_HANDLE], (HMENU)IDCANCEL, ServerManager::m_hInstance, nullptr);

	for(uint8_t ui8i = 0; ui8i < (sizeof(m_hWndWindowItems) / sizeof(m_hWndWindowItems[0])); ui8i++) {
		::SendMessage(m_hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)GuiSettingManager::m_hFont, MAKELPARAM(TRUE, 0));
	}

	GuiSettingManager::m_wpOldTreeProc = (WNDPROC)::SetWindowLongPtr(m_hWndWindowItems[TV_TREE], GWLP_WNDPROC, (LONG_PTR)TreeProc);

	::EnableWindow(hWndParent, FALSE);

	if(m_SettingPages[0]->CreateSettingPage(m_hWndWindowItems[WINDOW_HANDLE]) == false) {
		::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], "Setting page creation failed!", m_SettingPages[0]->GetPageName(), MB_OK);
		::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
	}

	RECT rcPage = { 0 };
	::GetWindowRect(m_SettingPages[0]->m_hWnd, &rcPage);

	int iDiff = rcParent.bottom - (rcPage.bottom-rcPage.top);

	::GetWindowRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

	if(iDiff != 0) {
		::SetWindowPos(m_hWndWindowItems[WINDOW_HANDLE], nullptr, 0, 0, (rcParent.right-rcParent.left), (rcParent.bottom-rcParent.top) - iDiff, SWP_NOMOVE | SWP_NOZORDER);

		::GetClientRect(m_hWndWindowItems[WINDOW_HANDLE], &rcParent);

		::SetWindowPos(m_hWndWindowItems[TV_TREE], nullptr, 0, 0, ScaleGui(154), rcParent.bottom - ( 2 * GuiSettingManager::m_iEditHeight) - 16, SWP_NOMOVE | SWP_NOZORDER);

		::SetWindowPos(m_hWndWindowItems[BTN_OK], nullptr, 4, rcParent.bottom - ( 2 * GuiSettingManager::m_iEditHeight) - 7, ScaleGui(154) + 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
		::SetWindowPos(m_hWndWindowItems[BTN_CANCEL], nullptr, 4, rcParent.bottom - GuiSettingManager::m_iEditHeight - 4, ScaleGui(154) + 2, GuiSettingManager::m_iEditHeight, SWP_NOZORDER);
	}

	::ShowWindow(m_hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//---------------------------------------------------------------------------

void SettingDialog::OnSelChanged() {
	HTREEITEM htiNode = (HTREEITEM)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETNEXTITEM, TVGN_CARET, 0);

	if(htiNode == nullptr) {
		return;
	}

	char buf[256];

	TVITEM tvItem;
	memset(&tvItem, 0, sizeof(TVITEM));
	tvItem.hItem = htiNode;
	tvItem.mask = TVIF_PARAM | TVIF_TEXT;
	tvItem.pszText = buf;
	tvItem.cchTextMax = 256;

	if((BOOL)::SendMessage(m_hWndWindowItems[TV_TREE], TVM_GETITEM, 0, (LPARAM)&tvItem) == FALSE) {
		return;
	}

	SettingPage * curSetPage = reinterpret_cast<SettingPage *>(tvItem.lParam);

	if(curSetPage->m_bCreated == false) {
		if(curSetPage->CreateSettingPage(m_hWndWindowItems[WINDOW_HANDLE]) == false) {
			::MessageBox(m_hWndWindowItems[WINDOW_HANDLE], "Setting page creation failed!", tvItem.pszText, MB_OK);
			::PostMessage(m_hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
		}
	}

	::BringWindowToTop(curSetPage->m_hWnd);
}
//---------------------------------------------------------------------------
