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
#include "hashRegManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "PXBReader.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
#include <tinyxml2.h>
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

std::unique_ptr<RegManager> RegManager::m_Ptr;
//---------------------------------------------------------------------------
static const char sPtokaXRegiteredUsers[] = "PtokaX Registered Users"; // NOLINT(modernize-avoid-c-arrays)
static constexpr size_t g_szPtokaXRegiteredUsersLen = sizeof(sPtokaXRegiteredUsers) - 1;
//---------------------------------------------------------------------------

// RegUser destructor is = default in header
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RegUser* RegUser::CreateReg(const char* sRegNick,
                            const size_t szRegNickLen,
                            const char* sRegPassword,
                            const size_t szRegPassLen,
                            const uint8_t* ui8RegPassHash,
                            const uint16_t ui16RegProfile)
{
    auto pReg = std::make_unique<RegUser>();
    if (!pReg)
    {
        LogDbg("[MEM] Cannot allocate new Reg in RegUser::CreateReg");

        return nullptr;
    }

    pReg->m_sNick = std::string(sRegNick, szRegNickLen);

    if (ui8RegPassHash)
    {
        pReg->m_vPassData.assign(ui8RegPassHash, ui8RegPassHash + 64);
        pReg->m_bPassHash = true;
    }
    else if (sRegPassword)
    {
        pReg->m_vPassData.assign(reinterpret_cast<const uint8_t*>(sRegPassword), reinterpret_cast<const uint8_t*>(sRegPassword) + szRegPassLen);
        pReg->m_vPassData.push_back('\0');
    }
    else
    {
        LogDbgErr("[ERR] Empty ui8RegPassHash and sRegPassword in RegUser::RegUser");

        return nullptr;
    }

    pReg->m_ui16Profile = ui16RegProfile;
    pReg->m_ui32Hash = HashNick(std::string_view(sRegNick, szRegNickLen));

    return pReg.release();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool RegUser::UpdatePassword(std::string_view sNewPass)
{
    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_HASH_PASSWORDS)])
    {
        if (m_bPassHash)
        {
            m_vPassData.assign(reinterpret_cast<const uint8_t*>(sNewPass.data()), reinterpret_cast<const uint8_t*>(sNewPass.data()) + sNewPass.size());
            m_vPassData.push_back('\0');
            m_bPassHash = false;
        }
        else
        {
            std::string_view stored(reinterpret_cast<const char*>(m_vPassData.data()));
            if (stored != sNewPass)
            {
                m_vPassData.assign(reinterpret_cast<const uint8_t*>(sNewPass.data()), reinterpret_cast<const uint8_t*>(sNewPass.data()) + sNewPass.size());
                m_vPassData.push_back('\0');
            }
        }
    }
    else
    {
        if (m_bPassHash)
        {
            if (!HashPassword(sNewPass.data(), sNewPass.size(), m_vPassData.data()))
            {
                LogDbgErr("[REG] Failed to hash password for {}", m_sNick);
            }
        }
        else
        {
            std::array<uint8_t, 64> ui8Hash = {};
            if (HashPassword(sNewPass.data(), sNewPass.size(), ui8Hash.data()))
            {
                m_vPassData.assign(ui8Hash.begin(), ui8Hash.end());
                m_bPassHash = true;
            }
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RegManager::RegManager() = default;
//---------------------------------------------------------------------------

RegManager::~RegManager() = default;
//---------------------------------------------------------------------------

bool RegManager::AddNew(const char* sNick, const char* sPasswd, const uint16_t iProfile)
{
    if (Find(sNick))
    {
        return false;
    }

    RegUser* pNewUser = nullptr;

    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_HASH_PASSWORDS)])
    {
        std::array<uint8_t, 64> ui8Hash = {};

        const size_t szPassLen = strlen(sPasswd);

        if (!HashPassword(sPasswd, szPassLen, ui8Hash.data()))
        {
            return false;
        }

        pNewUser = RegUser::CreateReg(sNick, strlen(sNick), nullptr, 0, ui8Hash.data(), iProfile);
    }
    else
    {
        pNewUser = RegUser::CreateReg(sNick, strlen(sNick), sPasswd, strlen(sPasswd), nullptr, iProfile);
    }

    if (!pNewUser)
    {
        LogDbg("[MEM] Cannot allocate pNewUser in RegManager::AddNew");

        return false;
    }

    Add(pNewUser);

    Save(true);

    if (!ServerManager::m_bServerRunning)
    {
        return true;
    }

    User* AddedUser = HashManager::m_Ptr->FindUser(pNewUser->m_sNick);

    if (AddedUser)
    {
        const bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT);
        AddedUser->m_i32Profile = iProfile;

        if (!((AddedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
        {
            if (ProfileManager::m_Ptr->IsAllowed(AddedUser, ProfileManager::HASKEYICON))
            {
                AddedUser->m_ui32BoolBits |= User::BIT_OPERATOR;
            }
            else
            {
                AddedUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
            }

            if (((AddedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
            {
                // alex82 ... HideUserKey / ������ ���� �����
                if (!((AddedUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
                {
                    Users::m_Ptr->Add2OpList(AddedUser);
                    GlobalDataQueue::m_Ptr->OpListStore(AddedUser->m_sNick.c_str());
                }
                if (bAllowedOpChat != ProfileManager::m_Ptr->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT))
                {
                    if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] &&
                        (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] || !SettingManager::m_Ptr->m_bBotsSameNick))
                    {
                        if (!((AddedUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO))
                        {
                            AddedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO)]);
                        }

                        AddedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO)]);
                        AddedUser->SendFormat("RegManager::AddNew", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
                    }
                }
            }
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void RegManager::Add(RegUser* pReg)
{
    Add2Table(pReg);

    m_RegList.emplace_back(pReg);

    return;
}
//---------------------------------------------------------------------------

void RegManager::Add2Table(RegUser* pReg)
{
    m_Table.emplace(pReg->m_ui32Hash, pReg);
}
//---------------------------------------------------------------------------

void RegManager::ChangeReg(RegUser* pReg, const char* sNewPasswd, const uint16_t ui16NewProfile)
{
    if (sNewPasswd)
    {
        if (!pReg->UpdatePassword(sNewPasswd))
        {
            LogDbgErr("[REG] Failed to update password for {}", pReg->m_sNick);
        }
    }

    pReg->m_ui16Profile = ui16NewProfile;

    RegManager::m_Ptr->Save(true);

    if (!ServerManager::m_bServerRunning)
    {
        return;
    }

    User* ChangedUser = HashManager::m_Ptr->FindUser(pReg->m_sNick);
    if (ChangedUser && ChangedUser->m_i32Profile != static_cast<int32_t>(ui16NewProfile))
    {
        const bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT);

        ChangedUser->m_i32Profile = static_cast<int32_t>(ui16NewProfile);

        if (((ChangedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) !=
            ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::HASKEYICON))
        {
            if (ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::HASKEYICON))
            {
                ChangedUser->m_ui32BoolBits |= User::BIT_OPERATOR;
                // alex82 ... HideUserKey / ������ ���� �����
                if (!((ChangedUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
                {
                    Users::m_Ptr->Add2OpList(ChangedUser);
                    GlobalDataQueue::m_Ptr->OpListStore(ChangedUser->m_sNick.c_str());
                }
            }
            else
            {
                ChangedUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
                // alex82 ... HideUserKey / ������ ���� �����
                if (!((ChangedUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY))
                {
                    // alex82 ... ��������� �������� OpList
                    const int imsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", ChangedUser->m_sNick.c_str());
                    if (CheckSprintf(imsgLen, 128, "RegManager::ChangeReg1"))
                    {
                        GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, imsgLen, nullptr, 0, GlobalDataQueue::Cmd::QUIT);
                    }
                    switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption)
                    {
                    case 0:
                        GlobalDataQueue::m_Ptr->AddQueueItem(
                            ChangedUser->m_sMyInfoLong.data(), ChangedUser->m_ui16MyInfoLongLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                        break;
                    case 1:
                        GlobalDataQueue::m_Ptr->AddQueueItem(ChangedUser->m_sMyInfoShort.data(),
                                                             ChangedUser->m_ui16MyInfoShortLen,
                                                             ChangedUser->m_sMyInfoLong.data(),
                                                             ChangedUser->m_ui16MyInfoLongLen,
                                                             GlobalDataQueue::Cmd::MYINFO);
                        break;
                    case 2:
                        GlobalDataQueue::m_Ptr->AddQueueItem(
                            ChangedUser->m_sMyInfoShort.data(), ChangedUser->m_ui16MyInfoShortLen, nullptr, 0, GlobalDataQueue::Cmd::MYINFO);
                        break;
                    default:
                        break;
                    }
                    Users::m_Ptr->DelFromOpList(ChangedUser->m_sNick.c_str());
                }
            }
        }

        if (bAllowedOpChat != ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT))
        {
            if (ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT))
            {
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] &&
                    (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] || !SettingManager::m_Ptr->m_bBotsSameNick))
                {
                    if (!((ChangedUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO))
                    {
                        ChangedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_HELLO)]);
                    }

                    ChangedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[std::to_underlying(SettingManager::SetPreTxtIds::SETPRETXT_OP_CHAT_MYINFO)]);
                    ChangedUser->SendFormat("RegManager::ChangeReg1", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
                }
            }
            else
            {
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] &&
                    (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] || !SettingManager::m_Ptr->m_bBotsSameNick))
                {
                    ChangedUser->SendFormat("RegManager::ChangeReg2", true, "$Quit %s|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void RegManager::Delete(RegUser* pReg, const bool /*bFromGui = false*/)
{
    if (ServerManager::m_bServerRunning)
    {
        User* pRemovedUser = HashManager::m_Ptr->FindUser(pReg->m_sNick);

        if (pRemovedUser)
        {
            pRemovedUser->m_i32Profile = -1;
            if (((pRemovedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR))
            {
                Users::m_Ptr->DelFromOpList(pRemovedUser->m_sNick.c_str());
                pRemovedUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;

                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_OP_CHAT)] &&
                    (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_REG_BOT)] || !SettingManager::m_Ptr->m_bBotsSameNick))
                {
                    pRemovedUser->SendFormat("RegManager::Delete", true, "$Quit %s|", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_OP_CHAT_NICK)].c_str());
                }
            }
        }
    }

    Rem(pReg);

    Save(true);
}
//---------------------------------------------------------------------------

