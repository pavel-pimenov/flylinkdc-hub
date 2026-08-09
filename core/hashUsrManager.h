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
#ifndef hashUsrManagerH
#define hashUsrManagerH
//---------------------------------------------------------------------------
#include <string_view>
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------
#include <cstddef>
#include <cstdint>
#include <array>
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>

inline constexpr size_t IP_USR_HASH_TABLE_SIZE = 65536;

struct IpHashHash
{
    auto operator()(const std::array<uint8_t, 16>& a) const noexcept -> size_t
    {
        return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(a.data()), a.size()));
    }
};

struct StringHash
{
    using is_transparent = void;
    auto operator()(std::string_view sv) const noexcept -> size_t
    {
        return std::hash<std::string_view>{}(sv);
    }
    auto operator()(const std::string& s) const noexcept -> size_t
    {
        return std::hash<std::string>{}(s);
    }
};

struct StringEqual
{
    using is_transparent = void;
    auto operator()(std::string_view a, std::string_view b) const noexcept -> bool
    {
        return a == b;
    }
};
/*
#include <unordered_set>
#include <map>

struct CFlyIPCountUser
{
    std::unordered_set<User *> m_Users;
    uint16_t m_ui16Count;
    CFlyIPCountUser() : m_ui16Count(0) {}
};
        std::unordered_map<std::string, CFlyIPCountUser> m_IPCountTable;
*/
class HashManager
{
private:
    std::unordered_map<std::string, User*, StringHash, StringEqual> m_NickTable;

    struct IpTableItem
    {
        User* m_pFirstUser = nullptr;

        uint16_t m_ui16Count = 0;

        IpTableItem() = default;
    };

    std::unordered_map<std::array<uint8_t, 16>, IpTableItem, IpHashHash> m_IpTable;

public:
    static std::unique_ptr<HashManager> m_Ptr;

    HashManager(const HashManager&) = delete;
    auto operator=(const HashManager&) -> HashManager& = delete;

    HashManager();
    ~HashManager();

    [[nodiscard]] auto Add(User* pUser) -> bool;
    void Remove(User* pUser);

    [[nodiscard]] auto FindUser(std::string_view sNick) const -> User*;
    [[nodiscard]] auto FindUser(const User* pUser) const -> User*;
    [[nodiscard]] auto FindUser(const uint8_t* m_ui128IpHash) const -> User*;

    [[nodiscard]] auto GetUserIpCount(const User* pUser) const -> uint32_t;
    [[nodiscard]] auto GetMaxIpCount() const -> uint16_t;
};
//---------------------------------------------------------------------------

#endif
