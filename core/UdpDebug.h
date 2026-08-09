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
#ifndef UdpDebugH
#define UdpDebugH
//---------------------------------------------------------------------------
#include "CriticalSection.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

struct User;
//---------------------------------------------------------------------------

class UdpDebug
{
private:
    mutable std::vector<char> m_sDebugBuffer;
    mutable char* m_sDebugHead = nullptr;
    uint32_t m_ui32SubscriberCount = 0;
    mutable CriticalSection m_csUdpDebug;

    struct UdpDbgItem
    {
        sockaddr_storage sas_to = {};

        std::string m_sNick;
        int s = -1;

        int sas_len = 0;

        uint32_t m_ui32Hash = 0;

        bool m_bIsScript = false, m_bAllData = true;

        UdpDbgItem();
        ~UdpDbgItem();
        UdpDbgItem(const UdpDbgItem&) = delete;
        auto operator=(const UdpDbgItem&) -> UdpDbgItem& = delete;
    };
    void CreateBuffer();
    void DeleteBuffer();
    void DeleteAllItems();

public:
    static std::unique_ptr<UdpDebug> m_Ptr;

    std::list<std::unique_ptr<UdpDbgItem>> m_DbgItemList;

    UdpDebug(const UdpDebug&) = delete;
    auto operator=(const UdpDebug&) -> UdpDebug& = delete;

    UdpDebug() = default;
    ~UdpDebug();

    void Broadcast(const std::string& p_msg) const
    {
        if (!p_msg.empty())
        {
            Broadcast(p_msg.c_str(), p_msg.size());
        }
    }
    void Broadcast(const char* sMsg, size_t szMsgLen) const;
    void BroadcastFormat(const char* sFormatMsg, ...) const;
    [[nodiscard]] auto New(User* pUser, uint16_t ui16Port) -> bool;
    [[nodiscard]] auto New(const char* sIP, uint16_t ui16Port, bool bAllData, const char* sScriptName) -> bool;
    [[nodiscard]] auto Remove(User* pUser) -> bool;
    void Remove(const char* sScriptName);
    [[nodiscard]] auto CheckUdpSub(User* pUser, bool bSndMess = false) const -> bool;
    void Send(const char* sScriptName, const char* sMessage, size_t szMsgLen) const;
    void Cleanup();
    void UpdateHubName();

    [[nodiscard]] auto GetSubscriberCount() const -> uint32_t
    {
        return m_ui32SubscriberCount;
    }
};
//---------------------------------------------------------------------------

#endif
