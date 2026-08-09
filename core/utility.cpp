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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <fstream>
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
#include "Log.h"
#include "hashBanManager.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "GlobalDataQueue.h"
#include "User.h"
#include "IP2Country.h"
//---------------------------------------------------------------------------
#include <tinyxml2.h>
#include <fmt/format.h>
//---------------------------------------------------------------------------
#ifndef _WITHOUT_SKEIN
#include <skein.h>
#endif
//---------------------------------------------------------------------------
/*
static constexpr int g_iMaxPatSize = 64;
static constexpr int g_iMaxAlphabetSize = 255;
*/
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/*
 */
//---------------------------------------------------------------------------

std::string Lock2Key(const char* sLock)
{
    static constexpr uint8_t ui8LockSize = 46;
    std::string result;
    result.reserve(461);

    // $Lock EXTENDEDPROTOCOL33MTL6@h5AIad^P2UoPv?fZU]ivM6 Pk=PtokaX|

    sLock = sLock + 6; // set begin after $Lock_

    uint8_t v;

    // first make the crypting stuff
    for (uint8_t ui8i = 0; ui8i < ui8LockSize; ui8i++)
    {
        if (ui8i == 0)
        {
            v = static_cast<uint8_t>(sLock[0] ^ sLock[45] ^ sLock[44] ^ 5);
        }
        else
        {
            v = sLock[ui8i] ^ sLock[ui8i - 1];
        }

        // swap nibbles (0xF0 = 11110000, 0x0F = 00001111)

        v = static_cast<uint8_t>(((v << 4) & 0xF0) | ((v >> 4) & 0x0F));

        if (result.size() >= 460)
        {
            break;
        }

        switch (v)
        {
        case 0:
            result += "/%DCN000%/";
            break;
        case 5:
            result += "/%DCN005%/";
            break;
        case 36:
            result += "/%DCN036%/";
            break;
        case 96:
            result += "/%DCN096%/";
            break;
        case 124:
            result += "/%DCN124%/";
            break;
        case 126:
            result += "/%DCN126%/";
            break;
        default:
            result += static_cast<char>(v);
            break;
        }
    }
    return result;
}
//---------------------------------------------------------------------------

bool VerifyLockKey(const char* sLock, const char* sReceivedKey)
{
    static constexpr uint8_t ui8LockSize = 46;

    if (!sReceivedKey)
    {
        return false;
    }

    sLock = sLock + 6; // set begin after $Lock_

    size_t pos = 0; // position in sReceivedKey

    for (uint8_t ui8i = 0; ui8i < ui8LockSize; ui8i++)
    {
        uint8_t v;
        if (ui8i == 0)
        {
            v = static_cast<uint8_t>(sLock[0] ^ sLock[45] ^ sLock[44] ^ 5);
        }
        else
        {
            v = sLock[ui8i] ^ sLock[ui8i - 1];
        }

        v = static_cast<uint8_t>(((v << 4) & 0xF0) | ((v >> 4) & 0x0F));

        // Expected encoded form for this byte
        const char* expected = nullptr;
        switch (v)
        {
        case 0:
            expected = "/%DCN000%/";
            break;
        case 5:
            expected = "/%DCN005%/";
            break;
        case 36:
            expected = "/%DCN036%/";
            break;
        case 96:
            expected = "/%DCN096%/";
            break;
        case 124:
            expected = "/%DCN124%/";
            break;
        case 126:
            expected = "/%DCN126%/";
            break;
        default:
            if (sReceivedKey[pos] != static_cast<char>(v))
            {
                return false;
            }
            pos++;
            continue;
        }

        // Compare the DCN-encoded form
        for (const char* p = expected; *p != '\0'; p++)
        {
            if (sReceivedKey[pos] != *p)
            {
                return false;
            }
            pos++;
        }
    }

    return sReceivedKey[pos] == '\0';
}
//---------------------------------------------------------------------------

