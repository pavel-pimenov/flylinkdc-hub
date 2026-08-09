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
#ifndef hashBanManagerH
#define hashBanManagerH

#include "utility.h"
#include <list>
#include <array>
#include <cstdint>
#include <unordered_map>
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct BanItemBase
{
    time_t m_tTempBanExpire = 0;
    std::string m_sReason;
    std::string m_sBy;
    uint8_t m_ui8Bits = 0;

    BanItemBase() = default;
    virtual ~BanItemBase() = default;
    BanItemBase(const BanItemBase&) = delete;
    auto operator=(const BanItemBase&) -> BanItemBase& = delete;
};
struct BanItem : public BanItemBase
{
    std::string m_sNick;

    BanItem *m_pHashIpTablePrev = nullptr, *m_pHashIpTableNext = nullptr;

    uint32_t m_ui32NickHash = 0;

    std::array<uint8_t, 16> m_ui128IpHash = {};

    std::array<char, 40> m_sIp = {};

    void initIP(const User* u);
    void initIP(const char* pIP);
    BanItem() = default;
    ~BanItem() override = default;

    BanItem(const BanItem&) = delete;

    auto operator=(const BanItem&) -> BanItem& = delete;
};
//---------------------------------------------------------------------------

struct RangeBanItem : public BanItemBase
{

    Hash128 m_ui128FromIpHash, m_ui128ToIpHash;

    std::array<char, 40> m_sIpFrom = {}, m_sIpTo = {};

    RangeBanItem() = default;
    ~RangeBanItem() override = default;

    RangeBanItem(const RangeBanItem&) = delete;

    auto operator=(const RangeBanItem&) -> RangeBanItem& = delete;
};
//---------------------------------------------------------------------------

class BanManager
{
private:
    std::unordered_multimap<uint32_t, BanItem*> m_NickBanTable;

    // Элемент IP-таблицы банов: голова per-IP цепочки банов (pFirstBan +
    // BanItem::m_pHashIpTableNext/Prev). Сама цепочка банов с одним IP не тронута —
    // она используется снаружи (LuaBanManLib, HubCommands-AE).
    struct IpTableItem
    {
        BanItem* pFirstBan = nullptr;
    };

    // Хешер по 16-байтовому IP-хешу для unordered_map.
    struct IpHashHash
    {
        auto operator()(const std::array<uint8_t, 16>& rKey) const noexcept -> size_t
        {
            size_t h = 1469598103934665603ULL;
            for (const uint8_t b : rKey)
            {
                h = (h ^ b) * 1099511628211ULL;
            }
            return h;
        }
    };

    // IP-таблица банов: ключ — 16-байтовый IP-хеш, значение — голова per-IP цепочки.
    std::unordered_map<std::array<uint8_t, 16>, IpTableItem, IpHashHash> m_IpBanTable;

    [[nodiscard]] auto FindIpBanGeneric(const uint8_t* ui128IpHash, time_t acc_time, uint8_t ui8MatchBits) -> BanItem*;
    [[nodiscard]] auto FindNickBanGeneric(uint32_t ui32Hash, time_t acc_time, std::string_view sNick, uint8_t ui8MatchBits) -> BanItem*;
    [[nodiscard]] auto FindRangeBanGeneric(const uint8_t* ui128IpHash, time_t acc_time, uint8_t ui8MatchBits) -> RangeBanItem*;

    [[nodiscard]] auto FindNick(uint32_t ui32Hash, time_t acc_time, std::string_view sNick) -> BanItem*;
    [[nodiscard]] auto FindTempNick(uint32_t ui32Hash, time_t acc_time, std::string_view sNick) -> BanItem*;
    [[nodiscard]] auto FindPermNick(uint32_t ui32Hash, std::string_view sNick) -> BanItem*;

    uint32_t m_ui32SaveCalled = 0;

public:
    static std::unique_ptr<BanManager> m_Ptr;

    std::list<std::unique_ptr<BanItem>> m_TempBanList;
    std::list<std::unique_ptr<BanItem>> m_PermBanList;
    std::list<std::unique_ptr<RangeBanItem>> m_RangeBanList;

    enum BanBits : uint8_t
    {
        PERM = 0x1,
        TEMP = 0x2,
        FULL = 0x4,
        IP = 0x8,
        NICK = 0x10
    };

    BanManager(const BanManager&) = delete;
    auto operator=(const BanManager&) -> BanManager& = delete;

    BanManager();
    ~BanManager();

    [[nodiscard]] auto Add(BanItem* pBan) -> bool;
    [[nodiscard]] auto Add2Table(BanItem* pBan) -> bool;
    void Add2NickTable(BanItem* pBan);
    [[nodiscard]] auto Add2IpTable(BanItem* pBan) -> bool;
    void Rem(BanItem* pBan, bool bFromGui = false);
    void RemFromTable(BanItem* pBan);
    void RemFromNickTable(BanItem* pBan);
    void RemFromIpTable(BanItem* pBan);

