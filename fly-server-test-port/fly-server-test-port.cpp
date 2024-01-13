//-----------------------------------------------------------------------------
//(c) 2007-2023 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include <map>

//============================================================================================

#include "CDBManager.h"

#include <ctime>

#ifndef _WIN32 // Only in linux
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>

#else

#include <direct.h>
#define snprintf _snprintf
#endif // _WIN32

#ifdef _WIN32 // Only in WIN32
#ifdef _DEBUG
// #define VLD_DEFAULT_MAX_DATA_DUMP 1
// #define VLD_FORCE_ENABLE // Uncoment this define to enable VLD in release
// #include "C:\Program Files (x86)\Visual Leak Detector\include\vld.h" // VLD ������ ��� http://vld.codeplex.com/
#endif
#endif

#include "prometheus/CivetServer.h"

#ifndef _WIN32

#ifdef USE_LIBUNWIND

#define UNW_LOCAL_ONLY
#include <libunwind.h>

void show_backtrace (void) {
  unw_cursor_t cursor; unw_context_t uc;
  unw_word_t ip, sp;

  unw_getcontext(&uc);
  unw_init_local(&cursor, &uc);
  while (unw_step(&cursor) > 0) {
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_REG_SP, &sp);
    printf ("ip = %lx, sp = %lx\n", (long) ip, (long) sp);
  }
}
#endif //

#endif // _WIN32
//==========================================================================
CDBManager g_DB;
//==========================================================================
unsigned long long g_count_query = 0;
unsigned long long g_sum_out_size = 0;
unsigned long long g_sum_in_size = 0;
unsigned long long g_z_sum_out_size = 0;
unsigned long long g_z_sum_in_size = 0;
//==========================================================================
struct CFlyFloodCommand
{
	sqlite_int64  m_start_tick;
	sqlite_int64  m_tick;
	unsigned m_count;
	void init()
	{
		m_start_tick = 0;
		m_tick = 0;
		m_count = 0;
	}
	bool isCheckPoint() const
	{
		return (m_tick - m_start_tick) / 100 > 10;
	}
	bool isBan() const
	{
		return m_count > 100 && isCheckPoint();
	}
	CFlyFloodCommand()
	{
		init();
	}
};
//==========================================================================
static std::map<std::string, CFlyFloodCommand> g_ip_counter;
//==========================================================================
static void check_ip_flood(const std::string& p_remote_ip, const int64_t p_tick)
{
	CFlyFloodCommand& l_ip_stat = g_ip_counter[p_remote_ip];
	if (l_ip_stat.m_count == 0)
	{
		l_ip_stat.m_start_tick = p_tick;
	}
	l_ip_stat.m_tick = p_tick;
	l_ip_stat.m_count++;
	if (l_ip_stat.isBan())
	{
		std::cout << "IP Flood IP = " << p_remote_ip << " count = " << l_ip_stat.m_count << std::endl;
#ifndef _WIN32
		syslog(LOG_ERR, "IP Flood %s count = %d", p_remote_ip.c_str(), l_ip_stat.m_count);
#endif
		// return MG_FALSE;
	}
	if (l_ip_stat.isCheckPoint())
	{
		l_ip_stat.init(); // �� 10 ��� �� ������� ����� ���������
	}
}
//==========================================================================
static const char *g_html_form =
    "<html><body>Error fly-server-test-port - contact: pavel.pimenov@gmail.com query</body></html>";
//==========================================================================
static const char* g_badRequestReply = "HTTP/1.1 400 Bad Request\r\n";
//==========================================================================

#if 0

