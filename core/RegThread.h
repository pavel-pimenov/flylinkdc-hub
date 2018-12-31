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
#ifndef RegThreadH
#define RegThreadH
//---------------------------------------------------------------------------

#ifdef FLYLINKDC_REMOVE_REGISTER_THREAD
class RegisterThread
{
	private:
		struct RegSocket
		{
			uint64_t m_ui64TotalShare;
			
			RegSocket * m_pPrev, * m_pNext;
			
			char * m_sAddress, * m_pRecvBuf, * m_pSendBuf, * m_pSendBufHead;
			
#ifdef _WIN32
			SOCKET m_Sock;
#else
			int m_Sock;
#endif
			
			uint32_t m_ui32RecvBufLen, m_ui32RecvBufSize, m_ui32SendBufLen, m_ui32TotalUsers;
			
			uint32_t m_ui32AddrLen;
			
			RegSocket();
			~RegSocket();
			
			DISALLOW_COPY_AND_ASSIGN(RegSocket);
		};
		
		RegSocket * m_pRegSockListS, * m_pRegSockListE;
		
#ifdef _WIN32
		HANDLE m_hThreadHandle;
		
		unsigned int m_ThreadId;
#else
		pthread_t m_ThreadId;
#endif
		
		bool m_bTerminated;
		
		char m_sMsg[2048];
		
		DISALLOW_COPY_AND_ASSIGN(RegisterThread);
		
		void AddSock(const char * sAddress, const size_t szLen);
		bool Receive(RegSocket * pSock);
		static void Add2SendBuf(RegSocket * pSock, const char * sData);
		bool Send(RegSocket * pSock);
		void RemoveSock(RegSocket * pSock);
	public:
		static RegisterThread * m_Ptr;
		
		uint32_t m_ui32BytesRead, m_ui32BytesSent;
		
		RegisterThread();
		~RegisterThread();
		
		void Setup(const char * sListAddresses, const uint16_t ui16AddrsLen);
		void Resume();
		void Run();
		void Close();
		void WaitFor();
};
//---------------------------------------------------------------------------
#endif // #ifdef FLYLINKDC_REMOVE_REGISTER_THREAD
#endif

