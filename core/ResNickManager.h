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
#ifndef ResNickManagerH
#define ResNickManagerH
//---------------------------------------------------------------------------
#include <list>
#include <string>
//---------------------------------------------------------------------------

class ReservedNicksManager
{
private:
    struct ReservedNick
    {
        std::string m_sNick;

        uint32_t m_ui32Hash = 0;

        bool m_bFromScript = false;

        ReservedNick() = default;
        ~ReservedNick() = default;

        [[nodiscard]] static std::unique_ptr<ReservedNick> CreateReservedNick(const char* sNewNick, uint32_t m_ui32NickHash);

        ReservedNick(const ReservedNick&) = delete;

        auto operator=(const ReservedNick&) -> ReservedNick& = delete;
    };

    std::list<std::unique_ptr<ReservedNick>> m_ReservedNicks;

    void Load();
    void LoadXML();

public:
    void Save() const;
    static std::unique_ptr<ReservedNicksManager> m_Ptr;

    ReservedNicksManager(const ReservedNicksManager&) = delete;
    auto operator=(const ReservedNicksManager&) -> ReservedNicksManager& = delete;

    ReservedNicksManager();
    ~ReservedNicksManager();

    [[nodiscard]] uint32_t GetCount() const
    {
        return static_cast<uint32_t>(m_ReservedNicks.size());
    }

    [[nodiscard]] auto CheckReserved(const char* sNick, uint32_t ui32Hash) const -> bool;
    void AddReservedNick(const char* sNick, bool bFromScript = false);
    void DelReservedNick(const char* sNick, bool bFromScript = false);
};
//---------------------------------------------------------------------------

#endif
