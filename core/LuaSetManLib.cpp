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
#include "LuaSetManLib.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "SettingManager.h"
#include "utility.h"
#include "GlobalDataQueue.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static int Save(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    ScriptManager::m_Ptr->SaveScripts();

    SettingManager::m_Ptr->Save();

    return 0;
}
//------------------------------------------------------------------------------

static int GetMOTD(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    if (!SettingManager::m_Ptr->m_sMOTD.empty())
    {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sMOTD.c_str(), SettingManager::m_Ptr->m_sMOTD.size());
    }
    else
    {
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int SetMOTD(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen = 0;
    const char* sTxt = lua_tolstring(pLua, 1, &szLen);

    SettingManager::m_Ptr->SetMOTD(std::string_view(sTxt, szLen));

    SettingManager::m_Ptr->UpdateMOTD();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetBool(lua_State* pLua)
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

    const auto szId = LuaInt<size_t>(pLua, 1);

    lua_settop(pLua, 0);

    if (szId >= std::to_underlying(SetBoolIds::SETBOOL_IDS_END))
    {
        luaL_error(pLua, "bad argument #1 to 'GetBool' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }

    SettingManager::m_Ptr->m_bBools[szId] ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);

    return 1;
}
//------------------------------------------------------------------------------

static int SetBool(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        return 0;
    }

    const auto szId = LuaInt<size_t>(pLua, 1);

    const bool bValue = (lua_toboolean(pLua, 2) != 0);

    lua_settop(pLua, 0);

    if (szId >= std::to_underlying(SetBoolIds::SETBOOL_IDS_END))
    {
        luaL_error(pLua, "bad argument #1 to 'SetBool' (it's not valid id)");
        return 0;
    }

    if (szId == std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING) && !bValue)
    {
        EventQueue::m_Ptr->AddNormal(EventQueue::EventType::STOP_SCRIPTING, nullptr);
        return 0;
    }

    if (szId == std::to_underlying(SetBoolIds::SETBOOL_HASH_PASSWORDS) && bValue)
    {
        RegManager::m_Ptr->HashPasswords();
    }

    SettingManager::m_Ptr->SetBool(szId, bValue);

    return 0;
}
//------------------------------------------------------------------------------

static int GetNumber(lua_State* pLua)
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

    const auto szId = LuaInt<size_t>(pLua, 1);

    lua_settop(pLua, 0);

    if (szId >= (std::to_underlying(SetShortIds::SETSHORT_IDS_END) - 1))
    {
        luaL_error(pLua, "bad argument #1 to 'GetNumber' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }

    lua_pushinteger(pLua, SettingManager::m_Ptr->m_i16Shorts[szId]);

    return 1;
}
//------------------------------------------------------------------------------

static int SetNumber(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        lua_settop(pLua, 0);
        return 0;
    }

    const auto szId = LuaInt<size_t>(pLua, 1);

    const auto iValue = LuaInt<int16_t>(pLua, 2);

    lua_settop(pLua, 0);

    if (szId >= (std::to_underlying(SetShortIds::SETSHORT_IDS_END) - 1))
    {
        luaL_error(pLua, "bad argument #1 to 'SetNumber' (it's not valid id)");
        return 0;
    }

    SettingManager::m_Ptr->SetShort(szId, iValue);

    return 0;
}
//------------------------------------------------------------------------------

