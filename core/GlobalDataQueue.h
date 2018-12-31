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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef GlobalDataQueueH
#define GlobalDataQueueH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct User;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class CFlyBuffer
{
public:
	char * m_pBuffer;
	size_t m_szLen;
	size_t m_szSize;
	void clean()
	{
		if (m_szLen == 0 && m_szSize > 256)
		{
			free(m_pBuffer);
			m_pBuffer = (char *) malloc(256);
			m_szSize = 255;
		}
	}
	CFlyBuffer() : m_pBuffer(NULL), m_szLen(0), m_szSize(0) { }
	DISALLOW_COPY_AND_ASSIGN(CFlyBuffer);
};

class GlobalDataQueue
{
private:
	struct QueueItem
	{
		QueueItem * m_pNext;
		
		std::string m_pCommand[2];
		
		uint8_t m_ui8CommandType;
		
		QueueItem() : m_pNext(NULL), m_ui8CommandType(0) { }
		
		DISALLOW_COPY_AND_ASSIGN(QueueItem);
	};
	
	struct GlobalQueue : public CFlyBuffer
	{
		GlobalQueue * m_pNext;
		
		bool m_bCreated, m_bZlined;
		char * m_pZbuffer;
		
		uint32_t m_szZlen, m_szZsize;
		
		GlobalQueue() : m_pNext(NULL), m_bCreated(false), m_bZlined(false),m_pZbuffer(NULL),m_szZlen(0), m_szZsize(0) { }
		
		DISALLOW_COPY_AND_ASSIGN(GlobalQueue);
	};
	
	struct OpsQueue : public CFlyBuffer
	{
	};
	
	struct IPsQueue : public CFlyBuffer
	{
		bool m_bHaveDollars;
		
		IPsQueue() : m_bHaveDollars(false) { }
		
		DISALLOW_COPY_AND_ASSIGN(IPsQueue);
	};
	
	struct SingleDataItem
	{
		SingleDataItem * m_pPrev, * m_pNext;
		
		User * m_pFromUser;
		
		char * m_pData;
		
		size_t m_szDataLen;
		
		int32_t m_i32Profile;
		
		uint8_t m_ui8Type;
		
		SingleDataItem() : m_pPrev(NULL), m_pNext(NULL), m_pFromUser(NULL), m_pData(NULL), m_szDataLen(0), m_i32Profile(0), m_ui8Type(0) { }
		
		DISALLOW_COPY_AND_ASSIGN(SingleDataItem);
	};
	
	GlobalQueue m_GlobalQueues[144];
	
	OpsQueue m_OpListQueue;
	IPsQueue m_UserIPQueue;
	
	GlobalQueue * m_pCreatedGlobalQueues;
	
	QueueItem * m_pNewQueueItems[2], * m_pQueueItems;
	SingleDataItem * m_pNewSingleItems[2];
	
	DISALLOW_COPY_AND_ASSIGN(GlobalDataQueue);
	
	static void AddDataToQueue(GlobalQueue &pQueue, const char * sData, const size_t szLen);
	static void AddDataToQueue(GlobalQueue &pQueue, const std::string& sData);
public:
	static GlobalDataQueue * m_Ptr;
	
	SingleDataItem * m_pSingleItems;
	
	bool m_bHaveItems;
	
	enum
	{
		CMD_HUBNAME,
		CMD_CHAT,
		CMD_HELLO,
		CMD_MYINFO,
		CMD_QUIT,
		CMD_OPS,
		CMD_LUA,
		CMD_ACTIVE_SEARCH_V6,
		CMD_ACTIVE_SEARCH_V64,
		CMD_ACTIVE_SEARCH_V4,
		CMD_PASSIVE_SEARCH_V6,
		CMD_PASSIVE_SEARCH_V64,
		CMD_PASSIVE_SEARCH_V4,
		CMD_PASSIVE_SEARCH_V4_ONLY,
		CMD_PASSIVE_SEARCH_V6_ONLY,
#ifdef USE_FLYLINKDC_EXT_JSON
		CMD_EXTJSON
#endif
	};
	
	enum
	{
		BIT_LONG_MYINFO                     = 0x1,
		BIT_ALL_SEARCHES_IPV64              = 0x2,
		BIT_ALL_SEARCHES_IPV6               = 0x4,
		BIT_ALL_SEARCHES_IPV4               = 0x8,
		BIT_ACTIVE_SEARCHES_IPV64           = 0x10,
		BIT_ACTIVE_SEARCHES_IPV6            = 0x20,
		BIT_ACTIVE_SEARCHES_IPV4            = 0x40,
		BIT_HELLO                           = 0x80,
		BIT_OPERATOR                        = 0x100,
		BIT_USERIP                          = 0x200,
		BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4   = 0x400,
		BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4   = 0x800,
	};
	
	enum
	{
		SI_PM2ALL,
		SI_PM2OPS,
		SI_OPCHAT,
		SI_TOPROFILE,
		SI_PM2PROFILE,
	};
	
	GlobalDataQueue();
	~GlobalDataQueue();
	
	void AddQueueItem(const char * sCommand1, const size_t szLen1, const char * sCommand2, const size_t szLen2, const uint8_t ui8CmdType);
	void OpListStore(const char * sNick);
	void UserIPStore(User * pUser);
	void PrepareQueueItems();
	void ClearQueues();
	void ProcessQueues(User * u);
	void AddSearchDataToQueue(const User * pUser, uint32_t ui32QueueType, const QueueItem * pCur); // FlylinkDC++
	void ProcessSingleItems(User * u) const;
	void SingleItemStore(const char * sData, const size_t szDataLen, User * pFromUser, const int32_t i32Profile, const uint8_t ui8Type);
	void SendFinalQueue();
	void * GetLastQueueItem();
	void * GetFirstQueueItem();
	void * InsertBlankQueueItem(void * pAfterItem, const uint8_t ui8CmdType);
	static void FillBlankQueueItem(const char * sCommand, const size_t szLen, void * pQueueItem);
	void StatusMessageFormat(const char * sFrom, const char * sFormatMsg, ...);
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif

