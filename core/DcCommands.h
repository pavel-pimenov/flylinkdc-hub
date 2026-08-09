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
#ifndef DcCommandsH
#define DcCommandsH

#include "utility.h"

#include <list>
#include <memory>

//---------------------------------------------------------------------------
struct User;
struct PrcsdUsrCmd;
struct PassBf;
struct DcCommand;
//---------------------------------------------------------------------------

class DcCommands
{
private:
    struct PassBf
    {
        int m_iCount = 1;

        Hash128 m_ui128IpHash;

        explicit PassBf(const uint8_t* ui128Hash);
        ~PassBf() = default;

        PassBf(const PassBf&) = delete;

        auto operator=(const PassBf&) -> PassBf& = delete;
    };

    std::list<std::unique_ptr<PassBf>> m_PasswdBfCheck;

    static void BotINFO(DcCommand* pDcCommand);
    static void ConnectToMe(DcCommand* pDcCommand, bool bMulti);
    void GetINFO(DcCommand* pDcCommand);
    [[nodiscard]] static auto GetNickList(DcCommand* pDcCommand) -> bool;
    static void Key(DcCommand* pDcCommand);
    static void Kick(DcCommand* pDcCommand);
    [[nodiscard]] static auto SearchDeflood(DcCommand* pDcCommand, bool bMulti) -> bool;
    static void Search(DcCommand* pDcCommand, bool bMulti);
    [[nodiscard]] static auto MyINFODeflood(DcCommand* pDcCommand) -> bool;
    [[nodiscard]] static auto MyINFO(DcCommand* pDcCommand) -> bool;
    void MyPass(DcCommand* pDcCommand);
    static void OpForceMove(DcCommand* pDcCommand);
    static void RevConnectToMe(DcCommand* pDcCommand);
    static void SR(DcCommand* pDcCommand);
    void Supports(DcCommand* pDcCommand);
    static void To(DcCommand* pDcCommand);
    static void ValidateNick(DcCommand* pDcCommand);
    static void Version(DcCommand* pDcCommand);
    [[nodiscard]] static auto ChatDeflood(DcCommand* pDcCommand) -> bool;
    static void Chat(DcCommand* pDcCommand);
    static void Close(DcCommand* pDcCommand);

    void Unknown(DcCommand* pDcCommand, bool bMyNick = false);
    void MyNick(DcCommand* pDcCommand);

    [[nodiscard]] static auto ValidateUserNick(DcCommand* pDcCommand, User* pUser, const char* sNick, size_t szNickLen, bool ValidateNick) -> bool;

    [[nodiscard]] auto Find(const uint8_t* ui128IpHash) -> PassBf*;
    void Remove(PassBf* pPassBfItem);

    [[nodiscard]] static auto CheckAndGetPort(const char* pPort, uint8_t ui8PortLen, uint32_t& ui32PortLen) -> uint16_t;
    static void SendIPFixedMsg(User* pUser, const char* pBadIP, const char* pRealIP);

    [[nodiscard]] static auto AddSearch(User* pUser, PrcsdUsrCmd* pCmdSearch, const char* sSearch, size_t szLen, bool bActive) -> PrcsdUsrCmd*;

    [[nodiscard]] auto TryBadStateClose(DcCommand* pDcCommand, const void* pRawTable, size_t szCount, uint8_t ui8Offset) -> bool;

#ifdef USE_FLYLINKDC_EXT_JSON
    [[nodiscard]] auto ExtJSONDeflood(User* pUser, const char* sData, uint32_t ui32Len, bool bCheck) -> bool;
    [[nodiscard]] static auto SetExtJSON(User* pUser, const char* sData, uint32_t ui32Len) -> bool;
    [[nodiscard]] static auto CheckExtJSON(User* pUser, const char* sData, uint32_t ui32Len) -> bool;
#endif
    [[nodiscard]] static auto ValidateUserNickFinally(bool pIsNotReg, User* pUser, size_t szNickLen, bool ValidateNick) -> bool; // [+] FlylinkDC++

public:
    static std::unique_ptr<DcCommands> m_Ptr;

    DcCommands(const DcCommands&) = delete;
    auto operator=(const DcCommands&) -> DcCommands& = delete;

    uint32_t m_ui32StatChat = 0, m_ui32StatCmdUnknown = 0, m_ui32StatCmdTo = 0, m_ui32StatCmdMyInfo = 0;
    uint32_t m_ui32StatCmdSearch = 0, m_ui32StatCmdSR = 0, m_ui32StatCmdRevCTM = 0, m_ui32StatCmdOpForceMove = 0;
    uint32_t m_ui32StatCmdMyPass = 0, m_ui32StatCmdValidate = 0, m_ui32StatCmdKey = 0, m_ui32StatCmdGetInfo = 0;
    uint32_t m_ui32StatCmdGetNickList = 0, m_ui32StatCmdConnectToMe = 0, m_ui32StatCmdVersion = 0, m_ui32StatCmdKick = 0;
    uint32_t m_ui32StatCmdSupports = 0, m_ui32StatBotINFO = 0, m_ui32StatZPipe = 0, m_ui32StatCmdMultiSearch = 0;
    uint32_t m_ui32StatCmdMultiConnectToMe = 0, m_ui32StatCmdClose = 0;

    uint32_t m_ui32StatBadState = 0;
    uint32_t m_ui32StatGarbageData = 0;
    uint32_t m_ui32StatNickSpoofing = 0;
    uint32_t m_ui32StatBruteforce = 0;
    uint32_t m_ui32StatFlood = 0;
    uint32_t m_ui32StatInvalidJson = 0;
    uint32_t m_ui32StatUnknownCmd = 0;

    uint64_t m_ui64TimeCmdChat = 0;
    uint64_t m_ui64TimeCmdSearch = 0;
    uint64_t m_ui64TimeCmdMyInfo = 0;
    uint64_t m_ui64TimeCmdTo = 0;
    uint64_t m_ui64TimeCmdSR = 0;
    uint64_t m_ui64TimeCmdKey = 0;
    uint64_t m_ui64TimeCmdValidateNick = 0;
    uint64_t m_ui64TimeCmdConnectToMe = 0;
    uint64_t m_ui64TimeCmdRevConnectToMe = 0;
    uint64_t m_ui64TimeCmdSupports = 0;
    uint64_t m_ui64TimeCmdVersion = 0;
    uint64_t m_ui64TimeCmdGetNickList = 0;
    uint64_t m_ui64TimeCmdGetINFO = 0;
    uint64_t m_ui64TimeCmdClose = 0;
    uint64_t m_ui64TimeCmdKick = 0;
    uint64_t m_ui64TimeCmdOpForceMove = 0;
    uint64_t m_ui64TimeCmdMyPass = 0;
    uint64_t m_ui64TimeCmdBotINFO = 0;
    uint64_t m_ui64TimeCmdOther = 0;
    uint64_t m_ui64TimeProcessCmds = 0;

    DcCommands() = default;
    ~DcCommands();

    void PreProcessData(DcCommand* pDcCommand);
    void ProcessCmds(User* pUser);

#ifdef FLYLINKDC_USE_UDP_THREAD
    static void SRFromUDP(DcCommand* pDcCommand);
#endif

#ifdef USE_FLYLINKDC_EXT_JSON
    uint32_t m_ui32StatCmdExtJSON = 0;
#endif
};
//---------------------------------------------------------------------------

#endif