static int GetString(lua_State* pLua)
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

    const auto szId = LuaInt<size_t>(pLua, 1);

    lua_settop(pLua, 0);

    if (szId >= std::to_underlying(SetTxtIds::SETTXT_IDS_END))
    {
        luaL_error(pLua, "bad argument #1 to 'GetString' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }

    if (!SettingManager::m_Ptr->m_sTexts[szId].empty())
    {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[szId].c_str(), SettingManager::m_Ptr->m_sTexts[szId].size());
    }
    else
    {
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int SetString(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    const auto szId = LuaInt<size_t>(pLua, 1);

    if (szId >= std::to_underlying(SetTxtIds::SETTXT_IDS_END))
    {
        luaL_error(pLua, "bad argument #1 to 'SetString' (it's not valid id)");
        return 0;
    }

    size_t szLen;
    const char* sValue = lua_tolstring(pLua, 2, &szLen);

    SettingManager::m_Ptr->SetText(szId, std::string_view(sValue, szLen));

    return 0;
}
//------------------------------------------------------------------------------

static int GetMinShare(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    lua_pushinteger(pLua, static_cast<lua_Integer>(SettingManager::m_Ptr->m_ui64MinShare));

    return 1;
}
//------------------------------------------------------------------------------

static int SetMinShare(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    const int n = lua_gettop(pLua);

    if (n == 2)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            luaL_checktype(pLua, 2, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

        SettingManager::m_Ptr->m_bUpdateLocked = true;

        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT), LuaInt<int16_t>(pLua, 1));
        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_UNITS), LuaInt<int16_t>(pLua, 2));

        SettingManager::m_Ptr->m_bUpdateLocked = false;
    }
    else if (n == 1)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

        double dBytes = lua_tonumber(pLua, 1);
        uint16_t iter = 0;
        for (; dBytes > 1024; iter++)
        {
            dBytes /= 1024;
        }

        SettingManager::m_Ptr->m_bUpdateLocked = true;

        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT), static_cast<int16_t>(dBytes));
        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_UNITS), static_cast<int16_t>(iter));

        SettingManager::m_Ptr->m_bUpdateLocked = false;
    }
    else
    {
        luaL_error(pLua, "bad argument count to 'SetMinShare' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    SettingManager::m_Ptr->UpdateMinShare();
    SettingManager::m_Ptr->UpdateShareLimitMessage();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetMaxShare(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    lua_pushinteger(pLua, static_cast<lua_Integer>(SettingManager::m_Ptr->m_ui64MaxShare));

    return 1;
}
//------------------------------------------------------------------------------

static int SetMaxShare(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    const int n = lua_gettop(pLua);

    if (n == 2)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            luaL_checktype(pLua, 2, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

        SettingManager::m_Ptr->m_bUpdateLocked = true;

        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT), LuaInt<int16_t>(pLua, 1));
        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_UNITS), LuaInt<int16_t>(pLua, 2));

        SettingManager::m_Ptr->m_bUpdateLocked = false;
    }
    else if (n == 1)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

        auto dBytes = static_cast<double>(lua_tonumber(pLua, 1));
        uint16_t iter = 0;
        for (; dBytes > 1024; iter++)
        {
            dBytes /= 1024;
        }

        SettingManager::m_Ptr->m_bUpdateLocked = true;

        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT), static_cast<int16_t>(dBytes));
        SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_UNITS), static_cast<int16_t>(iter));

        SettingManager::m_Ptr->m_bUpdateLocked = false;
    }
    else
    {
        luaL_error(pLua, "bad argument count to 'SetMaxShare' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    SettingManager::m_Ptr->UpdateMaxShare();
    SettingManager::m_Ptr->UpdateShareLimitMessage();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SetHubSlotRatio(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        lua_settop(pLua, 0);
        return 0;
    }

    SettingManager::m_Ptr->m_bUpdateLocked = true;

    SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_HUBS), LuaInt<int16_t>(pLua, 1));
    SettingManager::m_Ptr->SetShort(std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_SLOTS), LuaInt<int16_t>(pLua, 2));

    SettingManager::m_Ptr->m_bUpdateLocked = false;

    SettingManager::m_Ptr->UpdateHubSlotRatioMessage();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetOpChat(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    lua_newtable(pLua);
    const int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sNick");
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushlstring(pLua,
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str(),
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].size());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sDescription");
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)].empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushlstring(pLua,
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)].c_str(),
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)].size());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sEmail");
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)].empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushlstring(pLua,
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)].c_str(),
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)].size());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bEnabled");
    SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    return 1;
}
//------------------------------------------------------------------------------

