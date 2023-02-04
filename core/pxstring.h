/*
 * PtokaX - hub server for Direct Connect peer to peer network.

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

//------------------------------------------------------------------------------
#ifndef PXSTRING_H
#define PXSTRING_H
//------------------------------------------------------------------------------
/*
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
*/

/*
#include <string>
using std::string;

class std::string
{
    private:
        char * m_sData;

        size_t m_szDataLen;

        void stralloc(const char * sTxt, const size_t szLen);
        std::to_string(const std::string & sStr1, const std::string & sStr2);
        std::to_string(const char * sTxt, const std::string & sStr);
        std::to_string(const std::string & sStr, const char * sTxt);
    public:
        std::to_string();
        explicit std::to_string(const char * sTxt);
        std::to_string(const char * sTxt, const size_t szLen);
        std::to_string(const std::string & sStr);
        explicit std::to_string(const uint32_t ui32Number);
        explicit std::to_string(const int32_t i32Number);
        explicit std::to_string(const uint64_t ui64Number);
        explicit std::to_string(const int64_t i64Number);

        ~std::to_string();

        size_t size() const
         {
          return m_szDataLen;
         }
        const char * c_str() const
         {
           return m_sData;
         }
        void clear();

        std::string operator+(const char * sTxt) const;
        std::string operator+(const std::string & sStr) const;
        friend std::string operator+(const char * sTxt, const std::string & sStr);
        friend std::string operator+(const char * sTxt, const std::string & sStr)
        {
            std::string result(sTxt);
            result  += sStr;
            return result;
        }

        std::string & operator+=(const char * sTxt);
        std::string & operator+=(const std::string & sStr);
        std::string & operator+=(const std::string & sStr)
        {
            return operator+=(sStr.c_str());
        }

        std::string & operator=(const char * sTxt);
        std::string & operator=(const std::string & sStr);

        bool operator==(const char * sTxt);
        bool operator==(const std::string & sStr);
};
//------------------------------------------------------------------------------
*/
#endif
