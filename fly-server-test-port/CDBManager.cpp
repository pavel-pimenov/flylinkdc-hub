//-----------------------------------------------------------------------------
//(c) 2007-2024 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------

#include <stdio.h>
#include "CDBManager.h"

#ifdef _WIN32
#define snprintf _snprintf
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#define closesocket(x) close(x)

#endif
using std::string;
//==========================================================================
bool g_setup_log_disable_test_port = true;
bool g_setup_syslog_disable        = true;
//==========================================================================
CFlyLogThreadInfoArray* CFlyServerContext::g_log_array = NULL;
//==========================================================================
sqlite_int64 get_tick_count()
{
#ifdef _WIN32 // Only in windows
	LARGE_INTEGER l_counter;
	QueryPerformanceCounter(&l_counter);
	return l_counter.QuadPart;
	//return GetTickCount64();
#else // Linux
	struct timeval tim;
	gettimeofday(&tim, NULL);
	unsigned int t = ((tim.tv_sec * 1000) + (tim.tv_usec / 1000)) & 0xffffffff;
	return t;
#endif // _WIN32
}
//==========================================================================
// ������ - 172.23.17.18:30002172.23.17.18: CID = $FLY-TEST-PORT MSL2NL7QB24PKECJEJFPGWY7S3TGTPMXPWDTWPA172.23.17.18:30002
static void set_socket_opt(SOCKET p_sock, int p_option, int p_val)
{
	int len = sizeof(p_val); // x64 - x86 int ������ ������
	if(setsockopt(p_sock, SOL_SOCKET, p_option, (char*)&p_val, len) < 0)
  {
      std::cout << "set_socket_opt option = "<< p_option << " val = " << p_val << " failed!\n";
  }
}
//==========================================================================
static void send_udp_tcp_test_port(const std::string& p_PID, const std::string& p_CID, const std::string& p_ip, const string& p_port, bool p_is_tcp)
{
#ifdef FLYLINKDC_USE_TEST_PORT_PROMETHEUS	
    g_DB.flyserver_test_port_counter(p_is_tcp ? "tcp": "udp");
#endif

#ifdef _DEBUG
#ifdef _WIN32
	if (p_is_tcp)
	{
		std::cout << "TCP test_port - ip = " << p_ip << ":" << p_port << " CID = " << p_CID << " PID = " << p_PID << std::endl;
	}
	else
	{
		std::cout << "UDP test_port - ip = " << p_ip << ":" << p_port << " CID = " << p_CID << " PID = " << p_PID <<  std::endl;
	}
#endif
#endif
	const unsigned short l_port = atoi(p_port.c_str());
	const string l_header = "$FLY-TEST-PORT " + p_CID + p_ip + ':' + p_port + "|";
	struct sockaddr_in addr = {0};
	int l_result = 0;
	SOCKET sock = socket(AF_INET, p_is_tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "send_udp_tcp_test_port - socket error! error code = " << errno << " CID = " << p_CID <<  std::endl;
		return;
	}
  {
    struct timeval timeout;      
    timeout.tv_sec  = 5;
    timeout.tv_usec = 0;

    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        std::cout << "setsockopt SO_RCVTIMEO failed\n";

    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        std::cout << "setsockopt SO_SNDTIMEO failed\n";
  }
	addr.sin_family = AF_INET;
	addr.sin_port = htons(l_port);
	addr.sin_addr.s_addr = inet_addr(p_ip.c_str());
	if (p_is_tcp)
	{
		int sz = sizeof(addr);
		l_result = connect(sock, (struct sockaddr*) &addr, sz);
	}
	if (l_result == SOCKET_ERROR)
	{
		//std::cout << "connect error! error code = " << errno;
		l_result = closesocket(sock);
		if (l_result == SOCKET_ERROR)
		{
			std::cout << "send_udp_tcp_test_port - closesocket error! error code = " << errno << " CID = " << p_CID << std::endl;
		}
		return;
	}
  // TODO - add timeout for TCP
  // http://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections
	l_result = sendto(sock, l_header.c_str(), l_header.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
	if (l_result == SOCKET_ERROR)
	{
		std::cout << "send_udp_tcp_test_port - sendto error! error code = " << errno << " CID = " << p_CID << std::endl;
	}
	l_result = closesocket(sock);
	if (l_result == SOCKET_ERROR)
	{
		std::cout << "send_udp_tcp_test_port - closesocket error! error code = " << errno << " CID = " << p_CID << std::endl;
	}
}
//==========================================================================
static void* thread_proc_udp_tcp_test_port(void* p_param)
{
	static volatile LONG g_count_thread = 0;
	static volatile unsigned g_count_all = 0;
	CFlySafeGuard l_call_deep(g_count_thread); // TODO - ����� ������� ����� ������� ����� ��
	string l_str_deep_call(LONG(l_call_deep), '*');
	std::unique_ptr<CFlyPortTestThreadInfo> l_info(reinterpret_cast<CFlyPortTestThreadInfo*>(p_param));
	for (int i = 0; i < l_info->m_ports.size(); ++i)
	{
		send_udp_tcp_test_port(l_info->m_PID, l_info->m_CID, l_info->m_ip, l_info->m_ports[i].first, l_info->m_ports[i].second);
		//std::cout <<  "[" << l_str_deep_call << "]" <<
		//	          "[" << ++g_count_all << "] " <<
		//          l_info->get_type_port(i) << " port-thread-test ip = " << l_info->m_ip << ":" << l_info->m_ports[i].first <<
		//          " CID = " << l_info->m_CID << " PID = " << l_info->m_PID << std::endl;
#ifndef _WIN32
		if (g_setup_syslog_disable == false)
		{
			syslog(LOG_NOTICE, "[%s][%u] %s-port-thread-test %s:%s CID = %s PID = %s",
			       l_str_deep_call.c_str(),
				   ++g_count_all,
			       l_info->get_type_port(i),
			       l_info->m_ip.c_str(),
			       l_info->m_ports[i].first.c_str(),
			       l_info->m_CID.c_str(),
			       l_info->m_PID.c_str());
		}
#endif
	}
	return NULL;
}
//========================================================================================================
static string process_test_port(const CFlyServerContext& p_flyserver_cntx)
{
	string l_result;
	Json::Value l_root;
	Json::Reader reader(Json::Features::strictMode());
	const bool parsingSuccessful = reader.parse(p_flyserver_cntx.m_in_query, l_root);
	if (!parsingSuccessful)
	{
		const char* l_error = "[FLY_POST_QUERY_TEST_PORT] Failed to parse json configuration";
		std::cout  << l_error << std::endl;
#ifndef _WIN32
		syslog(LOG_ERR, "[FLY_POST_QUERY_TEST_PORT] Failed to parse json configuration - %s", l_error);
#endif
	}
	else
	{
		const Json::Value& l_udp = l_root["udp"];
		const Json::Value& l_tcp = l_root["tcp"];
		CFlyPortTestThreadInfo* l_info = NULL;
		if (l_tcp.size() || l_udp.size())
		{
			const string l_CID = l_root["CID"].asString();
			const string l_PID = l_root["PID"].asString();
			l_info = new CFlyPortTestThreadInfo;
			l_info->m_ip = p_flyserver_cntx.m_remote_ip;
			l_info->m_CID = l_CID;
			l_info->m_PID = l_PID;
			l_info->m_ports.reserve(l_tcp.size() + l_udp.size());
		}
		if (l_info)
		{
			for (int j = 0; j < l_udp.size(); ++j)
			{
				const std::string l_port = l_udp[j]["port"].asString();
				l_info->m_ports.push_back(std::make_pair(l_port, false));
			}
			for (int k = 0; k < l_tcp.size(); ++k)
			{
				const std::string l_port = l_tcp[k]["port"].asString();
				l_info->m_ports.push_back(std::make_pair(l_port, true));
			}
			// �������� ����� ��� ����� TCP
			if (mg_start_thread(thread_proc_udp_tcp_test_port, l_info))
			{
				delete l_info;
			}
		}
		Json::Value l_test_port_result;
		l_test_port_result["ip"] = p_flyserver_cntx.m_remote_ip;
		l_result = l_test_port_result.toStyledString();
	}
	return l_result;
}
//==========================================================================
static void* thread_proc_store_log(void* p_param)
{
	CFlyLogThreadInfoArray* l_p_array = (CFlyLogThreadInfoArray*)p_param;

	for (CFlyLogThreadInfoArray::iterator i = l_p_array->begin(); i != l_p_array->end(); ++i)
	{
		const char* l_log_dir_name =  NULL;
		switch (i->m_query_type)
		{
#ifdef FLY_SERVER_USE_FULL_LOCAL_LOG
			case FLY_POST_QUERY_LOGIN:
				if (!g_setup_log_disable_login)
					l_log_dir_name = "log-login";
				break;
			case FLY_POST_QUERY_GET:
				l_log_dir_name = "log-get";
				break;
#endif
			case FLY_POST_QUERY_TEST_PORT:
				//if (!g_setup_log_disable_test_port)
				//	l_log_dir_name = "log-test-port";
				break;
		}
		if (l_log_dir_name)
		{
			const string l_file_name = CFlyServerContext::get_json_file_name(l_log_dir_name, i->m_remote_ip.c_str(), i->m_now);
			std::fstream l_log_json(l_file_name.c_str(), std::ios_base::out | std::ios_base::trunc);
			if (!l_log_json.is_open())
			{
				std::cout << "Error open file: " << l_file_name << " errno = " << errno << "\r\n";
#ifndef _WIN32
				syslog(LOG_ERR, "Error open file: = %s errno = %d", l_file_name.c_str(), errno);
#endif
			}
			else
			{
				if (i->m_in_query.length())
				{
#ifdef FLYLINKDC_USE_TEST_PORT_PROMETHEUS	
						g_DB.flyserver_log_counter(l_log_dir_name, i->m_in_query.length());			
#endif						
						l_log_json.write(i->m_in_query.c_str(), i->m_in_query.length());
						if (l_log_json.fail() || !l_log_json.good()) 
							{
								std::cout << "Error: failed to write to l_log_json!" << l_file_name << " errno = " << errno <<"\r\n";
#ifndef _WIN32
								syslog(LOG_ERR, "Error: failed to write to l_log_json! file = %s errno = %d", l_file_name.c_str(), errno);
#endif
							}
				}
				else
				{
					std::cout << "Error: len(=0) for log file: " << l_file_name << " errno = " << errno << "\r\n";
#ifndef _WIN32
					syslog(LOG_ERR, "Error: len(=0) for log file = %s errno = %d", l_file_name.c_str(), errno);
#endif
				}
#ifdef _DEBUG
				if (i->m_query_type != FLY_POST_QUERY_TEST_PORT)
				{
					// TODO - ��������� ��� ������������
					//if (!l_p->l_flyserver_cntx.m_res_stat.empty())
					//{
					//  l_log_json << std::endl << "OUT:" << std::endl << l_flyserver_cntx.m_res_stat;
					//}
				}
#endif
			}
		}
	}
	std::cout << std::endl << "Flush log files count: " << l_p_array->size() << "\r\n";
#ifndef _WIN32
	syslog(LOG_NOTICE, "Flush log files count: = %d", int(l_p_array->size()));
#endif
	delete l_p_array;
	return NULL;
}
//==========================================================================
void CFlyServerContext::run_db_query(const char* p_content, size_t p_len, CDBManager& p_DB)
{
	zlib_uncompress((uint8_t*)p_content, p_len, m_decompress);
	m_tick_count_start_db = get_tick_count();
	init_in_query(p_content, p_len);

#ifdef MT_DEBUG
	if (m_query_type == FLY_POST_QUERY_GET && m_decompress.size() != p_len)
	{
		std::ofstream l_fs;
		static int g_id_file;
		l_fs.open(std::string("flylinkdc-extjson-zlib-file-" + toString(++g_id_file) + ".json.zlib").c_str(), std::ifstream::out);
#ifdef __Linux__
//		l_fs.open("flylinkdc-extjson-zlib-file-" + toString(++g_id_file) + ".json.zlib", std::ifstream::out);
#else
//      l_fs.open("flylinkdc-extjson-zlib-file-" + toString(++g_id_file) + ".json.zlib", std::ifstream::out | std::ifstream::binary);
#endif
		l_fs.write(p_content, p_len);
	}
#endif // MT_DEBUG

	if (m_query_type == FLY_POST_QUERY_TEST_PORT)
	{
		m_res_stat = process_test_port(*this);
	}
	m_tick_count_stop_db = get_tick_count();
	run_thread_log();
	comress_result();
}
//==========================================================================
void CFlyServerContext::send_syslog() const
{
	extern unsigned long long g_sum_out_size;
	extern unsigned long long g_sum_in_size;
	extern unsigned long long g_z_sum_out_size;
	extern unsigned long long g_z_sum_in_size;
	extern unsigned long long g_count_query;
	if (g_setup_syslog_disable == false)
	{
		char l_log_buf[512];
		l_log_buf[0]   = 0;// ���� ����������� 152-160
		char l_buf_cache[32];
		l_buf_cache[0] = 0;
		char l_buf_counter[64];
		l_buf_counter[0] = 0;
		if (m_count_cache)
		{
			snprintf(l_buf_cache, sizeof(l_buf_cache), "[cache=%u]", (unsigned) m_count_cache);
		}
		if (m_count_get_only_counter == 0 && m_count_get_base_media_counter == 1 && m_count_get_ext_media_counter == 1)
		{
			snprintf(l_buf_counter, sizeof(l_buf_counter), "%s","[get full Inform!]");
		}
		else if (m_count_get_base_media_counter != 0 || m_count_get_ext_media_counter != 0 || m_count_insert != 0)
		{
			snprintf(l_buf_counter, sizeof(l_buf_counter), "[cnt=%u,base=%u,ext=%u,new=%u]",
			         (unsigned)m_count_get_only_counter,
			         (unsigned)m_count_get_base_media_counter,
			         (unsigned)m_count_get_ext_media_counter,
			         (unsigned)m_count_insert);
		}
		if (m_query_type == FLY_POST_QUERY_TEST_PORT)
		{
			snprintf(l_log_buf, sizeof(l_log_buf), "[%s][%c][%u][%s][%s][in:%u/%u][c:%u][%s]",
			         m_fly_response.c_str(),
			         get_compress_flag(),
			         (unsigned)m_count_file_in_json,
			         m_uri.c_str(),
			         m_remote_ip.c_str(),
			         (unsigned)get_real_query_size(),
			         (unsigned)m_content_len,
					(unsigned)g_count_query,
			         m_user_agent.c_str()
			        );
		}
		else
		{
			snprintf(l_log_buf, sizeof(l_log_buf), "[%s][%c][%u]%s[%s][%s][%u/%u->%u/%u][time db=%u][%s]%s%s",
			         m_fly_response.c_str(),
			         get_compress_flag(),
			         (unsigned)m_count_file_in_json,
			         "",
			         m_uri.c_str(),
			         m_remote_ip.c_str(),
			         (unsigned)get_real_query_size(),
			         (unsigned)m_content_len,
			         (unsigned)m_res_stat.size(),
			         (unsigned)get_http_len(),
					 (unsigned)get_delta_db(),
			         m_user_agent.c_str(),
			         l_buf_cache,
			         l_buf_counter
			        );
		}
		std::cout << ".";
		static int g_cnt = 0;
		if ((++g_cnt % 30) == 0)
			std::cout << std::endl;
#ifndef _WIN32 // Only in linux
		syslog(LOG_NOTICE, "%s", l_log_buf);
#endif
		std::cout << l_log_buf << std::endl;
	}
}

//==========================================================================
void CFlyServerContext::flush_log_array(bool p_is_force)
{
	if (g_log_array)
	{
#ifndef _DEBUG
		if (g_log_array->size() > 50 || p_is_force)
#else
		if (g_log_array->size() > 0 || p_is_force)
#endif
		{
			CFlyLogThreadInfoArray* l_log_array = g_log_array;
			g_log_array = NULL;
			if (mg_start_thread(thread_proc_store_log, l_log_array))
			{
				thread_proc_store_log(l_log_array); // ��������� ��� ������ � �������� l_thread_param
			}
		}
	}
}
//==========================================================================
void CFlyServerContext::run_thread_log()
{
	if (is_valid_query())
	{
		if (!m_in_query.empty() &&
		        (m_query_type == FLY_POST_QUERY_TEST_PORT && g_setup_log_disable_test_port == false)
		   ) // ���� ������� ������ � ��� ������� ����� ��������� �� ����
		{
				if (!g_log_array)
				{
					g_log_array = new CFlyLogThreadInfoArray;
				}
				g_log_array->push_back(CFlyLogThreadInfo(m_query_type, m_remote_ip, m_in_query));
				flush_log_array(false);
		}
	}
	else
	{
		const char* l_log_text = "l_query_type == FLY_POST_QUERY_TEST_PORT";
#ifdef _DEBUG
		std::cout << l_log_text << std::endl;
#ifndef _WIN32
		syslog(LOG_NOTICE, "%s", l_log_text);
#endif
#endif // _DEBUG
	}
}
//========================================================================================================
bool zlib_compress(const char* p_source, size_t p_len, std::vector<unsigned char>& p_compress, int& p_zlib_result, int p_level /*= 9*/)
{
	auto l_dest_length = compressBound(p_len) + 2;
	p_compress.resize(l_dest_length);

	p_zlib_result = compress2(p_compress.data(), &l_dest_length, (uint8_t*)p_source, p_len, p_level);
	if (p_zlib_result == Z_OK) // TODO - Check memory
	{
#ifdef _DEBUG
		if (l_dest_length)
		{
			std::cout << std::endl << "Compress  zlib size " << p_len << "/" << l_dest_length << std::endl;
		}
#endif
		p_compress.resize(l_dest_length);
	}
	else
	{
		p_compress.clear();
	}
	return !p_compress.empty();
}
//========================================================================================================
bool zlib_uncompress(const uint8_t* p_zlib_source, size_t p_zlib_len, std::vector<unsigned char>& p_decompress)
{
	auto l_decompress_size = p_zlib_len * 3;
	if (p_zlib_len >= 2 && p_zlib_source[0] == 0x78) // zlib �������?
													 // && (unsigned char)p_zlib_source[1] == 0x9C  ���� ��� ����� ���� ������
	{
		// ������� ���������� - ��� ������� ����� (������� �������)
		// 1     - 0x01
		// [2-5] - 0x5e
		// 6 - 0x9c
		// [7-9] - 0xda
		//			#ifdef _DEBUG
		//					char l_dump_zlib_debug[10] = {0};
		//					sprintf(l_dump_zlib_debug, "%#x", (unsigned char)l_post_data[1] & 0xFF);
		//				    std::cout << "DEBUD zlib decompress header l_post_data[1] = " << l_dump_zlib_debug << std::endl;
		//			#endif

		p_decompress.resize(l_decompress_size);
		while (1)
		{
			const int l_un_compress_result = uncompress(p_decompress.data(), &l_decompress_size, p_zlib_source, p_zlib_len);
			if (l_un_compress_result == Z_BUF_ERROR)
			{
				l_decompress_size *= 2;
				p_decompress.resize(l_decompress_size);
				continue;
			}
			if (l_un_compress_result == Z_OK)
			{
				p_decompress.resize(l_decompress_size);
			}
			else
			{
				p_decompress.clear(); // ���� ������ - �������� ������. ������ ������ �������� �������.
									  // TODO ����������� � ��������� ������� ������.

				std::cout << "Error zlib_uncompress: code = " << l_un_compress_result << std::endl;
#ifndef _WIN32
				syslog(LOG_ERR, "Error zlib_uncompress: code = %d", l_un_compress_result);
#endif
			}
			break;
		};
	}
	return !p_decompress.empty();
}
//========================================================================================================
void CDBManager::init()
{
#ifndef _WIN32
	openlog("fly-server-test-port", 0, LOG_USER); // LOG_PID
	syslog(LOG_NOTICE, "CDBManager init");
#endif
}
//========================================================================================================
void CDBManager::shutdown()
{
}
//========================================================================================================
CDBManager::~CDBManager()
{
	std::cout << std::endl << "* fly-server-test-port CDBManager::~CDBManager" << std::endl;
#ifndef _WIN32
	syslog(LOG_NOTICE, "CDBManager destroy!");
	closelog();
#endif
	std::cout << std::endl << "* fly-server-test-port CDBManager destroy!" << std::endl;
}
