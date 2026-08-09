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
#ifndef UserH
#define UserH
//---------------------------------------------------------------------------

#include <array>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

//---------------------------------------------------------------------------
struct User; // needed for next struct, and next struct must be defined before user :-/
//---------------------------------------------------------------------------

struct DcCommand
{
    User* m_pUser = nullptr;

    char* m_sCommand = nullptr;

    uint32_t m_ui32CommandLen = 0;
};
//---------------------------------------------------------------------------

struct UserBan
{
    UserBan() = default;

    std::string m_sMessage;

    uint32_t m_ui32NickHash = 0;

    [[nodiscard]] static auto CreateUserBan(const char* sMess, uint32_t ui32MessLen, uint32_t ui32Hash) -> std::unique_ptr<UserBan>;

    UserBan(const UserBan&) = delete;

    auto operator=(const UserBan&) -> UserBan& = delete;
};
//---------------------------------------------------------------------------

struct LoginLogout
{
    LoginLogout() = default;
    ~LoginLogout();
    void Clean();

    uint64_t m_ui64LogonTick = 0, m_ui64IPv4CheckTick = 0;

    std::unique_ptr<UserBan> m_pBan;

    std::vector<char> m_Buffer;

    uint32_t m_ui32ToCloseLoops = 0, m_ui32UserConnectedLen = 0;
    LoginLogout(const LoginLogout&) = delete;
    auto operator=(const LoginLogout&) -> LoginLogout& = delete;
};
//---------------------------------------------------------------------------

struct PrcsdUsrCmd
{
    PrcsdUsrCmd() = default;
    ~PrcsdUsrCmd()
    {
#ifdef FLYLINKDC_USE_STAT_RELOCATION
        if (m_ui32CountRelocation > 1)
        {
            spdlog::info("~PrcsdUsrCmd this = {} m_ui32LenMax = {}, count = {} ui8Type = {}",
                         static_cast<const void*>(this),
                         m_ui32LenMax,
                         m_ui32CountRelocation,
                         static_cast<int>(ui8Type));
        }
#endif
    }
    enum PrcsdCmdsIds : uint8_t
    {
        CTM_MCTM_RCTM_SR_TO,
        SUPPORTS,
        LOGINHELLO,
        GETPASS,
        CHAT,
        TO_OP_CHAT
    };

    void* m_pPtr = nullptr;

    std::vector<char> m_sCommand;

    uint32_t m_ui32Len = 0;
    uint8_t m_ui8Type = 0;

#ifdef FLYLINKDC_USE_STAT_RELOCATION
    uint32_t m_ui32LenMax = 0;
    uint32_t m_ui32CountRelocation = 0;
    void stat()
    {
        if (m_ui32LenMax < ui32Len)
        {
            m_ui32LenMax = ui32Len;
        }
        m_ui32CountRelocation++;
    }
#else
    static void stat() {}
#endif

    PrcsdUsrCmd(const PrcsdUsrCmd&) = delete;

    auto operator=(const PrcsdUsrCmd&) -> PrcsdUsrCmd& = delete;
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
struct User; // needed for next struct, and next struct must be defined before user :-/
//---------------------------------------------------------------------------

struct PrcsdToUsrCmd
{
    PrcsdToUsrCmd() = default;

    User* m_pToUser = nullptr;

    std::string m_sCommand;
    std::string m_nick;

