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
#ifndef utilityH
#define utilityH
//---------------------------------------------------------------------------
struct BanItem;
struct RangeBanItem;
//---------------------------------------------------------------------------

void Cout(const string & sMsg);
//---------------------------------------------------------------------------

#ifdef _WIN32
static void preD(char *pat, int M, int D[]);
static void suffixes(char *pat, int M, int *suff);
static void preDD(char *pat, int M, int DD[]);

int BMFind(char *text, int N, char *pat, int M);
#endif

char * Lock2Key(char * sLock);

#ifdef _WIN32
char * WSErrorStr(const uint32_t ui32Error);
#else
const char * ErrnoStr(const uint32_t ui32Error);
#endif

const char * formatBytes(const uint64_t ui64Bytes);
const char * formatBytesPerSecond(const uint64_t ui64Bytes);
const char * formatTime(uint64_t ui64Rest);
const char * formatSecTime(uint64_t ui64Rest);

char * stristr(const char *str1, const char *str2);
char * stristr2(const char *str1, const char *str2);

bool isIP(const char * sIP);

uint32_t HashNick(const char * sNick, const size_t szNickLen);

class Hash128
{
	uint8_t m_ui128Hash[16];
public:
	// Hash128(int ) // Fast;
	// {
	// }
	Hash128()
	{
		memset(m_ui128Hash, 0, sizeof(m_ui128Hash));
	}
	void init(const uint8_t * ui128Hash)
	{
		memcpy(m_ui128Hash, ui128Hash, sizeof(m_ui128Hash));
	}
	operator uint8_t* ()
	{
		return m_ui128Hash;
	}
	const uint8_t* data() const
	{
		return m_ui128Hash;
	}
	bool compare(const uint8_t * ui128Hash) const
	{
		return memcmp(m_ui128Hash, ui128Hash, sizeof(m_ui128Hash)) == 0;
	}
};

bool HashIP(const char * sIP, uint8_t * ui128IpHash);
uint16_t GetIpTableIdx(const uint8_t * ui128IpHash);

int GenerateBanMessage(BanItem * pBan, const time_t &tAccTime);
int GenerateRangeBanMessage(RangeBanItem * pRangeBan, const time_t &tAccTime);

bool GenerateTempBanTime(const uint8_t ui8Multiplyer, const uint32_t ui32Time, time_t &tAccTime, time_t &tBanTime);

bool HaveOnlyNumbers(char *sData, const uint16_t ui16Len);

inline size_t Allign256(size_t n)
{
	return (n + 1);
}
inline size_t Allign512(size_t n)
{
	return (n + 1);
}
inline size_t Allign1024(size_t n)
{
	return (n + 1);
}
inline size_t Allign16K(size_t n)
{
	return (n + 1);
}
inline size_t Allign128K(size_t n)
{
	return (n + 1);
}

void AppendLog(const char * sData, const bool bScript = false);
inline void AppendLog(const string & sData, const bool bScript = false)
{
	AppendLog(sData.c_str(), bScript);
}
void AppendDebugLog(const char * sData);
void AppendDebugLogFormat(const char * sFormatMsg, ...);

#ifdef _WIN32
void GetHeapStats(void * pHeap, DWORD &dwCommitted, DWORD &dwUnCommitted);
#endif

void Memo(const string & sMessage);

#ifdef _WIN32
const char * ExtractFileName(const char * sPath);
#endif

bool FileExist(const char * sPath);
bool DirExist(const char * sPath);

#ifdef _WIN32
void SetupOsVersion();
void * LuaAlocator(void * pOld, void * pData, const size_t szOldSize, const size_t szNewSize);
#if !defined(_WIN64) && !defined(_WIN_IOT)
INT win_inet_pton(PCTSTR pAddrString, PVOID pAddrBuf);
void win_inet_ntop(PVOID pAddr, PTSTR pStringBuf, size_t szStringBufSize);
#endif
#endif

void CheckForIPv4();
void CheckForIPv6();

bool GetMacAddress(const char * sIP, char * sMac);

void CreateGlobalBuffer();
void DeleteGlobalBuffer();
bool CheckAndResizeGlobalBuffer(const size_t szWantedSize);
void ReduceGlobalBuffer();

bool HashPassword(const char * sPassword, const size_t szPassLen, uint8_t * ui8PassHash);
//[+]FlylinkDC++
inline uint16_t CalcHash(const uint32_t& ui32Hash)
{
	uint16_t ui16dx;
	memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));
	return ui16dx;
}

#ifdef _WIN32
uint64_t htobe64(const uint64_t & ui64Value);
uint64_t be64toh(const uint64_t & ui64Value);
#endif

bool WantAgain();
bool IsPrivateIP(const char * sIP);
//---------------------------------------------------------------------------
//[+]FlylinkDC++
#define safe_free(p) {if (p) { free(p); p = nullptr;} }
#define safe_free_and_init(p,p_new_ptr) {if (p) {free(p);}  p = p_new_ptr;}

/*
template <class T> inline void safe_free(T* & p)
{
    if (p)
    {
        free(p);
        p = nullptr;
    }
}
template <class T> inline void safe_free_and_init(T* & p, T* p_new_ptr)
{
    if (p)
    {
        free(p);
    }
    p = p_new_ptr;
}
*/
template <class T> inline void safe_delete(T* & p)
{
	if (p)
	{
#ifdef  _DEBUG
		// TODO boost::checked_delete(p);
#else
		delete p;
#endif
		p = nullptr;
	}
}
template <class T> inline void safe_delete_array(T* & p)
{
	if (p)
	{
#ifdef  _DEBUG
		// TODO boost::checked_array_delete(p);
#else
		delete[] p;
#endif
		p = nullptr;
	}
}
#ifdef _WIN32
static const int SHUT_RD = 0;
static const int SHUT_WR = 1;
static const int SHUT_RDWR = 2;
#endif
template <class T>inline void safe_closesocket(T& p_socket)
{
#ifdef _WIN32
	if (p_socket != INVALID_SOCKET)
	{
		closesocket(p_socket);
		p_socket = INVALID_SOCKET;
	}
#else
	if (p_socket != -1)
	{
		close(p_socket);
		p_socket = -1;
	}
#endif
}

template <class T>inline void shutdown_and_close(T& p_socket, int p_type)
{
#ifdef _WIN32
	(void)p_type;
	shutdown(p_socket, SD_SEND);
#else
	shutdown(p_socket, p_type);
#endif
	safe_closesocket(p_socket);
}

#endif