void RegManager::Rem(RegUser* pReg)
{
    RemFromTable(pReg);

    for (auto it = m_RegList.begin(); it != m_RegList.end(); ++it)
    {
        if (it->get() == pReg)
        {
            m_RegList.erase(it);
            break;
        }
    }
}
//---------------------------------------------------------------------------

void RegManager::RemFromTable(RegUser* pReg)
{
    const auto range = m_Table.equal_range(pReg->m_ui32Hash);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second == pReg)
        {
            m_Table.erase(it);
            break;
        }
    }
}
//---------------------------------------------------------------------------

RegUser* RegManager::Find(std::string_view sNick) const
{
    const uint32_t ui32Hash = HashNick(sNick);

    const auto range = m_Table.equal_range(ui32Hash);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (iequals(it->second->m_sNick, sNick))
        {
            return it->second;
        }
    }

    return nullptr;
}
//---------------------------------------------------------------------------

RegUser* RegManager::Find(User* pUser) const
{
    const auto range = m_Table.equal_range(pUser->m_ui32NickHash);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (iequals(it->second->m_sNick, pUser->m_sNick))
        {
            return it->second;
        }
    }

    return nullptr;
}
//---------------------------------------------------------------------------

RegUser* RegManager::Find(const uint32_t ui32Hash, std::string_view sNick) const
{
    const auto range = m_Table.equal_range(ui32Hash);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (iequals(it->second->m_sNick, sNick))
        {
            return it->second;
        }
    }

    return nullptr;
}
//---------------------------------------------------------------------------