    [[nodiscard]] auto Find(BanItem* pBan) -> BanItem*; // from gui
    void Remove(BanItem* pBan);                         // from gui

    void AddRange(std::unique_ptr<RangeBanItem> pRangeBan);
    void RemRange(RangeBanItem* pRangeBan, bool bFromGui = false);

    [[nodiscard]] auto FindRange(RangeBanItem* pRangeBan) -> RangeBanItem*; // from gui
    void RemoveRange(RangeBanItem* pRangeBan);                              // from gui

    [[nodiscard]] auto FindNick(User* pUser) -> BanItem*;
    [[nodiscard]] auto FindIP(User* pUser) -> BanItem*;
    [[nodiscard]] auto FindRange(User* pUser) -> RangeBanItem*;

    [[nodiscard]] auto FindFull(const uint8_t* ui128IpHash) -> BanItem*;
    [[nodiscard]] auto FindFull(const uint8_t* ui128IpHash, time_t acc_time) -> BanItem*;
    [[nodiscard]] auto FindFullRange(const uint8_t* ui128IpHash, time_t acc_time) -> RangeBanItem*;

    [[nodiscard]] auto FindNick(std::string_view sNick) -> BanItem*;

    [[nodiscard]] auto FindIP(const uint8_t* ui128IpHash, time_t acc_time) -> BanItem*;
    [[nodiscard]] auto FindRange(const uint8_t* ui128IpHash, time_t acc_time) -> RangeBanItem*;
    [[nodiscard]] auto FindRange(const uint8_t* ui128FromHash, const uint8_t* ui128ToHash, time_t acc_time) -> RangeBanItem*;

    [[nodiscard]] auto FindTempNick(std::string_view sNick) -> BanItem*;
    [[nodiscard]] auto FindTempIP(const uint8_t* ui128IpHash, time_t acc_time) -> BanItem*;

    [[nodiscard]] auto FindPermNick(std::string_view sNick) -> BanItem*;
    [[nodiscard]] auto FindPermIP(const uint8_t* ui128IpHash) -> BanItem*;

    void Load();
    void LoadXML();
    void Save(bool bForce = false);

    void ClearTemp();
    void ClearPerm();
    void ClearRange();
    void ClearTempRange();
    void ClearPermRange();

    [[nodiscard]] auto AddBanInternal(const char* sBy, BanItemBase* pBan) -> bool;
    void Ban(User* pUser, const char* sReason, const char* sBy, bool bFull);
    [[nodiscard]] auto BanIp(User* pUser, const char* sIp, const char* sReason, const char* sBy, bool bFull) -> char;
    [[nodiscard]] auto NickBan(User* pUser, const char* sNick, const char* sReason, const char* sBy) -> bool;

    void TempBan(User* pUser, const char* sReason, const char* sBy, uint32_t minutes, time_t expiretime, bool bFull);
    [[nodiscard]] auto TempBanIp(User* pUser, const char* sIp, const char* sReason, const char* sBy, uint32_t minutes, time_t expiretime, bool bFull) -> char;
    [[nodiscard]] auto NickTempBan(User* pUser, const char* sNick, const char* sReason, const char* sBy, uint32_t minutes, time_t expiretime) -> bool;

    [[nodiscard]] auto Unban(const char* sWhat) -> bool;
    [[nodiscard]] auto PermUnban(const char* sWhat) -> bool;
    [[nodiscard]] auto TempUnban(const char* sWhat) -> bool;

    void RemoveAllIP(const uint8_t* ui128IpHash);
    void RemovePermAllIP(const uint8_t* ui128IpHash);
    void RemoveTempAllIP(const uint8_t* ui128IpHash);

    [[nodiscard]] auto RangeBan(
        const char* sIpFrom, const uint8_t* ui128FromIpHash, const char* sIpTo, const uint8_t* ui128ToIpHash, const char* sReason, const char* sBy, bool bFull)
        -> bool;
    [[nodiscard]] auto RangeTempBan(const char* sIpFrom,
                                    const uint8_t* ui128FromIpHash,
                                    const char* sIpTo,
                                    const uint8_t* ui128ToIpHash,
                                    const char* sReason,
                                    const char* sBy,
                                    uint32_t ui32Minutes,
                                    time_t expiretime,
                                    bool bFull) -> bool;

    [[nodiscard]] auto RangeUnban(const uint8_t* ui128FromIpHash, const uint8_t* ui128ToIpHash) -> bool;
    [[nodiscard]] auto RangeUnban(const uint8_t* ui128FromIpHash, const uint8_t* ui128ToIpHash, unsigned char cType) -> bool;
};
//---------------------------------------------------------------------------

#endif
