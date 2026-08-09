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
#ifndef hashRegManagerH
#define hashRegManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct RegUser
{
    time_t m_tLastBadPass = 0;

    std::string m_sNick;

    std::vector<uint8_t> m_vPassData;

    uint32_t m_ui32Hash = 0;

    uint16_t m_ui16Profile = 0;

    uint8_t m_ui8BadPassCount = 0;

    bool m_bPassHash = false;

    RegUser() = default;
    ~RegUser() = default;

    [[nodiscard]] static auto
    CreateReg(const char* sRegNick, size_t szRegNickLen, const char* sRegPassword, size_t szRegPassLen, const uint8_t* ui8RegPassHash, uint16_t ui16RegProfile)
        -> RegUser*;
    [[nodiscard]] auto UpdatePassword(std::string_view sNewPass) -> bool;
    RegUser(const RegUser&) = delete;
    auto operator=(const RegUser&) -> RegUser& = delete;
};
//---------------------------------------------------------------------------

class RegManager
{
private:
    // Таблица регистраций: ключ — хеш ника, значение — non-owning указатель.
    // Владение RegUser остаётся в m_RegList (std::list<unique_ptr>).
    std::unordered_multimap<uint32_t, RegUser*> m_Table;

    uint8_t m_ui8SaveCalls = 0;

    void LoadXML();

public:
    static std::unique_ptr<RegManager> m_Ptr;

    std::list<std::unique_ptr<RegUser>> m_RegList;

    RegManager(const RegManager&) = delete;
    auto operator=(const RegManager&) -> RegManager& = delete;

    RegManager();
    ~RegManager();

    [[nodiscard]] auto AddNew(const char* sNick, const char* sPasswd, uint16_t iProfile) -> bool;

    void Add(RegUser* pReg);
    void Add2Table(RegUser* pReg);
    static void ChangeReg(RegUser* pReg, const char* sNewPasswd, uint16_t ui16NewProfile);
    void Delete(RegUser* pReg, bool bFromGui = false);
    void Rem(RegUser* pReg);
    void RemFromTable(RegUser* pReg);

    [[nodiscard]] auto Find(std::string_view sNick) const -> RegUser*;
    [[nodiscard]] auto Find(User* pUser) const -> RegUser*;
    [[nodiscard]] auto Find(uint32_t ui32Hash, std::string_view sNick) const -> RegUser*;

    void Load();
    void Save(bool bSaveOnChange = false, bool bSaveOnTime = false);

    void HashPasswords() const;

    void AddRegCmdLine();
};
//---------------------------------------------------------------------------

#endif
