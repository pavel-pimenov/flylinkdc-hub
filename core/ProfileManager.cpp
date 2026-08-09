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
#include "ProfileManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "PXBReader.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include <tinyxml2.h>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::unique_ptr<ProfileManager> ProfileManager::m_Ptr;
//---------------------------------------------------------------------------
static const char sPtokaXProfiles[] = "PtokaX Profiles"; // NOLINT(modernize-avoid-c-arrays)
static constexpr size_t g_szPtokaXProfilesLen = sizeof(sPtokaXProfiles) - 1;
static const char sProfilePermissionIds[] = // PN reserved for profile name! // NOLINT(modernize-avoid-c-arrays)
    "OP"                                    // HASKEYICON
    "DG"                                    // NODEFLOODGETNICKLIST
    "DM"                                    // NODEFLOODMYINFO
    "DS"                                    // NODEFLOODSEARCH
    "DP"                                    // NODEFLOODPM
    "DN"                                    // NODEFLOODMAINCHAT
    "MM"                                    // MASSMSG
    "TO"                                    // TOPIC
    "TB"                                    // TEMP_BAN
    "RT"                                    // REFRESHTXT
    "NT"                                    // NOTAGCHECK
    "TU"                                    // TEMP_UNBAN
    "DR"                                    // DELREGUSER
    "AR"                                    // ADDREGUSER
    "NC"                                    // NOCHATLIMITS
    "NH"                                    // NOMAXHUBCHECK
    "NR"                                    // NOSLOTHUBRATIO
    "NS"                                    // NOSLOTCHECK
    "NA"                                    // NOSHARELIMIT
    "CP"                                    // CLRPERMBAN
    "CT"                                    // CLRTEMPBAN
    "GI"                                    // GETINFO
    "GB"                                    // GETBANLIST
    "RS"                                    // RSTSCRIPTS
    "RH"                                    // RSTHUB
    "TP"                                    // TEMPOP
    "GG"                                    // GAG
    "RE"                                    // REDIRECT
    "BN"                                    // BAN
    "KI"                                    // KICK
    "DR"                                    // DROP
    "EF"                                    // ENTERFULLHUB
    "EB"                                    // ENTERIFIPBAN
    "AO"                                    // ALLOWEDOPCHAT
    "UI"                                    // SENDALLUSERIP
    "RB"                                    // RANGE_BAN
    "RU"                                    // RANGE_UNBAN
    "RT"                                    // RANGE_TBAN
    "RV"                                    // RANGE_TUNBAN
    "GR"                                    // GET_RANGE_BANS
    "CR"                                    // CLR_RANGE_BANS
    "CU"                                    // CLR_RANGE_TBANS
    "UN"                                    // UNBAN
    "NT"                                    // NOSEARCHLIMITS
    "SM"                                    // SENDFULLMYINFOS
    "NI"                                    // NOIPCHECK
    "CL"                                    // CLOSE
    "DC"                                    // NODEFLOODCTM
    "DR"                                    // NODEFLOODRCTM
    "DT"                                    // NODEFLOODSR
    "DU"                                    // NODEFLOODRECV
    "CI"                                    // NOCHATINTERVAL
    "PI"                                    // NOPMINTERVAL
    "SI"                                    // NOSEARCHINTERVAL
    "UI"                                    // NOUSRSAMEIP
    "RT"                                    // NORECONNTIME
    ;
static constexpr size_t g_szProfilePermissionIdsLen = sizeof(sProfilePermissionIds) - 1;
//---------------------------------------------------------------------------

