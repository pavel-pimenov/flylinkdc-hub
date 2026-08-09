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
#include "stdinc.h"
#include <sstream>
#include <utility>
//------------------------------------------------------------------------------
#include "LuaInc.h"
//------------------------------------------------------------------------------
#include "LuaScriptManager.h"
//------------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
#include <tinyxml2.h>
#include "GlobalDataQueue.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#include "LuaScript.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::unique_ptr<ScriptManager> ScriptManager::m_Ptr;
//------------------------------------------------------------------------------

void ScriptManager::LoadXML()
{
    // PPK ... first start all script in order from xml file
    tinyxml2::XMLDocument doc;
    if (LoadXmlConfig(doc, "Scripts.xml"))
    {
        tinyxml2::XMLHandle cfg(&doc);
        tinyxml2::XMLNode* scripts = cfg.FirstChildElement("Scripts").ToNode();
        if (scripts)
        {
            tinyxml2::XMLElement* child = scripts->FirstChildElement();
            while (child)
            {
                const char* name = XmlGetRequiredText(child, "Name");
                if (!name)
                {
                    child = child->NextSiblingElement();
                    continue;
                }

                if (!FileExist((ServerManager::m_sScriptPath + name).c_str()))
                {
                    child = child->NextSiblingElement();
                    continue;
                }

                int iEnabled = 0;
                if (!XmlGetRequiredInt(child, "Enabled", iEnabled))
                {
                    child = child->NextSiblingElement();
                    continue;
                }
                const bool enabled = iEnabled != 0;

                if (FindScript(name))
                {
                    child = child->NextSiblingElement();
                    continue;
                }

                if (!AddScript(name, enabled, false))
                {
                    LogDbgErr("[SCRIPTS] Failed to load script from XML: {}", name);
                }
                child = child->NextSiblingElement();
            }
        }
    }
}
//------------------------------------------------------------------------------

ScriptManager::ScriptManager()
{

    if (!FileExist((ServerManager::m_sPath + "/cfg/Scripts.pxt").c_str()))
    {
        LoadXML();

        return;
    }

    const std::string sPath = ServerManager::m_sPath + "/cfg/Scripts.pxt";
    std::string sData;
    if (!ReadWholeFile(sPath, sData))
    {
        LogInfo("Error loading file Scripts.pxt: file not found");
        exit(EXIT_FAILURE);
    }

    std::istringstream iss(sData);
    std::string sLine;
    while (std::getline(iss, sLine))
    {
        if (sLine.empty() || sLine.starts_with('#') || sLine.starts_with('\n'))
        {
            continue;
        }

        // Trim trailing whitespace/CR (e.g. "\r" on CRLF files).
        while (!sLine.empty() && isspace(static_cast<unsigned char>(sLine.back())))
        {
            sLine.pop_back();
        }

        if (sLine.size() < 7)
        {
            continue;
        }

        // Line format: "<name>.lua\t=\t<flag>" where <flag> is '0' or '1'.
        // The enabled flag is the last non-space character.
        const char cFlag = sLine.back();
        if (cFlag != '1' && cFlag != '0')
        {
            continue;
        }

        // The script name is everything up to the first whitespace or '='.
        std::string sName;
        for (const char c : sLine)
        {
            if (isspace(static_cast<unsigned char>(c)) || c == '=')
            {
                break;
            }
            sName.push_back(c);
        }

        if (sName.empty())
        {
            continue;
        }

        if (!FileExist((ServerManager::m_sScriptPath + sName).c_str()) || FindScript(sName.c_str()))
        {
            continue;
        }

        if (!AddScript(sName.c_str(), cFlag == '1', false))
        {
            LogDbgErr("[SCRIPTS] Failed to load script from Scripts.pxt: {}", sName);
        }
    }
}
//------------------------------------------------------------------------------