const char* ErrnoStr(const uint32_t ui32Error)
{
    static const char* const errStrings[] = {
        // NOLINT(modernize-avoid-c-arrays)
        "UNDEFINED",
        "EADDRINUSE",
        "EADDRNOTAVAIL",
        "ECONNRESET",
        "ETIMEDOUT",
        "ECONNREFUSED",
        "EHOSTUNREACH",
    };

    switch (ui32Error)
    {
    case 98:
        return errStrings[1];
    case 99:
        return errStrings[2];
    case 104:
        return errStrings[3];
    case 110:
        return errStrings[4];
    case 111:
        return errStrings[5];
    case 113:
        return errStrings[6];
    default:
        return errStrings[0];
    }
}
//---------------------------------------------------------------------------

std::string formatBytes(const uint64_t ui64Bytes)
{
    static const char* const unit[] = {
        "B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB", " ", " ", " ", " ", " ", " ", " "}; // NOLINT(modernize-avoid-c-arrays)

    if (ui64Bytes < 1024)
    {
        return std::to_string(ui64Bytes) + " " + unit[0];
    }

    auto ldBytes = static_cast<long double>(ui64Bytes);
    uint8_t iter = 0;
    for (; ldBytes > 1024; iter++)
    {
        ldBytes /= 1024;
    }

    return fmt::format("{:.2Lf} {}", ldBytes, unit[iter]);
}
//---------------------------------------------------------------------------

std::string formatBytesPerSecond(const uint64_t ui64Bytes)
{
    static const char* const secondunit[] = {
        "B/s", "kB/s", "MB/s", "GB/s", "TB/s", "PB/s", "EB/s", "ZB/s", "YB/s", " ", " ", " ", " ", " ", " ", " "}; // NOLINT(modernize-avoid-c-arrays)

    if (ui64Bytes < 1024)
    {
        return std::to_string(ui64Bytes) + " " + secondunit[0];
    }

    auto ldBytes = static_cast<long double>(ui64Bytes);
    uint8_t iter = 0;
    for (; ldBytes > 1024; iter++)
    {
        ldBytes /= 1024;
    }

    return fmt::format("{:.2Lf} {}", ldBytes, secondunit[iter]);
}
//---------------------------------------------------------------------------

std::string formatTime(uint64_t ui64Rest)
{
    std::string result;
    result.reserve(256);

    uint8_t i = 0;
    uint64_t n = ui64Rest / MINUTES_PER_YEAR;
    ui64Rest -= n * MINUTES_PER_YEAR;

    if (n != 0)
    {
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YEARS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YEAR_LWR)];
        i++;
    }

    n = ui64Rest / MINUTES_PER_MONTH;
    ui64Rest -= n * MINUTES_PER_MONTH;

    if (n != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MONTHS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MONTH_LWR)];
        i++;
    }

    n = ui64Rest / MINUTES_PER_DAY;
    ui64Rest -= n * MINUTES_PER_DAY;

    if (n != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DAYS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DAY_LWR)];
        i++;
    }

    n = ui64Rest / MINUTES_PER_HOUR;
    ui64Rest -= n * MINUTES_PER_HOUR;

    if (n != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HOURS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HOUR_LWR)];
        i++;
    }

    if (ui64Rest != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(ui64Rest);
        result += " ";
        result += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MIN_LWR)];
    }

    return result;
}
//---------------------------------------------------------------------------

std::string formatSecTime(uint64_t ui64Rest)
{
    std::string result;
    result.reserve(256);

    uint8_t i = 0;
    uint64_t n = ui64Rest / SECONDS_PER_YEAR;
    ui64Rest -= n * SECONDS_PER_YEAR;

    if (n != 0)
    {
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YEARS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YEAR_LWR)];
        i++;
    }

    n = ui64Rest / SECONDS_PER_MONTH;
    ui64Rest -= n * SECONDS_PER_MONTH;

    if (n != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MONTHS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MONTH_LWR)];
        i++;
    }

    n = ui64Rest / SECONDS_PER_DAY;
    ui64Rest -= n * SECONDS_PER_DAY;

    if (n != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DAYS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_DAY_LWR)];
        i++;
    }

    n = ui64Rest / SECONDS_PER_HOUR;
    ui64Rest -= n * SECONDS_PER_HOUR;

    if (n != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(n);
        result += " ";
        result += n > 1 ? LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HOURS_LWR)]
                        : LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HOUR_LWR)];
        i++;
    }

    n = ui64Rest / SECONDS_PER_MINUTE;
    ui64Rest -= n * SECONDS_PER_MINUTE;

    if (n != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(n);
        result += " ";
        result += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_MIN_LWR)];
        i++;
    }

    if (ui64Rest != 0)
    {
        if (i > 0)
            result += " ";
        result += std::to_string(ui64Rest);
        result += " ";
        result += LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SEC_LWR)];
    }

    return result;
}
//---------------------------------------------------------------------------

