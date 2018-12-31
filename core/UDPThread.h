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
#ifndef UDPThreadH
#define UDPThreadH
//---------------------------------------------------------------------------

#ifdef FLYLINKDC_USE_UDP_THREAD
class UDPThread
{
private:
#ifdef _WIN32
	HANDLE hThreadHandle;
	
	SOCKET m_Sock;
	
	unsigned int m_ThreadId;
#else
	pthread_t m_ThreadId;
	
	int m_Sock;
#endif
	
	bool m_bTerminated;
	
	char rcvbuf[1024];
	
	DISALLOW_COPY_AND_ASSIGN(UDPThread);
public:
	static UDPThread * mPtrIPv4;
#ifdef FLYLINKDC_USE_UDP_THREAD_IP6
	static UDPThread * mPtrIPv6;
#endif
	
	UDPThread();
	~UDPThread();
	
	bool Listen(const int iAddressFamily);
	void Resume();
	void Run();
	void Close();
	void WaitFor();
	
	static UDPThread * Create(const int iAddressFamily);
	static void Destroy(UDPThread *& pUDPThread);
};
//---------------------------------------------------------------------------
#endif // FLYLINKDC_USE_UDP_THREAD

#endif
