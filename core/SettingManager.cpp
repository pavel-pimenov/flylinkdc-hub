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
#include "SettingCom.h"
#include "SettingDefaults.h"
#include "SettingManager.h"
#include "SettingStr.h"
#include <fmt/format.h>
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include <tinyxml2.h>
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextFileManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#endif
//---------------------------------------------------------------------------
static constexpr const char* g_sMin = "[min]";
static constexpr const char* g_sMax = "[max]";
static constexpr const char* g_sHubSec = "Hub-Security";

static constexpr size_t MAX_MOTD_LEN = 65024;
static constexpr size_t MAX_NICK_LEN = 64;
static constexpr size_t MAX_SR_LEN = 8192;
static constexpr size_t MAX_CTM_LEN = 512;
//---------------------------------------------------------------------------
std::unique_ptr<SettingManager> SettingManager::m_Ptr;
time_t SettingManager::m_tConfigLoadTime = 0;
//---------------------------------------------------------------------------

SettingManager::SettingManager()
{
    m_tConfigLoadTime = time(nullptr);

    // Read default bools
    for (size_t szi = 0; szi < std::to_underlying(SetBoolIds::SETBOOL_IDS_END); szi++)
    {
        SetBool(szi, SetBoolDef[szi]);
    }

    // Read default shorts
    for (size_t szi = 0; szi < std::to_underlying(SetShortIds::SETSHORT_IDS_END); szi++)
    {
        SetShort(szi, SetShortDef[szi]);
    }

    // Read default texts
    for (size_t szi = 0; szi < std::to_underlying(SetTxtIds::SETTXT_IDS_END); szi++)
    {
        SetText(szi, SetTxtDef[szi]);
    }

    // Load settings
    Load();
}
//---------------------------------------------------------------------------

SettingManager::~SettingManager()
{
    Save();
}
//---------------------------------------------------------------------------

void SettingManager::CreateDefaultMOTD()
{
    m_sMOTD = "Welcome to PtokaX";
}
//---------------------------------------------------------------------------

void SettingManager::LoadMOTD()
{
    m_sMOTD.clear();

    const std::string sPath = ServerManager::m_sPath + "/cfg/Motd.txt";
    std::string sData;
    if (ReadWholeFile(sPath, sData))
    {
        m_sMOTD = sData.size() > MAX_MOTD_LEN ? sData.substr(0, MAX_MOTD_LEN) : sData;
    }
    else
    {
        CreateDefaultMOTD();
    }

    CheckMOTD();
}
//---------------------------------------------------------------------------

void SettingManager::SaveMOTD()
{
    const std::string sPath = ServerManager::m_sPath + "/cfg/Motd.txt";
    if (!WriteWholeFile(sPath, m_sMOTD))
    {
        LogError("WriteWholeFile failed in SettingManager::SaveMOTD");
    }
}
//---------------------------------------------------------------------------

