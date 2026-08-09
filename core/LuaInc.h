/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
#ifndef LuaIncH
#define LuaIncH
//---------------------------------------------------------------------------

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// Helper macros for Lua C binding argument validation
// Usage: LUA_CHECK_ARGS(pLua, 2) -- checks exactly 2 args
//        LUA_CHECK_ARGS_MIN(pLua, 1) -- checks at least 1 arg
//        LUA_CHECK_TYPE(pLua, 1, LUA_TSTRING) -- checks arg 1 is string

#define LUA_CHECK_ARGS(pLua, N)                                                                                                                                \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (lua_gettop(pLua) != (N))                                                                                                                           \
        {                                                                                                                                                      \
            luaL_error(pLua, "bad argument count to '%s' (%d expected, got %d)", __func__, (N), lua_gettop(pLua));                                             \
            lua_settop(pLua, 0);                                                                                                                               \
            return 0;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

#define LUA_CHECK_ARGS_RET_NIL(pLua, N)                                                                                                                        \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (lua_gettop(pLua) != (N))                                                                                                                           \
        {                                                                                                                                                      \
            luaL_error(pLua, "bad argument count to '%s' (%d expected, got %d)", __func__, (N), lua_gettop(pLua));                                             \
            lua_settop(pLua, 0);                                                                                                                               \
            lua_pushnil(pLua);                                                                                                                                 \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

#define LUA_CHECK_ARGS_MIN(pLua, N)                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (lua_gettop(pLua) < (N))                                                                                                                            \
        {                                                                                                                                                      \
            luaL_error(pLua, "bad argument count to '%s' (%d expected, got %d)", __func__, (N), lua_gettop(pLua));                                             \
            lua_settop(pLua, 0);                                                                                                                               \
            return 0;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

#define LUA_CHECK_TYPE(pLua, idx, t)                                                                                                                           \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (lua_type(pLua, (idx)) != (t))                                                                                                                      \
        {                                                                                                                                                      \
            luaL_checktype(pLua, (idx), (t));                                                                                                                  \
            lua_settop(pLua, 0);                                                                                                                               \
            lua_pushnil(pLua);                                                                                                                                 \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

#define LUA_PUSH_BOOL(pLua, val)                                                                                                                               \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        lua_settop(pLua, 0);                                                                                                                                   \
        lua_pushboolean(pLua, (val) ? 1 : 0);                                                                                                                  \
        return 1;                                                                                                                                              \
    } while (0)

#define LUA_PUSH_NIL(pLua)                                                                                                                                     \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        lua_settop(pLua, 0);                                                                                                                                   \
        lua_pushnil(pLua);                                                                                                                                     \
        return 1;                                                                                                                                              \
    } while (0)

// Type-safe lua_tointeger wrapper
template <typename T>
inline auto LuaInt(lua_State* pLua, int idx) -> T
{
    return static_cast<T>(lua_tointeger(pLua, idx));
}
//---------------------------------------------------------------------------

#endif
