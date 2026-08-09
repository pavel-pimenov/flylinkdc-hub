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
#include "LuaBanManLib.h"
//---------------------------------------------------------------------------
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include "GlobalDataQueue.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------

static void PushBan(lua_State* pLua, BanItem* pBan)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    const int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sIP");
    if (pBan->m_sIp[0] == '\0')
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushstring(pLua, pBan->m_sIp.data());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sNick");
    if (pBan->m_sNick.empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushstring(pLua, pBan->m_sNick.c_str());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sReason");
    if (pBan->m_sReason.empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushstring(pLua, pBan->m_sReason.c_str());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sBy");
    if (pBan->m_sBy.empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushstring(pLua, pBan->m_sBy.c_str());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iExpireTime");
    !((pBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) ? lua_pushnil(pLua) : lua_pushinteger(pLua, pBan->m_tTempBanExpire);

    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bIpBan");
    ((pBan->m_ui8Bits & BanManager::IP) == BanManager::IP) ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bNickBan");
    ((pBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK) ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bFullIpBan");
    ((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static void PushRangeBan(lua_State* pLua, RangeBanItem* pRangeBan)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    const int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sIPFrom");
    lua_pushstring(pLua, pRangeBan->m_sIpFrom.data());
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sIPTo");
    lua_pushstring(pLua, pRangeBan->m_sIpTo.data());
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sReason");
    if (pRangeBan->m_sReason.empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushstring(pLua, pRangeBan->m_sReason.c_str());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sBy");
    if (pRangeBan->m_sBy.empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushstring(pLua, pRangeBan->m_sBy.c_str());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iExpireTime");
    !((pRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) ? lua_pushnil(pLua) : lua_pushinteger(pLua, pRangeBan->m_tTempBanExpire);

    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bFullIpBan");
    ((pRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static int Save(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    BanManager::m_Ptr->Save(true);

    return 0;
}
//------------------------------------------------------------------------------

static int GetBans(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    time_t acc_time;
    time(&acc_time);

    {
        auto it = BanManager::m_Ptr->m_TempBanList.begin();
        while (it != BanManager::m_Ptr->m_TempBanList.end())
        {
            BanItem* curBan = it->get();
            ++it;

            if (acc_time > curBan->m_tTempBanExpire)
            {
                BanManager::m_Ptr->Rem(curBan);
                std::unique_ptr<BanItem> guard(curBan);

                continue;
            }

            lua_pushinteger(pLua, ++i);

            PushBan(pLua, curBan);
            lua_rawset(pLua, t);
        }
    }

    for (const auto& pBan : BanManager::m_Ptr->m_PermBanList)
    {
        lua_pushinteger(pLua, ++i);

        PushBan(pLua, pBan.get());
        lua_rawset(pLua, t);
    }

    return 1;
}
//---------------------------------------------------------------------------

static int GetTempBans(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    time_t acc_time;
    time(&acc_time);

    {
        auto it = BanManager::m_Ptr->m_TempBanList.begin();
        while (it != BanManager::m_Ptr->m_TempBanList.end())
        {
            BanItem* curBan = it->get();
            ++it;

            if (acc_time > curBan->m_tTempBanExpire)
            {
                BanManager::m_Ptr->Rem(curBan);
                std::unique_ptr<BanItem> guard(curBan);

                continue;
            }

            lua_pushinteger(pLua, ++i);

            PushBan(pLua, curBan);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetPermBans(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    for (const auto& pBan : BanManager::m_Ptr->m_PermBanList)
    {
        lua_pushinteger(pLua, ++i);

        PushBan(pLua, pBan.get());
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetBan(lua_State* pLua)
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

    time_t acc_time;
    time(&acc_time);

    size_t szLen;
    const char* sValue = lua_tolstring(pLua, 1, &szLen);

    BanItem* pBan = BanManager::m_Ptr->FindNick(std::string_view(sValue, szLen));

    Hash128 ui128Hash;

    if (HashIP(sValue, ui128Hash))
    {
        lua_settop(pLua, 0);

        lua_newtable(pLua);
        const int t = lua_gettop(pLua); int i = 0;

        if (pBan)
        {
            lua_pushinteger(pLua, ++i);

            PushBan(pLua, pBan);
            lua_rawset(pLua, t);
        }

        pBan = BanManager::m_Ptr->FindIP(ui128Hash, acc_time);
        if (pBan)
        {
            lua_pushinteger(pLua, ++i);

            PushBan(pLua, pBan);
            lua_rawset(pLua, t);

            BanItem *curBan = nullptr, *nextBan = pBan->m_pHashIpTableNext;

            while (nextBan)
            {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;

                if ((((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)) && acc_time > curBan->m_tTempBanExpire)
                {
                    BanManager::m_Ptr->Rem(curBan);
                    std::unique_ptr<BanItem> guard(curBan);

                    continue;
                }

                lua_pushinteger(pLua, ++i);

                PushBan(pLua, curBan);
                lua_rawset(pLua, t);
            }
        }
        return 1;
    }

    lua_settop(pLua, 0);

    if (!pBan)
    {
        lua_pushnil(pLua);
        return 1;
    }

    PushBan(pLua, pBan);
    return 1;
}
//------------------------------------------------------------------------------

static int GetPermBan(lua_State* pLua)
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
    const char* sValue = lua_tolstring(pLua, 1, &szLen);

    BanItem* Ban = BanManager::m_Ptr->FindPermNick(std::string_view(sValue, szLen));

    Hash128 ui128Hash;

    if (HashIP(sValue, ui128Hash))
    {
        lua_settop(pLua, 0);

        lua_newtable(pLua);
        const int t = lua_gettop(pLua); int i = 0;

        if (Ban)
        {
            lua_pushinteger(pLua, ++i);

            PushBan(pLua, Ban);
            lua_rawset(pLua, t);
        }

        Ban = BanManager::m_Ptr->FindPermIP(ui128Hash);
        if (Ban)
        {
            lua_pushinteger(pLua, ++i);

            PushBan(pLua, Ban);
            lua_rawset(pLua, t);

            BanItem *curBan = nullptr, *nextBan = Ban->m_pHashIpTableNext;

            while (nextBan)
            {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;

                if (!((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
                {
                    continue;
                }

                lua_pushinteger(pLua, ++i);

                PushBan(pLua, curBan);
                lua_rawset(pLua, t);
            }
        }
        return 1;
    }

    lua_settop(pLua, 0);

    if (!Ban)
    {
        lua_pushnil(pLua);
        return 1;
    }

    PushBan(pLua, Ban);
    return 1;
}
//------------------------------------------------------------------------------

static int GetTempBan(lua_State* pLua)
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

    time_t acc_time;
    time(&acc_time);

    size_t szLen;
    const char* sValue = lua_tolstring(pLua, 1, &szLen);

    BanItem* Ban = BanManager::m_Ptr->FindTempNick(std::string_view(sValue, szLen));

    Hash128 ui128Hash;

    if (HashIP(sValue, ui128Hash))
    {
        lua_settop(pLua, 0);

        lua_newtable(pLua);
        const int t = lua_gettop(pLua); int i = 0;

        if (Ban)
        {
            lua_pushinteger(pLua, ++i);

            PushBan(pLua, Ban);
            lua_rawset(pLua, t);
        }

        Ban = BanManager::m_Ptr->FindTempIP(ui128Hash, acc_time);
        if (Ban)
        {
            lua_pushinteger(pLua, ++i);

            PushBan(pLua, Ban);
            lua_rawset(pLua, t);

            BanItem *curBan = nullptr, *nextBan = Ban->m_pHashIpTableNext;

            while (nextBan)
            {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;

                if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
                {
                    if (acc_time > curBan->m_tTempBanExpire)
                    {
                        BanManager::m_Ptr->Rem(curBan);
                        std::unique_ptr<BanItem> guard(curBan);

                        continue;
                    }

                    lua_pushinteger(pLua, ++i);

                    PushBan(pLua, curBan);
                    lua_rawset(pLua, t);
                }
            }
        }
        return 1;
    }

    lua_settop(pLua, 0);

    if (!Ban)
    {
        lua_pushnil(pLua);
        return 1;
    }

    PushBan(pLua, Ban);
    return 1;
}
//------------------------------------------------------------------------------

static int GetRangeBans(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    time_t acc_time;
    time(&acc_time);

    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    auto it = rangeList.begin();
    while (it != rangeList.end())
    {
        RangeBanItem* curBan = it->get();

        if ((((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP)) && acc_time > curBan->m_tTempBanExpire)
        {
            it = rangeList.erase(it);

            continue;
        }

        lua_pushinteger(pLua, ++i);

        PushRangeBan(pLua, curBan);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetTempRangeBans(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    time_t acc_time;
    time(&acc_time);

    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    auto it = rangeList.begin();
    while (it != rangeList.end())
    {
        RangeBanItem* curBan = it->get();

        if (!((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
        {
            ++it;
            continue;
        }

        if (acc_time > curBan->m_tTempBanExpire)
        {
            it = rangeList.erase(it);

            continue;
        }

        lua_pushinteger(pLua, ++i);

        PushRangeBan(pLua, curBan);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetPermRangeBans(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    for (const auto& pCurBan : BanManager::m_Ptr->m_RangeBanList)
    {
        RangeBanItem* curBan = pCurBan.get();

        if (!((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
        {
            continue;
        }

        lua_pushinteger(pLua, ++i);

        PushRangeBan(pLua, curBan);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetRangeBan(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szFromLen, szToLen;
    const char* sFrom = lua_tolstring(pLua, 1, &szFromLen);
    const char* sTo = lua_tolstring(pLua, 2, &szToLen);

    Hash128 ui128FromHash, ui128ToHash;

    if (szFromLen == 0 || szToLen == 0 || !HashIP(sFrom, ui128FromHash) || !HashIP(sTo, ui128ToHash) || memcmp(ui128ToHash, ui128FromHash, 16) <= 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    time_t acc_time;
    time(&acc_time);

    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    auto it = rangeList.begin();
    while (it != rangeList.end())
    {
        RangeBanItem* cur = it->get();

        if (memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0)
        {
            // PPK ... check if it's temban and then if it's expired
            if (((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
            {
                if (acc_time >= cur->m_tTempBanExpire)
                {
                    it = rangeList.erase(it);

                    continue;
                }
            }
            PushRangeBan(pLua, cur);
            return 1;
        }

        ++it;
    }

    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetRangePermBan(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szFromLen, szToLen;
    const char* sFrom = lua_tolstring(pLua, 1, &szFromLen);
    const char* sTo = lua_tolstring(pLua, 2, &szToLen);

    Hash128 ui128FromHash, ui128ToHash;

    if (szFromLen == 0 || szToLen == 0 || !HashIP(sFrom, ui128FromHash) || !HashIP(sTo, ui128ToHash) || memcmp(ui128ToHash, ui128FromHash, 16) <= 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    auto it = rangeList.begin();
    while (it != rangeList.end())
    {
        RangeBanItem* cur = it->get();

        if (memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0)
        {
            if (((cur->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
            {
                PushRangeBan(pLua, cur);
                return 1;
            }
        }

        ++it;
    }

    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetRangeTempBan(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szFromLen, szToLen;
    const char* sFrom = lua_tolstring(pLua, 1, &szFromLen);
    const char* sTo = lua_tolstring(pLua, 2, &szToLen);

    Hash128 ui128FromHash, ui128ToHash;

    if (szFromLen == 0 || szToLen == 0 || !HashIP(sFrom, ui128FromHash) || !HashIP(sTo, ui128ToHash) || memcmp(ui128ToHash, ui128FromHash, 16) <= 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    time_t acc_time;
    time(&acc_time);

    auto& rangeList = BanManager::m_Ptr->m_RangeBanList;
    auto it = rangeList.begin();
    while (it != rangeList.end())
    {
        RangeBanItem* cur = it->get();

        if (memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0)
        {
            // PPK ... check if it's temban and then if it's expired
            if (((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
            {
                if (acc_time >= cur->m_tTempBanExpire)
                {
                    it = rangeList.erase(it);

                    continue;
                }

                PushRangeBan(pLua, cur);
                return 1;
            }
        }

        ++it;
    }

    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int UnbanByString(lua_State* pLua, bool (BanManager::*pUnbanMethod)(const char*))
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);
    LUA_CHECK_TYPE(pLua, 1, LUA_TSTRING);

    size_t szLen;
    const char* sWhat = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0)
    {
        LUA_PUSH_NIL(pLua);
    }

    if (!(*BanManager::m_Ptr.*pUnbanMethod)(sWhat))
    {
        LUA_PUSH_NIL(pLua);
    }
    else
    {
        LUA_PUSH_BOOL(pLua, true);
    }

    return 1;
}

static int Unban(lua_State* pLua)
{
    return UnbanByString(pLua, &BanManager::Unban);
}
static int UnbanPerm(lua_State* pLua)
{
    return UnbanByString(pLua, &BanManager::PermUnban);
}
static int UnbanTemp(lua_State* pLua)
{
    return UnbanByString(pLua, &BanManager::TempUnban);
}
//------------------------------------------------------------------------------

static int UnbanAllByMethod(lua_State* pLua, void (BanManager::*pRemoveMethod)(const uint8_t*))
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 1);
    LUA_CHECK_TYPE(pLua, 1, LUA_TSTRING);

    size_t szLen;
    const char* sIP = lua_tolstring(pLua, 1, &szLen);

    Hash128 ui128Hash;

    if (szLen == 0 || !HashIP(sIP, ui128Hash))
    {
        lua_settop(pLua, 0);
        return 0;
    }

    lua_settop(pLua, 0);

    (*BanManager::m_Ptr.*pRemoveMethod)(ui128Hash);

    return 0;
}

static int UnbanAll(lua_State* pLua)
{
    return UnbanAllByMethod(pLua, &BanManager::RemoveAllIP);
}
static int UnbanPermAll(lua_State* pLua)
{
    return UnbanAllByMethod(pLua, &BanManager::RemovePermAllIP);
}
static int UnbanTempAll(lua_State* pLua)
{
    return UnbanAllByMethod(pLua, &BanManager::RemoveTempAllIP);
}
//------------------------------------------------------------------------------

static int RangeUnbanByType(lua_State* pLua, uint8_t ui8Type)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);
    LUA_CHECK_TYPE(pLua, 1, LUA_TSTRING);
    LUA_CHECK_TYPE(pLua, 2, LUA_TSTRING);

    size_t szFromIpLen, szToIpLen;
    const char* sFromIp = lua_tolstring(pLua, 1, &szFromIpLen);
    const char* sToIp = lua_tolstring(pLua, 2, &szToIpLen);

    Hash128 ui128FromHash, ui128ToHash;

    if (szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIp, ui128FromHash) && HashIP(sToIp, ui128ToHash) && memcmp(ui128ToHash, ui128FromHash, 16) > 0 &&
        BanManager::m_Ptr->RangeUnban(ui128FromHash, ui128ToHash, ui8Type))
    {
        LUA_PUSH_BOOL(pLua, true);
    }

    LUA_PUSH_NIL(pLua);
}

static int RangeUnban(lua_State* pLua)
{
    return RangeUnbanByType(pLua, 0);
}
static int RangeUnbanPerm(lua_State* pLua)
{
    return RangeUnbanByType(pLua, BanManager::PERM);
}
static int RangeUnbanTemp(lua_State* pLua)
{
    return RangeUnbanByType(pLua, BanManager::TEMP);
}
//------------------------------------------------------------------------------

static int ClearBans(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    BanManager::m_Ptr->ClearTemp();
    BanManager::m_Ptr->ClearPerm();

    return 0;
}
//------------------------------------------------------------------------------

static int ClearSingleBanType(lua_State* pLua, void (BanManager::*pClearMethod)())
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    (*BanManager::m_Ptr.*pClearMethod)();

    return 0;
}

static int ClearPermBans(lua_State* pLua)
{
    return ClearSingleBanType(pLua, &BanManager::ClearPerm);
}
static int ClearTempBans(lua_State* pLua)
{
    return ClearSingleBanType(pLua, &BanManager::ClearTemp);
}
static int ClearRangeBans(lua_State* pLua)
{
    return ClearSingleBanType(pLua, &BanManager::ClearRange);
}
static int ClearRangePermBans(lua_State* pLua)
{
    return ClearSingleBanType(pLua, &BanManager::ClearPermRange);
}
static int ClearRangeTempBans(lua_State* pLua)
{
    return ClearSingleBanType(pLua, &BanManager::ClearTempRange);
}
//------------------------------------------------------------------------------

static int Ban(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 4);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User* u = ScriptGetUser(pLua, 4, "Ban");

    if (!u)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 2, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 3, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    const bool bFull = lua_toboolean(pLua, 4) != 0;

    BanManager::m_Ptr->Ban(u, sReason, sBy, bFull);

    UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) banned by script.", u->m_sNick.c_str(), u->m_sIP.data());

    u->Close();

    lua_settop(pLua, 0);
    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int BanIP(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 4);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szIpLen;
    const char* sIP = lua_tolstring(pLua, 1, &szIpLen);
    if (szIpLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 2, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 3, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    const bool bFull = lua_toboolean(pLua, 4) != 0;

    if (BanManager::m_Ptr->BanIp(nullptr, sIP, sReason, sBy, bFull) == 0)
    {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
    }
    else
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int BanNick(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
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

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 2, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 3, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    User* curUser = HashManager::m_Ptr->FindUser(std::string_view(sNick, szNickLen));
    if (curUser)
    {
        if (BanManager::m_Ptr->NickBan(curUser, nullptr, sReason, sBy))
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) nickbanned by script.", curUser->m_sNick.c_str(), curUser->m_sIP.data());

            curUser->Close();
            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        }
        else
        {
            curUser->Close();
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    }
    else
    {
        if (BanManager::m_Ptr->NickBan(nullptr, sNick, sReason, sBy))
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick %s nickbanned by script.", sNick);

            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        }
        else
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int TempBan(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 5);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING ||
        lua_type(pLua, 5) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User* u = ScriptGetUser(pLua, 5, "TempBan");

    if (!u)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto iMinutes = LuaInt<uint32_t>(pLua, 2);

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 3, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 4, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    const bool bFull = lua_toboolean(pLua, 5) != 0;

    BanManager::m_Ptr->TempBan(u, sReason, sBy, iMinutes, 0, bFull);

    UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) tempbanned by script.", u->m_sNick.c_str(), u->m_sIP.data());

    u->Close();

    lua_settop(pLua, 0);
    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int TempBanIP(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 5);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING ||
        lua_type(pLua, 5) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szIpLen;
    const char* sIP = lua_tolstring(pLua, 1, &szIpLen);
    if (szIpLen == 0)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto i32Minutes = LuaInt<uint32_t>(pLua, 2);

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 3, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 4, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    const bool bFull = lua_toboolean(pLua, 5) != 0;

    if (BanManager::m_Ptr->TempBanIp(nullptr, sIP, sReason, sBy, i32Minutes, 0, bFull) == 0)
    {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
    }
    else
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int TempBanNick(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 4);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
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

    const auto i32Minutes = LuaInt<uint32_t>(pLua, 2);

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 3, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 4, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    User* curUser = HashManager::m_Ptr->FindUser(std::string_view(sNick, szNickLen));
    if (curUser)
    {
        if (BanManager::m_Ptr->NickTempBan(curUser, nullptr, sReason, sBy, i32Minutes, 0))
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) nickbanned by script.", curUser->m_sNick.c_str(), curUser->m_sIP.data());

            curUser->Close();
            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        }
        else
        {
            curUser->Close();
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    }
    else
    {
        if (BanManager::m_Ptr->NickTempBan(nullptr, sNick, sReason, sBy, i32Minutes, 0))
        {
            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick %s nickbanned by script.", sNick);

            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        }
        else
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int RangeBan(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 5);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING ||
        lua_type(pLua, 5) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szFromIpLen;
    const char* sFromIP = lua_tolstring(pLua, 1, &szFromIpLen);

    size_t szToIpLen;
    const char* sToIP = lua_tolstring(pLua, 2, &szToIpLen);

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 3, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 4, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    const bool bFull = lua_toboolean(pLua, 5) != 0;

    Hash128 ui128FromHash, ui128ToHash;

    if (szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIP, ui128FromHash) && HashIP(sToIP, ui128ToHash) && memcmp(ui128ToHash, ui128FromHash, 16) > 0 &&
        BanManager::m_Ptr->RangeBan(sFromIP, ui128FromHash, sToIP, ui128ToHash, sReason, sBy, bFull))
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

static int RangeTempBan(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 6);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TNUMBER || lua_type(pLua, 4) != LUA_TSTRING ||
        lua_type(pLua, 5) != LUA_TSTRING || lua_type(pLua, 6) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TNUMBER);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TSTRING);
        luaL_checktype(pLua, 6, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szFromIpLen;
    const char* sFromIP = lua_tolstring(pLua, 1, &szFromIpLen);

    size_t szToIpLen;
    const char* sToIP = lua_tolstring(pLua, 2, &szToIpLen);

    const auto i32Minutes = LuaInt<uint32_t>(pLua, 3);

    size_t szReasonLen;
    const char* sReason = lua_tolstring(pLua, 4, &szReasonLen);
    if (szReasonLen == 0)
    {
        sReason = nullptr;
    }

    size_t szByLen;
    const char* sBy = lua_tolstring(pLua, 5, &szByLen);
    if (szByLen == 0)
    {
        sBy = nullptr;
    }

    const bool bFull = lua_toboolean(pLua, 6) != 0;

    Hash128 ui128FromHash, ui128ToHash;

    if (szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIP, ui128FromHash) && HashIP(sToIP, ui128ToHash) && memcmp(ui128ToHash, ui128FromHash, 16) > 0 &&
        BanManager::m_Ptr->RangeTempBan(sFromIP, ui128FromHash, sToIP, ui128ToHash, sReason, sBy, i32Minutes, 0, bFull))
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

static const luaL_Reg BanManRegs[] = {{"Save", Save}, // NOLINT(modernize-avoid-c-arrays)
                                      {"GetBans", GetBans},
                                      {"GetTempBans", GetTempBans},
                                      {"GetPermBans", GetPermBans},
                                      {"GetBan", GetBan},
                                      {"GetPermBan", GetPermBan},
                                      {"GetTempBan", GetTempBan},
                                      {"GetRangeBans", GetRangeBans},
                                      {"GetTempRangeBans", GetTempRangeBans},
                                      {"GetPermRangeBans", GetPermRangeBans},
                                      {"GetRangeBan", GetRangeBan},
                                      {"GetRangePermBan", GetRangePermBan},
                                      {"GetRangeTempBan", GetRangeTempBan},
                                      {"Unban", Unban},
                                      {"UnbanPerm", UnbanPerm},
                                      {"UnbanTemp", UnbanTemp},
                                      {"UnbanAll", UnbanAll},
                                      {"UnbanPermAll", UnbanPermAll},
                                      {"UnbanTempAll", UnbanTempAll},
                                      {"RangeUnban", RangeUnban},
                                      {"RangeUnbanPerm", RangeUnbanPerm},
                                      {"RangeUnbanTemp", RangeUnbanTemp},
                                      {"ClearBans", ClearBans},
                                      {"ClearPermBans", ClearPermBans},
                                      {"ClearTempBans", ClearTempBans},
                                      {"ClearRangeBans", ClearRangeBans},
                                      {"ClearRangePermBans", ClearRangePermBans},
                                      {"ClearRangeTempBans", ClearRangeTempBans},
                                      {"Ban", Ban},
                                      {"BanIP", BanIP},
                                      {"BanNick", BanNick},
                                      {"TempBan", TempBan},
                                      {"TempBanIP", TempBanIP},
                                      {"TempBanNick", TempBanNick},
                                      {"RangeBan", RangeBan},
                                      {"RangeTempBan", RangeTempBan},
                                      {nullptr, nullptr}};
//---------------------------------------------------------------------------

int RegBanMan(lua_State* pLua)
{
    luaL_newlib(pLua, BanManRegs);
    return 1;
}
//---------------------------------------------------------------------------