ScriptManager::~ScriptManager()
{
    m_RunningScriptList.clear();

    for (auto* pScript : m_ppScriptTable)
    {
        std::unique_ptr<Script> guard(pScript);
    }

    m_ppScriptTable.clear();

    m_pActualUser = nullptr;

    m_ui8BotsCount = 0;
}
//------------------------------------------------------------------------------

void ScriptManager::Start()
{
    m_ui8BotsCount = 0;
    m_pActualUser = nullptr;

    // PPK ... first look for deleted and new scripts
    CheckForDeletedScripts();
    CheckForNewScripts();

    // PPK ... second start all enabled scripts
    for (auto* pScript : m_ppScriptTable)
    {
        if (pScript->m_bEnabled)
        {
            if (ScriptStart(pScript))
            {
                AddRunningScript(pScript);
            }
            else
            {
                pScript->m_bEnabled = false;
            }
        }
    }
}
//------------------------------------------------------------------------------

bool ScriptManager::AddScript(const char* sName, const bool bEnabled, const bool /*bNew*/)
{
    if (static_cast<uint8_t>(m_ppScriptTable.size()) == 254)
    {
        return false;
    }

    Script* pNewScript = Script::CreateScript(sName, bEnabled);

    if (!pNewScript)
    {
        LogDbg("[MEM] Cannot allocate new Script in ScriptManager::AddScript");

        return false;
    }

    m_ppScriptTable.push_back(pNewScript);

    return true;
}
//------------------------------------------------------------------------------

void ScriptManager::Stop()
{
    // ScriptStop() calls RemoveRunningScript() which erases from
    // m_RunningScriptList, so iterate over a snapshot.
    const std::list<Script*> snapshot = m_RunningScriptList;

    for (Script* S : snapshot)
    {
        ScriptStop(S);
    }

    m_RunningScriptList.clear();

    m_pActualUser = nullptr;
}
//------------------------------------------------------------------------------

void ScriptManager::AddRunningScript(Script* pScript)
{
    // Keep the running list ordered by position in m_ppScriptTable: insert
    // before the first running script whose table index is greater.
    uint8_t ui8dx = 0;
    for (uint8_t i = 0; auto* p : m_ppScriptTable)
    {
        if (p == pScript)
        {
            ui8dx = i;
            break;
        }
        i++;
    }

    for (auto it = m_RunningScriptList.begin(); it != m_RunningScriptList.end(); ++it)
    {
        uint8_t ui8OtherIdx = 0;
        for (uint8_t i = 0; auto* p : m_ppScriptTable)
        {
            if (p == *it)
            {
                ui8OtherIdx = i;
                break;
            }
            i++;
        }

        if (ui8OtherIdx > ui8dx)
        {
            m_RunningScriptList.insert(it, pScript);
            return;
        }
    }

    m_RunningScriptList.push_back(pScript);
}
//------------------------------------------------------------------------------

void ScriptManager::RemoveRunningScript(Script* pScript)
{
    for (auto it = m_RunningScriptList.begin(); it != m_RunningScriptList.end(); ++it)
    {
        if (*it == pScript)
        {
            m_RunningScriptList.erase(it);
            return;
        }
    }
}
//------------------------------------------------------------------------------

void ScriptManager::RebuildRunningList()
{
    // Rebuild the running list from m_ppScriptTable order, keeping only
    // scripts that are actually running (have a live Lua state).
    m_RunningScriptList.clear();
    for (auto* pScript : m_ppScriptTable)
    {
        if (pScript->m_pLua)
        {
            m_RunningScriptList.push_back(pScript);
        }
    }
}
//------------------------------------------------------------------------------

