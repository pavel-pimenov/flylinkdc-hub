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
#ifndef utilityH
#define utilityH

#include <bit>
#include <cstdarg>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

// Case-insensitive string comparison for std::string_view
[[nodiscard]] inline auto iequals(std::string_view a, std::string_view b) noexcept -> bool
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); i++)
    {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);
        if (ca != cb)
        {
            const auto la = (ca >= 'A' && ca <= 'Z') ? static_cast<unsigned char>(ca + 32) : ca;
            const auto lb = (cb >= 'A' && cb <= 'Z') ? static_cast<unsigned char>(cb + 32) : cb;
            if (la != lb)
            {
                return false;
            }
        }
    }
    return true;
}

// Check if string_view contains any of the given characters
[[nodiscard]] inline auto contains_any(std::string_view sv, std::string_view chars) noexcept -> bool
{
    for (const char c : sv)
    {
        for (const char target : chars)
        {
            if (c == target)
            {
                return true;
            }
        }
    }
    return false;
}

//---------------------------------------------------------------------------
struct BanItem;
struct RangeBanItem;
struct User;
//---------------------------------------------------------------------------
constexpr size_t PTOKAX_GLOBAL_BUFF_SIZE = 131072 * 2;

// Time constants
constexpr uint64_t SECONDS_PER_MINUTE = 60;
constexpr uint64_t SECONDS_PER_HOUR = 3600;
constexpr uint64_t SECONDS_PER_DAY = 86400;
constexpr uint64_t SECONDS_PER_MONTH = 2592000;
constexpr uint64_t SECONDS_PER_YEAR = 31536000;
constexpr uint64_t MINUTES_PER_HOUR = 60;
constexpr uint64_t MINUTES_PER_DAY = 1440;
constexpr uint64_t MINUTES_PER_MONTH = 43200;
constexpr uint64_t MINUTES_PER_YEAR = 525600;

[[nodiscard]] inline auto px_str(const std::string& s) -> std::string
{
    return s;
}
[[nodiscard]] inline auto px_str(const char* s, const size_t len) -> std::string
{
    return (s) ? std::string(s, len) : "";
}
[[nodiscard]] inline auto px_str(const char* s) -> std::string
{
    return (s) ? std::string(s) : "";
}

// Append "<HubSec> " prefix to global buffer
[[nodiscard]] auto AppendHubSecPrefix(int& iMsgLen) -> bool;

//---------------------------------------------------------------------------

[[nodiscard]] auto Lock2Key(const char* sLock) -> std::string;
[[nodiscard]] auto VerifyLockKey(const char* sLock, const char* sReceivedKey) -> bool;

[[nodiscard]] auto ErrnoStr(uint32_t ui32Error) -> const char*;

[[nodiscard]] auto formatBytes(uint64_t ui64Bytes) -> std::string;
[[nodiscard]] auto formatBytesPerSecond(uint64_t ui64Bytes) -> std::string;
[[nodiscard]] auto formatTime(uint64_t ui64Rest) -> std::string;
[[nodiscard]] auto formatSecTime(uint64_t ui64Rest) -> std::string;

[[nodiscard]] auto stristr(const char* str1, const char* str2) -> char*;

[[nodiscard]] auto isIP(const char* sIP) -> bool;

// XML error logging helper
void LogXmlError(const char* sFileName, const char* sErrorDesc, int iColumn, int iRow);

// Load XML config from cfg/ dir, handle missing/empty gracefully, fatal on parse errors
namespace tinyxml2
{
class XMLDocument;
class XMLElement;
} // namespace tinyxml2
[[nodiscard]] auto LoadXmlConfig(tinyxml2::XMLDocument& doc, const std::string& fileName) -> bool;

// XML helpers — reduce boilerplate for reading child elements
// Returns text of required child element, or nullptr if missing/null (caller should skip)
[[nodiscard]] auto XmlGetRequiredText(tinyxml2::XMLElement* parent, const char* tagName) -> const char*;
// Returns text of optional child element, or nullptr if missing
[[nodiscard]] auto XmlGetOptionalText(tinyxml2::XMLElement* parent, const char* tagName) -> const char*;
// Reads required child as int via safe_stoi, returns false if missing/invalid
[[nodiscard]] auto XmlGetRequiredInt(tinyxml2::XMLElement* parent, const char* tagName, int& outVal) -> bool;

// Append "\nLabel: data" to global buffer with bounds check
[[nodiscard]] auto AppendLabeledField(int& iMsgLen, int iLangId, const char* pData, size_t uiDataLen) -> bool;

// Append user online info (Status/time, IP, Share, Description, Tag, Connection, Email, Country) to global buffer
[[nodiscard]] auto BuildUserOnlineInfo(int& iMsgLen, const User* pUser) -> bool;

