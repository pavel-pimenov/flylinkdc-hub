/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2017  Petr Kozelka, PPK at PtokaX dot org

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
#include "hashBanManager.h"
//---------------------------------------------------------------------------
#include "PXBReader.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include "tinyxml.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
#include "../gui.win/BansDialog.h"
#include "../gui.win/RangeBansDialog.h"
#endif
//---------------------------------------------------------------------------
BanManager * BanManager::m_Ptr = nullptr;
//---------------------------------------------------------------------------
static const char sPtokaXBans[] = "PtokaX Bans";
static const size_t szPtokaXBansLen = sizeof(sPtokaXBans) - 1;
static const char sPtokaXRangeBans[] = "PtokaX RangeBans";
static const size_t szPtokaXRangeBansLen = sizeof(sPtokaXRangeBans) - 1;
static const char sBanIds[] = "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX";
static const size_t szBanIdsLen = sizeof(sBanIds) - 1;
static const char sRangeBanIds[] = "BT" "RF" "RT" "FB" "RE" "BY" "EX";
static const size_t szRangeBanIdsLen = sizeof(sRangeBanIds) - 1;
//---------------------------------------------------------------------------
BanItemBase::~BanItemBase()
{
	free(m_sReason);
	free(m_sBy);
}

BanItem::BanItem(void) : m_sNick(NULL),  m_pPrev(NULL), m_pNext(NULL), m_pHashNickTablePrev(NULL), m_pHashNickTableNext(NULL), m_pHashIpTablePrev(NULL), m_pHashIpTableNext(NULL), m_ui32NickHash(0)
{
	memset(m_ui128IpHash, 0, 16);
	memset(m_sIp, 0, sizeof(m_sIp));
}
//---------------------------------------------------------------------------

BanItem::~BanItem(void)
{
	free(m_sNick);
}
//---------------------------------------------------------------------------

void BanItem::initIP(const char* pIP)
{
	strcpy(m_sIp, pIP);
}
//---------------------------------------------------------------------------

void BanItem::initIP(const User* u)
{
	strcpy(m_sIp, u->m_sIP);
	memcpy(m_ui128IpHash, u->m_ui128IpHash, sizeof(m_ui128IpHash));
}
//---------------------------------------------------------------------------

RangeBanItem::RangeBanItem(void) : m_pPrev(NULL), m_pNext(NULL)
{
	memset(m_sIpFrom, 0, sizeof(m_sIpFrom));
	memset(m_sIpTo, 0, sizeof(m_sIpTo));
}
//---------------------------------------------------------------------------

RangeBanItem::~RangeBanItem(void)
{
}

//---------------------------------------------------------------------------

BanManager::BanManager(void) : m_ui32SaveCalled(0), m_pTempBanListS(NULL), m_pTempBanListE(NULL), m_pPermBanListS(NULL), m_pPermBanListE(NULL),
	m_pRangeBanListS(NULL), m_pRangeBanListE(NULL)
{
	memset(m_pNickBanTable, 0, sizeof(m_pNickBanTable));
	memset(m_pIpBanTable, 0, sizeof(m_pIpBanTable));
}
//---------------------------------------------------------------------------

BanManager::~BanManager(void)
{
	BanItem * curBan = NULL,
	          * nextBan = m_pPermBanListS;

	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;

		delete curBan;
	}

	nextBan = m_pTempBanListS;

	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;

		delete curBan;
	}

	RangeBanItem * curRangeBan = NULL,
	               * nextRangeBan = m_pRangeBanListS;

	while (nextRangeBan != NULL)
	{
		curRangeBan = nextRangeBan;
		nextRangeBan = curRangeBan->m_pNext;

		delete curRangeBan;
	}

	IpTableItem * cur = NULL, * next = nullptr;

	for (uint32_t ui32i = 0; ui32i < IP_BAN_HASH_TABLE_SIZE; ui32i++)
	{
		next = m_pIpBanTable[ui32i];

		while (next != NULL)
		{
			cur = next;
			next = cur->m_pNext;

			delete cur;
		}
	}
}
//---------------------------------------------------------------------------

bool BanManager::Add(BanItem * pBan)
{
	if (Add2Table(pBan) == false)
	{
		return false;
	}

	if (((pBan->m_ui8Bits & PERM) == PERM) == true)
	{
		if (m_pPermBanListE == NULL)
		{
			m_pPermBanListS = pBan;
			m_pPermBanListE = pBan;
		}
		else
		{
			m_pPermBanListE->m_pNext = pBan;
			pBan->m_pPrev = m_pPermBanListE;
			m_pPermBanListE = pBan;
		}
	}
	else
	{
		if (m_pTempBanListE == NULL)
		{
			m_pTempBanListS = pBan;
			m_pTempBanListE = pBan;
		}
		else
		{
			m_pTempBanListE->m_pNext = pBan;
			pBan->m_pPrev = m_pTempBanListE;
			m_pTempBanListE = pBan;
		}
	}

#ifdef _BUILD_GUI
	if (BansDialog::m_Ptr != NULL)
	{
		BansDialog::m_Ptr->AddBan(pBan);
	}
#endif

	return true;
}
//---------------------------------------------------------------------------

bool BanManager::Add2Table(BanItem * pBan)
{
	if (((pBan->m_ui8Bits & IP) == IP) == true)
	{
		if (Add2IpTable(pBan) == false)
		{
			return false;
		}
	}

	if (((pBan->m_ui8Bits & NICK) == NICK) == true)
	{
		Add2NickTable(pBan);
	}

	return true;
}
//---------------------------------------------------------------------------

void BanManager::Add2NickTable(BanItem *Ban)
{
	const uint16_t ui16dx = CalcHash(Ban->m_ui32NickHash);

	if (m_pNickBanTable[ui16dx] != NULL)
	{
		m_pNickBanTable[ui16dx]->m_pHashNickTablePrev = Ban;
		Ban->m_pHashNickTableNext = m_pNickBanTable[ui16dx];
	}

	m_pNickBanTable[ui16dx] = Ban;
}
//---------------------------------------------------------------------------

bool BanManager::Add2IpTable(BanItem *Ban)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)Ban->m_ui128IpHash))
	{
		ui16IpTableIdx = Ban->m_ui128IpHash[14] * Ban->m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(Ban->m_ui128IpHash);
	}

	if (m_pIpBanTable[ui16IpTableIdx] == NULL)
	{
		m_pIpBanTable[ui16IpTableIdx] = new (std::nothrow) IpTableItem();

		if (m_pIpBanTable[ui16IpTableIdx] == NULL)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate IpTableItem in BanManager::Add2IpTable\n");
			return false;
		}

		m_pIpBanTable[ui16IpTableIdx]->m_pNext = nullptr;
		m_pIpBanTable[ui16IpTableIdx]->m_pPrev = nullptr;

		m_pIpBanTable[ui16IpTableIdx]->pFirstBan = Ban;

		return true;
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, Ban->m_ui128IpHash, 16) == 0)
		{
			cur->pFirstBan->m_pHashIpTablePrev = Ban;
			Ban->m_pHashIpTableNext = cur->pFirstBan;
			cur->pFirstBan = Ban;

			return true;
		}
	}

	cur = new (std::nothrow) IpTableItem;

	if (cur == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate IpTableBans2 in BanManager::Add2IpTable\n");
		return false;
	}

	cur->pFirstBan = Ban;

	cur->m_pNext = m_pIpBanTable[ui16IpTableIdx];
	cur->m_pPrev = nullptr;

	m_pIpBanTable[ui16IpTableIdx]->m_pPrev = cur;
	m_pIpBanTable[ui16IpTableIdx] = cur;

	return true;
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void BanManager::Rem(BanItem * Ban, const bool bFromGui/* = false*/)
{
#else
void BanManager::Rem(BanItem * Ban, const bool /*bFromGui = false*/)
{
#endif
	RemFromTable(Ban);

	if (((Ban->m_ui8Bits & PERM) == PERM) == true)
	{
		if (Ban->m_pPrev == NULL)
		{
			if (Ban->m_pNext == NULL)
			{
				m_pPermBanListS = nullptr;
				m_pPermBanListE = nullptr;
			}
			else
			{
				Ban->m_pNext->m_pPrev = nullptr;
				m_pPermBanListS = Ban->m_pNext;
			}
		}
		else if (Ban->m_pNext == NULL)
		{
			Ban->m_pPrev->m_pNext = nullptr;
			m_pPermBanListE = Ban->m_pPrev;
		}
		else
		{
			Ban->m_pPrev->m_pNext = Ban->m_pNext;
			Ban->m_pNext->m_pPrev = Ban->m_pPrev;
		}
	}
	else
	{
		if (Ban->m_pPrev == NULL)
		{
			if (Ban->m_pNext == NULL)
			{
				m_pTempBanListS = nullptr;
				m_pTempBanListE = nullptr;
			}
			else
			{
				Ban->m_pNext->m_pPrev = nullptr;
				m_pTempBanListS = Ban->m_pNext;
			}
		}
		else if (Ban->m_pNext == NULL)
		{
			Ban->m_pPrev->m_pNext = nullptr;
			m_pTempBanListE = Ban->m_pPrev;
		}
		else
		{
			Ban->m_pPrev->m_pNext = Ban->m_pNext;
			Ban->m_pNext->m_pPrev = Ban->m_pPrev;
		}
	}

#ifdef _BUILD_GUI
	if (bFromGui == false && BansDialog::m_Ptr != NULL)
	{
		BansDialog::m_Ptr->RemoveBan(Ban);
	}
#endif
}
//---------------------------------------------------------------------------

void BanManager::RemFromTable(BanItem *Ban)
{
	if (((Ban->m_ui8Bits & IP) == IP) == true)
	{
		RemFromIpTable(Ban);
	}

	if (((Ban->m_ui8Bits & NICK) == NICK) == true)
	{
		RemFromNickTable(Ban);
	}
}
//---------------------------------------------------------------------------

void BanManager::RemFromNickTable(BanItem *Ban)
{
	if (Ban->m_pHashNickTablePrev == NULL)
	{
		const uint16_t ui16dx = CalcHash(Ban->m_ui32NickHash);

		if (Ban->m_pHashNickTableNext == NULL)
		{
			m_pNickBanTable[ui16dx] = nullptr;
		}
		else
		{
			Ban->m_pHashNickTableNext->m_pHashNickTablePrev = nullptr;
			m_pNickBanTable[ui16dx] = Ban->m_pHashNickTableNext;
		}
	}
	else if (Ban->m_pHashNickTableNext == NULL)
	{
		Ban->m_pHashNickTablePrev->m_pHashNickTableNext = nullptr;
	}
	else
	{
		Ban->m_pHashNickTablePrev->m_pHashNickTableNext = Ban->m_pHashNickTableNext;
		Ban->m_pHashNickTableNext->m_pHashNickTablePrev = Ban->m_pHashNickTablePrev;
	}

	Ban->m_pHashNickTablePrev = nullptr;
	Ban->m_pHashNickTableNext = nullptr;
}
//---------------------------------------------------------------------------

void BanManager::RemFromIpTable(BanItem *Ban)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)Ban->m_ui128IpHash))
	{
		ui16IpTableIdx = Ban->m_ui128IpHash[14] * Ban->m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(Ban->m_ui128IpHash);
	}

	if (Ban->m_pHashIpTablePrev == NULL)
	{
		IpTableItem * cur = NULL,
		              * next = m_pIpBanTable[ui16IpTableIdx];

		while (next != NULL)
		{
			cur = next;
			next = cur->m_pNext;

			if (memcmp(cur->pFirstBan->m_ui128IpHash, Ban->m_ui128IpHash, 16) == 0)
			{
				if (Ban->m_pHashIpTableNext == NULL)
				{
					if (cur->m_pPrev == NULL)
					{
						if (cur->m_pNext == NULL)
						{
							m_pIpBanTable[ui16IpTableIdx] = nullptr;
						}
						else
						{
							cur->m_pNext->m_pPrev = nullptr;
							m_pIpBanTable[ui16IpTableIdx] = cur->m_pNext;
						}
					}
					else if (cur->m_pNext == NULL)
					{
						cur->m_pPrev->m_pNext = nullptr;
					}
					else
					{
						cur->m_pPrev->m_pNext = cur->m_pNext;
						cur->m_pNext->m_pPrev = cur->m_pPrev;
					}

					delete cur;
				}
				else
				{
					Ban->m_pHashIpTableNext->m_pHashIpTablePrev = nullptr;
					cur->pFirstBan = Ban->m_pHashIpTableNext;
				}

				break;
			}
		}
	}
	else if (Ban->m_pHashIpTableNext == NULL)
	{
		Ban->m_pHashIpTablePrev->m_pHashIpTableNext = nullptr;
	}
	else
	{
		Ban->m_pHashIpTablePrev->m_pHashIpTableNext = Ban->m_pHashIpTableNext;
		Ban->m_pHashIpTableNext->m_pHashIpTablePrev = Ban->m_pHashIpTablePrev;
	}

	Ban->m_pHashIpTablePrev = nullptr;
	Ban->m_pHashIpTableNext = nullptr;
}
//---------------------------------------------------------------------------

