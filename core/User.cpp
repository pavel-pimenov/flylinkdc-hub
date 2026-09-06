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
#include <atomic>
#include <fstream>
#include <random>
//---------------------------------------------------------------------------
#include "User.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
#include "UdpDebug.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
#include "DB-SQLite.h"
#endif
#include "DeFlood.h"
//---------------------------------------------------------------------------
static constexpr size_t g_ui32ZMinDataLen = 128;
static constexpr const char* g_sBadTag = "BAD TAG!";           // 8
static constexpr const char* g_sOtherNoTag = "OTHER (NO TAG)"; // 14
static constexpr const char* g_sUnknownTag = "UNKNOWN TAG";    // 11
static constexpr const char* g_sDefaultNick = "<unknown>";     // 9
//---------------------------------------------------------------------------

static constexpr uint32_t g_ui32MaxCmdLenPreLogin = 1024U;
static constexpr uint32_t g_ui32MaxCmdLenPostLogin = 65536U;
static constexpr size_t g_ui32MaxRecvChunk = 8U * 1024;
static constexpr uint32_t g_ui32SmallSendBufThreshold = 1024;
DcCommand g_ActualDcCommand;
//---------------------------------------------------------------------------

namespace {
bool UserProcessLines(User* pUser, const uint32_t ui32NewDataStart)
{
    char c = 0;

    char* pBuffer = pUser->m_pRecvBuf.get();
    static std::atomic<int> g_id{0};
    ++g_id;

    for (uint32_t ui32i = ui32NewDataStart; ui32i < pUser->m_ui32RecvBufDataLen; ++ui32i)
    {
        // look for pipes in the data - process lines one by one
        if (pUser->m_pRecvBuf[ui32i] == '|') [[unlikely]]
        {
            c = pUser->m_pRecvBuf[ui32i + 1];
            pUser->m_pRecvBuf[ui32i + 1] = '\0';

            const auto ui32CommandLen = static_cast<uint32_t>(((pUser->m_pRecvBuf.get() + ui32i) - pBuffer) + 1);
            if (ui32CommandLen <= (std::to_underlying(pUser->m_ui8State) < std::to_underlying(User::UserStates::STATE_ADDME) ? g_ui32MaxCmdLenPreLogin
                                                                                                                             : g_ui32MaxCmdLenPostLogin))
            {
                g_ActualDcCommand.m_pUser = pUser;
                g_ActualDcCommand.m_sCommand = pBuffer;
                g_ActualDcCommand.m_ui32CommandLen = ui32CommandLen;

                DcCommands::m_Ptr->PreProcessData(&g_ActualDcCommand);
            }
            else
            {
                pUser->SendFormat(
                    "UserProcessLines1", false, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CMD_TOO_LONG)].c_str());
                pUser->Close();

                UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s): Received command too long. User disconnected.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

                return false;
            }

            pUser->m_pRecvBuf[ui32i + 1] = c;
            pBuffer += ui32CommandLen;
            if (User::isPastLogin(pUser->m_ui8State))
            {
                return true;
            }
        }
        else if (pUser->m_pRecvBuf[ui32i] == '\0')
        {
            // look for nullptr character and replace with zero
            pUser->m_pRecvBuf[ui32i] = '0';
            continue;
        }
    }

    pUser->m_ui32RecvBufDataLen -= static_cast<uint32_t>(pBuffer - pUser->m_pRecvBuf.get());

    if (pUser->m_ui32RecvBufDataLen == 0)
    {
        DcCommands::m_Ptr->ProcessCmds(pUser);

        pUser->m_pRecvBuf[0] = '\0';

        return false;
    }
    if (pUser->m_ui32RecvBufDataLen == 1)
    {
        DcCommands::m_Ptr->ProcessCmds(pUser);

        pUser->m_pRecvBuf[0] = pBuffer[0];
        pUser->m_pRecvBuf[1] = '\0';

        return true;
    }

    if (pUser->m_ui32RecvBufDataLen >
        (std::to_underlying(pUser->m_ui8State) < std::to_underlying(User::UserStates::STATE_ADDME) ? g_ui32MaxCmdLenPreLogin : g_ui32MaxCmdLenPostLogin))
    {
        // PPK ... we don't want commands longer than 64 kB, drop this user !
        pUser->SendFormat("UserProcessLines2", false, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CMD_TOO_LONG)].c_str());
        pUser->Close();

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s): RecvBuffer overflow. User disconnected.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

        return false;
    }

    DcCommands::m_Ptr->ProcessCmds(pUser);

    memmove(pUser->m_pRecvBuf.get(), pBuffer, pUser->m_ui32RecvBufDataLen);
    pUser->m_pRecvBuf[pUser->m_ui32RecvBufDataLen] = '\0';

    return true;
}
} // namespace
//------------------------------------------------------------------------------

namespace {
void UserSetBadTag(User* pUser, const char* sDesc, const uint8_t ui8DescLen)
{
    // PPK ... clear all tag related things
    pUser->m_sTagVersion = {};

    pUser->m_sModes[0] = '\0';
    pUser->m_ui32Hubs = pUser->m_ui32Slots = pUser->m_ui32OLimit = pUser->m_ui32LLimit = pUser->m_ui32DLimit = pUser->m_ui32NormalHubs = pUser->m_ui32RegHubs =
        pUser->m_ui32OpHubs = 0;
    pUser->m_ui32BoolBits |= User::BIT_OLDHUBSTAG;
    pUser->m_ui32BoolBits |= User::BIT_HAVE_BADTAG;

    pUser->m_sDescription = std::string_view(sDesc, ui8DescLen);

    // PPK ... clear (fake) tag
    pUser->m_sTag = {};

    // PPK ... set bad tag
    pUser->m_sClient = g_sBadTag;

    // PPK ... send report to udp debug
    UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) have bad TAG (%s) ?!?", pUser->m_sNick.c_str(), pUser->m_sIP.data(), pUser->m_sMyInfoOriginal.data());
}
} // namespace
//---------------------------------------------------------------------------

namespace {
// Parse share bytes from a numeric string. Returns false on overflow (ERANGE) so the
// caller can treat a fake huge share as a protocol violation instead of corrupting
// ServerManager::m_ui64TotalShare via subtraction underflow.
[[nodiscard]] auto ParseShare(const char* sShare, uint64_t& ui64Share) -> bool
{
    errno = 0;
    char* sEnd = nullptr;
    const unsigned long long ullValue = strtoull(sShare, &sEnd, 10);
    if (errno == ERANGE || sEnd == sShare)
    {
        return false;
    }
    ui64Share = static_cast<uint64_t>(ullValue);
    return true;
}

void UserParseMyInfo(User* pUser)
{
    // PPK ... guard against OOB: m_ui16MyInfoOriginalLen - 1u would underflow to 0xFFFFFFFF
    // if the length is 0 (defense-in-depth on top of the DcCommands::MyINFO truncation guard).
    if (pUser->m_ui16MyInfoOriginalLen <= 14 + pUser->m_sNick.size())
    {
        LogWarn("[SECURITY] User {} ({}): MyINFO too short ({} bytes, nick len {}) - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data(),
            pUser->m_ui16MyInfoOriginalLen, pUser->m_sNick.size());

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s): truncated MyINFO (%u bytes) - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data(),
            pUser->m_ui16MyInfoOriginalLen);

