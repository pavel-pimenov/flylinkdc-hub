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
#include "stdinc.h"
#include <algorithm>
//---------------------------------------------------------------------------
#include "hashBanManager.h"
//---------------------------------------------------------------------------
#include "PXBReader.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include <tinyxml2.h>
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

std::unique_ptr<BanManager> BanManager::m_Ptr;
//---------------------------------------------------------------------------
namespace {
const char sPtokaXBans[] = "PtokaX Bans"; // NOLINT(modernize-avoid-c-arrays)
constexpr size_t g_szPtokaXBansLen = sizeof(sPtokaXBans) - 1;
const char sPtokaXRangeBans[] = "PtokaX RangeBans"; // NOLINT(modernize-avoid-c-arrays)
constexpr size_t g_szPtokaXRangeBansLen = sizeof(sPtokaXRangeBans) - 1;
const char sBanIds[] = "BT" // NOLINT(modernize-avoid-c-arrays)
                              "NI"
                              "NB"
                              "IP"
                              "IB"
                              "FB"
                              "RE"
                              "BY"
                              "EX";
constexpr size_t g_szBanIdsLen = sizeof(sBanIds) - 1;
const char sRangeBanIds[] = "BT" // NOLINT(modernize-avoid-c-arrays)
                                   "RF"
                                   "RT"
                                   "FB"
                                   "RE"
                                   "BY"
                                   "EX";
constexpr size_t g_szRangeBanIdsLen = sizeof(sRangeBanIds) - 1;
} // namespace
//---------------------------------------------------------------------------
// BanItemBase destructor is virtual = default in header

void BanItem::initIP(const char* pIP)
{
    SafeStrCopy(m_sIp, pIP);
}
//---------------------------------------------------------------------------

void BanItem::initIP(const User* u)
{
    SafeStrCopy(m_sIp, u->m_sIP.data());
    memcpy(m_ui128IpHash.data(), u->m_ui128IpHash.data(), m_ui128IpHash.size());
}
//---------------------------------------------------------------------------

// RangeBanItem destructor is = default in header

//---------------------------------------------------------------------------

BanManager::BanManager() = default;
//---------------------------------------------------------------------------

BanManager::~BanManager() = default;
//---------------------------------------------------------------------------

// Generic IP ban lookup. ui8MatchBits=0 → any ban. Otherwise returns first ban whose bits match.
BanItem* BanManager::FindIpBanGeneric(const uint8_t* ui128IpHash, time_t acc_time, uint8_t ui8MatchBits)
{
    std::array<uint8_t, 16> key;
    std::copy_n(ui128IpHash, 16, key.begin());

    const auto it = m_IpBanTable.find(key);
    if (it == m_IpBanTable.end())
    {
        return nullptr;
    }

    BanItem* fnd = nullptr;

    // Проходим по per-IP цепочке банов (BanItem::m_pHashIpTableNext).
    for (BanItem* ban = it->second.pFirstBan; ban;)
    {
        if (((ban->m_ui8Bits & TEMP) == TEMP) && acc_time >= ban->m_tTempBanExpire)
        {
            BanItem* next = ban->m_pHashIpTableNext;
            Rem(ban);
            std::unique_ptr<BanItem> guard(ban);
            ban = next;
            continue;
        }

        if (ui8MatchBits == 0)
        {
            return ban;
        }

        if ((ban->m_ui8Bits & ui8MatchBits) == ui8MatchBits)
        {
            return ban;
        }

        if (!fnd)
        {
            fnd = ban;
        }

        ban = ban->m_pHashIpTableNext;
    }

    return fnd;
}
//---------------------------------------------------------------------------

// Generic nick ban lookup. ui8MatchBits=0 → any ban. Otherwise first ban matching the bits.
BanItem* BanManager::FindNickBanGeneric(uint32_t ui32Hash, time_t acc_time, std::string_view sNick, uint8_t ui8MatchBits)
{
    const auto range = m_NickBanTable.equal_range(ui32Hash);
    for (auto it = range.first; it != range.second;)
    {
        BanItem* cur = it->second;

        if (!iequals(cur->m_sNick, sNick))
        {
            ++it;
            continue;
        }

        if (((cur->m_ui8Bits & TEMP) == TEMP) && acc_time >= cur->m_tTempBanExpire)
        {
            ++it;
            Rem(cur);
            std::unique_ptr<BanItem> guard(cur);
            continue;
        }

        if (ui8MatchBits == 0 || ((cur->m_ui8Bits & ui8MatchBits) == ui8MatchBits))
        {
            return cur;
        }

        ++it;
    }

    return nullptr;
}
//---------------------------------------------------------------------------

// Generic range ban lookup. ui8MatchBits=0 → any ban. Otherwise first matching the bits.
RangeBanItem* BanManager::FindRangeBanGeneric(const uint8_t* ui128IpHash, time_t acc_time, uint8_t ui8MatchBits)
{
    RangeBanItem* fnd = nullptr;

    auto it = m_RangeBanList.begin();
    while (it != m_RangeBanList.end())
    {
        RangeBanItem* cur = it->get();

        if (memcmp(cur->m_ui128FromIpHash, ui128IpHash, 16) > 0 || memcmp(cur->m_ui128ToIpHash, ui128IpHash, 16) < 0)
        {
            ++it;
            continue;
        }

        if (((cur->m_ui8Bits & TEMP) == TEMP) && acc_time >= cur->m_tTempBanExpire)
        {
            it = m_RangeBanList.erase(it);
            continue;
        }

        if (ui8MatchBits == 0)
        {
            return cur;
        }

        if ((cur->m_ui8Bits & ui8MatchBits) == ui8MatchBits)
        {
            return cur;
        }

        if (!fnd)
        {
            fnd = cur;
        }

        ++it;
    }

    return fnd;
}
//---------------------------------------------------------------------------

bool BanManager::Add(BanItem* pBan)
{
    if (!Add2Table(pBan))
    {
        return false;
    }

    if (((pBan->m_ui8Bits & PERM) == PERM))
    {
        m_PermBanList.emplace_back(pBan);
    }
    else
    {
        m_TempBanList.emplace_back(pBan);
    }

    return true;
}
//---------------------------------------------------------------------------

bool BanManager::Add2Table(BanItem* pBan)
{
    if (((pBan->m_ui8Bits & IP) == IP))
    {
        if (!Add2IpTable(pBan))
        {
            return false;
        }
    }

    if (((pBan->m_ui8Bits & NICK) == NICK))
    {
        Add2NickTable(pBan);
    }

    return true;
}
//---------------------------------------------------------------------------

void BanManager::Add2NickTable(BanItem* Ban)
{
    m_NickBanTable.emplace(Ban->m_ui32NickHash, Ban);
}
//---------------------------------------------------------------------------

bool BanManager::Add2IpTable(BanItem* Ban)
{
    std::array<uint8_t, 16> key = Ban->m_ui128IpHash;

    const auto it = m_IpBanTable.find(key);
    if (it == m_IpBanTable.end())
    {
        // Нового IP ещё нет — создаём запись, бан становится головой цепочки.
        IpTableItem item;
        item.pFirstBan = Ban;
        m_IpBanTable.emplace(key, item);
        return true;
    }

    // IP уже есть — вставляем бан в начало per-IP цепочки.
    it->second.pFirstBan->m_pHashIpTablePrev = Ban;
    Ban->m_pHashIpTableNext = it->second.pFirstBan;
    it->second.pFirstBan = Ban;

    return true;
}
//---------------------------------------------------------------------------