void SettingManager::CheckMOTD()
{
    for (char& i : m_sMOTD)
    {
        if (i == '|')
        {
            i = '0';
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::CheckAndSet(const char* sName, const char* sValue)
{
    // Booleans
    for (size_t szi = 0; szi < std::to_underlying(SetBoolIds::SETBOOL_IDS_END); szi++)
    {
        if (strcmp(SetBoolStr[szi], sName) == 0)
        {
            SetBool(szi, sValue[0] == '1');
            return;
        }
    }

    // Integers
    for (size_t szi = 0; szi < std::to_underlying(SetShortIds::SETSHORT_IDS_END); szi++)
    {
        if (strcmp(SetShortStr[szi], sName) == 0)
        {
            int32_t iValue = 0;
            if (!safe_stoi(sValue, iValue))
            {
                return;
            }

            // Check if is valid value
            if (sValue[0] == '\0' || iValue < 0 || iValue > 32767)
            {
                return;
            }

            SetShort(szi, static_cast<int16_t>(iValue));
            return;
        }
    }

    // Strings
    for (size_t szi = 0; szi < std::to_underlying(SetTxtIds::SETTXT_IDS_END); szi++) // NOLINT(modernize-loop-convert) index used in body
    {
        if (strcmp(SetTxtStr[szi], sName) == 0)
        {
            SetText(szi, sValue);
            return;
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::Load()
{
    m_bUpdateLocked = true;

    // Load MOTD
    LoadMOTD();

    if (!FileExist((ServerManager::m_sPath + "/cfg/Settings.pxt").c_str()))
    {
        LoadXML();

        m_bUpdateLocked = false;

        return;
    }

    FilePtr fSettingsFile(fopen((ServerManager::m_sPath + "/cfg/Settings.pxt").c_str(), "rt"));
    if (!fSettingsFile)
    {
        int iMsgLen =
            snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file Settings.pxt %s (%d)", ErrnoStr(errno), errno);
        if (iMsgLen > 0)
        {
            LogInfo("{}", ServerManager::m_pGlobalBuffer);
        }

        exit(EXIT_FAILURE);
    }

    char* sValue = nullptr;
    size_t szLen = 0;

    while (fgets(ServerManager::m_pGlobalBuffer, static_cast<int>(ServerManager::m_szGlobalBufferSize), fSettingsFile.get()))
    {
        if (ServerManager::m_pGlobalBuffer[0] == '#' || ServerManager::m_pGlobalBuffer[0] == '\n')
        {
            continue;
        }

        sValue = nullptr;

        szLen = strlen(ServerManager::m_pGlobalBuffer) - 1;

        ServerManager::m_pGlobalBuffer[szLen] = '\0';

        for (size_t szi = 0; szi < szLen; szi++)
        {
            if (isspace(static_cast<unsigned char>(ServerManager::m_pGlobalBuffer[szi])))
            {
                ServerManager::m_pGlobalBuffer[szi] = '\0';
                continue;
            }

            if (ServerManager::m_pGlobalBuffer[szi] == '=')
            {
                if (isspace(static_cast<unsigned char>(ServerManager::m_pGlobalBuffer[szi + 1])))
                {
                    sValue = ServerManager::m_pGlobalBuffer + szi + 2;
                }
                else
                {
                    sValue = ServerManager::m_pGlobalBuffer + szi + 1;
                }

                break;
            }
        }

        if (!sValue || ServerManager::m_pGlobalBuffer[0] == '\0')
        {
            continue;
        }

        CheckAndSet(ServerManager::m_pGlobalBuffer, sValue);
    }

    m_bUpdateLocked = false;
}
//---------------------------------------------------------------------------

void SettingManager::LoadXML()
{
    tinyxml2::XMLDocument doc;
    if (LoadXmlConfig(doc, "Settings.xml"))
    {
        tinyxml2::XMLHandle cfg(&doc);

        tinyxml2::XMLElement* settings = cfg.FirstChildElement("PtokaX").ToElement();
        if (!settings)
        {
            return;
        }

        // version first run
        const char* sVersion = settings->Attribute("Version");
        if (!sVersion || strcmp(sVersion, PtokaXVersionString) != 0)
        {
            m_bFirstRun = true;
        }
        else
        {
            m_bFirstRun = false;
        }

        // Read bools
        tinyxml2::XMLElement* SettingNode = settings->FirstChildElement("Booleans");
        if (SettingNode)
        {
            tinyxml2::XMLElement* SettingValue = SettingNode->FirstChildElement();
            while (SettingValue)
            {
                const char* sName = SettingValue->Attribute("Name");

                if (!sName)
                {
                    SettingValue = SettingValue->NextSiblingElement();
                    continue;
                }

                const char* sText = SettingValue->GetText();
                int iBoolVal = 0;
                if (!sText || !safe_stoi(sText, iBoolVal))
                {
                    SettingValue = SettingValue->NextSiblingElement();
                    continue;
                }
                const bool bValue = iBoolVal != 0;

                for (size_t szi = 0; szi < std::to_underlying(SetBoolIds::SETBOOL_IDS_END); szi++)
                {
                    if (strcmp(SetBoolStr[szi], sName) == 0)
                    {
                        SetBool(szi, bValue);
                    }
                }
                SettingValue = SettingValue->NextSiblingElement();
            }
        }

        // Read integers
        SettingNode = settings->FirstChildElement("Integers");
        if (SettingNode)
        {
            tinyxml2::XMLElement* SettingValue = SettingNode->FirstChildElement();
            while (SettingValue)
            {
                const char* sName = SettingValue->Attribute("Name");

                if (!sName)
                {
                    SettingValue = SettingValue->NextSiblingElement();
                    continue;
                }

                const char* sText = SettingValue->GetText();
                int32_t iValue = 0;
                if (!sText || !safe_stoi(sText, iValue))
                {
                    SettingValue = SettingValue->NextSiblingElement();
                    continue;
                }
                // Check if is valid value
                if (iValue < 0 || iValue > 32767)
                {
                    SettingValue = SettingValue->NextSiblingElement();
                    continue;
                }

                for (size_t szi = 0; szi < std::to_underlying(SetShortIds::SETSHORT_IDS_END); szi++)
                {
                    if (strcmp(SetShortStr[szi], sName) == 0)
                    {
                        SetShort(szi, static_cast<int16_t>(iValue));
                    }
                }
                SettingValue = SettingValue->NextSiblingElement();
            }
        }

        // Read strings
        SettingNode = settings->FirstChildElement("Strings");
        if (SettingNode)
        {
            tinyxml2::XMLElement* SettingValue = SettingNode->FirstChildElement();
            while (SettingValue)
            {
                const char* sName = SettingValue->Attribute("Name");

                if (!sName)
                {
                    SettingValue = SettingValue->NextSiblingElement();
                    continue;
                }

                const char* sText = SettingValue->GetText();

                if (!sText)
                {
                    SettingValue = SettingValue->NextSiblingElement();
                    continue;
                }

                for (size_t szi = 0; szi < std::to_underlying(SetTxtIds::SETTXT_IDS_END); szi++) // NOLINT(modernize-loop-convert) index used in body
                {
                    if (strcmp(SetTxtStr[szi], sName) == 0)
                    {
                        SetText(szi, sText);
                    }
                }
                SettingValue = SettingValue->NextSiblingElement();
            }
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::Save()
{
    SaveMOTD();

    std::string sOut;
    sOut += "#\n# PtokaX settings file\n#\n";

    // Save booleans
    sOut += "\n#\n# Boolean settings\n#\n\n";
    for (size_t szi = 0; szi < std::to_underlying(SetBoolIds::SETBOOL_IDS_END); szi++)
    {
        // Don't save empty hint
        if (SetBoolCom[szi][0] != '\0')
        {
            sOut += SetBoolCom[szi];
        }

        // Don't save setting with empty id
        if (SetBoolStr[szi][0] == '\0')
        {
            continue;
        }

        // Save setting with default value as comment
        if (m_bBools[szi] == SetBoolDef[szi])
        {
            sOut += fmt::format("#{}\t=\t{}\n", SetBoolStr[szi], m_bBools[szi] ? '1' : '0');
        }
        else
        {
            sOut += fmt::format("{}\t=\t{}\n", SetBoolStr[szi], m_bBools[szi] ? '1' : '0');
        }
    }

    // Save integers
    sOut += "\n#\n# Integer settings\n#\n\n";
    for (size_t szi = 0; szi < std::to_underlying(SetShortIds::SETSHORT_IDS_END); szi++)
    {
        // Don't save empty hint
        if (SetShortCom[szi][0] != '\0')
        {
            sOut += SetShortCom[szi];
        }

        // Don't save setting with empty id
        if (SetShortStr[szi][0] == '\0')
        {
            continue;
        }

        // Save setting with default value as comment
        if (m_i16Shorts[szi] == SetShortDef[szi])
        {
            sOut += fmt::format("#{}\t=\t{}\n", SetShortStr[szi], m_i16Shorts[szi]);
        }
        else
        {
            sOut += fmt::format("{}\t=\t{}\n", SetShortStr[szi], m_i16Shorts[szi]);
        }
    }

    // Save strings
    sOut += "\n#\n# String settings\n#\n\n";
    for (size_t szi = 0; szi < std::to_underlying(SetTxtIds::SETTXT_IDS_END); szi++)
    {
        // Don't save empty hint
        if (SetTxtCom[szi][0] != '\0')
        {
            sOut += SetTxtCom[szi];
        }

        // Don't save setting with empty id
        if (SetTxtStr[szi][0] == '\0')
        {
            continue;
        }

        // Save setting with default value as comment
        if ((m_sTexts[szi].empty() && SetTxtDef[szi][0] == '\0') || (!m_sTexts[szi].empty() && m_sTexts[szi] == SetTxtDef[szi]))
        {
            sOut += fmt::format("#{}\t=\t{}\n", SetTxtStr[szi], !m_sTexts[szi].empty() ? m_sTexts[szi].c_str() : "");
        }
        else
        {
            sOut += fmt::format("{}\t=\t{}\n", SetTxtStr[szi], !m_sTexts[szi].empty() ? m_sTexts[szi].c_str() : "");
        }
    }

    const std::string sPath = ServerManager::m_sPath + "/cfg/Settings.pxt";
    if (!WriteWholeFile(sPath, sOut))
    {
        LogError("WriteWholeFile failed in SettingManager::Save");
    }
}
//---------------------------------------------------------------------------

bool SettingManager::GetBool(size_t szBoolId) const
{
    Lock l(m_csSetting);
    const bool bValue = m_bBools[szBoolId];
    return bValue;
}
//---------------------------------------------------------------------------
uint16_t SettingManager::GetFirstPort() const
{
    Lock l(m_csSetting);
    const uint16_t iValue = m_ui16PortNumbers[0];
    return iValue;
}
//---------------------------------------------------------------------------

int16_t SettingManager::GetShort(size_t szShortId) const
{
    Lock l(m_csSetting);
    const int16_t iValue = m_i16Shorts[szShortId];
    return iValue;
}
//---------------------------------------------------------------------------

std::string SettingManager::GetText(size_t szTxtId) const
{
    Lock l(m_csSetting);
    return m_sTexts[szTxtId];
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void SettingManager::SetBool(size_t szBoolId, bool bValue)
{
    if (m_bBools[szBoolId] == bValue)
    {
        return;
    }

    if (szBoolId == std::to_underlying(SetBoolIds::SETBOOL_ANTI_MOGLO))
    {
        Lock l(m_csSetting);
        m_bBools[szBoolId] = bValue;
        return;
    }

    m_bBools[szBoolId] = bValue;

    switch (szBoolId)
    {
    case std::to_underlying(SetBoolIds::SETBOOL_REG_BOT):
        UpdateBotsSameNick();
        if (!bValue)
        {
            DisableBot();
        }
        UpdateBot();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT):
        UpdateBotsSameNick();
        if (!bValue)
        {
            DisableOpChat();
        }
        UpdateOpChat();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_USE_BOT_NICK_AS_HUB_SEC):
        UpdateHubSec();
        UpdateMOTD();
        UpdateHubNameWelcome();
        UpdateRegOnlyMessage();
        UpdateShareLimitMessage();
        UpdateSlotsLimitMessage();
        UpdateHubSlotRatioMessage();
        UpdateMaxHubsLimitMessage();
        UpdateNoTagMessage();
        UpdateNickLimitMessage();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_DISABLE_MOTD):
    case std::to_underlying(SetBoolIds::SETBOOL_MOTD_AS_PM):
        UpdateMOTD();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_REG_ONLY_REDIR):
        UpdateRegOnlyMessage();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_SHARE_LIMIT_REDIR):
        UpdateShareLimitMessage();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_SLOTS_LIMIT_REDIR):
        UpdateSlotsLimitMessage();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_HUB_SLOT_RATIO_REDIR):
        UpdateHubSlotRatioMessage();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_MAX_HUBS_LIMIT_REDIR):
        UpdateMaxHubsLimitMessage();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_NICK_LIMIT_REDIR):
        UpdateNickLimitMessage();
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_ENABLE_TEXT_FILES):
        if (bValue && !m_bUpdateLocked)
        {
            TextFilesManager::m_Ptr->RefreshTextFiles();
        }
        break;
    case std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING):
        UpdateScripting();
        break;
#ifdef _WITH_SQLITE
    case std::to_underlying(SetBoolIds::SETBOOL_ENABLE_DATABASE):
        if (!m_bUpdateLocked)
        {
            UpdateDatabase();
        }

        break;
#endif
    case std::to_underlying(SetBoolIds::SETBOOL_RESOLVE_TO_IP):
        if (!m_bUpdateLocked)
        {
            (void)ServerManager::ResolveHubAddress(true);
        }
        break;
    default:
        break;
    }
}
//---------------------------------------------------------------------------