        pUser->Close();
        return;
    }

    memcpy(ServerManager::m_pGlobalBuffer, pUser->m_sMyInfoOriginal.data(), pUser->m_ui16MyInfoOriginalLen);

    std::array<char*, 5> sMyINFOParts = {nullptr, nullptr, nullptr, nullptr, nullptr};
    std::array<uint16_t, 5> iMyINFOPartsLen = {0, 0, 0, 0, 0};

    unsigned char cPart = 0;

    sMyINFOParts[cPart] = ServerManager::m_pGlobalBuffer + 14 + pUser->m_sNick.size(); // desription start

    for (uint32_t ui32i = 14 + pUser->m_sNick.size(); ui32i < pUser->m_ui16MyInfoOriginalLen - 1u; ui32i++)
    {
        if (ServerManager::m_pGlobalBuffer[ui32i] == '$')
        {
            ServerManager::m_pGlobalBuffer[ui32i] = '\0';
            iMyINFOPartsLen[cPart] = static_cast<uint16_t>((ServerManager::m_pGlobalBuffer + ui32i) - sMyINFOParts[cPart]);

            // are we on end of myinfo ???
            if (cPart == 4)
            {
                break;
            }

            cPart++;
            sMyINFOParts[cPart] = ServerManager::m_pGlobalBuffer + ui32i + 1;
        }
    }

    // check if we have all myinfo parts, connection and sharesize must have length more than 0 !
    if (!sMyINFOParts[0] || !sMyINFOParts[1] || iMyINFOPartsLen[1] != 1 || !sMyINFOParts[2] || iMyINFOPartsLen[2] == 0 ||
        !sMyINFOParts[3] || !sMyINFOParts[4] || iMyINFOPartsLen[4] == 0)
    {
        pUser->SendFormat(
            "UserParseMyInfo1", false, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_YOU_MyINFO_IS_CORRUPTED)].c_str());

        UdpDebug::m_Ptr->BroadcastFormat(
            "[SYS] User %s (%s) with bad MyINFO (%s) disconnected.", pUser->m_sNick.c_str(), pUser->m_sIP.data(), pUser->m_sMyInfoOriginal.data());

        pUser->Close();
        return;
    }

    // connection
    pUser->m_ui8MagicByte = sMyINFOParts[2][iMyINFOPartsLen[2] - 1];
    pUser->m_sConnection =
        std::string_view(pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[2] - ServerManager::m_pGlobalBuffer), static_cast<size_t>(iMyINFOPartsLen[2] - 1));

    // email
    if (iMyINFOPartsLen[3] != 0)
    {
        pUser->m_sEmail =
            std::string_view(pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[3] - ServerManager::m_pGlobalBuffer), static_cast<size_t>(iMyINFOPartsLen[3]));
    }

    // share
    // PPK ... check for valid numeric share, kill fakers !
    if (!HaveOnlyNumbers(sMyINFOParts[4], iMyINFOPartsLen[4]))
    {
        LogWarn("[SECURITY] User {} ({}): non-numeric share value (len {}) - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data(), iMyINFOPartsLen[4]);

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) sent fake non-numeric share value - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

        pUser->Close();
        return;
    }

    uint64_t ui64NewShare = 0;
    if (!ParseShare(sMyINFOParts[4], ui64NewShare))
    {
        LogWarn("[SECURITY] User {} ({}): fake share value (overflow) '{}' - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data(), sMyINFOParts[4]);

        UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) sent fake share value (overflow) - user closed.", pUser->m_sNick.c_str(), pUser->m_sIP.data());

        pUser->Close();
        return;
    }

    if (((pUser->m_ui32BoolBits & User::BIT_HAVE_SHARECOUNTED) == User::BIT_HAVE_SHARECOUNTED))
    {
        ServerManager::m_ui64TotalShare -= pUser->m_ui64SharedSize;
        pUser->m_ui64SharedSize = ui64NewShare;
        ServerManager::m_ui64TotalShare += pUser->m_ui64SharedSize;
    }
    else
    {
        pUser->m_ui64SharedSize = ui64NewShare;
    }

    // Reset all tag infos...
    pUser->m_sModes[0] = '\0';
    pUser->m_ui32Hubs = 0;
    pUser->m_ui32NormalHubs = 0;
    pUser->m_ui32RegHubs = 0;
    pUser->m_ui32OpHubs = 0;
    pUser->m_ui32Slots = 0;
    pUser->m_ui32OLimit = 0;
    pUser->m_ui32LLimit = 0;
    pUser->m_ui32DLimit = 0;

    // description
    if (iMyINFOPartsLen[0] != 0)
    {
        if (sMyINFOParts[0][iMyINFOPartsLen[0] - 1] == '>')
        {
            char* DCTag = strrchr(sMyINFOParts[0], '<');
            if (!DCTag)
            {
                pUser->m_sDescription = std::string_view(pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                                         static_cast<size_t>(iMyINFOPartsLen[0]));

                pUser->m_sClient = g_sOtherNoTag;
                return;
            }

            pUser->m_sTag = std::string_view(pUser->m_sMyInfoOriginal.data() + (DCTag - ServerManager::m_pGlobalBuffer),
                                             static_cast<size_t>(iMyINFOPartsLen[0] - (DCTag - sMyINFOParts[0])));

            if (DCTag[3] == ' ' && MatchBytes(DCTag + 1, "++"))
            {
                pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
            }

            char* sTemp = strchr(DCTag, ' ');

            if (sTemp && MatchBytes(sTemp + 1, "V:"))
            {
                sTemp[0] = '\0';
                pUser->m_sClient = std::string_view(pUser->m_sMyInfoOriginal.data() + ((DCTag + 1) - ServerManager::m_pGlobalBuffer),
                                                    static_cast<size_t>((sTemp - DCTag) - 1));
            }
            else
            {
                pUser->m_sClient = g_sUnknownTag;
                pUser->m_sTag = {};
                sMyINFOParts[0][iMyINFOPartsLen[0] - 1] = '>'; //-V1048 intentional: restore > after sentinel check
                pUser->m_sDescription = std::string_view(pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                                         static_cast<size_t>(iMyINFOPartsLen[0]));
                return;
            }

            const size_t szTagPattLen = ((sTemp - DCTag) + 1);

            sMyINFOParts[0][iMyINFOPartsLen[0] - 1] = ','; // terminate tag end with ',' for easy tag parsing

            uint32_t reqVals = 0;
            char* sTagPart = DCTag + szTagPattLen;

            for (size_t szi = szTagPattLen; szi < static_cast<size_t>(iMyINFOPartsLen[0] - (DCTag - sMyINFOParts[0])); szi++)
            {
                if (DCTag[szi] == ',')
                {
                    DCTag[szi] = '\0';
                    if (sTagPart[1] != ':')
                    {
                        UserSetBadTag(pUser,
                                      pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                      static_cast<uint8_t>(iMyINFOPartsLen[0]));
                        return;
                    }

                    switch (sTagPart[0])
                    {
                    case 'V':
                        // PPK ... fix for potencial memory leak with fake tag
                        if (sTagPart[2] == '\0' || !pUser->m_sTagVersion.empty())
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        pUser->m_sTagVersion = std::string_view(pUser->m_sMyInfoOriginal.data() + ((sTagPart + 2) - ServerManager::m_pGlobalBuffer),
                                                                static_cast<size_t>((DCTag + szi) - (sTagPart + 2)));
                        reqVals++;
                        break;
                    case 'M':
                        if ((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 &&
                            (pUser->m_ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64)
                        {
                            if (sTagPart[2] == '\0' || sTagPart[3] == '\0' || sTagPart[4] != '\0')
                            {
                                UserSetBadTag(pUser,
                                              pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                              static_cast<uint8_t>(iMyINFOPartsLen[0]));
                                return;
                            }
                            pUser->m_sModes[0] = sTagPart[2];
                            pUser->m_sModes[1] = sTagPart[3];
                            pUser->m_sModes[2] = '\0';

                            if (toupper(sTagPart[3]) == 'A')
                            {
                                pUser->m_ui32BoolBits |= User::BIT_IPV6_ACTIVE;
                            }
                            else
                            {
                                pUser->m_ui32BoolBits &= ~User::BIT_IPV6_ACTIVE;
                            }
                        }
                        else
                        {
                            if (sTagPart[2] == '\0' || sTagPart[3] != '\0')
                            {
                                UserSetBadTag(pUser,
                                              pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                              static_cast<uint8_t>(iMyINFOPartsLen[0]));
                                return;
                            }
                            pUser->m_sModes[0] = sTagPart[2];
                            pUser->m_sModes[1] = '\0';
                        }

                        if (toupper(sTagPart[2]) == 'A')
                        {
                            pUser->m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
                        }
                        else
                        {
                            pUser->m_ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
                        }

                        reqVals++;
                        break;
                    case 'H':
                    {
                        if (sTagPart[2] == '\0')
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }

                        DCTag[szi] = '/';

                        std::array<char*, 3> sHubsParts = {nullptr, nullptr, nullptr};
                        std::array<uint16_t, 3> iHubsPartsLen = {0, 0, 0};

                        uint8_t ui8Part = 0;

                        sHubsParts[ui8Part] = sTagPart + 2;

                        for (uint32_t ui32j = 3; ui32j < static_cast<uint32_t>((DCTag + szi + 1) - sTagPart); ui32j++)
                        {
                            if (sTagPart[ui32j] == '/')
                            {
                                sTagPart[ui32j] = '\0';
                                iHubsPartsLen[ui8Part] = static_cast<uint16_t>((sTagPart + ui32j) - sHubsParts[ui8Part]);

                                // are we on end of hubs tag part ???
                                if (ui8Part == 2)
                                {
                                    break;
                                }

                                ui8Part++;
                                sHubsParts[ui8Part] = sTagPart + ui32j + 1;
                            }
                        }

                        if (sHubsParts[0] && sHubsParts[1] && sHubsParts[2])
                        {
                            if (iHubsPartsLen[0] != 0 && iHubsPartsLen[1] != 0 && iHubsPartsLen[2] != 0)
                            {
                                if (!HaveOnlyNumbers(sHubsParts[0], iHubsPartsLen[0]) || !HaveOnlyNumbers(sHubsParts[1], iHubsPartsLen[1]) ||
                                    !HaveOnlyNumbers(sHubsParts[2], iHubsPartsLen[2]))
                                {
                                    UserSetBadTag(pUser,
                                                  pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                                  static_cast<uint8_t>(iMyINFOPartsLen[0]));
                                    return;
                                }
                                int iNormalHubs = 0, iRegHubs = 0, iOpHubs = 0;
                                if (!safe_stoi(sHubsParts[0], iNormalHubs) || !safe_stoi(sHubsParts[1], iRegHubs) || !safe_stoi(sHubsParts[2], iOpHubs))
                                {
                                    UserSetBadTag(pUser,
                                                  pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                                  static_cast<uint8_t>(iMyINFOPartsLen[0]));
                                    return;
                                }
                                pUser->m_ui32NormalHubs = iNormalHubs;
                                pUser->m_ui32RegHubs = iRegHubs;
                                pUser->m_ui32OpHubs = iOpHubs;
                                pUser->m_ui32Hubs = pUser->m_ui32NormalHubs + pUser->m_ui32RegHubs + pUser->m_ui32OpHubs;
                                // PPK ... kill LAM3R with fake hubs
                                if (pUser->m_ui32Hubs != 0)
                                {
                                    pUser->m_ui32BoolBits &= ~User::BIT_OLDHUBSTAG;
                                    reqVals++;
                                    break;
                                }
                            }
                        }
                        else if (sHubsParts[1] == DCTag + szi + 1 && !sHubsParts[2])
                        {
                            DCTag[szi] = '\0';
                            int iHubs = 0;
                            if (!safe_stoi(sHubsParts[0], iHubs))
                            {
                                UserSetBadTag(pUser,
                                              pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                              static_cast<uint8_t>(iMyINFOPartsLen[0]));
                                return;
                            }
                            pUser->m_ui32Hubs = iHubs;
                            reqVals++;
                            pUser->m_ui32BoolBits |= User::BIT_OLDHUBSTAG;
                            break;
                        }

                        pUser->SendFormat(
                            "UserParseMyInfo2", false, "<%s> %s!|", SettingManager::HubSec(), LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FAKE_TAG)].c_str());

                        const std::string sTag(pUser->m_sTag);
                        UdpDebug::m_Ptr->BroadcastFormat(
                            "[SYS] User %s (%s) with fake Tag disconnected: %s", pUser->m_sNick.c_str(), pUser->m_sIP.data(), sTag.c_str());

                        pUser->m_ui32NormalHubs = 1; // [+]FlylinkDC++
                        if (pUser->m_ui32Hubs == 0)
                        {
                            pUser->m_ui32Hubs = 1;
                        }
                        break;
                    }
                    case 'S':
                    {
                        if (sTagPart[2] == '\0')
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        const auto l_slot_len = uint16_t(strlen(sTagPart + 2));
                        if (l_slot_len > 12)
                        {
                            pUser->m_is_bad_len_number_myinfo = true;
                        }
                        if (!HaveOnlyNumbers(sTagPart + 2, l_slot_len))
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        int iSlots = 0;
                        if (!safe_stoi(sTagPart + 2, iSlots))
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        pUser->m_ui32Slots = iSlots;
                        reqVals++;
                        break;
                    }
                    case 'O':
                    {
                        if (sTagPart[2] == '\0')
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        if (strlen(sTagPart) > 12)
                        {
                            pUser->m_is_bad_len_number_myinfo = true;
                        }
                        int iOLimit = 0;
                        if (!safe_stoi(sTagPart + 2, iOLimit))
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        pUser->m_ui32OLimit = iOLimit;
                        break;
                    }
                    case 'B':
                    case 'L':
                    {
                        if (sTagPart[2] == '\0')
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        if (strlen(sTagPart) > 12)
                        {
                            pUser->m_is_bad_len_number_myinfo = true;
                        }
                        int iLLimit = 0;
                        if (!safe_stoi(sTagPart + 2, iLLimit))
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        pUser->m_ui32LLimit = iLLimit;
                        break;
                    }
                    case 'D':
                    {
                        if (sTagPart[2] == '\0')
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        if (strlen(sTagPart) > 12)
                        {
                            pUser->m_is_bad_len_number_myinfo = true;
                        }
                        int iDLimit = 0;
                        if (!safe_stoi(sTagPart + 2, iDLimit))
                        {
                            UserSetBadTag(pUser,
                                          pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                          static_cast<uint8_t>(iMyINFOPartsLen[0]));
                            return;
                        }
                        pUser->m_ui32DLimit = iDLimit;
                        break;
                    }
                    default:
                        break;
                    }
                    sTagPart = DCTag + szi + 1;
                }
            }

            if (reqVals < 4)
            {
                UserSetBadTag(
                    pUser, pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), static_cast<uint8_t>(iMyINFOPartsLen[0]));
                return;
            }

            pUser->m_sDescription = std::string_view(pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer),
                                                     static_cast<size_t>(DCTag - sMyINFOParts[0]));
            return;
        }
        pUser->m_sDescription =
            std::string_view(pUser->m_sMyInfoOriginal.data() + (sMyINFOParts[0] - ServerManager::m_pGlobalBuffer), static_cast<size_t>(iMyINFOPartsLen[0]));
    }

    pUser->m_sClient = g_sOtherNoTag;

    pUser->m_sTag = {};

    pUser->m_sTagVersion = {};

    pUser->m_sModes[0] = '\0'; //-V1048 intentional re-init for clarity
    pUser->m_ui32Hubs = 0;
    pUser->m_ui32NormalHubs = 0;
    pUser->m_ui32RegHubs = 0;
    pUser->m_ui32OpHubs = 0;
    pUser->m_ui32Slots = 0;
    pUser->m_ui32OLimit = 0;
    pUser->m_ui32LLimit = 0;
    pUser->m_ui32DLimit = 0;
}
} // namespace
//---------------------------------------------------------------------------