void BanManager::Rem(BanItem* Ban, const bool /*bFromGui = false*/)
{
    RemFromTable(Ban);

    std::list<std::unique_ptr<BanItem>>& list = ((Ban->m_ui8Bits & PERM) == PERM) ? m_PermBanList : m_TempBanList;

    for (auto it = list.begin(); it != list.end(); ++it)
    {
        if (it->get() == Ban)
        {
            list.erase(it);
            break;
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::RemFromTable(BanItem* Ban)
{
    if (((Ban->m_ui8Bits & IP) == IP))
    {
        RemFromIpTable(Ban);
    }

    if (((Ban->m_ui8Bits & NICK) == NICK))
    {
        RemFromNickTable(Ban);
    }
}
//---------------------------------------------------------------------------

void BanManager::RemFromNickTable(BanItem* Ban)
{
    const auto range = m_NickBanTable.equal_range(Ban->m_ui32NickHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second == Ban)
        {
            m_NickBanTable.erase(it);
            break;
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::RemFromIpTable(BanItem* Ban)
{
    if (!Ban->m_pHashIpTablePrev)
    {
        // Бан — голова per-IP цепочки.
        std::array<uint8_t, 16> key = Ban->m_ui128IpHash;

        const auto it = m_IpBanTable.find(key);
        if (it != m_IpBanTable.end())
        {
            if (!Ban->m_pHashIpTableNext)
            {
                // Единственный бан для этого IP — убираем запись целиком.
                m_IpBanTable.erase(it);
            }
            else
            {
                // Сдвигаем голову цепочки на следующий бан.
                Ban->m_pHashIpTableNext->m_pHashIpTablePrev = nullptr;
                it->second.pFirstBan = Ban->m_pHashIpTableNext;
            }
        }
    }
    else if (!Ban->m_pHashIpTableNext)
    {
        Ban->m_pHashIpTablePrev->m_pHashIpTableNext = nullptr;
    }
    else
    {
        Ban->m_pHashIpTablePrev->m_pHashIpTableNext = Ban->m_pHashIpTableNext;
        Ban->m_pHashIpTableNext->m_pHashIpTablePrev = Ban->m_pHashIpTablePrev;
    }

    Ban->m_pHashIpTablePrev = nullptr;
    Ban->m_pHashIpTableNext = nullptr;
}
//---------------------------------------------------------------------------

BanItem* BanManager::Find(BanItem* Ban)
{
    {
        time_t acc_time;
        time(&acc_time);

        auto it = m_TempBanList.begin();
        while (it != m_TempBanList.end())
        {
            BanItem* curBan = it->get();
            ++it;

            if (acc_time > curBan->m_tTempBanExpire)
            {
                Rem(curBan);
                std::unique_ptr<BanItem> guard(curBan);

                continue;
            }

            if (curBan == Ban)
            {
                return curBan;
            }
        }
    }

    {
        for (const auto& pBan : m_PermBanList)
        {
            BanItem* curBan = pBan.get();

            if (curBan == Ban)
            {
                return curBan;
            }
        }
    }

    return nullptr;
}
//---------------------------------------------------------------------------

void BanManager::Remove(BanItem* Ban)
{
    {
        time_t acc_time;
        time(&acc_time);

        auto it = m_TempBanList.begin();
        while (it != m_TempBanList.end())
        {
            BanItem* curBan = it->get();
            ++it;

            if (acc_time > curBan->m_tTempBanExpire)
            {
                Rem(curBan);
                std::unique_ptr<BanItem> guard(curBan);

                continue;
            }

            if (curBan == Ban)
            {
                Rem(Ban);
                std::unique_ptr<BanItem> guard(Ban);

                return;
            }
        }
    }

    {
        for (const auto& pBan : m_PermBanList)
        {
            BanItem* curBan = pBan.get();

            if (curBan == Ban) //-V1051 false positive: curBan is correct, not nextBan
            {
                Rem(Ban);
                std::unique_ptr<BanItem> guard(Ban);

                return;
            }
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::AddRange(std::unique_ptr<RangeBanItem> pRangeBan)
{
    m_RangeBanList.push_back(std::move(pRangeBan));
}
//---------------------------------------------------------------------------

void BanManager::RemRange(RangeBanItem* RangeBan, const bool /*bFromGui = false*/)
{
    for (auto it = m_RangeBanList.begin(); it != m_RangeBanList.end(); ++it)
    {
        if (it->get() == RangeBan)
        {
            m_RangeBanList.erase(it);
            return;
        }
    }
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(RangeBanItem* RangeBan)
{
    if (!m_RangeBanList.empty())
    {
        time_t acc_time;
        time(&acc_time);

        auto it = m_RangeBanList.begin();
        while (it != m_RangeBanList.end())
        {
            RangeBanItem* curBan = it->get();

            if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) && acc_time > curBan->m_tTempBanExpire)
            {
                it = m_RangeBanList.erase(it);
                continue;
            }

            if (curBan == RangeBan)
            {
                return curBan;
            }

            ++it;
        }
    }

    return nullptr;
}
//---------------------------------------------------------------------------

void BanManager::RemoveRange(RangeBanItem* RangeBan)
{
    if (!m_RangeBanList.empty())
    {
        time_t acc_time;
        time(&acc_time);

        auto it = m_RangeBanList.begin();
        while (it != m_RangeBanList.end())
        {
            RangeBanItem* curBan = it->get();

            if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) && acc_time > curBan->m_tTempBanExpire)
            {
                it = m_RangeBanList.erase(it);
                continue;
            }

            if (curBan == RangeBan)
            {
                m_RangeBanList.erase(it);
                return;
            }

            ++it;
        }
    }
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(User* pUser)
{
    time_t acc_time;
    time(&acc_time);

    return FindNickBanGeneric(pUser->m_ui32NickHash, acc_time, pUser->m_sNick.c_str(), 0);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindIP(User* pUser)
{
    time_t acc_time;
    time(&acc_time);

    return FindIpBanGeneric(pUser->m_ui128IpHash.data(), acc_time, 0);
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(User* pUser)
{
    time_t acc_time;
    time(&acc_time);

    return FindRangeBanGeneric(pUser->m_ui128IpHash.data(), acc_time, 0);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindFull(const uint8_t* m_ui128IpHash)
{
    time_t acc_time;
    time(&acc_time);

    return FindFull(m_ui128IpHash, acc_time);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindFull(const uint8_t* m_ui128IpHash, time_t acc_time)
{
    return FindIpBanGeneric(m_ui128IpHash, acc_time, FULL);
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindFullRange(const uint8_t* m_ui128IpHash, time_t acc_time)
{
    return FindRangeBanGeneric(m_ui128IpHash, acc_time, FULL);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(std::string_view sNick)
{
    const uint32_t hash = HashNick(sNick);

    time_t acc_time;
    time(&acc_time);

    return FindNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(const uint32_t ui32Hash, time_t acc_time, std::string_view sNick)
{
    return FindNickBanGeneric(ui32Hash, acc_time, sNick, 0);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindIP(const uint8_t* m_ui128IpHash, time_t acc_time)
{
    return FindIpBanGeneric(m_ui128IpHash, acc_time, 0);
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(const uint8_t* m_ui128IpHash, time_t acc_time)
{
    return FindRangeBanGeneric(m_ui128IpHash, acc_time, 0);
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(const uint8_t* ui128FromHash, const uint8_t* ui128ToHash, time_t acc_time)
{
    auto it = m_RangeBanList.begin();
    while (it != m_RangeBanList.end())
    {
        RangeBanItem* cur = it->get();

        if (memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0)
        {
            // PPK ... check if temban expired
            if (((cur->m_ui8Bits & TEMP) == TEMP))
            {
                if (acc_time >= cur->m_tTempBanExpire)
                {
                    it = m_RangeBanList.erase(it);
                    continue;
                }
            }

            return cur;
        }

        ++it;
    }

    return nullptr;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempNick(std::string_view sNick)
{
    const uint32_t hash = HashNick(sNick);

    time_t acc_time;
    time(&acc_time);

    return FindTempNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempNick(const uint32_t ui32Hash, time_t acc_time, std::string_view sNick)
{
    return FindNickBanGeneric(ui32Hash, acc_time, sNick, TEMP);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempIP(const uint8_t* m_ui128IpHash, time_t acc_time)
{
    return FindIpBanGeneric(m_ui128IpHash, acc_time, TEMP);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermNick(std::string_view sNick)
{
    const uint32_t hash = HashNick(sNick);

    return FindPermNick(hash, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermNick(const uint32_t ui32Hash, std::string_view sNick)
{
    return FindNickBanGeneric(ui32Hash, 0, sNick, PERM);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermIP(const uint8_t* m_ui128IpHash)
{
    return FindIpBanGeneric(m_ui128IpHash, 0, PERM);
}
//---------------------------------------------------------------------------

void BanManager::Load()
{
    if (!FileExist((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str()))
    {
        LoadXML();
        return;
    }

    PXBReader pxbBans;

    // Open regs file
    if (!pxbBans.OpenFileRead((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str(), 9))
    {
        return;
    }

    // Read file header
    std::array<uint16_t, 9> ui16Identificators = {};
    memcpy(ui16Identificators.data(), "FI", 2);
    memcpy(ui16Identificators.data() + 1, "FV", 2);

    if (!pxbBans.ReadNextItem(ui16Identificators.data(), 2))
    {
        return;
    }

    // Check header if we have correct file
    if (pxbBans.m_ui16ItemLengths[0] != g_szPtokaXBansLen || strncmp(static_cast<const char*>(pxbBans.m_pItemDatas[0]), sPtokaXBans, g_szPtokaXBansLen) != 0)
    {
        return;
    }

    {
        uint32_t ui32FileVersion;
        memcpy(&ui32FileVersion, pxbBans.m_pItemDatas[1], sizeof(ui32FileVersion));
        ui32FileVersion = ntohl(ui32FileVersion);

        if (ui32FileVersion < 1)
        {
            return;
        }
    }

    time_t tmAccTime;
    time(&tmAccTime);

    // "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX"
    memcpy(ui16Identificators.data(), sBanIds, g_szBanIdsLen);

    bool bSuccess = pxbBans.ReadNextItem(ui16Identificators.data(), 9);

    while (bSuccess)
    {
        auto pBan = std::make_unique<BanItem>();

        // Permanent or temporary ban?
        pBan->m_ui8Bits |= (static_cast<const char*>(pxbBans.m_pItemDatas[0])[0] == '0' ? PERM : TEMP);

        // Do we have some nick?
        if (pxbBans.m_ui16ItemLengths[1] != 0)
        {
            if (pxbBans.m_ui16ItemLengths[1] > 64)
            {
                LogDbgErr("[ERR] sNick too long %hu in BanManager::Load", pxbBans.m_ui16ItemLengths[1]);

                exit(EXIT_FAILURE);
            }
            pBan->m_sNick.assign(static_cast<const char*>(pxbBans.m_pItemDatas[1]), pxbBans.m_ui16ItemLengths[1]);
            pBan->m_ui32NickHash = HashNick(std::string_view(pBan->m_sNick.c_str(), pxbBans.m_ui16ItemLengths[1]));

            // it is nick ban?
            if (static_cast<const char*>(pxbBans.m_pItemDatas[2])[0] != '0')
            {
                pBan->m_ui8Bits |= NICK;
            }
        }

        // Do we have some IP address?
        if (pxbBans.m_ui16ItemLengths[3] != 0)
        {
            in6_addr ipAddr;
            memcpy(&ipAddr, pxbBans.m_pItemDatas[3], sizeof(in6_addr));
            if (IN6_IS_ADDR_UNSPECIFIED(&ipAddr) == 0)
            {
                if (pxbBans.m_ui16ItemLengths[3] != 16)
                {
                    LogDbgErr("[ERR] Ban IP address have incorrect length %hu in BanManager::Load", pxbBans.m_ui16ItemLengths[3]);

                    exit(EXIT_FAILURE);
                }

                memcpy(pBan->m_ui128IpHash.data(), pxbBans.m_pItemDatas[3], 16);

                if (IsV4Mapped(pBan->m_ui128IpHash.data()))
                {
                    in_addr ipv4addr;
                    memcpy(&ipv4addr, pBan->m_ui128IpHash.data() + 12, 4);
                    inet_ntop(AF_INET, &ipv4addr, pBan->m_sIp.data(), pBan->m_sIp.size());
                }
                else
                {
                    in6_addr ip6{};
                    memcpy(&ip6, pBan->m_ui128IpHash.data(), 16);
                    inet_ntop(AF_INET6, &ip6, pBan->m_sIp.data(), 40);
                }

                // it is IP ban?
                if (static_cast<const char*>(pxbBans.m_pItemDatas[4])[0] != '0')
                {
                    pBan->m_ui8Bits |= IP;
                }

                // it is full ban ?
                if (static_cast<const char*>(pxbBans.m_pItemDatas[5])[0] != '0')
                {
                    pBan->m_ui8Bits |= FULL;
                }
            }
        }

        // Do we have reason?
        if (pxbBans.m_ui16ItemLengths[6] != 0)
        {
            if (pxbBans.m_ui16ItemLengths[6] > 511)
            {
                pxbBans.m_ui16ItemLengths[6] = 511;
            }

            pBan->m_sReason.assign(static_cast<const char*>(pxbBans.m_pItemDatas[6]), pxbBans.m_ui16ItemLengths[6]);
        }

        // Do we have who created ban?
        if (pxbBans.m_ui16ItemLengths[7] != 0)
        {
            if (pxbBans.m_ui16ItemLengths[7] > 64)
            {
                pxbBans.m_ui16ItemLengths[7] = 64;
            }

            pBan->m_sBy.assign(static_cast<const char*>(pxbBans.m_pItemDatas[7]), pxbBans.m_ui16ItemLengths[7]);
        }

        // it is temporary ban?
        if (((pBan->m_ui8Bits & TEMP) == TEMP))
        {
            if (pxbBans.m_ui16ItemLengths[8] != 8)
            {
                LogDbgErr("[ERR] Temp ban expire time have incorrect length %hu in BanManager::Load", pxbBans.m_ui16ItemLengths[8]);

                exit(EXIT_FAILURE);
            }
            else
            {
                // Temporary ban expiration datetime
                uint64_t ui64Expire;
                memcpy(&ui64Expire, pxbBans.m_pItemDatas[8], sizeof(ui64Expire));
                pBan->m_tTempBanExpire = static_cast<time_t>(be64toh(ui64Expire));

                if (tmAccTime >= pBan->m_tTempBanExpire)
                {
                    // Expired temp ban — pBan destroyed automatically by unique_ptr
                }
                else
                {
                    if (!Add(pBan.release()))
                    {
                        LogDbg("%s [ERR] Add ban failed in BanManager::Load\n");

                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        else
        {
            if (!Add(pBan.release()))
            {
                LogDbg("%s [ERR] Add2 ban failed in BanManager::Load\n");

                exit(EXIT_FAILURE);
            }
        }

        bSuccess = pxbBans.ReadNextItem(ui16Identificators.data(), 9);
    }

    PXBReader pxbRangeBans;

    // Open regs file
    if (!pxbRangeBans.OpenFileRead((ServerManager::m_sPath + "/cfg/RangeBans.pxb").c_str(), 7))
    {
        return;
    }

    // Read file header
    memcpy(ui16Identificators.data(), "FI", 2);
    memcpy(ui16Identificators.data() + 1, "FV", 2);

    if (!pxbRangeBans.ReadNextItem(ui16Identificators.data(), 2))
    {
        return;
    }

    // Check header if we have correct file
    if (pxbRangeBans.m_ui16ItemLengths[0] != g_szPtokaXRangeBansLen ||
        strncmp(static_cast<const char*>(pxbRangeBans.m_pItemDatas[0]), sPtokaXRangeBans, g_szPtokaXRangeBansLen) != 0)
    {
        return;
    }

    {
        uint32_t ui32FileVersion;
        memcpy(&ui32FileVersion, pxbRangeBans.m_pItemDatas[1], sizeof(ui32FileVersion));
        ui32FileVersion = ntohl(ui32FileVersion);

        if (ui32FileVersion < 1)
        {
            return;
        }
    }

    // // "BT" "RF" "RT" "FB" "RE" "BY" "EX";
    memcpy(ui16Identificators.data(), sRangeBanIds, g_szRangeBanIdsLen);

    bSuccess = pxbRangeBans.ReadNextItem(ui16Identificators.data(), 7);

    while (bSuccess)
    {
        auto pRangeBan = std::make_unique<RangeBanItem>();

        // Permanent or temporary ban?
        pRangeBan->m_ui8Bits |= (static_cast<const char*>(pxbRangeBans.m_pItemDatas[0])[0] == '0' ? PERM : TEMP);

        // Do we have first IP address?
        if (pxbRangeBans.m_ui16ItemLengths[1] != 16)
        {
            LogDbgErr("[ERR] Range Ban first IP address have incorrect length %hu in BanManager::Load", pxbBans.m_ui16ItemLengths[1]);

            exit(EXIT_FAILURE);
        }

        memcpy(pRangeBan->m_ui128FromIpHash, pxbRangeBans.m_pItemDatas[1], 16);

        in6_addr fromAddr{};
        memcpy(&fromAddr, pRangeBan->m_ui128FromIpHash.data(), 16);
        if (IN6_IS_ADDR_V4MAPPED(&fromAddr))
        {
            in_addr ipv4addr;
            memcpy(&ipv4addr, pRangeBan->m_ui128FromIpHash + 12, 4);
            inet_ntop(AF_INET, &ipv4addr, pRangeBan->m_sIpFrom.data(), pRangeBan->m_sIpFrom.size());
        }
        else
        {
            in6_addr ip6from{};
            memcpy(&ip6from, pRangeBan->m_ui128FromIpHash.data(), 16);
            inet_ntop(AF_INET6, &ip6from, pRangeBan->m_sIpFrom.data(), 40);
        }

        // Do we have second IP address?
        if (pxbRangeBans.m_ui16ItemLengths[2] != 16)
        {
            LogDbgErr("[ERR] Range Ban second IP address have incorrect length %hu in BanManager::Load", pxbBans.m_ui16ItemLengths[2]);

            exit(EXIT_FAILURE);
        }

        memcpy(pRangeBan->m_ui128ToIpHash, pxbRangeBans.m_pItemDatas[2], 16);

        in6_addr toAddr{};
        memcpy(&toAddr, pRangeBan->m_ui128ToIpHash.data(), 16);
        if (IN6_IS_ADDR_V4MAPPED(&toAddr))
        {
            in_addr ipv4addr;
            memcpy(&ipv4addr, pRangeBan->m_ui128ToIpHash + 12, 4);
            inet_ntop(AF_INET, &ipv4addr, pRangeBan->m_sIpTo.data(), pRangeBan->m_sIpTo.size());
        }
        else
        {
            in6_addr ip6to{};
            memcpy(&ip6to, pRangeBan->m_ui128ToIpHash.data(), 16);
            inet_ntop(AF_INET6, &ip6to, pRangeBan->m_sIpTo.data(), 40);
        }

        // it is full ban ?
        if (static_cast<const char*>(pxbRangeBans.m_pItemDatas[3])[0] != '0')
        {
            pRangeBan->m_ui8Bits |= FULL;
        }

        // Do we have reason?
        if (pxbRangeBans.m_ui16ItemLengths[4] != 0)
        {
            if (pxbRangeBans.m_ui16ItemLengths[4] > 511)
            {
                pxbRangeBans.m_ui16ItemLengths[4] = 511;
            }

            pRangeBan->m_sReason.assign(static_cast<const char*>(pxbRangeBans.m_pItemDatas[4]), pxbRangeBans.m_ui16ItemLengths[4]);
        }

        // Do we have who created ban?
        if (pxbRangeBans.m_ui16ItemLengths[5] != 0)
        {
            if (pxbRangeBans.m_ui16ItemLengths[5] > 64)
            {
                pxbRangeBans.m_ui16ItemLengths[5] = 64;
            }

            pRangeBan->m_sBy.assign(static_cast<const char*>(pxbRangeBans.m_pItemDatas[5]), pxbRangeBans.m_ui16ItemLengths[5]);
        }

        // it is temporary ban?
        if (((pRangeBan->m_ui8Bits & TEMP) == TEMP))
        {
            if (pxbRangeBans.m_ui16ItemLengths[6] != 8)
            {
                LogDbgErr("[ERR] Temp range ban expire time have incorrect lenght %hu in BanManager::Load", pxbRangeBans.m_ui16ItemLengths[6]);

                exit(EXIT_FAILURE);
            }
            else
            {
                // Temporary ban expiration datetime
                uint64_t ui64Expire;
                memcpy(&ui64Expire, pxbRangeBans.m_pItemDatas[6], sizeof(ui64Expire));
                pRangeBan->m_tTempBanExpire = static_cast<time_t>(be64toh(ui64Expire));

                if (tmAccTime >= pRangeBan->m_tTempBanExpire)
                {
                    // Expired temp range ban — pRangeBan destroyed automatically by unique_ptr
                }
                else
                {
                    AddRange(std::move(pRangeBan));
                }
            }
        }
        else
        {
            AddRange(std::move(pRangeBan));
        }

        bSuccess = pxbRangeBans.ReadNextItem(ui16Identificators.data(), 7);
    }
}
//---------------------------------------------------------------------------

void BanManager::LoadXML()
{
    double dVer;

    tinyxml2::XMLDocument doc;
    if (LoadXmlConfig(doc, "BanList.xml"))
    {
        tinyxml2::XMLHandle cfg(&doc);
        tinyxml2::XMLElement* banlist = cfg.FirstChildElement("BanList").ToElement();
        if (banlist)
        {
            if (banlist->QueryDoubleAttribute("version", &dVer) != tinyxml2::XML_SUCCESS)
            {
                return;
            }

            time_t acc_time;
            time(&acc_time);

            tinyxml2::XMLElement* bans = banlist->FirstChildElement("Bans");
            if (bans)
            {
                tinyxml2::XMLElement* child = bans->FirstChildElement();
                while (child)
                {
                    int iType = 0;
                    if (!XmlGetRequiredInt(child, "Type", iType))
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }
                    const char type = iType != 0 ? 1 : 0;

                    const char* ip = XmlGetOptionalText(child, "IP");
                    const char* nick = XmlGetOptionalText(child, "Nick");
                    const char* reason = XmlGetOptionalText(child, "Reason");
                    const char* by = XmlGetOptionalText(child, "By");

                    int iNickBan = 0;
                    if (!XmlGetRequiredInt(child, "NickBan", iNickBan))
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }
                    const bool nickban = (iNickBan != 0);

                    int iIpBan = 0;
                    if (!XmlGetRequiredInt(child, "IpBan", iIpBan))
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }
                    const bool ipban = (iIpBan != 0);

                    int iFullIpBan = 0;
                    if (!XmlGetRequiredInt(child, "FullIpBan", iFullIpBan))
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }
                    const bool fullipban = (iFullIpBan != 0);

                    auto Ban = std::make_unique<BanItem>();

                    if (type == 0)
                    {
                        Ban->m_ui8Bits |= PERM;
                    }
                    else
                    {
                        Ban->m_ui8Bits |= TEMP;
                    }

                    // PPK ... ipban
                    if (ipban)
                    {
                        if (ip && HashIP(ip, Ban->m_ui128IpHash.data()))
                        {
                            SafeStrCopy(Ban->m_sIp, ip);
                            Ban->m_ui8Bits |= IP;

                            if (fullipban)
                            {
                                Ban->m_ui8Bits |= FULL;
                            }
                        }
                        else
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }
                    }

                    // PPK ... nickban
                    if (nickban)
                    {
                        if (nick)
                        {
                            const size_t szNickLen = strlen(nick);
                            Ban->m_sNick.assign(nick, szNickLen);
                            Ban->m_ui32NickHash = HashNick(Ban->m_sNick);
                            Ban->m_ui8Bits |= NICK;
                        }
                        else
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }
                    }

                    if (reason)
                    {
                        const auto szReasonLen = std::min(strlen(reason), size_t{255});
                        Ban->m_sReason.assign(reason, szReasonLen);
                    }

                    if (by)
                    {
                        const auto szByLen = std::min(strlen(by), size_t{63});
                        Ban->m_sBy.assign(by, szByLen);
                    }

                    // PPK ... temp ban
                    if (((Ban->m_ui8Bits & TEMP) == TEMP))
                    {
                        tinyxml2::XMLElement* expireElem = child->FirstChildElement("Expire");
                        if (!expireElem)
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }

                        const char* sExpire = expireElem->GetText();

                        if (!sExpire)
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }

                        time_t expire = 0;
                        std::from_chars(sExpire, sExpire + strlen(sExpire), expire);

                        if (acc_time > expire)
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }

                        // PPK ... temp ban expiration
                        Ban->m_tTempBanExpire = expire;
                    }

                    if (fullipban)
                    {
                        Ban->m_ui8Bits |= FULL;
                    }

                    if (!Add(Ban.release()))
                    {
                        LogDbg("[ERR] BanManager::LoadXML Add ban failed");
                        exit(EXIT_FAILURE);
                    }
                    child = child->NextSiblingElement();
                }
            }

            tinyxml2::XMLElement* rangebans = banlist->FirstChildElement("RangeBans");
            if (rangebans)
            {
                tinyxml2::XMLElement* child = rangebans->FirstChildElement();
                while (child)
                {
                    int iType = 0;
                    if (!XmlGetRequiredInt(child, "Type", iType))
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }
                    const char type = iType != 0 ? 1 : 0;

                    const char* ipfrom = XmlGetRequiredText(child, "IpFrom");
                    if (!ipfrom)
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }

                    const char* ipto = XmlGetRequiredText(child, "IpTo");
                    if (!ipto)
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }

                    const char* reason = XmlGetOptionalText(child, "Reason");
                    const char* by = XmlGetOptionalText(child, "By");

                    int iFullIpBan = 0;
                    if (!XmlGetRequiredInt(child, "FullIpBan", iFullIpBan))
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }
                    const bool fullipban = (iFullIpBan != 0);

                    auto RangeBan = std::make_unique<RangeBanItem>();

                    if (type == 0)
                    {
                        RangeBan->m_ui8Bits |= PERM;
                    }
                    else
                    {
                        RangeBan->m_ui8Bits |= TEMP;
                    }

                    // PPK ... fromip
                    if (HashIP(ipfrom, RangeBan->m_ui128FromIpHash))
                    {
                        SafeStrCopy(RangeBan->m_sIpFrom, ipfrom);
                    }
                    else
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }

                    // PPK ... toip
                    if (HashIP(ipto, RangeBan->m_ui128ToIpHash) && memcmp(RangeBan->m_ui128ToIpHash, RangeBan->m_ui128FromIpHash, 16) > 0)
                    {
                        SafeStrCopy(RangeBan->m_sIpTo, ipto);
                    }
                    else
                    {
                        child = child->NextSiblingElement();
                        continue;
                    }

                    if (reason)
                    {
                        const auto szReasonLen = std::min(strlen(reason), size_t{255});
                        RangeBan->m_sReason.assign(reason, szReasonLen);
                    }

                    if (by)
                    {
                        const auto szByLen = std::min(strlen(by), size_t{63});
                        RangeBan->m_sBy.assign(by, szByLen);
                    }

                    // PPK ... temp ban
                    if (((RangeBan->m_ui8Bits & TEMP) == TEMP))
                    {
                        tinyxml2::XMLElement* expireElem = child->FirstChildElement("Expire");
                        if (!expireElem)
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }

                        const char* sExpire = expireElem->GetText();
                        if (!sExpire)
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }

                        time_t expire = 0;
                        std::from_chars(sExpire, sExpire + strlen(sExpire), expire);

                        if (acc_time > expire)
                        {
                            child = child->NextSiblingElement();
                            continue;
                        }

                        // PPK ... temp ban expiration
                        RangeBan->m_tTempBanExpire = expire;
                    }

                    if (fullipban)
                    {
                        RangeBan->m_ui8Bits |= FULL;
                    }

                    AddRange(std::move(RangeBan));
                    child = child->NextSiblingElement();
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::Save(bool bForce /* = false*/)
{
    if (!bForce)
    {
        // we don't want waste resources with save after every change in bans
        if (m_ui32SaveCalled < 100)
        {
            m_ui32SaveCalled++;
            return;
        }
    }

    m_ui32SaveCalled = 0;

    PXBReader pxbBans;

    // Open bans file
    if (!pxbBans.OpenFileSave((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str(), 9))
    {
        return;
    }

    // Write file header
    pxbBans.m_sItemIdentifiers[0] = 'F';
    pxbBans.m_sItemIdentifiers[1] = 'I';
    pxbBans.m_ui16ItemLengths[0] = static_cast<uint16_t>(g_szPtokaXBansLen);
    pxbBans.m_pItemDatas[0] = sPtokaXBans;
    pxbBans.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbBans.m_sItemIdentifiers[2] = 'F';
    pxbBans.m_sItemIdentifiers[3] = 'V';
    pxbBans.m_ui16ItemLengths[1] = 4;
    const uint32_t ui32Version = 1;
    pxbBans.m_pItemDatas[1] = &ui32Version;
    pxbBans.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if (!pxbBans.WriteNextItem(g_szPtokaXBansLen + 4, 2))
    {
        return;
    }

    // "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX"
    memcpy(pxbBans.m_sItemIdentifiers.data(), sBanIds, g_szBanIdsLen);

    pxbBans.m_ui8ItemValues[0] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[1] = PXBReader::PXB_STRING;
    pxbBans.m_ui8ItemValues[2] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[3] = PXBReader::PXB_STRING;
    pxbBans.m_ui8ItemValues[4] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[5] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[6] = PXBReader::PXB_STRING;
    pxbBans.m_ui8ItemValues[7] = PXBReader::PXB_STRING;
    pxbBans.m_ui8ItemValues[8] = PXBReader::PXB_EIGHT_BYTES;

    uint64_t ui64TempBanExpire = 0;

    {
        for (const auto& pBanPtr : m_TempBanList)
        {
            BanItem* pCur = pBanPtr.get();

            pxbBans.m_ui16ItemLengths[0] = 1;
            pxbBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) ? nullptr : &PXBReader::s_TrueSentinel);

            pxbBans.m_ui16ItemLengths[1] = pCur->m_sNick.empty() ? 0 : static_cast<uint16_t>(pCur->m_sNick.size());
            pxbBans.m_pItemDatas[1] = pCur->m_sNick.empty() ? "" : pCur->m_sNick.c_str();

            pxbBans.m_ui16ItemLengths[2] = 1;
            pxbBans.m_pItemDatas[2] = (((pCur->m_ui8Bits & NICK) == NICK) ? &PXBReader::s_TrueSentinel : nullptr);

            pxbBans.m_ui16ItemLengths[3] = 16;
            pxbBans.m_pItemDatas[3] = pCur->m_ui128IpHash.data();

            pxbBans.m_ui16ItemLengths[4] = 1;
            pxbBans.m_pItemDatas[4] = (((pCur->m_ui8Bits & IP) == IP) ? &PXBReader::s_TrueSentinel : nullptr);

            pxbBans.m_ui16ItemLengths[5] = 1;
            pxbBans.m_pItemDatas[5] = (((pCur->m_ui8Bits & FULL) == FULL) ? &PXBReader::s_TrueSentinel : nullptr);

            pxbBans.m_ui16ItemLengths[6] = pCur->m_sReason.empty() ? 0 : static_cast<uint16_t>(pCur->m_sReason.size());
            pxbBans.m_pItemDatas[6] = pCur->m_sReason.empty() ? "" : pCur->m_sReason.c_str();

            pxbBans.m_ui16ItemLengths[7] = pCur->m_sBy.empty() ? 0 : static_cast<uint16_t>(pCur->m_sBy.size());
            pxbBans.m_pItemDatas[7] = pCur->m_sBy.empty() ? "" : pCur->m_sBy.c_str();

            pxbBans.m_ui16ItemLengths[8] = 8;
            ui64TempBanExpire = static_cast<uint64_t>(pCur->m_tTempBanExpire);
            pxbBans.m_pItemDatas[8] = &ui64TempBanExpire;

            if (!pxbBans.WriteNextItem(pxbBans.m_ui16ItemLengths[0] + pxbBans.m_ui16ItemLengths[1] + pxbBans.m_ui16ItemLengths[2] +
                                           pxbBans.m_ui16ItemLengths[3] + pxbBans.m_ui16ItemLengths[4] + pxbBans.m_ui16ItemLengths[5] +
                                           pxbBans.m_ui16ItemLengths[6] + pxbBans.m_ui16ItemLengths[7] + pxbBans.m_ui16ItemLengths[8],
                                       9))
            {
                break;
            }
        }
    }

    {
        for (const auto& pBanPtr : m_PermBanList)
        {
            BanItem* pCur = pBanPtr.get();

            pxbBans.m_ui16ItemLengths[0] = 1;
            pxbBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) ? nullptr : &PXBReader::s_TrueSentinel);

            pxbBans.m_ui16ItemLengths[1] = pCur->m_sNick.empty() ? 0 : static_cast<uint16_t>(pCur->m_sNick.size());
            pxbBans.m_pItemDatas[1] = pCur->m_sNick.empty() ? "" : pCur->m_sNick.c_str();

            pxbBans.m_ui16ItemLengths[2] = 1;
            pxbBans.m_pItemDatas[2] = (((pCur->m_ui8Bits & NICK) == NICK) ? &PXBReader::s_TrueSentinel : nullptr);

            pxbBans.m_ui16ItemLengths[3] = 16;
            pxbBans.m_pItemDatas[3] = pCur->m_ui128IpHash.data();

            pxbBans.m_ui16ItemLengths[4] = 1;
            pxbBans.m_pItemDatas[4] = (((pCur->m_ui8Bits & IP) == IP) ? &PXBReader::s_TrueSentinel : nullptr);

            pxbBans.m_ui16ItemLengths[5] = 1;
            pxbBans.m_pItemDatas[5] = (((pCur->m_ui8Bits & FULL) == FULL) ? &PXBReader::s_TrueSentinel : nullptr);

            pxbBans.m_ui16ItemLengths[6] = pCur->m_sReason.empty() ? 0 : static_cast<uint16_t>(pCur->m_sReason.size());
            pxbBans.m_pItemDatas[6] = pCur->m_sReason.empty() ? "" : pCur->m_sReason.c_str();

            pxbBans.m_ui16ItemLengths[7] = pCur->m_sBy.empty() ? 0 : static_cast<uint16_t>(pCur->m_sBy.size());
            pxbBans.m_pItemDatas[7] = pCur->m_sBy.empty() ? "" : pCur->m_sBy.c_str();

            pxbBans.m_ui16ItemLengths[8] = 8;
            ui64TempBanExpire = static_cast<uint64_t>(pCur->m_tTempBanExpire);
            pxbBans.m_pItemDatas[8] = &ui64TempBanExpire;

            if (!pxbBans.WriteNextItem(pxbBans.m_ui16ItemLengths[0] + pxbBans.m_ui16ItemLengths[1] + pxbBans.m_ui16ItemLengths[2] +
                                           pxbBans.m_ui16ItemLengths[3] + pxbBans.m_ui16ItemLengths[4] + pxbBans.m_ui16ItemLengths[5] +
                                           pxbBans.m_ui16ItemLengths[6] + pxbBans.m_ui16ItemLengths[7] + pxbBans.m_ui16ItemLengths[8],
                                       9))
            {
                break;
            }
        }
    }

    pxbBans.WriteRemaining();

    PXBReader pxbRangeBans;

    // Open range bans file
    if (!pxbRangeBans.OpenFileSave((ServerManager::m_sPath + "/cfg/RangeBans.pxb").c_str(), 7))
    {
        return;
    }

    // Write file header
    pxbRangeBans.m_sItemIdentifiers[0] = 'F';
    pxbRangeBans.m_sItemIdentifiers[1] = 'I';
    pxbRangeBans.m_ui16ItemLengths[0] = static_cast<uint16_t>(g_szPtokaXRangeBansLen);
    pxbRangeBans.m_pItemDatas[0] = sPtokaXRangeBans;
    pxbRangeBans.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbRangeBans.m_sItemIdentifiers[2] = 'F';
    pxbRangeBans.m_sItemIdentifiers[3] = 'V';
    pxbRangeBans.m_ui16ItemLengths[1] = 4;
    pxbRangeBans.m_pItemDatas[1] = &ui32Version;
    pxbRangeBans.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if (!pxbRangeBans.WriteNextItem(g_szPtokaXRangeBansLen + 4, 2))
    {
        return;
    }

    // "BT" "RF" "RT" "FB" "RE" "BY" "EX"
    memcpy(pxbRangeBans.m_sItemIdentifiers.data(), sRangeBanIds, g_szRangeBanIdsLen);

    pxbRangeBans.m_ui8ItemValues[0] = PXBReader::PXB_BYTE;
    pxbRangeBans.m_ui8ItemValues[1] = PXBReader::PXB_STRING;
    pxbRangeBans.m_ui8ItemValues[2] = PXBReader::PXB_STRING;
    pxbRangeBans.m_ui8ItemValues[3] = PXBReader::PXB_BYTE;
    pxbRangeBans.m_ui8ItemValues[4] = PXBReader::PXB_STRING;
    pxbRangeBans.m_ui8ItemValues[5] = PXBReader::PXB_STRING;
    pxbRangeBans.m_ui8ItemValues[6] = PXBReader::PXB_EIGHT_BYTES;

    if (!m_RangeBanList.empty())
    {
        for (const auto& pCur : m_RangeBanList)
        {
            pxbRangeBans.m_ui16ItemLengths[0] = 1;
            pxbRangeBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) ? nullptr : &PXBReader::s_TrueSentinel);

            pxbRangeBans.m_ui16ItemLengths[1] = 16;
            pxbRangeBans.m_pItemDatas[1] = pCur->m_ui128FromIpHash.data();

            pxbRangeBans.m_ui16ItemLengths[2] = 16;
            pxbRangeBans.m_pItemDatas[2] = pCur->m_ui128ToIpHash.data();

            pxbRangeBans.m_ui16ItemLengths[3] = 1;
            pxbRangeBans.m_pItemDatas[3] = (((pCur->m_ui8Bits & FULL) == FULL) ? &PXBReader::s_TrueSentinel : nullptr);

            pxbRangeBans.m_ui16ItemLengths[4] = pCur->m_sReason.empty() ? 0 : static_cast<uint16_t>(pCur->m_sReason.size());
            pxbRangeBans.m_pItemDatas[4] = pCur->m_sReason.empty() ? "" : pCur->m_sReason.c_str();

            pxbRangeBans.m_ui16ItemLengths[5] = pCur->m_sBy.empty() ? 0 : static_cast<uint16_t>(pCur->m_sBy.size());
            pxbRangeBans.m_pItemDatas[5] = pCur->m_sBy.empty() ? "" : pCur->m_sBy.c_str();

            pxbRangeBans.m_ui16ItemLengths[6] = 8;
            ui64TempBanExpire = static_cast<uint64_t>(pCur->m_tTempBanExpire);
            pxbRangeBans.m_pItemDatas[6] = &ui64TempBanExpire;

            if (!pxbRangeBans.WriteNextItem(pxbRangeBans.m_ui16ItemLengths[0] + pxbRangeBans.m_ui16ItemLengths[1] + pxbRangeBans.m_ui16ItemLengths[2] +
                                                pxbRangeBans.m_ui16ItemLengths[3] + pxbRangeBans.m_ui16ItemLengths[4] + pxbRangeBans.m_ui16ItemLengths[5] +
                                                pxbRangeBans.m_ui16ItemLengths[6],
                                            7))
            {
                break;
            }
        }
    }

    pxbRangeBans.WriteRemaining();
}
//---------------------------------------------------------------------------

void BanManager::ClearTemp()
{
    while (!m_TempBanList.empty())
    {
        BanItem* curBan = m_TempBanList.front().get();

        Rem(curBan);
        std::unique_ptr<BanItem> guard(curBan);
    }

    Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearPerm()
{
    while (!m_PermBanList.empty())
    {
        BanItem* curBan = m_PermBanList.front().get();

        Rem(curBan);
        std::unique_ptr<BanItem> guard(curBan);
    }

    Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearRange()
{
    m_RangeBanList.clear();

    Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearTempRange()
{
    auto it = m_RangeBanList.begin();
    while (it != m_RangeBanList.end())
    {
        if (((it->get()->m_ui8Bits & TEMP) == TEMP))
        {
            it = m_RangeBanList.erase(it);
        }
        else
        {
            ++it;
        }
    }

    Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearPermRange()
{
    auto it = m_RangeBanList.begin();
    while (it != m_RangeBanList.end())
    {
        if (((it->get()->m_ui8Bits & PERM) == PERM))
        {
            it = m_RangeBanList.erase(it);
        }
        else
        {
            ++it;
        }
    }

    Save();
}
//---------------------------------------------------------------------------

void BanManager::Ban(User* pUser, const char* sReason, const char* sBy, const bool bFull)
{
    auto pBan = std::make_unique<BanItem>();

    pBan->m_ui8Bits |= PERM;

    pBan->initIP(pUser);
    pBan->m_ui8Bits |= IP;

    if (bFull)
    {
        pBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    // PPK ... check for <unknown> nick -> bad ban from script
    if (!pUser->m_sNick.starts_with('<'))
    {
        pBan->m_sNick.assign(pUser->m_sNick.c_str(), pUser->m_sNick.size());
        pBan->m_ui32NickHash = pUser->m_ui32NickHash;
        pBan->m_ui8Bits |= NICK;

        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick/ip multiple times :(
        BanItem* nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

        if (nxtBan)
        {
            if (((nxtBan->m_ui8Bits & PERM) == PERM))
            {
                if (((nxtBan->m_ui8Bits & IP) == IP))
                {
                    if (memcmp(pBan->m_ui128IpHash.data(), nxtBan->m_ui128IpHash.data(), 16) == 0)
                    {
                        if (!((pBan->m_ui8Bits & FULL) == FULL))
                        {
                            // PPK ... same ban and new is not full, delete new
                            return;
                        }

                        if (((nxtBan->m_ui8Bits & FULL) == FULL))
                        {
                            // PPK ... same ban and both full, delete new
                            return;
                        }
                        // PPK ... same ban but only new is full, delete old
                        Rem(nxtBan);
                        std::unique_ptr<BanItem> guard(nxtBan);
                    }
                    else // NOLINT(readability-misleading-indentation)
                    {
                        pBan->m_ui8Bits &= ~NICK;
                    }
                }
                else
                {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    std::unique_ptr<BanItem> guard(nxtBan);
                }
            }
            else
            {
                if (((nxtBan->m_ui8Bits & IP) == IP))
                {
                    if (memcmp(pBan->m_ui128IpHash.data(), nxtBan->m_ui128IpHash.data(), 16) == 0)
                    {
                        if (!((nxtBan->m_ui8Bits & FULL) == FULL))
                        {
                            // PPK ... same ban and old is only temp, delete old
                            Rem(nxtBan);
                            std::unique_ptr<BanItem> guard(nxtBan);
                        }
                        else
                        {
                            if (((pBan->m_ui8Bits & FULL) == FULL))
                            {
                                // PPK ... same full ban and old is only temp, delete old
                                Rem(nxtBan);
                                std::unique_ptr<BanItem> guard(nxtBan);
                            }
                            else
                            {
                                // PPK ... old ban is full, new not... set old ban to only ipban
                                RemFromNickTable(nxtBan);
                                nxtBan->m_ui8Bits &= ~NICK;
                            }
                        }
                    }
                    else
                    {
                        // PPK ... set old ban to ip ban only
                        RemFromNickTable(nxtBan);
                        nxtBan->m_ui8Bits &= ~NICK;
                    }
                }
                else
                {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    std::unique_ptr<BanItem> guard(nxtBan);
                }
            }
        }
    }

    // PPK ... clear bans with same ip without nickban and fullban if new ban is fullban
    BanItem *curBan = nullptr, *nxtBan = FindIP(pBan->m_ui128IpHash.data(), acc_time);

    while (nxtBan)
    {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;

        if (((curBan->m_ui8Bits & NICK) == NICK))
        {
            continue;
        }

        if (((curBan->m_ui8Bits & FULL) == FULL) && !((pBan->m_ui8Bits & FULL) == FULL))
        {
            continue;
        }

        Rem(curBan);
        std::unique_ptr<BanItem> guard(curBan);
    }

    if (sReason)
    {
        const size_t szReasonLen = strlen(sReason);
        if (szReasonLen > 511)
        {
            pBan->m_sReason.assign(sReason, 508);
            pBan->m_sReason += "...";
        }
        else
        {
            pBan->m_sReason.assign(sReason, szReasonLen);
        }
    }

    if (!AddBanInternal(sBy, pBan.get()))
    {
        return;
    }
    if (!Add(pBan.release()))
    {
        return;
    }

    Save();
}
//---------------------------------------------------------------------------
bool BanManager::AddBanInternal(const char* sBy, BanItemBase* pBan)
{
    if (sBy)
    {
        const auto szByLen = std::min(strlen(sBy), size_t{63});
        pBan->m_sBy.assign(sBy, szByLen);
    }
    return true;
}

//---------------------------------------------------------------------------
namespace {
void CreateReason(BanItemBase* pBan, const char* sReason)
{
    if (sReason)
    {
        const size_t szReasonLen = strlen(sReason);
        if (szReasonLen > 511)
        {
            pBan->m_sReason.assign(sReason, 508);
            pBan->m_sReason += "...";
        }
        else
        {
            pBan->m_sReason.assign(sReason, szReasonLen);
        }
    }
}
} // namespace
char BanManager::BanIp(User* pUser, const char* sIp, const char* sReason, const char* sBy, const bool bFull)
{
    auto pBan = std::make_unique<BanItem>();

    pBan->m_ui8Bits |= PERM;

    if (pUser)
    {
        pBan->initIP(pUser);
    }
    else
    {
        if (sIp && HashIP(sIp, pBan->m_ui128IpHash.data()))
        {
            pBan->initIP(sIp);
        }
        else
        {
            return 1;
        }
    }

    pBan->m_ui8Bits |= IP;

    if (bFull)
    {
        pBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    BanItem *curBan = nullptr, *nxtBan = FindIP(pBan->m_ui128IpHash.data(), acc_time);

    // PPK ... don't add ban if is already here perm (full) ban for same ip
    while (nxtBan)
    {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;

        if (((curBan->m_ui8Bits & TEMP) == TEMP))
        {
            if (!((curBan->m_ui8Bits & FULL) == FULL) || ((pBan->m_ui8Bits & FULL) == FULL))
            {
                if (!((curBan->m_ui8Bits & NICK) == NICK))
                {
                    Rem(curBan);
                    std::unique_ptr<BanItem> guard(curBan);
                }

                continue;
            }

            continue;
        }

        if (!((curBan->m_ui8Bits & FULL) == FULL) && ((pBan->m_ui8Bits & FULL) == FULL))
        {
            if (!((curBan->m_ui8Bits & NICK) == NICK))
            {
                Rem(curBan);
                std::unique_ptr<BanItem> guard(curBan);
            }
            continue;
        }

        return 2;
    }

    CreateReason(pBan.get(), sReason);

    if (!AddBanInternal(sBy, pBan.get()))
    {
        return 1;
    }
    if (!Add(pBan.release()))
    {
        return 1;
    }

    Save();

    return 0;
}
//---------------------------------------------------------------------------

bool BanManager::NickBan(User* pUser, const char* sNick, const char* sReason, const char* sBy)
{
    auto pBan = std::make_unique<BanItem>();

    pBan->m_ui8Bits |= PERM;

    if (!pUser)
    {
        // PPK ... this should never happen, but to be sure ;)
        if (!sNick)
        {

            return false;
        }

        // PPK ... bad script ban check
        if (sNick[0] == '<')
        {

            return false;
        }

        const size_t szNickLen = strlen(sNick);
        pBan->m_sNick.assign(sNick, szNickLen);
        pBan->m_ui32NickHash = HashNick(std::string_view(sNick, szNickLen));
    }
    else
    {
        // PPK ... bad script ban check
        if (pUser->m_sNick.starts_with('<'))
        {

            return false;
        }

        pBan->m_sNick.assign(pUser->m_sNick.c_str(), pUser->m_sNick.size());
        pBan->m_ui32NickHash = pUser->m_ui32NickHash;

        pBan->initIP(pUser);
    }

    pBan->m_ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

    BanItem* nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

    // PPK ... not allow same nickbans !
    if (nxtBan)
    {
        if (((nxtBan->m_ui8Bits & PERM) == PERM))
        {

            return false;
        }

        if (((nxtBan->m_ui8Bits & IP) == IP))
        {
            // PPK ... set old ban to ip ban only
            RemFromNickTable(nxtBan);
            nxtBan->m_ui8Bits &= ~NICK;
        }
        else // NOLINT(readability-misleading-indentation)
        {
            // PPK ... old ban is only nickban, remove it
            Rem(nxtBan);
            std::unique_ptr<BanItem> guard(nxtBan);
        }
    }
    CreateReason(pBan.get(), sReason);

    if (!AddBanInternal(sBy, pBan.get()))
    {
        return false;
    }
    if (!Add(pBan.release()))
    {
        return false;
    }

    Save();

    return true;
}
//---------------------------------------------------------------------------

void BanManager::TempBan(User* pUser, const char* sReason, const char* sBy, const uint32_t minutes, time_t expiretime, const bool bFull)
{
    auto pBan = std::make_unique<BanItem>();

    pBan->m_ui8Bits |= TEMP;

    pBan->initIP(pUser);

    pBan->m_ui8Bits |= IP;

    if (bFull)
    {
        pBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if (expiretime > 0)
    {
        pBan->m_tTempBanExpire = expiretime;
    }
    else
    {
        if (minutes > 0)
        {
            pBan->m_tTempBanExpire = acc_time + static_cast<time_t>(minutes * 60);
        }
        else
        {
            pBan->m_tTempBanExpire = acc_time + static_cast<time_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFAULT_TEMP_BAN_TIME)] * 60);
        }
    }

    // PPK ... check for <unknown> nick -> bad ban from script
    if (!pUser->m_sNick.starts_with('<'))
    {
        pBan->m_sNick = pUser->m_sNick;
        pBan->m_ui32NickHash = pUser->m_ui32NickHash;
        pBan->m_ui8Bits |= NICK;

        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick multiple times :(
        BanItem* nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

        if (nxtBan)
        {
            if (((nxtBan->m_ui8Bits & PERM) == PERM))
            {
                if (((nxtBan->m_ui8Bits & IP) == IP))
                {
                    if (memcmp(pBan->m_ui128IpHash.data(), nxtBan->m_ui128IpHash.data(), 16) == 0)
                    {
                        if (!((pBan->m_ui8Bits & FULL) == FULL))
                        {
                            // PPK ... same ban and old is perm, delete new
                            return;
                        }

                        if (((nxtBan->m_ui8Bits & FULL) == FULL))
                        {
                            // PPK ... same ban and old is full perm, delete new
                            return;
                        }
                        else // NOLINT(readability-misleading-indentation)
                        {
                            // PPK ... same ban and only new is full, set new to ipban only
                            pBan->m_ui8Bits &= ~NICK;
                        }
                    }
                }
                else // NOLINT(readability-misleading-indentation)
                {
                    // PPK ... perm ban to same nick already exist, set new to ipban only
                    pBan->m_ui8Bits &= ~NICK;
                }
            }
            else
            {
                if (nxtBan->m_tTempBanExpire < pBan->m_tTempBanExpire)
                {
                    if (((nxtBan->m_ui8Bits & IP) == IP))
                    {
                        if (memcmp(pBan->m_ui128IpHash.data(), nxtBan->m_ui128IpHash.data(), 16) == 0)
                        {
                            if (!((nxtBan->m_ui8Bits & FULL) == FULL))
                            {
                                // PPK ... same bans, but old with lower expiration -> delete old
                                Rem(nxtBan);
                                std::unique_ptr<BanItem> guard(nxtBan);
                            }
                            else
                            {
                                if (!((pBan->m_ui8Bits & FULL) == FULL))
                                {
                                    // PPK ... old ban with lower expiration is full ban, set old to ipban only
                                    RemFromNickTable(nxtBan);
                                    nxtBan->m_ui8Bits &= ~NICK;
                                }
                                else
                                {
                                    // PPK ... same bans, old have lower expiration -> delete old
                                    Rem(nxtBan);
                                    std::unique_ptr<BanItem> guard(nxtBan);
                                }
                            }
                        }
                        else
                        {
                            // PPK ... set old ban to ipban only
                            RemFromNickTable(nxtBan);
                            nxtBan->m_ui8Bits &= ~NICK;
                        }
                    }
                    else
                    {
                        // PPK ... old ban is only nickban with lower bantime, remove it
                        Rem(nxtBan);
                        std::unique_ptr<BanItem> guard(nxtBan);
                    }
                }
                else
                {
                    if (((nxtBan->m_ui8Bits & IP) == IP))
                    {
                        if (memcmp(pBan->m_ui128IpHash.data(), nxtBan->m_ui128IpHash.data(), 16) == 0)
                        {
                            if (!((pBan->m_ui8Bits & FULL) == FULL))
                            {
                                // PPK ... same bans, but new with lower expiration -> delete new
                                return;
                            }

                            if (!((nxtBan->m_ui8Bits & FULL) == FULL))
                            {
                                // PPK ... new ban with lower expiration is full ban, set new to ipban only
                                pBan->m_ui8Bits &= ~NICK;
                            }
                            else // NOLINT(readability-misleading-indentation)
                            {
                                // PPK ... same bans, new have lower expiration -> delete new
                                return;
                            }
                        }
                        else // NOLINT(readability-misleading-indentation)
                        {
                            // PPK ... set new ban to ipban only
                            pBan->m_ui8Bits &= ~NICK;
                        }
                    }
                    else
                    {
                        // PPK ... old ban is only nickban with higher bantime, set new to ipban only
                        pBan->m_ui8Bits &= ~NICK;
                    }
                }
            }
        }
    }

    // PPK ... clear bans with lower timeban and same ip without nickban and fullban if new ban is fullban
    BanItem *curBan = nullptr, *nxtBan = FindIP(pBan->m_ui128IpHash.data(), acc_time);

    while (nxtBan)
    {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;

        if (((curBan->m_ui8Bits & PERM) == PERM))
        {
            continue;
        }

        if (((curBan->m_ui8Bits & NICK) == NICK))
        {
            continue;
        }

        if (((curBan->m_ui8Bits & FULL) == FULL) && !((pBan->m_ui8Bits & FULL) == FULL))
        {
            continue;
        }

        if (curBan->m_tTempBanExpire > pBan->m_tTempBanExpire)
        {
            continue;
        }

        Rem(curBan);
        std::unique_ptr<BanItem> guard(curBan);
    }
    CreateReason(pBan.get(), sReason);

    if (!AddBanInternal(sBy, pBan.get()))
    {
        return;
    }
    if (!Add(pBan.release()))
    {
        return;
    }

    Save();
}
//---------------------------------------------------------------------------

char BanManager::TempBanIp(
    User* pUser, const char* sIp, const char* sReason, const char* sBy, const uint32_t minutes, time_t expiretime, const bool bFull)
{
    auto pBan = std::make_unique<BanItem>();

    pBan->m_ui8Bits |= TEMP;

    if (pUser)
    {
        pBan->initIP(pUser);
    }
    else
    {
        if (sIp && HashIP(sIp, pBan->m_ui128IpHash.data()))
        {
            pBan->initIP(sIp);
        }
        else
        {
            return 1;
        }
    }

    pBan->m_ui8Bits |= IP;

    if (bFull)
    {
        pBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if (expiretime > 0)
    {
        pBan->m_tTempBanExpire = expiretime;
    }
    else
    {
        if (minutes == 0)
        {
            pBan->m_tTempBanExpire = acc_time + static_cast<time_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFAULT_TEMP_BAN_TIME)] * 60);
        }
        else
        {
            pBan->m_tTempBanExpire = acc_time + static_cast<time_t>(minutes * 60);
        }
    }

    BanItem *curBan = nullptr, *nxtBan = FindIP(pBan->m_ui128IpHash.data(), acc_time);

    // PPK ... don't add ban if is already here perm (full) ban or longer temp ban for same ip
    while (nxtBan)
    {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;

        if (((curBan->m_ui8Bits & TEMP) == TEMP) && curBan->m_tTempBanExpire < pBan->m_tTempBanExpire)
        {
            if (!((curBan->m_ui8Bits & FULL) == FULL) || ((pBan->m_ui8Bits & FULL) == FULL))
            {
                if (!((curBan->m_ui8Bits & NICK) == NICK))
                {
                    Rem(curBan);
                    std::unique_ptr<BanItem> guard(curBan);
                }

                continue;
            }

            continue;
        }

        if (!((curBan->m_ui8Bits & FULL) == FULL) && ((pBan->m_ui8Bits & FULL) == FULL))
            continue;

        return 2;
    }
    CreateReason(pBan.get(), sReason);

    if (!AddBanInternal(sBy, pBan.get()))
    {
        return 1;
    }
    if (!Add(pBan.release()))
    {
        return 1;
    }

    Save();

    return 0;
}
//---------------------------------------------------------------------------

bool BanManager::NickTempBan(User* pUser, const char* sNick, const char* sReason, const char* sBy, const uint32_t minutes, time_t expiretime)
{
    auto pBan = std::make_unique<BanItem>();

    pBan->m_ui8Bits |= TEMP;

    if (!pUser)
    {
        // PPK ... this should never happen, but to be sure ;)
        if (!sNick)
        {

            return false;
        }

        // PPK ... bad script ban check
        if (sNick[0] == '<')
        {

            return false;
        }

        const size_t szNickLen = strlen(sNick);
        pBan->m_sNick.assign(sNick, szNickLen);
        pBan->m_ui32NickHash = HashNick(std::string_view(sNick, szNickLen));
    }
    else
    {
        // PPK ... bad script ban check
        if (pUser->m_sNick.starts_with('<'))
        {

            return false;
        }

        pBan->m_sNick.assign(pUser->m_sNick.c_str(), pUser->m_sNick.size());
        pBan->m_ui32NickHash = pUser->m_ui32NickHash;

        pBan->initIP(pUser);
    }

    pBan->m_ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

    if (expiretime > 0)
    {
        pBan->m_tTempBanExpire = expiretime;
    }
    else
    {
        if (minutes > 0)
        {
            pBan->m_tTempBanExpire = acc_time + static_cast<time_t>(minutes * 60);
        }
        else
        {
            pBan->m_tTempBanExpire = acc_time + static_cast<time_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFAULT_TEMP_BAN_TIME)] * 60);
        }
    }

    BanItem* nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

    // PPK ... not allow same nickbans !
    if (nxtBan)
    {
        if (((nxtBan->m_ui8Bits & PERM) == PERM))
        {

            return false;
        }

        if (nxtBan->m_tTempBanExpire < pBan->m_tTempBanExpire)
        {
            if (((nxtBan->m_ui8Bits & IP) == IP))
            {
                // PPK ... set old ban to ip ban only
                RemFromNickTable(nxtBan);
                nxtBan->m_ui8Bits &= ~NICK;
            }
            else
            {
                // PPK ... old ban is only nickban, remove it
                Rem(nxtBan);
                std::unique_ptr<BanItem> guard(nxtBan);
            }
        }
        else // NOLINT(readability-misleading-indentation)
        {

            return false;
        }
    }
    CreateReason(pBan.get(), sReason);

    if (!AddBanInternal(sBy, pBan.get()))
    {
        return false;
    }
    if (!Add(pBan.release()))
    {
        return false;
    }

    Save();

    return true;
}
//---------------------------------------------------------------------------

bool BanManager::Unban(const char* sWhat)
{
    const uint32_t hash = HashNick(sWhat);

    time_t acc_time;
    time(&acc_time);

    BanItem* Ban = FindNick(hash, acc_time, sWhat);

    if (!Ban)
    {
        Hash128 ui128Hash;

        if (HashIP(sWhat, ui128Hash) && (Ban = FindIP(ui128Hash, acc_time)))
        {
            Rem(Ban);
            std::unique_ptr<BanItem> guard(Ban);
        }
        else
        {
            return false;
        }
    }
    else
    {
        Rem(Ban);
        std::unique_ptr<BanItem> guard(Ban);
    }

    Save();
    return true;
}
//---------------------------------------------------------------------------

bool BanManager::PermUnban(const char* sWhat)
{
    const uint32_t hash = HashNick(sWhat);

    BanItem* Ban = FindPermNick(hash, sWhat);

    if (!Ban)
    {
        Hash128 ui128Hash;

        if (HashIP(sWhat, ui128Hash) && (Ban = FindPermIP(ui128Hash)))
        {
            Rem(Ban);
            std::unique_ptr<BanItem> guard(Ban);
        }
        else
        {
            return false;
        }
    }
    else
    {
        Rem(Ban);
        std::unique_ptr<BanItem> guard(Ban);
    }

    Save();
    return true;
}
//---------------------------------------------------------------------------

bool BanManager::TempUnban(const char* sWhat)
{
    const uint32_t hash = HashNick(sWhat);

    time_t acc_time;
    time(&acc_time);

    BanItem* Ban = FindTempNick(hash, acc_time, sWhat);

    if (!Ban)
    {
        Hash128 ui128Hash;

        if (HashIP(sWhat, ui128Hash) && (Ban = FindTempIP(ui128Hash, acc_time)))
        {
            Rem(Ban);
            std::unique_ptr<BanItem> guard(Ban);
        }
        else
        {
            return false;
        }
    }
    else
    {
        Rem(Ban);
        std::unique_ptr<BanItem> guard(Ban);
    }

    Save();
    return true;
}
//---------------------------------------------------------------------------

void BanManager::RemoveAllIP(const uint8_t* m_ui128IpHash)
{
    std::array<uint8_t, 16> key;
    std::copy_n(m_ui128IpHash, 16, key.begin());

    const auto it = m_IpBanTable.find(key);
    if (it == m_IpBanTable.end())
    {
        return;
    }

    BanItem *curBan = nullptr, *nextBan = it->second.pFirstBan;

    while (nextBan)
    {
        curBan = nextBan;
        nextBan = curBan->m_pHashIpTableNext;

        Rem(curBan);
        std::unique_ptr<BanItem> guard(curBan);
    }
}
//---------------------------------------------------------------------------

void BanManager::RemovePermAllIP(const uint8_t* m_ui128IpHash)
{
    std::array<uint8_t, 16> key;
    std::copy_n(m_ui128IpHash, 16, key.begin());

    const auto it = m_IpBanTable.find(key);
    if (it == m_IpBanTable.end())
    {
        return;
    }

    BanItem *curBan = nullptr, *nextBan = it->second.pFirstBan;

    while (nextBan)
    {
        curBan = nextBan;
        nextBan = curBan->m_pHashIpTableNext;

        if (((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM))
        {
            Rem(curBan);
            std::unique_ptr<BanItem> guard(curBan);
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::RemoveTempAllIP(const uint8_t* m_ui128IpHash)
{
    std::array<uint8_t, 16> key;
    std::copy_n(m_ui128IpHash, 16, key.begin());

    const auto it = m_IpBanTable.find(key);
    if (it == m_IpBanTable.end())
    {
        return;
    }

    BanItem *curBan = nullptr, *nextBan = it->second.pFirstBan;

    while (nextBan)
    {
        curBan = nextBan;
        nextBan = curBan->m_pHashIpTableNext;

        if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP))
        {
            Rem(curBan);
            std::unique_ptr<BanItem> guard(curBan);
        }
    }
}
//---------------------------------------------------------------------------

bool BanManager::RangeBan(const char* m_sIpFrom,
                          const uint8_t* ui128FromIpHash,
                          const char* m_sIpTo,
                          const uint8_t* ui128ToIpHash,
                          const char* sReason,
                          const char* sBy,
                          const bool bFull)
{
    auto pRangeBan = std::make_unique<RangeBanItem>();

    pRangeBan->m_ui8Bits |= PERM;

    SafeStrCopy(pRangeBan->m_sIpFrom, m_sIpFrom);
    memcpy(pRangeBan->m_ui128FromIpHash, ui128FromIpHash, 16);

    SafeStrCopy(pRangeBan->m_sIpTo, m_sIpTo);
    memcpy(pRangeBan->m_ui128ToIpHash, ui128ToIpHash, 16);

    if (bFull)
    {
        pRangeBan->m_ui8Bits |= FULL;
    }

    // PPK ... don't add range ban if is already here same perm (full) range ban
    auto it = m_RangeBanList.begin();
    while (it != m_RangeBanList.end())
    {
        RangeBanItem* curBan = it->get();

        if (memcmp(curBan->m_ui128FromIpHash, pRangeBan->m_ui128FromIpHash, 16) != 0 || memcmp(curBan->m_ui128ToIpHash, pRangeBan->m_ui128ToIpHash, 16) != 0)
        {
            ++it;
            continue;
        }

        if ((curBan->m_ui8Bits & TEMP) == TEMP)
        {
            ++it;
            continue;
        }

        if (!((curBan->m_ui8Bits & FULL) == FULL) && ((pRangeBan->m_ui8Bits & FULL) == FULL))
        {
            it = m_RangeBanList.erase(it);

            continue;
        }

        return false;
    }
    CreateReason(pRangeBan.get(), sReason);

    if (!AddBanInternal(sBy, pRangeBan.get()))
    {
        return false;
    }

    AddRange(std::move(pRangeBan));
    Save();

    return true;
}
//---------------------------------------------------------------------------

bool BanManager::RangeTempBan(const char* m_sIpFrom,
                              const uint8_t* ui128FromIpHash,
                              const char* m_sIpTo,
                              const uint8_t* ui128ToIpHash,
                              const char* sReason,
                              const char* sBy,
                              const uint32_t minutes,
                              time_t expiretime,
                              const bool bFull)
{
    auto pRangeBan = std::make_unique<RangeBanItem>();

    pRangeBan->m_ui8Bits |= TEMP;

    SafeStrCopy(pRangeBan->m_sIpFrom, m_sIpFrom);
    memcpy(pRangeBan->m_ui128FromIpHash, ui128FromIpHash, 16);

    SafeStrCopy(pRangeBan->m_sIpTo, m_sIpTo);
    memcpy(pRangeBan->m_ui128ToIpHash, ui128ToIpHash, 16);

    if (bFull)
    {
        pRangeBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if (expiretime > 0)
    {
        pRangeBan->m_tTempBanExpire = expiretime;
    }
    else
    {
        if (minutes > 0)
        {
            pRangeBan->m_tTempBanExpire = acc_time + static_cast<time_t>(minutes * 60);
        }
        else
        {
            pRangeBan->m_tTempBanExpire = acc_time + static_cast<time_t>(SettingManager::m_Ptr->m_i16Shorts[std::to_underlying(SetShortIds::SETSHORT_DEFAULT_TEMP_BAN_TIME)] * 60);
        }
    }

    // PPK ... don't add range ban if is already here same perm (full) range ban or longer temp ban for same range
    auto it = m_RangeBanList.begin();
    while (it != m_RangeBanList.end())
    {
        RangeBanItem* curBan = it->get();

        if (memcmp(curBan->m_ui128FromIpHash, pRangeBan->m_ui128FromIpHash, 16) != 0 || memcmp(curBan->m_ui128ToIpHash, pRangeBan->m_ui128ToIpHash, 16) != 0)
        {
            ++it;
            continue;
        }

        if (((curBan->m_ui8Bits & TEMP) == TEMP) && curBan->m_tTempBanExpire < pRangeBan->m_tTempBanExpire)
        {
            if (!((curBan->m_ui8Bits & FULL) == FULL) || ((pRangeBan->m_ui8Bits & FULL) == FULL))
            {
                it = m_RangeBanList.erase(it);

                continue;
            }

            ++it;
            continue;
        }

        if (!((curBan->m_ui8Bits & FULL) == FULL) && ((pRangeBan->m_ui8Bits & FULL) == FULL))
        {
            ++it;
            continue;
        }

        return false;
    }
    CreateReason(pRangeBan.get(), sReason);

    if (!AddBanInternal(sBy, pRangeBan.get()))
    {
        return false;
    }

    AddRange(std::move(pRangeBan));
    Save();

    return true;
}
//---------------------------------------------------------------------------

bool BanManager::RangeUnban(const uint8_t* ui128FromIpHash, const uint8_t* ui128ToIpHash)
{
    for (auto it = m_RangeBanList.begin(); it != m_RangeBanList.end(); ++it)
    {
        RangeBanItem* cur = it->get();

        if (memcmp(cur->m_ui128FromIpHash, ui128FromIpHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToIpHash, 16) == 0)
        {
            m_RangeBanList.erase(it);

            Save();
            return true;
        }
    }

    Save();
    return false;
}
//---------------------------------------------------------------------------

bool BanManager::RangeUnban(const uint8_t* ui128FromIpHash, const uint8_t* ui128ToIpHash, unsigned char cType)
{
    for (auto it = m_RangeBanList.begin(); it != m_RangeBanList.end(); ++it)
    {
        RangeBanItem* cur = it->get();

        if ((cur->m_ui8Bits & cType) == cType && memcmp(cur->m_ui128FromIpHash, ui128FromIpHash, 16) == 0 &&
            memcmp(cur->m_ui128ToIpHash, ui128ToIpHash, 16) == 0)
        {
            m_RangeBanList.erase(it);

            Save();
            return true;
        }
    }

    Save();
    return false;
}
//---------------------------------------------------------------------------