void RegManager::Load()
{
    if (!FileExist((ServerManager::m_sPath + "/cfg/RegisteredUsers.pxb").c_str()))
    {
        LoadXML();
        return;
    }

    const auto iProfilesCount = static_cast<uint16_t>(ProfileManager::m_Ptr->m_ui16ProfileCount - 1);

    PXBReader pxbRegs;

    // Open regs file
    if (!pxbRegs.OpenFileRead((ServerManager::m_sPath + "/cfg/RegisteredUsers.pxb").c_str(), 4))
    {
        return;
    }

    // Read file header
    std::array<uint16_t, 4> ui16Identificators = {};
    memcpy(ui16Identificators.data(), "FI", 2);
    memcpy(ui16Identificators.data() + 1, "FV", 2);
    ui16Identificators[2] = 0;
    ui16Identificators[3] = 0;

    if (!pxbRegs.ReadNextItem(ui16Identificators.data(), 2))
    {
        return;
    }

    // Check header if we have correct file
    if (pxbRegs.m_ui16ItemLengths[0] != g_szPtokaXRegiteredUsersLen ||
        strncmp(static_cast<const char*>(pxbRegs.m_pItemDatas[0]), sPtokaXRegiteredUsers, g_szPtokaXRegiteredUsersLen) != 0)
    {
        return;
    }

    {
        uint32_t ui32FileVersion;
        memcpy(&ui32FileVersion, pxbRegs.m_pItemDatas[1], sizeof(ui32FileVersion));
        ui32FileVersion = ntohl(ui32FileVersion);

        if (ui32FileVersion < 1)
        {
            return;
        }
    }

    // Read regs =)
    memcpy(ui16Identificators.data(), "NI", 2);
    memcpy(ui16Identificators.data() + 1, "PS", 2);
    memcpy(ui16Identificators.data() + 2, "PR", 2);
    memcpy(ui16Identificators.data() + 3, "PA", 2);

    uint16_t iProfile = UINT16_MAX;
    RegUser* pNewUser = nullptr;
    std::array<uint8_t, 64> ui8Hash = {};
    size_t szPassLen = 0;

    bool bSuccess = pxbRegs.ReadNextItem(ui16Identificators.data(), 3, 1);

    while (bSuccess)
    {
        if (pxbRegs.m_ui16ItemLengths[0] < 65 && pxbRegs.m_ui16ItemLengths[1] < 65 && pxbRegs.m_ui16ItemLengths[2] == 2)
        {
            uint16_t ui16Profile;
            memcpy(&ui16Profile, pxbRegs.m_pItemDatas[2], sizeof(ui16Profile));
            iProfile = ntohs(ui16Profile);

            if (iProfile > iProfilesCount)
            {
                iProfile = iProfilesCount;
            }

            pNewUser = nullptr;

            if (pxbRegs.m_ui16ItemLengths[3] != 0)
            {
                if (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_HASH_PASSWORDS)])
                {
                    szPassLen = static_cast<size_t>(pxbRegs.m_ui16ItemLengths[3]);

                    if (!HashPassword(static_cast<const char*>(pxbRegs.m_pItemDatas[3]), szPassLen, ui8Hash.data()))
                    {
                        pNewUser = RegUser::CreateReg(static_cast<const char*>(pxbRegs.m_pItemDatas[0]),
                                                      pxbRegs.m_ui16ItemLengths[0],
                                                      static_cast<const char*>(pxbRegs.m_pItemDatas[3]),
                                                      pxbRegs.m_ui16ItemLengths[3],
                                                      nullptr,
                                                      iProfile);
                    }
                    else
                    {
                        pNewUser = RegUser::CreateReg(
                            static_cast<const char*>(pxbRegs.m_pItemDatas[0]), pxbRegs.m_ui16ItemLengths[0], nullptr, 0, ui8Hash.data(), iProfile);
                    }
                }
                else
                {
                    pNewUser = RegUser::CreateReg(static_cast<const char*>(pxbRegs.m_pItemDatas[0]),
                                                  pxbRegs.m_ui16ItemLengths[0],
                                                  static_cast<const char*>(pxbRegs.m_pItemDatas[3]),
                                                  pxbRegs.m_ui16ItemLengths[3],
                                                  nullptr,
                                                  iProfile);
                }
            }
            else if (pxbRegs.m_ui16ItemLengths[1] == 64)
            {
#ifdef _WITHOUT_SKEIN
                LogDbg("[ERR] Hashed password found in RegisteredUsers, but PtokaX is compiled without hashing support!");

                exit(EXIT_FAILURE);
#endif
                pNewUser = RegUser::CreateReg(static_cast<const char*>(pxbRegs.m_pItemDatas[0]),
                                              pxbRegs.m_ui16ItemLengths[0],
                                              nullptr,
                                              0,
                                              static_cast<const uint8_t*>(pxbRegs.m_pItemDatas[1]),
                                              iProfile);
            }

            if (!pNewUser)
            {
                const std::string l_nick(static_cast<const char*>(pxbRegs.m_pItemDatas[0]), pxbRegs.m_ui16ItemLengths[0]);
                LogDbg("[MEM] Cannot allocate pNewUser in RegManager::Load user = {}", l_nick);
            }
            else
            {
                Add(pNewUser);
            }
        }

        bSuccess = pxbRegs.ReadNextItem(ui16Identificators.data(), 3, 1);
    }
}
//---------------------------------------------------------------------------

