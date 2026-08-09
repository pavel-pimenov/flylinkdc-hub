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
//---------------------------------------------------------------------------
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
#include "GlobalDataQueue.h"

//---------------------------------------------------------------------------
#include <zlib.h>
//---------------------------------------------------------------------------
static constexpr uint32_t g_ui32ZBufferLen = PTOKAX_GLOBAL_BUFF_SIZE;
static constexpr uint32_t g_ui32ZMinLen = 128;
static constexpr int Z_PTOKAX_COMPRESSION = 8;
//---------------------------------------------------------------------------
std::unique_ptr<ZlibUtility> ZlibUtility::m_Ptr;
//---------------------------------------------------------------------------

ZlibUtility::ZlibUtility()
{
    m_vZbuffer.resize(g_ui32ZBufferLen, '\0');
    memcpy(m_vZbuffer.data(), "$ZOn|", 5);
}
//---------------------------------------------------------------------------

// ZlibUtility destructor is = default in header
//---------------------------------------------------------------------------

char* ZlibUtility::CreateZPipe(std::string_view sInData, uint32_t& ui32OutDataLen)
{

#ifdef USE_FLYLINKDC_EXT_JSON
#ifdef _DEBUG
    LogDbg("[1] CreateZPipe {} bytes", sInData.size());
    LogDbg("{}", sInData);
#endif
#endif

    // prepare Zbuffer
    if (m_vZbuffer.size() < sInData.size() + 128)
    {
        m_vZbuffer.resize(Allign(sInData.size() + 128));
    }

    z_stream stream = {};

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_PTOKAX_COMPRESSION);

    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(sInData.data()));
    stream.avail_in = static_cast<uInt>(sInData.size());

    stream.next_out = reinterpret_cast<Bytef*>(m_vZbuffer.data()) + 5;
    stream.avail_out = static_cast<uInt>(m_vZbuffer.size() - 5);

    // compress
    if (deflate(&stream, Z_FINISH) != Z_STREAM_END)
    {
        deflateEnd(&stream);
        LogDbg("[ERR] deflate error");
        ui32OutDataLen = 0;
        return m_vZbuffer.data();
    }

    ui32OutDataLen = stream.total_out + 5;

    // cleanup zlib
    deflateEnd(&stream);

    if (ui32OutDataLen >= sInData.size())
    {
        ui32OutDataLen = 0;
    }
    GlobalDataQueue::m_Ptr->PrometheusZlibBytes("ZPipeIn", static_cast<int>(sInData.size()));
    GlobalDataQueue::m_Ptr->PrometheusZlibBytes("ZPipeOut", static_cast<int>(ui32OutDataLen));
    return m_vZbuffer.data();
}
//---------------------------------------------------------------------------

void ZlibUtility::CreateZPipe(std::string_view sInData, std::vector<char>& vOutData, uint32_t& ui32OutDataLen, const char* sMetricPrefix)
{
    if (sInData.size() < g_ui32ZMinLen)
    {
        return;
    }

    // prepare Zbuffer
    if (m_vZbuffer.size() < sInData.size() + 128)
    {
        m_vZbuffer.resize(Allign(sInData.size() + 128));
    }

    z_stream stream = {};

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_PTOKAX_COMPRESSION);

    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(sInData.data()));
    stream.avail_in = static_cast<uInt>(sInData.size());

    stream.next_out = reinterpret_cast<Bytef*>(m_vZbuffer.data()) + 5;
    stream.avail_out = static_cast<uInt>(m_vZbuffer.size() - 5);

    // compress
    if (deflate(&stream, Z_FINISH) != Z_STREAM_END)
    {
        deflateEnd(&stream);
        LogDbg("[ERR] deflate error");
        return;
    }

    ui32OutDataLen = stream.total_out + 5;

    // cleanup zlib
    deflateEnd(&stream);

    if (ui32OutDataLen >= sInData.size())
    {
        ui32OutDataLen = 0;
        return;
    }

    // prepare out buffer
    if (vOutData.size() < ui32OutDataLen)
    {
        vOutData.resize(Allign(ui32OutDataLen + 1));
    }

    memcpy(vOutData.data(), m_vZbuffer.data(), ui32OutDataLen);

    // Prometheus metrics with configurable prefix
    const std::string sInMetric = std::string(sMetricPrefix) + "In";
    const std::string sOutMetric = std::string(sMetricPrefix) + "Out";
    GlobalDataQueue::m_Ptr->PrometheusZlibBytes(sInMetric.c_str(), static_cast<int>(sInData.size()));
    GlobalDataQueue::m_Ptr->PrometheusZlibBytes(sOutMetric.c_str(), static_cast<int>(ui32OutDataLen));
}
//---------------------------------------------------------------------------
