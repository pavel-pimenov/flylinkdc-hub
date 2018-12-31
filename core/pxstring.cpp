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
#include "stdinc.h"

//---------------------------------------------------------------------------
#include "pxstring.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
#pragma hdrstop
#endif
//---------------------------------------------------------------------------
static const char * sEmpty = "";
//---------------------------------------------------------------------------

void px_string::stralloc(const char * sTxt, const size_t szLen)
{
	m_szDataLen = szLen;
	
	if (m_szDataLen == 0)
	{
		m_sData = (char *)sEmpty;
		return;
	}
	
	m_sData = (char *)malloc(m_szDataLen + 1);
	if (m_sData == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sData in px_string::stralloc\n", m_szDataLen+1);
		
		return;
	}
	
	memcpy(m_sData, sTxt, m_szDataLen);
	m_sData[m_szDataLen] = '\0';
}
//---------------------------------------------------------------------------

px_string::px_string() : m_sData((char *)sEmpty), m_szDataLen(0)
{
	// ...
}
//---------------------------------------------------------------------------

px_string::px_string(const char * sTxt) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	stralloc(sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

px_string::px_string(const char * sTxt, const size_t szLen) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	stralloc(sTxt, szLen);
}
//---------------------------------------------------------------------------

px_string::px_string(const px_string & sStr) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	stralloc(sStr.c_str(), sStr.size());
}
//---------------------------------------------------------------------------

px_string::px_string(const uint32_t ui32Number) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	char tmp[16];
#ifdef _WIN32
	ultoa(ui32Number, tmp, 10);
	stralloc(tmp, strlen(tmp));
#else
	int iLen = snprintf(tmp, 16, "%u", ui32Number);
	if (iLen > 0)
	{
	stralloc(tmp, iLen);
	}
#endif
}
//---------------------------------------------------------------------------

px_string::px_string(const int32_t i32Number) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	char tmp[16];
#ifdef _WIN32
	ltoa(i32Number, tmp, 10);
	stralloc(tmp, strlen(tmp));
#else
	int iLen = snprintf(tmp, 16, "%d", i32Number);
	if (iLen > 0)
	{
	stralloc(tmp, iLen);
	}
#endif
}
//---------------------------------------------------------------------------

px_string::px_string(const uint64_t ui64Number) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	char tmp[32];
#ifdef _WIN32
	_ui64toa(ui64Number, tmp, 10);
	stralloc(tmp, strlen(tmp));
#else
	int iLen = snprintf(tmp, 32, "%" PRIu64, ui64Number);
	if (iLen > 0)
	{
	stralloc(tmp, iLen);
	}
#endif
}
//---------------------------------------------------------------------------

px_string::px_string(const int64_t i64Number) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	char tmp[32];
#ifdef _WIN32
	_i64toa(i64Number, tmp, 10);
	stralloc(tmp, strlen(tmp));
#else
	int iLen = snprintf(tmp, 32, "%" PRId64, i64Number);
	if (iLen > 0)
	{
	stralloc(tmp, iLen);
	}
#endif
}
//---------------------------------------------------------------------------

px_string::px_string(const px_string & sStr1, const px_string & sStr2) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	m_szDataLen = sStr1.size() + sStr2.size();
	
	if (m_szDataLen == 0)
	{
		m_sData = (char *)sEmpty;
		return;
	}
	
	m_sData = (char *)malloc(m_szDataLen + 1);
	if (m_sData == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sData in px_string::px_string(px_string, px_string)\n", m_szDataLen+1);
		
		return;
	}
	
	memcpy(m_sData, sStr1.c_str(), sStr1.size());
	memcpy(m_sData + sStr1.size(), sStr2.c_str(), sStr2.size());
	m_sData[m_szDataLen] = '\0';
}
//---------------------------------------------------------------------------

px_string::px_string(const char * sTxt, const px_string & sStr) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	size_t szLen = strlen(sTxt);
	m_szDataLen = szLen + sStr.size();
	
	if (m_szDataLen == 0)
	{
		m_sData = (char *)sEmpty;
		return;
	}
	
	m_sData = (char *)malloc(m_szDataLen + 1);
	if (m_sData == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sData in px_string::px_string(char, px_string)\n", m_szDataLen+1);
		
		return;
	}
	
	memcpy(m_sData, sTxt, szLen);
	memcpy(m_sData + szLen, sStr.c_str(), sStr.size());
	m_sData[m_szDataLen] = '\0';
}
//---------------------------------------------------------------------------

