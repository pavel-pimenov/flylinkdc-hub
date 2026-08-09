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
#include "LuaCoreLib.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
// alex82 ... �������� ������� ����������
#include "DcCommands.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "ResNickManager.h"
#include "LuaScript.h"
//---------------------------------------------------------------------------

static constexpr size_t g_ui32MaxMsgDataLen = 128000;

static int Restart(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    EventQueue::m_Ptr->AddNormal(EventQueue::EventType::RESTART, nullptr);

    return 0;
}
//------------------------------------------------------------------------------

static int Shutdown(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    EventQueue::m_Ptr->AddNormal(EventQueue::EventType::SHUTDOWN, nullptr);

    return 0;
}
//------------------------------------------------------------------------------

static int ResumeAccepts(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 0);

    ServerManager::ResumeAccepts();

    return 0;
}
//------------------------------------------------------------------------------

static int SuspendAccepts(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    const int n = lua_gettop(pLua);

    if (n == 0)
    {
        ServerManager::SuspendAccepts(0);
    }
    else if (n == 1)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

        const auto iSec = LuaInt<uint32_t>(pLua, 1);

        if (iSec != 0)
        {
            ServerManager::SuspendAccepts(iSec);
        }
        lua_settop(pLua, 0);
    }
    else
    {
        luaL_error(pLua, "bad argument count to 'SuspendAccepts' (0 or 1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
    }

    return 0;
}
//------------------------------------------------------------------------------

