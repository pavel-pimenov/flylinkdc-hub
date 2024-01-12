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

#ifndef _WIN32
#define _REENTRANT 1
#define __STDC_FORMAT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#endif
//---------------------------------------------------------------------------
#include <stdlib.h>
#ifdef _WIN32
#include <malloc.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <new>
#include <stdint.h>
#include <stdarg.h>
#ifdef _WIN32
#include <dos.h>

#pragma warning(disable: 4996) // Deprecated stricmp

#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _BUILD_GUI
#include <commctrl.h>
#include <RichEdit.h>
#include <Windowsx.h>
#endif
#endif
#include <locale.h>
#include <time.h>
#include <inttypes.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#if defined(__SVR4) && defined(__sun)
#include <sys/filio.h>
#endif
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <iconv.h>
#endif
#include <fcntl.h>
#ifdef _WIN32
#define TIXML_USE_STL
#endif
#ifdef _WIN32
#define PSAPI_VERSION 1 // [+]FlylinkDC++
#include <psapi.h>
#include <io.h>
#include <Iphlpapi.h>
#elif __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#include <libkern/OSByteOrder.h>

#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#elif __HAIKU__
#include <support/ByteOrder.h>
#define htobe64(x) B_HOST_TO_BENDIAN_INT64(x)
#define be64toh(x) B_BENDIAN_TO_HOST_INT64(x)
#elif defined(__SVR4) && defined(__sun)
#define htobe64(x) htonll(x)
#define be64toh(x) ntohll(x)
#elif defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/endian.h>
#else // Linux, OpenBSD
#include <endian.h>
#endif
#include "pxstring.h"
//---------------------------------------------------------------------------
#define PtokaXVersionString "0.5.354"
#define BUILD_NUMBER "669"
#define ModString "/ Alex82 Mod's support v.04 (part.)"

#ifndef USE_FLYLINKDC_EXT_JSON
#define USE_FLYLINKDC_EXT_JSON
#endif

#ifndef FLYLINKDC_USE_REDIR
#define FLYLINKDC_USE_REDIR
#endif

#ifndef FLYLINKDC_USE_HUB_SLOT_RATIO
//#define FLYLINKDC_USE_HUB_SLOT_RATIO
#endif

#ifdef USE_FLYLINKDC_EXT_JSON
const char g_sPtokaXTitle[] = "PtokaX + json DC Hub for FlylinkDC++ " PtokaXVersionString
#else
const char g_sPtokaXTitle[] = "PtokaX DC Hub for FlylinkDC++" PtokaXVersionString
#endif // USE_FLYLINKDC_EXT_JSON
                              " [build " BUILD_NUMBER "] " ModString;
#ifdef _WIN32
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif
//---------------------------------------------------------------------------
// http://stackoverflow.com/questions/20026445/editing-googles-c-disallow-copy-and-assign-preprocessor-macro-for-c11-move
//
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    private:                   \
    TypeName(const TypeName&);                 \
    void operator=(const TypeName&)

/*
//---------------------------------------------------------------------------
inline void ptokax_mem_log(const char *log)
{
    FILE * fw = fopen("ptokax_mem.log", "a");
    if (fw)
    {
        time_t acc_time;
        time(&acc_time);
        struct tm * acc_tm;
        acc_tm = localtime(&acc_time);
        char sBuf[64];
        strftime(sBuf, 64, "%c", acc_tm);
        fprintf(fw, "%s\t%s", sBuf, log);
        fclose(fw);
    }
}
//---------------------------------------------------------------------------
inline void* ptokax_malloc(size_t size, const char *file, int line, const char *func)
{
    void *p = malloc(size);
    char log[1024];
    snprintf(log, sizeof(log), "ptokax_malloc = %s, %i, %s, %p, %li\r\n", file, line, func, p, size);
    ptokax_mem_log(log);
    return p;
}
inline void* ptokax_calloc(size_t num, size_t size, const char *file, int line, const char *func)
{
    void *p = calloc(num,size);
    char log[1024];
    snprintf(log, sizeof(log), "ptokax_calloc = %s, %i, %s, %p, %li, %li\r\n", file, line, func, p, num,  size);
    ptokax_mem_log(log);
    return p;
}
inline void ptokax_free(void *ptr, const char *file, int line, const char *func)
{
    free(ptr);
    char log[1024];
    snprintf(log, sizeof(log), "ptokax_free = %s, %i, %s, %p\r\n", file, line, func, ptr);
    ptokax_mem_log(log);
}
inline void *ptokax_realloc(void *ptr, size_t size, const char *file, int line, const char *func)
{
    void *p = realloc(ptr,size);
    char log[1024];
    snprintf(log, sizeof(log), "ptokax_realloc = %s, %i, %s, %p -> %p, %li\r\n", file, line, func, ptr, p, size);
    ptokax_mem_log(log);
    return p;
}
#define malloc(X) ptokax_malloc( (X), __FILE__, __LINE__, __FUNCTION__)
#define calloc(X,Y) ptokax_calloc( (X), (Y), __FILE__, __LINE__, __FUNCTION__)
#define free(X) ptokax_free( (X), __FILE__, __LINE__, __func__)
#define realloc(X,Y) ptokax_realloc( (X), (Y), __FILE__, __LINE__, __FUNCTION__)
*/

#endif