void ScriptManager::SaveScripts()
{
    std::string sOut = "#\n# PtokaX scripts settings file\n#\n\n";

    for (const auto* pScript : m_ppScriptTable)
    {
        if (!FileExist((ServerManager::m_sScriptPath + pScript->m_sName).c_str()))
        {
            continue;
        }

        sOut += pScript->m_sName;
        sOut += "\t=\t";
        sOut += (pScript->m_bEnabled ? '1' : '0');
        sOut += '\n';
    }

    const std::string sPath = ServerManager::m_sPath + "/cfg/Scripts.pxt";
    if (!WriteWholeFile(sPath, sOut))
    {
        LogError("WriteWholeFile failed in ScriptManager::SaveScripts");
    }
}
//------------------------------------------------------------------------------

void ScriptManager::CheckForDeletedScripts()
{
    uint8_t ui8i = 0;

    while (ui8i < static_cast<uint8_t>(m_ppScriptTable.size()))
    {
        if (FileExist((ServerManager::m_sScriptPath + m_ppScriptTable[ui8i]->m_sName).c_str()) || m_ppScriptTable[ui8i]->m_pLua)
        {
            ui8i++;
            continue;
        }

        std::unique_ptr<Script> guard(m_ppScriptTable[ui8i]);

        m_ppScriptTable.erase(m_ppScriptTable.begin() + ui8i);
    }
}
//------------------------------------------------------------------------------

void ScriptManager::CheckForNewScripts()
{
    DIR* p_scriptdir = opendir(ServerManager::m_sScriptPath.c_str());

    if (!p_scriptdir)
    {
        LogWarn("CheckForNewScripts: cannot open scripts directory {}", ServerManager::m_sScriptPath);
        return;
    }

    struct dirent* p_dirent;
    struct stat s_buf;

    while ((p_dirent = readdir(p_scriptdir)))
    {
        if (stat((ServerManager::m_sScriptPath + p_dirent->d_name).c_str(), &s_buf) != 0 || (s_buf.st_mode & S_IFDIR) != 0 ||
            strcasecmp(p_dirent->d_name + (strlen(p_dirent->d_name) - 4), ".lua") != 0)
        {
            continue;
        }

        if (FindScript(p_dirent->d_name))
        {
            continue;
        }

        if (!AddScript(p_dirent->d_name, false, false))
        {
            LogDbgErr("[SCRIPTS] Failed to load script from directory: {}", p_dirent->d_name);
        }
    }

    closedir(p_scriptdir);
}
//------------------------------------------------------------------------------

void ScriptManager::Restart()
{
    OnExit();
    Stop();

    CheckForDeletedScripts();

    Start();
    OnStartup();
}
//------------------------------------------------------------------------------

Script* ScriptManager::FindScript(const char* sName)
{
    for (auto* pScript : m_ppScriptTable)
    {
        if (iequals(pScript->m_sName, sName))
        {
            return pScript;
        }
    }

    return nullptr;
}
//------------------------------------------------------------------------------

Script* ScriptManager::FindScript(const lua_State* pLua)
{
    for (Script* cur : m_RunningScriptList)
    {
        if (cur->m_pLua == pLua)
        {
            return cur;
        }
    }

    return nullptr;
}
//------------------------------------------------------------------------------

uint8_t ScriptManager::FindScriptIdx(const char* sName)
{
    for (size_t ui8i = 0; ui8i < m_ppScriptTable.size(); ui8i++)
    {
        if (iequals(m_ppScriptTable[ui8i]->m_sName, sName))
        {
            return ui8i;
        }
    }

    return static_cast<uint8_t>(m_ppScriptTable.size());
}
//------------------------------------------------------------------------------

bool ScriptManager::StartScript(Script* pScript, const bool bEnable)
{
    uint8_t ui8dx = 255;
    for (size_t ui8i = 0; ui8i < m_ppScriptTable.size(); ui8i++)
    {
        if (pScript == m_ppScriptTable[ui8i])
        {
            ui8dx = ui8i;
            break;
        }
    }

    if (ui8dx == 255)
    {
        return false;
    }

    if (bEnable)
    {
        pScript->m_bEnabled = true;
    }

    if (!ScriptStart(pScript))
    {
        pScript->m_bEnabled = false;
        return false;
    }

    AddRunningScript(pScript);

    if (ServerManager::m_bServerRunning)
    {
        ScriptOnStartup(pScript);
    }

    return true;
}
//------------------------------------------------------------------------------