// UserBan constructor is = default in header
//---------------------------------------------------------------------------

std::unique_ptr<UserBan> UserBan::CreateUserBan(const char* sMess, const uint32_t ui32MessLen, const uint32_t ui32Hash)
{
    auto pUserBan = std::make_unique<UserBan>();

    pUserBan->m_sMessage.assign(sMess, ui32MessLen);

    pUserBan->m_ui32NickHash = ui32Hash;

    return pUserBan;
}
//---------------------------------------------------------------------------

// LoginLogout constructor is = default in header
//---------------------------------------------------------------------------
void LoginLogout::Clean()
{
    m_pBan.reset();
    m_Buffer.clear();
}
LoginLogout::~LoginLogout()
{
    Clean();
}
//---------------------------------------------------------------------------

User::User() : m_sNick(g_sDefaultNick), m_sClient(g_sOtherNoTag), m_ui8Country(246)

{
    m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
    m_ui32BoolBits |= User::BIT_OLDHUBSTAG;

    time(&m_tLoginTime);

    m_ui128IpHash = {};

    m_sIP[0] = '\0';
    m_sIPv4[0] = '\0';
    m_sModes[0] = '\0';
#ifdef USE_FLYLINKDC_EXT_JSON
    m_is_invalid_json = false;
    m_is_json_user = false;
#endif
    m_is_bad_len_port = false;
    m_is_bad_len_number_myinfo = false;
    m_is_bad_port = false;
    m_is_ddos_udp = false;
    m_is_proxy_user = false;
    m_is_max_int8 = false;
    m_is_max_ip_len = false;
    GlobalDataQueue::m_Ptr->PrometheusUsersGauge(1);
}
//---------------------------------------------------------------------------

User::~User()
{
    GlobalDataQueue::m_Ptr->PrometheusUsersGauge(-1);

    if (((m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE))
    {
        DcCommands::m_Ptr->m_ui32StatZPipe--;
    }

    ServerManager::m_ui32Parts++;
    ServerManager::m_ui32ConnectionsClosed++;

    User::DeletePrcsdUsrCmd(m_pCmdActive4Search);
    User::DeletePrcsdUsrCmd(m_pCmdActive6Search);
    User::DeletePrcsdUsrCmd(m_pCmdPassiveSearch);

    m_CmdList.clear();

    m_CmdToUserList.clear();
#ifdef USE_FLYLINKDC_EXT_JSON
    m_user_ext_info.reset();
    m_is_json_user = false;
#endif
}
//---------------------------------------------------------------------------

bool User::MakeLock()
{
    // This code computes the valid Lock string including the Pk= string
    // For maximum speed we just find two random numbers - start and step
    // Step is added each cycle to the start and the ascii 122 boundary is
    // checked. If overflow occurs then the overflowed value is added to the
    // ascii 48 value ("0") and continues.
    // The lock has fixed length 63 bytes

    static const char sLock[] = "$Lock EXTENDEDPROTOCOL                           nix Pk=PtokaX|"; // NOLINT(modernize-avoid-c-arrays)
    static constexpr size_t szLockLen = sizeof(sLock) - 1;

    const size_t szAllignLen = Allign(m_ui32SendBufDataLen + szLockLen);

    char* const pOldBuf = m_pSendBuf.get();
    auto pNewBuf = std::make_unique<char[]>(szAllignLen); // NOLINT(modernize-avoid-c-arrays) dynamic buffer
    if (!pNewBuf)
    {
        m_ui32BoolBits |= BIT_ERROR;

        LogDbgErr("[MEM] Cannot allocate {} bytes in User::MakeLock", szAllignLen);

        return false;
    }

    if (pOldBuf)
    {
        memcpy(pNewBuf.get(), pOldBuf, m_ui32SendBufDataLen);
    }
    m_pSendBuf = std::move(pNewBuf);

    m_ui32SendBufLen = static_cast<uint32_t>(szAllignLen - 1);
    m_pSendBufHead = m_pSendBuf.get();

    // append data to the buffer
    memcpy(m_pSendBuf.get(), sLock, szLockLen);
    m_ui32SendBufDataLen += szLockLen;
    m_pSendBuf[m_ui32SendBufDataLen] = '\0';

    for (uint8_t ui8i = 22; ui8i < 49; ui8i++)
    {
        static std::mt19937 rng(std::random_device{}()); // NOLINT(bugprone-narrowing-conversions) result_type is unsigned int, not narrowing
        std::uniform_int_distribution<int> dist(0, 73);
        m_pSendBuf[ui8i] = static_cast<char>(dist(rng) + 48);
    }

    m_LogInOut.m_Buffer.resize(64);
    memcpy(m_LogInOut.m_Buffer.data(), m_pSendBuf.get(), szLockLen);
    m_LogInOut.m_Buffer[szLockLen] = '\0';

    return true;
}
//---------------------------------------------------------------------------

bool User::DoRecv()
{
    if ((m_ui32BoolBits & BIT_ERROR) == BIT_ERROR || User::isPastLogin(m_ui8State)) [[unlikely]]
    {
        return false;
    }

    int iAvailBytes = 0;
    if (ioctl(m_Socket, FIONREAD, &iAvailBytes) == -1) [[unlikely]]
    {
        UdpDebug::m_Ptr->BroadcastFormat(
            "[ERR] %s (%s): ioctlsocket(FIONREAD) error %s (%d). User is being closed.", m_sNick.c_str(), m_sIP.data(), ErrnoStr(errno), errno);
        m_ui32BoolBits |= BIT_ERROR;
        Close();
        return false;
    }

    // PPK ... check flood ...
    if (iAvailBytes != 0 && !ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NODEFLOODRECV))
    {
        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION)] != 0)
        {
            if (m_ui32Recvs == 0)
            {
                m_ui64RecvsTick = ServerManager::m_ui64ActualTick;
            }

            m_ui32Recvs += iAvailBytes;

            if (DeFloodCheckForDataFlood(this,
                                         DefloodTypes::MAX_DOWN,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION)],
                                         m_ui32Recvs,
                                         m_ui64RecvsTick,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_KB)],
                                         static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_TIME)])))
            {
                return false;
            }

            if (m_ui32Recvs != 0)
            {
                m_ui32Recvs -= iAvailBytes;
            }
        }

        if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION2)] != 0)
        {
            if (m_ui32Recvs2 == 0)
            {
                m_ui64RecvsTick2 = ServerManager::m_ui64ActualTick;
            }

            m_ui32Recvs2 += iAvailBytes;

            if (DeFloodCheckForDataFlood(this,
                                         DefloodTypes::MAX_DOWN,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_ACTION2)],
                                         m_ui32Recvs2,
                                         m_ui64RecvsTick2,
                                         SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_KB2)],
                                         static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_DOWN_TIME2)])))
            {
                return false;
            }

            if (m_ui32Recvs2 != 0)
            {
                m_ui32Recvs2 -= iAvailBytes;
            }
        }
    }

    if (iAvailBytes == 0) [[unlikely]]
    {
        const int64_t l_delta = static_cast<int64_t>(ServerManager::m_ui64ActualTick) - static_cast<int64_t>(m_last_recv_tick);
        if (l_delta > 5 || l_delta < 0)
        {
            m_last_recv_tick = ServerManager::m_ui64ActualTick;
            // we need to try recv to catch connection error or closed connection
            iAvailBytes = 16;
        }
        else
        {
            return true;
        }
    }
    else if (iAvailBytes > static_cast<int>(g_ui32MaxRecvChunk))
    {
        // receive max. 8k bytes to receive buffer
        iAvailBytes = g_ui32MaxRecvChunk;
    }

    size_t szAllignLen = 0;

    if (m_ui32RecvBufLen < m_ui32RecvBufDataLen + iAvailBytes) [[unlikely]]
    {
        // Hard cap: 1MB recv buffer to prevent memory exhaustion DoS
        constexpr uint32_t MAX_RECV_BUF_SIZE = 1u << 20;
        if (m_ui32RecvBufDataLen + iAvailBytes > MAX_RECV_BUF_SIZE)
        {
            LogInfo("[SECURITY] Recv buffer exceeded {} bytes for {} ({}) - closing", MAX_RECV_BUF_SIZE, m_sNick, m_sIP.data());
            m_ui32BoolBits |= BIT_ERROR;
            Close();
            return false;
        }

        szAllignLen = Allign(m_ui32RecvBufDataLen + iAvailBytes);
    }
    else if (m_ui32RecvCalled > 60)
    {
        szAllignLen = Allign(m_ui32RecvBufDataLen + iAvailBytes);
        if (m_ui32RecvBufLen <= szAllignLen)
        {
            szAllignLen = 0;
        }

        m_ui32RecvCalled = 0;
    }

    if (szAllignLen != 0)
    {
        char* const pOldBuf = m_pRecvBuf.get();

        auto pNewBuf = std::make_unique<char[]>(szAllignLen); // NOLINT(modernize-avoid-c-arrays) dynamic buffer

        if (pOldBuf)
        {
            memcpy(pNewBuf.get(), pOldBuf, m_ui32RecvBufDataLen);
        }
        m_pRecvBuf = std::move(pNewBuf);

        m_ui32RecvBufLen = static_cast<uint32_t>(szAllignLen - 1);
    }

    // receive new data to pRecvBuf
    const ssize_t recvlen = recv(m_Socket, m_pRecvBuf.get() + m_ui32RecvBufDataLen, m_ui32RecvBufLen - m_ui32RecvBufDataLen, 0);
    GlobalDataQueue::m_Ptr->PrometheusRecvBytes("user", static_cast<int>(recvlen));
    m_ui32RecvCalled++;

    if (recvlen == -1) [[unlikely]]
    {
        if (errno != EAGAIN)
        {
            UdpDebug::m_Ptr->BroadcastFormat(
                "[ERR] %s (%s): recv() error %s (%d). User is being closed.", m_sNick.c_str(), m_sIP.data(), ErrnoStr(errno), errno);
            m_ui32BoolBits |= BIT_ERROR;
            Close();
            return false;
        }

        return false;
    }
    if (recvlen == 0) [[unlikely]] // regular close
    {
        m_ui32BoolBits |= BIT_ERROR;
        Close();
        return false;
    }

    m_ui32Recvs += recvlen;
    m_ui32Recvs2 += recvlen;
    ServerManager::m_ui64BytesRead += recvlen;
    m_ui32RecvBufDataLen += recvlen;
    m_pRecvBuf[m_ui32RecvBufDataLen] = '\0';
    if (UserProcessLines(this, m_ui32RecvBufDataLen - recvlen))
    {
        return true;
    }

    return false;
}
//---------------------------------------------------------------------------

