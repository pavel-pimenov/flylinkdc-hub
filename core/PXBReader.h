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
#ifndef PXBReaderH
#define PXBReaderH
//----------------------------------------------------------------------------------------------------------------------------

#include <vector>

class PXBReader
{
private:
    FILE* m_pFile = nullptr;

    char* m_pActualPosition = nullptr;

    size_t m_szRemainingSize = 0;

    bool m_bFullRead = false;

    void ReadNextFilePart();
    [[nodiscard]] auto PrepareArrays(uint8_t ui8Size) -> bool;

public:
    // Sentinel for PXB_BYTE boolean true — avoids reinterpret_cast<uintptr_t>(1) UB.
    // PXB write path checks `m_pItemDatas[i] == nullptr ? '0' : '1'` (never dereferences).
    static inline const char s_TrueSentinel = '\0';

    enum enmDataTypes
    {
        PXB_BYTE,
        PXB_TWO_BYTES,
        PXB_FOUR_BYTES,
        PXB_EIGHT_BYTES,
        PXB_STRING
    };

    std::vector<const void*> m_pItemDatas;

    std::vector<uint16_t> m_ui16ItemLengths;

    std::vector<char> m_sItemIdentifiers;

    std::vector<uint8_t> m_ui8ItemValues;

    PXBReader() = default;
    ~PXBReader();

    [[nodiscard]] auto OpenFileRead(const char* sFilename, uint8_t ui8SubItems) -> bool;
    [[nodiscard]] auto ReadNextItem(const uint16_t* pExpectedIdentificators, uint8_t ui8ExpectedSubItems, uint8_t ui8ExtraSubItems = 0) -> bool;

    [[nodiscard]] auto OpenFileSave(const char* sFilename, uint8_t ui8Size) -> bool;
    [[nodiscard]] auto WriteNextItem(uint32_t ui32Length, uint8_t ui8SubItems) -> bool;
    void WriteRemaining();
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
