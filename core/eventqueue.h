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
#ifndef eventqueueH
#define eventqueueH
//---------------------------------------------------------------------------
#include "CriticalSection.h"
class EventQueue
{
private:

	struct Event
	{
		Event * m_pPrev, * m_pNext;
		
		std::string m_sMsg;
		
		uint8_t m_ui128IpHash[16];
		uint8_t m_ui8Id;
		
		explicit Event(const char* p_message);
		
		DISALLOW_COPY_AND_ASSIGN(Event);
	};
	
	Event * m_pNormalE, * m_pThreadE;
	
	CriticalSection m_csEventQueue;
	DISALLOW_COPY_AND_ASSIGN(EventQueue);
public:
	static EventQueue * m_Ptr;
	
	Event * m_pNormalS, * m_pThreadS;
	
	enum
	{
		EVENT_RESTART,
		EVENT_RSTSCRIPTS,
		EVENT_RSTSCRIPT,
		EVENT_STOPSCRIPT,
		EVENT_STOP_SCRIPTING,
		EVENT_SHUTDOWN,
		EVENT_REGSOCK_MSG,
		EVENT_SRVTHREAD_MSG,
		EVENT_UDP_SR
	};
	
	EventQueue();
	~EventQueue();
	
	void AddNormal(const uint8_t ui8Id, const char * sMsg);
	void AddThread(const uint8_t ui8Id, const char * sMsg, const sockaddr_storage * sas = NULL);
	void ProcessEvents();
};
//---------------------------------------------------------------------------

#endif

