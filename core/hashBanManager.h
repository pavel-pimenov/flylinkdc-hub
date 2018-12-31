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
#ifndef hashBanManagerH
#define hashBanManagerH

#include "utility.h"
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct BanItemBase
{
	time_t m_tTempBanExpire;
	char * m_sReason;
	char * m_sBy;
	uint8_t m_ui8Bits;
	
	BanItemBase() : m_tTempBanExpire(0), m_sReason(NULL), m_sBy(NULL), m_ui8Bits(0)
	{
	}
	virtual ~BanItemBase();
	DISALLOW_COPY_AND_ASSIGN(BanItemBase);
};
struct BanItem : public BanItemBase
{
	char * m_sNick;
	
	BanItem * m_pPrev, * m_pNext;
	BanItem * m_pHashNickTablePrev, * m_pHashNickTableNext;
	BanItem * m_pHashIpTablePrev, * m_pHashIpTableNext;
	
	uint32_t m_ui32NickHash;
	
	uint8_t m_ui128IpHash[16];
	
	char m_sIp[40];
	
	void initIP(const User* u);
	void initIP(const char* pIP);
	BanItem();
	~BanItem();
	
	DISALLOW_COPY_AND_ASSIGN(BanItem);
};
//---------------------------------------------------------------------------

struct RangeBanItem : public BanItemBase
{

	RangeBanItem * m_pPrev, * m_pNext;
	
	Hash128 m_ui128FromIpHash, m_ui128ToIpHash;
	
	char m_sIpFrom[40], m_sIpTo[40] ;
	
	RangeBanItem();
	~RangeBanItem();
	
	DISALLOW_COPY_AND_ASSIGN(RangeBanItem);
};
//---------------------------------------------------------------------------

class BanManager
{
	private:
#define IP_BAN_HASH_TABLE_SIZE 65536
		BanItem * m_pNickBanTable[IP_BAN_HASH_TABLE_SIZE];
		
		struct IpTableItem
		{
			IpTableItem * m_pPrev, * m_pNext;
			
			BanItem * pFirstBan;
			
			IpTableItem() : m_pPrev(NULL), m_pNext(NULL), pFirstBan(NULL) { }
			
			DISALLOW_COPY_AND_ASSIGN(IpTableItem);
		};
		
		IpTableItem * m_pIpBanTable[IP_BAN_HASH_TABLE_SIZE];
		
		uint32_t m_ui32SaveCalled;
		
		DISALLOW_COPY_AND_ASSIGN(BanManager);
	public:
		static BanManager * m_Ptr;
		
		BanItem * m_pTempBanListS, * m_pTempBanListE;
		BanItem * m_pPermBanListS, * m_pPermBanListE;
		RangeBanItem * m_pRangeBanListS, * m_pRangeBanListE;
		
		enum BanBits
		{
			PERM        = 0x1,
			TEMP        = 0x2,
			FULL        = 0x4,
			IP          = 0x8,
			NICK        = 0x10
		};
		
		BanManager(void);
		~BanManager(void);
		
		
		bool Add(BanItem * pBan);
		bool Add2Table(BanItem * pBan);
		void Add2NickTable(BanItem * pBan);
		bool Add2IpTable(BanItem * pBan);
		void Rem(BanItem * pBan, const bool bFromGui = false);
		void RemFromTable(BanItem * pBan);
		void RemFromNickTable(BanItem * pBan);
		void RemFromIpTable(BanItem * pBan);
		
		BanItem* Find(BanItem * pBan); // from gui
		void Remove(BanItem * pBan); // from gui
		
		void AddRange(RangeBanItem * pRangeBan);
		void RemRange(RangeBanItem * pRangeBan, const bool bFromGui = false);
		
		RangeBanItem* FindRange(RangeBanItem * pRangeBan); // from gui
		void RemoveRange(RangeBanItem * pRangeBan); // from gui
		
		BanItem* FindNick(User * pUser);
		BanItem* FindIP(User * pUser);
		RangeBanItem* FindRange(User * pUser);
		
		BanItem* FindFull(const uint8_t * ui128IpHash);
		BanItem* FindFull(const uint8_t * ui128IpHash, const time_t &tAccTime);
		RangeBanItem* FindFullRange(const uint8_t * ui128IpHash, const time_t &tAccTime);
		
		BanItem* FindNick(const char * sNick, const size_t szNickLen);
		BanItem* FindNick(const uint32_t ui32Hash, const time_t &tAccTime, const char * sNick);
		BanItem* FindIP(const uint8_t * ui128IpHash, const time_t &tAccTime);
		RangeBanItem* FindRange(const uint8_t * ui128IpHash, const time_t &tAccTime);
		RangeBanItem* FindRange(const uint8_t * ui128FromHash, const uint8_t * ui128ToHash, const time_t &tAccTime);
		
		BanItem* FindTempNick(const char * sNick, const size_t szNickLen);
		BanItem* FindTempNick(const uint32_t ui32Hash, const time_t &tAccTime, const char * sNick);
		BanItem* FindTempIP(const uint8_t * ui128IpHash, const time_t &tAccTime);
		
		BanItem* FindPermNick(const char * sNick, const size_t szNickLen);
		BanItem* FindPermNick(const uint32_t ui32Hash, const char * sNick);
		BanItem* FindPermIP(const uint8_t * ui128IpHash);
		
		void Load();
		void LoadXML();
		void Save(const bool bForce = false);
		
		void ClearTemp(void);
		void ClearPerm(void);
		void ClearRange(void);
		void ClearTempRange(void);
		void ClearPermRange(void);
		
		bool AddBanInternal(const char * sBy, BanItemBase * pBan, const char* sFunction);
		void Ban(User * pUser, const char * sReason, const char * sBy, const bool bFull);
		char BanIp(User * pUser, const char * sIp, const char * sReason, const char * sBy, const bool bFull);
		bool NickBan(User * pUser, const char * sNick, const char * sReason, const char * sBy);
		
		void TempBan(User * pUser, const char * sReason, const char * sBy, const uint32_t minutes, const time_t &expiretime, const bool bFull);
		char TempBanIp(User * pUser, const char * sIp, const char * sReason, const char * sBy, const uint32_t minutes, const time_t &expiretime, const bool bFull);
		bool NickTempBan(User * pUser, const char * sNick, const char * sReason, const char * sBy, const uint32_t minutes, const time_t &expiretime);
		
		bool Unban(const char * sWhat);
		bool PermUnban(const char * sWhat);
		bool TempUnban(const char * sWhat);
		
		void RemoveAllIP(const uint8_t * ui128IpHash);
		void RemovePermAllIP(const uint8_t * ui128IpHash);
		void RemoveTempAllIP(const uint8_t * ui128IpHash);
		
		bool RangeBan(const char * sIpFrom, const uint8_t * ui128FromIpHash, const char * sIpTo, const uint8_t * ui128ToIpHash, const char * sReason, const char * sBy, const bool bFull);
		bool RangeTempBan(const char * sIpFrom, const uint8_t * ui128FromIpHash, const char * sIpTo, const uint8_t * ui128ToIpHash, const char * sReason, const char * sBy, const uint32_t ui32Minutes,
		                  const time_t &expiretime, const bool bFull);
		                  
		bool RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash);
		bool RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash, const uint8_t ui8Type);
};
//---------------------------------------------------------------------------

#endif