void User::SendChar(const char* sText, const size_t szTextLen)
{
    if (User::isPastLogin(m_ui8State) || szTextLen == 0) [[unlikely]]
    {
        return;
    }

    if (!((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) || szTextLen < g_ui32ZMinDataLen)
    {
        if (PutInSendBuf(sText, szTextLen))
        {
            (void)Try2Send();
        }
    }
    else
    {
        uint32_t iLen = 0;
        char* sData = ZlibUtility::m_Ptr->CreateZPipe(std::string_view(sText, szTextLen), iLen);

        if (iLen == 0)
        {
            if (PutInSendBuf(sText, szTextLen))
            {
                (void)Try2Send();
            }
        }
        else
        {
            ServerManager::m_ui64BytesSentSaved += szTextLen - iLen;
            if (PutInSendBuf(sData, iLen))
            {
                (void)Try2Send();
            }
        }
    }
}
//---------------------------------------------------------------------------

#ifdef USE_FLYLINKDC_EXT_JSON
void User::SendCharDelayedExtJSON()
{
    if (isSupportExtJSON())
    {
        if (!Users::m_Ptr->m_AllExtJSON.empty())
        {
            LogDbg("[EXTJSON] SendCharDelayedExtJSON user='{}' len={}", m_sNick, Users::m_Ptr->m_AllExtJSON.size());
            SendCharDelayed(Users::m_Ptr->m_AllExtJSON);
        }
    }
}
#endif // USE_FLYLINKDC_EXT_JSON
//---------------------------------------------------------------------------

void User::SendCharDelayed(const char* sText, const size_t szTextLen)
{
    if (User::isPastLogin(m_ui8State) || szTextLen == 0) [[unlikely]]
    {
        return;
    }

    if (!((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) || szTextLen < g_ui32ZMinDataLen)
    {
        (void)PutInSendBuf(sText, szTextLen);
    }
    else
    {
        uint32_t iLen = 0;
        char* sPipeData = ZlibUtility::m_Ptr->CreateZPipe(std::string_view(sText, szTextLen), iLen);

        if (iLen == 0)
        {
            (void)PutInSendBuf(sText, szTextLen);
        }
        else
        {
            (void)PutInSendBuf(sPipeData, iLen);
            ServerManager::m_ui64BytesSentSaved += szTextLen - iLen;
        }
    }
}
//---------------------------------------------------------------------------

void User::SendTextDelayed(const std::string& sText)
{
    SendCharDelayed(sText);
}

void User::SendFormat(const char* sFrom, const bool bDelayed, const char* sFormatMsg, ...)
{
    if (User::isPastLogin(m_ui8State)) [[unlikely]]
    {
        return;
    }

    va_list vlArgs;
    va_start(vlArgs, sFormatMsg);

    const int iRet = vsnprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, sFormatMsg, vlArgs);

    va_end(vlArgs);

    // iRet >= buffer size means truncation: PutInSendBuf would read past the buffer end.
    if (iRet <= 0 || static_cast<size_t>(iRet) >= ServerManager::m_szGlobalBufferSize)
    {
        LogDbgErr("[ERR] vsnprintf wrong value {} in User::SendFormatDelayed from: {}", iRet, sFrom);

        return;
    }

    if (!((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) || static_cast<size_t>(iRet) < g_ui32ZMinDataLen)
    {
        if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iRet) && !bDelayed)
        {
            (void)Try2Send();
        }
    }
    else
    {
        uint32_t iLen = 0;
        char* sData = ZlibUtility::m_Ptr->CreateZPipe(std::string_view(ServerManager::m_pGlobalBuffer, iRet), iLen);

        if (iLen == 0)
        {
            if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iRet) && !bDelayed)
            {
                (void)Try2Send();
            }
        }
        else
        {
            if (PutInSendBuf(sData, iLen) && !bDelayed)
            {
                (void)Try2Send();
            }
            ServerManager::m_ui64BytesSentSaved += iRet - iLen;
        }
    }
}
//---------------------------------------------------------------------------

void User::SendFormatCheckPM(const char* sFrom, const char* sOtherNick, const bool bDelayed, const char* sFormatMsg, ...)
{
    if (User::isPastLogin(m_ui8State)) [[unlikely]]
    {
        return;
    }

    int iMsgLen = 0;

    if (sOtherNick)
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iMsgLen, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $", m_sNick.c_str(), sOtherNick))
        {
            LogDbgErr("[ERR] snprintf wrong value {} in User::SendFormatCheckPM from: {}", iMsgLen, sFrom);

            return;
        }
    }

    va_list vlArgs;
    va_start(vlArgs, sFormatMsg);

    const int iRet = vsnprintf(ServerManager::m_pGlobalBuffer + iMsgLen, ServerManager::m_szGlobalBufferSize - iMsgLen, sFormatMsg, vlArgs);

    va_end(vlArgs);

    // iRet >= remaining space means truncation: iMsgLen would overshoot the buffer end.
    if (iRet <= 0 || static_cast<size_t>(iRet) >= ServerManager::m_szGlobalBufferSize - iMsgLen)
    {
        LogDbgErr("[ERR] vsnprintf wrong value {} in User::SendFormatCheckPM from: {}", iRet, sFrom);

        return;
    }

    iMsgLen += iRet;

    if (!((m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE) || static_cast<size_t>(iMsgLen) < g_ui32ZMinDataLen)
    {
        if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iMsgLen) && !bDelayed)
        {
            (void)Try2Send();
        }
    }
    else
    {
        uint32_t iLen = 0;
        char* sData = ZlibUtility::m_Ptr->CreateZPipe(std::string_view(ServerManager::m_pGlobalBuffer, iMsgLen), iLen);

        if (iLen == 0)
        {
            if (PutInSendBuf(ServerManager::m_pGlobalBuffer, iMsgLen) && !bDelayed)
            {
                (void)Try2Send();
            }
        }
        else
        {
            if (PutInSendBuf(sData, iLen) && !bDelayed)
            {
                (void)Try2Send();
            }
            ServerManager::m_ui64BytesSentSaved += iMsgLen - iLen;
        }
    }
}
//---------------------------------------------------------------------------

bool User::PutInSendBuf(const char* sText, const size_t szTxtLen)
{
    m_ui32SendCalled++;

    size_t szAllignLen = 0;

    if (m_ui32SendBufLen < m_ui32SendBufDataLen + szTxtLen) [[unlikely]]
    {
        if (!m_pSendBuf)
        {
            szAllignLen = Allign(m_ui32SendBufDataLen + szTxtLen);
        }
        else
        {
            if (static_cast<size_t>(m_pSendBufHead - m_pSendBuf.get()) > szTxtLen)
            {
                const auto offset = static_cast<uint32_t>(m_pSendBufHead - m_pSendBuf.get());
                memmove(m_pSendBuf.get(), m_pSendBufHead, (m_ui32SendBufDataLen - offset));
                m_pSendBufHead = m_pSendBuf.get();
                m_ui32SendBufDataLen = m_ui32SendBufDataLen - offset;
            }
            else
            {
                szAllignLen = Allign(m_ui32SendBufDataLen + szTxtLen);
                auto szMaxBufLen =
                    static_cast<size_t>(((m_ui32BoolBits & BIT_BIG_SEND_BUFFER) == BIT_BIG_SEND_BUFFER)
                                            ? ((Users::m_Ptr->m_ui32MyInfosTagLen > Users::m_Ptr->m_ui32MyInfosLen ? Users::m_Ptr->m_ui32MyInfosTagLen
                                                                                                                   : Users::m_Ptr->m_ui32MyInfosLen) *
                                               2)
                                            : (Users::m_Ptr->m_ui32MyInfosTagLen > Users::m_Ptr->m_ui32MyInfosLen ? Users::m_Ptr->m_ui32MyInfosTagLen
                                                                                                                  : Users::m_Ptr->m_ui32MyInfosLen));
                szMaxBufLen = szMaxBufLen < 262144 ? 262144 : szMaxBufLen;
                if (szAllignLen > szMaxBufLen)
                {
                    // does the buffer size reached the maximum
                    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_KEEP_SLOW_USERS)] || (m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE)
                    {
                        // we want to drop the slow user
                        m_ui32BoolBits |= BIT_ERROR;

                        LogWarn("[SECURITY] User {} ({}): SendBuffer overflow (AL:{}[SL:{}|NL:{}|FL:{}]/ML:{}) - user closed.",
                            m_sNick.c_str(),
                            m_sIP.data(),
                            szAllignLen,
                            m_ui32SendBufDataLen,
                            szTxtLen,
                            m_pSendBufHead - m_pSendBuf.get(),
                            szMaxBufLen);

                        UdpDebug::m_Ptr->BroadcastFormat("[SYS] %s (%s) SendBuffer overflow (AL:%zu[SL:%u|NL:%zu|FL:%zu]/ML:%zu). User disconnected.",
                                                         m_sNick.c_str(),
                                                         m_sIP.data(),
                                                         szAllignLen,
                                                         m_ui32SendBufDataLen,
                                                         szTxtLen,
                                                         m_pSendBufHead - m_pSendBuf.get(),
                                                         szMaxBufLen);

                        Close();
                        return false;
                    }

                    UdpDebug::m_Ptr->BroadcastFormat(
                        "[SYS] %s (%s) SendBuffer overflow (AL:%zu[SL:%u|NL:%zu|FL:%zu]/ML:%zu). Buffer cleared - user stays online.",
                        m_sNick.c_str(),
                        m_sIP.data(),
                        szAllignLen,
                        m_ui32SendBufDataLen,
                        szTxtLen,
                        m_pSendBufHead - m_pSendBuf.get(),
                        szMaxBufLen);

                    // we want to keep the slow user online
                    // PPK ... i don't want to corrupt last command, get rest of it and add to new buffer ;)
                    char* sTemp = static_cast<char*>(memchr(m_pSendBufHead, '|', m_ui32SendBufDataLen - (m_pSendBufHead - m_pSendBuf.get())));
                    if (sTemp)
                    {
                        const uint32_t iOldSBDataLen = m_ui32SendBufDataLen;

                        auto iRestCommandLen = static_cast<uint32_t>((sTemp - m_pSendBufHead) + 1);
                        if (m_pSendBuf.get() != m_pSendBufHead)
                        {
                            memmove(m_pSendBuf.get(), m_pSendBufHead, iRestCommandLen);
                        }
                        m_ui32SendBufDataLen = iRestCommandLen;

                        // If is not needed then don't lost all data, try to find some space with removing only few oldest commands
                        if (szTxtLen < szMaxBufLen && iOldSBDataLen > static_cast<uint32_t>((sTemp + 1) - m_pSendBuf.get()) &&
                            (iOldSBDataLen - ((sTemp + 1) - m_pSendBuf.get())) >
                                static_cast<uint32_t>(szTxtLen)) //-V1051 iOldSBDataLen is correct: saved before update
                        {
                            char* sTemp1;
                            // try to remove min half of send bufer
                            if (iOldSBDataLen > (m_ui32SendBufLen / 2) &&
                                static_cast<uint32_t>((sTemp + 1 + szTxtLen) - m_pSendBuf.get()) < (m_ui32SendBufLen / 2))
                            {
                                sTemp1 = static_cast<char*>(memchr(m_pSendBuf.get() + (m_ui32SendBufLen / 2), '|', iOldSBDataLen - (m_ui32SendBufLen / 2)));
                            }
                            else
                            {
                                sTemp1 = static_cast<char*>(memchr(sTemp + 1 + szTxtLen, '|', iOldSBDataLen - ((sTemp + 1 + szTxtLen) - m_pSendBuf.get())));
                            }

                            if (sTemp1)
                            {
                                iRestCommandLen = static_cast<uint32_t>(iOldSBDataLen - ((sTemp1 + 1) - m_pSendBuf.get()));
                                memmove(m_pSendBuf.get() + m_ui32SendBufDataLen, sTemp1 + 1, iRestCommandLen);
                                m_ui32SendBufDataLen += iRestCommandLen;
                            }
                        }
                    }
                    else
                    {
                        m_pSendBuf[0] = '|';
                        m_pSendBuf[1] = '\0';
                        m_ui32SendBufDataLen = 1;
                    }

                    const size_t szAllignTxtLen = Allign(szTxtLen + m_ui32SendBufDataLen);

                    char* const pOldBuf = m_pSendBuf.get();
                    auto pNewBuf = std::make_unique<char[]>(szAllignTxtLen); // NOLINT(modernize-avoid-c-arrays) dynamic buffer
                    if (!pNewBuf)
                    {
                        m_ui32BoolBits |= BIT_ERROR;
                        Close();

                        LogDbgErr("[MEM] Cannot reallocate {} bytes in User::PutInSendBuf-keepslow", szAllignLen);

                        return false;
                    }

                    memcpy(pNewBuf.get(), pOldBuf, m_ui32SendBufDataLen);
                    m_pSendBuf = std::move(pNewBuf);

                    m_ui32SendBufLen = static_cast<uint32_t>(szAllignTxtLen - 1);
                    m_pSendBufHead = m_pSendBuf.get();

                    szAllignLen = 0;
                }
                else
                {
                    szAllignLen = Allign(m_ui32SendBufDataLen + szTxtLen);
                }
            }
        }
    }
    else if (m_ui32SendCalled > 100)
    {
        szAllignLen = Allign(m_ui32SendBufDataLen + szTxtLen);
        if (m_ui32SendBufLen <= szAllignLen)
        {
            szAllignLen = 0;
        }

        m_ui32SendCalled = 0;
    }

    if (szAllignLen != 0)
    {
        const uint32_t offset = (!m_pSendBuf ? 0 : static_cast<uint32_t>(m_pSendBufHead - m_pSendBuf.get()));

        char* const pOldBuf = m_pSendBuf.get();
        auto pNewBuf = std::make_unique<char[]>(szAllignLen); // NOLINT(modernize-avoid-c-arrays) dynamic buffer
        if (!pNewBuf)
        {
            m_ui32BoolBits |= BIT_ERROR;
            Close();

            LogDbgErr("[MEM] Cannot (re)allocate {} bytes for new pSendBuf in User::PutInSendBuf", szAllignLen);

            return false;
        }

        if (pOldBuf)
        {
            memcpy(pNewBuf.get(), pOldBuf, m_ui32SendBufDataLen);
        }
        m_pSendBuf = std::move(pNewBuf);

        m_ui32SendBufLen = static_cast<uint32_t>(szAllignLen - 1);
        m_pSendBufHead = m_pSendBuf.get() + offset;
    }

    // append data to the buffer
    memcpy(m_pSendBuf.get() + m_ui32SendBufDataLen, sText, szTxtLen);
    m_ui32SendBufDataLen += static_cast<uint32_t>(szTxtLen);
    m_pSendBuf[m_ui32SendBufDataLen] = '\0';

    return true;
}
//---------------------------------------------------------------------------

