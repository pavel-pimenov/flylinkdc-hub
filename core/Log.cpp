/*
 * PtokaX - hub server for Direct Connect peer to peer network.
 * Copyright (C) 2004-2022  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 */

#include <iostream>
#include <cstdio>
#include <filesystem>

#include "stdinc.h"
#include "Log.h"
#include "GlobalDataQueue.h"
#include "UdpDebug.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <zlib.h>

static constexpr size_t g_ui32MaxLogFileSize = 20 * 1024 * 1024;
static constexpr size_t g_ui32MaxRotatedFiles = 3;
static constexpr size_t g_ui32CompressBufSize = 65536;

// ─── Compressed rotating file sink ───────────────────────────────────────────────────────────────────────────────────────────────────────
// Replaces spdlog's rotating_file_sink with gzip compression for rotated files.
// File layout after rotation:
//   system.log        – current (uncompressed)
//   system.log.1.gz   – most recent rotated (compressed)
//   system.log.2.gz   – second most recent rotated (compressed)
//   system.log.3.gz   – oldest rotated (compressed)
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────

class CompressedRotatingFileSink final : public spdlog::sinks::base_sink<std::mutex>
{
public:
    CompressedRotatingFileSink(std::string sBaseFilename, std::size_t szMaxSize, std::size_t szMaxFiles)
        : m_sBaseFilename(std::move(sBaseFilename)), m_szMaxSize(szMaxSize), m_szMaxFiles(szMaxFiles)
    {
        if (szMaxSize == 0)
        {
            throw spdlog::spdlog_ex("CompressedRotatingFileSink: max_size cannot be zero");
        }

        m_pFile = std::fopen(m_sBaseFilename.c_str(), "a");
        if (m_pFile)
        {
            m_szCurrentSize = static_cast<std::size_t>(std::ftell(m_pFile));
        }
    }

    ~CompressedRotatingFileSink() override
    {
        if (m_pFile)
        {
            std::fclose(m_pFile);
        }
    }

    CompressedRotatingFileSink(const CompressedRotatingFileSink&) = delete;
    CompressedRotatingFileSink& operator=(const CompressedRotatingFileSink&) = delete;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);

        auto szNewSize = m_szCurrentSize + formatted.size();

        if (szNewSize > m_szMaxSize && m_szCurrentSize > 0)
        {
            rotate();
            szNewSize = formatted.size();
        }

        if (m_pFile)
        {
            std::fwrite(formatted.data(), 1, formatted.size(), m_pFile);
            m_szCurrentSize = szNewSize;
        }
    }

    void flush_() override
    {
        if (m_pFile)
        {
            std::fflush(m_pFile);
        }
    }

private:
    static std::string calcFilename(const std::string& sBase, std::size_t szIndex)
    {
        if (szIndex == 0)
        {
            return sBase;
        }

        const auto dotPos = sBase.rfind('.');
        if (dotPos != std::string::npos)
        {
            return sBase.substr(0, dotPos) + "." + std::to_string(szIndex) + sBase.substr(dotPos);
        }
        return sBase + "." + std::to_string(szIndex);
    }

    static void compressFile(const std::string& sSrc, const std::string& sDst)
    {
        gzFile gz = gzopen(sDst.c_str(), "wb");
        if (!gz)
        {
            std::cerr << "[PXLog] CompressedRotatingFileSink: gzopen failed for " << sDst << std::endl;
            return;
        }

        FILE* const pSrc = std::fopen(sSrc.c_str(), "rb");
        if (!pSrc)
        {
            gzclose(gz);
            return;
        }

        std::array<char, g_ui32CompressBufSize> aBuf = {};
        std::size_t szRead;
        while ((szRead = std::fread(aBuf.data(), 1, aBuf.size(), pSrc)) > 0)
        {
            gzwrite(gz, aBuf.data(), static_cast<unsigned>(szRead));
        }

        std::fclose(pSrc);
        gzclose(gz);
    }

    void rotate()
    {
        if (m_pFile)
        {
            std::fclose(m_pFile);
            m_pFile = nullptr;
        }

        // Shift compressed files from highest index down
        for (std::size_t i = m_szMaxFiles; i > 0; --i)
        {
            const std::string sTarget = calcFilename(m_sBaseFilename, i) + ".gz";
            std::remove(sTarget.c_str());

            if (i == 1)
            {
                // Compress the current log file into .1.gz
                const std::string sSrc = calcFilename(m_sBaseFilename, 0);
                if (std::filesystem::exists(sSrc))
                {
                    compressFile(sSrc, sTarget);
                    std::remove(sSrc.c_str());
                }
            }
            else
            {
                // Shift: (i-1).gz → i.gz
                const std::string sPrevGz = calcFilename(m_sBaseFilename, i - 1) + ".gz";
                if (std::filesystem::exists(sPrevGz))
                {
                    std::rename(sPrevGz.c_str(), sTarget.c_str());
                }
            }
        }

        m_pFile = std::fopen(m_sBaseFilename.c_str(), "w");
        m_szCurrentSize = 0;
    }

    std::string m_sBaseFilename;
    std::size_t m_szMaxSize;
    std::size_t m_szMaxFiles;
    std::size_t m_szCurrentSize = 0;
    FILE* m_pFile = nullptr;
};

