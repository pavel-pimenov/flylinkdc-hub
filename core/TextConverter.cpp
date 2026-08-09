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
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "TextConverter.h"
#ifdef FLYLINKDC_USE_DB
#if defined(_WITH_SQLITE)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::unique_ptr<TextConverter> TextConverter::m_Ptr;
#ifndef ICONV_CONST
#if defined(__NetBSD__) || (defined(__sun) && defined(__SVR4))
#define ICONV_CONST const
#else
#define ICONV_CONST
#endif
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

TextConverter::TextConverter()
{
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_ENCODING)].empty())
    {
        LogInfo("TextConverter failed to initialize - TextEncoding not set!");
        exit(EXIT_FAILURE);
    }

    m_iconvUtfCheck = iconv_open("utf-8", "utf-8"); //-V549 intentional: same src/dst for UTF-8 validation
    if (m_iconvUtfCheck == (iconv_t)-1)
    {
        LogInfo("TextConverter iconv_open for m_iconvUtfCheck failed!");
        exit(EXIT_FAILURE);
    }

    m_iconvAsciiToUtf = iconv_open("utf-8//TRANSLIT//IGNORE", SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_ENCODING)].c_str());
    if (m_iconvAsciiToUtf == (iconv_t)-1)
    {
        LogInfo("TextConverter iconv_open for m_iconvAsciiToUtf failed!");
        exit(EXIT_FAILURE);
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

TextConverter::~TextConverter()
{
    iconv_close(m_iconvUtfCheck);
    iconv_close(m_iconvAsciiToUtf);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool TextConverter::CheckUtf8Validity(const char* sInput, const uint8_t ui8InputLen, char* sOutput, const uint8_t ui8OutputSize)
{
    const char* sInBuf = sInput;
    size_t szInbufLeft = ui8InputLen;

    char* sOutBuf = sOutput;
    size_t szOutbufLeft = ui8OutputSize - 1;

    const size_t szRet = iconv(m_iconvUtfCheck, (ICONV_CONST char**)&sInBuf, &szInbufLeft, &sOutBuf, &szOutbufLeft);
    if (szRet == static_cast<size_t>(-1))
    {
        iconv(m_iconvUtfCheck, nullptr, nullptr, nullptr, nullptr);
        return false;
    }
    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

size_t TextConverter::CheckUtf8AndConvert(const char* sInput, const uint8_t ui8InputLen, char* sOutput, const uint8_t ui8OutputSize)
{
    if (CheckUtf8Validity(sInput, ui8InputLen, sOutput, ui8OutputSize))
    {
        sOutput[ui8InputLen] = '\0';
        return ui8InputLen;
    }

    const char* sInBuf = sInput;
    size_t szInbufLeft = ui8InputLen;

    char* sOutBuf = sOutput;
    size_t szOutbufLeft = ui8OutputSize - 1;

    const size_t szRet = iconv(m_iconvAsciiToUtf, (ICONV_CONST char**)&sInBuf, &szInbufLeft, &sOutBuf, &szOutbufLeft);
    if (szRet == static_cast<size_t>(-1))
    {
        iconv(m_iconvAsciiToUtf, nullptr, nullptr, nullptr, nullptr);
        if (errno == E2BIG)
        {
            UdpDebug::m_Ptr->Broadcast("[LOG] TextConverter::DoIconv iconv E2BIG for param: " + std::string(sInput, ui8InputLen));
        }
        else if (errno == EILSEQ)
        {
            UdpDebug::m_Ptr->Broadcast("[LOG] TextConverter::DoIconv iconv EILSEQ for param: " + std::string(sInput, ui8InputLen));
        }
        else if (errno == EINVAL)
        {
            UdpDebug::m_Ptr->Broadcast("[LOG] TextConverter::DoIconv iconv EINVAL for param: " + std::string(sInput, ui8InputLen));
        }
        sOutput[0] = '\0';
        return 0;
    }

    sOutput[(ui8OutputSize - szOutbufLeft) - 1] = '\0';
    return (ui8OutputSize - szOutbufLeft) - 1;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif // _WITH_SQLITE
#endif // FLYLINKDC_USE_DB