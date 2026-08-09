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
#include "ResNickManager.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
#include <tinyxml2.h>
#include <sstream>

#include <algorithm>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::unique_ptr<ReservedNicksManager> ReservedNicksManager::m_Ptr;
//---------------------------------------------------------------------------

// ReservedNick destructor is = default in header
//---------------------------------------------------------------------------

std::unique_ptr<ReservedNicksManager::ReservedNick> ReservedNicksManager::ReservedNick::CreateReservedNick(const char* sNewNick, uint32_t ui32NickHash)
{
    auto pReservedNick = std::make_unique<ReservedNick>();

    pReservedNick->m_sNick = sNewNick;
    pReservedNick->m_ui32Hash = ui32NickHash;

    return pReservedNick;
}
//---------------------------------------------------------------------------

void ReservedNicksManager::Load()
{
    const std::string sPath = ServerManager::m_sPath + "/cfg/ReservedNicks.pxt";
    std::string sData;
    if (!ReadWholeFile(sPath, sData))
    {
        LogInfo("Error loading file ReservedNicks.pxt: file not found");
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

        AddReservedNick(sLine.c_str());
    }
}
//---------------------------------------------------------------------------

void ReservedNicksManager::Save() const
{
    std::vector<std::string> nicks;
    nicks.reserve(m_ReservedNicks.size());
    for (const auto& pNick : m_ReservedNicks)
    {
        nicks.push_back(pNick->m_sNick);
    }

    std::ranges::sort(nicks);

    std::string sOut = "#\n# PtokaX reserved nicks file\n#\n\n";
    for (const auto& nick : nicks)
    {
        sOut += nick;
        sOut += '\n';
    }

    const std::string sPath = ServerManager::m_sPath + "/cfg/ReservedNicks.pxt";
    if (!WriteWholeFile(sPath, sOut))
    {
        LogError("WriteWholeFile failed in ReservedNicksManager::Save");
    }
}
//---------------------------------------------------------------------------

void ReservedNicksManager::LoadXML()
{
    tinyxml2::XMLDocument doc;

    if (doc.LoadFile((ServerManager::m_sPath + "/cfg/ReservedNicks.xml").c_str()) == tinyxml2::XML_SUCCESS)
    {
        tinyxml2::XMLHandle cfg(&doc);
        tinyxml2::XMLNode* reservednicks = cfg.FirstChildElement("ReservedNicks").ToNode();
        if (reservednicks)
        {
            tinyxml2::XMLElement* child = reservednicks->FirstChildElement();
            while (child)
            {
                const char* sNick = child->GetText();

                if (sNick)
                {
                    AddReservedNick(sNick);
                }
                child = child->NextSiblingElement();
            }
        }
    }
}
//---------------------------------------------------------------------------

ReservedNicksManager::ReservedNicksManager()
{
    if (FileExist((ServerManager::m_sPath + "/cfg/ReservedNicks.pxt").c_str()))
    {
        Load();

        return;
    }
    else if (FileExist((ServerManager::m_sPath + "/cfg/ReservedNicks.xml").c_str()))
    {
        LoadXML();

        return;
    }
    else
    {
        const char* sNicks[] = {"Hub-Security", "Admin", "Client", "PtokaX", "OpChat"}; // NOLINT(modernize-avoid-c-arrays)
        for (const auto* nick : sNicks)
        {
            AddReservedNick(nick);
        }

        Save();
    }
}
//---------------------------------------------------------------------------

ReservedNicksManager::~ReservedNicksManager() = default;
//---------------------------------------------------------------------------

// Check for reserved nicks true = reserved
bool ReservedNicksManager::CheckReserved(const char* sNick, const uint32_t ui32Hash) const
{
    for (const auto& pNick : m_ReservedNicks)
    {
        if (pNick->m_ui32Hash == ui32Hash && iequals(pNick->m_sNick, sNick))
        {
            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

void ReservedNicksManager::AddReservedNick(const char* sNick, const bool bFromScript /* = false*/)
{
    const uint32_t ui32Hash = HashNick(sNick);

    if (!CheckReserved(sNick, ui32Hash))
    {
        auto pNewNick = ReservedNick::CreateReservedNick(sNick, ui32Hash);
        if (!pNewNick)
        {
            LogDbg("[MEM] Cannot allocate pNewNick in ReservedNicksManager::AddReservedNick");
            return;
        }

        pNewNick->m_bFromScript = bFromScript;

        m_ReservedNicks.push_back(std::move(pNewNick));
    }
}
//---------------------------------------------------------------------------

void ReservedNicksManager::DelReservedNick(const char* sNick, const bool bFromScript /* = false*/)
{
    const uint32_t ui32Hash = HashNick(sNick);

    for (auto it = m_ReservedNicks.begin(); it != m_ReservedNicks.end(); ++it)
    {
        const auto& pNick = *it;

        if (pNick->m_ui32Hash == ui32Hash && pNick->m_sNick == sNick)
        {
            if (bFromScript && !pNick->m_bFromScript)
            {
                continue;
            }

            m_ReservedNicks.erase(it);
            return;
        }
    }
}
//---------------------------------------------------------------------------
