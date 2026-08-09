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
#ifndef colUsersH
#define colUsersH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------
#include "GlobalDataQueue.h"
inline constexpr uint32_t NICKLISTSIZE = 1024 * 8;
inline constexpr uint32_t OPLISTSIZE = 512;
//---------------------------------------------------------------------------
#include <array>
#include <list>
#include <vector>
#include <string>

class Users
{
private:
    uint64_t m_ui64ChatMsgsTick = 0, m_ui64ChatLockFromTick = 0;

    struct RecTime
    {
        uint64_t m_ui64DisConnTick = 0;

        std::string m_sNick;
        uint32_t m_ui32NickHash = 0;

        std::array<uint8_t, 16> m_ui128IpHash{};

        explicit RecTime(const uint8_t* pIpHash);

        RecTime(const RecTime&) = delete;

        auto operator=(const RecTime&) -> RecTime& = delete;
    };

    std::list<std::unique_ptr<RecTime>> m_RecTimeList;

    uint16_t m_ui16ChatMsgs = 0;

    bool m_bChatLocked = false;

public:
    static std::unique_ptr<Users> m_Ptr;

    using UserList = std::list<std::unique_ptr<User>>;
    UserList m_UserList;

    std::vector<char> m_NickList, m_ZNickList, m_OpList, m_ZOpList;
    std::vector<char> m_UserIPList, m_ZUserIPList, m_MyInfos, m_ZMyInfos;
    std::vector<char> m_MyInfosTag, m_ZMyInfosTag;

    uint32_t m_ui32MyInfosLen = 0, m_ui32MyInfosSize = 0;
    uint32_t m_ui32ZMyInfosLen = 0, m_ui32ZMyInfosSize = 0;
    uint32_t m_ui32MyInfosTagLen = 0, m_ui32MyInfosTagSize = 0;
    uint32_t m_ui32ZMyInfosTagLen = 0, m_ui32ZMyInfosTagSize = 0;
    uint32_t m_ui32NickListLen = 0, m_ui32NickListSize = 0;
    uint32_t m_ui32ZNickListLen = 0, m_ui32ZNickListSize = 0;
    uint32_t m_ui32OpListLen = 0, m_ui32OpListSize = 0;
    uint32_t m_ui32ZOpListLen = 0, m_ui32ZOpListSize = 0;
    uint32_t m_ui32UserIPListSize = 0, m_ui32UserIPListLen = 0, m_ui32ZUserIPListSize = 0, m_ui32ZUserIPListLen = 0;

    uint16_t m_ui16ActSearchs = 0, m_ui16PasSearchs = 0;

#ifdef USE_FLYLINKDC_EXT_JSON
    std::string m_AllExtJSON;
#endif

    Users(const Users&) = delete;
    auto operator=(const Users&) -> Users& = delete;

    Users();
    ~Users();

    void DisconnectAll();
    void AddUser(std::unique_ptr<User> pUser);
    void RemUser(User* pUser);
    // Erase the user pointed to by the given iterator; returns the next valid iterator.
    // Safe to use while iterating m_UserList (avoids use-after-free on erase).
    UserList::iterator RemUser(UserList::iterator it);
    void Add2NickList(User* pUser);
    void AddBot2NickList(const char* sNick, size_t szNickLen, bool bIsOp);
    void AddBot2NickList(const std::string& sNick, const bool bIsOp)
    {
        AddBot2NickList(sNick.c_str(), sNick.size(), bIsOp);
    }
    void Add2OpList(User* pUser);
    void DelFromNickList(const char* sNick, bool bIsOp);
    void DelFromOpList(const char* sNick);
    void SendChat2All(User* pUser, const char* sData, size_t szChatLen, GlobalDataQueue::QueueItem* pQueueItem);
    void Add2MyInfos(User* pUser);
    void DelFromMyInfos(User* pUser);
#ifdef USE_FLYLINKDC_EXT_JSON
    void Add2ExtJSON(const User* pUser);
    void DelFromExtJSONInfos(const User* pUser);
#endif
    void Add2MyInfosTag(User* pUser);
    void DelFromMyInfosTag(User* pUser);
    void AddBot2MyInfos(const char* sMyInfo);
    void DelBotFromMyInfos(const char* sMyInfo);
    void Add2UserIP(User* pUser);
    void DelFromUserIP(User* pUser);
    void Add2RecTimes(User* pUser);
    [[nodiscard]] auto CheckRecTime(User* pUser) -> bool;
};
//---------------------------------------------------------------------------

#endif