ProfileItem::ProfileItem() // NOLINT(modernize-use-equals-default) fill(false) explicit for clarity
{
    m_bPermissions.fill(false);
}
//---------------------------------------------------------------------------
void ProfileManager::Load()
{
    PXBReader pxbProfiles;

    // Open profiles file
    if (!pxbProfiles.OpenFileRead((ServerManager::m_sPath + "/cfg/Profiles.pxb").c_str(), NORECONNTIME + 2))
    {
        LogDbg("[ERR] Cannot open Profiles.pxb in ProfileManager::Load");
        return;
    }

    // Read file header
    std::array<uint16_t, ProfileManager::NORECONNTIME + 2> ui16Identificators = {};
    memcpy(ui16Identificators.data(), "FI", 2);
    memcpy(ui16Identificators.data() + 1, "FV", 2);

    if (!pxbProfiles.ReadNextItem(ui16Identificators.data(), 2))
    {
        return;
    }

    // Check header if we have correct file
    if (pxbProfiles.m_ui16ItemLengths[0] != g_szPtokaXProfilesLen ||
        strncmp(static_cast<const char*>(pxbProfiles.m_pItemDatas[0]), sPtokaXProfiles, g_szPtokaXProfilesLen) != 0)
    {
        return;
    }

    {
        uint32_t ui32FileVersion;
        memcpy(&ui32FileVersion, pxbProfiles.m_pItemDatas[1], sizeof(ui32FileVersion));
        ui32FileVersion = ntohl(ui32FileVersion);

        if (ui32FileVersion < 1)
        {
            return;
        }
    }

    // Read settings =)
    memcpy(ui16Identificators.data(), "PN", 2);
    memcpy(ui16Identificators.data() + 1, sProfilePermissionIds, g_szProfilePermissionIdsLen);

    bool bSuccess = pxbProfiles.ReadNextItem(ui16Identificators.data(), NORECONNTIME + 2);

    while (bSuccess)
    {
        ProfileItem* pNewProfile = CreateProfile(static_cast<const char*>(pxbProfiles.m_pItemDatas[0]));

        for (uint16_t ui16i = 0; ui16i <= NORECONNTIME; ui16i++)
        {
            if ((static_cast<const char*>(pxbProfiles.m_pItemDatas[ui16i + 1]))[0] == '0')
            {
                pNewProfile->m_bPermissions[ui16i] = false;
            }
            else
            {
                pNewProfile->m_bPermissions[ui16i] = true;
            }
        }

        bSuccess = pxbProfiles.ReadNextItem(ui16Identificators.data(), NORECONNTIME + 2);
    }
}
//---------------------------------------------------------------------------

void ProfileManager::LoadXML()
{
    tinyxml2::XMLDocument doc;
    (void)LoadXmlConfig(doc, "Profiles.xml");

    tinyxml2::XMLHandle cfg(&doc);
    tinyxml2::XMLNode* profiles = cfg.FirstChildElement("Profiles").ToNode();
    if (profiles)
    {
        tinyxml2::XMLElement* child = profiles->FirstChildElement();
        while (child)
        {
            const char* sName = XmlGetRequiredText(child, "Name");
            if (!sName)
            {
                child = child->NextSiblingElement();
                continue;
            }

            const char* sRights = XmlGetRequiredText(child, "Permissions");
            if (!sRights)
            {
                child = child->NextSiblingElement();
                continue;
            }

            ProfileItem* pNewProfile = CreateProfile(sName);

            const size_t szRightsLen = strlen(sRights);
            if (szRightsLen == 32 || szRightsLen == 256)
            {
                for (size_t i = 0; i < szRightsLen; i++)
                {
                    pNewProfile->m_bPermissions[i] = (sRights[i] == '1');
                }
            }
            else
            {
                std::unique_ptr<ProfileItem> guard(pNewProfile);
                child = child->NextSiblingElement();
                continue;
            }
            child = child->NextSiblingElement();
        }
    }
    else
    {
        LogInfo("{}", LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_PROFILES_LOAD_FAIL)]);
        exit(EXIT_FAILURE);
    }
}
//---------------------------------------------------------------------------

ProfileManager::ProfileManager()
{
    if (FileExist((ServerManager::m_sPath + "/cfg/Profiles.pxb").c_str()))
    {
        Load();
        return;
    }
    if (FileExist((ServerManager::m_sPath + "/cfg/Profiles.xml").c_str()))
    {
        LoadXML();
        return;
    }

    {
        const char* sProfileNames[] = {"Master", "Operator", "VIP", "Reg"}; // NOLINT(modernize-avoid-c-arrays)
        const char* sProfilePermisions[] = {"10011111111111111111111111111111111111111111101000111111", // NOLINT(modernize-avoid-c-arrays)
                                            "10011111101111111110011000111111111000000011101000111111",
                                            "00000000000000011110000000000001100000000000000000000111",
                                            "00000000000000000000000000000001100000000000000000000000"};

        for (uint8_t ui8i = 0; ui8i < 4; ui8i++)
        {
            ProfileItem* pNewProfile = CreateProfile(sProfileNames[ui8i]);
            const size_t szProfilePermissionsLen = strlen(sProfilePermisions[ui8i]);
            for (size_t ui8j = 0; ui8j < szProfilePermissionsLen; ui8j++)
            {
                pNewProfile->m_bPermissions[ui8j] = (sProfilePermisions[ui8i][ui8j] == '1');
            }
        }

        SaveProfiles();
    }
}
//---------------------------------------------------------------------------

ProfileManager::~ProfileManager()
{
    // Do not call SaveProfiles() here — static destruction order fiasco.
    // SaveProfiles() is called explicitly in ServerManager::FinalClose().

    m_vpProfilesTable.clear();
    m_ui16ProfileCount = 0;
}
//---------------------------------------------------------------------------

void ProfileManager::SaveProfiles()
{
    PXBReader pxbProfiles;

    // Open profiles file
    if (!pxbProfiles.OpenFileSave((ServerManager::m_sPath + "/cfg/Profiles.pxb").c_str(), NORECONNTIME + 2))
    {
        LogDbg("[ERR] Cannot open Profiles.pxb in ProfileManager::SaveProfiles");
        return;
    }

    // Write file header
    pxbProfiles.m_sItemIdentifiers[0] = 'F';
    pxbProfiles.m_sItemIdentifiers[1] = 'I';
    pxbProfiles.m_ui16ItemLengths[0] = static_cast<uint16_t>(g_szPtokaXProfilesLen);
    pxbProfiles.m_pItemDatas[0] = sPtokaXProfiles;
    pxbProfiles.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbProfiles.m_sItemIdentifiers[2] = 'F';
    pxbProfiles.m_sItemIdentifiers[3] = 'V';
    pxbProfiles.m_ui16ItemLengths[1] = 4;
    const uint32_t ui32Version = 1;
    pxbProfiles.m_pItemDatas[1] = &ui32Version;
    pxbProfiles.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if (!pxbProfiles.WriteNextItem(g_szPtokaXProfilesLen + 4, 2))
    {
        return;
    }

    pxbProfiles.m_sItemIdentifiers[0] = 'P';
    pxbProfiles.m_sItemIdentifiers[1] = 'N';
    pxbProfiles.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    memcpy(pxbProfiles.m_sItemIdentifiers.data() + 2, sProfilePermissionIds, g_szProfilePermissionIdsLen);
    std::fill_n(pxbProfiles.m_ui8ItemValues.data() + 1, NORECONNTIME + 1, PXBReader::PXB_BYTE);

    for (const auto& pProfile : m_vpProfilesTable)
    {
        pxbProfiles.m_ui16ItemLengths[0] = static_cast<uint16_t>(pProfile->m_sName.size());
        pxbProfiles.m_pItemDatas[0] = pProfile->m_sName.c_str();
        pxbProfiles.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

        for (uint16_t ui16j = 0; ui16j <= NORECONNTIME; ui16j++)
        {
            pxbProfiles.m_ui16ItemLengths[ui16j + 1] = 1;
            pxbProfiles.m_pItemDatas[ui16j + 1] = (pProfile->m_bPermissions[ui16j] ? &PXBReader::s_TrueSentinel : nullptr);
            pxbProfiles.m_ui8ItemValues[ui16j + 1] = PXBReader::PXB_BYTE;
        }

        if (!pxbProfiles.WriteNextItem(pxbProfiles.m_ui16ItemLengths[0] + NORECONNTIME + 1, NORECONNTIME + 2))
        {
            break;
        }
    }

    pxbProfiles.WriteRemaining();
}
//---------------------------------------------------------------------------

bool ProfileManager::IsAllowed(const User* pUser, const uint32_t ui32Option) const
{
    // profile number -1 = normal user/no profile assigned
    if (pUser->m_i32Profile == -1)
    {
        return false;
    }

    // return right of the profile
    return m_vpProfilesTable[pUser->m_i32Profile]->m_bPermissions[ui32Option];
}
//---------------------------------------------------------------------------

bool ProfileManager::IsProfileAllowed(const int32_t i32Profile, const uint32_t ui32Option) const
{
    // profile number -1 = normal user/no profile assigned
    if (i32Profile == -1)
    {
        return false;
    }

    // return right of the profile
    return m_vpProfilesTable[i32Profile]->m_bPermissions[ui32Option];
}
//---------------------------------------------------------------------------

int32_t ProfileManager::AddProfile(const char* sName)
{
    for (const auto& pProfile : m_vpProfilesTable)
    {
        if (iequals(pProfile->m_sName, sName))
        {
            return -1;
        }
    }

    uint32_t ui32j = 0;
    while (true)
    {
        switch (sName[ui32j])
        {
        case '\0':
            break;
        case '|':
            return -2;
        default:
            if (sName[ui32j] < 33)
            {
                return -2;
            }

            ui32j++;
            continue;
        }

        break;
    }

    (void)CreateProfile(sName);

    return static_cast<int32_t>(m_ui16ProfileCount - 1);
}
//---------------------------------------------------------------------------

std::optional<uint16_t> ProfileManager::GetProfileIndex(const char* sName) const
{
    for (uint16_t ui16i = 0; ui16i < m_ui16ProfileCount; ui16i++)
    {
        if (iequals(m_vpProfilesTable[ui16i]->m_sName, sName))
        {
            return ui16i;
        }
    }

    return std::nullopt;
}
//---------------------------------------------------------------------------

// RemoveProfileByName(name)
// returns: 0 if the name doesnot exists or is a default profile idx 0-3
//          -1 if the profile is in use
//          1 on success
int32_t ProfileManager::RemoveProfileByName(const char* sName)
{
    for (uint16_t ui16i = 0; ui16i < m_ui16ProfileCount; ui16i++)
    {
        if (iequals(m_vpProfilesTable[ui16i]->m_sName, sName))
        {
            return (RemoveProfile(ui16i) ? 1 : -1);
        }
    }

    return 0;
}

//---------------------------------------------------------------------------

bool ProfileManager::RemoveProfile(const uint16_t ui16Profile)
{
    for (const auto& curRegPtr : RegManager::m_Ptr->m_RegList)
    {
        if (curRegPtr->m_ui16Profile == ui16Profile)
        {
            // Profile in use can't be deleted!
            return false;
        }
    }

    m_ui16ProfileCount--;

    m_vpProfilesTable[ui16Profile].reset();

    for (uint16_t ui16i = ui16Profile; ui16i < m_ui16ProfileCount; ui16i++)
    {
        m_vpProfilesTable[ui16i] = std::move(m_vpProfilesTable[ui16i + 1]);
    }

    m_vpProfilesTable[m_ui16ProfileCount].reset();

    // Update profiles for online users
    if (ServerManager::m_bServerRunning)
    {
        for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
        {
            User* curUser = curUserPtr.get();

            if (curUser->m_i32Profile > ui16Profile)
            {
                curUser->m_i32Profile--;
            }
        }
    }

    // Update profiles for registered users
    for (const auto& curRegPtr : RegManager::m_Ptr->m_RegList)
    {
        if (curRegPtr->m_ui16Profile > ui16Profile)
        {
            curRegPtr->m_ui16Profile--;
        }
    }

    m_vpProfilesTable.shrink_to_fit();

    return true;
}
//---------------------------------------------------------------------------

ProfileItem* ProfileManager::CreateProfile(const char* sName)
{
    auto pNewProfile = std::make_unique<ProfileItem>();
    pNewProfile->m_sName = sName;

    m_ui16ProfileCount++;
    m_vpProfilesTable.push_back(std::move(pNewProfile));

    return m_vpProfilesTable[m_ui16ProfileCount - 1].get();
}
//---------------------------------------------------------------------------

void ProfileManager::MoveProfileDown(const uint16_t ui16Profile)
{
    std::swap(m_vpProfilesTable[ui16Profile], m_vpProfilesTable[ui16Profile + 1]);

    for (const auto& curRegPtr : RegManager::m_Ptr->m_RegList)
    {
        if (curRegPtr->m_ui16Profile == ui16Profile)
        {
            curRegPtr->m_ui16Profile++;
        }
        else if (curRegPtr->m_ui16Profile == ui16Profile + 1)
        {
            curRegPtr->m_ui16Profile--;
        }
    }

    if (!Users::m_Ptr)
    {
        return;
    }

    for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
    {
        User* curUser = curUserPtr.get();

        if (curUser->m_i32Profile == static_cast<int32_t>(ui16Profile))
        {
            curUser->m_i32Profile++;
        }
        else if (curUser->m_i32Profile == static_cast<int32_t>(ui16Profile + 1))
        {
            curUser->m_i32Profile--;
        }
    }
}
//---------------------------------------------------------------------------

void ProfileManager::MoveProfileUp(const uint16_t ui16Profile)
{
    std::swap(m_vpProfilesTable[ui16Profile - 1], m_vpProfilesTable[ui16Profile]);

    for (const auto& curRegPtr : RegManager::m_Ptr->m_RegList)
    {
        if (curRegPtr->m_ui16Profile == ui16Profile)
        {
            curRegPtr->m_ui16Profile--;
        }
        else if (curRegPtr->m_ui16Profile == ui16Profile - 1)
        {
            curRegPtr->m_ui16Profile++;
        }
    }

    if (!Users::m_Ptr)
    {
        return;
    }

    for (const auto& curUserPtr : Users::m_Ptr->m_UserList)
    {
        User* curUser = curUserPtr.get();

        if (curUser->m_i32Profile == static_cast<int32_t>(ui16Profile))
        {
            curUser->m_i32Profile--;
        }
        else if (curUser->m_i32Profile == static_cast<int32_t>(ui16Profile - 1))
        {
            curUser->m_i32Profile++;
        }
    }
}
//---------------------------------------------------------------------------

void ProfileManager::ChangeProfileName(const uint16_t ui16Profile, const char* sName, const size_t szLen)
{
    m_vpProfilesTable[ui16Profile]->m_sName.assign(sName, szLen);
}
//---------------------------------------------------------------------------

void ProfileManager::ChangeProfilePermission(uint16_t ui16Profile, size_t szId, bool bValue)
{
    m_vpProfilesTable[ui16Profile]->m_bPermissions[szId] = bValue;
}
//---------------------------------------------------------------------------
