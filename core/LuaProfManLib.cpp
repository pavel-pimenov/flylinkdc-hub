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
#include "LuaProfManLib.h"
//---------------------------------------------------------------------------
#include "ProfileManager.h"
#include "GlobalDataQueue.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void PushProfilePermissions(lua_State* pLua, const uint16_t iProfile)
{
    const ProfileItem* Prof = ProfileManager::m_Ptr->m_vpProfilesTable[iProfile].get();

    lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    const int t = lua_gettop(pLua);

    struct PermEntry
    {
        const char* sName;
        uint8_t m_ui8Idx;
    };
    static const PermEntry g_PermEntries[] = { // NOLINT(modernize-avoid-c-arrays)
        {"bIsOP", ProfileManager::HASKEYICON},
        {"bNoDefloodGetNickList", ProfileManager::NODEFLOODGETNICKLIST},
        {"bNoDefloodNMyINFO", ProfileManager::NODEFLOODMYINFO},
        {"bNoDefloodSearch", ProfileManager::NODEFLOODSEARCH},
        {"bNoDefloodPM", ProfileManager::NODEFLOODPM},
        {"bNoDefloodMainChat", ProfileManager::NODEFLOODMAINCHAT},
        {"bMassMsg", ProfileManager::MASSMSG},
        {"bTopic", ProfileManager::TOPIC},
        {"bTempBan", ProfileManager::TEMP_BAN},
        {"bTempUnban", ProfileManager::TEMP_UNBAN},
        {"bRefreshTxt", ProfileManager::REFRESHTXT},
        {"bNoTagCheck", ProfileManager::NOTAGCHECK},
        {"bDelRegUser", ProfileManager::DELREGUSER},
        {"bAddRegUser", ProfileManager::ADDREGUSER},
        {"bNoChatLimits", ProfileManager::NOCHATLIMITS},
        {"bNoMaxHubCheck", ProfileManager::NOMAXHUBCHECK},
        {"bNoSlotHubRatio", ProfileManager::NOSLOTHUBRATIO},
        {"bNoSlotCheck", ProfileManager::NOSLOTCHECK},
        {"bNoShareLimit", ProfileManager::NOSHARELIMIT},
        {"bClrPermBan", ProfileManager::CLRPERMBAN},
        {"bClrTempBan", ProfileManager::CLRTEMPBAN},
        {"bGetInfo", ProfileManager::GETINFO},
        {"bGetBans", ProfileManager::GETBANLIST},
        {"bRestartScripts", ProfileManager::RSTSCRIPTS},
        {"bRestartHub", ProfileManager::RSTHUB},
        {"bTempOP", ProfileManager::TEMPOP},
        {"bGag", ProfileManager::GAG},
        {"bRedirect", ProfileManager::REDIRECT},
        {"bBan", ProfileManager::BAN},
        {"bUnban", ProfileManager::UNBAN},
        {"bKick", ProfileManager::KICK},
        {"bDrop", ProfileManager::DROP},
        {"bEnterFullHub", ProfileManager::ENTERFULLHUB},
        {"bEnterIfIPBan", ProfileManager::ENTERIFIPBAN},
        {"bAllowedOPChat", ProfileManager::ALLOWEDOPCHAT},
        {"bSendFullMyinfos", ProfileManager::SENDFULLMYINFOS},
        {"bSendAllUserIP", ProfileManager::SENDALLUSERIP},
        {"bRangeBan", ProfileManager::RANGE_BAN},
        {"bRangeUnban", ProfileManager::RANGE_UNBAN},
        {"bRangeTempBan", ProfileManager::RANGE_TBAN},
        {"bRangeTempUnban", ProfileManager::RANGE_TUNBAN},
        {"bGetRangeBans", ProfileManager::GET_RANGE_BANS},
        {"bClearRangePermBans", ProfileManager::CLR_RANGE_BANS},
        {"bClearRangeTempBans", ProfileManager::CLR_RANGE_TBANS},
        {"bNoIpCheck", ProfileManager::NOIPCHECK},
        {"bClose", ProfileManager::CLOSE},
        {"bNoSearchLimits", ProfileManager::NOSEARCHLIMITS},
        {"bNoDefloodCTM", ProfileManager::NODEFLOODCTM},
        {"bNoDefloodRCTM", ProfileManager::NODEFLOODRCTM},
        {"bNoDefloodSR", ProfileManager::NODEFLOODSR},
        {"bNoDefloodRecv", ProfileManager::NODEFLOODRECV},
        {"bNoChatInterval", ProfileManager::NOCHATINTERVAL},
        {"bNoPMInterval", ProfileManager::NOPMINTERVAL},
        {"bNoSearchInterval", ProfileManager::NOSEARCHINTERVAL},
        {"bNoMaxUsersSameIP", ProfileManager::NOUSRSAMEIP},
        {"bNoReConnTime", ProfileManager::NORECONNTIME},
    };

    for (const auto& e : g_PermEntries)
    {
        lua_pushstring(pLua, e.sName);
        Prof->m_bPermissions[e.m_ui8Idx] ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
        lua_rawset(pLua, t);
    }
}
//------------------------------------------------------------------------------

