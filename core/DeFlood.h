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
#ifndef DeFloodH
#define DeFloodH
//---------------------------------------------------------------------------

enum class DefloodTypes : uint8_t
{
    GETNICKLIST,
    MYINFO,
    SEARCH,
    CHAT,
    PM,
    SAME_SEARCH,
    SAME_PM,
    SAME_CHAT,
    SAME_MULTI_PM,
    SAME_MULTI_CHAT,
    CTM,
    RCTM,
    SR,
    MAX_DOWN,
    INTERVAL_CHAT,
    INTERVAL_PM,
    INTERVAL_SEARCH
};
//---------------------------------------------------------------------------

[[nodiscard]] auto DeFloodCheckForFlood(User* pUser,
                          DefloodTypes eDefloodType,
                          int16_t ui16Action,
                          uint16_t& ui16Count,
                          uint64_t& ui64LastOkTick,
                          int16_t ui16DefloodCount,
                          uint32_t ui32DefloodTime,
                          const char* sOtherNick = nullptr) -> bool;
[[nodiscard]] auto DeFloodCheckForSameFlood(User* pUser,
                              DefloodTypes eDefloodType,
                              int16_t ui16Action,
                              uint16_t& ui16Count,
                              uint64_t ui64LastOkTick,
                              int16_t ui16DefloodCount,
                              uint32_t ui32DefloodTime,
                              const char* sNewData,
                              size_t ui32NewDataLen,
                              const char* sOldData,
                              uint16_t ui16OldDataLen,
                              bool& bNewData,
                              const char* sOtherNick = nullptr) -> bool;
[[nodiscard]] auto DeFloodCheckForDataFlood(User* pUser,
                              DefloodTypes eDefloodType,
                              int16_t ui16Action,
                              uint32_t& ui32Count,
                              uint64_t& ui64LastOkTick,
                              int16_t ui16DefloodCount,
                              uint32_t ui32DefloodTime) -> bool;
void DeFloodDoAction(User* pUser, DefloodTypes eDefloodType, int16_t ui16Action, uint16_t& ui16Count, const char* sOtherNick);
[[nodiscard]] auto DeFloodCheckForWarn(User* pUser, DefloodTypes eDefloodType, const char* sOtherNick) -> bool;
[[nodiscard]] auto DeFloodGetMessage(DefloodTypes eDefloodType, uint8_t ui8MsgId) -> const char*;
void DeFloodReport(User* pUser, DefloodTypes eDefloodType, const char* sAction);
[[nodiscard]] auto DeFloodCheckInterval(User* pUser,
                          DefloodTypes eDefloodType,
                          uint16_t& ui16Count,
                          uint64_t& ui64LastOkTick,
                          int16_t ui16DefloodCount,
                          uint32_t ui32DefloodTime,
                          const char* sOtherNick = nullptr) -> bool;
//---------------------------------------------------------------------------

#endif