    uint32_t m_ui32PmCount = 0, m_ui32Loops = 0;
    PrcsdToUsrCmd(const PrcsdToUsrCmd&) = delete;
    auto operator=(const PrcsdToUsrCmd&) -> PrcsdToUsrCmd& = delete;
};
//---------------------------------------------------------------------------
struct QzBuf; // for send queue
//---------------------------------------------------------------------------
#ifdef USE_FLYLINKDC_EXT_JSON
struct User;
class ExtJSONInfo
{
    std::string m_ExtJSON;
    std::string m_ExtJSONOriginal;
    uint64_t m_iLastExtJSONSendTick = 0;

public:
    ExtJSONInfo() = default;
    explicit ExtJSONInfo(const char* p_info)
    {
        if (p_info)
        {
            m_ExtJSON = p_info;
        }
    }
    [[nodiscard]] auto getLastExtJSONSendTick() const -> uint64_t
    {
        return m_iLastExtJSONSendTick;
    }
    void setLastExtJSONSendTick(uint64_t p_tick)
    {
        m_iLastExtJSONSendTick = p_tick;
    }
    [[nodiscard]] auto ComparExtJSON(const char* sNewExtJSON, const uint16_t ui16NewExtJSONLen) const -> bool
    {
        if (!m_ExtJSONOriginal.empty())
        {
            return m_ExtJSONOriginal == std::string_view(sNewExtJSON, ui16NewExtJSONLen);
        }

        return m_ExtJSON == std::string_view(sNewExtJSON, ui16NewExtJSONLen);
    }
    void SetJSONOriginal(const std::string& p_json)
    {
        m_ExtJSON = p_json;
        m_ExtJSONOriginal = p_json;
    }
    [[nodiscard]] auto GetExtJSONCommand() const -> const std::string&
    {
        return m_ExtJSON;
    }
};
#endif

struct User
{
    uint64_t m_last_recv_tick = 0;

    bool m_is_proxy_user = false;
    bool m_is_bad_len_port = false;
    bool m_is_bad_len_number_myinfo = false;
    bool m_is_bad_port = false;
    bool m_is_ddos_udp = false;
    bool m_is_max_int8 = false;
    bool m_is_max_ip_len = false;

    [[nodiscard]] auto isBlockSearch() const -> bool
    {
        return m_is_bad_len_port || m_is_ddos_udp || m_is_bad_len_number_myinfo || m_is_max_int8 || m_is_max_ip_len;
    }

    uint64_t m_ui64SharedSize = 0, m_ui64ChangedSharedSizeShort = 0, m_ui64ChangedSharedSizeLong = 0;
    uint64_t m_ui64GetNickListsTick = 0, m_ui64MyINFOsTick = 0, m_ui64SearchsTick = 0, m_ui64ChatMsgsTick = 0;
    uint64_t m_ui64PMsTick = 0, m_ui64SameSearchsTick = 0, m_ui64SamePMsTick = 0, m_ui64SameChatsTick = 0;
    uint64_t m_ui64LastMyINFOSendTick = 0, m_ui64LastNicklist = 0, m_ui64ReceivedPmTick = 0, m_ui64ChatMsgsTick2 = 0;
    uint64_t m_ui64PMsTick2 = 0, m_ui64SearchsTick2 = 0, m_ui64MyINFOsTick2 = 0, m_ui64CTMsTick = 0;
    uint64_t m_ui64CTMsTick2 = 0, m_ui64RCTMsTick = 0, m_ui64RCTMsTick2 = 0, m_ui64SRsTick = 0;
    uint64_t m_ui64SRsTick2 = 0, m_ui64RecvsTick = 0, m_ui64RecvsTick2 = 0, m_ui64ChatIntMsgsTick = 0;
    uint64_t m_ui64PMsIntTick = 0, m_ui64SearchsIntTick = 0;

    time_t m_tLoginTime = 0;
    LoginLogout m_LogInOut;

    std::list<std::unique_ptr<PrcsdToUsrCmd>> m_CmdToUserList;

    std::list<std::unique_ptr<PrcsdUsrCmd>> m_CmdList;

    PrcsdUsrCmd *m_pCmdActive4Search = nullptr, *m_pCmdActive6Search = nullptr, *m_pCmdPassiveSearch = nullptr;

#ifdef USE_FLYLINKDC_EXT_JSON
    void SendCharDelayedExtJSON();
    void SetExtJSONOriginal(const char* sNewExtJSON, uint16_t ui16NewExtJSONLen);
    [[nodiscard]] auto ComparExtJSON(const char* sNewExtJSON, const uint16_t ui16NewExtJSONLen) const -> bool
    {
        if (!m_user_ext_info)
        {
            return true;
        }

        return m_user_ext_info->ComparExtJSON(sNewExtJSON, ui16NewExtJSONLen);
    }
    [[nodiscard]] auto isSupportExtJSON() const -> bool
    {
        return m_is_json_user; // (m_ui32SupportBits & SUPPORTBIT_EXTJSON2) == SUPPORTBIT_EXTJSON2;
    }
#endif
    User *m_pHashIpTablePrev = nullptr, *m_pHashIpTableNext = nullptr;