bool User::Try2Send()
{
    if ((m_ui32BoolBits & BIT_ERROR) == BIT_ERROR || m_ui32SendBufDataLen == 0)
    {
        return false;
    }

    // compute length of unsent data
    const auto offset = static_cast<int32_t>(m_pSendBufHead - m_pSendBuf.get());
    const int32_t len = static_cast<int32_t>(m_ui32SendBufDataLen) - offset;

    if (offset < 0 || len < 0)
    {
        LogDbgErr("[ERR] Negative send values!\nSendBuf: {}\nPlayHead: {}\nDataLen: {}", m_pSendBuf.get(), m_pSendBufHead, m_ui32SendBufDataLen);

        m_ui32BoolBits |= BIT_ERROR;
        Close();

        return false;
    }

    const ssize_t n = send(m_Socket, m_pSendBufHead, len < 32768 ? len : 32768, 0);

    if (n == -1) [[unlikely]]
    {
        if (errno != EAGAIN)
        {
            UdpDebug::m_Ptr->BroadcastFormat(
                "[ERR] %s (%s): send() error %s (%d). User is being closed.", m_sNick.c_str(), m_sIP.data(), ErrnoStr(errno), errno);
            m_ui32BoolBits |= BIT_ERROR;
            Close();
            return false;
        }

        return true;
    }

    ServerManager::m_ui64BytesSent += n;
    GlobalDataQueue::m_Ptr->PrometheusSendBytes(__func__, static_cast<int>(n));

    // if buffer is sent then mark it as empty (first byte = 0)
    // else move remaining data on new place and free old buffer
    if (n < len)
    {
        m_pSendBufHead += n;
        return true;
    }

    // PPK ... we need to free memory allocated for big buffer on login (userlist, motd...)
    if (((m_ui32BoolBits & BIT_BIG_SEND_BUFFER) == BIT_BIG_SEND_BUFFER) || m_ui32SendBufLen > g_ui32SmallSendBufThreshold)
    {
        if (m_pSendBuf)
        {
            m_pSendBuf.reset();
            m_pSendBufHead = nullptr;
            m_ui32SendBufLen = 0;
            m_ui32SendBufDataLen = 0;
        }
        m_ui32BoolBits &= ~BIT_BIG_SEND_BUFFER;
    }
    else
    {
        m_pSendBuf[0] = '\0';
        m_pSendBufHead = m_pSendBuf.get();
        m_ui32SendBufDataLen = 0;
    }
    return false;
}
//---------------------------------------------------------------------------

void User::SetIP(const char* sIP)
{
    const int iLen = snprintf(m_sIP.data(), m_sIP.size(), "%s", sIP);
    if (iLen < 0 || static_cast<size_t>(iLen) >= m_sIP.size())
    {
        m_ui8IpLen = static_cast<uint8_t>(m_sIP.size() - 1);
        LogDbgWarn("[WARN] User::SetIP truncated IP for nick {} (len={})", m_sNick.c_str(), iLen);
    }
    else
    {
        m_ui8IpLen = static_cast<uint8_t>(iLen);
    }
}
//------------------------------------------------------------------------------

void User::SetNick(std::string_view sNick)
{
    m_sNick.assign(sNick);
    m_ui32NickHash = HashNick(m_sNick);
}
//------------------------------------------------------------------------------
#ifdef USE_FLYLINKDC_EXT_JSON
void User::SetExtJSONOriginal(const char* sNewExtJSON, const uint16_t ui16NewExtJSONLen)
{
    if (m_user_ext_info)
    {
        Users::m_Ptr->DelFromExtJSONInfos(this);
        std::string l_info(sNewExtJSON, ui16NewExtJSONLen);
        m_user_ext_info->SetJSONOriginal(l_info);
        Users::m_Ptr->Add2ExtJSON(this);
    }
}
#endif
//------------------------------------------------------------------------------

void User::SetMyInfoOriginal(const char* sMyInfo, const uint16_t ui16MyInfoLen)
{
    std::vector<char> sOldMyInfo;
    sOldMyInfo.swap(m_sMyInfoOriginal);

    const std::string_view sOldDescription = m_sDescription;
    const std::string_view sOldTag = m_sTag;
    const std::string_view sOldConnection = m_sConnection;
    const std::string_view sOldEmail = m_sEmail;

    const uint64_t ui64OldShareSize = m_ui64SharedSize;

    if (!sOldMyInfo.empty())
    {
        m_sConnection = {};
        m_sDescription = {};
        m_sEmail = {};
        m_sTag = {};
        m_sClient = {};
        m_sTagVersion = {};
    }

    m_sMyInfoOriginal.resize(ui16MyInfoLen + 1);
    memcpy(m_sMyInfoOriginal.data(), sMyInfo, ui16MyInfoLen);
    m_sMyInfoOriginal[ui16MyInfoLen] = '\0';
    m_ui16MyInfoOriginalLen = ui16MyInfoLen;

    UserParseMyInfo(this);

    if (sOldDescription != m_sDescription)
    {
        m_ui32InfoBits |= INFOBIT_DESCRIPTION_CHANGED;
    }
    else
    {
        m_ui32InfoBits &= ~INFOBIT_DESCRIPTION_CHANGED;
    }

    if (sOldTag != m_sTag)
    {
        m_ui32InfoBits |= INFOBIT_TAG_CHANGED;
    }
    else
    {
        m_ui32InfoBits &= ~INFOBIT_TAG_CHANGED;
    }

    if (sOldConnection != m_sConnection)
    {
        m_ui32InfoBits |= INFOBIT_CONNECTION_CHANGED;
    }
    else
    {
        m_ui32InfoBits &= ~INFOBIT_CONNECTION_CHANGED;
    }

    if (sOldEmail != m_sEmail)
    {
        m_ui32InfoBits |= INFOBIT_EMAIL_CHANGED;
    }
    else
    {
        m_ui32InfoBits &= ~INFOBIT_EMAIL_CHANGED;
    }

    if (ui64OldShareSize != m_ui64SharedSize)
    {
        m_ui32InfoBits |= INFOBIT_SHARE_CHANGED;
    }
    else
    {
        m_ui32InfoBits &= ~INFOBIT_SHARE_CHANGED;
    }

    if (!((m_ui32InfoBits & INFOBIT_SHARE_SHORT_PERM) == INFOBIT_SHARE_SHORT_PERM))
    {
        m_ui64ChangedSharedSizeShort = m_ui64SharedSize;
    }

    if (!((m_ui32InfoBits & INFOBIT_SHARE_LONG_PERM) == INFOBIT_SHARE_LONG_PERM))
    {
        m_ui64ChangedSharedSizeLong = m_ui64SharedSize;
    }
}
//------------------------------------------------------------------------------