[[nodiscard]] constexpr auto HashNick(std::string_view sNick) -> uint32_t
{
    uint32_t h = 5381;

    for (const char ch : sNick)
    {
        const auto c = static_cast<unsigned char>(ch);
        const auto lc = (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c + 32) : c;
        h += (h << 5);
        h ^= lc;
    }

    return (h + 1);
}

class Hash128
{
    std::array<uint8_t, 16> m_ui128Hash{};

public:
    Hash128() = default;
    void init(const uint8_t* ui128Hash)
    {
        memcpy(m_ui128Hash.data(), ui128Hash, m_ui128Hash.size());
    }
    operator uint8_t*()
    {
        return m_ui128Hash.data();
    }
    [[nodiscard]] auto data() const -> const uint8_t*
    {
        return m_ui128Hash.data();
    }
    [[nodiscard]] constexpr auto compare(const uint8_t* ui128Hash) const -> bool
    {
        for (size_t i = 0; i < m_ui128Hash.size(); i++)
        {
            if (m_ui128Hash[i] != ui128Hash[i])
            {
                return false;
            }
        }
        return true;
    }
};

[[nodiscard]] auto HashIP(const char* sIP, uint8_t* ui128IpHash) -> bool;
[[nodiscard]] inline auto GetIpTableIdx(const uint8_t* ui128IpHash) noexcept -> uint16_t
{
    uint32_t h = 5381;

    for (uint8_t ui8i = 0; ui8i < 16; ui8i++)
    {
        const auto c = static_cast<unsigned char>(ui128IpHash[ui8i]);
        h += (h << 5);
        h ^= c;
    }

    h += 1;

    uint16_t ui16Idx = 0;
    memcpy(&ui16Idx, &h, sizeof(uint16_t));

    return ui16Idx;
}

// Alignment-safe check for IPv4-mapped IPv6 address (::ffff:a.b.c.d).
// Avoids casting uint8_t[16] to in6_addr* which may be misaligned (UBSan).
[[nodiscard]] constexpr auto IsV4Mapped(const uint8_t* ui128IpHash) -> bool
{
    return ui128IpHash[0] == 0 && ui128IpHash[1] == 0 && ui128IpHash[2] == 0 && ui128IpHash[3] == 0 && ui128IpHash[4] == 0 && ui128IpHash[5] == 0 &&
           ui128IpHash[6] == 0 && ui128IpHash[7] == 0 && ui128IpHash[8] == 0 && ui128IpHash[9] == 0 && ui128IpHash[10] == 0xFF && ui128IpHash[11] == 0xFF;
}

[[nodiscard]] auto GenerateBanMessage(BanItem* pBan, time_t tAccTime) -> int;
[[nodiscard]] auto GenerateRangeBanMessage(RangeBanItem* pRangeBan, time_t tAccTime) -> int;

[[nodiscard]] auto GenerateTempBanTime(uint8_t ui8Multiplyer, uint32_t ui32Time, time_t& tAccTime, time_t& tBanTime) -> bool;

[[nodiscard]] constexpr auto HaveOnlyNumbers(const char* sData, const uint16_t ui16Len) -> bool
{
    for (uint16_t ui16i = 0; ui16i < ui16Len; ui16i++)
    {
        if (isdigit(static_cast<unsigned char>(sData[ui16i])) == 0)
        {
            return false;
        }
    }
    return true;
}

// Safe integer parsing using from_chars (never throws)
[[nodiscard]] inline auto safe_stoi(const char* s, int& result) noexcept -> bool
{
    auto [ptr, ec] = std::from_chars(s, s + strlen(s), result);
    return ec == std::errc{} && ptr != s;
}

[[nodiscard]] inline auto safe_stoul(const char* s, unsigned long& result) noexcept -> bool
{
    auto [ptr, ec] = std::from_chars(s, s + strlen(s), result);
    return ec == std::errc{} && ptr != s;
}

// Safe multi-byte string comparison — replaces reinterpret_cast UB
template <size_t N>
[[nodiscard]] constexpr auto MatchBytes(const char* buf, const char (&literal)[N]) -> bool // NOLINT(modernize-avoid-c-arrays)
{
    return memcmp(buf, literal, N - 1) == 0; // N includes null terminator
}

// Safe comparison of 2 bytes at offset
[[nodiscard]] constexpr auto MatchU16(const char* buf, size_t offset, uint16_t val) -> bool
{
    uint16_t actual;
    memcpy(&actual, buf + offset, sizeof(val));
    return actual == val;
}

// Safe comparison of 4 bytes at offset
[[nodiscard]] constexpr auto MatchU32(const char* buf, size_t offset, uint32_t val) -> bool
{
    uint32_t actual;
    memcpy(&actual, buf + offset, sizeof(val));
    return actual == val;
}