px_string::px_string(const px_string & sStr, const char * sTxt) : m_sData((char *)sEmpty), m_szDataLen(0)
{
	size_t szLen = strlen(sTxt);
	m_szDataLen = sStr.size() + szLen;
	
	if (m_szDataLen == 0)
	{
		m_sData = (char *)sEmpty;
		return;
	}
	
	m_sData = (char *)malloc(m_szDataLen + 1);
	if (m_sData == NULL)
	{
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sData in px_string::px_string(px_string, char)\n", m_szDataLen+1);
		
		return;
	}
	
	memcpy(m_sData, sStr.c_str(), sStr.size());
	memcpy(m_sData + sStr.size(), sTxt, szLen);
	m_sData[m_szDataLen] = '\0';
}
//---------------------------------------------------------------------------

px_string::~px_string()
{
	if (m_sData != sEmpty)
	{
		free(m_sData);
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void px_string::clear()
{
	if (m_sData != sEmpty)
	{
		free(m_sData);
	}
	
	m_sData = (char *)sEmpty;
	m_szDataLen = 0;
}
//---------------------------------------------------------------------------

px_string px_string::operator+(const char * sTxt) const
{
	px_string result(*this, sTxt);
	return result;
}
//---------------------------------------------------------------------------

px_string px_string::operator+(const px_string & sStr) const
{
	px_string result(*this, sStr);
	return result;
}
//---------------------------------------------------------------------------

px_string operator+(const char * sTxt, const px_string & sStr)
{
	px_string result(sTxt, sStr);
	return result;
}
//---------------------------------------------------------------------------

px_string & px_string::operator+=(const char * sTxt)
{
	size_t szLen = strlen(sTxt);
	
	char * oldbuf = m_sData;
	
	if (m_sData == sEmpty)
	{
		m_sData = (char *)malloc(m_szDataLen + szLen + 1);
	}
	else
	{
		m_sData = (char *)realloc(oldbuf, m_szDataLen + szLen + 1);
	}
	
	if (m_sData == NULL)
	{
		m_sData = oldbuf;
		
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sData in px_string::operator+=(char)\n", m_szDataLen+szLen+1);
		
		return *this;
	}
	
	memcpy(m_sData + m_szDataLen, sTxt, szLen);
	m_szDataLen += szLen;
	m_sData[m_szDataLen] = '\0';
	
	return *this;
}
//---------------------------------------------------------------------------

px_string & px_string::operator+=(const px_string & sStr)
{
	if (sStr.c_str() == sEmpty)
	{
		return *this;
	}
	
	char * oldbuf = m_sData;
	
	if (m_sData == sEmpty)
	{
		m_sData = (char *)malloc(m_szDataLen + sStr.size() + 1);
	}
	else
	{
		m_sData = (char *)realloc(oldbuf, m_szDataLen + sStr.size() + 1);
	}
	
	if (m_sData == NULL)
	{
		m_sData = oldbuf;
		
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sData in px_string::operator+=(px_string)\n", m_szDataLen+sStr.size()+1);
		
		return *this;
	}
	
	memcpy(m_sData + m_szDataLen, sStr.c_str(), sStr.size());
	m_szDataLen += sStr.size();
	m_sData[m_szDataLen] = '\0';
	
	return *this;
}
//---------------------------------------------------------------------------

px_string & px_string::operator=(const char * sTxt)
{
	if (m_sData != sEmpty)
	{
		free(m_sData);
	}
	
	stralloc(sTxt, strlen(sTxt));
	
	return *this;
}
//---------------------------------------------------------------------------

px_string & px_string::operator=(const px_string & sStr)
{
	if (this == &sStr)
		return *this;
	if (m_sData != sEmpty)
	{
		free(m_sData);
	}
	
	stralloc(sStr.c_str(), sStr.size());
	
	return *this;
}
//---------------------------------------------------------------------------

bool px_string::operator==(const char * sTxt)
{
	if (m_szDataLen != strlen(sTxt) ||
	        memcmp(m_sData, sTxt, m_szDataLen) != 0)
	{
		return false;
	}
	
	return true;
}
//---------------------------------------------------------------------------

bool px_string::operator==(const px_string & sStr)
{
	if (m_szDataLen != sStr.size() ||
	        memcmp(m_sData, sStr.c_str(), m_szDataLen) != 0)
	{
		return false;
	}
	
	return true;
}
//---------------------------------------------------------------------------