namespace {
void UserSetMyInfoLong(User* pUser, char* sMyInfoLong, const uint16_t& ui16MyInfoLongLen)
{
    if (!pUser->m_sMyInfoLong.empty())
    {
        if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 2)
        {
            Users::m_Ptr->DelFromMyInfosTag(pUser);
        }

        pUser->m_sMyInfoLong.clear();
    }

    pUser->m_sMyInfoLong.resize(ui16MyInfoLongLen + 1);
    memcpy(pUser->m_sMyInfoLong.data(), sMyInfoLong, ui16MyInfoLongLen);
    pUser->m_sMyInfoLong[ui16MyInfoLongLen] = '\0';
    pUser->m_ui16MyInfoLongLen = ui16MyInfoLongLen;
}
} // namespace
//------------------------------------------------------------------------------

namespace {
void UserSetMyInfoShort(User* pUser, char* sMyInfoShort, const uint16_t& ui16MyInfoShortLen)
{
    if (!pUser->m_sMyInfoShort.empty())
    {
        if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 0)
        {
            Users::m_Ptr->DelFromMyInfos(pUser);
        }

        pUser->m_sMyInfoShort.clear();
    }

    pUser->m_sMyInfoShort.resize(ui16MyInfoShortLen + 1);
    memcpy(pUser->m_sMyInfoShort.data(), sMyInfoShort, ui16MyInfoShortLen);
    pUser->m_sMyInfoShort[ui16MyInfoShortLen] = '\0';
    pUser->m_ui16MyInfoShortLen = ui16MyInfoShortLen;
}
} // namespace
//------------------------------------------------------------------------------

void User::SetVersion([[maybe_unused]] const char* sVersion)
{
#ifdef FLYLINKDC_USE_VERSION
    m_sVersion.assign(sVersion);
#endif
}
//------------------------------------------------------------------------------

void User::SetLastChat(std::string_view sData)
{
    m_sLastChat.assign(sData);
    m_ui16SameChatMsgs = 1;
    m_ui64SameChatsTick = ServerManager::m_ui64ActualTick;
    m_ui16SameMultiChats = 0;
}
//------------------------------------------------------------------------------

void User::SetLastPM(std::string_view sData)
{
    m_sLastPM.assign(sData);
    m_ui16SamePMs = 1;
    m_ui64SamePMsTick = ServerManager::m_ui64ActualTick;
    m_ui16SameMultiPms = 0;
}
//------------------------------------------------------------------------------

void User::SetLastSearch(std::string_view sData)
{
    m_LastSearch = sData;
    m_ui16SameSearchs = 1;
    m_ui64SameSearchsTick = ServerManager::m_ui64ActualTick;
}
//------------------------------------------------------------------------------

void User::SetBuffer(const char* sKickMsg, size_t szLen /* = 0*/)
{
    if (szLen == 0)
    {
        szLen = strlen(sKickMsg);
    }

    if (szLen < 512)
    {
        m_LogInOut.m_Buffer.resize(szLen + 1);
        memcpy(m_LogInOut.m_Buffer.data(), sKickMsg, szLen);
        m_LogInOut.m_Buffer[szLen] = '\0';
    }
    else
    {
        m_LogInOut.m_Buffer.resize(512);
        memcpy(m_LogInOut.m_Buffer.data(), sKickMsg, 508);
        m_LogInOut.m_Buffer[508] = '.';
        m_LogInOut.m_Buffer[509] = '.';
        m_LogInOut.m_Buffer[510] = '.';
        m_LogInOut.m_Buffer[511] = '\0';
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void User::FreeBuffer()
{
    m_LogInOut.m_Buffer.clear();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void User::Close(const bool bNoQuit /* = false*/)
{
    m_is_json_user = false;
    if (User::isPastLogin(m_ui8State))
    {
        return;
    }

    // Store old state for use in next lines.
    // State must be updated here and now to avoid multiple closes.
    const auto ui8OldState = std::to_underlying(m_ui8State);
    m_ui8State = UserStates::STATE_CLOSING;

    {
        const bool bError = (m_ui32BoolBits & BIT_ERROR) == BIT_ERROR;
        const char* sReason = bError ? "socket/protocol error" : "normal disconnect";
        LogDbg("[DCONN] Close nick='{}' ip='{}' old_state={} reason={}",
               m_sNick.empty() ? "<unknown>" : m_sNick,
               m_sIP.data(),
               static_cast<int>(ui8OldState),
               sReason);
    }

    // nick in hash table ?
    if ((m_ui32BoolBits & BIT_HASHED) == BIT_HASHED)
    {
        HashManager::m_Ptr->Remove(this);
    }

    // nick in nick/op list ?
    if (ui8OldState >= std::to_underlying(UserStates::STATE_ADDME_2LOOP))
    {
        Users::m_Ptr->DelFromNickList(m_sNick.c_str(), (m_ui32BoolBits & BIT_OPERATOR) == BIT_OPERATOR);
        Users::m_Ptr->DelFromUserIP(this);

        // PPK ... fix for QuickList nad ghost...
        // and fixing redirect all too ;)
        // and fix disconnect on send error too =)
        if (!bNoQuit)
        {
            // alex82 ... HideUser / ������� �����
            // alex82 ... NoQuit / ��������� $Quit ��� �����
            if (!((m_ui32InfoBits & INFOBIT_HIDDEN) == INFOBIT_HIDDEN) && !((m_ui32InfoBits & INFOBIT_NO_QUIT) == INFOBIT_NO_QUIT))
            {
                const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", m_sNick.c_str());
                if (CheckSprintf(iMsgLen, ServerManager::m_szGlobalBufferSize, "User::Close"))
                {
                    GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, nullptr, 0, GlobalDataQueue::Cmd::QUIT);
                }
            }
            Users::m_Ptr->Add2RecTimes(this);
        }

#ifdef FLYLINKDC_USE_DB
#ifdef _WITH_SQLITE
        DBSQLite::m_Ptr->UpdateRecord(this);
#endif
#endif
        if (((m_ui32BoolBits & BIT_HAVE_SHARECOUNTED) == BIT_HAVE_SHARECOUNTED))
        {
            ServerManager::m_ui64TotalShare -= m_ui64SharedSize;
            m_ui32BoolBits &= ~BIT_HAVE_SHARECOUNTED;
        }

        ScriptManager::m_Ptr->UserDisconnected(this);
    }

    // + alex82 ... HideUser / ������� �����
    if (!((m_ui32InfoBits & INFOBIT_HIDDEN) == User::INFOBIT_HIDDEN) && ui8OldState > std::to_underlying(UserStates::STATE_ADDME_2LOOP))
    {
        ServerManager::m_ui32Logged--;
    }

    User::DeletePrcsdUsrCmd(m_pCmdActive4Search);
    User::DeletePrcsdUsrCmd(m_pCmdActive6Search);
    User::DeletePrcsdUsrCmd(m_pCmdPassiveSearch);

    m_CmdList.clear();

    m_CmdToUserList.clear();

    if (!m_sMyInfoLong.empty())
    {
        if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 2)
        {
            Users::m_Ptr->DelFromMyInfosTag(this);
        }
    }

    if (!m_sMyInfoShort.empty())
    {
        if (SettingManager::m_Ptr->m_ui8FullMyINFOOption != 0)
        {
            Users::m_Ptr->DelFromMyInfos(this);
        }
    }

#ifdef USE_FLYLINKDC_EXT_JSON
    Users::m_Ptr->DelFromExtJSONInfos(this);
#endif

    if (m_ui32SendBufDataLen == 0 || (m_ui32BoolBits & BIT_ERROR) == BIT_ERROR)
    {
        m_ui8State = UserStates::STATE_REMME;
    }
    else
    {
        m_LogInOut.m_ui32ToCloseLoops = 100;
    }
}
//---------------------------------------------------------------------------

void User::Add2Userlist()
{
    Users::m_Ptr->Add2NickList(this);
    Users::m_Ptr->Add2UserIP(this);

    switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
    {
    case 0:
    {
        (void)GenerateMyInfoLong();
        Users::m_Ptr->Add2MyInfosTag(this);
        return;
    }
    case 1:
    {
        (void)GenerateMyInfoLong();
        Users::m_Ptr->Add2MyInfosTag(this);
        (void)GenerateMyInfoShort();
        Users::m_Ptr->Add2MyInfos(this);
        return;
    }
    case 2:
    {
        (void)GenerateMyInfoShort();
        Users::m_Ptr->Add2MyInfos(this);
        return;
    }
    default:
        break;
    }
}
//------------------------------------------------------------------------------

void User::SendCompressedOrPlain(const char* pData, uint32_t ui32DataLen, std::vector<char>& vZData, uint32_t& ui32ZDataLen)
{
    if (!isSupportZpipe())
    {
        SendCharDelayed(pData, ui32DataLen);
        return;
    }

    if (ui32ZDataLen == 0)
    {
        ZlibUtility::m_Ptr->CreateZPipe(std::string_view(pData, ui32DataLen), vZData, ui32ZDataLen, "ZPipeAlign");
        if (ui32ZDataLen == 0)
        {
            SendCharDelayed(pData, ui32DataLen);
            return;
        }
    }

    (void)PutInSendBuf(vZData.data(), ui32ZDataLen);
    ServerManager::m_ui64BytesSentSaved += ui32DataLen - ui32ZDataLen;
}
//------------------------------------------------------------------------------

void User::AddUserList()
{
    m_ui32BoolBits |= BIT_BIG_SEND_BUFFER;
    m_ui64LastNicklist = ServerManager::m_ui64ActualTick;

    if (!((m_ui32SupportBits & SUPPORTBIT_NOHELLO) == SUPPORTBIT_NOHELLO))
    {
        if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::ALLOWEDOPCHAT) ||
            (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] ||
             (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] && SettingManager::m_Ptr->m_bBotsSameNick)))
        {
            SendCompressedOrPlain(
                Users::m_Ptr->m_NickList.data(), Users::m_Ptr->m_ui32NickListLen, Users::m_Ptr->m_ZNickList, Users::m_Ptr->m_ui32ZNickListLen);
        }
        else
        {
            // PPK ... OpChat bot is now visible only for OPs ;)
            const int iLen = snprintf(
                ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s$$|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
            if (iLen > 0)
            {
                if (Users::m_Ptr->m_ui32NickListSize < Users::m_Ptr->m_ui32NickListLen + iLen)
                {
                    try
                    {
                        Users::m_Ptr->m_NickList.resize(Users::m_Ptr->m_ui32NickListSize + NICKLISTSIZE + 1);
                    }
                    catch (const std::bad_alloc&)
                    {
                        m_ui32BoolBits |= BIT_ERROR;
                        Close();

                        LogDbgErr("[MEM] Cannot reallocate {} bytes for m_NickList in User::AddUserList", Users::m_Ptr->m_ui32NickListSize + NICKLISTSIZE + 1);

                        return;
                    }
                    Users::m_Ptr->m_ui32NickListSize = static_cast<uint32_t>(Users::m_Ptr->m_NickList.size()) - 1;
                }

                memcpy(Users::m_Ptr->m_NickList.data() + Users::m_Ptr->m_ui32NickListLen - 1, ServerManager::m_pGlobalBuffer, iLen);
                Users::m_Ptr->m_NickList[Users::m_Ptr->m_ui32NickListLen + (iLen - 1)] = '\0';
                SendCharDelayed(Users::m_Ptr->m_NickList.data(), Users::m_Ptr->m_ui32NickListLen + (iLen - 1));
                Users::m_Ptr->m_NickList[Users::m_Ptr->m_ui32NickListLen - 1] = '|';
                Users::m_Ptr->m_NickList[Users::m_Ptr->m_ui32NickListLen] = '\0';
            }
        }
    }

    switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
    {
    case 0:
    {
#ifdef USE_FLYLINKDC_EXT_JSON
        SendCharDelayedExtJSON();
#endif
        if (Users::m_Ptr->m_ui32MyInfosTagLen == 0)
        {
            break;
        }

        SendCompressedOrPlain(
            Users::m_Ptr->m_MyInfosTag.data(), Users::m_Ptr->m_ui32MyInfosTagLen, Users::m_Ptr->m_ZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
        break;
    }

    case 1:
    {
#ifdef USE_FLYLINKDC_EXT_JSON
        SendCharDelayedExtJSON();
#endif
        if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::SENDFULLMYINFOS))
        {
            if (Users::m_Ptr->m_ui32MyInfosLen == 0)
            {
                break;
            }

            SendCompressedOrPlain(Users::m_Ptr->m_MyInfos.data(), Users::m_Ptr->m_ui32MyInfosLen, Users::m_Ptr->m_ZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
        }
        else
        {

            if (Users::m_Ptr->m_ui32MyInfosTagLen == 0)
            {
                break;
            }
            SendCompressedOrPlain(
                Users::m_Ptr->m_MyInfosTag.data(), Users::m_Ptr->m_ui32MyInfosTagLen, Users::m_Ptr->m_ZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
        }
        break;
    }
    case 2:
    {
#ifdef USE_FLYLINKDC_EXT_JSON
        SendCharDelayedExtJSON();
#endif
        if (Users::m_Ptr->m_ui32MyInfosLen == 0)
        {
            break;
        }

        SendCompressedOrPlain(Users::m_Ptr->m_MyInfos.data(), Users::m_Ptr->m_ui32MyInfosLen, Users::m_Ptr->m_ZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
    }
    default:
        break;
    }

    if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::ALLOWEDOPCHAT) ||
        (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] || (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] && SettingManager::m_Ptr->m_bBotsSameNick)))
    {
        if (Users::m_Ptr->m_ui32OpListLen > 9)
        {
            SendCompressedOrPlain(Users::m_Ptr->m_OpList.data(), Users::m_Ptr->m_ui32OpListLen, Users::m_Ptr->m_ZOpList, Users::m_Ptr->m_ui32ZOpListLen);
        }
    }
    else
    {
        // PPK ... OpChat bot is now visible only for OPs ;)
        SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO)]);
        const int iLen = snprintf(
            ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s$$|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
        if (iLen > 0)
        {
            if (Users::m_Ptr->m_ui32OpListSize < Users::m_Ptr->m_ui32OpListLen + iLen)
            {
                try
                {
                    Users::m_Ptr->m_OpList.resize(Users::m_Ptr->m_ui32OpListSize + OPLISTSIZE + 1);
                }
                catch (const std::bad_alloc&)
                {
                    m_ui32BoolBits |= BIT_ERROR;
                    Close();

                    LogDbgErr("[MEM] Cannot reallocate {} bytes for m_OpList in User::AddUserList", Users::m_Ptr->m_ui32OpListSize + OPLISTSIZE + 1);

                    return;
                }
                Users::m_Ptr->m_ui32OpListSize = static_cast<uint32_t>(Users::m_Ptr->m_OpList.size()) - 1;
            }

            memcpy(Users::m_Ptr->m_OpList.data() + Users::m_Ptr->m_ui32OpListLen - 1, ServerManager::m_pGlobalBuffer, iLen);
            Users::m_Ptr->m_OpList[Users::m_Ptr->m_ui32OpListLen + (iLen - 1)] = '\0';
            SendCharDelayed(Users::m_Ptr->m_OpList.data(), Users::m_Ptr->m_ui32OpListLen + (iLen - 1));
            Users::m_Ptr->m_OpList[Users::m_Ptr->m_ui32OpListLen - 1] = '|';
            Users::m_Ptr->m_OpList[Users::m_Ptr->m_ui32OpListLen] = '\0';
        }
    }

    if (ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::SENDALLUSERIP) && ((m_ui32SupportBits & SUPPORTBIT_USERIP2) == SUPPORTBIT_USERIP2))
    {
        if (Users::m_Ptr->m_ui32UserIPListLen > 9)
        {
            SendCompressedOrPlain(
                Users::m_Ptr->m_UserIPList.data(), Users::m_Ptr->m_ui32UserIPListLen, Users::m_Ptr->m_ZUserIPList, Users::m_Ptr->m_ui32ZUserIPListLen);
        }
    }
}
//---------------------------------------------------------------------------

