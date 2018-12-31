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

//------------------------------------------------------------------------------
#ifndef ProfilesDialogH
#define ProfilesDialogH
//------------------------------------------------------------------------------

class ProfilesDialog {
public:
    static ProfilesDialog * m_Ptr;

    HWND m_hWndWindowItems[9];

    enum enmWindowItems {
        WINDOW_HANDLE,
        BTN_ADD_PROFILE,
        LV_PROFILES,
        BTN_MOVE_UP,
        BTN_MOVE_DOWN,
        GB_PERMISSIONS,
        LV_PERMISSIONS,
        BTN_SET_ALL,
        BTN_CLEAR_ALL
    };

    ProfilesDialog();
    ~ProfilesDialog();

    static LRESULT CALLBACK StaticProfilesDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void DoModal(HWND hWndParent);
	void RemoveProfile(const uint16_t iProfile);
	void AddProfile();
    void MoveDown(const uint16_t iProfile);
    void MoveUp(const uint16_t iProfile);
private:
    bool m_bIgnoreItemChanged;

    ProfilesDialog(const ProfilesDialog&) = delete;
    const ProfilesDialog& operator=(const ProfilesDialog&) = delete;

    LRESULT ProfilesDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void AddAllProfiles();
    void OnContextMenu(HWND hWindow, LPARAM lParam);
    void OnProfileChanged(const LPNMLISTVIEW pListView);
    void ChangePermissionChecks(const bool bCheck);
    void RenameProfile(const int iProfile);
    void UpdateUpDown();
    void OnPermissionChanged(const LPNMLISTVIEW pListView);
};
//------------------------------------------------------------------------------

#endif
