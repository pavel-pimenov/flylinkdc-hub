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
#include "BasicSplitter.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma hdrstop
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

BasicSplitter::BasicSplitter() : m_iSplitterPos(0), m_iPercentagePos(0), m_bUpdatePercentagePos(true) {
	::SetRectEmpty(&m_rcSplitter);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool BasicSplitter::BasicSplitterProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_MOUSEMOVE:
		return OnMouseMove(wParam, lParam);
	case WM_LBUTTONDOWN:
		OnLButtonDown(lParam);
		return false;
	case WM_LBUTTONUP:
		OnLButtonUp();
		return false;
	}

	return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool BasicSplitter::OnMouseMove(WPARAM wParam, LPARAM lParam) {
	int ixPos = GET_X_LPARAM(lParam);
	int iyPos = GET_Y_LPARAM(lParam);

	if((wParam & MK_LBUTTON) && ::GetCapture() == GetWindowHandle()) {
		int iNewSplitPos = ixPos - m_rcSplitter.left;

		if(m_iSplitterPos != iNewSplitPos) {
			SetSplitterPosition(iNewSplitPos, true);
		}

		return true;
	} else {
		if(IsCursorOverSplitter(ixPos, iyPos) == true) {
			::SetCursor(GuiSettingManager::m_hVerticalCursor);
		} else {
			::SetCursor(GuiSettingManager::m_hArrowCursor);
		}

		return false;
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void BasicSplitter::OnLButtonDown(LPARAM lParam) {
	int ixPos = GET_X_LPARAM(lParam);
	int iyPos = GET_Y_LPARAM(lParam);

	if(IsCursorOverSplitter(ixPos, iyPos) == true) {
		::SetCapture(GetWindowHandle());
		::SetCursor(GuiSettingManager::m_hVerticalCursor);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void BasicSplitter::OnLButtonUp() {
	::ReleaseCapture();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void BasicSplitter::SetSplitterRect(const LPRECT lpRect) {
	m_rcSplitter = *lpRect;

	int iTotal = (m_rcSplitter.right - m_rcSplitter.left - 4);

	if(iTotal > 0) {
		m_bUpdatePercentagePos = false;

		int iNewPos = (m_iPercentagePos * iTotal) / 100;

		SetSplitterPosition(iNewPos, false);
	}

	UpdateSplitterParts();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void BasicSplitter::SetSplitterPosition(int iPos, const bool bUpdate/* = true*/) {
	if(iPos < 100) {
		iPos = 100;
	} else {
		int iMaxPos = (m_rcSplitter.right - m_rcSplitter.left) - 100;
		if(iPos > iMaxPos) {
			iPos = iMaxPos;
		}
	}

	bool bPosChanged = (m_iSplitterPos != iPos);

	m_iSplitterPos = iPos;

	if(m_bUpdatePercentagePos == true) {
		int iTotal = (m_rcSplitter.right - m_rcSplitter.left - 4);

		if(iTotal > 0) {
			m_iPercentagePos = (m_iSplitterPos * 100)/ iTotal;
		} else {
			m_iPercentagePos = 0;
		}
	} else {
		m_bUpdatePercentagePos = true;
	}

	if(bUpdate == true && bPosChanged == true) {
		UpdateSplitterParts();
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool BasicSplitter::IsCursorOverSplitter(const int iX, const int iY) const {
	if(iX == -1 || iY == -1 || iX < m_rcSplitter.left || iX > m_rcSplitter.right || iY < m_rcSplitter.top || iY > m_rcSplitter.bottom) {
		return false;
	}

	if(iX > (m_rcSplitter.left + m_iSplitterPos - 2) && iX < (m_rcSplitter.left + m_iSplitterPos + 2)) {
		return true;
	}

	return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