bool User::GenerateMyInfoLong() // true == changed
{
    // Prepare myinfo with nick
    int iLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$MyINFO $ALL %s ", m_sNick.c_str());
    if (iLen <= 0)
    {
        return false;
    }

    // Add description
    if (!m_sChangedDescriptionLong.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedDescriptionLong.data(), m_sChangedDescriptionLong.size());
        iLen += static_cast<int>(m_sChangedDescriptionLong.size());

        if (!((m_ui32InfoBits & INFOBIT_DESCRIPTION_LONG_PERM) == INFOBIT_DESCRIPTION_LONG_PERM))
        {
            m_sChangedDescriptionLong.clear();
        }
    }
    else if (!m_sDescription.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sDescription.data(), m_sDescription.size());
        iLen += static_cast<int>(m_sDescription.size());
    }

    // Add tag
    if (!m_sChangedTagLong.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedTagLong.data(), m_sChangedTagLong.size());
        iLen += static_cast<int>(m_sChangedTagLong.size());

        if (!((m_ui32InfoBits & INFOBIT_TAG_LONG_PERM) == INFOBIT_TAG_LONG_PERM))
        {
            m_sChangedTagLong.clear();
        }
    }
    else if (!m_sTag.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sTag.data(), m_sTag.size());
        iLen += static_cast<int>(m_sTag.size());
    }

    memcpy(ServerManager::m_pGlobalBuffer + iLen, "$ $", 3); // NOLINT(bugprone-not-null-terminated-result) length-tracked buffer
    iLen += 3;

    // Add connection
    if (!m_sChangedConnectionLong.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedConnectionLong.data(), m_sChangedConnectionLong.size());
        iLen += static_cast<int>(m_sChangedConnectionLong.size());

        if (!((m_ui32InfoBits & INFOBIT_CONNECTION_LONG_PERM) == INFOBIT_CONNECTION_LONG_PERM))
        {
            m_sChangedConnectionLong.clear();
        }
    }
    else if (!m_sConnection.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sConnection.data(), m_sConnection.size());
        iLen += static_cast<int>(m_sConnection.size());
    }

    // add magicbyte
    uint8_t ui8Magic = m_ui8MagicByte;

    if (!((m_ui32BoolBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2))
    {
        // should not be set if user not have TLS2 support
        ui8Magic &= ~0x10;
        ui8Magic &= ~0x20;
    }

    if ((m_ui32BoolBits & BIT_IPV4) == BIT_IPV4)
    {
        ui8Magic |= 0x40; // IPv4 support
    }
    else
    {
        ui8Magic &= ~0x40; // IPv4 support
    }

    if ((m_ui32BoolBits & BIT_IPV6) == BIT_IPV6)
    {
        ui8Magic |= 0x80; // IPv6 support
    }
    else
    {
        ui8Magic &= ~0x80; // IPv6 support
    }

    ServerManager::m_pGlobalBuffer[iLen] = static_cast<char>(ui8Magic);
    ServerManager::m_pGlobalBuffer[iLen + 1] = '$';
    iLen += 2;

    // Add email
    if (!m_sChangedEmailLong.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedEmailLong.data(), m_sChangedEmailLong.size());
        iLen += static_cast<int>(m_sChangedEmailLong.size());

        if (!((m_ui32InfoBits & INFOBIT_EMAIL_LONG_PERM) == INFOBIT_EMAIL_LONG_PERM))
        {
            m_sChangedEmailLong.clear();
        }
    }
    else if (!m_sEmail.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sEmail.data(), m_sEmail.size());
        iLen += static_cast<int>(m_sEmail.size());
    }

    // Add share and end of myinfo
    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iLen, ServerManager::m_szGlobalBufferSize, "$%" PRIu64 "$|", m_ui64ChangedSharedSizeLong))
    {
        return false;
    }

    if (!((m_ui32InfoBits & INFOBIT_SHARE_LONG_PERM) == INFOBIT_SHARE_LONG_PERM))
    {
        m_ui64ChangedSharedSizeLong = m_ui64SharedSize;
    }

    if (!m_sMyInfoLong.empty())
    {
        if (m_ui16MyInfoLongLen == static_cast<uint16_t>(iLen) && memcmp(m_sMyInfoLong.data() + 14 + m_sNick.size(),
                                                                         ServerManager::m_pGlobalBuffer + 14 + m_sNick.size(),
                                                                         m_ui16MyInfoLongLen - 14 - m_sNick.size()) == 0)
        {
            return false;
        }
    }

    UserSetMyInfoLong(this, ServerManager::m_pGlobalBuffer, static_cast<uint16_t>(iLen));

    return true;
}
//---------------------------------------------------------------------------

