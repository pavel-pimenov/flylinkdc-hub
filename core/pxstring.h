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

//------------------------------------------------------------------------------
#ifndef PXSTRING_H
#define PXSTRING_H
//------------------------------------------------------------------------------
class string
{
private:
	char * m_sData;

	size_t m_szDataLen;

	void stralloc(const char * sTxt, const size_t szLen);
	string(const string & sStr1, const string & sStr2);
	string(const char * sTxt, const string & sStr);
	string(const string & sStr, const char * sTxt);
public:
	string();
	explicit string(const char * sTxt);
	string(const char * sTxt, const size_t szLen);
	string(const string & sStr);
	explicit string(const uint32_t ui32Number);
	explicit string(const int32_t i32Number);
	explicit string(const uint64_t ui64Number);
	explicit string(const int64_t i64Number);

	~string();

	size_t size() const
	{
		return m_szDataLen;
	}
	const char * c_str() const
	{
		return m_sData;
	}
	void clear();

	string operator+(const char * sTxt) const;
	string operator+(const string & sStr) const;
	friend string operator+(const char * sTxt, const string & sStr);

	string & operator+=(const char * sTxt);
	string & operator+=(const string & sStr);

	string & operator=(const char * sTxt);
	string & operator=(const string & sStr);

	bool operator==(const char * sTxt);
	bool operator==(const string & sStr);
};

/*
#include <string>
using std::string;

class px_string
{
    private:
        char * m_sData;

        size_t m_szDataLen;

        void stralloc(const char * sTxt, const size_t szLen);
        px_string(const px_string & sStr1, const px_string & sStr2);
        px_string(const char * sTxt, const px_string & sStr);
        px_string(const px_string & sStr, const char * sTxt);
    public:
        px_string();
        explicit px_string(const char * sTxt);
        px_string(const char * sTxt, const size_t szLen);
        px_string(const px_string & sStr);
        explicit px_string(const uint32_t ui32Number);
        explicit px_string(const int32_t i32Number);
        explicit px_string(const uint64_t ui64Number);
        explicit px_string(const int64_t i64Number);

        ~px_string();

        size_t size() const
         {
          return m_szDataLen;
         }
        const char * c_str() const
         {
           return m_sData;
         }
        void clear();

        px_string operator+(const char * sTxt) const;
        px_string operator+(const px_string & sStr) const;
        friend px_string operator+(const char * sTxt, const px_string & sStr);
        friend px_string operator+(const char * sTxt, const std::string & sStr)
        {
            px_string result(sTxt);
            result  += sStr;
            return result;
        }

        px_string & operator+=(const char * sTxt);
        px_string & operator+=(const px_string & sStr);
        px_string & operator+=(const std::string & sStr)
        {
            return operator+=(sStr.c_str());
        }

        px_string & operator=(const char * sTxt);
        px_string & operator=(const px_string & sStr);

        bool operator==(const char * sTxt);
        bool operator==(const px_string & sStr);
};
//------------------------------------------------------------------------------
*/
#endif
