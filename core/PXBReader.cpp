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
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "PXBReader.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <algorithm>
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
#include "UdpDebug.h"
#include "utility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PXBReader::~PXBReader()
{
    if (m_pFile)
    {
        fclose(m_pFile);
        m_pFile = nullptr;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileRead(const char* sFilename, const uint8_t ui8SubItems)
{
    if (!PrepareArrays(ui8SubItems))
    {
        return false;
    }

    m_pFile = fopen(sFilename, "rb");

    if (!m_pFile)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[ERR] PXBReader::OpenFileRead failed to open '%s'", sFilename);
        return false;
    }

    fseek(m_pFile, 0, SEEK_END);
    const long lFileLen = ftell(m_pFile);

    if (lFileLen <= 0)
    {
        return false;
    }

    fseek(m_pFile, 0, SEEK_SET);

    m_szRemainingSize = PTOKAX_GLOBAL_BUFF_SIZE;

    if (static_cast<size_t>(lFileLen) < m_szRemainingSize)
    {
        m_szRemainingSize = lFileLen;

        m_bFullRead = true;
    }

    if (fread(ServerManager::m_pGlobalBuffer, 1, m_szRemainingSize, m_pFile) != m_szRemainingSize)
    {
        return false;
    }

    m_pActualPosition = ServerManager::m_pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::ReadNextFilePart()
{
    memmove(ServerManager::m_pGlobalBuffer, m_pActualPosition, m_szRemainingSize);

    const size_t szReadSize = fread(ServerManager::m_pGlobalBuffer + m_szRemainingSize, 1, PTOKAX_GLOBAL_BUFF_SIZE - m_szRemainingSize, m_pFile);

    if (szReadSize != (PTOKAX_GLOBAL_BUFF_SIZE - m_szRemainingSize))
    {
        m_bFullRead = true;
    }

    m_pActualPosition = ServerManager::m_pGlobalBuffer;
    m_szRemainingSize += szReadSize;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::ReadNextItem(const uint16_t* pExpectedIdentificators, const uint8_t ui8ExpectedSubItems, const uint8_t ui8ExtraSubItems /* = 0*/)
{
    if (m_szRemainingSize == 0)
    {
        return false;
    }

    std::fill(m_pItemDatas.begin(), m_pItemDatas.end(), nullptr);
    std::fill(m_ui16ItemLengths.begin(), m_ui16ItemLengths.end(), 0);

    if (m_szRemainingSize < 4)
    {
        if (m_bFullRead)
        {
            return false;
        }
        ReadNextFilePart();

        if (m_szRemainingSize < 4)
        {
            return false;
        }
    }

    uint32_t ui32ItemSize;
    memcpy(&ui32ItemSize, m_pActualPosition, sizeof(ui32ItemSize));
    ui32ItemSize = ntohl(ui32ItemSize);

    if (ui32ItemSize > m_szRemainingSize)
    {
        if (m_bFullRead)
        {
            return false;
        }
        ReadNextFilePart();

        if (ui32ItemSize > m_szRemainingSize)
        {
            return false;
        }
    }

    m_pActualPosition += 4;
    m_szRemainingSize -= 4;
    ui32ItemSize -= 4;

    uint8_t ui8ActualItem = 0;

    uint16_t ui16SubItemSize = 0;

    while (ui32ItemSize > 0)
    {
        memcpy(&ui16SubItemSize, m_pActualPosition, sizeof(ui16SubItemSize));
        ui16SubItemSize = ntohs(ui16SubItemSize);

        if (ui16SubItemSize > ui32ItemSize)
        {
            return false;
        }

        if (ui8ActualItem < ui8ExpectedSubItems)
        {
            uint16_t ui16Ident;
            memcpy(&ui16Ident, m_pActualPosition + 2, sizeof(ui16Ident));
            if (pExpectedIdentificators[ui8ActualItem] == ui16Ident)
            {
                m_ui16ItemLengths[ui8ActualItem] = (ui16SubItemSize - 4);
                m_pItemDatas[ui8ActualItem] = (m_pActualPosition + 4);
                ui8ActualItem++;
            }
            else
            {
                for (uint8_t ui8i = 0; ui8i < (ui8ExpectedSubItems + ui8ExtraSubItems); ui8i++)
                {
                    uint16_t ui16Ident2;
                    memcpy(&ui16Ident2, m_pActualPosition + 2, sizeof(ui16Ident2));
                    if (pExpectedIdentificators[ui8i] == ui16Ident2)
                    {
                        m_ui16ItemLengths[ui8i] = (ui16SubItemSize - 4);
                        m_pItemDatas[ui8i] = (m_pActualPosition + 4);
                        ui8ActualItem++;
                    }
                }
            }
        }

        m_pActualPosition += ui16SubItemSize;
        m_szRemainingSize -= ui16SubItemSize;
        ui32ItemSize -= ui16SubItemSize;
    }

    return ui8ActualItem == ui8ExpectedSubItems;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileSave(const char* sFilename, const uint8_t ui8Size)
{
    if (!PrepareArrays(ui8Size))
    {
        return false;
    }

    m_pFile = fopen(sFilename, "wb");

    if (!m_pFile)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[ERR] PXBReader::OpenFileWrite failed to open '%s'", sFilename);
        return false;
    }

    m_szRemainingSize = PTOKAX_GLOBAL_BUFF_SIZE;

    m_pActualPosition = ServerManager::m_pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::WriteNextItem(const uint32_t ui32Length, const uint8_t ui8SubItems)
{
    const uint32_t ui32ItemLength = ui32Length + 4 + (4 * ui8SubItems);

    if (ui32ItemLength > m_szRemainingSize)
    {
        const size_t szFlushLen = m_pActualPosition - ServerManager::m_pGlobalBuffer;
        if (fwrite(ServerManager::m_pGlobalBuffer, 1, szFlushLen, m_pFile) != szFlushLen)
        {
            LogError("fwrite failed in PXBReader::WriteNextItem");
        }
        m_pActualPosition = ServerManager::m_pGlobalBuffer;
        m_szRemainingSize = PTOKAX_GLOBAL_BUFF_SIZE;
    }

    const uint32_t ui32NetLen = htonl(ui32ItemLength);
    memcpy(m_pActualPosition, &ui32NetLen, sizeof(ui32NetLen));
    m_pActualPosition += 4;
    m_szRemainingSize -= 4;

    for (uint8_t ui8i = 0; ui8i < ui8SubItems; ui8i++)
    {
        const uint16_t ui16NetLen = htons(m_ui16ItemLengths[ui8i] + 4);
        memcpy(m_pActualPosition, &ui16NetLen, sizeof(ui16NetLen));

        m_pActualPosition[2] = m_sItemIdentifiers[(ui8i * 2)];
        m_pActualPosition[3] = m_sItemIdentifiers[(ui8i * 2) + 1];

        switch (m_ui8ItemValues[ui8i])
        {
        case PXB_BYTE:
            m_pActualPosition[4] = (!m_pItemDatas[ui8i] ? '0' : '1');
            break;
        case PXB_TWO_BYTES:
        {
            uint16_t ui16Val;
            memcpy(&ui16Val, m_pItemDatas[ui8i], sizeof(ui16Val));
            const uint16_t ui16Net = htons(ui16Val);
            memcpy(m_pActualPosition + 4, &ui16Net, sizeof(ui16Net));
            break;
        }
        case PXB_FOUR_BYTES:
        {
            uint32_t ui32Val;
            memcpy(&ui32Val, m_pItemDatas[ui8i], sizeof(ui32Val));
            const uint32_t ui32Net = htonl(ui32Val);
            memcpy(m_pActualPosition + 4, &ui32Net, sizeof(ui32Net));
            break;
        }
        case PXB_EIGHT_BYTES:
        {
            uint64_t ui64Val;
            memcpy(&ui64Val, m_pItemDatas[ui8i], sizeof(ui64Val));
            const uint64_t ui64Net = htobe64(ui64Val);
            memcpy(m_pActualPosition + 4, &ui64Net, sizeof(ui64Net));
            break;
        }
        case PXB_STRING:
            memcpy(m_pActualPosition + 4, m_pItemDatas[ui8i], m_ui16ItemLengths[ui8i]);
            break;
        default:
            break;
        }

        m_pActualPosition += m_ui16ItemLengths[ui8i] + 4;
        m_szRemainingSize -= m_ui16ItemLengths[ui8i] + 4;
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::WriteRemaining()
{
    if ((m_pActualPosition - ServerManager::m_pGlobalBuffer) > 0)
    {
        const size_t szFlushLen = m_pActualPosition - ServerManager::m_pGlobalBuffer;
        if (fwrite(ServerManager::m_pGlobalBuffer, 1, szFlushLen, m_pFile) != szFlushLen)
        {
            LogError("fwrite failed in PXBReader::WriteRemaining");
        }
    }

    fclose(m_pFile);
    m_pFile = nullptr;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::PrepareArrays(const uint8_t ui8Size)
{
    m_pItemDatas.resize(ui8Size, nullptr);

    m_ui16ItemLengths.resize(ui8Size, 0);

    m_sItemIdentifiers.resize(ui8Size * 2, 0);

    m_ui8ItemValues.resize(ui8Size, 0);

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