void RegManager::LoadXML()
{
    const auto iProfilesCount = static_cast<uint16_t>(ProfileManager::m_Ptr->m_ui16ProfileCount - 1);

    tinyxml2::XMLDocument doc;
    if (LoadXmlConfig(doc, "RegisteredUsers.xml"))
    {
        tinyxml2::XMLHandle cfg(&doc);
        tinyxml2::XMLNode* registeredusers = cfg.FirstChildElement("RegisteredUsers").ToNode();
        if (registeredusers)
        {
            bool bIsBuggy = false;
            tinyxml2::XMLElement* child = registeredusers->FirstChildElement();

            while (child)
            {
                const char* nick = XmlGetRequiredText(child, "Nick");
                if (!nick || strlen(nick) > 64)
                {
                    child = child->NextSiblingElement();
                    continue;
                }

                const char* pass = XmlGetRequiredText(child, "Password");
                if (!pass || strlen(pass) > 64)
                {
                    child = child->NextSiblingElement();
                    continue;
                }

                int iProfile = 0;
                if (!XmlGetRequiredInt(child, "Profile", iProfile))
                {
                    child = child->NextSiblingElement();
                    continue;
                }
                if (iProfile > iProfilesCount)
                {
                    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                           ServerManager::m_szGlobalBufferSize,
                                           "%s %s %s! %s %s.",
                                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USER)].c_str(),
                                           nick,
                                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_HAVE_NOT_EXIST_PROFILE)].c_str(),
                                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_CHANGED_PROFILE_TO)].c_str(),
                                           ProfileManager::m_Ptr->m_vpProfilesTable[iProfilesCount]->m_sName.c_str());
                    if (iMsgLen > 0)
                    {
                        LogInfo("{}", ServerManager::m_pGlobalBuffer);
                    }

                    iProfile = iProfilesCount;
                    bIsBuggy = true;
                }

                if (!Find(nick))
                {
                    RegUser* pNewUser = RegUser::CreateReg(nick, strlen(nick), pass, strlen(pass), nullptr, iProfile);
                    if (!pNewUser)
                    {
                        LogDbg("[MEM] Cannot allocate pNewUser in RegManager::LoadXML");

                        exit(EXIT_FAILURE);
                    }
                    Add(pNewUser);
                }
                else
                {
                    const int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer,
                                           ServerManager::m_szGlobalBufferSize,
                                           "%s %s %s! %s.",
                                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USER)].c_str(),
                                           nick,
                                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_IS_ALREADY_IN_REGS)].c_str(),
                                           LanguageManager::m_Ptr->m_sTexts[std::to_underlying(LangIds::LAN_USER_DELETED)].c_str());
                    if (iMsgLen > 0)
                    {
                        LogInfo("{}", ServerManager::m_pGlobalBuffer);
                    }

                    bIsBuggy = true;
                }
                child = child->NextSiblingElement();
            }
            if (bIsBuggy)
            {
                Save();
            }
        }
    }
}
//---------------------------------------------------------------------------