void SettingManager::SetMOTD(std::string_view sTxt)
{
    if (m_sMOTD.size() == sTxt.size() && m_sMOTD == sTxt)
    {
        return;
    }

    if (sTxt.empty())
    {
        m_sMOTD.clear();
    }
    else
    {
        const size_t szActualLen = sTxt.size() < MAX_MOTD_LEN ? sTxt.size() : MAX_MOTD_LEN;
        m_sMOTD.assign(sTxt.data(), szActualLen);
        CheckMOTD();
    }
}
//---------------------------------------------------------------------------

void SettingManager::SetShort(size_t szShortId, int16_t i16Value)
{
    if (i16Value < 0 || m_i16Shorts[szShortId] == i16Value)
    {
        return;
    }

    switch (szShortId)
    {
    case std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT):
        if (i16Value > 9999)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_UNITS):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_UNITS):
        if (i16Value > 4)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_NO_TAG_OPTION):
    case std::to_underlying(SetShortIds::SETSHORT_FULL_MYINFO_OPTION):
    case std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE):
        if (i16Value > 2)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_USERS):
    case std::to_underlying(SetShortIds::SETSHORT_DEFAULT_TEMP_BAN_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_TEMP_BAN_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_SR_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_SR_MESSAGES2):
        if (i16Value == 0)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MIN_SLOTS_LIMIT):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SLOTS_LIMIT):
    case std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_HUBS):
    case std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_SLOTS):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_HUBS_LIMIT):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_CHAT_LINES):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_PM_LINES):
    case std::to_underlying(SetShortIds::SETSHORT_MYINFO_DELAY):
    case std::to_underlying(SetShortIds::SETSHORT_MIN_SEARCH_LEN):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SEARCH_LEN):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_PM_COUNT_TO_USER):
        if (i16Value > 999)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_PM_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_PM_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_PM_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_COUNT):
    case std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT):
    case std::to_underlying(SetShortIds::SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_MESSAGES2):
    case std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_TIME2):
    case std::to_underlying(SetShortIds::SETSHORT_PM_MESSAGES2):
    case std::to_underlying(SetShortIds::SETSHORT_PM_TIME2):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_MESSAGES2):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_TIME2):
    case std::to_underlying(SetShortIds::SETSHORT_MYINFO_MESSAGES2):
    case std::to_underlying(SetShortIds::SETSHORT_MYINFO_TIME2):
    case std::to_underlying(SetShortIds::SETSHORT_CHAT_INTERVAL_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_CHAT_INTERVAL_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_PM_INTERVAL_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_PM_INTERVAL_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_INTERVAL_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_INTERVAL_TIME):
        if (i16Value == 0 || i16Value > 999)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_CTM_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_CTM_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_CTM_MESSAGES2):
    case std::to_underlying(SetShortIds::SETSHORT_CTM_TIME2):
    case std::to_underlying(SetShortIds::SETSHORT_RCTM_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_RCTM_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_RCTM_MESSAGES2):
    case std::to_underlying(SetShortIds::SETSHORT_RCTM_TIME2):
    case std::to_underlying(SetShortIds::SETSHORT_SR_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_SR_TIME2):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_KB):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_TIME):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_KB2):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_TIME2):
        if (i16Value == 0 || i16Value > 9999)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_NEW_CONNECTIONS_COUNT):
    case std::to_underlying(SetShortIds::SETSHORT_NEW_CONNECTIONS_TIME):
        if (i16Value == 0 || i16Value > 999)
        {
            return;
        }
        {
            Lock l(m_csSetting);
            m_i16Shorts[szShortId] = i16Value;
            return;
        }
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_LINES):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_PM_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_MESSAGES):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_LINES):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_MESSAGES):
        if (i16Value < 2 || i16Value > 999)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_MAIN_CHAT_ACTION2):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MAIN_CHAT_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_PM_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_PM_ACTION2):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_PM_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_MULTI_PM_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_SEARCH_ACTION2):
    case std::to_underlying(SetShortIds::SETSHORT_SAME_SEARCH_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_MYINFO_ACTION2):
    case std::to_underlying(SetShortIds::SETSHORT_GETNICKLIST_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_CTM_ACTION2):
    case std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_RCTM_ACTION2):
    case std::to_underlying(SetShortIds::SETSHORT_SR_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_SR_ACTION2):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION2):
        if (i16Value > 6)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_DEFLOOD_WARNING_ACTION):
        if (i16Value > 3)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MIN_NICK_LEN):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_NICK_LEN):
        if (i16Value > 64)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SIMULTANEOUS_LOGINS):
        if (i16Value == 0 || i16Value > 1000)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_MYINFO_LEN):
        if (i16Value < 64 || i16Value > 512)
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_CTM_LEN):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_RCTM_LEN):
        if (i16Value == 0 || i16Value > static_cast<int16_t>(MAX_CTM_LEN))
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SR_LEN):
        if (i16Value == 0 || i16Value > static_cast<int16_t>(MAX_SR_LEN))
        {
            return;
        }
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_CONN_SAME_IP):
    case std::to_underlying(SetShortIds::SETSHORT_MIN_RECONN_TIME):
        if (i16Value == 0 || i16Value > 256)
        {
            return;
        }
        break;
    default:
        break;
    }

    m_i16Shorts[szShortId] = i16Value;

    switch (szShortId)
    {
    case std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT):
    case std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_UNITS):
        UpdateMinShare();
        UpdateShareLimitMessage();
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_UNITS):
        UpdateMaxShare();
        UpdateShareLimitMessage();
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MIN_SLOTS_LIMIT):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_SLOTS_LIMIT):
        UpdateSlotsLimitMessage();
        break;
    case std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_HUBS):
    case std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_SLOTS):
        UpdateHubSlotRatioMessage();
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MAX_HUBS_LIMIT):
        UpdateMaxHubsLimitMessage();
        break;
    case std::to_underlying(SetShortIds::SETSHORT_NO_TAG_OPTION):
        UpdateNoTagMessage();
        break;
    case std::to_underlying(SetShortIds::SETSHORT_MIN_NICK_LEN):
    case std::to_underlying(SetShortIds::SETSHORT_MAX_NICK_LEN):
        UpdateNickLimitMessage();
        break;
    default:
        break;
    }
}
//---------------------------------------------------------------------------