BanItem* BanManager::Find(BanItem *Ban)
{
	if (m_pTempBanListS != NULL)
	{
		time_t acc_time;
		time(&acc_time);

		BanItem * curBan = NULL,
		          * nextBan = m_pTempBanListS;

		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;

			if (acc_time > curBan->m_tTempBanExpire)
			{
				Rem(curBan);
				delete curBan;

				continue;
			}

			if (curBan == Ban)
			{
				return curBan;
			}
		}
	}

	if (m_pPermBanListS != NULL)
	{
		BanItem * curBan = NULL,
		          * nextBan = m_pPermBanListS;

		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;

			if (curBan == Ban)
			{
				return curBan;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

void BanManager::Remove(BanItem *Ban)
{
	if (m_pTempBanListS != NULL)
	{
		time_t acc_time;
		time(&acc_time);

		BanItem * curBan = NULL,
		          * nextBan = m_pTempBanListS;

		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;

			if (acc_time > curBan->m_tTempBanExpire)
			{
				Rem(curBan);
				delete curBan;

				continue;
			}

			if (curBan == Ban)
			{
				Rem(Ban);
				delete Ban;

				return;
			}
		}
	}

	if (m_pPermBanListS != NULL)
	{
		BanItem * curBan = NULL,
		          * nextBan = m_pPermBanListS;

		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;

			if (curBan == Ban)
			{
				Rem(Ban);
				delete Ban;

				return;
			}
		}
	}
}
//---------------------------------------------------------------------------

