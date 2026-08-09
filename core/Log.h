/*
 * PtokaX - hub server for Direct Connect peer to peer network.
 * Copyright (C) 2004-2022  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 */

#ifndef LogH
#define LogH

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace PXLog
{
void Init(const std::string& sLogPath);
void Shutdown();
[[nodiscard]] auto System() -> spdlog::logger*;
[[nodiscard]] auto Debug() -> spdlog::logger*;
[[nodiscard]] auto Script() -> spdlog::logger*;

void NotifySystemLog(const char* sData, size_t szLen);
void NotifyDebugLog(size_t szLen);
void NotifyScriptLog(size_t szLen);
} // namespace PXLog

#define LogInfo(...)                                                                                                                                           \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::System();                                                                                                                            \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->info(__VA_ARGS__);                                                                                                                             \
            PXLog::NotifySystemLog(nullptr, 0);                                                                                                                \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d info\n", __FILE__, __LINE__);                                                                               \
        }                                                                                                                                                      \
    } while (0)

#define LogWarn(...)                                                                                                                                           \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::System();                                                                                                                            \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->warn(__VA_ARGS__);                                                                                                                             \
            PXLog::NotifySystemLog(nullptr, 0);                                                                                                                \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d warn\n", __FILE__, __LINE__);                                                                               \
        }                                                                                                                                                      \
    } while (0)

#define LogError(...)                                                                                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::System();                                                                                                                            \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->error(__VA_ARGS__);                                                                                                                            \
            PXLog::NotifySystemLog(nullptr, 0);                                                                                                                \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d error\n", __FILE__, __LINE__);                                                                              \
        }                                                                                                                                                      \
    } while (0)

#define LogCritical(...)                                                                                                                                       \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::System();                                                                                                                            \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->critical(__VA_ARGS__);                                                                                                                         \
            PXLog::NotifySystemLog(nullptr, 0);                                                                                                                \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d critical\n", __FILE__, __LINE__);                                                                           \
        }                                                                                                                                                      \
    } while (0)

#define LogDbg(...)                                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::Debug();                                                                                                                             \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->debug(__VA_ARGS__);                                                                                                                            \
            PXLog::NotifyDebugLog(0);                                                                                                                          \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d debug\n", __FILE__, __LINE__);                                                                              \
        }                                                                                                                                                      \
    } while (0)

#define LogDbgInfo(...)                                                                                                                                        \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::Debug();                                                                                                                             \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->info(__VA_ARGS__);                                                                                                                             \
            PXLog::NotifyDebugLog(0);                                                                                                                          \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d dbginfo\n", __FILE__, __LINE__);                                                                            \
        }                                                                                                                                                      \
    } while (0)

#define LogDbgWarn(...)                                                                                                                                        \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::Debug();                                                                                                                             \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->warn(__VA_ARGS__);                                                                                                                             \
            PXLog::NotifyDebugLog(0);                                                                                                                          \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d dbgwarn\n", __FILE__, __LINE__);                                                                            \
        }                                                                                                                                                      \
    } while (0)

#define LogDbgErr(...)                                                                                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::Debug();                                                                                                                             \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->error(__VA_ARGS__);                                                                                                                            \
            PXLog::NotifyDebugLog(0);                                                                                                                          \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d dbgerr\n", __FILE__, __LINE__);                                                                             \
        }                                                                                                                                                      \
    } while (0)

#define LogScript(...)                                                                                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto* _l = PXLog::Script();                                                                                                                            \
        if (_l)                                                                                                                                                \
        {                                                                                                                                                      \
            _l->info(__VA_ARGS__);                                                                                                                             \
            PXLog::NotifyScriptLog(0);                                                                                                                         \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            fprintf(stderr, "[PXLog:NOTINIT] %s:%d script\n", __FILE__, __LINE__);                                                                             \
        }                                                                                                                                                      \
    } while (0)

#endif // LogH