void SettingManager::SetText(const size_t szTxtId, std::string_view sTxt)
{
    if (m_sTexts[szTxtId] == sTxt)
    {
        return;
    }

    switch (szTxtId)
    {
    case std::to_underlying(SetTxtIds::SETTXT_HUB_NAME):
    case std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS):
        if (sTxt.empty() || sTxt.size() > 256 || contains_any(sTxt, "$|"))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_NO_TAG_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG):
        if (sTxt.empty() || sTxt.size() > 256 || contains_any(sTxt, "|"))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_BOT_NICK):
        if (sTxt.empty() || sTxt.size() > 64 || contains_any(sTxt, " $|"))
        {
            return;
        }
        if (!ServerManager::m_Servers.empty() && !m_bBotsSameNick)
        {
            ReservedNicksManager::m_Ptr->DelReservedNick(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str());
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)])
        {
            DisableBot();
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK):
        if (sTxt.empty() || sTxt.size() > 64 || contains_any(sTxt, " $|"))
        {
            return;
        }
        if (!ServerManager::m_Servers.empty() && !m_bBotsSameNick)
        {
            ReservedNicksManager::m_Ptr->DelReservedNick(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)])
        {
            DisableOpChat();
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_ADMIN_NICK):
        if (sTxt.empty() || sTxt.size() > 64 || contains_any(sTxt, " $|"))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS):
        if (sTxt.empty() || sTxt.size() > 64)
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_UDP_PORT):
        if (sTxt.empty() || sTxt.size() > 5)
        {
            return;
        }
        UpdateUDPPort();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_CHAT_COMMANDS_PREFIXES):
        if (sTxt.empty() || sTxt.size() > 5 || contains_any(sTxt, "| "))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_HUB_DESCRIPTION):
    case std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC):
        if (sTxt.size() > 256 || (!sTxt.empty() && contains_any(sTxt, "$|")))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_REDIRECT_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_NO_TAG_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_TEMP_BAN_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_PERM_BAN_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_REDIR_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_MSG_TO_ADD_TO_BAN_MSG):
        if (sTxt.size() > 256 || (!sTxt.empty() && contains_any(sTxt, "|")))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_REGISTER_SERVERS):
        if (sTxt.size() > 1024)
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION):
    case std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL):
        if (sTxt.size() > 64 || (!sTxt.empty() && contains_any(sTxt, "$|")))
        {
            return;
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)])
        {
            DisableBot(false);
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION):
    case std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL):
        if (sTxt.size() > 64 || (!sTxt.empty() && contains_any(sTxt, "$|")))
        {
            return;
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)])
        {
            DisableOpChat(false);
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_HUB_OWNER_EMAIL):
        if (sTxt.size() > 64 || (!sTxt.empty() && contains_any(sTxt, "$|")))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_LANGUAGE):
        if (!sTxt.empty() && !FileExist((ServerManager::m_sPath + "/language/" + std::string(sTxt) + ".xml").c_str()))
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_IPV4_ADDRESS):
        if (sTxt.size() > 15)
        {
            return;
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_IPV6_ADDRESS):
        if (sTxt.size() > 39)
        {
            return;
        }
        break;
    default:
        if (sTxt.size() > 4096)
        {
            return;
        }
        break;
    }

    {
        Lock l(m_csSetting);

        if (sTxt.empty())
        {
            m_sTexts[szTxtId].clear();
        }
        else
        {
            m_sTexts[szTxtId].assign(sTxt);
        }
    }
    switch (szTxtId)
    {
    case std::to_underlying(SetTxtIds::SETTXT_BOT_NICK):
        UpdateHubSec();
        UpdateMOTD();
        UpdateHubNameWelcome();
        UpdateRegOnlyMessage();
        UpdateShareLimitMessage();
        UpdateSlotsLimitMessage();
        UpdateHubSlotRatioMessage();
        UpdateMaxHubsLimitMessage();
        UpdateNoTagMessage();
        UpdateNickLimitMessage();
        UpdateBotsSameNick();

        if (!ServerManager::m_Servers.empty() && !m_bBotsSameNick)
        {
            ReservedNicksManager::m_Ptr->AddReservedNick(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str());
        }

        UpdateBot();

        break;
    case std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION):
    case std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL):
        UpdateBot(false);
        break;
    case std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK):
        UpdateBotsSameNick();
        if (!ServerManager::m_Servers.empty() && !m_bBotsSameNick)
        {
            ReservedNicksManager::m_Ptr->AddReservedNick(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
        }
        UpdateOpChat();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC):
    case std::to_underlying(SetTxtIds::SETTXT_HUB_NAME):
        UpdateHubNameWelcome();
        UpdateHubName();

        if (UdpDebug::m_Ptr)
        {
            UdpDebug::m_Ptr->UpdateHubName();
        }
        break;
    case std::to_underlying(SetTxtIds::SETTXT_LANGUAGE):
        UpdateLanguage();
        UpdateHubNameWelcome();
        break;
#ifdef FLYLINKDC_USE_REDIR
    case std::to_underlying(SetTxtIds::SETTXT_REDIRECT_ADDRESS):
        UpdateRedirectAddress();
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_ONLY_REDIR)])
        {
            UpdateRegOnlyMessage();
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SHARE_LIMIT_REDIR)])
        {
            UpdateShareLimitMessage();
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SLOTS_LIMIT_REDIR)])
        {
            UpdateSlotsLimitMessage();
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_HUB_SLOT_RATIO_REDIR)])
        {
            UpdateHubSlotRatioMessage();
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_MAX_HUBS_LIMIT_REDIR)])
        {
            UpdateMaxHubsLimitMessage();
        }
        if (m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_NO_TAG_OPTION)] == 2)
        {
            UpdateNoTagMessage();
        }
        if (!m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TEMP_BAN_REDIR_ADDRESS)].empty())
        {
            UpdateTempBanRedirAddress();
        }
        if (!m_sTexts[std::to_underlying(SetTxtIds::SETTXT_PERM_BAN_REDIR_ADDRESS)].empty())
        {
            UpdatePermBanRedirAddress();
        }
        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_NICK_LIMIT_REDIR)])
        {
            UpdateNickLimitMessage();
        }
        break;
#endif // FLYLINKDC_USE_REDIR

    case std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_REDIR_ADDRESS):
        UpdateRegOnlyMessage();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_REDIR_ADDRESS):
        UpdateShareLimitMessage();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_REDIR_ADDRESS):
        UpdateSlotsLimitMessage();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS):
        UpdateHubSlotRatioMessage();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS):
        UpdateMaxHubsLimitMessage();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_NO_TAG_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_NO_TAG_REDIR_ADDRESS):
        UpdateNoTagMessage();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_TEMP_BAN_REDIR_ADDRESS):
        UpdateTempBanRedirAddress();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_PERM_BAN_REDIR_ADDRESS):
        UpdatePermBanRedirAddress();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG):
    case std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_REDIR_ADDRESS):
        UpdateNickLimitMessage();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS):
        UpdateTCPPorts();
        break;
    case std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_IPV4_ADDRESS):
    case std::to_underlying(SetTxtIds::SETTXT_IPV6_ADDRESS):
        if (!m_bUpdateLocked)
        {
            (void)ServerManager::ResolveHubAddress(true);
        }
        break;
    default:
        break;
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateAll()
{
    m_ui8FullMyINFOOption = static_cast<uint8_t>(m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_FULL_MYINFO_OPTION)]);

    UpdateHubSec();
    UpdateMOTD();
    UpdateHubNameWelcome();
    UpdateHubName();
    UpdateRedirectAddress();
    UpdateRegOnlyMessage();
    UpdateMinShare();
    UpdateMaxShare();
    UpdateShareLimitMessage();
    UpdateSlotsLimitMessage();
    UpdateHubSlotRatioMessage();
    UpdateMaxHubsLimitMessage();
    UpdateNoTagMessage();
    UpdateTempBanRedirAddress();
    UpdatePermBanRedirAddress();
    UpdateNickLimitMessage();
    UpdateTCPPorts();
    UpdateBotsSameNick();
}
//---------------------------------------------------------------------------
// Helper: append redirect address to a buffer (used by multiple Update*Message methods)
//---------------------------------------------------------------------------
int SettingManager::AppendRedirectAddress(char* sDest, size_t szDestSize, int iMsgLen, size_t szBoolId, size_t szTxtRedirId) const
{
    if (m_bBools[szBoolId])
    {
        if (!m_sTexts[szTxtRedirId].empty())
        {
            if (!SnprintfAppend(sDest, iMsgLen, szDestSize, "$ForceMove %s|", m_sTexts[szTxtRedirId].c_str()))
            {
                LogDbg("[ERR] SnprintfAppend failed in SettingManager::AppendRedirectAddress");
                exit(EXIT_FAILURE);
            }
        }
        else if (!m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].empty())
        {
            if (!SnprintfAppend(sDest, iMsgLen, szDestSize, "%s", m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].c_str()))
            {
                LogDbg("[ERR] SnprintfAppend failed in SettingManager::AppendRedirectAddress");
                exit(EXIT_FAILURE);
            }
        }
    }
    return iMsgLen;
}
//---------------------------------------------------------------------------
// Helper: realloc m_sPreTexts[szPreTxtId] and copy iMsgLen bytes from m_pGlobalBuffer
//---------------------------------------------------------------------------
void SettingManager::CommitGlobalBufferToPreText(size_t szPreTxtId, int iMsgLen, [[maybe_unused]] const char* sFuncName)
{
    m_sPreTexts[szPreTxtId].assign(ServerManager::m_pGlobalBuffer, static_cast<size_t>(iMsgLen));
}
//---------------------------------------------------------------------------