static void PushProfile(lua_State* pLua, const uint16_t iProfile)
{
    lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    const int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sProfileName");
    lua_pushstring(pLua, ProfileManager::m_Ptr->m_vpProfilesTable[iProfile]->m_sName.c_str());
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iProfileNumber");
    lua_pushinteger(pLua, iProfile);

    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "tProfilePermissions");
    PushProfilePermissions(pLua, iProfile);
    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static int AddProfile(lua_State* pLua)
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
    const char* sProfileName = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0 || szLen > 64)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const int32_t idx = ProfileManager::m_Ptr->AddProfile(sProfileName);
    if (idx == -1 || idx == -2)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    lua_pushinteger(pLua, idx);

    return 1;
}
//------------------------------------------------------------------------------

static int RemoveProfile(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) == LUA_TSTRING)
    {
        size_t szLen;
        const char* sProfileName = lua_tolstring(pLua, 1, &szLen);

        if (szLen == 0)
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        const int32_t result = ProfileManager::m_Ptr->RemoveProfileByName(sProfileName);
        if (result != 1)
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    }
    else if (lua_type(pLua, 1) == LUA_TNUMBER)
    {
        const auto idx = LuaInt<uint16_t>(pLua, 1);

        lua_settop(pLua, 0);

        // if the requested index is out of bounds return nil
        if (idx >= ProfileManager::m_Ptr->m_ui16ProfileCount)
        {
            lua_pushnil(pLua);
            return 1;
        }

        if (!ProfileManager::m_Ptr->RemoveProfile(idx))
        {
            lua_pushnil(pLua);
            return 1;
        }

        lua_pushboolean(pLua, 1);
        return 1;
    }

    luaL_error(pLua, "bad argument #1 to 'RemoveProfile' (string or number expected, got %d)", lua_typename(pLua, lua_type(pLua, 1)));
    lua_settop(pLua, 0);
    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveDown(lua_State* pLua)
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

    // if the requested index is out of bounds return nil
    if (iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount - 1)
    {
        lua_pushnil(pLua);
        return 1;
    }

    ProfileManager::m_Ptr->MoveProfileDown(iProfile);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveUp(lua_State* pLua)
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

    // if the requested index is out of bounds return nil
    if (iProfile == 0 || iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount)
    {
        lua_pushnil(pLua);
        return 1;
    }

    ProfileManager::m_Ptr->MoveProfileUp(iProfile);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetProfile(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) == LUA_TSTRING)
    {
        size_t szLen;
        const char* profName = lua_tolstring(pLua, 1, &szLen);

        if (szLen == 0)
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        const auto idx = ProfileManager::m_Ptr->GetProfileIndex(profName);

        lua_settop(pLua, 0);

        if (!idx)
        {
            lua_pushnil(pLua);
            return 1;
        }

        PushProfile(pLua, *idx);
        return 1;
    }
    else if (lua_type(pLua, 1) == LUA_TNUMBER)
    {
        const auto idx = LuaInt<uint16_t>(pLua, 1);

        lua_settop(pLua, 0);

        // if the requested index is out of bounds return nil
        if (idx >= ProfileManager::m_Ptr->m_ui16ProfileCount)
        {
            lua_pushnil(pLua);
            return 1;
        }

        PushProfile(pLua, idx);
        return 1;
    }

    luaL_error(pLua, "bad argument #1 to 'GetProfile' (string or number expected, got %d)", lua_typename(pLua, lua_type(pLua, 1)));
    lua_settop(pLua, 0);
    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetProfiles(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);
    const int t = lua_gettop(pLua);

    for (uint16_t ui16i = 0; ui16i < ProfileManager::m_Ptr->m_ui16ProfileCount; ui16i++)
    {
        lua_pushinteger(pLua, (ui16i + 1));

        PushProfile(pLua, ui16i);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetProfilePermission(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto iProfile = LuaInt<uint16_t>(pLua, 1);
    const auto szId = LuaInt<size_t>(pLua, 2);

    lua_settop(pLua, 0);

    if (iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount)
    {
        lua_pushnil(pLua);
        return 1;
    }

    if (szId > ProfileManager::NORECONNTIME)
    {
        luaL_error(pLua, "bad argument #2 to 'GetProfilePermission' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }

    ProfileManager::m_Ptr->m_vpProfilesTable[iProfile]->m_bPermissions[szId] ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);

    return 1;
}
//------------------------------------------------------------------------------

static int GetProfilePermissions(lua_State* pLua)
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

    // if the requested index is out of bounds return nil
    if (iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount)
    {
        lua_pushnil(pLua);
        return 1;
    }

    PushProfilePermissions(pLua, iProfile);

    return 1;
}
//------------------------------------------------------------------------------

static int SetProfileName(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto iProfile = LuaInt<uint16_t>(pLua, 1);

    lua_settop(pLua, 0);

    if (iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount)
    {
        lua_pushnil(pLua);
        return 1;
    }

    size_t szLen;
    const char* sName = lua_tolstring(pLua, 2, &szLen);

    if (szLen == 0 || szLen > 64)
    {
        lua_pushnil(pLua);
        return 1;
    }

    ProfileManager::m_Ptr->ChangeProfileName(iProfile, sName, szLen);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int SetProfilePermission(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto iProfile = LuaInt<uint16_t>(pLua, 1);
    const auto szId = LuaInt<size_t>(pLua, 2);

    const bool bValue = lua_toboolean(pLua, 3) != 0;

    lua_settop(pLua, 0);

    if (iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount)
    {
        lua_pushnil(pLua);
        return 1;
    }

    if (szId > ProfileManager::NORECONNTIME)
    {
        luaL_error(pLua, "bad argument #2 to 'SetProfilePermission' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }

    ProfileManager::m_Ptr->ChangeProfilePermission(iProfile, szId, bValue);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Save(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    ProfileManager::m_Ptr->SaveProfiles();

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg ProfManRegs[] = {{"AddProfile", AddProfile}, // NOLINT(modernize-avoid-c-arrays)
                                       {"RemoveProfile", RemoveProfile},
                                       {"MoveDown", MoveDown},
                                       {"MoveUp", MoveUp},
                                       {"GetProfile", GetProfile},
                                       {"GetProfiles", GetProfiles},
                                       {"GetProfilePermission", GetProfilePermission},
                                       {"GetProfilePermissions", GetProfilePermissions},
                                       {"SetProfileName", SetProfileName},
                                       {"SetProfilePermission", SetProfilePermission},
                                       {"Save", Save},
                                       {nullptr, nullptr}};
//---------------------------------------------------------------------------

int RegProfMan(lua_State* pLua)
{
    luaL_newlib(pLua, ProfManRegs);
    return 1;
}
//---------------------------------------------------------------------------