// Safe comparison of 8 bytes at offset
[[nodiscard]] constexpr auto MatchU64(const char* buf, size_t offset, uint64_t val) -> bool
{
    uint64_t actual;
    memcpy(&actual, buf + offset, sizeof(val));
    return actual == val;
}

// Buffer alignment helper — ensures at least 1.5x growth to reduce reallocs
[[nodiscard]] constexpr auto Allign(size_t n) -> size_t
{
    return n + (n >> 1) + 1;
}

// Safe string copy into fixed-size std::array — replaces snprintf(buf, size, "%s", src)
template <size_t N>
inline void SafeStrCopy(std::array<char, N>& dest, const char* src) // NOLINT(modernize-avoid-c-arrays)
{
    auto len = strlen(src);
    if (len >= N)
    {
        len = N - 1;
    }
    memcpy(dest.data(), src, len);
    dest[len] = '\0';
}

// + alex82 ... from MOD
[[nodiscard]] auto CheckSprintf(int iRetVal, size_t szMax, const char* sMsg) -> bool;                   // CheckSprintf(imsgLen, 64, "UdpDebug::New");
[[nodiscard]] auto CheckSprintf1(int iRetVal, size_t szLenVal, size_t szMax, const char* sMsg) -> bool; // CheckSprintf1(iret, imsgLen, 64, "UdpDebug::New");

[[nodiscard]] auto FileExist(const char* sPath) -> bool;
[[nodiscard]] auto DirExist(const char* sPath) -> bool;

void CheckForIPv4();
void CheckForIPv6();

[[nodiscard]] auto GetMacAddress(const char* sIP, char* sMac) -> bool;

void CreateGlobalBuffer();
void DeleteGlobalBuffer();
[[nodiscard]] auto CheckAndResizeGlobalBuffer(size_t szWantedSize) -> bool;
void ReduceGlobalBuffer();

[[nodiscard]] auto HashPassword(const char* sPassword, size_t szPassLen, uint8_t* ui8PassHash) -> bool;
//[+]FlylinkDC++
[[nodiscard]] constexpr auto CalcHash(uint32_t ui32Hash) -> uint16_t
{
    return static_cast<uint16_t>(ui32Hash & 0xFFFF);
}

[[nodiscard]] auto WantAgain() -> bool;
[[nodiscard]] auto IsPrivateIP(const char* sIP) -> bool;

// Writes the entire buffer to the file, replacing any existing content.
// Returns true on success, false on any I/O error. Replaces the common
// fopen/fwrite/fclose pattern used across the codebase.
[[nodiscard]] auto WriteWholeFile(const std::string& sPath, std::string_view sData) -> bool;

// Reads the entire file into sOut. Returns true on success (including an
// empty existing file), false if the file cannot be opened or read.
// Replaces the common fopen/fseek/ftell/fread/fclose pattern.
[[nodiscard]] auto ReadWholeFile(const std::string& sPath, std::string& sOut) -> bool;
//---------------------------------------------------------------------------
//[+]FlylinkDC++
template <class T>
inline void safe_closesocket(T& p_socket)
{
    if (p_socket != -1)
    {
        close(p_socket);
        p_socket = -1;
    }
}

template <class T>
inline void shutdown_and_close(T& p_socket, int p_type)
{
    shutdown(p_socket, p_type);
    safe_closesocket(p_socket);
}

// Appends formatted text to buffer at offset. Returns true on success, false on snprintf error
// or when the output does not fit (truncation). On success, offset is advanced by the number
// of characters written; on failure offset is left unchanged.
[[nodiscard]] inline auto SnprintfAppend(char* buf, int& offset, size_t size, const char* fmt, ...) -> bool
{
    if (offset < 0 || static_cast<size_t>(offset) >= size)
    {
        return false;
    }
    const size_t uiAvail = size - static_cast<size_t>(offset);
    va_list args;
    va_start(args, fmt);
    const int iRet = vsnprintf(buf + offset, uiAvail, fmt, args);
    va_end(args);
    // vsnprintf returns the length it WOULD have written: iRet >= uiAvail means truncation.
    // All callers must treat false as failure - continuing with a truncated offset would
    // underflow (size - offset) into a huge size_t and corrupt memory past the buffer.
    if (iRet <= 0 || static_cast<size_t>(iRet) >= uiAvail)
    {
        return false;
    }
    offset += iRet;
    return true;
}

// Returns current monotonic clock time in milliseconds.
// Platform-specific clock access is isolated here so callers stay portable.
[[nodiscard]] auto NowMonotonicMs() -> uint64_t;

#endif
