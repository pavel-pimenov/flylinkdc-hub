/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef GlobalDataQueueH
#define GlobalDataQueueH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <string>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>

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
	private:
	        // The data that we're exposing to Prometheus:
#ifndef NDEBUG
            std::shared_ptr<prometheus::Exposer> m_exposer = std::make_shared<prometheus::Exposer>("0.0.0.0:9999");
#else
            std::shared_ptr<prometheus::Exposer> m_exposer = std::make_shared<prometheus::Exposer>("0.0.0.0:8888");
#endif
            std::shared_ptr<prometheus::Registry> m_registry = std::make_shared<prometheus::Registry>();

	public:
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_dc_commands_counter = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_dc_commands_counter")
                    .Help("The number of dc++ commands")
                   // .Labels({{"node", "node_name"}})
                    .Register(*m_registry);
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_lua_commands_counter = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_lua_commands_counter")
                    .Help("The number of lua commands")
                    .Register(*m_registry);
			void PrometheusLuaInc(const char* p_command)
			{
				m_flylinkdc_hub_lua_commands_counter.Add({{"func", p_command}}).Increment();
			}		
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_lua_user_value_counter = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_lua_user_value_counter")
                    .Help("The number of lua user_value")
                    .Register(*m_registry);
			void PrometheusLuaUserValueInc(const uint8_t p_id)
			{
				m_flylinkdc_hub_lua_commands_counter.Add({{"value", std::to_string(p_id).c_str()}}).Increment();
			}		

            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_lua_user_data_counter = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_lua_user_data_counter")
                    .Help("The number of lua user_data")
                    .Register(*m_registry);
			void PrometheusLuaUserDataInc(const uint8_t p_id)
			{
				m_flylinkdc_hub_lua_commands_counter.Add({{"value", std::to_string(p_id).c_str()}}).Increment();
			}		

            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_recv_bytes = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_recv_bytes")
                    .Register(*m_registry);
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_recv_counter = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_recv_bytes")
                    .Register(*m_registry);
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_send_bytes = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_send_bytes")
                    .Register(*m_registry);
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_send_counter = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_send_bytes")
                    .Register(*m_registry);
            prometheus::Family<prometheus::Gauge>& m_flylinkdc_hub_users = prometheus::BuildGauge()
                    .Name("flylinkdc_hub_users")
                    .Register(*m_registry);
            prometheus::Family<prometheus::Gauge>& m_flylinkdc_hub_messages = prometheus::BuildGauge()
                    .Name("flylinkdc_hub_messages")
                    .Register(*m_registry);
            prometheus::Family<prometheus::Gauge>& m_flylinkdc_hub_rusage = prometheus::BuildGauge()
                    .Name("flylinkdc_hub_rusage")
                    .Register(*m_registry);
				
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_compress_bytes = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_compress_bytes")
                    .Register(*m_registry);
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_log_bytes = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_log_bytes")
                    .Register(*m_registry);

			void PrometheusRusageValue(const char* p_type, long p_val)
			{
				m_flylinkdc_hub_rusage.Add({{"rusage", p_type}}).Set(p_val);
			}		
			void PrometheusLogBytes(const char* p_type, int p_len)
			{
				m_flylinkdc_hub_log_bytes.Add({{"bytes", p_type}}).Increment(p_len);
			}		
			void PrometheusZlibBytes(const char* p_type, int p_len)
			{
				m_flylinkdc_hub_compress_bytes.Add({{"bytes", p_type}}).Increment(p_len);
			}		
			void PrometheusRecvBytes(const char* p_type, int p_len)
			{
				m_flylinkdc_hub_recv_bytes.Add({{p_type, "size"}}).Increment(p_len);
				m_flylinkdc_hub_recv_counter.Add({{p_type, "count"}}).Increment();
			}		
            prometheus::Family<prometheus::Counter>& m_flylinkdc_hub_send_user_counter = prometheus::BuildCounter()
                    .Name("flylinkdc_hub_send_user_bytes")
                    .Register(*m_registry);
			void PrometheusSendBytes(const char* p_type, int p_len)
			{
				m_flylinkdc_hub_send_bytes.Add({{p_type, "size"}}).Increment(p_len);
				m_flylinkdc_hub_send_counter.Add({{p_type, "count"}}).Increment();
			}		
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif

