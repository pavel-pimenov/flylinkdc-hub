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
#ifndef ProfileManagerH
#define ProfileManagerH
//---------------------------------------------------------------------------
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct ProfileItem
{
    std::string m_sName;

    std::array<bool, 256> m_bPermissions{};

    ProfileItem();

    ProfileItem(const ProfileItem&) = delete;

    auto operator=(const ProfileItem&) -> ProfileItem& = delete;
};
//---------------------------------------------------------------------------

class ProfileManager
{
private:
    [[nodiscard]] auto CreateProfile(const char* sName) -> ProfileItem*;

    void Load();
    void LoadXML();

public:
    static std::unique_ptr<ProfileManager> m_Ptr;

    std::vector<std::unique_ptr<ProfileItem>> m_vpProfilesTable;

    uint16_t m_ui16ProfileCount = 0;

    enum ProfilePermissions : uint8_t
    {
        HASKEYICON,
        NODEFLOODGETNICKLIST,
        NODEFLOODMYINFO,
        NODEFLOODSEARCH,
        NODEFLOODPM,
        NODEFLOODMAINCHAT,
        MASSMSG,
        TOPIC,
        TEMP_BAN,
        REFRESHTXT,
        NOTAGCHECK,
        TEMP_UNBAN,
        DELREGUSER,
        ADDREGUSER,
        NOCHATLIMITS,
        NOMAXHUBCHECK,
        NOSLOTHUBRATIO,
        NOSLOTCHECK,
        NOSHARELIMIT,
        CLRPERMBAN,
        CLRTEMPBAN,
        GETINFO,
        GETBANLIST,
        RSTSCRIPTS,
        RSTHUB,
        TEMPOP,
        GAG,
        REDIRECT,
        BAN,
        KICK,
        DROP,
        ENTERFULLHUB,
        ENTERIFIPBAN,
        ALLOWEDOPCHAT,
        SENDALLUSERIP,
        RANGE_BAN,
        RANGE_UNBAN,
        RANGE_TBAN,
        RANGE_TUNBAN,
        GET_RANGE_BANS,
        CLR_RANGE_BANS,
        CLR_RANGE_TBANS,
        UNBAN,
        NOSEARCHLIMITS,
        SENDFULLMYINFOS,
        NOIPCHECK,
        CLOSE,
        NODEFLOODCTM,
        NODEFLOODRCTM,
        NODEFLOODSR,
        NODEFLOODRECV,
        NOCHATINTERVAL,
        NOPMINTERVAL,
        NOSEARCHINTERVAL,
        NOUSRSAMEIP,
        NORECONNTIME
    };

    ProfileManager();
    ~ProfileManager();

    [[nodiscard]] auto IsAllowed(const User* pUser, uint32_t ui32Option) const -> bool;
    [[nodiscard]] auto IsProfileAllowed(int32_t i32Profile, uint32_t ui32Option) const -> bool;
    [[nodiscard]] auto AddProfile(const char* sName) -> int32_t;
    [[nodiscard]] std::optional<uint16_t> GetProfileIndex(const char* sName) const;
    [[nodiscard]] auto RemoveProfileByName(const char* sName) -> int32_t;
    void MoveProfileDown(uint16_t ui16Profile);
    void MoveProfileUp(uint16_t ui16Profile);
    void ChangeProfileName(uint16_t ui16Profile, const char* sName, size_t szLen);
    void ChangeProfilePermission(uint16_t ui16Profile, size_t szId, bool bValue);
    void SaveProfiles();
    [[nodiscard]] auto RemoveProfile(uint16_t ui16Profile) -> bool;

    ProfileManager(const ProfileManager&) = delete;

    auto operator=(const ProfileManager&) -> ProfileManager& = delete;
};
//---------------------------------------------------------------------------

#endif
