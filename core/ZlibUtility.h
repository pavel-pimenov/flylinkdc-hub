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
#ifndef zlibutilityH
#define zlibutilityH
//---------------------------------------------------------------------------
#include <string_view>
#include <vector>

class ZlibUtility
{
private:
    std::vector<char> m_vZbuffer;

public:
    ZlibUtility(const ZlibUtility&) = delete;
    auto operator=(const ZlibUtility&) -> ZlibUtility& = delete;

    static std::unique_ptr<ZlibUtility> m_Ptr;

    ZlibUtility();
    ~ZlibUtility() = default;

    [[nodiscard]] auto CreateZPipe(std::string_view sInData, uint32_t& ui32OutDataLen) -> char*;

    void CreateZPipe(std::string_view sInData, std::vector<char>& vOutData, uint32_t& ui32OutDataLen, const char* sMetricPrefix = "ZPipe");
};
//---------------------------------------------------------------------------

#endif
