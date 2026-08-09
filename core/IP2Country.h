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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef IP2CountryH
#define IP2CountryH
//----------------------------------------------------------------------------------------------------------------------------

#include <vector>

class IpP2Country
{
private:
    std::vector<uint32_t> m_ui32RangeFrom, m_ui32RangeTo;
    std::vector<uint8_t> m_ui8RangeCI, m_ui8IPv6RangeCI;
    std::vector<uint8_t> m_ui128IPv6RangeFrom, m_ui128IPv6RangeTo;

    void LoadIPv4();
    void LoadIPv6();

public:
    IpP2Country(const IpP2Country&) = delete;
    auto operator=(const IpP2Country&) -> IpP2Country& = delete;

    static std::unique_ptr<IpP2Country> m_Ptr;

    uint32_t m_ui32Count = 0, m_ui32IPv6Count = 0;

    IpP2Country();
    ~IpP2Country() = default;

    [[nodiscard]] auto Find(const uint8_t* ui128IpHash, bool bCountryName) const -> const char*;
    [[nodiscard]] uint8_t Find(const uint8_t* ui128IpHash) const;

    [[nodiscard]] static auto GetCountry(uint8_t ui8dx, bool bCountryName) -> const char*;
    [[nodiscard]] static auto GetCountryName(const char* sCode) -> const char*;

    void Reload();
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
