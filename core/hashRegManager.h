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
#ifndef hashRegManagerH
#define hashRegManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------
#include <string>

struct RegUser
{
	time_t m_tLastBadPass;
	
    std::string m_sNick;
	
	union
	{
		char * m_sPass;
		uint8_t * m_ui8PassHash;
	};
	
	RegUser * m_pPrev, * m_pNext;
	RegUser * m_pHashTablePrev, * m_pHashTableNext;
	
	uint32_t m_ui32Hash;
	
	uint16_t m_ui16Profile;
	
	uint8_t m_ui8BadPassCount;
	
	bool m_bPassHash;
	
	RegUser();
	~RegUser();
	
	static RegUser * CreateReg(const char * sRegNick, const size_t szRegNickLen, const char * sRegPassword, const size_t szRegPassLen, const uint8_t * ui8RegPassHash, const uint16_t ui16RegProfile);
	bool UpdatePassword(const char * sNewPass, const size_t szNewLen);
	DISALLOW_COPY_AND_ASSIGN(RegUser);
};
//---------------------------------------------------------------------------

class RegManager
{
	private:
		RegUser * m_pTable[65536];
		
		uint8_t m_ui8SaveCalls;
		
		DISALLOW_COPY_AND_ASSIGN(RegManager);
		
		void LoadXML();
	public:
		static RegManager * m_Ptr;
		
		RegUser * m_pRegListS, * m_pRegListE;
		
		RegManager(void);
		~RegManager(void);
		
		bool AddNew(const char * sNick, const char * sPasswd, const uint16_t iProfile);
		
		void Add(RegUser * pReg);
		void Add2Table(RegUser * pReg);
		static void ChangeReg(RegUser * pReg, const char * sNewPasswd, const uint16_t ui16NewProfile);
		void Delete(RegUser * pReg, const bool bFromGui = false);
		void Rem(RegUser * pReg);
		void RemFromTable(RegUser * pReg);
		
		RegUser * Find(const char * sNick, const size_t szNickLen);
		RegUser * Find(User * pUser);
		RegUser * Find(const uint32_t ui32Hash, const char * sNick);
		
		void Load(void);
		void Save(const bool bSaveOnChange = false, const bool bSaveOnTime = false);
		
		void HashPasswords() const;
		
		void AddRegCmdLine();
};
//---------------------------------------------------------------------------

#endif
