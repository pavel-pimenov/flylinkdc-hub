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
#include "LuaTmrManLib.h"
//---------------------------------------------------------------------------
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "utility.h"
#include "GlobalDataQueue.h"
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------

enum class AddTimerResult : uint8_t
{
    Ok,
    BadArgs,
    NoScript,
    EmptyName,
    BadFunc,
    NoMem
};

static AddTimerResult ParseAddTimerArgs(lua_State* pLua, int nArgs, const char*& sFunctionName, size_t& szLen, int& iRef)
{
    if (nArgs == 2)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            return AddTimerResult::BadArgs;
        }

        if (lua_type(pLua, 2) == LUA_TSTRING)
        {
            sFunctionName = lua_tolstring(pLua, 2, &szLen);
            if (szLen == 0)
            {
                return AddTimerResult::EmptyName;
            }
        }
        else if (lua_type(pLua, 2) == LUA_TFUNCTION)
        {
            iRef = luaL_ref(pLua, LUA_REGISTRYINDEX);
        }
        else
        {
            luaL_error(pLua, "bad argument #2 to 'AddTimer' (string or function expected, got %s)", lua_typename(pLua, lua_type(pLua, 2)));
            return AddTimerResult::BadArgs;
        }
    }
    else if (nArgs == 1)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            return AddTimerResult::BadArgs;
        }

        sFunctionName = ScriptTimer::m_sDefaultTimerFunc;
    }
    else
    {
        luaL_error(pLua, "bad argument count to 'AddTimer' (1 or 2 expected, got %d)", nArgs);
        return AddTimerResult::BadArgs;
    }

    if (sFunctionName)
    {
        lua_getglobal(pLua, sFunctionName);
        if (lua_isfunction(pLua, lua_gettop(pLua)) == 0)
        {
            return AddTimerResult::BadFunc;
        }
    }

    return AddTimerResult::Ok;
}

static int AddTimer(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    const Script* cur = ScriptManager::m_Ptr->FindScript(pLua);
    if (!cur)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const char* sFunctionName = nullptr;
    size_t szLen = 0;
    int iRef = 0;

    const AddTimerResult res = ParseAddTimerArgs(pLua, lua_gettop(pLua), sFunctionName, szLen, iRef);
    if (res != AddTimerResult::Ok)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    ScriptTimer* const pNewtimer = ScriptTimer::CreateScriptTimer(sFunctionName, szLen, iRef, cur->m_pLua);

    if (!pNewtimer)
    {
        LogDbg("[MEM] Cannot allocate pNewtimer in TmrMan.AddTimer");
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    pNewtimer->m_ui64Interval = LuaInt<uint64_t>(pLua, 1); // ms

    pNewtimer->m_ui64LastTick = NowMonotonicMs();

    lua_settop(pLua, 0);

    lua_pushlightuserdata(pLua, pNewtimer);

    ScriptManager::m_Ptr->m_TimerList.emplace_back(pNewtimer);
    ScriptManager::m_Ptr->m_ui64TimerListGen++;

    return 1;
}
//------------------------------------------------------------------------------

static int RemoveTimer(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TLIGHTUSERDATA)
    {
        luaL_checktype(pLua, 1, LUA_TLIGHTUSERDATA);
        lua_settop(pLua, 0);
        return 0;
    }

    const Script* cur = ScriptManager::m_Ptr->FindScript(pLua);
    if (!cur)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    auto* timer = reinterpret_cast<ScriptTimer*>(lua_touserdata(pLua, 1));

    auto& timerList = ScriptManager::m_Ptr->m_TimerList;
    for (auto it = timerList.begin(); it != timerList.end(); ++it)
    {
        if (it->get() == timer)
        {
            if ((*it)->m_sFunctionName.empty())
            {
                luaL_unref(pLua, LUA_REGISTRYINDEX, (*it)->m_iFunctionRef);
            }

            timerList.erase(it);
            ScriptManager::m_Ptr->m_ui64TimerListGen++;

            break;
        }
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg TmrManRegs[] = {{"AddTimer", AddTimer}, {"RemoveTimer", RemoveTimer}, {nullptr, nullptr}}; // NOLINT(modernize-avoid-c-arrays)
//---------------------------------------------------------------------------

int RegTmrMan(lua_State* pLua)
{
    luaL_newlib(pLua, TmrManRegs);
    return 1;
}
//---------------------------------------------------------------------------
