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
#include "LuaScriptManLib.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "utility.h"
#include "GlobalDataQueue.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static int GetScript(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    Script* const cur = ScriptManager::m_Ptr->FindScript(pLua);
    if (!cur)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    const int s = lua_gettop(pLua);

    lua_pushliteral(pLua, "sName");
    lua_pushstring(pLua, cur->m_sName.c_str());
    lua_rawset(pLua, s);

    lua_pushliteral(pLua, "bEnabled");
    lua_pushboolean(pLua, 1);
    lua_rawset(pLua, s);

    lua_pushliteral(pLua, "iMemUsage");

    lua_pushinteger(pLua, lua_gc(cur->m_pLua, LUA_GCCOUNT, 0));

    lua_rawset(pLua, s);

    return 1;
}
//------------------------------------------------------------------------------

static int GetScripts(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);

    const int t = lua_gettop(pLua); int n = 0;

    for (const auto& pScript : ScriptManager::m_Ptr->m_ppScriptTable)
    {
        lua_pushinteger(pLua, ++n);

        lua_newtable(pLua);
        const int s = lua_gettop(pLua);

        lua_pushliteral(pLua, "sName");
        lua_pushstring(pLua, pScript->m_sName.c_str());
        lua_rawset(pLua, s);

        lua_pushliteral(pLua, "bEnabled");
        pScript->m_bEnabled ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
        lua_rawset(pLua, s);

        lua_pushliteral(pLua, "iMemUsage");
        !pScript->m_pLua
            ? lua_pushnil(pLua)
            : lua_pushinteger(pLua, lua_gc(pScript->m_pLua, LUA_GCCOUNT, 0));

        lua_rawset(pLua, s);

        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int Move(lua_State* pLua, const bool bUp)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    if (lua_gettop(pLua) != 1)
    {
        luaL_error(pLua, "bad argument count to '%s' (1 expected, got %d)", bUp ? "MoveUp" : "MoveDown", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szLen;
    const char* sName = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const uint8_t ui8idx = ScriptManager::m_Ptr->FindScriptIdx(sName);
    if (ui8idx == static_cast<uint8_t>(ScriptManager::m_Ptr->m_ppScriptTable.size()) || (bUp && ui8idx == 0) ||
        (!bUp && ui8idx == static_cast<uint8_t>(ScriptManager::m_Ptr->m_ppScriptTable.size()) - 1))
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    ScriptManager::m_Ptr->PrepareMove(pLua);

    ScriptManager::m_Ptr->MoveScript(ui8idx, bUp);

    lua_settop(pLua, 0);
    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveUp(lua_State* pLua)
{
    return Move(pLua, true);
}
//------------------------------------------------------------------------------

static int MoveDown(lua_State* pLua)
{
    return Move(pLua, false);
}
//------------------------------------------------------------------------------

static int StartScript(lua_State* pLua)
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
    const char* sName = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if (!FileExist((ServerManager::m_sScriptPath + sName).c_str()))
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    Script* const curScript = ScriptManager::m_Ptr->FindScript(sName);
    if (curScript)
    {
        lua_settop(pLua, 0);

        if (curScript->m_pLua)
        {
            lua_pushnil(pLua);
            return 1;
        }

        if (!ScriptManager::m_Ptr->StartScript(curScript, true))
        {
            lua_pushnil(pLua);
            return 1;
        }

        lua_pushboolean(pLua, 1);
        return 1;
    }

    if (ScriptManager::m_Ptr->AddScript(sName, true, true) &&
        ScriptManager::m_Ptr->StartScript(ScriptManager::m_Ptr->m_ppScriptTable[static_cast<uint8_t>(ScriptManager::m_Ptr->m_ppScriptTable.size()) - 1], false))
    {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    }

    lua_settop(pLua, 0);
    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int RestartScript(lua_State* pLua)
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
    const char* sName = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    Script* const curScript = ScriptManager::m_Ptr->FindScript(sName);
    if (!curScript || !curScript->m_pLua)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    if (curScript->m_pLua == pLua)
    {
        EventQueue::m_Ptr->AddNormal(EventQueue::EventType::RSTSCRIPT, curScript->m_sName.c_str());

        lua_pushboolean(pLua, 1);
        return 1;
    }

    ScriptManager::m_Ptr->StopScript(curScript, false);

    if (ScriptManager::m_Ptr->StartScript(curScript, false))
    {
        lua_pushboolean(pLua, 1);
        return 1;
    }

    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int StopScript(lua_State* pLua)
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
    const char* sName = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    Script* const curScript = ScriptManager::m_Ptr->FindScript(sName);
    if (!curScript || !curScript->m_pLua)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    if (curScript->m_pLua == pLua)
    {
        EventQueue::m_Ptr->AddNormal(EventQueue::EventType::STOPSCRIPT, curScript->m_sName.c_str());

        lua_pushboolean(pLua, 1);
        return 1;
    }

    ScriptManager::m_Ptr->StopScript(curScript, true);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Restart(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    EventQueue::m_Ptr->AddNormal(EventQueue::EventType::RSTSCRIPTS, nullptr);

    return 0;
}
//------------------------------------------------------------------------------

static int Refresh(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    ScriptManager::m_Ptr->CheckForDeletedScripts();
    ScriptManager::m_Ptr->CheckForNewScripts();

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg ScriptManRegs[] = {{"GetScript", GetScript}, // NOLINT(modernize-avoid-c-arrays)
                                         {"GetScripts", GetScripts},
                                         {"MoveUp", MoveUp},
                                         {"MoveDown", MoveDown},
                                         {"StartScript", StartScript},
                                         {"RestartScript", RestartScript},
                                         {"StopScript", StopScript},
                                         {"Restart", Restart},
                                         {"Refresh", Refresh},
                                         {nullptr, nullptr}};
//---------------------------------------------------------------------------

int RegScriptMan(lua_State* pLua)
{
    luaL_newlib(pLua, ScriptManRegs);
    return 1;
}
//---------------------------------------------------------------------------
