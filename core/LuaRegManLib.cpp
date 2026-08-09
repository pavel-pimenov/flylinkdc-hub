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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "LuaInc.h"
//---------------------------------------------------------------------------
#include "LuaRegManLib.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void PushReg(lua_State* pLua, RegUser* pReg)
{
    lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    const int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sNick");
    lua_pushstring(pLua, pReg->m_sNick.c_str());
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sPassword");
    if (pReg->m_bPassHash)
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushstring(pLua, reinterpret_cast<const char*>(pReg->m_vPassData.data()));
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iProfile");
    lua_pushinteger(pLua, pReg->m_ui16Profile);

    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static int Save(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    RegManager::m_Ptr->Save();

    return 0;
}
//------------------------------------------------------------------------------

static int GetRegsByProfile(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TNUMBER)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto iProfile = LuaInt<uint16_t>(pLua, 1);

    lua_settop(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    for (const auto& curPtr : RegManager::m_Ptr->m_RegList)
    {
        RegUser* cur = curPtr.get();

        if (cur->m_ui16Profile == iProfile)
        {
            lua_pushinteger(pLua, ++i);

            PushReg(pLua, cur);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetRegsByOpStatus(lua_State* pLua, const bool bOperator)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    if (lua_gettop(pLua) != 0)
    {
        if (bOperator)
        {
            luaL_error(pLua, "bad argument count to 'GetOps' (0 expected, got %d)", lua_gettop(pLua));
        }
        else
        {
            luaL_error(pLua, "bad argument count to 'GetNonOps' (0 expected, got %d)", lua_gettop(pLua));
        }
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    for (const auto& curRegPtr : RegManager::m_Ptr->m_RegList)
    {
        RegUser* curReg = curRegPtr.get();

        if (ProfileManager::m_Ptr->IsProfileAllowed(curReg->m_ui16Profile, ProfileManager::HASKEYICON) == bOperator)
        {
            lua_pushinteger(pLua, ++i);

            PushReg(pLua, curReg);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetNonOps(lua_State* pLua)
{
    return GetRegsByOpStatus(pLua, false);
}
//------------------------------------------------------------------------------

static int GetOps(lua_State* pLua)
{
    return GetRegsByOpStatus(pLua, true);
}
//------------------------------------------------------------------------------

static int GetReg(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szLen;
    const char* sNick = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    RegUser* r = RegManager::m_Ptr->Find(std::string_view(sNick, szLen));

    lua_settop(pLua, 0);

    if (!r)
    {
        lua_pushnil(pLua);
        return 1;
    }

    PushReg(pLua, r);

    return 1;
}
//------------------------------------------------------------------------------

static int GetRegs(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    for (const auto& curRegPtr : RegManager::m_Ptr->m_RegList)
    {
        RegUser* curReg = curRegPtr.get();

        lua_pushinteger(pLua, ++i);

        PushReg(pLua, curReg);

        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int AddReg(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    if (lua_gettop(pLua) == 3)
    {
        if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TSTRING);
            luaL_checktype(pLua, 3, LUA_TNUMBER);
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        size_t szNickLen, szPassLen;
        const char* sNick = lua_tolstring(pLua, 1, &szNickLen);
        const char* sPass = lua_tolstring(pLua, 2, &szPassLen);

        const auto i16Profile = LuaInt<uint16_t>(pLua, 3);

        if (i16Profile > ProfileManager::m_Ptr->m_ui16ProfileCount - 1 || szNickLen == 0 || szNickLen > 64 || szPassLen == 0 || szPassLen > 64 ||
            contains_any(std::string_view(sNick, szNickLen), " $|") || contains_any(std::string_view(sPass, szPassLen), "|"))
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        const bool bAdded = RegManager::m_Ptr->AddNew(sNick, sPass, i16Profile);

        lua_settop(pLua, 0);

        if (!bAdded)
        {
            lua_pushnil(pLua);
            return 1;
        }

        lua_pushboolean(pLua, 1);
        return 1;
    }
    else if (lua_gettop(pLua) == 2)
    {
        if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TNUMBER);
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        size_t szNickLen;
        const char* sNick = lua_tolstring(pLua, 1, &szNickLen);

        const auto ui16Profile = LuaInt<uint16_t>(pLua, 2);

        if (ui16Profile > ProfileManager::m_Ptr->m_ui16ProfileCount - 1 || szNickLen == 0 || szNickLen > 64 || contains_any(std::string_view(sNick, szNickLen), " $|"))
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        // check if user is registered
        if (RegManager::m_Ptr->Find(std::string_view(sNick, szNickLen)))
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        User* pUser = HashManager::m_Ptr->FindUser(std::string_view(sNick, szNickLen));
        if (!pUser)
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        pUser->SetBuffer(ProfileManager::m_Ptr->m_vpProfilesTable[ui16Profile]->m_sName.c_str());
        pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;

        pUser->SendFormat("RegMan.AddReg",
                          true,
                          "<%s> %s.|$GetPass|",
                          SettingManager::HubSec(),
                          LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_WERE_REGISTERED_PLEASE_ENTER_YOUR_PASSWORD)].c_str());

        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    }
    else
    {
        luaL_error(pLua, "bad argument count to 'RegMan.AddReg' (2 or 3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }
}
//------------------------------------------------------------------------------

static int DelReg(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szNickLen;
    const char* sNick = lua_tolstring(pLua, 1, &szNickLen);

    if (szNickLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    RegUser* reg = RegManager::m_Ptr->Find(std::string_view(sNick, szNickLen));

    lua_settop(pLua, 0);

    if (!reg)
    {
        lua_pushnil(pLua);
        return 1;
    }

    RegManager::m_Ptr->Delete(reg);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int ChangeReg(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TNUMBER)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TNUMBER);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szNickLen, szPassLen = 0;
    const char* sNick = lua_tolstring(pLua, 1, &szNickLen);
    const char* sPass = nullptr;

    if (lua_type(pLua, 2) == LUA_TSTRING)
    {
        sPass = lua_tolstring(pLua, 2, &szPassLen);
        if (szPassLen == 0 || szPassLen > 64 || contains_any(std::string_view(sPass, szPassLen), "|"))
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }
    }
    else if (lua_type(pLua, 2) != LUA_TNIL)
    {
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto i16Profile = LuaInt<uint16_t>(pLua, 3);

    if (i16Profile > ProfileManager::m_Ptr->m_ui16ProfileCount - 1 || szNickLen == 0 || szNickLen > 64 || contains_any(std::string_view(sNick, szNickLen), " $|"))
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    RegUser* reg = RegManager::m_Ptr->Find(std::string_view(sNick, szNickLen));

    if (!reg)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    RegManager::m_Ptr->ChangeReg(reg, sPass, i16Profile);

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int ClrRegBadPass(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szNickLen;
    const char* sNick = lua_tolstring(pLua, 1, &szNickLen);

    if (szNickLen != 0)
    {
        RegUser* Reg = RegManager::m_Ptr->Find(std::string_view(sNick, szNickLen));
        if (Reg)
        {
            Reg->m_ui8BadPassCount = 0;
        }
        else
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }
    }

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static const luaL_Reg RegManRegs[] = {{"Save", Save}, // NOLINT(modernize-avoid-c-arrays)
                                      {"GetRegsByProfile", GetRegsByProfile},
                                      {"GetNonOps", GetNonOps},
                                      {"GetOps", GetOps},
                                      {"GetReg", GetReg},
                                      {"GetRegs", GetRegs},
                                      {"AddReg", AddReg},
                                      {"DelReg", DelReg},
                                      {"ChangeReg", ChangeReg},
                                      {"ClrRegBadPass", ClrRegBadPass},
                                      {nullptr, nullptr}};
//---------------------------------------------------------------------------

int RegRegMan(lua_State* pLua)
{
    luaL_newlib(pLua, RegManRegs);
    return 1;
}
//---------------------------------------------------------------------------