// ─── Logger init ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────

namespace {
std::shared_ptr<spdlog::logger> g_pSystemLogger;
std::shared_ptr<spdlog::logger> g_pDebugLogger;
std::shared_ptr<spdlog::logger> g_pScriptLogger;
} // namespace

void PXLog::Init(const std::string& sLogPath)
{
    const std::string sLogsDir = sLogPath + "/logs";
    mkdir(sLogsDir.c_str(), 0755);

    try
    {
        const auto pConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always);
        pConsoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        const auto pSystemFileSink = std::make_shared<CompressedRotatingFileSink>(sLogsDir + "/system.log", g_ui32MaxLogFileSize, g_ui32MaxRotatedFiles);
        pSystemFileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        const auto pDebugFileSink = std::make_shared<CompressedRotatingFileSink>(sLogsDir + "/debug.log", g_ui32MaxLogFileSize, g_ui32MaxRotatedFiles);
        pDebugFileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        const auto pScriptFileSink = std::make_shared<CompressedRotatingFileSink>(sLogsDir + "/script.log", g_ui32MaxLogFileSize, g_ui32MaxRotatedFiles);
        pScriptFileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        g_pSystemLogger = std::make_shared<spdlog::logger>("system", spdlog::sinks_init_list{pConsoleSink, pSystemFileSink});
        g_pSystemLogger->set_level(spdlog::level::info);
        g_pSystemLogger->flush_on(spdlog::level::info);

        g_pDebugLogger = std::make_shared<spdlog::logger>("debug", pDebugFileSink);
        g_pDebugLogger->set_level(spdlog::level::debug);
        g_pDebugLogger->flush_on(spdlog::level::debug);

        g_pScriptLogger = std::make_shared<spdlog::logger>("script", pScriptFileSink);
        g_pScriptLogger->set_level(spdlog::level::info);
        g_pScriptLogger->flush_on(spdlog::level::info);

        spdlog::set_default_logger(g_pSystemLogger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "Log init failed: " << ex.what() << std::endl;
    }
}

void PXLog::Shutdown()
{
    spdlog::shutdown();
    g_pSystemLogger.reset();
    g_pDebugLogger.reset();
    g_pScriptLogger.reset();
}

spdlog::logger* PXLog::System()
{
    return g_pSystemLogger.get();
}

spdlog::logger* PXLog::Debug()
{
    return g_pDebugLogger.get();
}

spdlog::logger* PXLog::Script()
{
    return g_pScriptLogger.get();
}

void PXLog::NotifySystemLog(const char* /*sData*/, size_t /*szLen*/)
{
    if (GlobalDataQueue::m_Ptr)
    {
        GlobalDataQueue::m_Ptr->PrometheusLogBytes("logs", 0);
    }

    if (UdpDebug::m_Ptr)
    {
        UdpDebug::m_Ptr->BroadcastFormat("[LOG] spdlog system log entry");
    }
}

void PXLog::NotifyDebugLog(size_t /*szLen*/)
{
    if (GlobalDataQueue::m_Ptr)
    {
        GlobalDataQueue::m_Ptr->PrometheusLogBytes("logsd", 0);
    }
}

void PXLog::NotifyScriptLog(size_t /*szLen*/)
{
    if (GlobalDataQueue::m_Ptr)
    {
        GlobalDataQueue::m_Ptr->PrometheusLogBytes("logs", 0);
    }
}
