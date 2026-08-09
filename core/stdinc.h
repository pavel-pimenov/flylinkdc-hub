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
#ifndef stdincH
#define stdincH
//---------------------------------------------------------------------------

#define _REENTRANT 1
#define __STDC_FORMAT_MACROS 1
#define __STDC_LIMIT_MACROS 1

#include <array>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <new>
#include <cstdint>
#include <cstdarg>
#include <clocale>
#include <ctime>
#include <cinttypes>
#include <memory>
#include <utility>

struct FileDeleter {
    void operator()(FILE* f) const { if (f) fclose(f); }
};
using FilePtr = std::unique_ptr<FILE, FileDeleter>;

struct PipeDeleter {
    void operator()(FILE* p) const { if (p) pclose(p); }
};
using PipePtr = std::unique_ptr<FILE, PipeDeleter>;
#include <unistd.h>
#include <cerrno>
#include <dirent.h>
#include <climits>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <iconv.h>
#include <fcntl.h>
#include <endian.h>
#include "version.h"
#include "Log.h"
//---------------------------------------------------------------------------
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define PtokaXVersionString PtokaX_VERSION
#define BUILD_NUMBER STRINGIFY(PtokaX_BUILD)
#define ModString "/ Alex82 Mod's support v.04 (part.)"

#ifndef USE_FLYLINKDC_EXT_JSON
#define USE_FLYLINKDC_EXT_JSON
#endif

#ifndef FLYLINKDC_USE_REDIR
#define FLYLINKDC_USE_REDIR
#endif

#ifndef FLYLINKDC_USE_HUB_SLOT_RATIO
// #define FLYLINKDC_USE_HUB_SLOT_RATIO
#endif

#ifdef USE_FLYLINKDC_EXT_JSON
inline const char g_sPtokaXTitle[] = "PtokaX + json DC Hub for FlylinkDC++ " PtokaXVersionString // NOLINT(modernize-avoid-c-arrays)
#else
inline const char g_sPtokaXTitle[] = "PtokaX DC Hub for FlylinkDC++" PtokaXVersionString // NOLINT(modernize-avoid-c-arrays)
#endif // USE_FLYLINKDC_EXT_JSON
                                     " [build " BUILD_NUMBER "] " ModString;
//---------------------------------------------------------------------------
#endif