void ScriptManager::StopScript(Script* pScript, const bool bDisable)
{
    if (bDisable)
    {
        pScript->m_bEnabled = false;
    }

    RemoveRunningScript(pScript);

    if (ServerManager::m_bServerRunning)
    {
        ScriptOnExit(pScript);
    }

    ScriptStop(pScript);
}
//------------------------------------------------------------------------------

void ScriptManager::MoveScript(uint8_t ui8ScriptPosInTbl, bool bUp)
{
    if (bUp)
    {
        if (ui8ScriptPosInTbl == 0)
        {
            return;
        }

        std::swap(m_ppScriptTable[ui8ScriptPosInTbl], m_ppScriptTable[ui8ScriptPosInTbl - 1]);
    }
    else
    {
        if (ui8ScriptPosInTbl == static_cast<uint8_t>(m_ppScriptTable.size()) - 1)
        {
            return;
        }

        std::swap(m_ppScriptTable[ui8ScriptPosInTbl], m_ppScriptTable[ui8ScriptPosInTbl + 1]);
    }

    // Reorder the running list to match the new table order.
    RebuildRunningList();
}
//------------------------------------------------------------------------------

void ScriptManager::DeleteScript(const uint8_t ui8ScriptPosInTbl)
{
    Script* pScript = m_ppScriptTable[ui8ScriptPosInTbl];

    if (pScript->m_pLua)
    {
        StopScript(pScript, false);
    }

    if (FileExist((ServerManager::m_sScriptPath + pScript->m_sName).c_str()))
    {
        unlink((ServerManager::m_sScriptPath + pScript->m_sName).c_str());
    }

    std::unique_ptr<Script> guard(pScript);

    m_ppScriptTable.erase(m_ppScriptTable.begin() + ui8ScriptPosInTbl);
}
//------------------------------------------------------------------------------

void ScriptManager::OnStartup()
{
    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        return;
    }

    m_pActualUser = nullptr;
    m_bMoved = false;

    // A callback may reorder the running list (m_bMoved via MoveScript, which
    // rebuilds m_RunningScriptList); iterate a snapshot. Scripts survive because
    // m_ppScriptTable owns them; the m_bProcessed guard prevents double-runs.
    const std::list<Script*> snapshot = m_RunningScriptList;

    for (Script* pScript : snapshot)
    {
        if (pScript->m_pLua && ((pScript->m_ui16Functions & Script::ONSTARTUP) == Script::ONSTARTUP) && (!m_bMoved || !pScript->m_bProcessed))
        {
            pScript->m_bProcessed = true;
            ScriptOnStartup(pScript);
        }
    }
}
//------------------------------------------------------------------------------

void ScriptManager::OnExit(const bool bForce /* = false*/)
{
    if (!bForce && !SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        return;
    }

    m_pActualUser = nullptr;
    m_bMoved = false;

    const std::list<Script*> snapshot = m_RunningScriptList;

    for (Script* pScript : snapshot)
    {
        if (pScript->m_pLua && ((pScript->m_ui16Functions & Script::ONEXIT) == Script::ONEXIT) && (!m_bMoved || !pScript->m_bProcessed))
        {
            pScript->m_bProcessed = true;
            ScriptOnExit(pScript);
        }
    }
}
//------------------------------------------------------------------------------

