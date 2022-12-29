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
#include "tinyxml.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
#include "../gui.win/RegisteredUserDialog.h"
#include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------
RegManager * RegManager::m_Ptr = nullptr;
//---------------------------------------------------------------------------
static const char sPtokaXRegiteredUsers[] = "PtokaX Registered Users";
static const size_t szPtokaXRegiteredUsersLen = sizeof(sPtokaXRegiteredUsers) - 1;
//---------------------------------------------------------------------------

RegUser::RegUser() : m_tLastBadPass(0), m_pPrev(NULL), m_pNext(NULL), m_pHashTablePrev(NULL), m_pHashTableNext(NULL), m_ui32Hash(0), m_ui16Profile(0), m_ui8BadPassCount(0), m_bPassHash(false)
{
	m_sPass = nullptr;
}
//---------------------------------------------------------------------------

RegUser::~RegUser()
{
	if (m_bPassHash == true)
	{
		free(m_ui8PassHash);
	}
	else
	{
		free(m_sPass);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RegUser * RegUser::CreateReg(const char * sRegNick, const size_t szRegNickLen, const char * sRegPassword, const size_t szRegPassLen, const uint8_t * ui8RegPassHash, const uint16_t ui16RegProfile)
{
	RegUser * pReg = new (std::nothrow) RegUser();

	if (pReg == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate new Reg in RegUser::CreateReg\n");

		return NULL;
	}

	pReg->m_sNick = std::string(sRegNick, szRegNickLen);

	if (ui8RegPassHash != NULL)
	{
		pReg->m_ui8PassHash = (uint8_t *)malloc(64);
		if (pReg->m_ui8PassHash == NULL)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate 64 bytes for ui8PassHash in RegUser::RegUser\n");

			delete pReg;
			return NULL;
		}
		memcpy(pReg->m_ui8PassHash, ui8RegPassHash, 64);
		pReg->m_bPassHash = true;
	}
	else if(sRegPassword != NULL)
  	       {
		pReg->m_sPass = (char *)malloc(szRegPassLen + 1);
		if (pReg->m_sPass == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sPass in RegUser::RegUser\n", szRegPassLen+1);

			delete pReg;
			return NULL;
		}
		memcpy(pReg->m_sPass, sRegPassword, szRegPassLen);
		pReg->m_sPass[szRegPassLen] = '\0';
    } else {
        AppendDebugLogFormat("[ERR] Empty ui8RegPassHash and sRegPassword in RegUser::RegUser\n");

        delete pReg;
        return NULL;
	}

	pReg->m_ui16Profile = ui16RegProfile;
	pReg->m_ui32Hash = HashNick(sRegNick, szRegNickLen);

	return pReg;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool RegUser::UpdatePassword(const char * sNewPass, const size_t szNewLen)
{
	if (SettingManager::m_Ptr->m_bBools[SETBOOL_HASH_PASSWORDS] == false)
	{
		if (m_bPassHash == true)
		{
			void * sOldBuf = m_ui8PassHash;
			m_sPass = (char *)realloc(sOldBuf, szNewLen + 1);
			if (m_sPass == NULL)
			{
				m_ui8PassHash = (uint8_t *)sOldBuf;

				AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for m_ui8PassHash->sPass in RegUser::UpdatePassword\n", szNewLen+1);

				return false;
			}
			memcpy(m_sPass, sNewPass, szNewLen);
			m_sPass[szNewLen] = '\0';

			m_bPassHash = false;
		}
		else if (strcmp(m_sPass, sNewPass) != 0)
		{
			char * sOldPass = m_sPass;
			m_sPass = (char *)realloc(sOldPass, szNewLen + 1);
			if (m_sPass == NULL)
			{
				m_sPass = sOldPass;

				AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for m_sPass in RegUser::UpdatePassword\n", szNewLen+1);

				return false;
			}
			memcpy(m_sPass, sNewPass, szNewLen);
			m_sPass[szNewLen] = '\0';
		}
	}
	else
	{
		if (m_bPassHash == true)
		{
			HashPassword(sNewPass, szNewLen, m_ui8PassHash);
		}
		else
		{
			char * sOldPass = m_sPass;
			m_ui8PassHash = (uint8_t *)realloc(sOldPass, 64);
			if (m_ui8PassHash == NULL)
			{
				m_sPass = sOldPass;

				AppendDebugLog("%s - [MEM] Cannot reallocate 64 bytes for m_sPass->m_ui8PassHash in RegUser::UpdatePassword\n");

				return false;
			}

			if (HashPassword(sNewPass, szNewLen, m_ui8PassHash) == true)
			{
				m_bPassHash = true;
			}
			else
			{
				m_sPass = (char *)m_ui8PassHash;
			}
		}
	}

	return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RegManager::RegManager(void) : m_ui8SaveCalls(0), m_pRegListS(NULL), m_pRegListE(NULL)
{
	memset(m_pTable, 0, sizeof(m_pTable));
}
//---------------------------------------------------------------------------

RegManager::~RegManager(void)
{
	RegUser * curReg = NULL,
	          * next = m_pRegListS;

	while (next != NULL)
	{
		curReg = next;
		next = curReg->m_pNext;

		delete curReg;
	}
}
//---------------------------------------------------------------------------

bool RegManager::AddNew(const char * sNick, const char * sPasswd, const uint16_t iProfile)
{
	if (Find(sNick, strlen(sNick)) != NULL)
	{
		return false;
	}

	RegUser * pNewUser = nullptr;

	if (SettingManager::m_Ptr->m_bBools[SETBOOL_HASH_PASSWORDS] == true)
	{
		uint8_t ui8Hash[64];

		size_t szPassLen = strlen(sPasswd);

		if (HashPassword(sPasswd, szPassLen, ui8Hash) == false)
		{
			return false;
		}

		pNewUser = RegUser::CreateReg(sNick, strlen(sNick), NULL, 0, ui8Hash, iProfile);
	}
	else
	{
		pNewUser = RegUser::CreateReg(sNick, strlen(sNick), sPasswd, strlen(sPasswd), NULL, iProfile);
	}

	if (pNewUser == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in RegManager::AddNew\n");

		return false;
	}

	Add(pNewUser);

#ifdef _BUILD_GUI
	if (RegisteredUsersDialog::m_Ptr != NULL)
	{
		RegisteredUsersDialog::m_Ptr->AddReg(pNewUser);
	}
#endif

	Save(true);

	if (ServerManager::m_bServerRunning == false)
	{
		return true;
	}

	User * AddedUser = HashManager::m_Ptr->FindUser(pNewUser->m_sNick);

	if (AddedUser != NULL)
	{
		bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT);
		AddedUser->m_i32Profile = iProfile;

		if (((AddedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false)
		{
			if (ProfileManager::m_Ptr->IsAllowed(AddedUser, ProfileManager::HASKEYICON) == true)
			{
				AddedUser->m_ui32BoolBits |= User::BIT_OPERATOR;
			}
			else
			{
				AddedUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
			}

			if (((AddedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true)
			{
				// alex82 ... HideUserKey / ������ ���� �����
				if (((AddedUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY) == false)
				{
					Users::m_Ptr->Add2OpList(AddedUser);
					GlobalDataQueue::m_Ptr->OpListStore(AddedUser->m_sNick);
				}
				if (bAllowedOpChat != ProfileManager::m_Ptr->IsAllowed(AddedUser, ProfileManager::ALLOWEDOPCHAT))
				{
					if (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true &&
					        (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false || SettingManager::m_Ptr->m_bBotsSameNick == false))
					{
						if (((AddedUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false)
						{
							AddedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_HELLO],
							                           SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_HELLO]);
						}

						AddedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_MYINFO], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_MYINFO]);
						AddedUser->SendFormat("RegManager::AddNew", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
					}
				}
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------

void RegManager::Add(RegUser * pReg)
{
	Add2Table(pReg);

	if (m_pRegListE == NULL)
	{
		m_pRegListS = pReg;
		m_pRegListE = pReg;
	}
	else
	{
		pReg->m_pPrev = m_pRegListE;
		m_pRegListE->m_pNext = pReg;
		m_pRegListE = pReg;
	}

	return;
}
//---------------------------------------------------------------------------

void RegManager::Add2Table(RegUser * pReg)
{
	const uint16_t ui16dx = CalcHash(pReg->m_ui32Hash);

	if (m_pTable[ui16dx] != NULL)
	{
		m_pTable[ui16dx]->m_pHashTablePrev = pReg;
		pReg->m_pHashTableNext = m_pTable[ui16dx];
	}

	m_pTable[ui16dx] = pReg;
}
//---------------------------------------------------------------------------

void RegManager::ChangeReg(RegUser * pReg, const char * sNewPasswd, const uint16_t ui16NewProfile)
{
	if (sNewPasswd != NULL)
	{
		size_t szPassLen = strlen(sNewPasswd);

		pReg->UpdatePassword(sNewPasswd, szPassLen);
	}

	pReg->m_ui16Profile = ui16NewProfile;

#ifdef _BUILD_GUI
	if (RegisteredUsersDialog::m_Ptr != NULL)
	{
		RegisteredUsersDialog::m_Ptr->RemoveReg(pReg);
		RegisteredUsersDialog::m_Ptr->AddReg(pReg);
	}
#endif

	RegManager::m_Ptr->Save(true);

	if (ServerManager::m_bServerRunning == false)
	{
		return;
	}

	User *ChangedUser = HashManager::m_Ptr->FindUser(pReg->m_sNick);
	if (ChangedUser != NULL && ChangedUser->m_i32Profile != (int32_t)ui16NewProfile)
	{
		bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT);

		ChangedUser->m_i32Profile = (int32_t)ui16NewProfile;

		if (((ChangedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) !=
		        ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::HASKEYICON))
		{
			if (ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::HASKEYICON) == true)
			{
				ChangedUser->m_ui32BoolBits |= User::BIT_OPERATOR;
				// alex82 ... HideUserKey / ������ ���� �����
				if (((ChangedUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY) == false) {
					Users::m_Ptr->Add2OpList(ChangedUser);
					GlobalDataQueue::m_Ptr->OpListStore(ChangedUser->m_sNick);
				}
			}
			else
			{
				ChangedUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
				// alex82 ... HideUserKey / ������ ���� �����
				if (((ChangedUser->m_ui32InfoBits & User::INFOBIT_HIDE_KEY) == User::INFOBIT_HIDE_KEY) == false) {
					// alex82 ... ��������� �������� OpList
					int imsgLen = sprintf(ServerManager::m_pGlobalBuffer, "$Quit %s|", ChangedUser->m_sNick);
					if (CheckSprintf(imsgLen, 128, "RegManager::ChangeReg1") == true) {
						GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, imsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
					}
					switch (SettingManager::m_Ptr->m_ui8FullMyINFOOption) {
					case 0:
						GlobalDataQueue::m_Ptr->AddQueueItem(ChangedUser->m_sMyInfoLong, ChangedUser->m_ui16MyInfoLongLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
						break;
					case 1:
						GlobalDataQueue::m_Ptr->AddQueueItem(ChangedUser->m_sMyInfoShort, ChangedUser->m_ui16MyInfoShortLen, ChangedUser->m_sMyInfoLong, ChangedUser->m_ui16MyInfoLongLen, GlobalDataQueue::CMD_MYINFO);
						break;
					case 2:
						GlobalDataQueue::m_Ptr->AddQueueItem(ChangedUser->m_sMyInfoShort, ChangedUser->m_ui16MyInfoShortLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
						break;
					default:
						break;
					}
					Users::m_Ptr->DelFromOpList(ChangedUser->m_sNick);
				}
			}
		}

		if (bAllowedOpChat != ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT))
		{
			if (ProfileManager::m_Ptr->IsAllowed(ChangedUser, ProfileManager::ALLOWEDOPCHAT) == true)
			{
				if (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true &&
				        (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false || SettingManager::m_Ptr->m_bBotsSameNick == false))
				{
					if (((ChangedUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false)
					{
						ChangedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_HELLO],
						                             SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_HELLO]);
					}

					ChangedUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_MYINFO], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_MYINFO]);
					ChangedUser->SendFormat("RegManager::ChangeReg1", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
				}
			}
			else
			{
				if (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true && (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false || SettingManager::m_Ptr->m_bBotsSameNick == false))
				{
					ChangedUser->SendFormat("RegManager::ChangeReg2", true, "$Quit %s|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
				}
			}
		}
	}

#ifdef _BUILD_GUI
	if (RegisteredUserDialog::m_Ptr != NULL)
	{
		RegisteredUserDialog::m_Ptr->RegChanged(pReg);
	}
#endif
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void RegManager::Delete(RegUser * pReg, const bool bFromGui/* = false*/)
{
#else
void RegManager::Delete(RegUser * pReg, const bool /*bFromGui = false*/)
{
#endif
	if (ServerManager::m_bServerRunning == true)
	{
		User * pRemovedUser = HashManager::m_Ptr->FindUser(pReg->m_sNick);

		if (pRemovedUser != NULL)
		{
			pRemovedUser->m_i32Profile = -1;
			if (((pRemovedUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true)
			{
				Users::m_Ptr->DelFromOpList(pRemovedUser->m_sNick);
				pRemovedUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;

				if (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true && (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false || SettingManager::m_Ptr->m_bBotsSameNick == false))
				{
					pRemovedUser->SendFormat("RegManager::Delete", true, "$Quit %s|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
				}
			}
		}
	}

#ifdef _BUILD_GUI
	if (bFromGui == false && RegisteredUsersDialog::m_Ptr != NULL)
	{
		RegisteredUsersDialog::m_Ptr->RemoveReg(pReg);
	}
#endif

	Rem(pReg);

#ifdef _BUILD_GUI
	if (RegisteredUserDialog::m_Ptr != NULL)
	{
		RegisteredUserDialog::m_Ptr->RegDeleted(pReg);
	}
#endif

	delete pReg;

	Save(true);
}
//---------------------------------------------------------------------------

void RegManager::Rem(RegUser * pReg)
{
	RemFromTable(pReg);

	RegUser *prev, *next;
	prev = pReg->m_pPrev;
	next = pReg->m_pNext;

	if (prev == NULL)
	{
		if (next == NULL)
		{
			m_pRegListS = NULL;
			m_pRegListE = NULL;
		}
		else
		{
			next->m_pPrev = NULL;
			m_pRegListS = next;
		}
	}
	else if (next == NULL)
	{
		prev->m_pNext = NULL;
		m_pRegListE = prev;
	}
	else
	{
		prev->m_pNext = next;
		next->m_pPrev = prev;
	}
}
//---------------------------------------------------------------------------

void RegManager::RemFromTable(RegUser * pReg)
{
	if (pReg->m_pHashTablePrev == NULL)
	{
		const uint16_t ui16dx = CalcHash(pReg->m_ui32Hash);

		if (pReg->m_pHashTableNext == NULL)
		{
			m_pTable[ui16dx] = NULL;
		}
		else
		{
			pReg->m_pHashTableNext->m_pHashTablePrev = NULL;
			m_pTable[ui16dx] = pReg->m_pHashTableNext;
		}
	}
	else if (pReg->m_pHashTableNext == NULL)
	{
		pReg->m_pHashTablePrev->m_pHashTableNext = NULL;
	}
	else
	{
		pReg->m_pHashTablePrev->m_pHashTableNext = pReg->m_pHashTableNext;
		pReg->m_pHashTableNext->m_pHashTablePrev = pReg->m_pHashTablePrev;
	}

	pReg->m_pHashTablePrev = NULL;
	pReg->m_pHashTableNext = NULL;
}
//---------------------------------------------------------------------------

RegUser* RegManager::Find(const char * sNick, const size_t szNickLen)
{
	uint32_t ui32Hash = HashNick(sNick, szNickLen);

	const uint16_t ui16dx = CalcHash(ui32Hash);

	RegUser * cur = NULL,
	          * next = m_pTable[ui16dx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pHashTableNext;

		if (cur->m_ui32Hash == ui32Hash && strcasecmp(cur->m_sNick.c_str(), sNick) == 0)
		{
			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

RegUser* RegManager::Find(User * pUser)
{
	const uint16_t ui16dx = CalcHash(pUser->m_ui32NickHash);

	RegUser * cur = NULL,
	          * next = m_pTable[ui16dx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pHashTableNext;

		if (cur->m_ui32Hash == pUser->m_ui32NickHash && strcasecmp(cur->m_sNick.c_str(), pUser->m_sNick) == 0)
		{
			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

RegUser* RegManager::Find(const uint32_t ui32Hash, const char * sNick)
{
	const uint16_t ui16dx = CalcHash(ui32Hash);

	RegUser * cur = NULL,
	          * next = m_pTable[ui16dx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pHashTableNext;

		if (cur->m_ui32Hash == ui32Hash && strcasecmp(cur->m_sNick.c_str(), sNick) == 0)
		{
			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

void RegManager::Load(void)
{
#ifdef _WIN32
	if (FileExist((ServerManager::m_sPath + "\\cfg\\RegisteredUsers.pxb").c_str()) == false)
	{
#else
	if (FileExist((ServerManager::m_sPath + "/cfg/RegisteredUsers.pxb").c_str()) == false)
	{
#endif
		LoadXML();
		return;
	}

	uint16_t iProfilesCount = (uint16_t)(ProfileManager::m_Ptr->m_ui16ProfileCount - 1);

	PXBReader pxbRegs;

	// Open regs file
#ifdef _WIN32
	if (pxbRegs.OpenFileRead((ServerManager::m_sPath + "\\cfg\\RegisteredUsers.pxb").c_str(), 4) == false)
	{
#else
	if (pxbRegs.OpenFileRead((ServerManager::m_sPath + "/cfg/RegisteredUsers.pxb").c_str(), 4) == false)
	{
#endif
		return;
	}

	// Read file header
	uint16_t ui16Identificators[4] = { *((uint16_t *)"FI"), *((uint16_t *)"FV"), *((uint16_t *)"  "), *((uint16_t *)"  ") };

	if (pxbRegs.ReadNextItem(ui16Identificators, 2) == false)
	{
		return;
	}

	// Check header if we have correct file
	if (pxbRegs.m_ui16ItemLengths[0] != szPtokaXRegiteredUsersLen || strncmp((char *)pxbRegs.m_pItemDatas[0], sPtokaXRegiteredUsers, szPtokaXRegiteredUsersLen) != 0)
	{
		return;
	}

	{
		uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbRegs.m_pItemDatas[1])));

		if (ui32FileVersion < 1)
		{
			return;
		}
	}

	// Read regs =)
	ui16Identificators[0] = *((uint16_t *)"NI");
	ui16Identificators[1] = *((uint16_t *)"PS");
	ui16Identificators[2] = *((uint16_t *)"PR");
	ui16Identificators[3] = *((uint16_t *)"PA");

	uint16_t iProfile = UINT16_MAX;
	RegUser * pNewUser = NULL;
	uint8_t ui8Hash[64];
	size_t szPassLen = 0;

	bool bSuccess = pxbRegs.ReadNextItem(ui16Identificators, 3, 1);

	while (bSuccess == true)
	{
		if (pxbRegs.m_ui16ItemLengths[0] < 65 && pxbRegs.m_ui16ItemLengths[1] < 65 && pxbRegs.m_ui16ItemLengths[2] == 2)
		{
			iProfile = (uint16_t)ntohs(*((uint16_t *)(pxbRegs.m_pItemDatas[2])));

			if (iProfile > iProfilesCount)
			{
				iProfile = iProfilesCount;
			}

			pNewUser = NULL;

			if (pxbRegs.m_ui16ItemLengths[3] != 0)
			{
				if (SettingManager::m_Ptr->m_bBools[SETBOOL_HASH_PASSWORDS] == true)
				{
					szPassLen = (size_t)pxbRegs.m_ui16ItemLengths[3];

					if (HashPassword((char *)pxbRegs.m_pItemDatas[3], szPassLen, ui8Hash) == false)
					{
						pNewUser = RegUser::CreateReg((char *)pxbRegs.m_pItemDatas[0], pxbRegs.m_ui16ItemLengths[0], (char *)pxbRegs.m_pItemDatas[3], pxbRegs.m_ui16ItemLengths[3], NULL, iProfile);
					}
					else
					{
						pNewUser = RegUser::CreateReg((char *)pxbRegs.m_pItemDatas[0], pxbRegs.m_ui16ItemLengths[0], NULL, 0, ui8Hash, iProfile);
					}
				}
				else
				{
					pNewUser = RegUser::CreateReg((char *)pxbRegs.m_pItemDatas[0], pxbRegs.m_ui16ItemLengths[0], (char *)pxbRegs.m_pItemDatas[3], pxbRegs.m_ui16ItemLengths[3], NULL, iProfile);
				}
			}
			else if (pxbRegs.m_ui16ItemLengths[1] == 64)
			{
#ifdef _WITHOUT_SKEIN
				AppendDebugLog("%s - [ERR] Hashed password found in RegisteredUsers, but PtokaX is compiled without hashing support!\n");

				exit(EXIT_FAILURE);
#endif
				pNewUser = RegUser::CreateReg((char *)pxbRegs.m_pItemDatas[0], pxbRegs.m_ui16ItemLengths[0], NULL, 0, (uint8_t *)pxbRegs.m_pItemDatas[1], iProfile);
			}

			if (pNewUser == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in RegManager::Load\n");
				const string l_nick((const char *)pxbRegs.m_pItemDatas[0], pxbRegs.m_ui16ItemLengths[0]);
				printf("[MEM] Cannot allocate pNewUser in RegManager::Load user = %s\n", l_nick.c_str());
				// [-] PPA 09.07.2015 exit(EXIT_FAILURE);
			}
			else
			{
				Add(pNewUser);
#ifdef _DEBUG
				//const string l_nick((const char *)pxbRegs.m_pItemDatas[0], pxbRegs.m_ui16ItemLengths[0]);
				//printf("[MEM] OK Allocate new User = %s\n", l_nick.c_str());
#endif
			}

		}

		bSuccess = pxbRegs.ReadNextItem(ui16Identificators, 3, 1);
	}
}
//---------------------------------------------------------------------------

void RegManager::LoadXML()
{
	uint16_t iProfilesCount = (uint16_t)(ProfileManager::m_Ptr->m_ui16ProfileCount - 1);

#ifdef _WIN32
	TiXmlDocument doc((ServerManager::m_sPath + "\\cfg\\RegisteredUsers.xml").c_str());
#else
	TiXmlDocument doc((ServerManager::m_sPath + "/cfg/RegisteredUsers.xml").c_str());
#endif

	if (doc.LoadFile() == false)
	{
		if (doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY)
		{
			int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file RegisteredUsers.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
			if (iMsgLen > 0)
			{
#ifdef _BUILD_GUI
				::MessageBox(NULL, ServerManager::m_pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
				AppendLog(ServerManager::m_pGlobalBuffer);
#endif
			}

			exit(EXIT_FAILURE);
		}
	}
	else
	{
		TiXmlHandle cfg(&doc);
		TiXmlNode *registeredusers = cfg.FirstChild("RegisteredUsers").Node();
		if (registeredusers != NULL)
		{
			bool bIsBuggy = false;
			TiXmlNode *child = NULL;

			while ((child = registeredusers->IterateChildren(child)) != NULL)
			{
				TiXmlNode *registereduser = child->FirstChild("Nick");

				if (registereduser == NULL || (registereduser = registereduser->FirstChild()) == NULL)
				{
					continue;
				}

				char *nick = (char *)registereduser->Value();

				if (strlen(nick) > 64 || (registereduser = child->FirstChild("Password")) == NULL ||
				        (registereduser = registereduser->FirstChild()) == NULL)
				{
					continue;
				}

				char *pass = (char *)registereduser->Value();

				if (strlen(pass) > 64 || (registereduser = child->FirstChild("Profile")) == NULL ||
				        (registereduser = registereduser->FirstChild()) == NULL)
				{
					continue;
				}

				uint16_t iProfile = (uint16_t)atoi(registereduser->Value());

				if (iProfile > iProfilesCount)
				{
					int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s %s %s! %s %s.", LanguageManager::m_Ptr->m_sTexts[LAN_USER], nick, LanguageManager::m_Ptr->m_sTexts[LAN_HAVE_NOT_EXIST_PROFILE],
					                       LanguageManager::m_Ptr->m_sTexts[LAN_CHANGED_PROFILE_TO], ProfileManager::m_Ptr->m_ppProfilesTable[iProfilesCount]->m_sName);
					if (iMsgLen > 0)
					{
#ifdef _BUILD_GUI
						::MessageBox(NULL, ServerManager::m_pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
#else
						AppendLog(ServerManager::m_pGlobalBuffer);
#endif
					}

					iProfile = iProfilesCount;
					bIsBuggy = true;
				}

				if (Find((char*)nick, strlen(nick)) == NULL)
				{
					RegUser * pNewUser = RegUser::CreateReg(nick, strlen(nick), pass, strlen(pass), NULL, iProfile);
					if (pNewUser == NULL)
					{
						AppendDebugLog("%s - [MEM] Cannot allocate pNewUser in RegManager::LoadXML\n");

						exit(EXIT_FAILURE);
					}
					Add(pNewUser);
				}
				else
				{
					int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s %s %s! %s.", LanguageManager::m_Ptr->m_sTexts[LAN_USER], nick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREADY_IN_REGS],
					                       LanguageManager::m_Ptr->m_sTexts[LAN_USER_DELETED]);
					if (iMsgLen > 0)
					{
#ifdef _BUILD_GUI
						::MessageBox(NULL, ServerManager::m_pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONEXCLAMATION);
#else
						AppendLog(ServerManager::m_pGlobalBuffer);
#endif
					}

					bIsBuggy = true;
				}
			}
			if (bIsBuggy == true)
				Save();
		}
	}
}
//---------------------------------------------------------------------------

void RegManager::Save(const bool bSaveOnChange/* = false*/, const bool bSaveOnTime/* = false*/)
{
	if (bSaveOnTime == true && m_ui8SaveCalls == 0)
	{
		return;
	}

	m_ui8SaveCalls++;

	if (bSaveOnChange == true && m_ui8SaveCalls < 100)
	{
		return;
	}

	m_ui8SaveCalls = 0;

	PXBReader pxbRegs;

	// Open regs file
#ifdef _WIN32
	if (pxbRegs.OpenFileSave((ServerManager::m_sPath + "\\cfg\\RegisteredUsers.pxb").c_str(), 3) == false)
	{
#else
	if (pxbRegs.OpenFileSave((ServerManager::m_sPath + "/cfg/RegisteredUsers.pxb").c_str(), 3) == false)
	{
#endif
		return;
	}

	// Write file header
	pxbRegs.m_sItemIdentifiers[0] = 'F';
	pxbRegs.m_sItemIdentifiers[1] = 'I';
	pxbRegs.m_ui16ItemLengths[0] = (uint16_t)szPtokaXRegiteredUsersLen;
	pxbRegs.m_pItemDatas[0] = (void *)sPtokaXRegiteredUsers;
	pxbRegs.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

	pxbRegs.m_sItemIdentifiers[2] = 'F';
	pxbRegs.m_sItemIdentifiers[3] = 'V';
	pxbRegs.m_ui16ItemLengths[1] = 4;
	uint32_t ui32Version = 1;
	pxbRegs.m_pItemDatas[1] = (void *)&ui32Version;
	pxbRegs.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

	if (pxbRegs.WriteNextItem(szPtokaXRegiteredUsersLen + 4, 2) == false)
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

	RegUser * curReg = NULL,
	          * next = m_pRegListS;

	while (next != NULL)
	{
		curReg = next;
		next = curReg->m_pNext;

		pxbRegs.m_ui16ItemLengths[0] = uint16_t(curReg->m_sNick.length());
		pxbRegs.m_pItemDatas[0] = (void *)curReg->m_sNick.c_str();
		pxbRegs.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

		if (curReg->m_bPassHash == true)
		{
			pxbRegs.m_sItemIdentifiers[3] = 'S';

			pxbRegs.m_ui16ItemLengths[1] = 64;
			pxbRegs.m_pItemDatas[1] = (void *)curReg->m_ui8PassHash;
		}
		else
		{
			pxbRegs.m_sItemIdentifiers[3] = 'A';

			pxbRegs.m_ui16ItemLengths[1] = (uint16_t)strlen(curReg->m_sPass);
			pxbRegs.m_pItemDatas[1] = (void *)curReg->m_sPass;
		}

		pxbRegs.m_ui16ItemLengths[2] = 2;
		pxbRegs.m_pItemDatas[2] = (void *)&curReg->m_ui16Profile;

		if (pxbRegs.WriteNextItem(pxbRegs.m_ui16ItemLengths[0] + pxbRegs.m_ui16ItemLengths[1] + pxbRegs.m_ui16ItemLengths[2], 3) == false)
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
	char * sOldPass = NULL;

	RegUser * pCurReg = NULL,
	          * pNextReg = m_pRegListS;

	while (pNextReg != NULL)
	{
		pCurReg = pNextReg;
		pNextReg = pCurReg->m_pNext;

		if (pCurReg->m_bPassHash == false)
		{
			sOldPass = pCurReg->m_sPass;
			pCurReg->m_ui8PassHash = (uint8_t *)malloc(64);
			if (pCurReg->m_ui8PassHash == NULL)
			{
				pCurReg->m_sPass = sOldPass;

				AppendDebugLog("%s - [MEM] Cannot reallocate 64 bytes for sPass->ui8PassHash in RegManager::HashPasswords\n");

				continue;
			}

			szPassLen = strlen(sOldPass);

			if (HashPassword(sOldPass, szPassLen, pCurReg->m_ui8PassHash) == true)
			{
				pCurReg->m_bPassHash = true;
				free(sOldPass);
			}
			else
			{
				free(pCurReg->m_ui8PassHash);

				pCurReg->m_sPass = sOldPass;
			}
		}
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RegManager::AddRegCmdLine()
{
	char sNick[66];

nick:
	printf("Please enter Nick for new Registered User (Maximal length 64 characters. Characters |, $ and space are not allowed): ");
	if (fgets(sNick, 66, stdin) != NULL)
	{
		char * sMatch = strchr(sNick, '\n');
		if (sMatch != NULL)
		{
			sMatch[0] = '\0';
		}

		sMatch = strpbrk(sNick, " $|");
		if (sMatch != NULL)
		{
			printf("Character '%c' is not allowed in Nick!\n", sMatch[0]);

			if (WantAgain() == false)
			{
				return;
			}

			goto nick;
		}

		size_t szLen = strlen(sNick);

		if (szLen == 0)
		{
			printf("No Nick specified!\n");

			if (WantAgain() == false)
			{
				return;
			}

			goto nick;
		}

		RegUser * pReg = Find(sNick, strlen(sNick));
		if (pReg != NULL)
		{
			printf("Registered user with nick '%s' already exist!\n", sNick);

			if (WantAgain() == false)
			{
				return;
			}

			goto nick;
		}
	}
	else
	{
		printf("Error reading Nick... ending.\n");
		exit(EXIT_FAILURE);
	}

	char sPassword[66];

password:
	printf("Please enter Password for new Registered User (Maximal length 64 characters. Character | is not allowed): ");
	if (fgets(sPassword, 66, stdin) != NULL)
	{
		char * sMatch = strchr(sPassword, '\n');
		if (sMatch != NULL)
		{
			sMatch[0] = '\0';
		}

		sMatch = strchr(sPassword, '|');
		if (sMatch != NULL)
		{
			printf("Character | is not allowed in Password!\n");

			if (WantAgain() == false)
			{
				return;
			}

			goto password;
		}

		size_t szLen = strlen(sPassword);

		if (szLen == 0)
		{
			printf("No Password specified!\n");

			if (WantAgain() == false)
			{
				return;
			}

			goto password;
		}
	}
	else
	{
		printf("Error reading Password... ending.\n");
		exit(EXIT_FAILURE);
	}

	printf("\nAvailable profiles: \n");
	for (uint16_t ui16i = 0; ui16i < ProfileManager::m_Ptr->m_ui16ProfileCount; ui16i++)
	{
		printf("%hu - %s\n", ui16i, ProfileManager::m_Ptr->m_ppProfilesTable[ui16i]->m_sName);
	}

	uint16_t ui16Profile = 0;
	char sProfile[7];

profile:

	printf("Please enter Profile number for new Registered User: ");
	if (fgets(sProfile, 7, stdin) != NULL)
	{
		char * sMatch = strchr(sProfile, '\n');
		if (sMatch != NULL)
		{
			sMatch[0] = '\0';
		}

		uint8_t ui8Len = (uint8_t)strlen(sProfile);

		if (ui8Len == 0)
		{
			printf("No Profile specified!\n");

			if (WantAgain() == false)
			{
				return;
			}

			goto profile;
		}

		for (uint8_t ui8i = 0; ui8i < ui8Len; ui8i++)
		{
			if (isdigit(sProfile[ui8i]) == 0)
			{
				printf("Character '%c' is not valid number!\n", sProfile[ui8i]);

				if (WantAgain() == false)
				{
					return;
				}

				goto profile;
			}
		}

		ui16Profile = (uint16_t)atoi(sProfile);
		if (ui16Profile >= ProfileManager::m_Ptr->m_ui16ProfileCount)
		{
			printf("Profile number %hu not exist!\n", ui16Profile);

			if (WantAgain() == false)
			{
				return;
			}

			goto profile;
		}
	}
	else
	{
		printf("Error reading Profile... ending.\n");
		exit(EXIT_FAILURE);
	}

	if (AddNew(sNick, sPassword, ui16Profile) == false)
	{
		printf("Error adding new Registered User... ending.\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("Registered User with Nick '%s' Password '%s' and Profile '%hu' was added.", sNick, sPassword, ui16Profile);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