    std::string m_sNick;

#ifdef FLYLINKDC_USE_VERSION
    std::string m_sVersion;
#endif
    std::vector<char> m_sMyInfoOriginal, m_sMyInfoShort, m_sMyInfoLong;
    std::string_view m_sDescription, m_sTag, m_sConnection, m_sEmail;
    std::string_view m_sClient, m_sTagVersion;
    std::string m_sLastChat, m_sLastPM;
    std::unique_ptr<char[]> m_pSendBuf;  // NOLINT(modernize-avoid-c-arrays)
    std::unique_ptr<char[]> m_pRecvBuf;  // NOLINT(modernize-avoid-c-arrays)
    char* m_pSendBufHead = nullptr;
    std::string m_sChangedDescriptionShort, m_sChangedDescriptionLong, m_sChangedTagShort, m_sChangedTagLong;
    std::string m_sChangedConnectionShort, m_sChangedConnectionLong, m_sChangedEmailShort, m_sChangedEmailLong;

    uint32_t m_ui32Recvs = 0, m_ui32Recvs2 = 0;

    uint32_t m_ui32Hubs = 0, m_ui32Slots = 0, m_ui32OLimit = 0, m_ui32LLimit = 0, m_ui32DLimit = 0, m_ui32NormalHubs = 0, m_ui32RegHubs = 0, m_ui32OpHubs = 0;
    uint32_t m_ui32SendCalled = 0, m_ui32RecvCalled = 0, m_ui32ReceivedPmCount = 0, m_ui32SR = 0, m_ui32DefloodWarnings = 0;
    uint32_t m_ui32BoolBits = 0, m_ui32InfoBits = 0, m_ui32SupportBits = 0;

    uint32_t m_ui32SendBufLen = 0, m_ui32RecvBufLen = 0, m_ui32SendBufDataLen = 0, m_ui32RecvBufDataLen = 0;

    uint32_t m_ui32NickHash = 0;

    int32_t m_i32Profile = -1;

    std::string m_LastSearch;

    int m_Socket = -1;

    uint16_t m_ui16MyInfoOriginalLen = 0, m_ui16MyInfoShortLen = 0, m_ui16MyInfoLongLen = 0;
    uint16_t m_ui16GetNickLists = 0, m_ui16MyINFOs = 0, m_ui16Searchs = 0, m_ui16ChatMsgs = 0, m_ui16PMs = 0;
    uint16_t m_ui16SameSearchs = 0, m_ui16SamePMs = 0;
    uint16_t m_ui16SameChatMsgs = 0, m_ui16LastPmLines = 0, m_ui16SameMultiPms = 0;
    uint16_t m_ui16LastChatLines = 0, m_ui16SameMultiChats = 0, m_ui16ChatMsgs2 = 0, m_ui16PMs2 = 0;
    uint16_t m_ui16Searchs2 = 0, m_ui16MyINFOs2 = 0, m_ui16CTMs = 0, m_ui16CTMs2 = 0;
    uint16_t m_ui16RCTMs = 0, m_ui16RCTMs2 = 0, m_ui16SRs = 0, m_ui16SRs2 = 0;
    uint16_t m_ui16ChatIntMsgs = 0, m_ui16PMsInt = 0, m_ui16SearchsInt = 0;
    uint16_t m_ui16IpTableIdx = 0;

    uint8_t m_ui8MagicByte = 0;

    uint8_t m_ui8IpLen = 0;
    uint8_t m_ui8Country = 0, m_ui8IPv4Len = 0;

