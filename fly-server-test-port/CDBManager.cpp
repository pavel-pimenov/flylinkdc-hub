//-----------------------------------------------------------------------------
//(c) 2007-2026 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------

#include <cstdio>
#include "CDBManager.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#define closesocket(x) close(x)
using std::string;
//==========================================================================
bool g_setup_log_disable_test_port = true;
//==========================================================================
CFlyLogThreadInfoArray* CFlyServerContext::g_log_array = nullptr;
//==========================================================================
namespace {
std::atomic<size_t> g_active_worker_threads{0};
std::mutex g_worker_mutex;
std::condition_variable g_worker_cv;

// RAII guard tracking detached worker threads so the hub can wait for them
// before tearing down spdlog at process exit (prevents heap-use-after-free).
class CFlyWorkerThreadGuard
{
public:
	CFlyWorkerThreadGuard() { ++g_active_worker_threads; }
	~CFlyWorkerThreadGuard()
	{
		if (--g_active_worker_threads == 0)
		{
			g_worker_cv.notify_all();
		}
	}
};
} // namespace
//==========================================================================
void CFlyServerContext::WaitWorkerThreads()
{
	std::unique_lock<std::mutex> l_lock(g_worker_mutex);
	g_worker_cv.wait_for(l_lock, std::chrono::seconds(30), [] { return g_active_worker_threads == 0; });
}
//==========================================================================
sqlite_int64 get_tick_count()
{
	struct timeval tim;
	gettimeofday(&tim, nullptr);
	unsigned int t = ((tim.tv_sec * 1000) + (tim.tv_usec / 1000)) & 0xffffffff;
	return t;
}
//==========================================================================
#ifdef FLYLINKDC_DEAD_CODE
static void set_socket_opt(SOCKET p_sock, int p_option, int p_val)
{
	int len = sizeof(p_val);
	if(setsockopt(p_sock, SOL_SOCKET, p_option, (char*)&p_val, len) < 0)
	{
		std::cout << "set_socket_opt option = "<< p_option << " val = " << p_val << " failed!\n";
	}
}
#endif // FLYLINKDC_DEAD_CODE
//==========================================================================
static void send_udp_tcp_test_port(const std::string&, const std::string& p_CID, const std::string& p_ip, const string& p_port, bool p_is_tcp)
{
#ifdef FLYLINKDC_USE_TEST_PORT_PROMETHEUS	
    g_DB.flyserver_test_port_counter(p_is_tcp ? "tcp": "udp");
#endif

#ifdef _DEBUG
	if (p_is_tcp)
	{
		std::cout << "TCP test_port - ip = " << p_ip << ":" << p_port << " CID = " << p_CID << " PID = " << p_PID << std::endl;
	}
	else
	{
		std::cout << "UDP test_port - ip = " << p_ip << ":" << p_port << " CID = " << p_CID << " PID = " << p_PID <<  std::endl;
	}
#endif
	const unsigned short l_port = atoi(p_port.c_str());
	const string l_header = "$FLY-TEST-PORT " + p_CID + p_ip + ':' + p_port + "|";
	struct sockaddr_in addr = {};
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
    {
        std::cout << "setsockopt SO_RCVTIMEO failed\n";
    }

    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
    {
        std::cout << "setsockopt SO_SNDTIMEO failed\n";
    }
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
	CFlySafeGuard l_call_deep(g_count_thread); // TODO - �������� ������� ����� ������� ����� ��
	CFlyWorkerThreadGuard l_worker_guard;
	string l_str_deep_call(LONG(l_call_deep), '*');
	std::unique_ptr<CFlyPortTestThreadInfo> l_info(reinterpret_cast<CFlyPortTestThreadInfo*>(p_param));
	for (size_t i = 0; i < l_info->m_ports.size(); ++i)
	{
		send_udp_tcp_test_port(l_info->m_PID, l_info->m_CID, l_info->m_ip, l_info->m_ports[i].first, l_info->m_ports[i].second);
		//std::cout <<  "[" << l_str_deep_call << "]" <<
		//	          "[" << ++g_count_all << "] " <<
		//	          l_info->get_type_port(i) << " port-thread-test ip = " << l_info->m_ip << ":" << l_info->m_ports[i].first <<
		//	          " CID = " << l_info->m_CID << " PID = " << l_info->m_PID << std::endl;
		g_count_all += 1;
		spdlog::info("[{}][{}] {}-port-thread-test {}:{} CID = {} PID = {}",
			       l_str_deep_call,
			   g_count_all,
		       l_info->get_type_port(i),
		       l_info->m_ip,
		       l_info->m_ports[i].first,
		       l_info->m_CID,
		       l_info->m_PID);
	}
	return nullptr;
}
//========================================================================================================
static string process_test_port(const CFlyServerContext& p_flyserver_cntx)
{
	string l_result;
	nlohmann::json l_root;
	try
	{
		l_root = nlohmann::json::parse(p_flyserver_cntx.m_in_query);
	}
	catch (const nlohmann::json::parse_error&)
	{
		const char* l_error = "[FLY_POST_QUERY_TEST_PORT] Failed to parse json configuration";
		std::cout  << l_error << std::endl;
		spdlog::error("[FLY_POST_QUERY_TEST_PORT] Failed to parse json configuration - {}", l_error);
		return l_result;
	}
	const auto& l_udp = l_root["udp"];
	const auto& l_tcp = l_root["tcp"];
	CFlyPortTestThreadInfo* l_info = nullptr;
	if (!l_tcp.empty() || !l_udp.empty())
	{
		if (l_root["CID"].is_null() || l_root["PID"].is_null())
		{
			spdlog::error("[FLY_POST_QUERY_TEST_PORT] Missing CID or PID in JSON: {}", p_flyserver_cntx.m_in_query);
			return l_result;
		}
		const string l_CID = l_root["CID"].is_string() ? l_root["CID"].get<std::string>() : std::to_string(l_root["CID"].get<int64_t>());
		const string l_PID = l_root["PID"].is_string() ? l_root["PID"].get<std::string>() : std::to_string(l_root["PID"].get<int64_t>());
		l_info = new CFlyPortTestThreadInfo;
		l_info->m_ip = p_flyserver_cntx.m_remote_ip;
		l_info->m_CID = l_CID;
		l_info->m_PID = l_PID;
		l_info->m_ports.reserve(l_tcp.size() + l_udp.size());
	}
	if (l_info)
	{
		for (size_t j = 0; j < l_udp.size(); ++j)
		{
			if (l_udp[j]["port"].is_null())
			{
				spdlog::warn("[FLY_POST_QUERY_TEST_PORT] Missing port in UDP entry {}", j);
				continue;
			}
			const std::string l_port = l_udp[j]["port"].is_string() ? l_udp[j]["port"].get<std::string>() : std::to_string(l_udp[j]["port"].get<int64_t>());
			l_info->m_ports.push_back(std::make_pair(l_port, false));
		}
		for (size_t k = 0; k < l_tcp.size(); ++k)
		{
			if (l_tcp[k]["port"].is_null())
			{
				spdlog::warn("[FLY_POST_QUERY_TEST_PORT] Missing port in TCP entry {}", k);
				continue;
			}
			const std::string l_port = l_tcp[k]["port"].is_string() ? l_tcp[k]["port"].get<std::string>() : std::to_string(l_tcp[k]["port"].get<int64_t>());
			l_info->m_ports.push_back(std::make_pair(l_port, true));
		}
		// �������� ����� ��� ����� TCP
		if (mg_start_thread(thread_proc_udp_tcp_test_port, l_info))
		{
			delete l_info;
		}
	}
	nlohmann::json l_test_port_result;
	l_test_port_result["ip"] = p_flyserver_cntx.m_remote_ip;
	l_result = l_test_port_result.dump(4);
	return l_result;
}
//==========================================================================
static void* thread_proc_store_log(void* p_param)
{
	CFlyWorkerThreadGuard l_worker_guard;
	CFlyLogThreadInfoArray* l_p_array = static_cast<CFlyLogThreadInfoArray*>(p_param);

	for (CFlyLogThreadInfoArray::iterator i = l_p_array->begin(); i != l_p_array->end(); ++i)
	{
		const char* l_log_dir_name = nullptr;
		switch (i->m_query_type)
		{
		case FLY_POST_QUERY_TEST_PORT:
			//if (!g_setup_log_disable_test_port)
			//	l_log_dir_name = "log-test-port";
			break;
		default:
			break;
		}
		if (l_log_dir_name) //-V547 always false: case assignments are commented out
		{
			const string l_file_name = CFlyServerContext::get_json_file_name(l_log_dir_name, i->m_remote_ip.c_str(), i->m_now);
			std::fstream l_log_json(l_file_name.c_str(), std::ios_base::out | std::ios_base::trunc);
			if (!l_log_json.is_open())
			{
				std::cout << "Error open file: " << l_file_name << " errno = " << errno << "\r\n";
				spdlog::error("Error open file: = {} errno = {}", l_file_name, errno);
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
						spdlog::error("Error: failed to write to l_log_json! file = {} errno = {}", l_file_name, errno);
							}
				}
				else
				{
				std::cout << "Error: len(=0) for log file: " << l_file_name << " errno = " << errno << "\r\n";
				spdlog::error("Error: len(=0) for log file = {} errno = {}", l_file_name, errno);
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
	spdlog::info("Flush log files count: = {}", l_p_array->size());
	delete l_p_array;
	return nullptr;
}
//==========================================================================
void CFlyServerContext::run_db_query(const char* p_content, size_t p_len, CDBManager&)
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
void CFlyServerContext::sendDebugLog() const
{
	extern unsigned long long g_count_query;
	char l_log_buf[512];
	l_log_buf[0]   = 0;
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
	spdlog::info("{}", l_log_buf);
	std::cout << l_log_buf << std::endl;
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
			g_log_array = nullptr;
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
#ifdef _DEBUG
		const char* l_log_text = "l_query_type == FLY_POST_QUERY_TEST_PORT";
		std::cout << l_log_text << std::endl;
		spdlog::debug("{}", l_log_text);
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
		while (true)
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
			spdlog::error("Error zlib_uncompress: code = {}", l_un_compress_result);
			}
			break;
		};
	}
	return !p_decompress.empty();
}
//========================================================================================================
void CDBManager::init()
{
	spdlog::info("CDBManager init");
}
//========================================================================================================
void CDBManager::shutdown()
{
}
//========================================================================================================
CDBManager::~CDBManager()
{
	std::cout << std::endl << "* fly-server-test-port CDBManager::~CDBManager" << std::endl;
	std::cout << std::endl << "* fly-server-test-port CDBManager destroy!" << std::endl;
}