char* stristr(const char* str1, const char* str2)
{
    char* cp = const_cast<char*>(str1); // NOLINT(misc-const-correctness)
    if (*str2 == 0)
    {
        return const_cast<char*>(str1);
    }
    while (*cp != 0)
    {
        const char* s1 = cp;
        const char* s2 = str2;
        while (*s1 != 0 && *s2 != 0 && ((*s1 - *s2) == 0 || (*s1 - tolower(*s2)) == 0 || (*s1 - toupper(*s2)) == 0))
        {
            s1++, s2++;
        }
        if (*s2 == 0)
        {
            return cp;
        }
        cp++;
    }
    return nullptr;
}
//---------------------------------------------------------------------------

// check IP string.
// false - no ip
// true - valid ip
bool isIP(const char* sIP)
{
    if (ServerManager::m_bUseIPv6 && !strchr(sIP, '.'))
    {
        if (strlen(sIP) > 39)
        {
            return false;
        }

        std::array<uint8_t, 16> ui128IpHash = {};
        if (inet_pton(AF_INET6, sIP, ui128IpHash.data()) != 1)
        {
            return false;
        }
    }
    else
    {
        if (strlen(sIP) > 15)
        {
            return false;
        }

        const uint32_t ui32IpHash = inet_addr(sIP);

        if (ui32IpHash == INADDR_NONE)
        {
            return false;
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void LogXmlError(const char* sFileName, const char* sErrorDesc, const int iColumn, const int iRow)
{
    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                 ServerManager::m_szGlobalBufferSize,
                                 "Error loading file %s. %s (Col: %d, Row: %d)",
                                 sFileName,
                                 sErrorDesc,
                                 iColumn,
                                 iRow);
    if (iMsgLen > 0)
    {
        LogInfo("{}", ServerManager::m_pGlobalBuffer);
    }
}
//---------------------------------------------------------------------------

bool LoadXmlConfig(tinyxml2::XMLDocument& doc, const std::string& fileName)
{

    if (doc.LoadFile((ServerManager::m_sPath + "/cfg/" + fileName).c_str()) != tinyxml2::XML_SUCCESS)
    {
        if (doc.ErrorID() != tinyxml2::XML_ERROR_FILE_NOT_FOUND && doc.ErrorID() != tinyxml2::XML_ERROR_EMPTY_DOCUMENT)
        {
            LogXmlError(fileName.c_str(), doc.ErrorStr(), 0, 0);
            exit(EXIT_FAILURE);
        }
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

const char* XmlGetRequiredText(tinyxml2::XMLElement* parent, const char* tagName)
{
    tinyxml2::XMLElement* elem = parent->FirstChildElement(tagName); // NOLINT(misc-const-correctness)
    if (!elem)
    {
        return nullptr;
    }
    return elem->GetText();
}
//---------------------------------------------------------------------------

const char* XmlGetOptionalText(tinyxml2::XMLElement* parent, const char* tagName)
{
    tinyxml2::XMLElement* elem = parent->FirstChildElement(tagName); // NOLINT(misc-const-correctness)
    if (!elem)
    {
        return nullptr;
    }
    return elem->GetText();
}
//---------------------------------------------------------------------------

bool XmlGetRequiredInt(tinyxml2::XMLElement* parent, const char* tagName, int& outVal)
{
    const char* sVal = XmlGetRequiredText(parent, tagName);
    if (!sVal)
    {
        return false;
    }
    return safe_stoi(sVal, outVal);
}
//---------------------------------------------------------------------------

bool AppendHubSecPrefix(int& iMsgLen)
{
    return SnprintfAppend(ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "<%s> ", SettingManager::HubSec());
}
//---------------------------------------------------------------------------

bool AppendLabeledField(int& iMsgLen, const int iLangId, const char* pData, const size_t uiDataLen)
{
    if (!pData || uiDataLen == 0)
    {
        return true;
    }

    if (!SnprintfAppend(
            ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[iLangId].c_str()))
    {
        return false;
    }

    if (static_cast<size_t>(iMsgLen) + uiDataLen >= ServerManager::m_szGlobalBufferSize)
    {
        return false;
    }

    memcpy(ServerManager::m_pGlobalBuffer + iMsgLen, pData, uiDataLen);
    iMsgLen += static_cast<int>(uiDataLen);
    return true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

bool HashIP(const char* sIP, uint8_t* ui128IpHash)
{
    if (ServerManager::m_bUseIPv6 && !strchr(sIP, '.'))
    {
        if (strlen(sIP) > 39)
        {
            return false;
        }

        if (inet_pton(AF_INET6, sIP, ui128IpHash) != 1)
        {
            return false;
        }
    }
    else
    {
        if (strlen(sIP) > 15)
        {
            return false;
        }

        const uint32_t ui32IpHash = inet_addr(sIP);

        if (ui32IpHash == INADDR_NONE)
        {
            return false;
        }

        memset(ui128IpHash, 0, 16);

        ui128IpHash[10] = 255;
        ui128IpHash[11] = 255;

        memcpy(ui128IpHash + 12, &ui32IpHash, 4);
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Shared footer for ban messages — Reason, By, CustomMsg, Redirect
namespace
{
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool AppendBanFooter(int& iMsgLen, const BanItemBase* pBan)
{
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_REASON)] && !pBan->m_sReason.empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_REASON)].c_str(),
                            pBan->m_sReason.c_str()))
        {
            LogDbgErr("[ERR] snprintf append failed in AppendBanFooter");
            return false;
        }
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_BY)] && !pBan->m_sBy.empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_BANNED_BY)].c_str(),
                            pBan->m_sBy.c_str()))
        {
            LogDbgErr("[ERR] snprintf append failed in AppendBanFooter");
            return false;
        }
    }

    if (!SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MSG_TO_ADD_TO_BAN_MSG)].empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s|",
                            SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_MSG_TO_ADD_TO_BAN_MSG)].c_str()))
        {
            LogDbgErr("[ERR] snprintf append failed in AppendBanFooter");
            return false;
        }
    }
    else
    {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
        iMsgLen++;
        ServerManager::m_pGlobalBuffer[iMsgLen] = '\0';
    }