void BanManager::AddRange(RangeBanItem *RangeBan)
{
	if (m_pRangeBanListE == NULL)
	{
		m_pRangeBanListS = RangeBan;
		m_pRangeBanListE = RangeBan;
	}
	else
	{
		m_pRangeBanListE->m_pNext = RangeBan;
		RangeBan->m_pPrev = m_pRangeBanListE;
		m_pRangeBanListE = RangeBan;
	}

#ifdef _BUILD_GUI
	if (RangeBansDialog::m_Ptr != NULL)
	{
		RangeBansDialog::m_Ptr->AddRangeBan(RangeBan);
	}
#endif
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void BanManager::RemRange(RangeBanItem *RangeBan, const bool bFromGui/* = false*/)
{
#else
void BanManager::RemRange(RangeBanItem *RangeBan, const bool /*bFromGui = false*/)
{
#endif
	if (RangeBan->m_pPrev == NULL)
	{
		if (RangeBan->m_pNext == NULL)
		{
			m_pRangeBanListS = nullptr;
			m_pRangeBanListE = nullptr;
		}
		else
		{
			RangeBan->m_pNext->m_pPrev = nullptr;
			m_pRangeBanListS = RangeBan->m_pNext;
		}
	}
	else if (RangeBan->m_pNext == NULL)
	{
		RangeBan->m_pPrev->m_pNext = nullptr;
		m_pRangeBanListE = RangeBan->m_pPrev;
	}
	else
	{
		RangeBan->m_pPrev->m_pNext = RangeBan->m_pNext;
		RangeBan->m_pNext->m_pPrev = RangeBan->m_pPrev;
	}

#ifdef _BUILD_GUI
	if (bFromGui == false && RangeBansDialog::m_Ptr != NULL)
	{
		RangeBansDialog::m_Ptr->RemoveRangeBan(RangeBan);
	}
#endif
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(RangeBanItem *RangeBan)
{
	if (m_pRangeBanListS != NULL)
	{
		time_t acc_time;
		time(&acc_time);

		RangeBanItem * curBan = NULL,
		               * nextBan = m_pRangeBanListS;

		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;

			if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true && acc_time > curBan->m_tTempBanExpire)
			{
				RemRange(curBan);
				delete curBan;

				continue;
			}

			if (curBan == RangeBan)
			{
				return curBan;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

void BanManager::RemoveRange(RangeBanItem *RangeBan)
{
	if (m_pRangeBanListS != NULL)
	{
		time_t acc_time;
		time(&acc_time);

		RangeBanItem * curBan = NULL,
		               * nextBan = m_pRangeBanListS;

		while (nextBan != NULL)
		{
			curBan = nextBan;
			nextBan = curBan->m_pNext;

			if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true && acc_time > curBan->m_tTempBanExpire)
			{
				RemRange(curBan);
				delete curBan;

				continue;
			}

			if (curBan == RangeBan)
			{
				RemRange(RangeBan);
				delete RangeBan;

				return;
			}
		}
	}
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(User* pUser)
{
	const uint16_t ui16dx = CalcHash(pUser->m_ui32NickHash);

	time_t acc_time;
	time(&acc_time);

	BanItem * cur = NULL,
	          * next = m_pNickBanTable[ui16dx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pHashNickTableNext;

		if (cur->m_ui32NickHash == pUser->m_ui32NickHash && strcasecmp(cur->m_sNick, pUser->m_sNick) == 0)
		{
			// PPK ... check if temban expired
			if (((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
			{
				if (acc_time >= cur->m_tTempBanExpire)
				{
					Rem(cur);
					delete cur;

					continue;
				}
			}
			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindIP(User* u)
{
	time_t acc_time;
	time(&acc_time);

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[u->m_ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, u->m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				// PPK ... check if temban expired
				if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
				{
					if (acc_time >= curBan->m_tTempBanExpire)
					{
						Rem(curBan);
						delete curBan;

						continue;
					}
				}

				return curBan;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(User* u)
{
	time_t acc_time;
	time(&acc_time);

	RangeBanItem * cur = NULL,
	               * next = m_pRangeBanListS;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->m_ui128FromIpHash, u->m_ui128IpHash, 16) <= 0 && memcmp(cur->m_ui128ToIpHash, u->m_ui128IpHash, 16) >= 0)
		{
			// PPK ... check if temban expired
			if (((cur->m_ui8Bits & TEMP) == TEMP) == true)
			{
				if (acc_time >= cur->m_tTempBanExpire)
				{
					RemRange(cur);
					delete cur;

					continue;
				}
			}
			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindFull(const uint8_t * m_ui128IpHash)
{
	time_t acc_time;
	time(&acc_time);

	return FindFull(m_ui128IpHash, acc_time);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindFull(const uint8_t * m_ui128IpHash, const time_t &acc_time)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)m_ui128IpHash))
	{
		ui16IpTableIdx = m_ui128IpHash[14] * m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(m_ui128IpHash);
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = NULL, * fnd = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				// PPK ... check if temban expired
				if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
				{
					if (acc_time >= curBan->m_tTempBanExpire)
					{
						Rem(curBan);
						delete curBan;

						continue;
					}
				}

				if (((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true)
				{
					return curBan;
				}
				else if (fnd == NULL)
				{
					fnd = curBan;
				}
			}
		}
	}

	return fnd;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindFullRange(const uint8_t * m_ui128IpHash, const time_t &acc_time)
{
	RangeBanItem * cur = NULL, * fnd = NULL,
	               * next = m_pRangeBanListS;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->m_ui128FromIpHash, m_ui128IpHash, 16) <= 0 && memcmp(cur->m_ui128ToIpHash, m_ui128IpHash, 16) >= 0)
		{
			// PPK ... check if temban expired
			if (((cur->m_ui8Bits & TEMP) == TEMP) == true)
			{
				if (acc_time >= cur->m_tTempBanExpire)
				{
					RemRange(cur);
					delete cur;

					continue;
				}
			}

			if (((cur->m_ui8Bits & FULL) == FULL) == true)
			{
				return cur;
			}
			else if (fnd == NULL)
			{
				fnd = cur;
			}
		}
	}

	return fnd;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(const char * sNick, const size_t szNickLen)
{
	uint32_t hash = HashNick(sNick, szNickLen);

	time_t acc_time;
	time(&acc_time);

	return FindNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(const uint32_t ui32Hash, const time_t &acc_time, const char * sNick)
{
	const uint16_t ui16dx = CalcHash(ui32Hash);

	BanItem * cur = NULL,
	          * next = m_pNickBanTable[ui16dx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pHashNickTableNext;

		if (cur->m_ui32NickHash == ui32Hash && strcasecmp(cur->m_sNick, sNick) == 0)
		{
			// PPK ... check if temban expired
			if (((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
			{
				if (acc_time >= cur->m_tTempBanExpire)
				{
					Rem(cur);
					delete cur;

					continue;
				}
			}
			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindIP(const uint8_t * m_ui128IpHash, const time_t &acc_time)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)m_ui128IpHash))
	{
		ui16IpTableIdx = m_ui128IpHash[14] * m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(m_ui128IpHash);
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				// PPK ... check if temban expired
				if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
				{
					if (acc_time >= curBan->m_tTempBanExpire)
					{
						Rem(curBan);
						delete curBan;

						continue;
					}
				}

				return curBan;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(const uint8_t * m_ui128IpHash, const time_t &acc_time)
{
	RangeBanItem * cur = NULL,
	               * next = m_pRangeBanListS;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		// PPK ... check if temban expired
		if (((cur->m_ui8Bits & TEMP) == TEMP) == true)
		{
			if (acc_time >= cur->m_tTempBanExpire)
			{
				RemRange(cur);
				delete cur;

				continue;
			}
		}

		if (memcmp(cur->m_ui128FromIpHash, m_ui128IpHash, 16) <= 0 && memcmp(cur->m_ui128ToIpHash, m_ui128IpHash, 16) >= 0)
		{
			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(const uint8_t * ui128FromHash, const uint8_t * ui128ToHash, const time_t &acc_time)
{
	RangeBanItem * cur = NULL,
	               * next = m_pRangeBanListS;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0)
		{
			// PPK ... check if temban expired
			if (((cur->m_ui8Bits & TEMP) == TEMP) == true)
			{
				if (acc_time >= cur->m_tTempBanExpire)
				{
					RemRange(cur);
					delete cur;

					continue;
				}
			}

			return cur;
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempNick(const char * sNick, const size_t szNickLen)
{
	uint32_t hash = HashNick(sNick, szNickLen);

	time_t acc_time;
	time(&acc_time);

	return FindTempNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempNick(const uint32_t ui32Hash,  const time_t &acc_time, const char * sNick)
{
	const uint16_t ui16dx = CalcHash(ui32Hash);

	BanItem * cur = NULL,
	          * next = m_pNickBanTable[ui16dx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pHashNickTableNext;

		if (cur->m_ui32NickHash == ui32Hash && strcasecmp(cur->m_sNick, sNick) == 0)
		{
			// PPK ... check if temban expired
			if (((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
			{
				if (acc_time >= cur->m_tTempBanExpire)
				{
					Rem(cur);
					delete cur;

					continue;
				}
				return cur;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempIP(const uint8_t * m_ui128IpHash, const time_t &acc_time)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)m_ui128IpHash))
	{
		ui16IpTableIdx = m_ui128IpHash[14] * m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(m_ui128IpHash);
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				// PPK ... check if temban expired
				if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
				{
					if (acc_time >= curBan->m_tTempBanExpire)
					{
						Rem(curBan);
						delete curBan;

						continue;
					}

					return curBan;
				}
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermNick(const char * sNick, const size_t szNickLen)
{
	uint32_t hash = HashNick(sNick, szNickLen);

	return FindPermNick(hash, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermNick(const uint32_t ui32Hash, const char * sNick)
{
	const uint16_t ui16dx = CalcHash(ui32Hash);

	BanItem * cur = NULL,
	          * next = m_pNickBanTable[ui16dx];

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pHashNickTableNext;

		if (cur->m_ui32NickHash == ui32Hash && strcasecmp(cur->m_sNick, sNick) == 0)
		{
			if (((cur->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true)
			{
				return cur;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermIP(const uint8_t * m_ui128IpHash)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)m_ui128IpHash))
	{
		ui16IpTableIdx = m_ui128IpHash[14] * m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(m_ui128IpHash);
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				if (((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true)
				{
					return curBan;
				}
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

void BanManager::Load()
{
#ifdef _WIN32
	if (FileExist((ServerManager::m_sPath + "\\cfg\\Bans.pxb").c_str()) == false)
	{
#else
	if (FileExist((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str()) == false)
	{
#endif
		LoadXML();
		return;
	}

	PXBReader pxbBans;

	// Open regs file
#ifdef _WIN32
	if (pxbBans.OpenFileRead((ServerManager::m_sPath + "\\cfg\\Bans.pxb").c_str(), 9) == false)
	{
#else
	if (pxbBans.OpenFileRead((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str(), 9) == false)
	{
#endif
		return;
	}

	// Read file header
	uint16_t ui16Identificators[9];
	ui16Identificators[0] = *((uint16_t *)"FI");
	ui16Identificators[1] = *((uint16_t *)"FV");

	if (pxbBans.ReadNextItem(ui16Identificators, 2) == false)
	{
		return;
	}

	// Check header if we have correct file
	if (pxbBans.m_ui16ItemLengths[0] != szPtokaXBansLen || strncmp((char *)pxbBans.m_pItemDatas[0], sPtokaXBans, szPtokaXBansLen) != 0)
	{
		return;
	}

	{
		uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbBans.m_pItemDatas[1])));

		if (ui32FileVersion < 1)
		{
			return;
		}
	}

	time_t tmAccTime;
	time(&tmAccTime);

	// "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX"
	memcpy(ui16Identificators, sBanIds, szBanIdsLen);

	bool bSuccess = pxbBans.ReadNextItem(ui16Identificators, 9);

	while (bSuccess == true)
	{
		BanItem * pBan = new (std::nothrow) BanItem();
		if (pBan == NULL)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::Load\n");
			exit(EXIT_FAILURE);
		}

		// Permanent or temporary ban?
		pBan->m_ui8Bits |= (((char *)pxbBans.m_pItemDatas[0])[0] == '0' ? PERM : TEMP);

		// Do we have some nick?
		if (pxbBans.m_ui16ItemLengths[1] != 0)
		{
			if (pxbBans.m_ui16ItemLengths[1] > 64)
			{
				AppendDebugLogFormat("[ERR] sNick too long %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[1]);

				exit(EXIT_FAILURE);
			}
			pBan->m_sNick = (char *)malloc(pxbBans.m_ui16ItemLengths[1] + 1);
			if (pBan->m_sNick == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sNick in BanManager::Load\n", pxbBans.m_ui16ItemLengths[1] + 1);

				exit(EXIT_FAILURE);
			}

			memcpy(pBan->m_sNick, pxbBans.m_pItemDatas[1], pxbBans.m_ui16ItemLengths[1]);
			pBan->m_sNick[pxbBans.m_ui16ItemLengths[1]] = '\0';
			pBan->m_ui32NickHash = HashNick(pBan->m_sNick, pxbBans.m_ui16ItemLengths[1]);

			// it is nick ban?
			if (((char *)pxbBans.m_pItemDatas[2])[0] != '0')
			{
				pBan->m_ui8Bits |= NICK;
			}
		}

		// Do we have some IP address?
		if (pxbBans.m_ui16ItemLengths[3] != 0 && IN6_IS_ADDR_UNSPECIFIED((const in6_addr *)pxbBans.m_pItemDatas[3]) == 0)
		{
			if (pxbBans.m_ui16ItemLengths[3] != 16)
			{
				AppendDebugLogFormat("[ERR] Ban IP address have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[3]);

				exit(EXIT_FAILURE);
			}

			memcpy(pBan->m_ui128IpHash, pxbBans.m_pItemDatas[3], 16);

			if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)pBan->m_ui128IpHash))
			{
				in_addr ipv4addr;
				memcpy(&ipv4addr, pBan->m_ui128IpHash + 12, 4);
				pBan->initIP(inet_ntoa(ipv4addr));
			}
			else
			{
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
				win_inet_ntop(pBan->m_ui128IpHash, pBan->m_sIp, 40);
#else
				inet_ntop(AF_INET6, pBan->m_ui128IpHash, pBan->m_sIp, 40);
#endif
			}

			// it is IP ban?
			if (((char *)pxbBans.m_pItemDatas[4])[0] != '0')
			{
				pBan->m_ui8Bits |= IP;
			}

			// it is full ban ?
			if (((char *)pxbBans.m_pItemDatas[5])[0] != '0')
			{
				pBan->m_ui8Bits |= FULL;
			}
		}

		// Do we have reason?
		if (pxbBans.m_ui16ItemLengths[6] != 0)
		{
			if (pxbBans.m_ui16ItemLengths[6] > 511)
			{
				pxbBans.m_ui16ItemLengths[6] = 511;
			}

			pBan->m_sReason = (char *)malloc(pxbBans.m_ui16ItemLengths[6] + 1);
			if (pBan->m_sReason == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sReason in BanManager::Load\n", pxbBans.m_ui16ItemLengths[6] + 1);

				exit(EXIT_FAILURE);
			}

			memcpy(pBan->m_sReason, pxbBans.m_pItemDatas[6], pxbBans.m_ui16ItemLengths[6]);
			pBan->m_sReason[pxbBans.m_ui16ItemLengths[6]] = '\0';
		}

		// Do we have who created ban?
		if (pxbBans.m_ui16ItemLengths[7] != 0)
		{
			if (pxbBans.m_ui16ItemLengths[7] > 64)
			{
				pxbBans.m_ui16ItemLengths[7] = 64;
			}

			pBan->m_sBy = (char *)malloc(pxbBans.m_ui16ItemLengths[7] + 1);
			if (pBan->m_sBy == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sBy in BanManager::Load\n", pxbBans.m_ui16ItemLengths[7] + 1);

				exit(EXIT_FAILURE);
			}

			memcpy(pBan->m_sBy, pxbBans.m_pItemDatas[7], pxbBans.m_ui16ItemLengths[7]);
			pBan->m_sBy[pxbBans.m_ui16ItemLengths[7]] = '\0';
		}

		// it is temporary ban?
		if (((pBan->m_ui8Bits & TEMP) == TEMP) == true)
		{
			if (pxbBans.m_ui16ItemLengths[8] != 8)
			{
				AppendDebugLogFormat("[ERR] Temp ban expire time have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[8]);

				exit(EXIT_FAILURE);
			}
			else
			{
				// Temporary ban expiration datetime
				pBan->m_tTempBanExpire = (time_t)be64toh(*((uint64_t *)(pxbBans.m_pItemDatas[8])));

				if (tmAccTime >= pBan->m_tTempBanExpire)
				{
					delete pBan;
				}
				else
				{
					if (Add(pBan) == false)
					{
						AppendDebugLog("%s [ERR] Add ban failed in BanManager::Load\n");

						exit(EXIT_FAILURE);
					}
				}
			}
		}
		else
		{
			if (Add(pBan) == false)
			{
				AppendDebugLog("%s [ERR] Add2 ban failed in BanManager::Load\n");

				exit(EXIT_FAILURE);
			}
		}

		bSuccess = pxbBans.ReadNextItem(ui16Identificators, 9);
	}

	PXBReader pxbRangeBans;

	// Open regs file
#ifdef _WIN32
	if (pxbRangeBans.OpenFileRead((ServerManager::m_sPath + "\\cfg\\RangeBans.pxb").c_str(), 7) == false)
	{
#else
	if (pxbRangeBans.OpenFileRead((ServerManager::m_sPath + "/cfg/RangeBans.pxb").c_str(), 7) == false)
	{
#endif
		return;
	}

	// Read file header
	ui16Identificators[0] = *((uint16_t *)"FI");
	ui16Identificators[1] = *((uint16_t *)"FV");

	if (pxbRangeBans.ReadNextItem(ui16Identificators, 2) == false)
	{
		return;
	}

	// Check header if we have correct file
	if (pxbRangeBans.m_ui16ItemLengths[0] != szPtokaXRangeBansLen || strncmp((char *)pxbRangeBans.m_pItemDatas[0], sPtokaXRangeBans, szPtokaXRangeBansLen) != 0)
	{
		return;
	}

	{
		uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbRangeBans.m_pItemDatas[1])));

		if (ui32FileVersion < 1)
		{
			return;
		}
	}

	// // "BT" "RF" "RT" "FB" "RE" "BY" "EX";
	memcpy(ui16Identificators, sRangeBanIds, szRangeBanIdsLen);

	bSuccess = pxbRangeBans.ReadNextItem(ui16Identificators, 7);

	while (bSuccess == true)
	{
		RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
		if (pRangeBan == NULL)
		{
			AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in BanManager::Load\n");
			exit(EXIT_FAILURE);
		}

		// Permanent or temporary ban?
		pRangeBan->m_ui8Bits |= (((char *)pxbRangeBans.m_pItemDatas[0])[0] == '0' ? PERM : TEMP);

		// Do we have first IP address?
		if (pxbRangeBans.m_ui16ItemLengths[1] != 16)
		{
			AppendDebugLogFormat("[ERR] Range Ban first IP address have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[1]);

			exit(EXIT_FAILURE);
		}

		memcpy(pRangeBan->m_ui128FromIpHash, pxbRangeBans.m_pItemDatas[1], 16);

		if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)pRangeBan->m_ui128FromIpHash.data()))
		{
			in_addr ipv4addr;
			memcpy(&ipv4addr, pRangeBan->m_ui128FromIpHash + 12, 4);
			strcpy(pRangeBan->m_sIpFrom, inet_ntoa(ipv4addr));
		}
		else
		{
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
			win_inet_ntop(pRangeBan->m_ui128FromIpHash, pRangeBan->m_sIpFrom, 40);
#else
			inet_ntop(AF_INET6, pRangeBan->m_ui128FromIpHash, pRangeBan->m_sIpFrom, 40);
#endif
		}

		// Do we have second IP address?
		if (pxbRangeBans.m_ui16ItemLengths[2] != 16)
		{
			AppendDebugLogFormat("[ERR] Range Ban second IP address have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[2]);

			exit(EXIT_FAILURE);
		}

		memcpy(pRangeBan->m_ui128ToIpHash, pxbRangeBans.m_pItemDatas[2], 16);

		if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)pRangeBan->m_ui128ToIpHash.data()))
		{
			in_addr ipv4addr;
			memcpy(&ipv4addr, pRangeBan->m_ui128ToIpHash + 12, 4);
			strcpy(pRangeBan->m_sIpTo, inet_ntoa(ipv4addr));
		}
		else
		{
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
			win_inet_ntop(pRangeBan->m_ui128ToIpHash, pRangeBan->m_sIpTo, 40);
#else
			inet_ntop(AF_INET6, pRangeBan->m_ui128ToIpHash, pRangeBan->m_sIpTo, 40);
#endif
		}

		// it is full ban ?
		if (((char *)pxbRangeBans.m_pItemDatas[3])[0] != '0')
		{
			pRangeBan->m_ui8Bits |= FULL;
		}

		// Do we have reason?
		if (pxbRangeBans.m_ui16ItemLengths[4] != 0)
		{
			if (pxbRangeBans.m_ui16ItemLengths[4] > 511)
			{
				pxbRangeBans.m_ui16ItemLengths[4] = 511;
			}

			pRangeBan->m_sReason = (char *)malloc(pxbRangeBans.m_ui16ItemLengths[4] + 1);
			if (pRangeBan->m_sReason == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sReason2 in BanManager::Load\n", pxbRangeBans.m_ui16ItemLengths[4] + 1);

				exit(EXIT_FAILURE);
			}

			memcpy(pRangeBan->m_sReason, pxbRangeBans.m_pItemDatas[4], pxbRangeBans.m_ui16ItemLengths[4]);
			pRangeBan->m_sReason[pxbRangeBans.m_ui16ItemLengths[4]] = '\0';
		}

		// Do we have who created ban?
		if (pxbRangeBans.m_ui16ItemLengths[5] != 0)
		{
			if (pxbRangeBans.m_ui16ItemLengths[5] > 64)
			{
				pxbRangeBans.m_ui16ItemLengths[5] = 64;
			}

			pRangeBan->m_sBy = (char *)malloc(pxbRangeBans.m_ui16ItemLengths[5] + 1);
			if (pRangeBan->m_sBy == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sBy2 in BanManager::Load\n", pxbRangeBans.m_ui16ItemLengths[5] + 1);

				exit(EXIT_FAILURE);
			}

			memcpy(pRangeBan->m_sBy, pxbRangeBans.m_pItemDatas[5], pxbRangeBans.m_ui16ItemLengths[5]);
			pRangeBan->m_sBy[pxbRangeBans.m_ui16ItemLengths[5]] = '\0';
		}

		// it is temporary ban?
		if (((pRangeBan->m_ui8Bits & TEMP) == TEMP) == true)
		{
			if (pxbRangeBans.m_ui16ItemLengths[6] != 8)
			{
				AppendDebugLogFormat("[ERR] Temp range ban expire time have incorrect lenght %hu in BanManager::Load\n", pxbRangeBans.m_ui16ItemLengths[6]);

				exit(EXIT_FAILURE);
			}
			else
			{
				// Temporary ban expiration datetime
				pRangeBan->m_tTempBanExpire = (time_t)be64toh(*((uint64_t *)(pxbRangeBans.m_pItemDatas[6])));

				if (tmAccTime >= pRangeBan->m_tTempBanExpire)
				{
					delete pRangeBan;
				}
				else
				{
					AddRange(pRangeBan);
				}
			}
		}
		else
		{
			AddRange(pRangeBan);
		}

		bSuccess = pxbRangeBans.ReadNextItem(ui16Identificators, 7);
	}

}
//---------------------------------------------------------------------------

void BanManager::LoadXML()
{
	double dVer;

#ifdef _WIN32
	TiXmlDocument doc((ServerManager::m_sPath + "\\cfg\\BanList.xml").c_str());
#else
	TiXmlDocument doc((ServerManager::m_sPath + "/cfg/BanList.xml").c_str());
#endif

	if (doc.LoadFile() == false)
	{
		if (doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY)
		{
			int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file BanList.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
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
		TiXmlElement *banlist = cfg.FirstChild("BanList").Element();
		if (banlist != NULL)
		{
			if (banlist->Attribute("version", &dVer) == NULL)
			{
				return;
			}

			time_t acc_time;
			time(&acc_time);

			TiXmlNode *bans = banlist->FirstChild("Bans");
			if (bans != NULL)
			{
				TiXmlNode *child = nullptr;
				while ((child = bans->IterateChildren(child)) != NULL)
				{
					const char *ip = NULL, *nick = NULL, *reason = NULL, *by = nullptr;
					TiXmlNode *ban = child->FirstChild("Type");

					if (ban == NULL || (ban = ban->FirstChild()) == NULL)
					{
						continue;
					}

					char type = atoi(ban->Value()) == 0 ? (char)0 : (char)1;

					if ((ban = child->FirstChild("IP")) != NULL &&
					        (ban = ban->FirstChild()) != NULL)
					{
						ip = ban->Value();
					}

					if ((ban = child->FirstChild("Nick")) != NULL &&
					        (ban = ban->FirstChild()) != NULL)
					{
						nick = ban->Value();
					}

					if ((ban = child->FirstChild("Reason")) != NULL &&
					        (ban = ban->FirstChild()) != NULL)
					{
						reason = ban->Value();
					}

					if ((ban = child->FirstChild("By")) != NULL &&
					        (ban = ban->FirstChild()) != NULL)
					{
						by = ban->Value();
					}

					if ((ban = child->FirstChild("NickBan")) == NULL ||
					        (ban = ban->FirstChild()) == NULL)
					{
						continue;
					}

					bool nickban = (atoi(ban->Value()) == 0 ? false : true);

					if ((ban = child->FirstChild("IpBan")) == NULL ||
					        (ban = ban->FirstChild()) == NULL)
					{
						continue;
					}

					bool ipban = (atoi(ban->Value()) == 0 ? false : true);

					if ((ban = child->FirstChild("FullIpBan")) == NULL ||
					        (ban = ban->FirstChild()) == NULL)
					{
						continue;
					}

					bool fullipban = (atoi(ban->Value()) == 0 ? false : true);

					BanItem * Ban = new (std::nothrow) BanItem();
					if (Ban == NULL)
					{
						AppendDebugLog("%s - [MEM] Cannot allocate Ban in BanManager::LoadXML\n");
						exit(EXIT_FAILURE);
					}

					if (type == 0)
					{
						Ban->m_ui8Bits |= PERM;
					}
					else
					{
						Ban->m_ui8Bits |= TEMP;
					}

					// PPK ... ipban
					if (ipban == true)
					{
						if (ip != NULL && HashIP(ip, Ban->m_ui128IpHash) == true)
						{
							strcpy(Ban->m_sIp, ip);
							Ban->m_ui8Bits |= IP;

							if (fullipban == true)
							{
								Ban->m_ui8Bits |= FULL;
							}
						}
						else
						{
							delete Ban;

							continue;
						}
					}

					// PPK ... nickban
					if (nickban == true)
					{
						if (nick != NULL)
						{
							size_t szNickLen = strlen(nick);
							Ban->m_sNick = (char *)malloc(szNickLen + 1);
							if (Ban->m_sNick == NULL)
							{
								AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::LoadXML\n", szNickLen+1);

								exit(EXIT_FAILURE);
							}

							memcpy(Ban->m_sNick, nick, szNickLen);
							Ban->m_sNick[szNickLen] = '\0';
							Ban->m_ui32NickHash = HashNick(Ban->m_sNick, strlen(Ban->m_sNick));
							Ban->m_ui8Bits |= NICK;
						}
						else
						{
							delete Ban;

							continue;
						}
					}

					if (reason != NULL)
					{
						size_t szReasonLen = strlen(reason);
						if (szReasonLen > 255)
						{
							szReasonLen = 255;
						}
						Ban->m_sReason = (char *)malloc(szReasonLen + 1);
						if (Ban->m_sReason == NULL)
						{
							AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::LoadXML\n", szReasonLen+1);

							exit(EXIT_FAILURE);
						}

						memcpy(Ban->m_sReason, reason, szReasonLen);
						Ban->m_sReason[szReasonLen] = '\0';
					}

					if (by != NULL)
					{
						size_t szByLen = strlen(by);
						if (szByLen > 63)
						{
							szByLen = 63;
						}
						Ban->m_sBy = (char *)malloc(szByLen + 1);
						if (Ban->m_sBy == NULL)
						{
							AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy1 in BanManager::LoadXML\n", szByLen+1);
							exit(EXIT_FAILURE);
						}

						memcpy(Ban->m_sBy, by, szByLen);
						Ban->m_sBy[szByLen] = '\0';
					}

					// PPK ... temp ban
					if (((Ban->m_ui8Bits & TEMP) == TEMP) == true)
					{
						if ((ban = child->FirstChild("Expire")) == NULL ||
						        (ban = ban->FirstChild()) == NULL)
						{
							delete Ban;

							continue;
						}
						time_t expire = strtoul(ban->Value(), NULL, 10);

						if (acc_time > expire)
						{
							delete Ban;

							continue;
						}

						// PPK ... temp ban expiration
						Ban->m_tTempBanExpire = expire;
					}

					if (fullipban == true)
					{
						Ban->m_ui8Bits |= FULL;
					}

					if (Add(Ban) == false)
					{
						exit(EXIT_FAILURE);
					}
				}
			}

			TiXmlNode *rangebans = banlist->FirstChild("RangeBans");
			if (rangebans != NULL)
			{
				TiXmlNode *child = nullptr;
				while ((child = rangebans->IterateChildren(child)) != NULL)
				{
					const char *reason = NULL, *by = nullptr;
					TiXmlNode *rangeban = child->FirstChild("Type");

					if (rangeban == NULL ||
					        (rangeban = rangeban->FirstChild()) == NULL)
					{
						continue;
					}

					char type = atoi(rangeban->Value()) == 0 ? (char)0 : (char)1;

					if ((rangeban = child->FirstChild("IpFrom")) == NULL ||
					        (rangeban = rangeban->FirstChild()) == NULL)
					{
						continue;
					}

					const char *ipfrom = rangeban->Value();

					if ((rangeban = child->FirstChild("IpTo")) == NULL ||
					        (rangeban = rangeban->FirstChild()) == NULL)
					{
						continue;
					}

					const char *ipto = rangeban->Value();

					if ((rangeban = child->FirstChild("Reason")) != NULL &&
					        (rangeban = rangeban->FirstChild()) != NULL)
					{
						reason = rangeban->Value();
					}

					if ((rangeban = child->FirstChild("By")) != NULL &&
					        (rangeban = rangeban->FirstChild()) != NULL)
					{
						by = rangeban->Value();
					}

					if ((rangeban = child->FirstChild("FullIpBan")) == NULL ||
					        (rangeban = rangeban->FirstChild()) == NULL)
					{
						continue;
					}

					bool fullipban = (atoi(rangeban->Value()) == 0 ? false : true);

					RangeBanItem * RangeBan = new (std::nothrow) RangeBanItem();
					if (RangeBan == NULL)
					{
						AppendDebugLog("%s - [MEM] Cannot allocate RangeBan in BanManager::LoadXML\n");
						exit(EXIT_FAILURE);
					}

					if (type == 0)
					{
						RangeBan->m_ui8Bits |= PERM;
					}
					else
					{
						RangeBan->m_ui8Bits |= TEMP;
					}

					// PPK ... fromip
					if (HashIP(ipfrom, RangeBan->m_ui128FromIpHash) == true)
					{
						strcpy(RangeBan->m_sIpFrom, ipfrom);
					}
					else
					{
						delete RangeBan;

						continue;
					}

					// PPK ... toip
					if (HashIP(ipto, RangeBan->m_ui128ToIpHash) == true && memcmp(RangeBan->m_ui128ToIpHash, RangeBan->m_ui128FromIpHash, 16) > 0)
					{
						strcpy(RangeBan->m_sIpTo, ipto);
					}
					else
					{
						delete RangeBan;

						continue;
					}

					if (reason != NULL)
					{
						size_t szReasonLen = strlen(reason);
						if (szReasonLen > 255)
						{
							szReasonLen = 255;
						}
						RangeBan->m_sReason = (char *)malloc(szReasonLen + 1);
						if (RangeBan->m_sReason == NULL)
						{
							AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason3 in BanManager::LoadXML\n", szReasonLen+1);
							exit(EXIT_FAILURE);
						}

						memcpy(RangeBan->m_sReason, reason, szReasonLen);
						RangeBan->m_sReason[szReasonLen] = '\0';
					}

					if (by != NULL)
					{
						size_t szByLen = strlen(by);
						if (szByLen > 63)
						{
							szByLen = 63;
						}
						RangeBan->m_sBy = (char *)malloc(szByLen + 1);
						if (RangeBan->m_sBy == NULL)
						{
							AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy3 in BanManager::LoadXML\n", szByLen+1);
							exit(EXIT_FAILURE);
						}

						memcpy(RangeBan->m_sBy, by, szByLen);
						RangeBan->m_sBy[szByLen] = '\0';
					}

					// PPK ... temp ban
					if (((RangeBan->m_ui8Bits & TEMP) == TEMP) == true)
					{
						if ((rangeban = child->FirstChild("Expire")) == NULL ||
						        (rangeban = rangeban->FirstChild()) == NULL)
						{
							delete RangeBan;

							continue;
						}
						time_t expire = strtoul(rangeban->Value(), NULL, 10);

						if (acc_time > expire)
						{
							delete RangeBan;
							continue;
						}

						// PPK ... temp ban expiration
						RangeBan->m_tTempBanExpire = expire;
					}

					if (fullipban == true)
					{
						RangeBan->m_ui8Bits |= FULL;
					}

					AddRange(RangeBan);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------

void BanManager::Save(bool bForce/* = false*/)
{
	if (bForce == false)
	{
		// we don't want waste resources with save after every change in bans
		if (m_ui32SaveCalled < 100)
		{
			m_ui32SaveCalled++;
			return;
		}
	}

	m_ui32SaveCalled = 0;

	PXBReader pxbBans;

	// Open bans file
#ifdef _WIN32
	if (pxbBans.OpenFileSave((ServerManager::m_sPath + "\\cfg\\Bans.pxb").c_str(), 9) == false)
	{
#else
	if (pxbBans.OpenFileSave((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str(), 9) == false)
	{
#endif
		return;
	}

	// Write file header
	pxbBans.m_sItemIdentifiers[0] = 'F';
	pxbBans.m_sItemIdentifiers[1] = 'I';
	pxbBans.m_ui16ItemLengths[0] = (uint16_t)szPtokaXBansLen;
	pxbBans.m_pItemDatas[0] = (void *)sPtokaXBans;
	pxbBans.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

	pxbBans.m_sItemIdentifiers[2] = 'F';
	pxbBans.m_sItemIdentifiers[3] = 'V';
	pxbBans.m_ui16ItemLengths[1] = 4;
	uint32_t ui32Version = 1;
	pxbBans.m_pItemDatas[1] = (void *)&ui32Version;
	pxbBans.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

	if (pxbBans.WriteNextItem(szPtokaXBansLen + 4, 2) == false)
	{
		return;
	}

	// "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX"
	memcpy(pxbBans.m_sItemIdentifiers, sBanIds, szBanIdsLen);

	pxbBans.m_ui8ItemValues[0] = PXBReader::PXB_BYTE;
	pxbBans.m_ui8ItemValues[1] = PXBReader::PXB_STRING;
	pxbBans.m_ui8ItemValues[2] = PXBReader::PXB_BYTE;
	pxbBans.m_ui8ItemValues[3] = PXBReader::PXB_STRING;
	pxbBans.m_ui8ItemValues[4] = PXBReader::PXB_BYTE;
	pxbBans.m_ui8ItemValues[5] = PXBReader::PXB_BYTE;
	pxbBans.m_ui8ItemValues[6] = PXBReader::PXB_STRING;
	pxbBans.m_ui8ItemValues[7] = PXBReader::PXB_STRING;
	pxbBans.m_ui8ItemValues[8] = PXBReader::PXB_EIGHT_BYTES;

	uint64_t ui64TempBanExpire = 0;

	if (m_pTempBanListS != NULL)
	{
		BanItem * pCur = NULL,
		          * m_pNext = m_pTempBanListS;

		while (m_pNext != NULL)
		{
			pCur = m_pNext;
			m_pNext = pCur->m_pNext;

			pxbBans.m_ui16ItemLengths[0] = 1;
			pxbBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) == true ? 0 : (void *)1);

			pxbBans.m_ui16ItemLengths[1] = pCur->m_sNick == NULL ? 0 : (uint16_t)strlen(pCur->m_sNick);
			pxbBans.m_pItemDatas[1] = pCur->m_sNick == NULL ? (void *)"" : (void *)pCur->m_sNick;

			pxbBans.m_ui16ItemLengths[2] = 1;
			pxbBans.m_pItemDatas[2] = (((pCur->m_ui8Bits & NICK) == NICK) == true ? (void *)1 : 0);

			pxbBans.m_ui16ItemLengths[3] = 16;
			pxbBans.m_pItemDatas[3] = (void *)pCur->m_ui128IpHash;

			pxbBans.m_ui16ItemLengths[4] = 1;
			pxbBans.m_pItemDatas[4] = (((pCur->m_ui8Bits & IP) == IP) == true ? (void *)1 : 0);

			pxbBans.m_ui16ItemLengths[5] = 1;
			pxbBans.m_pItemDatas[5] = (((pCur->m_ui8Bits & FULL) == FULL) == true ? (void *)1 : 0);

			pxbBans.m_ui16ItemLengths[6] = pCur->m_sReason == NULL ? 0 : (uint16_t)strlen(pCur->m_sReason);
			pxbBans.m_pItemDatas[6] = pCur->m_sReason == NULL ? (void *)"" : (void *)pCur->m_sReason;

			pxbBans.m_ui16ItemLengths[7] = pCur->m_sBy == NULL ? 0 : (uint16_t)strlen(pCur->m_sBy);
			pxbBans.m_pItemDatas[7] = pCur->m_sBy == NULL ? (void *)"" : (void *)pCur->m_sBy;

			pxbBans.m_ui16ItemLengths[8] = 8;
			ui64TempBanExpire = (uint64_t)pCur->m_tTempBanExpire;
			pxbBans.m_pItemDatas[8] = (void *)&ui64TempBanExpire;

			if (pxbBans.WriteNextItem(pxbBans.m_ui16ItemLengths[0] + pxbBans.m_ui16ItemLengths[1] + pxbBans.m_ui16ItemLengths[2] + pxbBans.m_ui16ItemLengths[3] + pxbBans.m_ui16ItemLengths[4] + pxbBans.m_ui16ItemLengths[5] + pxbBans.m_ui16ItemLengths[6] + pxbBans.m_ui16ItemLengths[7] + pxbBans.m_ui16ItemLengths[8], 9) == false)
			{
				break;
			}
		}
	}

	if (m_pPermBanListS != NULL)
	{
		BanItem * pCur = NULL,
		          * m_pNext = m_pPermBanListS;

		while (m_pNext != NULL)
		{
			pCur = m_pNext;
			m_pNext = pCur->m_pNext;

			pxbBans.m_ui16ItemLengths[0] = 1;
			pxbBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) == true ? 0 : (void *)1);

			pxbBans.m_ui16ItemLengths[1] = pCur->m_sNick == NULL ? 0 : (uint16_t)strlen(pCur->m_sNick);
			pxbBans.m_pItemDatas[1] = pCur->m_sNick == NULL ? (void *)"" : (void *)pCur->m_sNick;

			pxbBans.m_ui16ItemLengths[2] = 1;
			pxbBans.m_pItemDatas[2] = (((pCur->m_ui8Bits & NICK) == NICK) == true ? (void *)1 : 0);

			pxbBans.m_ui16ItemLengths[3] = 16;
			pxbBans.m_pItemDatas[3] = (void *)pCur->m_ui128IpHash;

			pxbBans.m_ui16ItemLengths[4] = 1;
			pxbBans.m_pItemDatas[4] = (((pCur->m_ui8Bits & IP) == IP) == true ? (void *)1 : 0);

			pxbBans.m_ui16ItemLengths[5] = 1;
			pxbBans.m_pItemDatas[5] = (((pCur->m_ui8Bits & FULL) == FULL) == true ? (void *)1 : 0);

			pxbBans.m_ui16ItemLengths[6] = pCur->m_sReason == NULL ? 0 : (uint16_t)strlen(pCur->m_sReason);
			pxbBans.m_pItemDatas[6] = pCur->m_sReason == NULL ? (void *)"" : (void *)pCur->m_sReason;

			pxbBans.m_ui16ItemLengths[7] = pCur->m_sBy == NULL ? 0 : (uint16_t)strlen(pCur->m_sBy);
			pxbBans.m_pItemDatas[7] = pCur->m_sBy == NULL ? (void *)"" : (void *)pCur->m_sBy;

			pxbBans.m_ui16ItemLengths[8] = 8;
			ui64TempBanExpire = (uint64_t)pCur->m_tTempBanExpire;
			pxbBans.m_pItemDatas[8] = (void *)&ui64TempBanExpire;

			if (pxbBans.WriteNextItem(pxbBans.m_ui16ItemLengths[0] + pxbBans.m_ui16ItemLengths[1] + pxbBans.m_ui16ItemLengths[2] + pxbBans.m_ui16ItemLengths[3] + pxbBans.m_ui16ItemLengths[4] + pxbBans.m_ui16ItemLengths[5] + pxbBans.m_ui16ItemLengths[6] + pxbBans.m_ui16ItemLengths[7] + pxbBans.m_ui16ItemLengths[8], 9) == false)
			{
				break;
			}
		}
	}

	pxbBans.WriteRemaining();

	PXBReader pxbRangeBans;

	// Open range bans file
#ifdef _WIN32
	if (pxbRangeBans.OpenFileSave((ServerManager::m_sPath + "\\cfg\\RangeBans.pxb").c_str(), 7) == false)
	{
#else
	if (pxbRangeBans.OpenFileSave((ServerManager::m_sPath + "/cfg/RangeBans.pxb").c_str(), 7) == false)
	{
#endif
		return;
	}

	// Write file header
	pxbRangeBans.m_sItemIdentifiers[0] = 'F';
	pxbRangeBans.m_sItemIdentifiers[1] = 'I';
	pxbRangeBans.m_ui16ItemLengths[0] = (uint16_t)szPtokaXRangeBansLen;
	pxbRangeBans.m_pItemDatas[0] = (void *)sPtokaXRangeBans;
	pxbRangeBans.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

	pxbRangeBans.m_sItemIdentifiers[2] = 'F';
	pxbRangeBans.m_sItemIdentifiers[3] = 'V';
	pxbRangeBans.m_ui16ItemLengths[1] = 4;
	pxbRangeBans.m_pItemDatas[1] = (void *)&ui32Version;
	pxbRangeBans.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

	if (pxbRangeBans.WriteNextItem(szPtokaXRangeBansLen + 4, 2) == false)
	{
		return;
	}

	// "BT" "RF" "RT" "FB" "RE" "BY" "EX"
	memcpy(pxbRangeBans.m_sItemIdentifiers, sRangeBanIds, szRangeBanIdsLen);

	pxbRangeBans.m_ui8ItemValues[0] = PXBReader::PXB_BYTE;
	pxbRangeBans.m_ui8ItemValues[1] = PXBReader::PXB_STRING;
	pxbRangeBans.m_ui8ItemValues[2] = PXBReader::PXB_STRING;
	pxbRangeBans.m_ui8ItemValues[3] = PXBReader::PXB_BYTE;
	pxbRangeBans.m_ui8ItemValues[4] = PXBReader::PXB_STRING;
	pxbRangeBans.m_ui8ItemValues[5] = PXBReader::PXB_STRING;
	pxbRangeBans.m_ui8ItemValues[6] = PXBReader::PXB_EIGHT_BYTES;

	if (m_pRangeBanListS != NULL)
	{
		RangeBanItem * pCur = NULL,
		               * m_pNext = m_pRangeBanListS;

		while (m_pNext != NULL)
		{
			pCur = m_pNext;
			m_pNext = pCur->m_pNext;

			pxbRangeBans.m_ui16ItemLengths[0] = 1;
			pxbRangeBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) == true ? 0 : (void *)1);

			pxbRangeBans.m_ui16ItemLengths[1] = 16;
			pxbRangeBans.m_pItemDatas[1] = (void *)pCur->m_ui128FromIpHash;

			pxbRangeBans.m_ui16ItemLengths[2] = 16;
			pxbRangeBans.m_pItemDatas[2] = (void *)pCur->m_ui128ToIpHash;

			pxbRangeBans.m_ui16ItemLengths[3] = 1;
			pxbRangeBans.m_pItemDatas[3] = (((pCur->m_ui8Bits & FULL) == FULL) == true ? (void *)1 : 0);

			pxbRangeBans.m_ui16ItemLengths[4] = pCur->m_sReason == NULL ? 0 : (uint16_t)strlen(pCur->m_sReason);
			pxbRangeBans.m_pItemDatas[4] = pCur->m_sReason == NULL ? (void *)"" : (void *)pCur->m_sReason;

			pxbRangeBans.m_ui16ItemLengths[5] = pCur->m_sBy == NULL ? 0 : (uint16_t)strlen(pCur->m_sBy);
			pxbRangeBans.m_pItemDatas[5] = pCur->m_sBy == NULL ? (void *)"" : (void *)pCur->m_sBy;

			pxbRangeBans.m_ui16ItemLengths[6] = 8;
			ui64TempBanExpire = (uint64_t)pCur->m_tTempBanExpire;
			pxbRangeBans.m_pItemDatas[6] = (void *)&ui64TempBanExpire;

			if (pxbRangeBans.WriteNextItem(pxbRangeBans.m_ui16ItemLengths[0] + pxbRangeBans.m_ui16ItemLengths[1] + pxbRangeBans.m_ui16ItemLengths[2] + pxbRangeBans.m_ui16ItemLengths[3] + pxbRangeBans.m_ui16ItemLengths[4] + pxbRangeBans.m_ui16ItemLengths[5] + pxbRangeBans.m_ui16ItemLengths[6], 7) == false)
			{
				break;
			}
		}
	}

	pxbRangeBans.WriteRemaining();
}
//---------------------------------------------------------------------------

void BanManager::ClearTemp(void)
{
	BanItem * curBan = NULL,
	          * nextBan = m_pTempBanListS;

	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;

		Rem(curBan);
		delete curBan;
	}

	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearPerm(void)
{
	BanItem * curBan = NULL,
	          * nextBan = m_pPermBanListS;

	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;

		Rem(curBan);
		delete curBan;
	}

	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearRange(void)
{
	RangeBanItem * curBan = NULL,
	               * nextBan = m_pRangeBanListS;

	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;

		RemRange(curBan);
		delete curBan;
	}

	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearTempRange(void)
{
	RangeBanItem * curBan = NULL,
	               * nextBan = m_pRangeBanListS;

	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;

		if (((curBan->m_ui8Bits & TEMP) == TEMP) == true)
		{
			RemRange(curBan);
			delete curBan;
		}
	}

	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearPermRange(void)
{
	RangeBanItem * curBan = NULL,
	               * nextBan = m_pRangeBanListS;

	while (nextBan != NULL)
	{
		curBan = nextBan;
		nextBan = curBan->m_pNext;

		if (((curBan->m_ui8Bits & PERM) == PERM) == true)
		{
			RemRange(curBan);
			delete curBan;
		}
	}

	Save();
}
//---------------------------------------------------------------------------

void BanManager::Ban(User * pUser, const char * sReason, const char * sBy, const bool bFull)
{
	BanItem * pBan = new (std::nothrow) BanItem();
	if (pBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::Ban\n");
		return;
	}

	pBan->m_ui8Bits |= PERM;

	pBan->initIP( pUser);
	pBan->m_ui8Bits |= IP;

	if (bFull == true)
	{
		pBan->m_ui8Bits |= FULL;
	}

	time_t acc_time;
	time(&acc_time);

	// PPK ... check for <unknown> nick -> bad ban from script
	if (pUser->m_sNick[0] != '<')
	{
		pBan->m_sNick = (char *)malloc(pUser->m_ui8NickLen + 1);
		if (pBan->m_sNick == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for sNick in BanManager::Ban\n", pUser->m_ui8NickLen + 1);

			return;
		}

		memcpy(pBan->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
		pBan->m_sNick[pUser->m_ui8NickLen] = '\0';
		pBan->m_ui32NickHash = pUser->m_ui32NickHash;
		pBan->m_ui8Bits |= NICK;

		// PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick/ip multiple times :(
		BanItem * nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

		if (nxtBan != NULL)
		{
			if (((nxtBan->m_ui8Bits & PERM) == PERM) == true)
			{
				if (((nxtBan->m_ui8Bits & IP) == IP) == true)
				{
					if (memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0)
					{
						if (((pBan->m_ui8Bits & FULL) == FULL) == false)
						{
							// PPK ... same ban and new is not full, delete new
							delete pBan;

							return;
						}
						else
						{
							if (((nxtBan->m_ui8Bits & FULL) == FULL) == true)
							{
								// PPK ... same ban and both full, delete new
								delete pBan;

								return;
							}
							else
							{
								// PPK ... same ban but only new is full, delete old
								Rem(nxtBan);
								delete nxtBan;
							}
						}
					}
					else
					{
						pBan->m_ui8Bits &= ~NICK;
					}
				}
				else
				{
					// PPK ... old ban is only nickban, remove it
					Rem(nxtBan);
					delete nxtBan;
				}
			}
			else
			{
				if (((nxtBan->m_ui8Bits & IP) == IP) == true)
				{
					if (memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0)
					{
						if (((nxtBan->m_ui8Bits & FULL) == FULL) == false)
						{
							// PPK ... same ban and old is only temp, delete old
							Rem(nxtBan);
							delete nxtBan;
						}
						else
						{
							if (((pBan->m_ui8Bits & FULL) == FULL) == true)
							{
								// PPK ... same full ban and old is only temp, delete old
								Rem(nxtBan);
								delete nxtBan;
							}
							else
							{
								// PPK ... old ban is full, new not... set old ban to only ipban
								RemFromNickTable(nxtBan);
								nxtBan->m_ui8Bits &= ~NICK;
							}
						}
					}
					else
					{
						// PPK ... set old ban to ip ban only
						RemFromNickTable(nxtBan);
						nxtBan->m_ui8Bits &= ~NICK;
					}
				}
				else
				{
					// PPK ... old ban is only nickban, remove it
					Rem(nxtBan);
					delete nxtBan;
				}
			}
		}
	}

	// PPK ... clear bans with same ip without nickban and fullban if new ban is fullban
	BanItem * curBan = NULL,
	          * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);

	while (nxtBan != NULL)
	{
		curBan = nxtBan;
		nxtBan = curBan->m_pHashIpTableNext;

		if (((curBan->m_ui8Bits & NICK) == NICK) == true)
		{
			continue;
		}

		if (((curBan->m_ui8Bits & FULL) == FULL) == true && ((pBan->m_ui8Bits & FULL) == FULL) == false)
		{
			continue;
		}

		Rem(curBan);
		delete curBan;
	}

	if (sReason != NULL)
	{
		size_t szReasonLen = strlen(sReason);
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen + 1);
		if (pBan->m_sReason == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sReason in BanManager::Ban\n", szReasonLen > 511 ? 512 : szReasonLen+1);

			return;
		}

		if (szReasonLen > 511)
		{
			memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
			pBan->m_sReason[508] = '.';
			szReasonLen = 511;
		}
		else
		{
			memcpy(pBan->m_sReason, sReason, szReasonLen);
		}
		pBan->m_sReason[szReasonLen] = '\0';
	}

	if (AddBanInternal(sBy, pBan, __FUNCTION__) == false)
	{
		return;
	}
	if (Add(pBan) == false)
	{
		delete pBan;
		return;
	}

	Save();
}
//---------------------------------------------------------------------------
bool BanManager::AddBanInternal(const char * sBy, BanItemBase * pBan, const char* sFunction)
{
	if (sBy != NULL)
	{
		size_t szByLen = strlen(sBy);
		if (szByLen > 63)
		{
			szByLen = 63;
		}
		pBan->m_sBy = (char *)malloc(szByLen + 1);
		if (pBan->m_sBy == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::Ban\n", szByLen+1);

			return false;
		}
		memcpy(pBan->m_sBy, sBy, szByLen);
		pBan->m_sBy[szByLen] = '\0';
	}
	return true;
}

//---------------------------------------------------------------------------
static bool CreateReason(BanItemBase * pBan, const char * sReason, const char* pFunctionName)
{
	if (sReason != NULL)
	{
		size_t szReasonLen = strlen(sReason);
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen + 1);
		if (pBan->m_sReason == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in %s\n", szReasonLen > 511 ? 512 : szReasonLen + 1, pFunctionName);

			return true;
		}

		if (szReasonLen > 511)
		{
			memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
			pBan->m_sReason[508] = '.';
			szReasonLen = 511;
		}
		else
		{
			memcpy(pBan->m_sReason, sReason, szReasonLen);
		}
		pBan->m_sReason[szReasonLen] = '\0';
	}
	return false;
}
char BanManager::BanIp(User * pUser, const char * sIp, const char * sReason, const char * sBy, const bool bFull)
{
	BanItem * pBan = new (std::nothrow) BanItem();
	if (pBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::BanIp\n");
		return 1;
	}

	pBan->m_ui8Bits |= PERM;

	if (pUser)
	{
		pBan->initIP( pUser);
	}
	else
	{
		if (sIp != NULL && HashIP(sIp, pBan->m_ui128IpHash) == true)
		{
			pBan->initIP(sIp);
		}
		else
		{
			delete pBan;

			return 1;
		}
	}

	pBan->m_ui8Bits |= IP;

	if (bFull == true)
	{
		pBan->m_ui8Bits |= FULL;
	}

	time_t acc_time;
	time(&acc_time);

	BanItem * curBan = NULL,
	          * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);

	// PPK ... don't add ban if is already here perm (full) ban for same ip
	while (nxtBan != NULL)
	{
		curBan = nxtBan;
		nxtBan = curBan->m_pHashIpTableNext;

		if (((curBan->m_ui8Bits & TEMP) == TEMP) == true)
		{
			if (((curBan->m_ui8Bits & FULL) == FULL) == false || ((pBan->m_ui8Bits & FULL) == FULL) == true)
			{
				if (((curBan->m_ui8Bits & NICK) == NICK) == false)
				{
					Rem(curBan);
					delete curBan;
				}

				continue;
			}

			continue;
		}

		if (((curBan->m_ui8Bits & FULL) == FULL) == false && ((pBan->m_ui8Bits & FULL) == FULL) == true)
		{
			if (((curBan->m_ui8Bits & NICK) == NICK) == false)
			{
				Rem(curBan);
				delete curBan;
			}
			continue;
		}

		delete pBan;

		return 2;
	}

	if (CreateReason(pBan, sReason, __FUNCTION__))
		return 1;

	if (AddBanInternal(sBy, pBan, __FUNCTION__) == false)
	{
		return 1;
	}
	if (Add(pBan) == false)
	{
		delete pBan;
		return 1;
	}

	Save();

	return 0;
}
//---------------------------------------------------------------------------

bool BanManager::NickBan(User * pUser, const char * sNick, const char * sReason, const char * sBy)
{
	BanItem * pBan = new (std::nothrow) BanItem();
	if (pBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::NickBan\n");
		return false;
	}

	pBan->m_ui8Bits |= PERM;

	if (!pUser)
	{
		// PPK ... this should never happen, but to be sure ;)
		if (sNick == NULL)
		{
			delete pBan;

			return false;
		}

		// PPK ... bad script ban check
		if (sNick[0] == '<')
		{
			delete pBan;

			return false;
		}

		size_t szNickLen = strlen(sNick);
		pBan->m_sNick = (char *)malloc(szNickLen + 1);
		if (pBan->m_sNick == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::NickBan\n", szNickLen+1);

			return false;
		}

		memcpy(pBan->m_sNick, sNick, szNickLen);
		pBan->m_sNick[szNickLen] = '\0';
		pBan->m_ui32NickHash = HashNick(sNick, szNickLen);
	}
	else
	{
		// PPK ... bad script ban check
		if (pUser->m_sNick[0] == '<')
		{
			delete pBan;

			return false;
		}

		pBan->m_sNick = (char *)malloc(pUser->m_ui8NickLen + 1);
		if (pBan->m_sNick == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for sNick1 in BanManager::NickBan\n", pUser->m_ui8NickLen + 1);

			return false;
		}

		memcpy(pBan->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
		pBan->m_sNick[pUser->m_ui8NickLen] = '\0';
		pBan->m_ui32NickHash = pUser->m_ui32NickHash;

		pBan->initIP( pUser);
	}

	pBan->m_ui8Bits |= NICK;

	time_t acc_time;
	time(&acc_time);

	BanItem *nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

	// PPK ... not allow same nickbans !
	if (nxtBan != NULL)
	{
		if (((nxtBan->m_ui8Bits & PERM) == PERM) == true)
		{
			delete pBan;

			return false;
		}
		else
		{
			if (((nxtBan->m_ui8Bits & IP) == IP) == true)
			{
				// PPK ... set old ban to ip ban only
				RemFromNickTable(nxtBan);
				nxtBan->m_ui8Bits &= ~NICK;
			}
			else
			{
				// PPK ... old ban is only nickban, remove it
				Rem(nxtBan);
				delete nxtBan;
			}
		}
	}
	if (CreateReason(pBan, sReason, __FUNCTION__))
	{
		return false;
	}
	if (AddBanInternal(sBy, pBan, __FUNCTION__) == false)
	{
		return false;
	}
	if (Add(pBan) == false)
	{
		delete pBan;
		return false;
	}

	Save();

	return true;
}
//---------------------------------------------------------------------------

void BanManager::TempBan(User * pUser, const char * sReason, const char * sBy, const uint32_t minutes, const time_t &expiretime, const bool bFull)
{
	BanItem * pBan = new (std::nothrow) BanItem();
	if (pBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::TempBan\n");
		return;
	}

	pBan->m_ui8Bits |= TEMP;

	pBan->initIP( pUser);

	pBan->m_ui8Bits |= IP;

	if (bFull == true)
	{
		pBan->m_ui8Bits |= FULL;
	}

	time_t acc_time;
	time(&acc_time);

	if (expiretime > 0)
	{
		pBan->m_tTempBanExpire = expiretime;
	}
	else
	{
		if (minutes > 0)
		{
			pBan->m_tTempBanExpire = acc_time + (minutes * 60);
		}
		else
		{
			pBan->m_tTempBanExpire = acc_time + (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME] * 60);
		}
	}

	// PPK ... check for <unknown> nick -> bad ban from script
	if (pUser->m_sNick[0] != '<')
	{
		size_t szNickLen = strlen(pUser->m_sNick);
		pBan->m_sNick = (char *)malloc(szNickLen + 1);
		if (pBan->m_sNick == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::TempBan\n", szNickLen+1);

			return;
		}

		memcpy(pBan->m_sNick, pUser->m_sNick, szNickLen);
		pBan->m_sNick[szNickLen] = '\0';
		pBan->m_ui32NickHash = pUser->m_ui32NickHash;
		pBan->m_ui8Bits |= NICK;

		// PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick multiple times :(
		BanItem *nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

		if (nxtBan != NULL)
		{
			if (((nxtBan->m_ui8Bits & PERM) == PERM) == true)
			{
				if (((nxtBan->m_ui8Bits & IP) == IP) == true)
				{
					if (memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0)
					{
						if (((pBan->m_ui8Bits & FULL) == FULL) == false)
						{
							// PPK ... same ban and old is perm, delete new
							delete pBan;

							return;
						}
						else
						{
							if (((nxtBan->m_ui8Bits & FULL) == FULL) == true)
							{
								// PPK ... same ban and old is full perm, delete new
								delete pBan;

								return;
							}
							else
							{
								// PPK ... same ban and only new is full, set new to ipban only
								pBan->m_ui8Bits &= ~NICK;
							}
						}
					}
				}
				else
				{
					// PPK ... perm ban to same nick already exist, set new to ipban only
					pBan->m_ui8Bits &= ~NICK;
				}
			}
			else
			{
				if (nxtBan->m_tTempBanExpire < pBan->m_tTempBanExpire)
				{
					if (((nxtBan->m_ui8Bits & IP) == IP) == true)
					{
						if (memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0)
						{
							if (((nxtBan->m_ui8Bits & FULL) == FULL) == false)
							{
								// PPK ... same bans, but old with lower expiration -> delete old
								Rem(nxtBan);
								delete nxtBan;
							}
							else
							{
								if (((pBan->m_ui8Bits & FULL) == FULL) == false)
								{
									// PPK ... old ban with lower expiration is full ban, set old to ipban only
									RemFromNickTable(nxtBan);
									nxtBan->m_ui8Bits &= ~NICK;
								}
								else
								{
									// PPK ... same bans, old have lower expiration -> delete old
									Rem(nxtBan);
									delete nxtBan;
								}
							}
						}
						else
						{
							// PPK ... set old ban to ipban only
							RemFromNickTable(nxtBan);
							nxtBan->m_ui8Bits &= ~NICK;
						}
					}
					else
					{
						// PPK ... old ban is only nickban with lower bantime, remove it
						Rem(nxtBan);
						delete nxtBan;
					}
				}
				else
				{
					if (((nxtBan->m_ui8Bits & IP) == IP) == true)
					{
						if (memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0)
						{
							if (((pBan->m_ui8Bits & FULL) == FULL) == false)
							{
								// PPK ... same bans, but new with lower expiration -> delete new
								delete pBan;

								return;
							}
							else
							{
								if (((nxtBan->m_ui8Bits & FULL) == FULL) == false)
								{
									// PPK ... new ban with lower expiration is full ban, set new to ipban only
									pBan->m_ui8Bits &= ~NICK;
								}
								else
								{
									// PPK ... same bans, new have lower expiration -> delete new
									delete pBan;

									return;
								}
							}
						}
						else
						{
							// PPK ... set new ban to ipban only
							pBan->m_ui8Bits &= ~NICK;
						}
					}
					else
					{
						// PPK ... old ban is only nickban with higher bantime, set new to ipban only
						pBan->m_ui8Bits &= ~NICK;
					}
				}
			}
		}
	}

	// PPK ... clear bans with lower timeban and same ip without nickban and fullban if new ban is fullban
	BanItem * curBan = NULL,
	          * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);

	while (nxtBan != NULL)
	{
		curBan = nxtBan;
		nxtBan = curBan->m_pHashIpTableNext;

		if (((curBan->m_ui8Bits & PERM) == PERM) == true)
		{
			continue;
		}

		if (((curBan->m_ui8Bits & NICK) == NICK) == true)
		{
			continue;
		}

		if (((curBan->m_ui8Bits & FULL) == FULL) == true && ((pBan->m_ui8Bits & FULL) == FULL) == false)
		{
			continue;
		}

		if (curBan->m_tTempBanExpire > pBan->m_tTempBanExpire)
		{
			continue;
		}

		Rem(curBan);
		delete curBan;
	}
	if (CreateReason(pBan, sReason, __FUNCTION__))
	{
		return;
	}

	if (AddBanInternal(sBy, pBan, __FUNCTION__) == false)
	{
		return;
	}
	if (Add(pBan) == false)
	{
		delete pBan;
		return;
	}

	Save();
}
//---------------------------------------------------------------------------

char BanManager::TempBanIp(User * pUser, const char * sIp, const char * sReason, const char * sBy, const uint32_t minutes, const time_t &expiretime, const bool bFull)
{
	BanItem * pBan = new (std::nothrow) BanItem();
	if (pBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::TempBanIp\n");
		return 1;
	}

	pBan->m_ui8Bits |= TEMP;

	if (pUser != NULL)
	{
		pBan->initIP( pUser);
	}
	else
	{
		if (sIp != NULL && HashIP(sIp, pBan->m_ui128IpHash) == true)
		{
			pBan->initIP(sIp);
		}
		else
		{
			delete pBan;

			return 1;
		}
	}

	pBan->m_ui8Bits |= IP;

	if (bFull == true)
	{
		pBan->m_ui8Bits |= FULL;
	}

	time_t acc_time;
	time(&acc_time);

	if (expiretime > 0)
	{
		pBan->m_tTempBanExpire = expiretime;
	}
	else
	{
		if (minutes == 0)
		{
			pBan->m_tTempBanExpire = acc_time + (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME] * 60);
		}
		else
		{
			pBan->m_tTempBanExpire = acc_time + (minutes * 60);
		}
	}

	BanItem * curBan = NULL,
	          * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);

	// PPK ... don't add ban if is already here perm (full) ban or longer temp ban for same ip
	while (nxtBan != NULL)
	{
		curBan = nxtBan;
		nxtBan = curBan->m_pHashIpTableNext;

		if (((curBan->m_ui8Bits & TEMP) == TEMP) == true && curBan->m_tTempBanExpire < pBan->m_tTempBanExpire)
		{
			if (((curBan->m_ui8Bits & FULL) == FULL) == false || ((pBan->m_ui8Bits & FULL) == FULL) == true)
			{
				if (((curBan->m_ui8Bits & NICK) == NICK) == false)
				{
					Rem(curBan);
					delete curBan;
				}

				continue;
			}

			continue;
		}

		if (((curBan->m_ui8Bits & FULL) == FULL) == false && ((pBan->m_ui8Bits & FULL) == FULL) == true) continue;

		delete pBan;

		return 2;
	}
	if (CreateReason(pBan, sReason, __FUNCTION__))
	{
		return 1;
	}

	if (AddBanInternal(sBy, pBan, __FUNCTION__) == false)
	{
		return 1;
	}
	if (Add(pBan) == false)
	{
		delete pBan;
		return 1;
	}

	Save();

	return 0;
}
//---------------------------------------------------------------------------

bool BanManager::NickTempBan(User * pUser, const char * sNick, const char * sReason, const char * sBy, const uint32_t minutes, const time_t &expiretime)
{
	BanItem * pBan = new (std::nothrow) BanItem();
	if (pBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::NickTempBan\n");
		return false;
	}

	pBan->m_ui8Bits |= TEMP;

	if (!pUser)
	{
		// PPK ... this should never happen, but to be sure ;)
		if (sNick == NULL)
		{
			delete pBan;

			return false;
		}

		// PPK ... bad script ban check
		if (sNick[0] == '<')
		{
			delete pBan;

			return false;
		}

		size_t szNickLen = strlen(sNick);
		pBan->m_sNick = (char *)malloc(szNickLen + 1);
		if (pBan->m_sNick == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::NickTempBan\n", szNickLen+1);

			return false;
		}
		memcpy(pBan->m_sNick, sNick, szNickLen);
		pBan->m_sNick[szNickLen] = '\0';
		pBan->m_ui32NickHash = HashNick(sNick, szNickLen);
	}
	else
	{
		// PPK ... bad script ban check
		if (pUser->m_sNick[0] == '<')
		{
			delete pBan;

			return false;
		}

		pBan->m_sNick = (char *)malloc(pUser->m_ui8NickLen + 1);
		if (pBan->m_sNick == NULL)
		{
			delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for sNick1 in BanManager::NickTempBan\n", pUser->m_ui8NickLen + 1);

			return false;
		}
		memcpy(pBan->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
		pBan->m_sNick[pUser->m_ui8NickLen] = '\0';
		pBan->m_ui32NickHash = pUser->m_ui32NickHash;

		pBan->initIP( pUser);
	}

	pBan->m_ui8Bits |= NICK;

	time_t acc_time;
	time(&acc_time);

	if (expiretime > 0)
	{
		pBan->m_tTempBanExpire = expiretime;
	}
	else
	{
		if (minutes > 0)
		{
			pBan->m_tTempBanExpire = acc_time + (minutes * 60);
		}
		else
		{
			pBan->m_tTempBanExpire = acc_time + (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME] * 60);
		}
	}

	BanItem *nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

	// PPK ... not allow same nickbans !
	if (nxtBan != NULL)
	{
		if (((nxtBan->m_ui8Bits & PERM) == PERM) == true)
		{
			delete pBan;

			return false;
		}
		else
		{
			if (nxtBan->m_tTempBanExpire < pBan->m_tTempBanExpire)
			{
				if (((nxtBan->m_ui8Bits & IP) == IP) == true)
				{
					// PPK ... set old ban to ip ban only
					RemFromNickTable(nxtBan);
					nxtBan->m_ui8Bits &= ~NICK;
				}
				else
				{
					// PPK ... old ban is only nickban, remove it
					Rem(nxtBan);
					delete nxtBan;
				}
			}
			else
			{
				delete pBan;

				return false;
			}
		}
	}
	if (CreateReason(pBan, sReason, __FUNCTION__))
	{
		return false;
	}

	if (AddBanInternal(sBy, pBan, __FUNCTION__) == false)
	{
		return false;
	}
	if (Add(pBan) == false)
	{
		delete pBan;
		return false;
	}

	Save();

	return true;
}
//---------------------------------------------------------------------------

bool BanManager::Unban(const char * sWhat)
{
	uint32_t hash = HashNick(sWhat, strlen(sWhat));

	time_t acc_time;
	time(&acc_time);

	BanItem *Ban = FindNick(hash, acc_time, sWhat);

	if (Ban == NULL)
	{
		Hash128 ui128Hash;

		if (HashIP(sWhat, ui128Hash) == true && (Ban = FindIP(ui128Hash, acc_time)) != NULL)
		{
			Rem(Ban);
			delete Ban;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Rem(Ban);
		delete Ban;
	}

	Save();
	return true;
}
//---------------------------------------------------------------------------

bool BanManager::PermUnban(const char * sWhat)
{
	uint32_t hash = HashNick(sWhat, strlen(sWhat));

	BanItem *Ban = FindPermNick(hash, sWhat);

	if (Ban == NULL)
	{
		Hash128 ui128Hash;

		if (HashIP(sWhat, ui128Hash) == true && (Ban = FindPermIP(ui128Hash)) != NULL)
		{
			Rem(Ban);
			delete Ban;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Rem(Ban);
		delete Ban;
	}

	Save();
	return true;
}
//---------------------------------------------------------------------------

bool BanManager::TempUnban(const char * sWhat)
{
	uint32_t hash = HashNick(sWhat, strlen(sWhat));

	time_t acc_time;
	time(&acc_time);

	BanItem *Ban = FindTempNick(hash, acc_time, sWhat);

	if (Ban == NULL)
	{
		Hash128 ui128Hash;

		if (HashIP(sWhat, ui128Hash) == true && (Ban = FindTempIP(ui128Hash, acc_time)) != NULL)
		{
			Rem(Ban);
			delete Ban;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Rem(Ban);
		delete Ban;
	}

	Save();
	return true;
}
//---------------------------------------------------------------------------

void BanManager::RemoveAllIP(const uint8_t * m_ui128IpHash)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)m_ui128IpHash))
	{
		ui16IpTableIdx = m_ui128IpHash[14] * m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(m_ui128IpHash);
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				Rem(curBan);
				delete curBan;
			}

			return;
		}
	}
}
//---------------------------------------------------------------------------

void BanManager::RemovePermAllIP(const uint8_t * m_ui128IpHash)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)m_ui128IpHash))
	{
		ui16IpTableIdx = m_ui128IpHash[14] * m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(m_ui128IpHash);
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				if (((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true)
				{
					Rem(curBan);
					delete curBan;
				}
			}

			return;
		}
	}
}
//---------------------------------------------------------------------------

void BanManager::RemoveTempAllIP(const uint8_t * m_ui128IpHash)
{
	uint16_t ui16IpTableIdx = 0;

	if (IN6_IS_ADDR_V4MAPPED((const in6_addr *)m_ui128IpHash))
	{
		ui16IpTableIdx = m_ui128IpHash[14] * m_ui128IpHash[15];
	}
	else
	{
		ui16IpTableIdx = GetIpTableIdx(m_ui128IpHash);
	}

	IpTableItem * cur = NULL,
	              * next = m_pIpBanTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = nullptr;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->pFirstBan->m_ui128IpHash, m_ui128IpHash, 16) == 0)
		{
			nextBan = cur->pFirstBan;

			while (nextBan != NULL)
			{
				curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				if (((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true)
				{
					Rem(curBan);
					delete curBan;
				}
			}

			return;
		}
	}
}
//---------------------------------------------------------------------------

bool BanManager::RangeBan(const char * m_sIpFrom, const uint8_t * ui128FromIpHash, const char * m_sIpTo, const uint8_t * ui128ToIpHash, const char * sReason, const char * sBy, const bool bFull)
{
	RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
	if (pRangeBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in BanManager::RangeBan\n");
		return false;
	}

	pRangeBan->m_ui8Bits |= PERM;

	strcpy(pRangeBan->m_sIpFrom, m_sIpFrom);
	memcpy(pRangeBan->m_ui128FromIpHash, ui128FromIpHash, 16);

	strcpy(pRangeBan->m_sIpTo, m_sIpTo);
	memcpy(pRangeBan->m_ui128ToIpHash, ui128ToIpHash, 16);

	if (bFull == true)
	{
		pRangeBan->m_ui8Bits |= FULL;
	}

	RangeBanItem * curBan = NULL,
	               * nxtBan = m_pRangeBanListS;

	// PPK ... don't add range ban if is already here same perm (full) range ban
	while (nxtBan != NULL)
	{
		curBan = nxtBan;
		nxtBan = curBan->m_pNext;

		if (memcmp(curBan->m_ui128FromIpHash, pRangeBan->m_ui128FromIpHash, 16) != 0 ||
		        memcmp(curBan->m_ui128ToIpHash, pRangeBan->m_ui128ToIpHash, 16) != 0)
		{
			continue;
		}

		if ((curBan->m_ui8Bits & TEMP) == TEMP)
		{
			continue;
		}

		if (((curBan->m_ui8Bits & FULL) == FULL) == false && ((pRangeBan->m_ui8Bits & FULL) == FULL) == true)
		{
			RemRange(curBan);
			delete curBan;

			continue;
		}

		delete pRangeBan;

		return false;
	}
	if (CreateReason(pRangeBan, sReason, __FUNCTION__))
	{
		return false;
	}

	if (AddBanInternal(sBy, pRangeBan, __FUNCTION__) == false)
	{
		return false;
	}

	AddRange(pRangeBan);
	Save();

	return true;
}
//---------------------------------------------------------------------------

bool BanManager::RangeTempBan(const char * m_sIpFrom, const uint8_t * ui128FromIpHash, const char * m_sIpTo, const uint8_t * ui128ToIpHash, const char * sReason, const char * sBy, const uint32_t minutes,
                              const time_t &expiretime, const bool bFull)
{
	RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
	if (pRangeBan == NULL)
	{
		AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in BanManager::RangeTempBan\n");
		return false;
	}

	pRangeBan->m_ui8Bits |= TEMP;

	strcpy(pRangeBan->m_sIpFrom, m_sIpFrom);
	memcpy(pRangeBan->m_ui128FromIpHash, ui128FromIpHash, 16);

	strcpy(pRangeBan->m_sIpTo, m_sIpTo);
	memcpy(pRangeBan->m_ui128ToIpHash, ui128ToIpHash, 16);

	if (bFull == true)
	{
		pRangeBan->m_ui8Bits |= FULL;
	}

	time_t acc_time;
	time(&acc_time);

	if (expiretime > 0)
	{
		pRangeBan->m_tTempBanExpire = expiretime;
	}
	else
	{
		if (minutes > 0)
		{
			pRangeBan->m_tTempBanExpire = acc_time + (minutes * 60);
		}
		else
		{
			pRangeBan->m_tTempBanExpire = acc_time + (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME] * 60);
		}
	}

	RangeBanItem * curBan = NULL,
	               * nxtBan = m_pRangeBanListS;

	// PPK ... don't add range ban if is already here same perm (full) range ban or longer temp ban for same range
	while (nxtBan != NULL)
	{
		curBan = nxtBan;
		nxtBan = curBan->m_pNext;

		if (memcmp(curBan->m_ui128FromIpHash, pRangeBan->m_ui128FromIpHash, 16) != 0 || memcmp(curBan->m_ui128ToIpHash, pRangeBan->m_ui128ToIpHash, 16) != 0)
		{
			continue;
		}

		if (((curBan->m_ui8Bits & TEMP) == TEMP) == true && curBan->m_tTempBanExpire < pRangeBan->m_tTempBanExpire)
		{
			if (((curBan->m_ui8Bits & FULL) == FULL) == false || ((pRangeBan->m_ui8Bits & FULL) == FULL) == true)
			{
				RemRange(curBan);
				delete curBan;

				continue;
			}

			continue;
		}

		if (((curBan->m_ui8Bits & FULL) == FULL) == false && ((pRangeBan->m_ui8Bits & FULL) == FULL) == true)
		{
			continue;
		}

		delete pRangeBan;

		return false;
	}
	if (CreateReason(pRangeBan, sReason, __FUNCTION__))
	{
		return false;
	}

	if (AddBanInternal(sBy, pRangeBan, __FUNCTION__) == false)
	{
		return false;
	}

	AddRange(pRangeBan);
	Save();

	return true;
}
//---------------------------------------------------------------------------

bool BanManager::RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash)
{
	RangeBanItem * cur = NULL,
	               * next = m_pRangeBanListS;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if (memcmp(cur->m_ui128FromIpHash, ui128FromIpHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToIpHash, 16) == 0)
		{
			RemRange(cur);
			delete cur;

			return true;
		}
	}

	Save();
	return false;
}
//---------------------------------------------------------------------------

bool BanManager::RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash, unsigned char cType)
{
	RangeBanItem * cur = NULL,
	               * next = m_pRangeBanListS;

	while (next != NULL)
	{
		cur = next;
		next = cur->m_pNext;

		if ((cur->m_ui8Bits & cType) == cType && memcmp(cur->m_ui128FromIpHash, ui128FromIpHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToIpHash, 16) == 0)
		{
			RemRange(cur);
			delete cur;

			return true;
		}
	}

	Save();
	return false;
}
//---------------------------------------------------------------------------