static int RegBot(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    if (ScriptManager::m_Ptr->m_ui8BotsCount > 63)
    {
        LUA_PUSH_NIL(pLua);
    }

    // alex82 ... RegBot / �������� �������������� ������� ��� �������� ���� � ����������� $MyINFO
    bool bFullMyINFO = false;
    if (lua_gettop(pLua) == 3)
    {
        bFullMyINFO = true;
    }

    if (lua_gettop(pLua) != 4 && !bFullMyINFO)
    {
        luaL_error(pLua, "bad argument count to 'RegBot' (3 or 4 expected, got %d)", lua_gettop(pLua));
        LUA_PUSH_NIL(pLua);
    }
    // alex82 ... RegBot /
    std::unique_ptr<ScriptBot> pNewBot;
    size_t szNickLen;
    if (!bFullMyINFO)
    {
        if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TBOOLEAN)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TSTRING);
            luaL_checktype(pLua, 3, LUA_TSTRING);
            luaL_checktype(pLua, 4, LUA_TBOOLEAN);

            LUA_PUSH_NIL(pLua);
        }

        size_t /*szNickLen,*/ szDescrLen, szEmailLen;

        const char* nick = lua_tolstring(pLua, 1, &szNickLen);
        const char* description = lua_tolstring(pLua, 2, &szDescrLen);
        const char* email = lua_tolstring(pLua, 3, &szEmailLen);

        const bool bIsOP = lua_toboolean(pLua, 4) != 0;

        if (szNickLen == 0 || szNickLen > 64 || contains_any(std::string_view(nick, szNickLen), " $|") || szDescrLen > 64 || contains_any(std::string_view(description, szDescrLen), "$|") ||
            szEmailLen > 64 || contains_any(std::string_view(email, szEmailLen), "$|") || HashManager::m_Ptr->FindUser(std::string_view(nick, szNickLen)) ||
            ReservedNicksManager::m_Ptr->CheckReserved(nick, HashNick(std::string_view(nick, szNickLen))))
        {
            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        pNewBot.reset(ScriptBot::CreateScriptBot(nick, szNickLen, description, szDescrLen, email, szEmailLen, bIsOP));
    }
    else
    {
        // alex82 ... RegBot /
        if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TBOOLEAN)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TSTRING);
            luaL_checktype(pLua, 3, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        size_t szMyINFOLen;

        const char* nick = lua_tolstring(pLua, 1, &szNickLen);
        const char* myinfo = lua_tolstring(pLua, 2, &szMyINFOLen);

        const bool bIsOP = lua_toboolean(pLua, 3) != 0;

        if (szNickLen == 0 || szNickLen > 64 || contains_any(std::string_view(nick, szNickLen), " $|") || HashManager::m_Ptr->FindUser(std::string_view(nick, szNickLen)) ||
            ReservedNicksManager::m_Ptr->CheckReserved(nick, HashNick(std::string_view(nick, szNickLen))))
        {
            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        std::string test_myinfo;
        test_myinfo.resize(64 + 15);
        const int iMyinfoLen = snprintf(test_myinfo.data(), test_myinfo.size(), "$MyINFO $ALL %s ", nick);
        test_myinfo.resize(static_cast<size_t>(iMyinfoLen));
        if (memcmp(test_myinfo.data(), myinfo, szNickLen + 14) != 0)
        {
            luaL_error(pLua, "invalid MyINFO string");
            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        pNewBot.reset(ScriptBot::CreateScriptBot(nick, szNickLen, myinfo, szMyINFOLen, bIsOP));
    }

    if (!pNewBot)
    {
        LogDbg("[MEM] Cannot allocate pNewBot in Core.RegBot");

        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    // PPK ... finally we can clear stack
    lua_settop(pLua, 0);

    Script* pScript = ScriptManager::m_Ptr->FindScript(pLua);
    if (!pScript)
    {
        lua_pushnil(pLua);
        return 1;
    }

    ScriptBot *cur = nullptr, *next = pScript->m_pBotList;

    while (next)
    {
        cur = next;
        next = cur->m_pNext;

        if (iequals(pNewBot->m_sNick, cur->m_sNick))
        {
            lua_pushnil(pLua);
            return 1;
        }
    }

    if (!pScript->m_pBotList)
    {
        pScript->m_pBotList = pNewBot.release();
    }
    else
    {
        ScriptBot* raw = pNewBot.get();
        pScript->m_pBotList->m_pPrev = raw;
        raw->m_pNext = pScript->m_pBotList;
        pScript->m_pBotList = raw;
        pNewBot.release(); // NOLINT(bugprone-unused-return-value)
    }

    ReservedNicksManager::m_Ptr->AddReservedNick(pScript->m_pBotList->m_sNick.c_str(), true);

    Users::m_Ptr->AddBot2NickList(pScript->m_pBotList->m_sNick.c_str(), szNickLen, pScript->m_pBotList->m_bIsOP);

    Users::m_Ptr->AddBot2MyInfos(pScript->m_pBotList->m_sMyINFO.c_str());

    // PPK ... fixed hello sending only to users without NoHello
    ScriptBot* bot = pScript->m_pBotList;
    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Hello %s|", bot->m_sNick.c_str());
    if (CheckSprintf(iMsgLen, ServerManager::m_szGlobalBufferSize, "RegBot"))
    {
        GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::HELLO);
    }

    const size_t szMyINFOLen = bot->m_sMyINFO.size();
    GlobalDataQueue::m_Ptr->AddQueueItem(bot->m_sMyINFO.c_str(), szMyINFOLen, bot->m_sMyINFO.c_str(), szMyINFOLen, GlobalDataQueue::Cmd::MYINFO);

    if (bot->m_bIsOP)
    {
        GlobalDataQueue::m_Ptr->OpListStore(bot->m_sNick.c_str());
    }

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int UnregBot(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    LUA_CHECK_TYPE(pLua, 1, LUA_TSTRING);

    size_t szLen;
    const char* botnick = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0)
    {
        LUA_PUSH_NIL(pLua);
    }

    Script* obj = ScriptManager::m_Ptr->FindScript(pLua);
    if (!obj)
    {
        LUA_PUSH_NIL(pLua);
    }

    ScriptBot *cur = nullptr, *next = obj->m_pBotList;

    while (next)
    {
        cur = next;
        next = cur->m_pNext;

        if (iequals(botnick, cur->m_sNick))
        {
            ReservedNicksManager::m_Ptr->DelReservedNick(cur->m_sNick.c_str(), true);

            Users::m_Ptr->DelFromNickList(cur->m_sNick.c_str(), cur->m_bIsOP);

            Users::m_Ptr->DelBotFromMyInfos(cur->m_sMyINFO.c_str());

            const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", cur->m_sNick.c_str());
            if (iMsgLen > 0)
            {
                GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::QUIT);
            }

            if (!cur->m_pPrev)
            {
                if (!cur->m_pNext)
                {
                    obj->m_pBotList = nullptr;
                }
                else
                {
                    cur->m_pNext->m_pPrev = nullptr;
                    obj->m_pBotList = cur->m_pNext;
                }
            }
            else if (!cur->m_pNext)
            {
                cur->m_pPrev->m_pNext = nullptr;
            }
            else
            {
                cur->m_pPrev->m_pNext = cur->m_pNext;
                cur->m_pNext->m_pPrev = cur->m_pPrev;
            }

            std::unique_ptr<ScriptBot> guard(cur);

            lua_settop(pLua, 0);

            lua_pushboolean(pLua, 1);
            return 1;
        }
    }

    lua_settop(pLua, 0);

    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetBots(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_newtable(pLua);

    const int t = lua_gettop(pLua); int i = 0;

    for (Script* cur : ScriptManager::m_Ptr->m_RunningScriptList)
    {
        ScriptBot *pBot = nullptr, *nextbot = cur->m_pBotList;

        while (nextbot)
        {
            pBot = nextbot;
            nextbot = pBot->m_pNext;

            lua_pushinteger(pLua, ++i);

            lua_newtable(pLua);

            const int b = lua_gettop(pLua);

            lua_pushliteral(pLua, "sNick");
            lua_pushstring(pLua, pBot->m_sNick.c_str());
            lua_rawset(pLua, b);

            lua_pushliteral(pLua, "sMyINFO");
            lua_pushstring(pLua, pBot->m_sMyINFO.c_str());
            lua_rawset(pLua, b);

            lua_pushliteral(pLua, "bIsOP");
            pBot->m_bIsOP ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
            lua_rawset(pLua, b);

            lua_pushliteral(pLua, "sScriptName");
            lua_pushstring(pLua, cur->m_sName.c_str());
            lua_rawset(pLua, b);

            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetActualUsersPeak(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_pushinteger(pLua, ServerManager::m_ui32Peak);

    return 1;
}
//------------------------------------------------------------------------------

static int GetMaxUsersPeak(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_pushinteger(pLua, SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS_PEAK)]);

    return 1;
}
//------------------------------------------------------------------------------

static int GetCurrentSharedSize(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_pushinteger(pLua, static_cast<lua_Integer>(ServerManager::m_ui64TotalShare));

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubIP(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    if (ServerManager::m_sHubIP[0] != '\0')
    {
        lua_pushstring(pLua, ServerManager::m_sHubIP.data());
    }
    else
    {
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubIPs(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    if (ServerManager::m_sHubIP[0] == '\0' && ServerManager::m_sHubIP6[0] == '\0')
    {
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    const int t = lua_gettop(pLua); int i = 0;

    if (ServerManager::m_sHubIP6[0] != '\0')
    {
        i++;

        lua_pushinteger(pLua, i);

        lua_pushstring(pLua, ServerManager::m_sHubIP6.data());
        lua_rawset(pLua, t);
    }

    if (ServerManager::m_sHubIP[0] != '\0')
    {
        i++;

        lua_pushinteger(pLua, i);

        lua_pushstring(pLua, ServerManager::m_sHubIP.data());
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubSecAlias(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_pushlstring(pLua, SettingManager::HubSec(), SettingManager::HubSecStr().size());

    return 1;
}
//------------------------------------------------------------------------------

static int GetPtokaXPath(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    const std::string path = ServerManager::m_sPath + "/";
    lua_pushlstring(pLua, path.c_str(), path.size());

    return 1;
}
//------------------------------------------------------------------------------

static int GetUsersCount(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    lua_pushinteger(pLua, ServerManager::m_ui32Logged);

    return 1;
}
//------------------------------------------------------------------------------

static int GetUpTime(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 0);

    time_t t;
    time(&t);

    lua_pushinteger(pLua, t - ServerManager::m_tStartTime);

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineByOpStatus(lua_State* pLua, const bool bOperator)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    bool bFullTable = false;

    const int n = lua_gettop(pLua);

    if (n != 0)
    {
        if (n != 1)
        {
            if (bOperator)
            {
                luaL_error(pLua, "bad argument count to 'GetOnlineOps' (0 or 1 expected, got %d)", lua_gettop(pLua));
            }
            else
            {
                luaL_error(pLua, "bad argument count to 'GetOnlineNonOps' (0 or 1 expected, got %d)", lua_gettop(pLua));
            }
            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }
        if (lua_type(pLua, 1) != LUA_TBOOLEAN)
        {
            luaL_checktype(pLua, 1, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }
        else
        {
            bFullTable = lua_toboolean(pLua, 1) != 0;

            lua_settop(pLua, 0);
        }
    }

    lua_newtable(pLua);

    const int t = lua_gettop(pLua); int i = 0;

    for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
    {
        User* curUser = curUserPtr.get();

        if (curUser->m_ui8State == User::UserStates::STATE_ADDED && ((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == bOperator)
        {
            lua_pushinteger(pLua, ++i);

            ScriptPushUser(pLua, curUser, bFullTable);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineNonOps(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    return GetOnlineByOpStatus(pLua, false);
}
//------------------------------------------------------------------------------

static int GetOnlineOps(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    return GetOnlineByOpStatus(pLua, true);
}
//------------------------------------------------------------------------------

static int GetOnlineRegs(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    bool bFullTable = false;

    const int n = lua_gettop(pLua);

    if (n != 0)
    {
        if (n != 1)
        {
            luaL_error(pLua, "bad argument count to 'GetOnlineRegs' (0 or 1 expected, got %d)", lua_gettop(pLua));
            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }
        if (lua_type(pLua, 1) != LUA_TBOOLEAN)
        {
            luaL_checktype(pLua, 1, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }
        else
        {
            bFullTable = lua_toboolean(pLua, 1) != 0;

            lua_settop(pLua, 0);
        }
    }

    lua_newtable(pLua);

    const int t = lua_gettop(pLua); int i = 0;

    for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
    {
        User* curUser = curUserPtr.get();

        if (curUser->m_ui8State == User::UserStates::STATE_ADDED && curUser->m_i32Profile != -1)
        {
            lua_pushinteger(pLua, ++i);

            ScriptPushUser(pLua, curUser, bFullTable);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineUsers(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    bool bFullTable = false;

    int32_t iProfile = -2;

    const int n = lua_gettop(pLua);

    if (n == 2)
    {
        if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TBOOLEAN)
        {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            luaL_checktype(pLua, 2, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        iProfile = LuaInt<int32_t>(pLua, 1);
        bFullTable = lua_toboolean(pLua, 2) != 0;

        lua_settop(pLua, 0);
    }
    else if (n == 1)
    {
        if (lua_type(pLua, 1) == LUA_TNUMBER)
        {
            iProfile = LuaInt<int32_t>(pLua, 1);
        }
        else if (lua_type(pLua, 1) == LUA_TBOOLEAN)
        {
            bFullTable = lua_toboolean(pLua, 1) != 0;
        }
        else
        {
            luaL_error(pLua, "bad argument #1 to 'GetOnlineUsers' (number or boolean expected, got %s)", lua_typename(pLua, lua_type(pLua, 1)));
            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        lua_settop(pLua, 0);
    }
    else if (n != 0)
    {
        luaL_error(pLua, "bad argument count to 'GetOnlineUsers' (0, 1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);

    const int t = lua_gettop(pLua); int i = 0;

    User* curUser = nullptr;

    if (iProfile == -2)
    {
        for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
        {
            curUser = curUserPtr.get();

            if (curUser->m_ui8State == User::UserStates::STATE_ADDED)
            {
                lua_pushinteger(pLua, ++i);

                ScriptPushUser(pLua, curUser, bFullTable);
                lua_rawset(pLua, t);
            }
        }
    }
    else
    {
        for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
        {
            curUser = curUserPtr.get();

            if (curUser->m_ui8State == User::UserStates::STATE_ADDED && curUser->m_i32Profile == iProfile)
            {
                lua_pushinteger(pLua, ++i);

                ScriptPushUser(pLua, curUser, bFullTable);
                lua_rawset(pLua, t);
            }
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetUser(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    bool bFullTable = false;

    size_t szLen = 0;

    const char* nick;

    const int n = lua_gettop(pLua);

    if (n == 2)
    {
        if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TBOOLEAN)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        nick = lua_tolstring(pLua, 1, &szLen);

        bFullTable = lua_toboolean(pLua, 2) != 0;
    }
    else if (n == 1)
    {
        if (lua_type(pLua, 1) != LUA_TSTRING)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        nick = lua_tolstring(pLua, 1, &szLen);
    }
    else
    {
        luaL_error(pLua, "bad argument count to 'GetUser' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    if (szLen == 0)
    {
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    User* u = HashManager::m_Ptr->FindUser(std::string_view(nick, szLen));

    lua_settop(pLua, 0);

    if (u)
    {
        ScriptPushUser(pLua, u, bFullTable);
    }
    else
    {
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetUsers(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    bool bFullTable = false;

    size_t szLen = 0;

    const char* sIP;

    const int n = lua_gettop(pLua);

    if (n == 2)
    {
        if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TBOOLEAN)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        sIP = lua_tolstring(pLua, 1, &szLen);

        bFullTable = lua_toboolean(pLua, 2) != 0;
    }
    else if (n == 1)
    {
        if (lua_type(pLua, 1) != LUA_TSTRING)
        {
            luaL_checktype(pLua, 1, LUA_TSTRING);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }

        sIP = lua_tolstring(pLua, 1, &szLen);
    }
    else
    {
        luaL_error(pLua, "bad argument count to 'GetUsers' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    Hash128 ui128Hash;

    if (szLen == 0 || !HashIP(sIP, ui128Hash))
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);

        return 1;
    }

    User* next = HashManager::m_Ptr->FindUser(ui128Hash);

    lua_settop(pLua, 0);

    if (!next)
    {
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);

    const int t = lua_gettop(pLua); int i = 0;

    User* curUser = nullptr;

    while (next)
    {
        curUser = next;
        next = curUser->m_pHashIpTableNext;

        lua_pushinteger(pLua, ++i);

        ScriptPushUser(pLua, curUser, bFullTable);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetUserAllData(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TTABLE)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User* u = ScriptGetUser(pLua, 1, "GetUserAllData");

    if (!u)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    ScriptPushUserExtended(pLua, u, 1);

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserData(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User* u = ScriptGetUser(pLua, 2, "GetUserData");

    if (!u)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto ui8DataId = LuaInt<uint8_t>(pLua, 2);

    GlobalDataQueue::m_Ptr->PrometheusLuaUserDataInc(ui8DataId);
    switch (ui8DataId)
    {
    case 0:
        lua_pushliteral(pLua, "sMode");
        if (u->m_sModes[0] != '\0')
        {
            lua_pushstring(pLua, u->m_sModes.data());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 1:
        lua_pushliteral(pLua, "sMyInfoString");
        if (!u->m_sMyInfoOriginal.empty())
        {
            lua_pushlstring(pLua, u->m_sMyInfoOriginal.data(), u->m_ui16MyInfoOriginalLen);
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 2:
        lua_pushliteral(pLua, "sDescription");
        if (!u->m_sDescription.empty())
        {
            lua_pushlstring(pLua, u->m_sDescription.data(), u->m_sDescription.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 3:
        lua_pushliteral(pLua, "sTag");
        if (!u->m_sTag.empty())
        {
            lua_pushlstring(pLua, u->m_sTag.data(), u->m_sTag.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 4:
        lua_pushliteral(pLua, "sConnection");
        if (!u->m_sConnection.empty())
        {
            lua_pushlstring(pLua, u->m_sConnection.data(), u->m_sConnection.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 5:
        lua_pushliteral(pLua, "sEmail");
        if (!u->m_sEmail.empty())
        {
            lua_pushlstring(pLua, u->m_sEmail.data(), u->m_sEmail.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 6:
        lua_pushliteral(pLua, "sClient");
        if (!u->m_sClient.empty())
        {
            lua_pushlstring(pLua, u->m_sClient.data(), u->m_sClient.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 7:
        lua_pushliteral(pLua, "sClientVersion");
        if (!u->m_sTagVersion.empty())
        {
            lua_pushlstring(pLua, u->m_sTagVersion.data(), u->m_sTagVersion.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
#ifdef FLYLINKDC_USE_VERSION
    case 8:
        lua_pushliteral(pLua, "sVersion");
        if (!u->m_sVersion.empty())
        {
            lua_pushstring(pLua, u->m_sVersion.c_str());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
#endif
    case 9:
        lua_pushliteral(pLua, "bConnected");
        u->m_ui8State == User::UserStates::STATE_ADDED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 10:
        lua_pushliteral(pLua, "bActive");
        if ((u->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
        {
            (u->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        }
        else
        {
            (u->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        }
        lua_rawset(pLua, 1);
        break;
    case 11:
        lua_pushliteral(pLua, "bOperator");
        (u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 12:
        lua_pushliteral(pLua, "bUserCommand");
        (u->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 13:
        lua_pushliteral(pLua, "bQuickList");
        (u->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 14:
        lua_pushliteral(pLua, "bSuspiciousTag");
        (u->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 15:
        lua_pushliteral(pLua, "iProfile");
        lua_pushinteger(pLua, u->m_i32Profile);
        lua_rawset(pLua, 1);
        break;
    case 16:
        lua_pushliteral(pLua, "iShareSize");
        lua_pushinteger(pLua, static_cast<lua_Integer>(u->m_ui64SharedSize));

        lua_rawset(pLua, 1);
        break;
    case 17:
        lua_pushliteral(pLua, "iHubs");
        lua_pushinteger(pLua, u->m_ui32Hubs);

        lua_rawset(pLua, 1);
        break;
    case 18:
        lua_pushliteral(pLua, "iNormalHubs");
        (u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32NormalHubs);

        lua_rawset(pLua, 1);
        break;
    case 19:
        lua_pushliteral(pLua, "iRegHubs");
        (u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32RegHubs);

        lua_rawset(pLua, 1);
        break;
    case 20:
        lua_pushliteral(pLua, "iOpHubs");
        (u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32OpHubs);

        lua_rawset(pLua, 1);
        break;
    case 21:
        lua_pushliteral(pLua, "iSlots");
        lua_pushinteger(pLua, u->m_ui32Slots);

        lua_rawset(pLua, 1);
        break;
    case 22:
        lua_pushliteral(pLua, "iLlimit");
        lua_pushinteger(pLua, u->m_ui32LLimit);

        lua_rawset(pLua, 1);
        break;
    case 23:
        lua_pushliteral(pLua, "iDefloodWarns");
        lua_pushinteger(pLua, u->m_ui32DefloodWarnings);

        lua_rawset(pLua, 1);
        break;
    case 24:
        lua_pushliteral(pLua, "iMagicByte");
        lua_pushinteger(pLua, u->m_ui8MagicByte);

        lua_rawset(pLua, 1);
        break;
    case 25:
        lua_pushliteral(pLua, "iLoginTime");
        lua_pushinteger(pLua, u->m_tLoginTime);

        lua_rawset(pLua, 1);
        break;
    case 26:
        lua_pushliteral(pLua, "sCountryCode");
        if (IpP2Country::m_Ptr->m_ui32Count != 0)
        {
            lua_pushlstring(pLua, IpP2Country::m_Ptr->GetCountry(u->m_ui8Country, false), 2);
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 27:
    {
        lua_pushliteral(pLua, "sMac");
        std::array<char, 18> sMac = {};
        if (GetMacAddress(u->m_sIP.data(), sMac.data()))
        {
            lua_pushlstring(pLua, sMac.data(), 17);
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    }
    case 28:
        lua_pushliteral(pLua, "bDescriptionChanged");
        (u->m_ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 29:
        lua_pushliteral(pLua, "bTagChanged");
        (u->m_ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 30:
        lua_pushliteral(pLua, "bConnectionChanged");
        (u->m_ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 31:
        lua_pushliteral(pLua, "bEmailChanged");
        (u->m_ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 32:
        lua_pushliteral(pLua, "bShareChanged");
        (u->m_ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    case 33:
        lua_pushliteral(pLua, "sScriptedDescriptionShort");
        if (!u->m_sChangedDescriptionShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedDescriptionShort.data(), u->m_sChangedDescriptionShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 34:
        lua_pushliteral(pLua, "sScriptedDescriptionLong");
        if (!u->m_sChangedDescriptionLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedDescriptionLong.data(), u->m_sChangedDescriptionLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 35:
        lua_pushliteral(pLua, "sScriptedTagShort");
        if (!u->m_sChangedTagShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedTagShort.data(), u->m_sChangedTagShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 36:
        lua_pushliteral(pLua, "sScriptedTagLong");
        if (!u->m_sChangedTagLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedTagLong.data(), u->m_sChangedTagLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 37:
        lua_pushliteral(pLua, "sScriptedConnectionShort");
        if (!u->m_sChangedConnectionShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedConnectionShort.data(), u->m_sChangedConnectionShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 38:
        lua_pushliteral(pLua, "sScriptedConnectionLong");
        if (!u->m_sChangedConnectionLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedConnectionLong.data(), u->m_sChangedConnectionLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 39:
        lua_pushliteral(pLua, "sScriptedEmailShort");
        if (!u->m_sChangedEmailShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedEmailShort.data(), u->m_sChangedEmailShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 40:
        lua_pushliteral(pLua, "sScriptedEmailLong");
        if (!u->m_sChangedEmailLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedEmailLong.data(), u->m_sChangedEmailLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        break;
    case 41:
        lua_pushliteral(pLua, "iScriptediShareSizeShort");
        lua_pushinteger(pLua, static_cast<lua_Integer>(u->m_ui64ChangedSharedSizeShort));

        lua_rawset(pLua, 1);
        break;
    case 42:
        lua_pushliteral(pLua, "iScriptediShareSizeLong");
        lua_pushinteger(pLua, static_cast<lua_Integer>(u->m_ui64ChangedSharedSizeLong));

        lua_rawset(pLua, 1);
        break;
    case 43:
    {
        lua_pushliteral(pLua, "tIPs");
        lua_newtable(pLua);

        const int t = lua_gettop(pLua);

        lua_pushinteger(pLua, 1);

        lua_pushlstring(pLua, u->m_sIP.data(), u->m_ui8IpLen);
        lua_rawset(pLua, t);

        if (u->m_sIPv4[0] != '\0')
        {
            lua_pushinteger(pLua, 2);

            lua_pushlstring(pLua, u->m_sIPv4.data(), u->m_ui8IPv4Len);
            lua_rawset(pLua, t);
        }

        lua_rawset(pLua, 1);
        break;
    }

    // alex82 ... HideUser / ������� �����
    case 65:
        lua_pushliteral(pLua, "bHidden");
        (u->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    // alex82 ... NoQuit / ��������� $Quit ��� �����
    case 66:
        lua_pushliteral(pLua, "bNoQuit");
        (u->m_ui32InfoBits & User::INFOBIT_NO_QUIT) == User::INFOBIT_NO_QUIT ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
    // alex82 ... HideUserKey / ������ ���� �����
    case 67:
        lua_pushliteral(pLua, "bHiddenKey");
        (u->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        lua_rawset(pLua, 1);
        break;
#ifdef USE_FLYLINKDC_EXT_JSON
    case 100:
    {
        lua_pushliteral(pLua, "sExtJson");
        if (u->m_user_ext_info && !u->m_user_ext_info->GetExtJSONCommand().empty())
        {
            lua_pushlstring(pLua, u->m_user_ext_info->GetExtJSONCommand().c_str(), u->m_user_ext_info->GetExtJSONCommand().length());
        }
        else
        {
            lua_pushnil(pLua);
        }
        lua_rawset(pLua, 1);
        return 1;
    }
#endif
    default:
        luaL_error(pLua, "bad argument #2 to 'GetUserData' (it's not valid id)");
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserValue(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User* u = ScriptGetUser(pLua, 2, "GetUserValue");

    if (!u)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const auto ui8DataId = LuaInt<uint8_t>(pLua, 2);

    lua_settop(pLua, 0);
    GlobalDataQueue::m_Ptr->PrometheusLuaUserValueInc(ui8DataId);
    switch (ui8DataId)
    {
    case 0:
        if (u->m_sModes[0] != '\0')
        {
            lua_pushstring(pLua, u->m_sModes.data());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 1:
        if (!u->m_sMyInfoOriginal.empty())
        {
            lua_pushlstring(pLua, u->m_sMyInfoOriginal.data(), u->m_ui16MyInfoOriginalLen);
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 2:
        if (!u->m_sDescription.empty())
        {
            lua_pushlstring(pLua, u->m_sDescription.data(), u->m_sDescription.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 3:
        if (!u->m_sTag.empty())
        {
            lua_pushlstring(pLua, u->m_sTag.data(), u->m_sTag.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 4:
        if (!u->m_sConnection.empty())
        {
            lua_pushlstring(pLua, u->m_sConnection.data(), u->m_sConnection.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 5:
        if (!u->m_sEmail.empty())
        {
            lua_pushlstring(pLua, u->m_sEmail.data(), u->m_sEmail.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 6:
        if (!u->m_sClient.empty())
        {
            lua_pushlstring(pLua, u->m_sClient.data(), u->m_sClient.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 7:
        if (!u->m_sTagVersion.empty())
        {
            lua_pushlstring(pLua, u->m_sTagVersion.data(), u->m_sTagVersion.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
#ifdef FLYLINKDC_USE_VERSION
    case 8:
        if (!u->m_sVersion.empty())
        {
            lua_pushstring(pLua, u->m_sVersion.c_str());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
#endif
    case 9:
        u->m_ui8State == User::UserStates::STATE_ADDED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 10:
        if ((u->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)
        {
            (u->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        }
        else
        {
            (u->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        }
        return 1;
    case 11:
        (u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 12:
        (u->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 13:
        (u->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 14:
        (u->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 15:
        lua_pushinteger(pLua, u->m_i32Profile);
        return 1;
    case 16:
        lua_pushinteger(pLua, static_cast<lua_Integer>(u->m_ui64SharedSize));

        return 1;
    case 17:
        lua_pushinteger(pLua, u->m_ui32Hubs);

        return 1;
    case 18:
        (u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32NormalHubs);

        return 1;
    case 19:
        (u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32RegHubs);

        return 1;
    case 20:
        (u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32OpHubs);

        return 1;
    case 21:
        lua_pushinteger(pLua, u->m_ui32Slots);

        return 1;
    case 22:
        lua_pushinteger(pLua, u->m_ui32LLimit);

        return 1;
    case 23:
        lua_pushinteger(pLua, u->m_ui32DefloodWarnings);

        return 1;
    case 24:
        lua_pushinteger(pLua, u->m_ui8MagicByte);

        return 1;
    case 25:
        lua_pushinteger(pLua, u->m_tLoginTime);

        return 1;
    case 26:
        if (IpP2Country::m_Ptr->m_ui32Count != 0)
        {
            lua_pushlstring(pLua, IpP2Country::m_Ptr->GetCountry(u->m_ui8Country, false), 2);
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 27:
    {
        std::array<char, 18> sMac = {};
        if (GetMacAddress(u->m_sIP.data(), sMac.data()))
        {
            lua_pushlstring(pLua, sMac.data(), 17);
            return 1;
        }

        lua_pushnil(pLua);
        return 1;
    }
    case 28:
        (u->m_ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 29:
        (u->m_ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 30:
        (u->m_ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 31:
        (u->m_ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 32:
        (u->m_ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    case 33:
        if (!u->m_sChangedDescriptionShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedDescriptionShort.data(), u->m_sChangedDescriptionShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 34:
        if (!u->m_sChangedDescriptionLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedDescriptionLong.data(), u->m_sChangedDescriptionLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 35:
        if (!u->m_sChangedTagShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedTagShort.data(), u->m_sChangedTagShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 36:
        if (!u->m_sChangedTagLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedTagLong.data(), u->m_sChangedTagLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 37:
        if (!u->m_sChangedConnectionShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedConnectionShort.data(), u->m_sChangedConnectionShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 38:
        if (!u->m_sChangedConnectionLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedConnectionLong.data(), u->m_sChangedConnectionLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 39:
        if (!u->m_sChangedEmailShort.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedEmailShort.data(), u->m_sChangedEmailShort.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 40:
        if (!u->m_sChangedEmailLong.empty())
        {
            lua_pushlstring(pLua, u->m_sChangedEmailLong.data(), u->m_sChangedEmailLong.size());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    case 41:
        lua_pushinteger(pLua, static_cast<lua_Integer>(u->m_ui64ChangedSharedSizeShort));

        return 1;
    case 42:
        lua_pushinteger(pLua, static_cast<lua_Integer>(u->m_ui64ChangedSharedSizeLong));

        return 1;
    case 43:
    {
        lua_newtable(pLua);

        const int t = lua_gettop(pLua);

        lua_pushinteger(pLua, 1);

        lua_pushlstring(pLua, u->m_sIP.data(), u->m_ui8IpLen);
        lua_rawset(pLua, t);

        if (u->m_sIPv4[0] != '\0')
        {
            lua_pushinteger(pLua, 2);

            lua_pushlstring(pLua, u->m_sIPv4.data(), u->m_ui8IPv4Len);
            lua_rawset(pLua, t);
        }

        return 1;
    }

    // alex82 ... HideUser / ������� �����
    case 65:
        (u->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    // alex82 ... NoQuit / ��������� $Quit ��� �����
    case 66:
        (u->m_ui32InfoBits & User::INFOBIT_NO_QUIT) == User::INFOBIT_NO_QUIT ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;
    // alex82 ... HideUserKey / ������ ���� �����
    case 67:
        (u->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        return 1;

#ifdef USE_FLYLINKDC_EXT_JSON
    case 100:
    {
        if (u->m_user_ext_info && !u->m_user_ext_info->GetExtJSONCommand().empty())
        {
            lua_pushlstring(pLua, u->m_user_ext_info->GetExtJSONCommand().c_str(), u->m_user_ext_info->GetExtJSONCommand().length());
        }
        else
        {
            lua_pushnil(pLua);
        }
        return 1;
    }
#endif
    default:
        luaL_error(pLua, "bad argument #2 to 'GetUserValue' (it's not valid id)");

        lua_pushnil(pLua);
        return 1;
    }
}
//------------------------------------------------------------------------------

static int Disconnect(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    User* pUser;

    if (lua_type(pLua, 1) == LUA_TTABLE)
    {
        pUser = ScriptGetUser(pLua, 1, "Disconnect");

        if (!pUser)
        {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }
    }
    else if (lua_type(pLua, 1) == LUA_TSTRING)
    {
        size_t szLen;
        const char* pNick = lua_tolstring(pLua, 1, &szLen);

        if (szLen == 0)
        {
            return 0;
        }

        pUser = HashManager::m_Ptr->FindUser(std::string_view(pNick, szLen));

        if (!pUser)
        {
            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        }
    }
    else
    {
        luaL_error(pLua, "bad argument #1 to 'Disconnect' (user table or string expected, got %s)", lua_typename(pLua, lua_type(pLua, 1)));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    pUser->Close();

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Kick(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    User* pUser = ScriptGetUser(pLua, 3, "Kick");

    if (!pUser)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szKickerLen, szReasonLen;
    const char* sKicker = lua_tolstring(pLua, 2, &szKickerLen);
    const char* sReason = lua_tolstring(pLua, 3, &szReasonLen);

    if (szKickerLen == 0 || szKickerLen > 64 || szReasonLen == 0 || szReasonLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    BanManager::m_Ptr->TempBan(pUser, sReason, sKicker, 0, 0, false);

    pUser->SendFormat(
        "Core.Kick", false, "<%s> %s: %s|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_BEING_KICKED_BCS)].c_str(), sReason);

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("Core.Kick",
                                                    "<%s> *** %s %s IP %s %s %s %s: %s|",
                                                    SettingManager::HubSec(),
                                                    pUser->m_sNick.c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_LWR)].c_str(),
                                                    pUser->m_sIP.data(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WAS_KICKED_BY)].c_str(),
                                                    sKicker,
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BECAUSE_LWR)].c_str(),
                                                    sReason);
    }

    // disconnect the user
    UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) kicked by script.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

    pUser->Close();

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Redirect(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User* pUser = ScriptGetUser(pLua, 3, "Redirect");

    if (!pUser)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szAddressLen, szReasonLen;
    const char* sAddress = lua_tolstring(pLua, 2, &szAddressLen);
    const char* sReason = lua_tolstring(pLua, 3, &szReasonLen);

    if (szAddressLen == 0 || szAddressLen > 1024 || szReasonLen == 0 || szReasonLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    pUser->SendFormat("Core.Redirect",
                      false,
                      "<%s> %s %s. %s: %s|$ForceMove %s|",
                      SettingManager::HubSec(),
                      LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_REDIR_TO)].c_str(),
                      sAddress,
                      LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MESSAGE)].c_str(),
                      sReason,
                      sAddress);

    pUser->Close();

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int DefloodWarn(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS_RET_NIL(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TTABLE)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    User* u = ScriptGetUser(pLua, 1, "DefloodWarn");

    if (!u)
    {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    u->m_ui32DefloodWarnings++;

    if (u->m_ui32DefloodWarnings >= static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_COUNT)]))
    {
        switch (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_ACTION)])
        {
        case 0:
            break;
        case 1:
            BanManager::m_Ptr->TempBan(u, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FLOODING)].c_str(), nullptr, 0, 0, false);
            break;
        case 2:
            BanManager::m_Ptr->TempBan(u,
                                       LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FLOODING)].c_str(),
                                       nullptr,
                                       SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_TEMP_BAN_TIME)],
                                       0,
                                       false);
            break;
        case 3:
        {
            BanManager::m_Ptr->Ban(u, LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FLOODING)].c_str(), nullptr, false);
            break;
        }
        default:
            break;
        }

        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_DEFLOOD_REPORT)])
        {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("Core.DefloodWarn",
                                                        "<%s> *** %s %s %s %s %s.|",
                                                        SettingManager::HubSec(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FLOODER)].c_str(),
                                                        u->m_sNick.c_str(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_WITH_IP)].c_str(),
                                                        u->m_sIP.data(),
                                                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DISCONN_BY_SCRIPT)].c_str());
        }

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Flood from %s (%s) - user closed by script.", u->m_sNick.c_str(), u->m_sIP.data());

        u->Close();
    }

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int SendToAll(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    const char* sData = lua_tolstring(pLua, 1, &szLen);

    if (sData[0] != '\0' && szLen < 128001)
    {
        if (sData[szLen - 1] != '|')
        {
            memcpy(ServerManager::m_pGlobalBuffer, sData, szLen);
            ServerManager::m_pGlobalBuffer[szLen] = '|';
            ServerManager::m_pGlobalBuffer[szLen + 1] = '\0';
            GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, szLen + 1, nullptr, 0, GlobalDataQueue::Cmd::LUA);
        }
        else
        {
            GlobalDataQueue::m_Ptr->AddQueueItem(sData, szLen, nullptr, 0, GlobalDataQueue::Cmd::LUA);
        }
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToNick(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szNickLen, szDataLen;
    const char* sNick = lua_tolstring(pLua, 1, &szNickLen);
    const char* sData = lua_tolstring(pLua, 2, &szDataLen);

    if (szNickLen == 0 || szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    User* u = HashManager::m_Ptr->FindUser(std::string_view(sNick, szNickLen));
    if (u)
    {
        if (sData[szDataLen - 1] != '|')
        {
            memcpy(ServerManager::m_pGlobalBuffer, sData, szDataLen);
            ServerManager::m_pGlobalBuffer[szDataLen] = '|';
            ServerManager::m_pGlobalBuffer[szDataLen + 1] = '\0';
            u->SendCharDelayed(ServerManager::m_pGlobalBuffer, szDataLen + 1);
        }
        else
        {
            u->SendCharDelayed(sData, szDataLen);
        }
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToOpChat(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szDataLen;
    const char* sData = lua_tolstring(pLua, 1, &szDataLen);

    if (szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)])
    {
        const int iLen = snprintf(ServerManager::m_pGlobalBuffer,
                            ServerManager::m_szGlobalBufferSize,
                            "%s $<%s> %s|",
                            SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str(),
                            SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str(),
                            sData);
        if (iLen > 0)
        {
            GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iLen, nullptr, 0, GlobalDataQueue::SendItem::OPCHAT);
        }
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToOps(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 1);

    if (lua_type(pLua, 1) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    const char* sData = lua_tolstring(pLua, 1, &szLen);

    if (szLen == 0 || szLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    if (sData[szLen - 1] != '|')
    {
        memcpy(ServerManager::m_pGlobalBuffer, sData, szLen);
        ServerManager::m_pGlobalBuffer[szLen] = '|';
        ServerManager::m_pGlobalBuffer[szLen + 1] = '\0';
        GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, szLen + 1, nullptr, 0, GlobalDataQueue::Cmd::OPS);
    }
    else
    {
        GlobalDataQueue::m_Ptr->AddQueueItem(sData, szLen, nullptr, 0, GlobalDataQueue::Cmd::OPS);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToProfile(lua_State* pLua)
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

    const auto i32Profile = LuaInt<int32_t>(pLua, 1);

    size_t szDataLen;
    const char* sData = lua_tolstring(pLua, 2, &szDataLen);

    if (szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    if (sData[szDataLen - 1] != '|')
    {
        memcpy(ServerManager::m_pGlobalBuffer, sData, szDataLen);
        ServerManager::m_pGlobalBuffer[szDataLen] = '|';
        ServerManager::m_pGlobalBuffer[szDataLen + 1] = '\0';
        GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, szDataLen + 1, nullptr, i32Profile, GlobalDataQueue::SendItem::TOPROFILE);
    }
    else
    {
        GlobalDataQueue::m_Ptr->SingleItemStore(sData, szDataLen, nullptr, i32Profile, GlobalDataQueue::SendItem::TOPROFILE);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToUser(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    User* u = ScriptGetUser(pLua, 2, "SendToUser");

    if (!u)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    const char* sData = lua_tolstring(pLua, 2, &szLen);

    if (szLen == 0 || szLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    if (sData[szLen - 1] != '|')
    {
        memcpy(ServerManager::m_pGlobalBuffer, sData, szLen);
        ServerManager::m_pGlobalBuffer[szLen] = '|';
        ServerManager::m_pGlobalBuffer[szLen + 1] = '\0';
        u->SendCharDelayed(ServerManager::m_pGlobalBuffer, szLen + 1);
    }
    else
    {
        u->SendCharDelayed(sData, szLen);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToAll(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szFromLen, szDataLen;
    const char* sFrom = lua_tolstring(pLua, 1, &szFromLen);
    const char* sData = lua_tolstring(pLua, 2, &szDataLen);

    if (szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", sFrom, sFrom, sData);
    if (iMsgLen > 0)
    {
        GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::SendItem::PM2ALL);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToNick(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szToLen, szFromLen, szDataLen;
    const char* sTo = lua_tolstring(pLua, 1, &szToLen);
    const char* sFrom = lua_tolstring(pLua, 2, &szFromLen);
    const char* sData = lua_tolstring(pLua, 3, &szDataLen);

    if (szToLen == 0 || szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    User* pUser = HashManager::m_Ptr->FindUser(std::string_view(sTo, szToLen));
    if (pUser)
    {
        pUser->SendFormat("Core.SendPmToNick", true, "$To: %s From: %s $<%s> %s|", sTo, sFrom, sFrom, sData);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToOps(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);

    if (lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szFromLen, szDataLen;
    const char* sFrom = lua_tolstring(pLua, 1, &szFromLen);
    const char* sData = lua_tolstring(pLua, 2, &szDataLen);

    if (szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", sFrom, sFrom, sData);
    if (iMsgLen > 0)
    {
        GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::SendItem::PM2OPS);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToProfile(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    const auto iProfile = LuaInt<int32_t>(pLua, 1);

    size_t szFromLen, szDataLen;
    const char* sFrom = lua_tolstring(pLua, 2, &szFromLen);
    const char* sData = lua_tolstring(pLua, 3, &szDataLen);

    if (szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", sFrom, sFrom, sData);
    if (iMsgLen > 0)
    {
        GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, iProfile, GlobalDataQueue::SendItem::PM2PROFILE);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToUser(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 3);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    User* pUser = ScriptGetUser(pLua, 3, "SendPmToUser");

    if (!pUser)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    size_t szFromLen, szDataLen;
    const char* sFrom = lua_tolstring(pLua, 2, &szFromLen);
    const char* sData = lua_tolstring(pLua, 3, &szDataLen);

    if (szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > g_ui32MaxMsgDataLen)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    pUser->SendFormat("Core.SendPmToUser", true, "$To: %s From: %s $<%s> %s|", pUser->m_sNick.c_str(), sFrom, sFrom, sData);

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------
#ifdef USE_FLYLINKDC_EXT_JSON
static int SetUserJson(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 2);
    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }
    User* pUser = ScriptGetUser(pLua, 2, "SetUserJson");

    if (!pUser)
    {
        lua_settop(pLua, 0);
        return 0;
    }
    size_t szDataLen;
    const char* sData = lua_tolstring(pLua, 2, &szDataLen);
    if (sData)
    {
        if (szDataLen > 5 * 1024)
        {
            lua_settop(pLua, 0);
            return 0;
        }
        pUser->initExtJSON(sData);
        pUser->SetExtJSONOriginal(sData, uint16_t(strlen(sData)));
    }
    lua_settop(pLua, 0);
    return 0;
}
#endif
//------------------------------------------------------------------------------
static int SetUserInfo(lua_State* pLua)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    LUA_CHECK_ARGS(pLua, 4);

    if (lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 4) != LUA_TBOOLEAN)
    {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 4, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        return 0;
    }

    User* pUser = ScriptGetUser(pLua, 4, "SetUserInfo");

    if (!pUser)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    const auto ui32DataToChange = LuaInt<uint32_t>(pLua, 2);

    if (ui32DataToChange > 9)
    {
        lua_settop(pLua, 0);
        return 0;
    }

    const bool bPermanent = lua_toboolean(pLua, 4) != 0;

    if (lua_type(pLua, 3) == LUA_TSTRING)
    {
        if (ui32DataToChange > 7)
        {
            lua_settop(pLua, 0);
            return 0;
        }

        size_t szDataLen;
        const char* sData = lua_tolstring(pLua, 3, &szDataLen);

        if (szDataLen > 64 || contains_any(std::string_view(sData, szDataLen), "$|"))
        {
            lua_settop(pLua, 0);
            return 0;
        }

        switch (ui32DataToChange)
        {
        case 0:
            User::SetUserInfo(pUser->m_sChangedDescriptionShort, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_DESCRIPTION_SHORT_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_SHORT_PERM;
            }
            break;
        case 1:
            User::SetUserInfo(pUser->m_sChangedDescriptionLong, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_DESCRIPTION_LONG_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_LONG_PERM;
            }
            break;
        case 2:
            User::SetUserInfo(pUser->m_sChangedTagShort, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_TAG_SHORT_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_SHORT_PERM;
            }
            break;
        case 3:
            User::SetUserInfo(pUser->m_sChangedTagLong, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_TAG_LONG_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_LONG_PERM;
            }
            break;
        case 4:
            User::SetUserInfo(pUser->m_sChangedConnectionShort, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_CONNECTION_SHORT_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_SHORT_PERM;
            }
            break;
        case 5:
            User::SetUserInfo(pUser->m_sChangedConnectionLong, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_CONNECTION_LONG_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_LONG_PERM;
            }
            break;
        case 6:
            User::SetUserInfo(pUser->m_sChangedEmailShort, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_EMAIL_SHORT_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_SHORT_PERM;
            }
            break;
        case 7:
            User::SetUserInfo(pUser->m_sChangedEmailLong, std::string_view(sData, szDataLen));
            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_EMAIL_LONG_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_LONG_PERM;
            }
            break;
        default:
            break;
        }
    }
    else if (lua_type(pLua, 3) == LUA_TNIL)
    {
        switch (ui32DataToChange)
        {
        case 0:
            pUser->m_sChangedDescriptionShort.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_SHORT_PERM;
            break;
        case 1:
            pUser->m_sChangedDescriptionLong.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_LONG_PERM;
            break;
        case 2:
            pUser->m_sChangedTagShort.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_SHORT_PERM;
            break;
        case 3:
            pUser->m_sChangedTagLong.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_LONG_PERM;
            break;
        case 4:
            pUser->m_sChangedConnectionShort.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_SHORT_PERM;
            break;
        case 5:
            pUser->m_sChangedConnectionLong.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_LONG_PERM;
            break;
        case 6:
            pUser->m_sChangedEmailShort.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_SHORT_PERM;
            break;
        case 7:
            pUser->m_sChangedEmailLong.clear();
            pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_LONG_PERM;
            break;
        case 8:
            pUser->m_ui64ChangedSharedSizeShort = pUser->m_ui64SharedSize;
            pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_SHORT_PERM;
            break;
        case 9:
            pUser->m_ui64ChangedSharedSizeLong = pUser->m_ui64SharedSize;
            pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_LONG_PERM;
            break;
        default:
            break;
        }
    }
    else if (lua_type(pLua, 3) == LUA_TNUMBER)
    {

        if (ui32DataToChange == 8)
        {
            pUser->m_ui64ChangedSharedSizeShort = LuaInt<uint64_t>(pLua, 3);

            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_SHARE_SHORT_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_SHORT_PERM;
            }
        }
        else if (ui32DataToChange == 9)
        {
            pUser->m_ui64ChangedSharedSizeLong = LuaInt<uint64_t>(pLua, 3);

            if (bPermanent)
            {
                pUser->m_ui32InfoBits |= User::INFOBIT_SHARE_LONG_PERM;
            }
            else
            {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_LONG_PERM;
            }
        }
        else
        {
            lua_settop(pLua, 0);
            return 0;
        }
    }
    else
    {
        luaL_error(pLua, "bad argument #3 to 'SetUserInfo' (string or number or nil expected, got %s)", lua_typename(pLua, lua_type(pLua, 3)));
        lua_settop(pLua, 0);
        return 0;
    }

    lua_settop(pLua, 0);
    return 0;
}

// alex82 ... HideUser / ������� �����
static int HideUser(lua_State* L)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    if (lua_gettop(L) != 2)
    {
        luaL_error(L, "bad argument count to 'HideUser' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    if (lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TBOOLEAN)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TBOOLEAN);

        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    const bool bHide = lua_toboolean(L, 2) != 0;

    User* u = ScriptGetUser(L, 2, "HideUser");

    lua_settop(L, 0);

    if (u)
    {
        if (bHide && !((u->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
        {
            u->m_ui32InfoBits |= User::INFOBIT_HIDDEN;

            if (std::to_underlying(u->m_ui8State) >= std::to_underlying(User::UserStates::STATE_ADDME_2LOOP))
            {
                Users::m_Ptr->DelFromNickList(u->m_sNick.c_str(),
                                              ((u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR &&
                                               !((u->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))); // + HideUserKey

                Users::m_Ptr->DelFromUserIP(u);

                {
                    std::string msg = "$Quit " + u->m_sNick + "|";
                    GlobalDataQueue::m_Ptr->AddQueueItem(msg, "", GlobalDataQueue::Cmd::QUIT);
                }

                if (((u->m_ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED))
                {
                    ServerManager::m_ui64TotalShare -= u->m_ui64SharedSize;
                    u->m_ui32BoolBits &= ~User::BIT_HAVE_SHARECOUNTED;
                }

                if (!u->m_sMyInfoLong.empty())
                {
                    if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 2)
                    {
                        Users::m_Ptr->DelFromMyInfosTag(u);
                    }
                }

                if (!u->m_sMyInfoShort.empty())
                {
                    if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 0)
                    {
                        Users::m_Ptr->DelFromMyInfos(u);
                    }
                }
            }
            if (std::to_underlying(u->m_ui8State) > std::to_underlying(User::UserStates::STATE_ADDME_2LOOP))
            {
                ServerManager::m_ui32Logged--;
            }
        }
        else if (!bHide && ((u->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
        {
            if (std::to_underlying(u->m_ui8State) > std::to_underlying(User::UserStates::STATE_ADDME_2LOOP))
            {
                ServerManager::m_ui32Logged++;
            }
            if (std::to_underlying(u->m_ui8State) >= std::to_underlying(User::UserStates::STATE_ADDME_2LOOP))
            {
                u->Add2Userlist();

                if (!((u->m_ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED))
                {
                    ServerManager::m_ui64TotalShare += u->m_ui64SharedSize;
                    u->m_ui32BoolBits |= User::BIT_HAVE_SHARECOUNTED;
                }

                GlobalDataQueue::m_Ptr->UserIPStore(u);

                switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
                {
                case 0:
                    GlobalDataQueue::m_Ptr->AddQueueItem(u->m_sMyInfoLong.data(), u->m_ui16MyInfoLongLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    break;
                case 1:
                    GlobalDataQueue::m_Ptr->AddQueueItem(
                        u->m_sMyInfoShort.data(), u->m_ui16MyInfoShortLen, u->m_sMyInfoLong.data(), u->m_ui16MyInfoLongLen, GlobalDataQueue::Cmd::MYINFO);
                    break;
                case 2:
                    GlobalDataQueue::m_Ptr->AddQueueItem(u->m_sMyInfoShort.data(), u->m_ui16MyInfoShortLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    break;
                default:
                    break;
                }
                // alex82 ... HideUserKey / ������ ���� �����
                if (((u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) &&
                    !((u->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
                {
                    GlobalDataQueue::m_Ptr->OpListStore(u->m_sNick.c_str());
                }
            }

            u->m_ui32InfoBits &= ~User::INFOBIT_HIDDEN;
        }
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

//------------------------------------------------------------------------------
// alex82 ... NoQuit / ��������� $Quit ��� �����
static int UserNoQuit(lua_State* L)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    if (lua_gettop(L) != 2)
    {
        luaL_error(L, "bad argument count to 'UserNoQuit' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    if (lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TBOOLEAN)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TBOOLEAN);

        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    const bool bNoQuit = lua_toboolean(L, 2) != 0;

    User* u = ScriptGetUser(L, 2, "UserNoQuit");

    lua_settop(L, 0);

    if (u)
    {
        if (bNoQuit && !((u->m_ui32InfoBits & User::INFOBIT_NO_QUIT) == User::INFOBIT_NO_QUIT))
        {
            u->m_ui32InfoBits |= User::INFOBIT_NO_QUIT;
        }
        else if (!bNoQuit && ((u->m_ui32InfoBits & User::INFOBIT_NO_QUIT) == User::INFOBIT_NO_QUIT))
        {
            u->m_ui32InfoBits &= ~User::INFOBIT_NO_QUIT;
        }
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

//------------------------------------------------------------------------------
// alex82 ... HideUserKey / ������ ���� �����
static int HideUserKey(lua_State* L)
{
    GlobalDataQueue::m_Ptr->PrometheusLuaInc(__func__);
    if (lua_gettop(L) != 2)
    {
        luaL_error(L, "bad argument count to 'HideUserKey' (2 expected, got %d)", lua_gettop(L));
        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    if (lua_type(L, 1) != LUA_TTABLE || lua_type(L, 2) != LUA_TBOOLEAN)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TBOOLEAN);

        lua_settop(L, 0);

        lua_pushnil(L);
        return 1;
    }

    const bool bHide = lua_toboolean(L, 2) != 0;

    User* u = ScriptGetUser(L, 2, "HideUserKey");

    lua_settop(L, 0);

    if (u)
    {
        if (bHide && !((u->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
        {

            u->m_ui32InfoBits |= User::INFOBIT_HIDE_KEY;

            if (((u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) &&
                std::to_underlying(u->m_ui8State) >= std::to_underlying(User::UserStates::STATE_ADDME_2LOOP) &&
                !((u->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
            {
                Users::m_Ptr->DelFromOpList(u->m_sNick.c_str());

                std::string msg = "$Quit " + u->m_sNick + "|";
                GlobalDataQueue::m_Ptr->AddQueueItem(msg, "", GlobalDataQueue::Cmd::QUIT);
                switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
                {
                case 0:
                    GlobalDataQueue::m_Ptr->AddQueueItem(u->m_sMyInfoLong.data(), u->m_ui16MyInfoLongLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    break;
                case 1:
                    GlobalDataQueue::m_Ptr->AddQueueItem(
                        u->m_sMyInfoShort.data(), u->m_ui16MyInfoShortLen, u->m_sMyInfoLong.data(), u->m_ui16MyInfoLongLen, GlobalDataQueue::Cmd::MYINFO);
                    break;
                case 2:
                    GlobalDataQueue::m_Ptr->AddQueueItem(u->m_sMyInfoShort.data(), u->m_ui16MyInfoShortLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                    break;
                default:
                    break;
                }
            }
        }
        else if (!bHide && ((u->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
        {
            if (((u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) &&
                std::to_underlying(u->m_ui8State) >= std::to_underlying(User::UserStates::STATE_ADDME_2LOOP) &&
                !((u->m_ui32InfoBits & User::INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN))
            {
                Users::m_Ptr->Add2OpList(u);
                GlobalDataQueue::m_Ptr->OpListStore(u->m_sNick.c_str());
            }

            u->m_ui32InfoBits &= ~User::INFOBIT_HIDE_KEY;
        }
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

//------------------------------------------------------------------------------

static const luaL_Reg CoreRegs[] = {{"Restart", Restart}, // NOLINT(modernize-avoid-c-arrays)
                                    {"Shutdown", Shutdown},
                                    {"ResumeAccepts", ResumeAccepts},
                                    {"SuspendAccepts", SuspendAccepts},
                                    {"RegBot", RegBot},
                                    {"UnregBot", UnregBot},
                                    {"GetBots", GetBots},
                                    {"GetActualUsersPeak", GetActualUsersPeak},
                                    {"GetMaxUsersPeak", GetMaxUsersPeak},
                                    {"GetCurrentSharedSize", GetCurrentSharedSize},
                                    {"GetHubIP", GetHubIP},
                                    {"GetHubIPs", GetHubIPs},
                                    {"GetHubSecAlias", GetHubSecAlias},
                                    {"GetPtokaXPath", GetPtokaXPath},
                                    {"GetUsersCount", GetUsersCount},
                                    {"GetUpTime", GetUpTime},
                                    {"GetOnlineNonOps", GetOnlineNonOps},
                                    {"GetOnlineOps", GetOnlineOps},
                                    {"GetOnlineRegs", GetOnlineRegs},
                                    {"GetOnlineUsers", GetOnlineUsers},
                                    {"GetUser", GetUser},
                                    {"GetUsers", GetUsers},
                                    {"GetUserAllData", GetUserAllData},
                                    {"GetUserData", GetUserData},
                                    {"GetUserValue", GetUserValue},
                                    {"Disconnect", Disconnect},
                                    {"Kick", Kick},
                                    {"Redirect", Redirect},
                                    {"DefloodWarn", DefloodWarn},
                                    {"SendToAll", SendToAll},
                                    {"SendToNick", SendToNick},
                                    {"SendToOpChat", SendToOpChat},
                                    {"SendToOps", SendToOps},
                                    {"SendToProfile", SendToProfile},
                                    {"SendToUser", SendToUser},
                                    {"SendPmToAll", SendPmToAll},
                                    {"SendPmToNick", SendPmToNick},
                                    {"SendPmToOps", SendPmToOps},
                                    {"SendPmToProfile", SendPmToProfile},
                                    {"SendPmToUser", SendPmToUser},
                                    {"SetUserInfo", SetUserInfo},
                                    // alex82 ... HideUser / ������� �����
                                    {"HideUser", HideUser},
                                    // alex82 ... NoQuit / ��������� $Quit ��� �����
                                    {"UserNoQuit", UserNoQuit},
                                    // alex82 ... HideUserKey / ������ ���� �����
                                    {"HideUserKey", HideUserKey},
#ifdef USE_FLYLINKDC_EXT_JSON
                                    {"SetUserJson", SetUserJson},
#endif
                                    {nullptr, nullptr}};
//---------------------------------------------------------------------------

int RegCore(lua_State* pLua)
{
    luaL_newlib(pLua, CoreRegs);

    lua_pushliteral(pLua, "Version");
    lua_pushliteral(pLua, PtokaXVersionString);
    lua_settable(pLua, -3);
    lua_pushliteral(pLua, "BuildNumber");
    lua_pushinteger(pLua, static_cast<lua_Integer>(strtoull(BUILD_NUMBER, nullptr, 10)));

    lua_settable(pLua, -3);

    return 1;
}
//---------------------------------------------------------------------------