    enum class UserStates : uint8_t
    {
        STATE_SOCKET_ACCEPTED,
        STATE_KEY_OR_SUP,
        STATE_VALIDATE,
        STATE_VERSION_OR_MYPASS,
        STATE_GETNICKLIST_OR_MYINFO,
        STATE_IPV4_CHECK,
        STATE_ADDME,
        STATE_ADDME_1LOOP,
        STATE_ADDME_2LOOP,
        STATE_ADDED,
        STATE_CLOSING,
        STATE_REMME
    };

    [[nodiscard]] static constexpr auto isPastLogin(UserStates s) noexcept -> bool
    {
        return std::to_underlying(s) >= std::to_underlying(UserStates::STATE_CLOSING);
    }

    UserStates m_ui8State = UserStates::STATE_SOCKET_ACCEPTED;

    bool m_is_invalid_json = false;
    bool m_is_json_user = false;

    std::array<uint8_t, 16> m_ui128IpHash = {};

    std::array<char, 40> m_sIP = {};
    std::array<char, 16> m_sIPv4 = {};

    std::array<char, 3> m_sModes = {};

    //  u->ui32BoolBits |= BIT_PRCSD_MYINFO;   <- set to 1
    //  u->ui32BoolBits &= ~BIT_PRCSD_MYINFO;  <- set to 0
    //  (u->ui32BoolBits & BIT_PRCSD_MYINFO) == BIT_PRCSD_MYINFO    <- test if is 1/true
    enum class UserBits : uint32_t
    {
        BIT_HASHED = 0x1,
        BIT_ERROR = 0x2,
        BIT_OPERATOR = 0x4,
        BIT_GAGGED = 0x8,
        BIT_GETNICKLIST = 0x10,
        BIT_IPV4_ACTIVE = 0x20,
        BIT_OLDHUBSTAG = 0x40,
        BIT_TEMP_OPERATOR = 0x80,
        BIT_PINGER = 0x100,
        BIT_BIG_SEND_BUFFER = 0x200,
        BIT_HAVE_SUPPORTS = 0x400,
        BIT_HAVE_BADTAG = 0x800,
        BIT_HAVE_GETNICKLIST = 0x1000,
        BIT_HAVE_BOTINFO = 0x2000,
        BIT_HAVE_KEY = 0x4000,
        BIT_HAVE_SHARECOUNTED = 0x8000,
        BIT_PRCSD_MYINFO = 0x10000,
        BIT_RECV_FLOODER = 0x20000,
        BIT_QUACK_SUPPORTS = 0x400000,
        BIT_IPV6 = 0x800000,
        BIT_IPV4 = 0x1000000,
        BIT_WAITING_FOR_PASS = 0x2000000,
        BIT_WARNED_WRONG_IP = 0x4000000,
        BIT_IPV6_ACTIVE = 0x8000000,
        BIT_CHAT_INSERT = 0x10000000,
#ifdef USE_FLYLINKDC_EXT_JSON
        BIT_PRCSD_EXT_JSON = 0x20000000
#endif
    };
    using enum UserBits;

    enum class UserInfoBits : uint32_t
    {
        INFOBIT_DESCRIPTION_CHANGED = 0x1,
        INFOBIT_TAG_CHANGED = 0x2,
        INFOBIT_CONNECTION_CHANGED = 0x4,
        INFOBIT_EMAIL_CHANGED = 0x8,
        INFOBIT_SHARE_CHANGED = 0x10,
        INFOBIT_DESCRIPTION_SHORT_PERM = 0x20,
        INFOBIT_DESCRIPTION_LONG_PERM = 0x40,
        INFOBIT_TAG_SHORT_PERM = 0x80,
        INFOBIT_TAG_LONG_PERM = 0x100,
        INFOBIT_CONNECTION_SHORT_PERM = 0x200,
        INFOBIT_CONNECTION_LONG_PERM = 0x400,
        INFOBIT_EMAIL_SHORT_PERM = 0x800,
        INFOBIT_EMAIL_LONG_PERM = 0x1000,
        INFOBIT_SHARE_SHORT_PERM = 0x2000,
        INFOBIT_SHARE_LONG_PERM = 0x4000,
        // alex82 ... HideUser / ������� �����
        INFOBIT_HIDDEN = 0x8000,
        // alex82 ... NoQuit / ��������� $Quit ��� �����
        INFOBIT_NO_QUIT = 0x10000,
        // alex82 ... HideUserKey / ������ ���� �����
        INFOBIT_HIDE_KEY = 0x20000,
        // alex82 feature...
    };
    using enum UserInfoBits;

