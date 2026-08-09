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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef TextConverterH
#define TextConverterH
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef FLYLINKDC_USE_DB
#if defined(_WITH_SQLITE)
class TextConverter
{
private:
    iconv_t m_iconvUtfCheck;
    iconv_t m_iconvAsciiToUtf;

    [[nodiscard]] auto CheckUtf8Validity(const char* sInput, uint8_t ui8InputLen, char* sOutput, uint8_t ui8OutputSize) -> bool;

public:
    static std::unique_ptr<TextConverter> m_Ptr;

    TextConverter();
    ~TextConverter();

    [[nodiscard]] size_t CheckUtf8AndConvert(const char* sInput, uint8_t ui8InputLen, char* sOutput, uint8_t ui8OutputSize);
    TextConverter(const TextConverter&) = delete;
    auto operator=(const TextConverter&) -> TextConverter& = delete;
};
#endif // _WITH_SQLITE
#endif // FLYLINKDC_USE_DB
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
