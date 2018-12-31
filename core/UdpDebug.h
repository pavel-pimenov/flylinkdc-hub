/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
#ifndef UdpDebugH
#define UdpDebugH
//---------------------------------------------------------------------------
#include <string>

struct User;
//---------------------------------------------------------------------------
extern bool g_isUseSyslog;

class UdpDebug
{
	private:
		char * sDebugBuffer, * sDebugHead;
		
		struct UdpDbgItem
		{
			sockaddr_storage sas_to;
			
			UdpDbgItem * m_pPrev, * m_pNext;
			
			std::string m_sNick;
#ifdef _WIN32
			SOCKET s;
#else
			int s;
#endif
			
			int sas_len;
			
			uint32_t ui32Hash;
			
			bool bIsScript, bAllData;
			
			UdpDbgItem();
			~UdpDbgItem();
			DISALLOW_COPY_AND_ASSIGN(UdpDbgItem);
		};
		DISALLOW_COPY_AND_ASSIGN(UdpDebug);
		
		void CreateBuffer();
		void DeleteBuffer();
	public:
		static UdpDebug * m_Ptr;
		
		UdpDbgItem * pDbgItemList;
		
		UdpDebug();
		~UdpDebug();
		
		void Broadcast(const std::string& p_msg) const
		{
			if (!p_msg.empty())
			{
				Broadcast(p_msg.c_str(), p_msg.size());
			}
		}
		void Broadcast(const char * sMsg, const size_t szMsgLen) const;
		void BroadcastFormat(const char * sFormatMsg, ...) const;
		bool New(User * pUser, const uint16_t ui16Port);
		bool New(const char * sIP, const uint16_t ui16Port, const bool bAllData, const char * sScriptName);
		bool Remove(User * pUser);
		void Remove(const char * sScriptName);
		bool CheckUdpSub(User * pUser, const bool bSendMsg = false) const;
		void Send(const char * sScriptName, const char * sMessage, const size_t szMsgLen) const;
		void Cleanup();
		void UpdateHubName();
};
//---------------------------------------------------------------------------

#endif