    enum class UserSupportBits : uint16_t
    {
        SUPPORTBIT_NOGETINFO = 0x1,
        SUPPORTBIT_USERCOMMAND = 0x2,
        SUPPORTBIT_NOHELLO = 0x4,
        SUPPORTBIT_QUICKLIST = 0x8,
        SUPPORTBIT_USERIP2 = 0x10,
        SUPPORTBIT_ZPIPE = 0x20,
        SUPPORTBIT_IP64 = 0x40,
        SUPPORTBIT_IPV4 = 0x80,
        SUPPORTBIT_TLS2 = 0x100,
        SUPPORTBIT_ZPIPE0 = 0x200,
#ifdef USE_FLYLINKDC_EXT_JSON
    // Bug SUPPORTBIT_EXTJSON2                  = 0x400,
#endif
        SUPPORTBIT_HUBURL = 0x800
    };
    using enum UserSupportBits;

    // Bitwise operators for bitmask enum classes interoperating with uint32_t
#define DECLARE_BITMASK_ENUM(EnumType)                                                                                                                         \
    friend constexpr uint32_t operator|(uint32_t a, EnumType b)                                                                                                \
    {                                                                                                                                                          \
        return a | static_cast<uint32_t>(b);                                                                                                                   \
    }                                                                                                                                                          \
    friend constexpr uint32_t operator|(EnumType a, uint32_t b)                                                                                                \
    {                                                                                                                                                          \
        return static_cast<uint32_t>(a) | b;                                                                                                                   \
    }                                                                                                                                                          \
    friend constexpr uint32_t operator|(EnumType a, EnumType b)                                                                                                \
    {                                                                                                                                                          \
        return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);                                                                                            \
    }                                                                                                                                                          \
    friend constexpr uint32_t operator&(uint32_t a, EnumType b)                                                                                                \
    {                                                                                                                                                          \
        return a & static_cast<uint32_t>(b);                                                                                                                   \
    }                                                                                                                                                          \
    friend constexpr uint32_t operator&(EnumType a, uint32_t b)                                                                                                \
    {                                                                                                                                                          \
        return static_cast<uint32_t>(a) & b;                                                                                                                   \
    }                                                                                                                                                          \
    friend constexpr uint32_t operator&(EnumType a, EnumType b)                                                                                                \
    {                                                                                                                                                          \
        return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);                                                                                            \
    }                                                                                                                                                          \
    friend constexpr uint32_t& operator|=(uint32_t& a, EnumType b)                                                                                             \
    {                                                                                                                                                          \
        return a |= static_cast<uint32_t>(b);                                                                                                                  \
    }                                                                                                                                                          \
    friend constexpr uint32_t& operator&=(uint32_t& a, EnumType b)                                                                                             \
    {                                                                                                                                                          \
        return (a &= static_cast<uint32_t>(b));                                                                                                                \
    }                                                                                                                                                          \
    friend constexpr EnumType operator~(EnumType a)                                                                                                            \
    {                                                                                                                                                          \
        return static_cast<EnumType>(~(static_cast<uint32_t>(a)));                                                                                             \
    }                                                                                                                                                          \
    friend constexpr bool operator==(uint32_t a, EnumType b)                                                                                                   \
    {                                                                                                                                                          \
        return a == static_cast<uint32_t>(b);                                                                                                                  \
    }                                                                                                                                                          \
    friend constexpr bool operator==(EnumType a, uint32_t b)                                                                                                   \
    {                                                                                                                                                          \
        return static_cast<uint32_t>(a) == b;                                                                                                                  \
    }                                                                                                                                                          \
    friend constexpr bool operator!=(uint32_t a, EnumType b)                                                                                                   \
    {                                                                                                                                                          \
        return a != static_cast<uint32_t>(b);                                                                                                                  \
    }                                                                                                                                                          \
    friend constexpr bool operator!=(EnumType a, uint32_t b)                                                                                                   \
    {                                                                                                                                                          \
        return static_cast<uint32_t>(a) != b;                                                                                                                  \
    }

    DECLARE_BITMASK_ENUM(UserBits)
    DECLARE_BITMASK_ENUM(UserInfoBits)
    DECLARE_BITMASK_ENUM(UserSupportBits)