//==========================================================================
static int begin_request_handler(struct mg_connection *conn, enum mg_event ev)
{
	if (ev == MG_AUTH)
	{
		//dcassert(0);
		return MG_TRUE;
	}
	if (ev == MG_REQUEST)
	{
		check_ip_flood(conn->remote_ip, get_tick_count());
		CFlyServerContext l_flyserver_cntx;
		l_flyserver_cntx.init_uri(conn->uri, mg_get_header(conn, "User-Agent"), mg_get_header(conn, "X-fly-response"));
		l_flyserver_cntx.m_remote_ip = conn->remote_ip;
		l_flyserver_cntx.m_content_len = conn->content_len;
		if (l_flyserver_cntx.m_query_type != FLY_POST_QUERY_UNKNOWN)
		{
			if (conn->content_len <= 0)
			{
				mg_printf(conn, "%s", g_badRequestReply);
				mg_printf(conn, "Error: no content\n");
				std::cout << "Error: no content." << std::endl;
#ifndef _WIN32
				syslog(LOG_ERR, "Error: no content");
#endif
				return MG_TRUE;
			}
			l_flyserver_cntx.run_db_query(conn->content, conn->content_len, g_DB);
			const int l_result_mg_printf = mg_printf(conn, "HTTP/1.1 200 OK\r\n"          // TODO - HTTP/1.1 ?
			                                         "Content-Length: %lu\r\n\r\n",
			                                         l_flyserver_cntx.get_http_len());
			                                         
			/*
			TODO -  http://www.rsdn.ru/forum/network/5060321.1
			const int l_result_mg_printf = mg_printf_dynamic(conn, l_http_len + 300, "HTTP/1.1 200 OK\r\n"
			                "Content-Length: %u\r\n"
			                "Content-Type: application/json\r\n"
			                "%s",
			                l_http_len,
			                l_is_zlib ? "Content-Encoding: deflate\r\n\r\n" : "\r\n"
			                );
			*/
			const int l_result_mg_write = mg_write(conn, l_flyserver_cntx.get_result_content(), l_flyserver_cntx.get_http_len());
			g_sum_out_size += l_flyserver_cntx.m_res_stat.size();
			g_sum_in_size  += l_flyserver_cntx.get_real_query_size();
			g_z_sum_out_size += l_flyserver_cntx.get_http_len();
			g_z_sum_in_size  += conn->content_len;
			l_flyserver_cntx.send_syslog();
			++g_count_query;
			
#ifdef _DEBUG
			std::cout << "[DEBUG] l_result_mg_write = " << l_result_mg_write << " l_http_len = " << l_flyserver_cntx.get_http_len() << " l_result_mg_printf = " << l_result_mg_printf << std::endl;
#endif
			return MG_TRUE;
		}
		else
		{
			// Show HTML form.
			mg_printf(conn, "HTTP/1.1 200 OK\r\n"
			          "Content-Length: %d\r\n"
			          "Content-Type: text/html\r\n\r\n%s",
			          (int) strlen(g_html_form), g_html_form);
			return MG_TRUE;
		}
		// Mark as processed
	}
	return MG_FALSE;
}

#endif

int g_test_port_exit_flag = 0;
//======================================================================================
class FlyServerHandler : public CivetHandler
{
private:
	bool
		handleAll(const char *method,
			CivetServer *server,
			struct mg_connection *conn)
	{
		/* Handler may access the request info using mg_get_request_info */
		const struct mg_request_info *req_info = mg_get_request_info(conn);

