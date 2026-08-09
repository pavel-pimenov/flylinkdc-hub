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
#ifndef HubCommandsH
#define HubCommandsH
//---------------------------------------------------------------------------
struct User;
struct BanItem;
struct RangeBanItem;
//---------------------------------------------------------------------------

struct ChatCommand
{
    User* m_pUser = nullptr;

    char* m_sCommand = nullptr;

    uint32_t m_ui32CommandLen = 0;

    bool m_bFromPM = false;
};

class HubCommands
{
private:
    static ChatCommand m_ChatCommand;

    enum class BanClearType : uint8_t
    {
        Temp,
        Perm,
        TempRange,
        PermRange
    };

    [[nodiscard]] static auto ClrBans(ChatCommand* pChatCommand, uint8_t ui8Profile, BanClearType eClearType, int iStatusLang, int iReplyLang) -> bool;
    [[nodiscard]] static auto AddRegUser(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Ban(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto BanIp(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto ClrTempBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto ClrPermBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto ClrRangeTempBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto ClrRangePermBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto CheckNickBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto CheckIpBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto CheckRangeBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Drop(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto DelRegUser(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Debug(ChatCommand* pChatCommand) -> bool;

    [[nodiscard]] static auto FullBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto FullBanIp(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto FullTempBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto FullTempBanIp(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto FullRangeBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto FullRangeTempBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Gag(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetInfo(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetIpInfo(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetTempBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetScripts(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetPermBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetRangeBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetRangePermBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto GetRangeTempBans(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Help(ChatCommand* pChatCommand) -> bool;

    [[nodiscard]] static auto MyIp(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto MassMsg(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto NickBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto NickTempBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Op(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto OpMassMsg(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Passwd(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto PermUnban(ChatCommand* pChatCommand) -> bool;

    [[nodiscard]] static auto RestartScripts(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Restart(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto ReloadTxt(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RestartScript(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RangeBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RangeTempBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RangeUnBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RangeTempUnBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RangePermUnBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RegNewUser(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Stats(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto StopScript(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto StartScript(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto TempBan(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto TempBanIp(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto TempUnban(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Topic(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Unban(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto Ungag(ChatCommand* pChatCommand) -> bool;

    [[nodiscard]] static auto Ban(ChatCommand* pChatCommand, bool bFull) -> bool;
    [[nodiscard]] static auto BanIp(ChatCommand* pChatCommand, bool bFull) -> bool;
    [[nodiscard]] static auto NickBan(ChatCommand* pChatCommand, const char* sReason) -> bool;
    [[nodiscard]] static auto TempBan(ChatCommand* pChatCommand, bool bFull) -> bool;
    [[nodiscard]] static auto TempBanIp(ChatCommand* pChatCommand, bool bFull) -> bool;
    [[nodiscard]] static auto TempNickBan(ChatCommand* pChatCommand, const char* sNick, char* sTime, uint16_t ui16TimeLen, const char* sReason, bool bNotNickBan = false)
        -> bool;
    [[nodiscard]] static auto RangeBan(ChatCommand* pChatCommand, bool bFull) -> bool;
    [[nodiscard]] static auto RangeTempBan(ChatCommand* pChatCommand, bool bFull) -> bool;
    [[nodiscard]] static auto RangeUnban(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto RangeUnban(ChatCommand* pChatCommand, uint8_t ui8Type) -> bool;

    static void SendNoPermission(ChatCommand* pChatCommand);
    [[nodiscard]] static auto CheckFromPm(ChatCommand* pChatCommand) -> int;
    [[nodiscard]] static auto PrepareReply(ChatCommand* pChatCommand) -> int;
    [[nodiscard]] static auto BeginBanList(ChatCommand* pChatCommand, uint8_t ui8Profile) -> int;
    static void UncountDeflood(ChatCommand* pChatCommand);

    // Deduplication helpers
    [[nodiscard]] static auto CheckPermission(ChatCommand* pChatCommand, uint8_t ui8Profile) -> bool;
    [[nodiscard]] static auto CheckMinLength(ChatCommand* pChatCommand, uint32_t ui32MinLen, const char* sFuncName, const char* sSyntax) -> bool;
    static void StripPrefix(ChatCommand* pChatCommand, uint32_t ui32Len);
    [[nodiscard]] static auto FullBanDelegate(ChatCommand* pChatCommand,
                                uint8_t ui8Profile,
                                uint32_t ui32MinLen,
                                uint32_t ui32PrefixLen,
                                const char* sFuncName,
                                const char* sSyntax,
                                bool (*pDelegate)(ChatCommand*, const bool)) -> bool;
    [[nodiscard]] static auto ShouldReplyPM(ChatCommand* pChatCommand) -> bool;
    [[nodiscard]] static auto CheckSelfPermission(ChatCommand* pChatCommand, int iLangId) -> bool;
    [[nodiscard]] static auto CheckHigherProfile(ChatCommand* pChatCommand, const User* pOtherUser, int iLangId1, int iLangId2 = -1) -> bool;
    [[nodiscard]] static auto GetHubSecPM(const ChatCommand* p) -> const char*;
    static void FormatBanEntry(std::string& out, uint32_t& num, const BanItem* pBan, bool bShowExpire);
    static void FormatRangeBanEntry(std::string& out, uint32_t& num, const RangeBanItem* pBan, bool bShowExpire);

    // Ban listing helpers — reduce duplication in GetBans/GetTempBans/GetPermBans/GetRangeBans/etc.
    // Returns number of bans listed. Removes expired temp bans from the list.
    [[nodiscard]] static uint32_t ListTempBans(std::string& out);
    [[nodiscard]] static uint32_t ListPermBans(std::string& out);
    [[nodiscard]] static uint32_t ListTempRangeBans(std::string& out);
    [[nodiscard]] static uint32_t ListPermRangeBans(std::string& out);
    [[nodiscard]] static auto FindUserOrReply(ChatCommand* pChatCommand, uint32_t ui32NickLen, const char* sFunc, int iLangId) -> User*;
    static void TruncateReason(char* s, uint32_t ui32MaxLen = 511);
    [[nodiscard]] static uint8_t ParseCmdParts(ChatCommand* pChatCommand, uint32_t ui32StartOffset, char* sParts[], uint16_t ui16PartLens[], uint8_t ui8MaxParts = 3); // NOLINT(modernize-avoid-c-arrays) variable-size call-site arrays
    static void CloseIpBannedUsers(ChatCommand* pChatCommand, const char* sReason);
    static void AppendMatchingRangeBans(std::string& Bans, uint32_t& iBanNum, const uint8_t* ui128IpHash, time_t acc_time);

public:
    [[nodiscard]] static auto DoCommand(User* pUser, char* sCommand, uint32_t ui32CmdLen, bool bFromPM = false) -> bool;
};
//---------------------------------------------------------------------------

#endif