#undef DECLARE_BITMASK_ENUM

    [[nodiscard]] auto isSupportZpipe() const -> bool
    {
        return (m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE;
    }
    void SendTextDelayed(const std::string& sText);

#ifdef USE_FLYLINKDC_EXT_JSON
    std::unique_ptr<ExtJSONInfo> m_user_ext_info;

    void initExtJSON(const char* p_json)
    {
        if (!m_user_ext_info)
        {
            m_user_ext_info = std::make_unique<ExtJSONInfo>(p_json);
            m_ui32BoolBits |= User::BIT_PRCSD_EXT_JSON;
        }
    }

    [[nodiscard]] auto getLastExtJSONSendTick() const -> uint64_t
    {
        return m_user_ext_info ? m_user_ext_info->getLastExtJSONSendTick() : 0;
    }
    void setLastExtJSONSendTick(uint64_t p_tick)
    {
        if (m_user_ext_info)
        {
            m_user_ext_info->setLastExtJSONSendTick(p_tick);
        }
    }
#endif
    User();
    ~User();

    [[nodiscard]] auto MakeLock() -> bool;
    [[nodiscard]] auto DoRecv() -> bool;

    void SendChar(const char* sText, size_t szTextLen);
    void SendChar(const std::string& p_Text)
    {
        if (!p_Text.empty())
        {
            SendChar(p_Text.c_str(), p_Text.size());
        }
    }
    void SendCharDelayed(const std::string& p_Text)
    {
        if (!p_Text.empty())
        {
            SendCharDelayed(p_Text.c_str(), p_Text.size());
        }
    }

    void SendCharDelayed(const char* sText, size_t szTextLen);
    void SendFormat(const char* sFrom, bool bDelayed, const char* sFormatMsg, ...);
    void SendFormatCheckPM(const char* sFrom, const char* sOtherNick, bool bDelayed, const char* sFormatMsg, ...);

    [[nodiscard]] auto PutInSendBuf(const char* sText, size_t szTxtLen) -> bool;
    [[nodiscard]] auto Try2Send() -> bool;

    void SetIP(const char* sIP);
    void SetNick(std::string_view sNick);
    void SetMyInfoOriginal(const char* sMyInfo, uint16_t ui16MyInfoLen);
    void SetVersion(const char* sVersion);
    void SetLastChat(std::string_view sData);
    void SetLastPM(std::string_view sData);
    void SetLastSearch(std::string_view sData);
    void SetBuffer(const char* sKickMsg, size_t szLen = 0);
    void FreeBuffer();

    void Close(bool bNoQuit = false);

    void Add2Userlist();
    void AddUserList();

    [[nodiscard]] auto GenerateMyInfoLong() -> bool;
    [[nodiscard]] auto GenerateMyInfoShort() -> bool;

    void HasSuspiciousTag();

    [[nodiscard]] auto ProcessRules() -> bool;

    void AddPrcsdCmd(uint8_t ui8Type, const char* sCommand, size_t szCommandLen, void* pExtraData, bool bIsPm = false);

    void AddMeOrIPv4Check();

    void SendCompressedOrPlain(const char* pData, uint32_t ui32DataLen, std::vector<char>& vZData, uint32_t& ui32ZDataLen);

    static void SetUserInfo(std::string& sOldData, std::string_view sNewData);

    void RemFromSendBuf(const char* sData, uint32_t ui32Len, uint32_t ui32SendBufLen);

    static void DeletePrcsdUsrCmd(PrcsdUsrCmd*& pCommand); //[+]FlylinkDC++

    User(const User&) = delete;

    auto operator=(const User&) -> User& = delete;
};
//---------------------------------------------------------------------------

#endif
