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
#ifndef GuiSettingIdsH
#define GuiSettingIdsH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

enum GuiSetBoolIds {
	GUISETBOOL_SHOW_CHAT,
	GUISETBOOL_SHOW_COMMANDS,
	GUISETBOOL_AUTO_UPDATE_USERLIST,
	GUISETBOOL_IDS_END,
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

enum GuiSetIntegerIds {
	GUISETINT_MAIN_WINDOW_WIDTH,
	GUISETINT_MAIN_WINDOW_HEIGHT,
	GUISETINT_USERS_CHAT_SPLITTER,
	GUISETINT_SCRIPTS_SPLITTER,
	GUISETINT_SCRIPT_NAMES,
	GUISETINT_SCRIPT_MEMORY_USAGES,
	GUISETINT_REGS_WINDOW_WIDTH,
	GUISETINT_REGS_WINDOW_HEIGHT,
	GUISETINT_REGS_NICK,
	GUISETINT_REGS_PASSWORD,
	GUISETINT_REGS_PROFILE,
	GUISETINT_PROFILES_WINDOW_WIDTH,
	GUISETINT_PROFILES_WINDOW_HEIGHT,
	GUISETINT_BANS_WINDOW_WIDTH,
	GUISETINT_BANS_WINDOW_HEIGHT,
	GUISETINT_BANS_NICK,
	GUISETINT_BANS_IP,
	GUISETINT_BANS_REASON,
	GUISETINT_BANS_EXPIRE,
	GUISETINT_BANS_BY,
	GUISETINT_RANGE_BANS_WINDOW_WIDTH,
	GUISETINT_RANGE_BANS_WINDOW_HEIGHT,
	GUISETINT_RANGE_BANS_RANGE,
	GUISETINT_RANGE_BANS_REASON,
	GUISETINT_RANGE_BANS_EXPIRE,
	GUISETINT_RANGE_BANS_BY,
	GUISETINT_IDS_END
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
