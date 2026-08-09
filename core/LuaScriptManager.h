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

//------------------------------------------------------------------------------
#ifndef LuaScriptManagerH
#define LuaScriptManagerH
//------------------------------------------------------------------------------
struct lua_State;
struct Script;
struct ScriptTimer;
struct User;
struct DcCommand;
//------------------------------------------------------------------------------

#include <atomic>
#include <list>
#include <memory>
#include <vector>

class ScriptManager
{
private:
    void AddRunningScript(Script* pScript);
    void RemoveRunningScript(Script* pScript);
    void RebuildRunningList();

    void LoadXML();

public:
    static std::unique_ptr<ScriptManager> m_Ptr;

    // Non-owning list of currently-running scripts, kept in m_ppScriptTable
    // order. Memory is owned by m_ppScriptTable (freed in the destructor).
    std::list<Script*> m_RunningScriptList;

    std::vector<Script*> m_ppScriptTable;
    User* m_pActualUser = nullptr;

    std::list<std::unique_ptr<ScriptTimer>> m_TimerList;
    // Incremented on every timer add/remove so ScriptOnTimer can detect the
    // list being modified from inside a Lua callback and bail out safely.
    uint64_t m_ui64TimerListGen = 0;

    uint8_t m_ui8BotsCount = 0;

    std::atomic<bool> m_bMoved{false};

    enum LuaArrivals : uint8_t
    {
        CHAT_ARRIVAL,
        KEY_ARRIVAL,
        VALIDATENICK_ARRIVAL,
        PASSWORD_ARRIVAL,
        VERSION_ARRIVAL,
        GETNICKLIST_ARRIVAL,
        MYINFO_ARRIVAL,
        GETINFO_ARRIVAL,
        SEARCH_ARRIVAL,
        TO_ARRIVAL,
        CONNECTTOME_ARRIVAL,
        MULTICONNECTTOME_ARRIVAL,
        REVCONNECTTOME_ARRIVAL,
        SR_ARRIVAL,
        UDP_SR_ARRIVAL,
        KICK_ARRIVAL,
        OPFORCEMOVE_ARRIVAL,
        SUPPORTS_ARRIVAL,
        BOTINFO_ARRIVAL,
        CLOSE_ARRIVAL,
        UNKNOWN_ARRIVAL
#ifdef USE_FLYLINKDC_EXT_JSON
        ,
        EXTJSON_ARRIVAL
#endif
        // alex82 ... More arrivals
        ,
        BAD_PASS_ARRIVAL,
        VALIDATE_DENIDE_ARRIVAL
    };

    ScriptManager(const ScriptManager&) = delete;
    auto operator=(const ScriptManager&) -> ScriptManager& = delete;

    ScriptManager();
    ~ScriptManager();

    void Start();
    void Stop();

    void SaveScripts();

    void CheckForDeletedScripts();
    void CheckForNewScripts();

    void Restart();
    [[nodiscard]] auto FindScript(const char* sName) -> Script*;
    [[nodiscard]] auto FindScript(const lua_State* pLua) -> Script*;
    [[nodiscard]] auto FindScriptIdx(const char* sName) -> uint8_t;

    [[nodiscard]] auto AddScript(const char* sName, bool bEnabled, bool bNew) -> bool;

    [[nodiscard]] auto StartScript(Script* pScript, bool bEnable) -> bool;
    void StopScript(Script* pScript, bool bDisable);

    void MoveScript(uint8_t ui8ScriptPosInTbl, bool bUp);

    void DeleteScript(uint8_t ui8ScriptPosInTbl);

    void OnStartup();
    void OnExit(bool bForce = false);
    [[nodiscard]] auto Arrival(DcCommand* pDcCommand, uint8_t ui8Type) -> bool;
    [[nodiscard]] auto UserConnected(User* pUser) -> bool;
    void UserDisconnected(User* pUser, Script* pScript = nullptr);

    void PrepareMove(lua_State* pLua);
};
//------------------------------------------------------------------------------

#endif