bool ScriptManager::Arrival(DcCommand* pDcCommand, const uint8_t ui8Type)
{
    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        return false;
    }

    static constexpr uint32_t iLuaArrivalBits[] = {0x1, // NOLINT(modernize-avoid-c-arrays)
                                               0x2,
                                               0x4,
                                               0x8,
                                               0x10,
                                               0x20,
                                               0x40,
                                               0x80,
                                               0x100,
                                               0x200,
                                               0x400,
                                               0x800,
                                               0x1000,
                                               0x2000,
                                               0x4000,
                                               0x8000,
                                               0x10000,
                                               0x20000,
                                               0x40000,
                                               0x80000,
                                               0x100000
#ifdef USE_FLYLINKDC_EXT_JSON
                                               ,
                                               0x200000
#endif
                                               // alex82... More arrivals
                                               ,
                                               0x400000,
                                               0x800000};

    m_bMoved = false;

    int iTop = 0, iTraceback = 0;

    // A callback may reorder the running list (rebuilds m_RunningScriptList);
    // iterate a snapshot. Scripts survive because m_ppScriptTable owns them.
    const std::list<Script*> snapshot = m_RunningScriptList;

    for (Script* cur : snapshot)
    {
        if (!cur->m_pLua)
        {
            continue;
        }

        // if any of the scripts returns a nonzero value,
        // then stop for all other scripts
        if (((cur->m_ui32DataArrivals & iLuaArrivalBits[ui8Type]) == iLuaArrivalBits[ui8Type]) && (!m_bMoved || !cur->m_bProcessed))
        {
            cur->m_bProcessed = true;

            lua_pushcfunction(cur->m_pLua, ScriptTraceback);
            iTraceback = lua_gettop(cur->m_pLua);

            // PPK ... table of arrivals
            static constexpr const char* arrival[] = {"ChatArrival",
                                            "KeyArrival",
                                            "ValidateNickArrival",
                                            "PasswordArrival",
                                            "VersionArrival",
                                            "GetNickListArrival",
                                            "MyINFOArrival",
                                            "GetINFOArrival",
                                            "SearchArrival",
                                            "ToArrival",
                                            "ConnectToMeArrival",
                                            "MultiConnectToMeArrival",
                                            "RevConnectToMeArrival",
                                            "SRArrival",
                                            "UDPSRArrival",
                                            "KickArrival",
                                            "OpForceMoveArrival",
                                            "SupportsArrival",
                                            "BotINFOArrival",
                                            "CloseArrival",
                                            "UnknownArrival"
#ifdef USE_FLYLINKDC_EXT_JSON
                                            ,
                                            "ExtJSONArrival"
#endif
                                            // alex82 ... More arrivals
                                            ,
                                            "BadPassArrival",
                                            "ValidateDenideArrival"};

            lua_getglobal(cur->m_pLua, arrival[ui8Type]);
            iTop = lua_gettop(cur->m_pLua);
            if (lua_isfunction(cur->m_pLua, iTop) == 0)
            {
                cur->m_ui32DataArrivals &= ~iLuaArrivalBits[ui8Type];

                lua_settop(cur->m_pLua, 0);
                continue;
            }

            m_pActualUser = pDcCommand->m_pUser;

            lua_checkstack(cur->m_pLua, 2); // we need 2 empty slots in stack, check it to be sure

            ScriptPushUser(cur->m_pLua, pDcCommand->m_pUser);                                   // usertable
            lua_pushlstring(cur->m_pLua, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen); // sData

            // two passed parameters, zero returned
            struct timespec ts_before, ts_after;
            clock_gettime(CLOCK_MONOTONIC, &ts_before);
            const int lua_res = lua_pcall(cur->m_pLua, 2, LUA_MULTRET, iTraceback);
            clock_gettime(CLOCK_MONOTONIC, &ts_after);
            cur->m_ui32LuaCallCount++;
            cur->m_ui64LuaTimeNsec += (ts_after.tv_sec - ts_before.tv_sec) * 1000000000ULL + (ts_after.tv_nsec - ts_before.tv_nsec);
            if (lua_res != 0)
            {
                ScriptError(cur);

                lua_settop(cur->m_pLua, 0);
                continue;
            }

            m_pActualUser = nullptr;

            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return it

            iTop = lua_gettop(cur->m_pLua);

            // no return value
            if (iTop == 0)
            {
                continue;
            }

            if (lua_type(cur->m_pLua, iTop) != LUA_TBOOLEAN || lua_toboolean(cur->m_pLua, iTop) == 0)
            {
                lua_settop(cur->m_pLua, 0);
                continue;
            }

            // clear the stack for sure
            lua_settop(cur->m_pLua, 0);

            return true; // true means DO NOT process data by the hub's core
        }
    }

    return false;
}
//------------------------------------------------------------------------------