#ifdef FLYLINKDC_USE_REDIR
    if (((pBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
    {
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_PERM_BAN_REDIR)] &&
            !SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_PERM_BAN_REDIR_ADDRESS)].empty())
        {
            (void)SnprintfAppend(
                ServerManager::m_pGlobalBuffer,
                iMsgLen,
                ServerManager::m_szGlobalBufferSize,
                "%s",
                SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_PERM_BAN_REDIR_ADDRESS)].c_str());
        }
    }
    else
    {
        if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_TEMP_BAN_REDIR)] &&
            !SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_TEMP_BAN_REDIR_ADDRESS)].empty())
        {
            (void)SnprintfAppend(
                ServerManager::m_pGlobalBuffer,
                iMsgLen,
                ServerManager::m_szGlobalBufferSize,
                "%s",
                SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_TEMP_BAN_REDIR_ADDRESS)].c_str());
        }
    }
#endif

    return true;
}
} // namespace
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int GenerateBanMessage(BanItem* pBan, time_t tAccTime)
{
    int iMsgLen = 0;

    if (((pBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "<%s> %s.",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_PERM_BANNED)].c_str());
    }
    else
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "<%s> %s: %s.",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_TEMP_BANNED)].c_str(),
                           formatSecTime(pBan->m_tTempBanExpire - tAccTime).c_str());
    }

    if (iMsgLen <= 0)
    {
        LogDbgErr("[ERR] snprintf wrong value {} in GenerateBanMessage1", iMsgLen);

        return 0;
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_IP)] && pBan->m_sIp[0] != '\0')
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                            pBan->m_sIp.data()))
        {
            LogDbgErr("[ERR] snprintf append failed in GenerateBanMessage2");

            return 0;
        }
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_NICK)] && !pBan->m_sNick.empty())
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_NICK)].c_str(),
                            pBan->m_sNick.c_str()))
        {
            LogDbgErr("[ERR] snprintf append failed in GenerateBanMessage3");

            return 0;
        }
    }

    if (!AppendBanFooter(iMsgLen, pBan))
    {
        return 0;
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

int GenerateRangeBanMessage(RangeBanItem* pRangeBan, time_t tAccTime)
{
    int iMsgLen = 0;

    if (((pRangeBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "<%s> %s.",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_PERM_BANNED)].c_str());
    }
    else
    {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                           ServerManager::m_szGlobalBufferSize,
                           "<%s> %s: %s",
                           SettingManager::HubSec(),
                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SORRY_TEMP_BANNED)].c_str(),
                           formatSecTime(pRangeBan->m_tTempBanExpire - tAccTime).c_str());
    }

    if (iMsgLen <= 0)
    {
        LogDbgErr("[ERR] snprintf wrong value {} in GenerateRangeBanMessage1", iMsgLen);

        return 0;
    }

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_BAN_MSG_SHOW_RANGE)])
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s-%s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_RANGE)].c_str(),
                            pRangeBan->m_sIpFrom.data(),
                            pRangeBan->m_sIpTo.data()))
        {
            LogDbgErr("[ERR] snprintf append failed in GenerateRangeBanMessage2");

            return 0;
        }
    }

    if (!AppendBanFooter(iMsgLen, pRangeBan))
    {
        return 0;
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

bool GenerateTempBanTime(const uint8_t ui8Multiplyer, const uint32_t ui32Time, time_t& tAccTime, time_t& tBanTime)
{
    time(&tAccTime);
    struct tm* tm = localtime(&tAccTime);

    switch (ui8Multiplyer)
    {
    case 'm':
        tm->tm_min += static_cast<int>(ui32Time);
        break;
    case 'h':
        tm->tm_hour += static_cast<int>(ui32Time);
        break;
    case 'd':
        tm->tm_mday += static_cast<int>(ui32Time);
        break;
    case 'w':
        tm->tm_mday += static_cast<int>(ui32Time * 7);
        break;
    case 'M':
        tm->tm_mon += static_cast<int>(ui32Time);
        break;
    case 'Y':
        tm->tm_year += static_cast<int>(ui32Time);
        break;
    default:
        return false;
    }

    tm->tm_isdst = -1;

    tBanTime = mktime(tm);

    if (tBanTime == static_cast<time_t>(-1))
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------
// + alex82 ... from MOD
bool CheckSprintf(const int iRetVal, const size_t szMax, const char* sMsg)
{
    if (iRetVal > 0)
    {
        if (szMax != 0 && iRetVal >= static_cast<int>(szMax))
        {
            const std::string sDbgstr =
                "[ERR] sprintf high value " + std::to_string(iRetVal) + "/" + std::to_string(static_cast<uint64_t>(szMax)) + " in " + px_str(sMsg) + "\n";
            LogDbg("{}", sDbgstr);
            return false;
        }
    }
    else
    {
        const std::string sDbgstr = "[ERR] sprintf low value " + std::to_string(iRetVal) + " in " + px_str(sMsg) + "\n";
        LogDbg("{}", sDbgstr);
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

bool CheckSprintf1(const int iRetVal, const size_t szLenVal, const size_t szMax, const char* sMsg)
{
    if (iRetVal > 0)
    {
        if (szMax != 0 && szLenVal >= szMax)
        {
            const std::string sDbgstr = "[ERR] sprintf high value " + std::to_string(static_cast<uint64_t>(szLenVal)) + "/" +
                                        std::to_string(static_cast<uint64_t>(szMax)) + " in " + px_str(sMsg) + "\n";
            LogDbg("{}", sDbgstr);
            return false;
        }
    }
    else
    {
        const std::string sDbgstr = "[ERR] sprintf low value " + std::to_string(iRetVal) + " in " + px_str(sMsg) + "\n";
        LogDbg("{}", sDbgstr);
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

bool FileExist(const char* sPath)
{
    struct stat st;
    if (stat(sPath, &st) == 0 && S_ISDIR(st.st_mode) == 0)
    {
        return true;
    }

    return false;
}
//---------------------------------------------------------------------------

bool DirExist(const char* sPath)
{
    struct stat st;
    if (stat(sPath, &st) == 0 && S_ISDIR(st.st_mode))
    {
        return true;
    }

    return false;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CheckForIPv4()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        if (errno == EAFNOSUPPORT)
        {
            ServerManager::m_bUseIPv4 = false;
            return;
        }
    }
    safe_closesocket(sock);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CheckForIPv6()
{
    int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        if (errno == EAFNOSUPPORT)
        {
            ServerManager::m_bUseIPv6 = false;
            return;
        }
    }

    const int iIPv6 = 0;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &iIPv6, sizeof(iIPv6)) != -1)
    {
        ServerManager::m_bIPv6DualStack = true;
    }

    safe_closesocket(sock);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool GetMacAddress([[maybe_unused]] const char* sIP, [[maybe_unused]] char* sMac)
{
#ifdef FLYLINKDC_USE_MAC
    FILE* fp = fopen("/proc/net/arp", "r");
    if (fp)
    {
        bool bLastCharSpace = true;
        uint8_t ui8NonSpaces = 0;
        const uint16_t ui16IpLen = static_cast<uint16_t>(strlen(sIP));
        std::array<char, 1024> buf = {};

        while (fgets(buf.data(), buf.size(), fp))
        {
            if (strncmp(buf.data(), sIP, ui16IpLen) == 0 && buf[ui16IpLen] == ' ')
            {
                bLastCharSpace = true;
                ui8NonSpaces = 0;
                while (buf[ui16IpLen] != '\0')
                {
                    if (buf[ui16IpLen] == ' ')
                    {
                        bLastCharSpace = true;
                    }
                    else
                    {
                        if (bLastCharSpace)
                        {
                            bLastCharSpace = false;
                            ui8NonSpaces++;
                        }
                    }

                    if (ui8NonSpaces == 3)
                    {
                        std::copy_n(buf.data() + ui16IpLen, 17, sMac);
                        sMac[17] = '\0';
                        fclose(fp);
                        return true;
                    }

                    ui16IpLen++;
                }
            }
        }

        fclose(fp);
    }
#endif // FLYLINKDC_USE_MAC
    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CreateGlobalBuffer()
{
    ServerManager::m_szGlobalBufferSize = PTOKAX_GLOBAL_BUFF_SIZE;
    ServerManager::m_pGlobalBuffer = new (std::nothrow) char[ServerManager::m_szGlobalBufferSize]();
    if (!ServerManager::m_pGlobalBuffer)
    {
        LogDbg("[MEM] Cannot create ServerManager::m_pGlobalBuffer");
        exit(EXIT_FAILURE);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DeleteGlobalBuffer()
{
    delete[] ServerManager::m_pGlobalBuffer;
    ServerManager::m_pGlobalBuffer = nullptr;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool CheckAndResizeGlobalBuffer(size_t szWantedSize)
{
    if (ServerManager::m_szGlobalBufferSize >= szWantedSize)
    {
        return true;
    }

    const size_t szOldSize = ServerManager::m_szGlobalBufferSize;
    // NOLINTNEXTLINE(misc-const-correctness)
    char* sOldBuf = ServerManager::m_pGlobalBuffer;

    ServerManager::m_szGlobalBufferSize = Allign(szWantedSize);

    char* sNewBuf = new (std::nothrow) char[ServerManager::m_szGlobalBufferSize]();
    if (!sNewBuf)
    {
        LogDbgErr("[MEM] Cannot reallocate {} bytes in CheckAndResizeGlobalBuffer for ServerManager::m_pGlobalBuffer", ServerManager::m_szGlobalBufferSize);

        ServerManager::m_szGlobalBufferSize = szOldSize;
        return false;
    }

    memcpy(sNewBuf, sOldBuf, szOldSize);
    delete[] sOldBuf;
    ServerManager::m_pGlobalBuffer = sNewBuf;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReduceGlobalBuffer()
{
    if (ServerManager::m_szGlobalBufferSize == PTOKAX_GLOBAL_BUFF_SIZE)
    {
        return;
    }

    const size_t szOldSize = ServerManager::m_szGlobalBufferSize;
    // NOLINTNEXTLINE(misc-const-correctness)
    char* sOldBuf = ServerManager::m_pGlobalBuffer;

    ServerManager::m_szGlobalBufferSize = PTOKAX_GLOBAL_BUFF_SIZE;

    char* sNewBuf = new (std::nothrow) char[ServerManager::m_szGlobalBufferSize]();
    if (!sNewBuf)
    {
        LogDbgErr("[MEM] Cannot reallocate {} bytes in ReduceGlobalBuffer for ServerManager::m_pGlobalBuffer", ServerManager::m_szGlobalBufferSize);

        ServerManager::m_szGlobalBufferSize = szOldSize;
        return;
    }

    memcpy(sNewBuf, sOldBuf, ServerManager::m_szGlobalBufferSize);
    delete[] sOldBuf;
    ServerManager::m_pGlobalBuffer = sNewBuf;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HashPassword(const char* sPassword, const size_t szPassLen, uint8_t* ui8PassHash)
{
#ifndef _WITHOUT_SKEIN
    Skein1024_Ctxt_t ctx1024;

    if (Skein1024_Init(&ctx1024, 512) == SKEIN_SUCCESS)
    {
        if (Skein1024_Update(&ctx1024, reinterpret_cast<const u08b_t*>(sPassword), szPassLen) == SKEIN_SUCCESS)
        {
            if (Skein1024_Final(&ctx1024, ui8PassHash) == SKEIN_SUCCESS)
            {
                return true;
            }
        }
    }
#endif
    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool WantAgain()
{
    printf("Do you want to try it again (Y/n) ? ");
    const int iChar = getchar();

    while (getchar() != '\n')
    {
        // boredom...
    };

    if (toupper(iChar) == 'Y')
    {
        return true;
    }

    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool IsPrivateIP(const char* sIP)
{
    if (ServerManager::m_bUseIPv6 && !strchr(sIP, '.'))
    {
        Hash128 ui128IpHash;
        if (inet_pton(AF_INET6, sIP, ui128IpHash) != 1)
        {
            return false;
        }

        in6_addr addr;
        memcpy(&addr, ui128IpHash.data(), sizeof(addr));
        if (IN6_IS_ADDR_LOOPBACK(&addr) || IN6_IS_ADDR_LINKLOCAL(&addr) || IN6_IS_ADDR_SITELOCAL(&addr))
        {
            return true;
        }
    }
    else
    {
        const uint32_t ui32IpHash = inet_addr(sIP);

        if (ui32IpHash == INADDR_NONE)
        {
            return false;
        }

        std::array<uint8_t, 4> ui32IP = {};
        memcpy(ui32IP.data(), &ui32IpHash, 4);

        if (ui32IP[0] == 10 || ui32IP[0] == 127 || (ui32IP[0] == 169 && ui32IP[1] == 254) || (ui32IP[0] == 172 && ui32IP[1] == 16) ||
            (ui32IP[0] == 192 && ui32IP[1] == 168))
        {
            return true;
        }
    }

    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool BuildUserOnlineInfo(int& iMsgLen, const User* pUser)
{
    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                        iMsgLen,
                        ServerManager::m_szGlobalBufferSize,
                        "\n%s: %s ",
                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_STATUS)].c_str(),
                        LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_ONLINE_FROM)].c_str()))
    {
        return false;
    }

    const struct tm* tm = localtime(&pUser->m_tLoginTime);
    const int iStrftimeRet = static_cast<int>(strftime(ServerManager::m_pGlobalBuffer + iMsgLen, ServerManager::m_szGlobalBufferSize - iMsgLen, "%c", tm));
    if (iStrftimeRet <= 0)
    {
        return false;
    }
    iMsgLen += iStrftimeRet;

    if (pUser->m_sIPv4[0] != '\0')
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s / %s\n%s: %0.02f %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                            pUser->m_sIP.data(),
                            pUser->m_sIPv4.data(),
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SHARE_SIZE)].c_str(),
                            static_cast<double>(pUser->m_ui64SharedSize) / 1073741824,
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_GIGA_BYTES)].c_str()))
        {
            return false;
        }
    }
    else
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: %s\n%s: %0.02f %s",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IP)].c_str(),
                            pUser->m_sIP.data(),
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_SHARE_SIZE)].c_str(),
                            static_cast<double>(pUser->m_ui64SharedSize) / 1073741824,
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_GIGA_BYTES)].c_str()))
        {
            return false;
        }
    }

    if (!pUser->m_sDescription.empty())
    {
        if (!AppendLabeledField(iMsgLen, std::to_underlying(LangIds::LAN_DESCRIPTION), pUser->m_sDescription.data(), pUser->m_sDescription.size()))
        {
            return false;
        }
    }

    if (!pUser->m_sTag.empty())
    {
        if (!AppendLabeledField(iMsgLen, std::to_underlying(LangIds::LAN_TAG), pUser->m_sTag.data(), pUser->m_sTag.size()))
        {
            return false;
        }
    }

    if (!pUser->m_sConnection.empty())
    {
        if (!AppendLabeledField(iMsgLen, std::to_underlying(LangIds::LAN_CONNECTION), pUser->m_sConnection.data(), pUser->m_sConnection.size()))
        {
            return false;
        }
    }

    if (!pUser->m_sEmail.empty())
    {
        if (!AppendLabeledField(iMsgLen, std::to_underlying(LangIds::LAN_EMAIL), pUser->m_sEmail.data(), pUser->m_sEmail.size()))
        {
            return false;
        }
    }

    if (IpP2Country::m_Ptr->m_ui32Count != 0)
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer,
                            iMsgLen,
                            ServerManager::m_szGlobalBufferSize,
                            "\n%s: ",
                            LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_COUNTRY)].c_str()))
        {
            return false;
        }
        if (static_cast<size_t>(iMsgLen) + 2 >= ServerManager::m_szGlobalBufferSize)
        {
            return false;
        }
        memcpy(ServerManager::m_pGlobalBuffer + iMsgLen, IpP2Country::m_Ptr->GetCountry(pUser->m_ui8Country, false), 2);
        iMsgLen += 2;
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint64_t NowMonotonicMs()
{
#ifdef __MACH__
    mach_timespec_t mts;
    clock_get_time(ServerManager::m_csMachClock, &mts);
    return (uint64_t(mts.tv_sec) * 1000) + (uint64_t(mts.tv_nsec) / 1000000);
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t(ts.tv_sec) * 1000) + (uint64_t(ts.tv_nsec) / 1000000);
#endif
}

bool WriteWholeFile(const std::string& sPath, std::string_view sData)
{
    std::ofstream f(sPath, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
    {
        return false;
    }
    if (!sData.empty())
    {
        f.write(sData.data(), static_cast<std::streamsize>(sData.size()));
        if (!f.good())
        {
            return false;
        }
    }
    return true;
}

bool ReadWholeFile(const std::string& sPath, std::string& sOut)
{
    std::ifstream f(sPath, std::ios::binary);
    if (!f.is_open())
    {
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    if (!f.good() && !f.eof())
    {
        return false;
    }
    sOut = ss.str();
    return true;
}