static int SetOpChat(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 4);

    if (lua_type(pLua, 1) != LUA_TBOOLEAN || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TBOOLEAN);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    const char *NewBotName, *NewBotDescription, *NewBotEmail;
    size_t szNameLen, szDescrLen, szEmailLen;

    NewBotName = lua_tolstring(pLua, 2, &szNameLen);
    NewBotDescription = lua_tolstring(pLua, 3, &szDescrLen);
    NewBotEmail = lua_tolstring(pLua, 4, &szEmailLen);

    if (szNameLen == 0 || szNameLen > 64 || szDescrLen > 64 || szEmailLen > 64 || contains_any(std::string_view(NewBotName, szNameLen), " $|") ||
        contains_any(std::string_view(NewBotDescription, szDescrLen), "$|") || contains_any(std::string_view(NewBotEmail, szEmailLen), "$|") ||
        HashManager::m_Ptr->FindUser(std::string_view(NewBotName, szNameLen)))
    {
        lua_settop(pLua, 0);
        return 0;
    }

    const bool bBotHaveNewNick = (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)] != NewBotName);
    const bool bEnableBot = (lua_toboolean(pLua, 1) != 0);

    bool bRegStateChange = false, bDescriptionChange = false, bEmailChange = false;

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] != bEnableBot)
    {
        bRegStateChange = true;
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)])
    {
        SettingManager::m_Ptr->DisableOpChat((bBotHaveNewNick || !bEnableBot));
    }

    SettingManager::m_Ptr->m_bUpdateLocked = true;

    SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT), bEnableBot);

    if (bBotHaveNewNick)
    {
        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK), NewBotName);
    }

    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)].empty() ||
        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)] != NewBotDescription)
    {
        if (szDescrLen != static_cast<size_t>(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)].empty() ? 0 : -1))
        {
            bDescriptionChange = true;
        }

        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION), NewBotDescription);
    }

    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)].empty() || SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)] != NewBotEmail)
    {
        if (szEmailLen != static_cast<size_t>(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)].empty() ? 0 : -1))
        {
            bEmailChange = true;
        }

        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL), NewBotEmail);
    }

    SettingManager::m_Ptr->m_bUpdateLocked = false;

    SettingManager::m_Ptr->UpdateBotsSameNick();

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] && (bRegStateChange || bBotHaveNewNick || bDescriptionChange || bEmailChange))
    {
        SettingManager::m_Ptr->UpdateOpChat((bBotHaveNewNick || bRegStateChange));
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetHubBot(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    lua_newtable(pLua);
    const int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sNick");
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushlstring(
            pLua, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str(), SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].size());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sDescription");
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)].empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushlstring(pLua,
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)].c_str(),
                        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)].size());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sEmail");
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)].empty())
    {
        lua_pushnil(pLua);
    }
    else
    {
        lua_pushlstring(
            pLua, SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)].c_str(), SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)].size());
    }
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bEnabled");
    SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bUsedAsHubSecAlias");
    SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_USE_BOT_NICK_AS_HUB_SEC)] ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    return 1;
}
//------------------------------------------------------------------------------

