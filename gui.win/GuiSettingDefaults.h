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
#ifndef GuiSettingDefaultsH
#define GuiSettingDefaultsH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool GuiSetBoolDef[] = {
	false, // SHOW_CHAT
	false, // SHOW_COMMANDS
	false, // AUTO_UPDATE_USERLIST
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int32_t GuiSetIntegerDef[] = {
	400, // MAIN_WINDOW_WIDTH
	318, // MAIN_WINDOW_HEIGHT
	60, // USERS_CHAT_SPLITTER
	60, // SCRIPTS_SPLITTER
	100, // SCRIPT_NAMES
	50, // SCRIPT_MEMORY_USAGES
	443, // REGS_WINDOW_WIDTH
	454, // REGS_WINDOW_HEIGHT
	132, // REGS_NICK
	132, // REGS_PASSWORD
	132, // REGS_PROFILE
	443, // PROFILES_WINDOW_WIDTH
	454, // PROFILES_WINDOW_HEIGHT
	443, // BANS_WINDOW_WIDTH
	454, // BANS_WINDOW_HEIGHT
	200, // BANS_NICK
	160, // BANS_IP
	125, // BANS_REASON
	125, // BANS_EXPIRE
	100, // BANS_BY
	443, // RANGE_BANS_WINDOW_WIDTH
	454, // RANGE_BANS_WINDOW_HEIGHT
	250, // RANGE_BANS_RANGE
	125, // RANGE_BANS_REASON
	125, // RANGE_BANS_EXPIRE
	100, // RANGE_BANS_BY
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