int SettingManager::BeginLimitMessage() const
{
    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str());
    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::BeginLimitMessage");
        exit(EXIT_FAILURE);
    }
    return iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::EndLimitMessage(int iMsgLen, size_t preTxtId, size_t boolRedirId, size_t txtRedirId, const char* funcName)
{
    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    iMsgLen++;
    iMsgLen = AppendRedirectAddress(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, iMsgLen, boolRedirId, txtRedirId);
    CommitGlobalBufferToPreText(preTxtId, iMsgLen, funcName);
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubSec()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_USE_BOT_NICK_AS_HUB_SEC)])
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)] = m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)];
    }
    else
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)] = g_sHubSec;
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMOTD()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_DISABLE_MOTD)] || m_sMOTD.empty())
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_MOTD)].clear();
        return;
    }

    int iMsgLen = 0;

    if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_MOTD_AS_PM)])
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "$To: %%s From: %s $<%s> %s|",
                           m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str(),
                           m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str(),
                           m_sMOTD.c_str());
    }
    else
    {
        iMsgLen =
            snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> %s|", m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str(), m_sMOTD.c_str());
    }

    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateMOTD");
        exit(EXIT_FAILURE);
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_MOTD), iMsgLen, "UpdateMOTD");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubNameWelcome()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = 0;

    if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC)].empty())
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "$HubName %s|<%s> %s %s (%s: ",
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].c_str(),
                           m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THIS_HUB_IS_RUNNING)].c_str(),
                           g_sPtokaXTitle,
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UPTIME)].c_str());
    }
    else
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "$HubName %s - %s|<%s> %s %s (%s: ",
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].c_str(),
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC)].c_str(),
                           m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_THIS_HUB_IS_RUNNING)].c_str(),
                           g_sPtokaXTitle,
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UPTIME)].c_str());
    }

    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateHubNameWelcome");
        exit(EXIT_FAILURE);
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_NAME_WLCM), iMsgLen, "UpdateHubNameWelcome");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubName()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = 0;

    if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC)].empty())
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$HubName %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].c_str());
    }
    else
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "$HubName %s - %s|",
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_NAME)].c_str(),
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_TOPIC)].c_str());
    }

    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateHubName");
        exit(EXIT_FAILURE);
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_NAME), iMsgLen, "UpdateHubName");

    if (ServerManager::m_bServerRunning)
    {
        GlobalDataQueue::m_Ptr->AddQueueItem(m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_NAME)], "", GlobalDataQueue::Cmd::HUBNAME);
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateRedirectAddress()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_REDIRECT_ADDRESS)].empty())
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].clear();

        return;
    }

    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$ForceMove %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_REDIRECT_ADDRESS)].c_str());
    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateRedirectAddress");
        exit(EXIT_FAILURE);
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS), iMsgLen, "UpdateRedirectAddress");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateRegOnlyMessage()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "<%s> %s|",
                           m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str(),
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_MSG)].c_str());
    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateRegOnlyMessage");
        exit(EXIT_FAILURE);
    }

    iMsgLen = AppendRedirectAddress(
        ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, iMsgLen, std::to_underlying(SetBoolIds::SETBOOL_REG_ONLY_REDIR), std::to_underlying(SetTxtIds::SETTXT_REG_ONLY_REDIR_ADDRESS));

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REG_ONLY_MSG), iMsgLen, "UpdateRegOnlyMessage");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateShareLimitMessage()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = BeginLimitMessage();

    static const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", " ", " ", " ", " ", " ", " ", " ", " ", " "}; // NOLINT(modernize-avoid-c-arrays)

    for (size_t ui16i = 0; ui16i < m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG)].size(); ui16i++)
    {
        if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG)][ui16i] == '%')
        {
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG)].compare(ui16i + 1, 5, g_sMin) == 0)
            {
                if (m_ui64MinShare != 0)
                {
                    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                        iMsgLen,
                                        ServerManager::m_szGlobalBufferSize,
                                        "%hd %s",
                                        m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT)],
                                        units[m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_UNITS)]]))
                    {
                        LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateShareLimitMessage");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    memcpy(ServerManager::m_pGlobalBuffer + iMsgLen, "0 B", 3); // NOLINT(bugprone-not-null-terminated-result) length-tracked buffer
                    iMsgLen += 3;
                }
                ui16i += static_cast<uint16_t>(5);
                continue;
            }
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG)].compare(ui16i + 1, 5, g_sMax) == 0)
            {
                if (m_ui64MaxShare != 0)
                {
                    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                        iMsgLen,
                                        ServerManager::m_szGlobalBufferSize,
                                        "%hd %s",
                                        m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT)],
                                        units[m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_UNITS)]]))
                    {
                        LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateShareLimitMessage");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    memcpy(ServerManager::m_pGlobalBuffer + iMsgLen,
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].size());
                    iMsgLen += static_cast<int>(LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].size());
                }
                ui16i += static_cast<uint16_t>(5);
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_MSG)][ui16i];
        iMsgLen++;
    }

    EndLimitMessage(iMsgLen, std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_SHARE_LIMIT_MSG), std::to_underlying(SetBoolIds::SETBOOL_SHARE_LIMIT_REDIR), std::to_underlying(SetTxtIds::SETTXT_SHARE_LIMIT_REDIR_ADDRESS), "UpdateShareLimitMessage");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateSlotsLimitMessage()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = BeginLimitMessage();

    for (size_t ui16i = 0; ui16i < m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG)].size(); ui16i++)
    {
        if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG)][ui16i] == '%')
        {
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG)].compare(ui16i + 1, 5, g_sMin) == 0)
            {
                if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "%hd", m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SLOTS_LIMIT)]))
                {
                    LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateSlotsLimitMessage");
                    exit(EXIT_FAILURE);
                }

                ui16i += static_cast<uint16_t>(5);
                continue;
            }
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG)].compare(ui16i + 1, 5, g_sMax) == 0)
            {
                if (m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SLOTS_LIMIT)] != 0)
                {
                    if (!SnprintfAppend(
                            ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "%hd", m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SLOTS_LIMIT)]))
                    {
                        LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateSlotsLimitMessage");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    memcpy(ServerManager::m_pGlobalBuffer + iMsgLen,
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].size());
                    iMsgLen += static_cast<int>(LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].size());
                }
                ui16i += static_cast<uint16_t>(5);
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_MSG)][ui16i];
        iMsgLen++;
    }

    EndLimitMessage(iMsgLen, std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_SLOTS_LIMIT_MSG), std::to_underlying(SetBoolIds::SETBOOL_SLOTS_LIMIT_REDIR), std::to_underlying(SetTxtIds::SETTXT_SLOTS_LIMIT_REDIR_ADDRESS), "UpdateSlotsLimitMessage");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubSlotRatioMessage()
{
#ifdef FLYLINKDC_USE_HUB_SLOT_RATIO
    return;
#endif

    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = BeginLimitMessage();

    static constexpr const char* sHubs = "[hubs]";
    static constexpr const char* sSlots = "[slots]";

    for (size_t ui16i = 0; ui16i < m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG)].size(); ui16i++)
    {
        if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG)][ui16i] == '%')
        {
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG)].compare(ui16i + 1, 6, sHubs) == 0)
            {
                if (!SnprintfAppend(
                        ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "%hd", m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_HUBS)]))
                {
                    LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateHubSlotRatioMessage");
                    exit(EXIT_FAILURE);
                }

                ui16i += static_cast<uint16_t>(6);
                continue;
            }
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG)].compare(ui16i + 1, 7, sSlots) == 0)
            {
                if (!SnprintfAppend(
                        ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "%hd", m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_SLOTS)]))
                {
                    LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateHubSlotRatioMessage");
                    exit(EXIT_FAILURE);
                }

                ui16i += static_cast<uint16_t>(7);
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_MSG)][ui16i];
        iMsgLen++;
    }

    EndLimitMessage(iMsgLen, std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SLOT_RATIO_MSG), std::to_underlying(SetBoolIds::SETBOOL_HUB_SLOT_RATIO_REDIR), std::to_underlying(SetTxtIds::SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS), "UpdateHubSlotRatioMessage");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMaxHubsLimitMessage()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = BeginLimitMessage();

    static constexpr const char* sHubs = "%[hubs]";

    const char* sMatch = strstr(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].c_str(), sHubs);

    if (sMatch)
    {
        if (sMatch > m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].data())
        {
            const size_t szLen = sMatch - m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].data();
            memcpy(ServerManager::m_pGlobalBuffer + iMsgLen, m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].data(), szLen);
            iMsgLen += static_cast<int>(szLen);
        }

        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "%hd", m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_HUBS_LIMIT)]))
        {
            LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateMaxHubsLimitMessage");
            exit(EXIT_FAILURE);
        }

        if (sMatch + 7 < m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].c_str() + m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].size())
        {
            const size_t szLen = (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].c_str() + m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].size()) - (sMatch + 7);
            memcpy(ServerManager::m_pGlobalBuffer + iMsgLen, sMatch + 7, szLen);
            iMsgLen += static_cast<int>(szLen);
        }
    }
    else
    {
        memcpy(ServerManager::m_pGlobalBuffer, m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].data(), m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].size());
        iMsgLen = static_cast<int>(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_MSG)].size());
    }

    EndLimitMessage(iMsgLen, std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_MAX_HUBS_LIMIT_MSG), std::to_underlying(SetBoolIds::SETBOOL_MAX_HUBS_LIMIT_REDIR), std::to_underlying(SetTxtIds::SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS), "UpdateMaxHubsLimitMessage");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateNoTagMessage()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    if (m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_NO_TAG_OPTION)] == 0)
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_NO_TAG_MSG)].clear();

        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "<%s> %s|",
                           m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SEC)].c_str(),
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NO_TAG_MSG)].c_str());
    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateNoTagMessage");
        exit(EXIT_FAILURE);
    }

    if (m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_NO_TAG_OPTION)] == 2)
    {
        if (!m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NO_TAG_REDIR_ADDRESS)].empty())
        {
            if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                                iMsgLen,
                                ServerManager::m_szGlobalBufferSize,
                                "$ForceMove %s|",
                                m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NO_TAG_REDIR_ADDRESS)].c_str()))
            {
                LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateNoTagMessage");
                exit(EXIT_FAILURE);
            }
        }
        else if (!m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].empty())
        {
            memcpy(ServerManager::m_pGlobalBuffer + iMsgLen, m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].data(), m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].size());
            iMsgLen += static_cast<int>(m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].size());
        }
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_NO_TAG_MSG), iMsgLen, "UpdateNoTagMessage");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateTempBanRedirAddress()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = 0;

    if (!m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TEMP_BAN_REDIR_ADDRESS)].empty())
    {
        iMsgLen =
            snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$ForceMove %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TEMP_BAN_REDIR_ADDRESS)].c_str());
        if (iMsgLen <= 0)
        {
            LogDbg("[ERR] snprintf failed in SettingManager::UpdateTempBanRedirAddress");
            exit(EXIT_FAILURE);
        }
    }
    else if (!m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].empty())
    {
        const auto& redirAddr = m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)];
        memcpy(ServerManager::m_pGlobalBuffer, redirAddr.data(), redirAddr.size());
        iMsgLen = static_cast<int>(redirAddr.size());
    }
    else
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_TEMP_BAN_REDIR_ADDRESS)].clear();
        return;
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_TEMP_BAN_REDIR_ADDRESS), iMsgLen, "UpdateTempBanRedirAddress");
}
//---------------------------------------------------------------------------