bool ScriptManager::UserConnected(User* pUser)
{
    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        return false;
    }

    uint8_t ui8Type = 0; // User
    if (pUser->m_i32Profile != -1)
    {
        if (!((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            ui8Type = 1; // Reg
        }
        else
        {
            ui8Type = 2; // OP
        }
    }

    m_bMoved = false;

    int iTop = 0, iTraceback = 0;

    // A callback may reorder the running list (rebuilds m_RunningScriptList);
    // iterate a snapshot. Scripts survive because m_ppScriptTable owns them.
    const std::list<Script*> snapshot = m_RunningScriptList;

    for (Script* cur : snapshot)
    {
        if (!cur->m_pLua)
        {
            continue;
        }

        static constexpr uint32_t iConnectedBits[] = {Script::USERCONNECTED, Script::REGCONNECTED, Script::OPCONNECTED};

        if (((cur->m_ui16Functions & iConnectedBits[ui8Type]) == iConnectedBits[ui8Type]) && (!m_bMoved || !cur->m_bProcessed))
        {
            cur->m_bProcessed = true;

            lua_pushcfunction(cur->m_pLua, ScriptTraceback);
            iTraceback = lua_gettop(cur->m_pLua);

            // PPK ... table of connected functions
            static constexpr const char* ConnectedFunction[] = {"UserConnected", "RegConnected", "OpConnected"};

            lua_getglobal(cur->m_pLua, ConnectedFunction[ui8Type]);
            iTop = lua_gettop(cur->m_pLua);
            if (lua_isfunction(cur->m_pLua, iTop) == 0)
            {
                switch (ui8Type)
                {
                case 0:
                    cur->m_ui16Functions &= ~Script::USERCONNECTED;
                    break;
                case 1:
                    cur->m_ui16Functions &= ~Script::REGCONNECTED;
                    break;
                case 2:
                    cur->m_ui16Functions &= ~Script::OPCONNECTED;
                    break;
                default:
                    break;
                }

                lua_settop(cur->m_pLua, 0);
                continue;
            }

            m_pActualUser = pUser;

            lua_checkstack(cur->m_pLua, 1); // we need 1 empty slots in stack, check it to be sure

            ScriptPushUser(cur->m_pLua, pUser); // usertable

            // 1 passed parameters, zero returned
            struct timespec ts_before, ts_after;
            clock_gettime(CLOCK_MONOTONIC, &ts_before);
            const int lua_res = lua_pcall(cur->m_pLua, 1, LUA_MULTRET, iTraceback);
            clock_gettime(CLOCK_MONOTONIC, &ts_after);
            cur->m_ui32LuaCallCount++;
            cur->m_ui64LuaTimeNsec += (ts_after.tv_sec - ts_before.tv_sec) * 1000000000ULL + (ts_after.tv_nsec - ts_before.tv_nsec);
            if (lua_res != 0)
            {
                ScriptError(cur);

                lua_settop(cur->m_pLua, 0);
                continue;
            }

            m_pActualUser = nullptr;

            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return

            iTop = lua_gettop(cur->m_pLua);

            // no return value
            if (iTop == 0)
            {
                continue;
            }

            if (lua_type(cur->m_pLua, iTop) != LUA_TBOOLEAN || lua_toboolean(cur->m_pLua, iTop) == 0)
            {
                lua_settop(cur->m_pLua, 0);
                continue;
            }

            // clear the stack for sure
            lua_settop(cur->m_pLua, 0);

            UserDisconnected(pUser, cur);

            return true; // means DO NOT process by next scripts
        }
    }

    return false;
}
//------------------------------------------------------------------------------

void ScriptManager::UserDisconnected(User* pUser, [[maybe_unused]] Script* pScript /* = nullptr*/)
{
    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        return;
    }

    uint8_t ui8Type = 0; // User
    if (pUser->m_i32Profile != -1)
    {
        if (!((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            ui8Type = 1; // Reg
        }
        else
        {
            ui8Type = 2; // OP
        }
    }

    m_bMoved = false;

    int iTop = 0, iTraceback = 0;

    // A callback may reorder the running list (rebuilds m_RunningScriptList);
    // iterate a snapshot. Scripts survive because m_ppScriptTable owns them.
    const std::list<Script*> snapshot = m_RunningScriptList;

    for (Script* cur : snapshot)
    {
        if (!cur->m_pLua)
        {
            continue;
        }

        static constexpr uint32_t iDisconnectedBits[] = {Script::USERDISCONNECTED, Script::REGDISCONNECTED, Script::OPDISCONNECTED};

        if (((cur->m_ui16Functions & iDisconnectedBits[ui8Type]) == iDisconnectedBits[ui8Type]) && (!m_bMoved || !cur->m_bProcessed))
        {
            cur->m_bProcessed = true;

            lua_pushcfunction(cur->m_pLua, ScriptTraceback);
            iTraceback = lua_gettop(cur->m_pLua);

            // PPK ... table of disconnected functions
            static constexpr const char* DisconnectedFunction[] = {"UserDisconnected", "RegDisconnected", "OpDisconnected"};

            lua_getglobal(cur->m_pLua, DisconnectedFunction[ui8Type]);
            iTop = lua_gettop(cur->m_pLua);
            if (lua_isfunction(cur->m_pLua, iTop) == 0)
            {
                switch (ui8Type)
                {
                case 0:
                    cur->m_ui16Functions &= ~Script::USERDISCONNECTED;
                    break;
                case 1:
                    cur->m_ui16Functions &= ~Script::REGDISCONNECTED;
                    break;
                case 2:
                    cur->m_ui16Functions &= ~Script::OPDISCONNECTED;
                    break;
                default:
                    break;
                }

                lua_settop(cur->m_pLua, 0);
                continue;
            }

            m_pActualUser = pUser;

            lua_checkstack(cur->m_pLua, 1); // we need 1 empty slots in stack, check it to be sure

            ScriptPushUser(cur->m_pLua, pUser); // usertable

            // 1 passed parameters, zero returned
            struct timespec ts_before, ts_after;
            clock_gettime(CLOCK_MONOTONIC, &ts_before);
            const int lua_res = lua_pcall(cur->m_pLua, 1, 0, iTraceback);
            clock_gettime(CLOCK_MONOTONIC, &ts_after);
            cur->m_ui32LuaCallCount++;
            cur->m_ui64LuaTimeNsec += (ts_after.tv_sec - ts_before.tv_sec) * 1000000000ULL + (ts_after.tv_nsec - ts_before.tv_nsec);
            if (lua_res != 0)
            {
                ScriptError(cur);

                lua_settop(cur->m_pLua, 0);
                continue;
            }

            m_pActualUser = nullptr;

            // clear the stack for sure
            lua_settop(cur->m_pLua, 0);
        }
    }
}
//------------------------------------------------------------------------------

void ScriptManager::PrepareMove(lua_State* pLua)
{
    if (m_bMoved)
    {
        return;
    }

    bool bBefore = true;

    m_bMoved = true;

    for (Script* cur : m_RunningScriptList)
    {
        if (bBefore)
        {
            cur->m_bProcessed = true;
        }
        else
        {
            cur->m_bProcessed = false;
        }

        if (cur->m_pLua == pLua)
        {
            bBefore = false;
        }
    }
}
//------------------------------------------------------------------------------