static int SetHubBot(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 5);

    if (lua_type(pLua, 1) != LUA_TBOOLEAN || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING ||
        lua_type(pLua, 5) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TBOOLEAN);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        return 0;
    }

    const char *NewBotName, *NewBotDescription, *NewBotEmail;
    size_t szNameLen, szDescrLen, szEmailLen;

    NewBotName = lua_tolstring(pLua, 2, &szNameLen);
    NewBotDescription = lua_tolstring(pLua, 3, &szDescrLen);
    NewBotEmail = lua_tolstring(pLua, 4, &szEmailLen);

    if (szNameLen == 0 || szNameLen > 64 || szDescrLen > 64 || szEmailLen > 64 || contains_any(std::string_view(NewBotName, szNameLen), " $|") ||
        contains_any(std::string_view(NewBotDescription, szDescrLen), "$|") || contains_any(std::string_view(NewBotEmail, szEmailLen), "$|") ||
        HashManager::m_Ptr->FindUser(std::string_view(NewBotName, szNameLen)))
    {
        lua_settop(pLua, 0);
        return 0;
    }

    const bool bBotHaveNewNick = (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)] != NewBotName);
    const bool bEnableBot = (lua_toboolean(pLua, 1) != 0);

    bool bRegStateChange = false, bDescriptionChange = false, bEmailChange = false;

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] != bEnableBot)
    {
        bRegStateChange = true;
    }

    SettingManager::m_Ptr->m_bUpdateLocked = true;

    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)].empty() ||
        SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)] != NewBotDescription)
    {
        if (szDescrLen != static_cast<size_t>(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)].empty() ? 0 : -1))
        {
            bDescriptionChange = true;
        }

        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION), NewBotDescription);
    }

    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)].empty() || SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)] != NewBotEmail)
    {
        if (szEmailLen != static_cast<size_t>(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)].empty() ? 0 : -1))
        {
            bEmailChange = true;
        }

        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL), NewBotEmail);
    }

    SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_USE_BOT_NICK_AS_HUB_SEC), (lua_toboolean(pLua, 5) != 0));

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)])
    {
        SettingManager::m_Ptr->m_bUpdateLocked = false;
        SettingManager::m_Ptr->DisableBot((bBotHaveNewNick || !bEnableBot), (bRegStateChange || bBotHaveNewNick || bDescriptionChange || bEmailChange));
        SettingManager::m_Ptr->m_bUpdateLocked = true;
    }

    SettingManager::m_Ptr->SetBool(std::to_underlying(SetBoolIds::SETBOOL_REG_BOT), bEnableBot);

    if (bBotHaveNewNick)
    {
        SettingManager::m_Ptr->SetText(std::to_underlying(SetTxtIds::SETTXT_BOT_NICK), NewBotName);
    }

    SettingManager::m_Ptr->m_bUpdateLocked = false;

    SettingManager::m_Ptr->UpdateHubSec();
    SettingManager::m_Ptr->UpdateMOTD();
    SettingManager::m_Ptr->UpdateHubNameWelcome();
    SettingManager::m_Ptr->UpdateRegOnlyMessage();
    SettingManager::m_Ptr->UpdateShareLimitMessage();
    SettingManager::m_Ptr->UpdateSlotsLimitMessage();
    SettingManager::m_Ptr->UpdateHubSlotRatioMessage();
    SettingManager::m_Ptr->UpdateMaxHubsLimitMessage();
    SettingManager::m_Ptr->UpdateNoTagMessage();
    SettingManager::m_Ptr->UpdateNickLimitMessage();
    SettingManager::m_Ptr->UpdateBotsSameNick();

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] && (bRegStateChange || bBotHaveNewNick || bDescriptionChange || bEmailChange))
    {
        SettingManager::m_Ptr->UpdateBot((bBotHaveNewNick || bRegStateChange));
    }

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg SetManRegs[] = {{"Save", Save}, // NOLINT(modernize-avoid-c-arrays)
                                      {"GetMOTD", GetMOTD},
                                      {"SetMOTD", SetMOTD},
                                      {"GetBool", GetBool},
                                      {"SetBool", SetBool},
                                      {"GetNumber", GetNumber},
                                      {"SetNumber", SetNumber},
                                      {"GetString", GetString},
                                      {"SetString", SetString},
                                      {"GetMinShare", GetMinShare},
                                      {"SetMinShare", SetMinShare},
                                      {"GetMaxShare", GetMaxShare},
                                      {"SetMaxShare", SetMaxShare},
                                      {"SetHubSlotRatio", SetHubSlotRatio},
                                      {"GetOpChat", GetOpChat},
                                      {"SetOpChat", SetOpChat},
                                      {"GetHubBot", GetHubBot},
                                      {"SetHubBot", SetHubBot},
                                      {nullptr, nullptr}};
//---------------------------------------------------------------------------

int RegSetMan(lua_State* pLua)
{
    luaL_newlib(pLua, SetManRegs);
    return 1;
}
//---------------------------------------------------------------------------