void SettingManager::UpdatePermBanRedirAddress()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = 0;

    if (!m_sTexts[std::to_underlying(SetTxtIds::SETTXT_PERM_BAN_REDIR_ADDRESS)].empty())
    {
        iMsgLen =
            snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$ForceMove %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_PERM_BAN_REDIR_ADDRESS)].c_str());
        if (iMsgLen <= 0)
        {
            LogDbg("[ERR] snprintf failed in SettingManager::UpdatePermBanRedirAddress");
            exit(EXIT_FAILURE);
        }
    }
    else if (!m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)].empty())
    {
        const auto& redirAddr = m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_REDIRECT_ADDRESS)];
        memcpy(ServerManager::m_pGlobalBuffer, redirAddr.data(), redirAddr.size());
        iMsgLen = static_cast<int>(redirAddr.size());
    }
    else
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_PERM_BAN_REDIR_ADDRESS)].clear();
        return;
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_PERM_BAN_REDIR_ADDRESS), iMsgLen, "UpdatePermBanRedirAddress");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateNickLimitMessage()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    int iMsgLen = BeginLimitMessage();

    for (size_t ui16i = 0; ui16i < m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG)].size(); ui16i++)
    {
        if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG)][ui16i] == '%')
        {
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG)].compare(ui16i + 1, 5, g_sMin) == 0)
            {
                if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "%hd", m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_NICK_LEN)]))
                {
                    LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateNickLimitMessage");
                    exit(EXIT_FAILURE);
                }

                ui16i += static_cast<uint16_t>(5);
                continue;
            }
            if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG)].compare(ui16i + 1, 5, g_sMax) == 0)
            {
                if (m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_NICK_LEN)] != 0)
                {
                    if (!SnprintfAppend(
                            ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "%hd", m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_NICK_LEN)]))
                    {
                        LogDbg("[ERR] SnprintfAppend failed in SettingManager::UpdateNickLimitMessage");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    memcpy(ServerManager::m_pGlobalBuffer + iMsgLen,
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].c_str(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].size());
                    iMsgLen += static_cast<int>(LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_UNLIMITED)].size());
                }
                ui16i += static_cast<uint16_t>(5);
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_MSG)][ui16i];
        iMsgLen++;
    }

    EndLimitMessage(iMsgLen, std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_NICK_LIMIT_MSG), std::to_underlying(SetBoolIds::SETBOOL_NICK_LIMIT_REDIR), std::to_underlying(SetTxtIds::SETTXT_NICK_LIMIT_REDIR_ADDRESS), "UpdateNickLimitMessage");
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMinShare()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    m_ui64MinShare = static_cast<uint64_t>(m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT)] == 0
                                               ? 0
                                               : m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_LIMIT)] * pow(1024.0, static_cast<int>(m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SHARE_UNITS)])));
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMaxShare()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    m_ui64MaxShare = static_cast<uint64_t>(m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT)] == 0
                                               ? 0
                                               : m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_LIMIT)] * pow(1024.0, static_cast<int>(m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SHARE_UNITS)])));
}
//---------------------------------------------------------------------------

