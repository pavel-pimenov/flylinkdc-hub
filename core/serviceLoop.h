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
#ifndef serviceLoopH
#define serviceLoopH

#include <vector>
#include "CriticalSection.h"

//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class ServiceLoop
{
	private:
		uint64_t m_ui64LstUptmTck;
		CriticalSection m_csAcceptQueue;
		
#ifdef _WIN32
#ifdef _WIN_IOT
		uint64_t m_ui64LastSecond;
#endif
#else
		uint64_t m_ui64LastSecond;
#endif
		
		struct AcceptedSocket
		{
			sockaddr_storage m_Addr;
			
			AcceptedSocket * m_pNext;
			
#ifdef _WIN32
			SOCKET m_Socket;
#else
			int m_Socket;
#endif
			
			AcceptedSocket();
			
			DISALLOW_COPY_AND_ASSIGN(AcceptedSocket);
		};
		
		AcceptedSocket * m_pAcceptedSocketsS, * m_pAcceptedSocketsE;
		
		DISALLOW_COPY_AND_ASSIGN(ServiceLoop);
	
		
		static void AcceptUser(AcceptedSocket * pAccptSocket);
	protected:
	public:
		double m_dLoggedUsers, m_dActualSrvLoopLogins;
		
#if defined(_WIN32) && !defined(_WIN_IOT)
		static HANDLE m_hLoopEvents[2];
		HANDLE m_hThreadHandle;
#else
#ifdef FLYLINKDC_REMOVE_REGISTER_THREAD
		uint64_t m_ui64LastRegToHublist;
#endif
#endif
		
		static ServiceLoop * m_Ptr;

#if defined(_WIN32) && !defined(_WIN_IOT)
		static DWORD m_dwMainThreadId;
#endif
		
		uint32_t m_ui32LastSendRest,  m_ui32SendRestsPeak,  m_ui32LastRecvRest,  m_ui32RecvRestsPeak,  m_ui32LoopsForLogins;
		
		bool m_bRecv;
		
		ServiceLoop();
		~ServiceLoop();
		
#ifdef _WIN32
		void AcceptSocket(SOCKET &s, const sockaddr_storage &addr);
#else
		void AcceptSocket(int& s, const sockaddr_storage &addr);
#endif
		void ReceiveLoop();
		void SendLoop();
		void Looper();
	private:
		struct CFlyIPRange
		{
			uint32_t m_start_ip;
			uint32_t m_stop_ip;
			CFlyIPRange(uint32_t p_start_ip, uint32_t p_stop_ip) : m_start_ip(p_start_ip), m_stop_ip(p_stop_ip)
			{
			}
		};
		typedef std::vector<CFlyIPRange> CFlyP2PGuardArray;
		static CFlyP2PGuardArray g_ProxyArray;
		static bool g_ProxyGuardLoad;
		static void loadProxyGuard();
		static bool isProxy(const in_addr& p_ip4);
		
};
//---------------------------------------------------------------------------

#endif