void RegManager::Save(const bool bSaveOnChange /* = false*/, const bool bSaveOnTime /* = false*/)
{
    if (bSaveOnTime && m_ui8SaveCalls == 0)
    {
        return;
    }

    m_ui8SaveCalls++;

    if (bSaveOnChange && m_ui8SaveCalls < 100)
    {
        return;
    }

    m_ui8SaveCalls = 0;

    PXBReader pxbRegs;

    // Open regs file
    if (!pxbRegs.OpenFileSave((ServerManager::m_sPath + "/cfg/RegisteredUsers.pxb").c_str(), 3))
    {
        return;
    }

    // Write file header
    pxbRegs.m_sItemIdentifiers[0] = 'F';
    pxbRegs.m_sItemIdentifiers[1] = 'I';
    pxbRegs.m_ui16ItemLengths[0] = static_cast<uint16_t>(g_szPtokaXRegiteredUsersLen);
    pxbRegs.m_pItemDatas[0] = sPtokaXRegiteredUsers;
    pxbRegs.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbRegs.m_sItemIdentifiers[2] = 'F';
    pxbRegs.m_sItemIdentifiers[3] = 'V';
    pxbRegs.m_ui16ItemLengths[1] = 4;
    const uint32_t ui32Version = 1;
    pxbRegs.m_pItemDatas[1] = &ui32Version;
    pxbRegs.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if (!pxbRegs.WriteNextItem(g_szPtokaXRegiteredUsersLen + 4, 2))
    {
        return;
    }

    pxbRegs.m_sItemIdentifiers[0] = 'N';
    pxbRegs.m_sItemIdentifiers[1] = 'I';
    pxbRegs.m_sItemIdentifiers[2] = 'P';
    pxbRegs.m_sItemIdentifiers[3] = 'A';
    pxbRegs.m_sItemIdentifiers[4] = 'P';
    pxbRegs.m_sItemIdentifiers[5] = 'R';

    pxbRegs.m_ui8ItemValues[0] = PXBReader::PXB_STRING;
    pxbRegs.m_ui8ItemValues[1] = PXBReader::PXB_STRING;
    pxbRegs.m_ui8ItemValues[2] = PXBReader::PXB_TWO_BYTES;

    for (const auto& curRegPtr : m_RegList)
    {
        RegUser* curReg = curRegPtr.get();

        pxbRegs.m_ui16ItemLengths[0] = uint16_t(curReg->m_sNick.length());
        pxbRegs.m_pItemDatas[0] = curReg->m_sNick.c_str();
        pxbRegs.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

        if (curReg->m_bPassHash)
        {
            pxbRegs.m_sItemIdentifiers[3] = 'S';

            pxbRegs.m_ui16ItemLengths[1] = 64;
            pxbRegs.m_pItemDatas[1] = curReg->m_vPassData.data();
        }
        else
        {
            pxbRegs.m_sItemIdentifiers[3] = 'A';

            pxbRegs.m_ui16ItemLengths[1] = static_cast<uint16_t>(!curReg->m_vPassData.empty() ? curReg->m_vPassData.size() - 1 : 0);
            pxbRegs.m_pItemDatas[1] = curReg->m_vPassData.data();
        }

        pxbRegs.m_ui16ItemLengths[2] = 2;
        pxbRegs.m_pItemDatas[2] = &curReg->m_ui16Profile;

        if (!pxbRegs.WriteNextItem(pxbRegs.m_ui16ItemLengths[0] + pxbRegs.m_ui16ItemLengths[1] + pxbRegs.m_ui16ItemLengths[2], 3))
        {
            break;
        }
    }

    pxbRegs.WriteRemaining();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RegManager::HashPasswords() const
{
    size_t szPassLen = 0;

    for (const auto& pCurRegPtr : m_RegList)
    {
        RegUser* pCurReg = pCurRegPtr.get();

        if (!pCurReg->m_bPassHash)
        {
            std::array<uint8_t, 64> ui8Hash = {};

            szPassLen = !pCurReg->m_vPassData.empty() ? pCurReg->m_vPassData.size() - 1 : 0;

            if (HashPassword(reinterpret_cast<const char*>(pCurReg->m_vPassData.data()), szPassLen, ui8Hash.data()))
            {
                pCurReg->m_vPassData.assign(ui8Hash.begin(), ui8Hash.end());
                pCurReg->m_bPassHash = true;
            }
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RegManager::AddRegCmdLine()
{
    std::string sNick(66, '\0');

    for (;;)
    {
        printf("Please enter Nick for new Registered User (Maximal length 64 characters. Characters |, $ and space are not allowed): ");
        if (fgets(sNick.data(), static_cast<int>(sNick.size()), stdin))
        {
            char* sMatch = strchr(sNick.data(), '\n');
            if (sMatch)
            {
                sMatch[0] = '\0';
                sNick.resize(static_cast<size_t>(sMatch - sNick.data()));
            }
            else
            {
                sNick.resize(strlen(sNick.data()));
            }

            sMatch = strpbrk(sNick.data(), " $|");
            if (sMatch)
            {
                printf("Character '%c' is not allowed in Nick!\n", sMatch[0]);
                if (!WantAgain())
                    return;
                continue;
            }

            const size_t szLen = sNick.size();

            if (szLen == 0)
            {
                printf("No Nick specified!\n");
                if (!WantAgain())
                    return;
                continue;
            }

            RegUser* pReg = Find(sNick);
            if (pReg)
            {
                printf("Registered user with nick '%s' already exist!\n", sNick.c_str());
                if (!WantAgain())
                    return;
                continue;
            }
        }
        else
        {
            printf("Error reading Nick... ending.\n");
            exit(EXIT_FAILURE);
        }
        break;
    }

    std::string sPassword(66, '\0');

    for (;;)
    {
        printf("Please enter Password for new Registered User (Maximal length 64 characters. Character | is not allowed): ");
        if (fgets(sPassword.data(), static_cast<int>(sPassword.size()), stdin))
        {
            char* sMatch = strchr(sPassword.data(), '\n');
            if (sMatch)
            {
                sMatch[0] = '\0';
                sPassword.resize(static_cast<size_t>(sMatch - sPassword.data()));
            }
            else
            {
                sPassword.resize(strlen(sPassword.data()));
            }

            sMatch = strchr(sPassword.data(), '|');
            if (sMatch)
            {
                printf("Character | is not allowed in Password!\n");
                if (!WantAgain())
                    return;
                continue;
            }

            const size_t szLen = sPassword.size();

            if (szLen == 0)
            {
                printf("No Password specified!\n");
                if (!WantAgain())
                    return;
                continue;
            }
        }
        else
        {
            printf("Error reading Password... ending.\n");
            exit(EXIT_FAILURE);
        }
        break;
    }

    printf("\nAvailable profiles: \n");
    for (uint16_t ui16i = 0; ui16i < ProfileManager::m_Ptr->m_ui16ProfileCount; ui16i++)
    {
        printf("%hu - %s\n", ui16i, ProfileManager::m_Ptr->m_vpProfilesTable[ui16i]->m_sName.c_str());
    }

    uint16_t ui16Profile = 0;
    std::array<char, 7> sProfile = {};

    for (;;)
    {
        printf("Please enter Profile number for new Registered User: ");
        if (fgets(sProfile.data(), sProfile.size(), stdin))
        {
            char* sMatch = strchr(sProfile.data(), '\n');
            if (sMatch)
            {
                sMatch[0] = '\0';
            }

            const auto ui8Len = static_cast<uint8_t>(strlen(sProfile.data()));

            if (ui8Len == 0)
            {
                printf("No Profile specified!\n");
                if (!WantAgain())
                    return;
                continue;
            }

            bool bValid = true;
            for (const char ch : std::string_view(sProfile.data(), ui8Len))
            {
                if (!isdigit(static_cast<unsigned char>(ch)))
                {
                    printf("Character '%c' is not valid number!\n", ch);
                    bValid = false;
                    break;
                }
            }

            if (!bValid)
            {
                if (!WantAgain())
                    return;
                continue;
            }

            int iProfile = 0;
            if (!safe_stoi(sProfile.data(), iProfile))
            {
                printf("Invalid profile number '%s'!\n", sProfile.data());
                if (!WantAgain())
                    return;
                continue;
            }
            ui16Profile = static_cast<uint16_t>(iProfile);
            if (ui16Profile >= ProfileManager::m_Ptr->m_ui16ProfileCount)
            {
                printf("Profile number %hu not exist!\n", ui16Profile);
                if (!WantAgain())
                    return;
                continue;
            }
        }
        else
        {
            printf("Error reading Profile... ending.\n");
            exit(EXIT_FAILURE);
        }
        break;
    }

    if (!AddNew(sNick.c_str(), sPassword.c_str(), ui16Profile))
    {
        printf("Error adding new Registered User... ending.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Registered User with Nick '%s' Password '%s' and Profile '%hu' was added.", sNick.c_str(), sPassword.c_str(), ui16Profile);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