void SettingManager::UpdateTCPPorts()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    char* sPort = m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS)].data();
    uint8_t ui8ActualPort = 0;
    for (uint16_t ui16i = 0; ui16i < m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS)].size() && ui8ActualPort < 25; ui16i++)
    {
        if (m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS)][ui16i] == ';')
        {
            m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS)][ui16i] = '\0';

            int iPort = 0;
            if (safe_stoi(sPort, iPort))
            {
                if (ui8ActualPort != 0)
                {
                    m_ui16PortNumbers[ui8ActualPort] = static_cast<uint16_t>(iPort);
                }
                else
                {
                    Lock l(m_csSetting);
                    m_ui16PortNumbers[ui8ActualPort] = static_cast<uint16_t>(iPort);
                }
            }

            m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS)][ui16i] = ';';

            sPort = m_sTexts[std::to_underlying(SetTxtIds::SETTXT_TCP_PORTS)].data() + ui16i + 1;
            ui8ActualPort++;
            continue;
        }
    }

    if (sPort[0] != '\0')
    {
        int iPort = 0;
        if (safe_stoi(sPort, iPort))
        {
            m_ui16PortNumbers[ui8ActualPort] = static_cast<uint16_t>(iPort);
            ui8ActualPort++;
        }
    }

    while (ui8ActualPort < 25)
    {
        m_ui16PortNumbers[ui8ActualPort] = 0;
        ui8ActualPort++;
    }

    if (!ServerManager::m_bServerRunning)
    {
        return;
    }

    ServerManager::UpdateServers();
}
//---------------------------------------------------------------------------

void SettingManager::UpdateBotsSameNick()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    if (!m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].empty() && !m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].empty() && m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] && m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)])
    {
        m_bBotsSameNick = iequals(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)], m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)]);
    }
    else
    {
        m_bBotsSameNick = false;
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateLanguage()
{
    if (m_bUpdateLocked)
    {
        return;
    }

    LanguageManager::m_Ptr->Load();

    UpdateHubNameWelcome();
}
//---------------------------------------------------------------------------

void SettingManager::UpdateBot(const bool bNickChanged /* = true*/)
{
    if (m_bUpdateLocked)
    {
        return;
    }

    if (!m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)])
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_BOT_MYINFO)].clear();

        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "$MyINFO $ALL %s %s$ $ $%s$$|",
                           m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str(),
                           !m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)].empty() ? m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_DESCRIPTION)].c_str() : "",
                           !m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)].empty() ? m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_EMAIL)].c_str() : "");
    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateBot");
        exit(EXIT_FAILURE);
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_BOT_MYINFO), iMsgLen, "UpdateBot");

    if (ServerManager::m_Servers.empty())
    {
        return;
    }

    if (bNickChanged && (!m_bBotsSameNick || !ServerManager::m_Servers.front()->m_bActive))
    {
        Users::m_Ptr->AddBot2NickList(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)], true);
    }

    Users::m_Ptr->AddBot2MyInfos(m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_BOT_MYINFO)].c_str());

    if (!ServerManager::m_Servers.front()->m_bActive)
    {
        return;
    }

    if (bNickChanged && !m_bBotsSameNick)
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Hello %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str());
        if (iMsgLen > 0)
        {
            GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::HELLO);
        }
    }

    GlobalDataQueue::m_Ptr->AddQueueItem(m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_BOT_MYINFO)], m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_BOT_MYINFO)], GlobalDataQueue::Cmd::MYINFO);

    if (bNickChanged)
    {
        GlobalDataQueue::m_Ptr->OpListStore(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str());
    }
}
//---------------------------------------------------------------------------