		CFlyServerContext l_flyserver_cntx;
		l_flyserver_cntx.init_uri(req_info->request_uri, 
			mg_get_header(conn, "User-Agent"), 
			mg_get_header(conn, "X-fly-response"));
		const char* l_real_ip = mg_get_header(conn, "X-Real-IP");
		if (l_real_ip)
		{
			l_flyserver_cntx.m_remote_ip = l_real_ip;
			//std::cout << "X-Real-IP = " << l_flyserver_cntx.m_remote_ip << std::endl;
		}
		else
		{
			l_flyserver_cntx.m_remote_ip = req_info->remote_addr;
		}
		l_flyserver_cntx.m_content_len = req_info->content_length;
		if (l_flyserver_cntx.m_query_type != FLY_POST_QUERY_UNKNOWN)
		{
			if (req_info->content_length <= 0)
			{
				mg_printf(conn, "%s", g_badRequestReply);
				mg_printf(conn, "Error: no content\n");
				std::cout << "Error: no content." << std::endl;
#ifndef _WIN32
				syslog(LOG_NOTICE, "Error: no content");
#endif
				return true;
			}
			std::vector<char> buf; // TODO unique_ptr[]
			buf.resize(l_flyserver_cntx.m_content_len+1);
			buf[l_flyserver_cntx.m_content_len - 1] = 0;
			long long rlen;
			long long nlen = 0;
			const long long tlen = req_info->content_length;
			while (nlen < tlen) {
				rlen = tlen - nlen;
				rlen = mg_read(conn, buf.data()+ nlen, (size_t)rlen);
				if (rlen <= 0) {
					break;
				}
#ifdef _DEBUG
				std::cout << "[DEBUG] mg_read = " << rlen << std::endl;
#endif
				nlen += rlen;
			}
			l_flyserver_cntx.run_db_query(buf.data(), l_flyserver_cntx.m_content_len, g_DB);
			const int l_result_mg_printf = mg_printf(conn, "HTTP/1.1 200 OK\r\n"          // TODO - HTTP/1.1 ?
				"Content-Length: %lu\r\n\r\n",
				l_flyserver_cntx.get_http_len());

			const int l_result_mg_write = mg_write(conn, l_flyserver_cntx.get_result_content(), l_flyserver_cntx.get_http_len());
			g_sum_out_size += l_flyserver_cntx.m_res_stat.size();
			g_sum_in_size += l_flyserver_cntx.get_real_query_size();
			g_z_sum_out_size += l_flyserver_cntx.get_http_len();
			g_z_sum_in_size += l_flyserver_cntx.m_content_len;
			l_flyserver_cntx.send_syslog();
			++g_count_query;

#ifdef _DEBUG
			std::cout << "[DEBUG] l_result_mg_write = " << l_result_mg_write <<
				" l_http_len = " << l_flyserver_cntx.get_http_len() << " l_result_mg_printf = " << l_result_mg_printf << std::endl;
#endif
			return true;
		}
		else
		{
			// Show HTML form.
			mg_printf(conn, "HTTP/1.1 200 OK\r\n"
				"Content-Length: %d\r\n"
				"Content-Type: text/html\r\n\r\n%s",
				(int)strlen(g_html_form), g_html_form);
			return true;
		}
		// Mark as processed
		return false; //TODO ? 
	}

public:
	bool
		handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("POST", server, conn);
	}
};
//======================================================================================
void* run_fly_server_test_port(void*)
{
	std::cout << std::endl << "* FlylinkDC++ server for test port (c) 2012-2023 pavel.pimenov@gmail.com " << std::endl
		<< "  - civetweb " << CIVETWEB_VERSION << " (c) https://github.com/civetweb/civetweb" << std::endl;
//		<< std::endl << "Usage: fly-server-test-port [-disable-syslog] [-disable-log-test-port]"
//		<< std::endl << std::endl;
	
	g_setup_log_disable_test_port = true;
	g_setup_syslog_disable = true;
/*
	for (int i = 1; i < argc; i++)
	{
		const std::string l_argv = argv[i];
		std::cout << "* try: " << l_argv << std::endl;
		if (l_argv == "-disable-log-test-port")
		{
			g_setup_log_disable_test_port = true;
			std::cout << "*[+] " << l_argv << std::endl;
		}
		if (l_argv == "-disable-syslog")
		{
			std::cout << "*[+] " << l_argv << std::endl;
		}
	}
*/
#ifndef _WIN32 // Only in linux
	if (!g_setup_log_disable_test_port)
		mkdir("log-test-port", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
	if (!g_setup_log_disable_test_port)
		mkdir("log-test-port");
#endif
	g_DB.init();

	const char *options[] = {
			  "document_root", ".",
#ifndef NDEBUG
			  "listening_ports", "37015",
#else
			  "listening_ports", "37015",
#endif			  
			  "num_threads", "2",
			  "enable_directory_listing", "no", NULL
	};

	std::vector<std::string> cpp_options;
	for (int i = 0; i < (sizeof(options) / sizeof(options[0]) - 1); i++) {
		cpp_options.push_back(options[i]);
		std::cout << options[i];
		if((i & 0x1) == 0)
			std::cout << " = ";
		else	
			std::cout << std::endl;
	}
	try
	{
	CivetServer server(cpp_options);
	FlyServerHandler h_a;
	server.addHandler("/", h_a);
	while (g_test_port_exit_flag == 0)
	{
#ifdef _WIN32
		Sleep(10);
#else
		sleep(1);
#endif
	}
	}
	catch (CivetException& e)
	{
		std::cout << "CivetException:" << e.what() << std::endl;
	}

	CFlyServerContext::flush_log_array(true);
	g_DB.shutdown();
	std::cout << std::endl << "* fly-server-test-port shutdown!" << std::endl;
	return NULL;
}

