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
#ifndef LuaScriptH
#define LuaScriptH
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------
struct User;
struct Script;
//---------------------------------------------------------------------------

struct ScriptBot
{
    ScriptBot* m_pPrev = nullptr;
    ScriptBot* m_pNext = nullptr;

    std::string m_sNick;
    std::string m_sMyINFO;

    bool m_bIsOP = false;

    ScriptBot();
    ~ScriptBot();

    ScriptBot(const ScriptBot&) = delete;
    auto operator=(const ScriptBot&) -> ScriptBot& = delete;

    [[nodiscard]] static auto
    CreateScriptBot(const char* sBotNick, size_t szNickLen, const char* sDescription, size_t szDscrLen, const char* sEmail, size_t szEmailLen, bool bOP)
        -> ScriptBot*;
    // alex82 ... RegBot / �������� �������������� ������� ��� �������� ���� � ����������� $MyINFO
    [[nodiscard]] static auto CreateScriptBot(const char* sBotNick, size_t szNickLen, const char* sBotMyINFO, size_t szMyINFOLen, bool bOP) -> ScriptBot*;
};
//------------------------------------------------------------------------------

struct ScriptTimer
{
    uint64_t m_ui64Interval = 0;
    uint64_t m_ui64LastTick = 0;

    lua_State* m_pLua = nullptr;

    std::string m_sFunctionName;

    int m_iFunctionRef = 0;

    static constexpr const char m_sDefaultTimerFunc[] = "OnTimer"; // NOLINT(modernize-avoid-c-arrays)

    ScriptTimer() = default;
    ~ScriptTimer() = default;

    ScriptTimer(const ScriptTimer&) = delete;
    auto operator=(const ScriptTimer&) -> ScriptTimer& = delete;

    [[nodiscard]] static auto CreateScriptTimer(const char* sFunctName, size_t szLen, int iRef, lua_State* pLuaState) -> ScriptTimer*;
};
//------------------------------------------------------------------------------

struct Script
{
    ScriptBot* m_pBotList = nullptr;

    lua_State* m_pLua = nullptr;

    std::string m_sName;

    uint32_t m_ui32DataArrivals = UINT32_MAX;

    uint16_t m_ui16Functions = UINT16_MAX;

    uint32_t m_ui32LuaCallCount = 0;
    uint64_t m_ui64LuaTimeNsec = 0;

    bool m_bEnabled = false;
    bool m_bProcessed = false;

    enum LuaFunctions : uint16_t
    {
        ONSTARTUP = 0x1,
        ONEXIT = 0x2,
        ONERROR = 0x4,
        USERCONNECTED = 0x8,
        REGCONNECTED = 0x10,
        OPCONNECTED = 0x20,
        USERDISCONNECTED = 0x40,
        REGDISCONNECTED = 0x80,
        OPDISCONNECTED = 0x100
    };

    Script() = default;
    ~Script();

    Script(const Script&) = delete;
    auto operator=(const Script&) -> Script& = delete;

    [[nodiscard]] static auto CreateScript(const char* sName, bool enabled) -> Script*;
};
//------------------------------------------------------------------------------

[[nodiscard]] auto ScriptStart(Script* pScript) -> bool;
void ScriptStop(Script* pScript);

[[nodiscard]] auto ScriptGetGC(Script* pScript) -> int;

void ScriptOnStartup(Script* pScript);
void ScriptOnExit(Script* pScript);

void ScriptPushUser(lua_State* pLua, User* pUser, bool bFullTable = false);
void ScriptPushUserExtended(lua_State* pLua, User* pUser, int iTable);

[[nodiscard]] auto ScriptGetUser(lua_State* pLua, int iTop, const char* sFunction) -> User*;

void ScriptError(Script* pScript);

void ScriptOnTimer(uint64_t ui64ActualMillis);

[[nodiscard]] auto ScriptTraceback(lua_State* pLua) -> int;
//------------------------------------------------------------------------------

#endif