bool User::GenerateMyInfoShort() // true == changed
{
    // Prepare myinfo with nick
    int iLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$MyINFO $ALL %s ", m_sNick.c_str());
    if (iLen <= 0)
    {
        return false;
    }

    // Add mode to start of description if is enabled
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_MODE_TO_DESCRIPTION)] && m_sModes[0] != 0)
    {
        std::string_view sActualDescription;

        if (!m_sChangedDescriptionShort.empty())
        {
            sActualDescription = m_sChangedDescriptionShort;
        }
        else if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_STRIP_DESCRIPTION)])
        {
            sActualDescription = m_sDescription;
        }

        if (sActualDescription.empty())
        {
            ServerManager::m_pGlobalBuffer[iLen] = m_sModes[0];
            iLen++;
        }
        else if (sActualDescription[0] != m_sModes[0] && sActualDescription[1] != ' ')
        {
            ServerManager::m_pGlobalBuffer[iLen] = m_sModes[0];
            ServerManager::m_pGlobalBuffer[iLen + 1] = ' ';
            iLen += 2;
        }
    }

    // Add description
    if (!m_sChangedDescriptionShort.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedDescriptionShort.data(), m_sChangedDescriptionShort.size());
        iLen += static_cast<int>(m_sChangedDescriptionShort.size());

        if (!((m_ui32InfoBits & INFOBIT_DESCRIPTION_SHORT_PERM) == INFOBIT_DESCRIPTION_SHORT_PERM))
        {
            m_sChangedDescriptionShort.clear();
        }
    }
    else if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_STRIP_DESCRIPTION)] && !m_sDescription.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sDescription.data(), m_sDescription.size());
        iLen += static_cast<int>(m_sDescription.size());
    }

    // Add tag
    if (!m_sChangedTagShort.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedTagShort.data(), m_sChangedTagShort.size());
        iLen += static_cast<int>(m_sChangedTagShort.size());

        if (!((m_ui32InfoBits & INFOBIT_TAG_SHORT_PERM) == INFOBIT_TAG_SHORT_PERM))
        {
            m_sChangedTagShort.clear();
        }
    }
    else if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_STRIP_TAG)] && !m_sTag.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sTag.data(), m_sTag.size());
        iLen += static_cast<int>(m_sTag.size());
    }

    // Add mode to myinfo if is enabled
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_MODE_TO_MYINFO)] && m_sModes[0] != 0)
    {
        if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iLen, ServerManager::m_szGlobalBufferSize, "$%c$", m_sModes[0]))
        {
            return false;
        }
    }
    else
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, "$ $", 3); // NOLINT(bugprone-not-null-terminated-result) length-tracked buffer
        iLen += 3;
    }

    // Add connection
    if (!m_sChangedConnectionShort.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedConnectionShort.data(), m_sChangedConnectionShort.size());
        iLen += static_cast<int>(m_sChangedConnectionShort.size());

        if (!((m_ui32InfoBits & INFOBIT_CONNECTION_SHORT_PERM) == INFOBIT_CONNECTION_SHORT_PERM))
        {
            m_sChangedConnectionShort.clear();
        }
    }
    else if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_STRIP_CONNECTION)] && !m_sConnection.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sConnection.data(), m_sConnection.size());
        iLen += static_cast<int>(m_sConnection.size());
    }

    // add magicbyte
    uint8_t ui8Magic = m_ui8MagicByte;

    if (!((m_ui32BoolBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2))
    {
        // should not be set if user not have TLS2 support
        ui8Magic &= ~0x10;
        ui8Magic &= ~0x20;
    }

    if ((m_ui32BoolBits & BIT_IPV4) == BIT_IPV4)
    {
        ui8Magic |= 0x40; // IPv4 support
    }
    else
    {
        ui8Magic &= ~0x40; // IPv4 support
    }

    if ((m_ui32BoolBits & BIT_IPV6) == BIT_IPV6)
    {
        ui8Magic |= 0x80; // IPv6 support
    }
    else
    {
        ui8Magic &= ~0x80; // IPv6 support
    }

    ServerManager::m_pGlobalBuffer[iLen] = static_cast<char>(ui8Magic);
    ServerManager::m_pGlobalBuffer[iLen + 1] = '$';
    iLen += 2;

    // Add email
    if (!m_sChangedEmailShort.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sChangedEmailShort.data(), m_sChangedEmailShort.size());
        iLen += static_cast<int>(m_sChangedEmailShort.size());

        if (!((m_ui32InfoBits & INFOBIT_EMAIL_SHORT_PERM) == INFOBIT_EMAIL_SHORT_PERM))
        {
            m_sChangedEmailShort.clear();
        }
    }
    else if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_STRIP_EMAIL)] && !m_sEmail.empty())
    {
        memcpy(ServerManager::m_pGlobalBuffer + iLen, m_sEmail.data(), m_sEmail.size());
        iLen += static_cast<int>(m_sEmail.size());
    }

    // Add share and end of myinfo
    if (!SnprintfAppend(ServerManager::m_pGlobalBuffer, iLen, ServerManager::m_szGlobalBufferSize, "$%" PRIu64 "$|", m_ui64ChangedSharedSizeShort))
    {
        return false;
    }

    if (!((m_ui32InfoBits & INFOBIT_SHARE_SHORT_PERM) == INFOBIT_SHARE_SHORT_PERM))
    {
        m_ui64ChangedSharedSizeShort = m_ui64SharedSize;
    }

    if (!m_sMyInfoShort.empty())
    {
        if (m_ui16MyInfoShortLen == static_cast<uint16_t>(iLen) && memcmp(m_sMyInfoShort.data() + 14 + m_sNick.size(),
                                                                          ServerManager::m_pGlobalBuffer + 14 + m_sNick.size(),
                                                                          m_ui16MyInfoShortLen - 14 - m_sNick.size()) == 0)
        {
            return false;
        }
    }

    UserSetMyInfoShort(this, ServerManager::m_pGlobalBuffer, static_cast<uint16_t>(iLen));

    return true;
}
//---------------------------------------------------------------------------

void User::HasSuspiciousTag()
{
    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REPORT_SUSPICIOUS_TAG)] && SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_STATUS_MESSAGES)])
    {
        const std::string sDesc(m_sDescription);
        GlobalDataQueue::m_Ptr->StatusMessageFormat("User::HasSuspiciousTag",
                                                    "<%s> *** %s (%s) %s. %s: %s|",
                                                    SettingManager::HubSec(),
                                                    m_sNick.c_str(),
                                                    m_sIP.data(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAS_SUSPICIOUS_TAG_CHECK_HIM)].c_str(),
                                                    LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_FULL_DESCRIPTION)].c_str(),
                                                    sDesc.c_str());
    }
    m_ui32BoolBits &= ~BIT_HAVE_BADTAG;
}
//---------------------------------------------------------------------------

bool User::ProcessRules()
{
    // if share limit enabled, check it
    if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOSHARELIMIT))
    {
        if ((SettingManager::m_Ptr->m_ui64MinShare != 0 && m_ui64SharedSize < SettingManager::m_Ptr->m_ui64MinShare) ||
            (SettingManager::m_Ptr->m_ui64MaxShare != 0 && m_ui64SharedSize > SettingManager::m_Ptr->m_ui64MaxShare))
        {
            SendChar(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_SHARE_LIMIT_MSG)]);
            return false;
        }
    }

    // no Tag? Apply rule
    if (m_sTag.empty())
    {
        if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOTAGCHECK))
        {
            if (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_NO_TAG_OPTION)] != 0)
            {
                SendChar(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_NO_TAG_MSG)]);
                return false;
            }
        }
    }
    else
    {
        // min and max slot check
        if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOSLOTCHECK))
        {
            // TODO 2 -oPTA -ccheckers: $SR based slots fetching for no_tag users

            if ((SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SLOTS_LIMIT)] != 0 &&
                 m_ui32Slots < static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MIN_SLOTS_LIMIT)])) ||
                (SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SLOTS_LIMIT)] != 0 &&
                 m_ui32Slots > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_SLOTS_LIMIT)])))
            {
                SendChar(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_SLOTS_LIMIT_MSG)]);
                return false;
            }
        }

        // slots/hub ration check
        if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOSLOTHUBRATIO) && SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_HUBS)] != 0 &&
            SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_SLOTS)] != 0)
        {
            const uint32_t slots = m_ui32Slots;
            const uint32_t hubs = m_ui32Hubs > 0 ? m_ui32Hubs : 1;
            if ((static_cast<double>(slots) / hubs) < (static_cast<double>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_SLOTS)]) /
                                                       SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_HUB_SLOT_RATIO_HUBS)]))
            {
                SendChar(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_HUB_SLOT_RATIO_MSG)]);
                return false;
            }
        }

        // hub checker
        if (!ProfileManager::m_Ptr->IsAllowed(this, ProfileManager::NOMAXHUBCHECK) && SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_HUBS_LIMIT)] != 0)
        {
            if (m_ui32Hubs > static_cast<uint32_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_MAX_HUBS_LIMIT)]))
            {
                SendChar(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_MAX_HUBS_LIMIT_MSG)]);
                return false;
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------

void User::AddPrcsdCmd(const uint8_t ui8Type, const char* sCommand, const size_t szCommandLen, void* pExtraData, const bool bIsPm /* = false*/)
{
    if (ui8Type == PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO)
    {
        auto* pToUser = static_cast<User*>(pExtraData);
        for (const auto& pItem : m_CmdToUserList)
        {
            if (pItem->m_pToUser == pToUser)
            {
                pItem->m_sCommand.append(!sCommand ? "" : sCommand, szCommandLen);
                pItem->m_ui32PmCount += bIsPm ? 1 : 0;
                return;
            }
        }

        auto pNewToCmd = std::make_unique<PrcsdToUsrCmd>();

        pNewToCmd->m_sCommand.assign(!sCommand ? "" : sCommand, szCommandLen);

        pNewToCmd->m_ui32PmCount = bIsPm ? 1 : 0;
        pNewToCmd->m_ui32Loops = 0;
        pNewToCmd->m_pToUser = pToUser;

        pNewToCmd->m_nick = pToUser->m_sNick;

        m_CmdToUserList.push_back(std::move(pNewToCmd));

        return;
    }

    auto pNewcmd = std::make_unique<PrcsdUsrCmd>();

    if (sCommand && szCommandLen > 0)
    {
        pNewcmd->m_sCommand.assign(sCommand, sCommand + szCommandLen);
    }
    pNewcmd->m_sCommand.resize(szCommandLen + 1);
    pNewcmd->m_sCommand[szCommandLen] = '\0';

    pNewcmd->m_ui32Len = static_cast<uint32_t>(szCommandLen);
    pNewcmd->m_ui8Type = ui8Type;
    pNewcmd->m_pPtr = pExtraData;

    m_CmdList.push_back(std::move(pNewcmd));
}
//---------------------------------------------------------------------------

void User::AddMeOrIPv4Check()
{
    if (((m_ui32BoolBits & BIT_IPV6) == BIT_IPV6) && ((m_ui32SupportBits & SUPPORTBIT_IPV4) == SUPPORTBIT_IPV4) && ServerManager::m_sHubIP[0] != '\0' &&
        ServerManager::m_bUseIPv4)
    {
        m_ui8State = UserStates::STATE_IPV4_CHECK;
        m_LogInOut.m_ui64IPv4CheckTick = ServerManager::m_ui64ActualTick;

        SendFormat(
            "AddMeOrIPv4Check", true, "$ConnectToMe %s %s:%hu|", m_sNick.c_str(), ServerManager::m_sHubIP.data(), SettingManager::m_Ptr->m_ui16PortNumbers[0]);
    }
    else
    {
        m_ui8State = UserStates::STATE_ADDME;
    }
}
//---------------------------------------------------------------------------

void User::SetUserInfo(std::string& sOldData, std::string_view sNewData)
{
    sOldData.assign(sNewData);
}
//---------------------------------------------------------------------------

void User::RemFromSendBuf(const char* sData, const uint32_t ui32Len, const uint32_t ui32SendBufLen)
{
    char* match = strstr(m_pSendBuf.get() + ui32SendBufLen, sData);
    if (match)
    {
        memmove(match, match + ui32Len, m_ui32SendBufDataLen - ((match + (ui32Len)) - m_pSendBuf.get()));
        m_ui32SendBufDataLen -= ui32Len;
        m_pSendBuf[m_ui32SendBufDataLen] = '\0';
    }
}
//------------------------------------------------------------------------------

void User::DeletePrcsdUsrCmd(PrcsdUsrCmd*& pCommand)
{
    if (pCommand)
    {
        std::unique_ptr<PrcsdUsrCmd> guard(pCommand);
        pCommand = nullptr;
    }
}
//------------------------------------------------------------------------------