void SettingManager::DisableBot(const bool bNickChanged /* = true*/, const bool bRemoveMyINFO /* = true*/)
{
    if (m_bUpdateLocked || !ServerManager::m_bServerRunning)
    {
        return;
    }

    if (bNickChanged)
    {
        if (!m_bBotsSameNick)
        {
            Users::m_Ptr->DelFromNickList(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str(), true);
        }

        const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_BOT_NICK)].c_str());
        if (iMsgLen > 0)
        {
            if (m_bBotsSameNick)
            {
                // PPK ... send Quit only to users without opchat permission...
                for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
                {
                    User* curUser = curUserPtr.get();
                    if (curUser->m_ui8State == User::UserStates::STATE_ADDED && !ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT))
                    {
                        curUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);
                    }
                }
            }
            else
            {
                GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::QUIT);
            }
        }
    }

    if (!m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_BOT_MYINFO)].empty() && !m_bBotsSameNick && bRemoveMyINFO)
    {
        Users::m_Ptr->DelBotFromMyInfos(m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_BOT_MYINFO)].c_str());
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateOpChat(const bool bNickChanged /* = true*/)
{
    if (m_bUpdateLocked)
    {
        return;
    }

    if (!m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)])
    {
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO)].clear();
        m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO)].clear();

        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Hello %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateOpChat");
        exit(EXIT_FAILURE);
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO), iMsgLen, "UpdateOpChatHello");

    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                       ServerManager::m_szGlobalBufferSize,
                       "$MyINFO $ALL %s %s$ $ $%s$$|",
                       m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str(),
                       !m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)].empty() ? m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_DESCRIPTION)].c_str() : "",
                       !m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)].empty() ? m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_EMAIL)].c_str() : "");
    if (iMsgLen <= 0)
    {
        LogDbg("[ERR] snprintf failed in SettingManager::UpdateOpChat");
        exit(EXIT_FAILURE);
    }

    CommitGlobalBufferToPreText(std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO), iMsgLen, "UpdateOpChatMyInfo");

    if (!ServerManager::m_bServerRunning)
    {
        return;
    }

    if (!m_bBotsSameNick)
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$OpList %s$$|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
        if (iMsgLen <= 0)
        {
            LogDbg("[ERR] snprintf failed in SettingManager::UpdateOpChat");
            exit(EXIT_FAILURE);
        }

        for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
        {
            User* curUser = curUserPtr.get();
            if (curUser->m_ui8State == User::UserStates::STATE_ADDED && ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT))
            {
                if (bNickChanged && (!((curUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO)))
                {
                    curUser->SendCharDelayed(m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO)]);
                }
                curUser->SendCharDelayed(m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO)]);
                if (bNickChanged)
                {
                    curUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::DisableOpChat(const bool bNickChanged /* = true*/)
{
    if (m_bUpdateLocked || ServerManager::m_Servers.empty() || m_bBotsSameNick)
    {
        return;
    }

    if (bNickChanged)
    {
        Users::m_Ptr->DelFromNickList(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str(), true);

        const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
        if (iMsgLen > 0)
        {
            for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
            {
                User* curUser = curUserPtr.get();
                if (curUser->m_ui8State == User::UserStates::STATE_ADDED && ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT))
                {
                    curUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateUDPPort()
{
    if (m_bUpdateLocked || !ServerManager::m_bServerRunning)
    {
        return;
    }
#ifdef FLYLINKDC_USE_UDP_THREAD
    UDPThread::Destroy(UDPThread::m_PtrIPv6);
    UDPThread::m_PtrIPv6 = nullptr;

    UDPThread::Destroy(UDPThread::m_PtrIPv4);
    UDPThread::m_PtrIPv4 = nullptr;

    int iUdpPort = 0;
    if (safe_stoi(m_sTexts[std::to_underlying(SetTxtIds::SETTXT_UDP_PORT)].c_str(), iUdpPort) && static_cast<uint16_t>(iUdpPort) != 0)
    {
        if (!ServerManager::m_bUseIPv6)
        {
            UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
            return;
        }

        UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET6);

        if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BIND_ONLY_SINGLE_IP)] || !ServerManager::m_bIPv6DualStack)
        {
            UDPThread::Destroy(UDPThread::m_PtrIPv6);
            UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
        }
    }
#endif
}
//---------------------------------------------------------------------------

void SettingManager::UpdateScripting() const
{
    if (m_bUpdateLocked || !ServerManager::m_bServerRunning)
    {
        return;
    }

    if (m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_SCRIPTING)])
    {
        ScriptManager::m_Ptr->Start();
        ScriptManager::m_Ptr->OnStartup();
    }
    else
    {
        ScriptManager::m_Ptr->OnExit(true);
        ScriptManager::m_Ptr->Stop();
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateDatabase()
{
#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
    if (!DBSQLite::m_Ptr)
    {
        return;
    }

    DBSQLite::m_Ptr = std::make_unique<DBSQLite>();
    if (!DBSQLite::m_Ptr) //-V547 new(nothrow) can return nullptr
    {
        LogDbg("[MEM] Cannot allocate DBSQLite::m_Ptr in SettingManager::SetBool");
        exit(EXIT_FAILURE);
    }
#endif
#endif // FLYLINKDC_USE_DB
}
//---------------------------------------------------------------------------

void SettingManager::CmdLineBasicSetup()
{
    m_bUpdateLocked = true;

    printf("\nWelcome to basic setup.\nYou will now be asked for few settings required to run PtokaX.\nWhen you don't want to change default settings, then "
           "simply press enter.\n");

    int16_t i16MaxUsers = 0;
    std::array<char, 7> g_sMaxUsers = {};

    for (;;)
    {
        printf("%sActual value is: %hd\nEnter new value: ", SetShortCom[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS)] + 2, m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS)]);
        if (fgets(g_sMaxUsers.data(), g_sMaxUsers.size(), stdin))
        {
            if (g_sMaxUsers[0] != '\n')
            {
                char* sMatch = strchr(g_sMaxUsers.data(), '\n');
                if (sMatch)
                {
                    sMatch[0] = '\0';
                }

                const auto ui8Len = static_cast<uint8_t>(strlen(g_sMaxUsers.data()));

                bool bValid = true;
                for (const auto& c : std::string_view(g_sMaxUsers.data(), ui8Len))
                {
                    if (!isdigit(static_cast<unsigned char>(c)))
                    {
                        printf("Character '%c' is not valid number!\n", c);
                        bValid = false;
                        break;
                    }
                }

                if (!bValid)
                {
                    if (!WantAgain())
                        return;
                    continue;
                }

                int iMaxUsers = 0;
                if (!safe_stoi(g_sMaxUsers.data(), iMaxUsers))
                {
                    printf("Invalid number '%s'!\n", g_sMaxUsers.data());
                    if (!WantAgain())
                        return;
                    continue;
                }

                i16MaxUsers = static_cast<int16_t>(iMaxUsers);
                SetShort(std::to_underlying(SetShortIds::SETSHORT_MAX_USERS), i16MaxUsers);

                if (i16MaxUsers != m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_USERS)])
                {
                    printf("Failed to set value %hd!\n", i16MaxUsers);
                    if (!WantAgain())
                        return;
                    continue;
                }
            }
        }
        else
        {
            printf("Error reading value... ending.\n");
            exit(EXIT_FAILURE);
        }

        break;
    }

    const uint8_t ui8Strings[] = {std::to_underlying(SetTxtIds::SETTXT_HUB_NAME), std::to_underlying(SetTxtIds::SETTXT_HUB_ADDRESS), std::to_underlying(SetTxtIds::SETTXT_ENCODING)}; // NOLINT(modernize-avoid-c-arrays)

    std::string sValue(4098, '\0');

    for (unsigned char ui8String : ui8Strings)
    {
        for (;;)
        {
            printf("%sActual value is: %s\nEnter new value: ", SetTxtCom[ui8String] + 2, !m_sTexts[ui8String].empty() ? m_sTexts[ui8String].c_str() : "");
            if (fgets(sValue.data(), static_cast<int>(sValue.size()), stdin))
            {
                if (sValue[0] == '\n')
                {
                    break;
                }

                char* sMatch = strchr(sValue.data(), '\n');
                if (sMatch)
                {
                    sMatch[0] = '\0';
                    sValue.resize(static_cast<size_t>(sMatch - sValue.data()));
                }
                else
                {
                    sValue.resize(strlen(sValue.data()));
                }

                SetText(ui8String, sValue);

                if ((sValue.empty() && !m_sTexts[ui8String].empty()) || sValue != m_sTexts[ui8String])
                {
                    printf("Failed to set new string value. Incorrect length or invalid characters?\n");
                    if (!WantAgain())
                        return;
                    continue;
                }
            }
            else
            {
                printf("Error reading string value... ending.\n");
                exit(EXIT_FAILURE);
            }

            break;
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::CmdLineCompleteSetup()
{
    m_bUpdateLocked = true;

    printf("\nWelcome to complete setup.\nYou will now be asked for all PtokaX settings.\nWhen you don't want to change default settings, then simply press "
           "enter.\n\nFirst we set boolean settings. Use 1 for enabled and 0 for disabled.\n\n");

    std::string sValue(4098, '\0');

    for (size_t szi = 0; szi < std::to_underlying(SetBoolIds::SETBOOL_IDS_END); szi++)
    {
        // skip obsolete settings
        if (SetBoolStr[szi][0] == '\0')
        {
            continue;
        }
        for (;;)
        {
            printf("%sActual value is: %c\nEnter new value: ", SetBoolCom[szi] + 2, m_bBools[szi] ? '1' : '0');
            if (fgets(sValue.data(), 3, stdin))
            {
                if (sValue[0] == '\n')
                {
                    break;
                }

                if (sValue[0] != '0' && sValue[0] != '1')
                {
                    printf("You need to use 1 or 0 for new value!\n");
                    if (!WantAgain())
                        return;
                    continue;
                }

                SetBool(szi, sValue[0] != '0');
            }
            else
            {
                printf("Error reading boolean value... ending.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
    }

    printf("\nWe finished boolean settings. Now we will set number settings.\n\n");

    int16_t i16Value = 0;

    for (size_t szi = 0; szi < (std::to_underlying(SetShortIds::SETSHORT_IDS_END) - 1); szi++)
    {
        for (;;)
        {
            printf("%sActual value is: %hd\nEnter new value: ", SetShortCom[szi] + 2, m_i16Shorts[szi]);
            if (fgets(sValue.data(), 7, stdin))
            {
                if (sValue[0] == '\n')
                {
                    break;
                }

                char* sMatch = strchr(sValue.data(), '\n');
                if (sMatch)
                {
                    sMatch[0] = '\0';
                    sValue.resize(static_cast<size_t>(sMatch - sValue.data()));
                }
                else
                {
                    sValue.resize(strlen(sValue.data()));
                }

                const auto ui8Len = static_cast<uint8_t>(sValue.size());

                bool bValid = true;
                for (const auto& c : std::string_view(sValue.data(), ui8Len))
                {
                    if (!isdigit(static_cast<unsigned char>(c)))
                    {
                        printf("Character '%c' is not valid number!\n", c);
                        bValid = false;
                        break;
                    }
                }

                if (!bValid)
                {
                    if (!WantAgain())
                        return;
                    continue;
                }

                int iParsed = 0;
                if (!safe_stoi(sValue.c_str(), iParsed))
                {
                    printf("Invalid number '%s'!\n", sValue.c_str());
                    if (!WantAgain())
                        return;
                    continue;
                }

                i16Value = static_cast<int16_t>(iParsed);
                SetShort(szi, i16Value);

                if (i16Value != m_i16Shorts[szi])
                {
                    printf("Failed to set value %hd!\n", i16Value);
                    if (!WantAgain())
                        return;
                    continue;
                }
            }
            else
            {
                printf("Error reading number value... ending.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
    }

    printf("\nWe finished number settings. Now we will set string settings.\n\n");

    for (size_t szi = 0; szi < std::to_underlying(SetTxtIds::SETTXT_IDS_END); szi++) // NOLINT(modernize-loop-convert) index used in body
    {
        for (;;)
        {
            printf("%sActual value is: %s\nEnter new value: ", SetTxtCom[szi] + 2, !m_sTexts[szi].empty() ? m_sTexts[szi].c_str() : "");
            if (fgets(sValue.data(), static_cast<int>(sValue.size()), stdin))
            {
                if (sValue[0] == '\n')
                {
                    break;
                }

                char* sMatch = strchr(sValue.data(), '\n');
                if (sMatch)
                {
                    sMatch[0] = '\0';
                    sValue.resize(static_cast<size_t>(sMatch - sValue.data()));
                }
                else
                {
                    sValue.resize(strlen(sValue.data()));
                }

                SetText(szi, sValue);

                if ((sValue.empty() && !m_sTexts[szi].empty()) || sValue != m_sTexts[szi])
                {
                    printf("Failed to set new string value. Incorrect length or invalid characters?\n");
                    if (!WantAgain())
                        return;
                    continue;
                }
            }
            else
            {
                printf("Error reading string value... ending.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
    }
}
//---------------------------------------------------------------------------
