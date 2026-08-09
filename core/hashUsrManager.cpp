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
#include "hashUsrManager.h"
//---------------------------------------------------------------------------
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::unique_ptr<HashManager> HashManager::m_Ptr;
//---------------------------------------------------------------------------

HashManager::HashManager() = default;
//---------------------------------------------------------------------------

HashManager::~HashManager() = default;
//---------------------------------------------------------------------------

bool HashManager::Add(User* pUser)
{

    m_NickTable[pUser->m_sNick] = pUser;

    const auto it = m_IpTable.find(pUser->m_ui128IpHash);
    if (it != m_IpTable.end())
    {
        IpTableItem& rItem = it->second;
        rItem.m_pFirstUser->m_pHashIpTablePrev = pUser;
        pUser->m_pHashIpTableNext = rItem.m_pFirstUser;
        rItem.m_pFirstUser = pUser;
        rItem.m_ui16Count++;

        return true;
    }

    IpTableItem& rItem = m_IpTable[pUser->m_ui128IpHash];
    rItem.m_pFirstUser = pUser;
    rItem.m_ui16Count = 1;

    return true;
}
//---------------------------------------------------------------------------

void HashManager::Remove(User* pUser)
{

    const auto itNick = m_NickTable.find(pUser->m_sNick);
    if (itNick != m_NickTable.end())
    {
        m_NickTable.erase(itNick);
    }

    if (!pUser->m_pHashIpTablePrev)
    {
        const auto it = m_IpTable.find(pUser->m_ui128IpHash);
        if (it != m_IpTable.end())
        {
            IpTableItem& rItem = it->second;
            rItem.m_ui16Count--;

            if (!pUser->m_pHashIpTableNext)
            {
                m_IpTable.erase(it);
            }
            else
            {
                pUser->m_pHashIpTableNext->m_pHashIpTablePrev = nullptr;
                rItem.m_pFirstUser = pUser->m_pHashIpTableNext;
            }

            pUser->m_pHashIpTablePrev = nullptr;
            pUser->m_pHashIpTableNext = nullptr;
        }

        return;
    }

    if (!pUser->m_pHashIpTableNext)
    {
        pUser->m_pHashIpTablePrev->m_pHashIpTableNext = nullptr;
    }
    else
    {
        pUser->m_pHashIpTablePrev->m_pHashIpTableNext = pUser->m_pHashIpTableNext;
        pUser->m_pHashIpTableNext->m_pHashIpTablePrev = pUser->m_pHashIpTablePrev;
    }

    pUser->m_pHashIpTablePrev = nullptr;
    pUser->m_pHashIpTableNext = nullptr;

    const auto it = m_IpTable.find(pUser->m_ui128IpHash);
    if (it != m_IpTable.end())
    {
        it->second.m_ui16Count--;
    }
}
//---------------------------------------------------------------------------
User* HashManager::FindUser(std::string_view sNick) const
{
    const auto i = m_NickTable.find(sNick);
    if (i != m_NickTable.end())
    {
        return i->second;
    }
    return nullptr;
}
//---------------------------------------------------------------------------
User* HashManager::FindUser(const User* pUser) const
{
    return FindUser(pUser->m_sNick);
}
//---------------------------------------------------------------------------

User* HashManager::FindUser(const uint8_t* ui128IpHash) const
{
    std::array<uint8_t, 16> key;
    memcpy(key.data(), ui128IpHash, 16);

    const auto it = m_IpTable.find(key);
    if (it != m_IpTable.end())
    {
        return it->second.m_pFirstUser;
    }

    return nullptr;
}
//---------------------------------------------------------------------------

uint32_t HashManager::GetUserIpCount(const User* pUser) const
{
    const auto it = m_IpTable.find(pUser->m_ui128IpHash);
    if (it != m_IpTable.end())
    {
        return it->second.m_ui16Count;
    }

    return 0;
}
//---------------------------------------------------------------------------

uint16_t HashManager::GetMaxIpCount() const
{
    uint16_t maxCount = 0;
    for (const auto& [key, value] : m_IpTable)
    {
        if (value.m_ui16Count > maxCount)
        {
            maxCount = value.m_ui16Count;
        }
    }
    return maxCount;
}
//---------------------------------------------------------------------------
