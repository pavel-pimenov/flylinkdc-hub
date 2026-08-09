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

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::unique_ptr<TextFilesManager> TextFilesManager::m_Ptr;
//---------------------------------------------------------------------------

// TextFile destructor is = default in header
//---------------------------------------------------------------------------

TextFilesManager::~TextFilesManager() = default;
//---------------------------------------------------------------------------

bool TextFilesManager::ProcessTextFilesCmd(User* pUser, const char* sCommand, const bool bFromPM /* = false*/) const
{
    for (const auto& pFile : m_TextFiles)
    {
        TextFile* const cur = pFile.get();

        if (iequals(cur->m_sCommand, sCommand))
        {
            const bool bInPM = (SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_SEND_TEXT_FILES_AS_PM)] || bFromPM);
            const auto szHubSecLen = SettingManager::HubSecStr().size();
            size_t szChatLen = 0;

            // PPK ... to chat or to PM ???
            if (bInPM)
            {
                szChatLen = 18 + pUser->m_sNick.size() + (2 * szHubSecLen) + cur->m_sText.size();
            }
            else
            {
                szChatLen = 4 + szHubSecLen + cur->m_sText.size();
            }

            std::string sMSG;
            sMSG.resize(szChatLen);

            if (bInPM)
            {
                const int iRet = snprintf(sMSG.data(),
                                    szChatLen,
                                    "$To: %s From: %s $<%s> %s",
                                    pUser->m_sNick.c_str(),
                                    SettingManager::HubSec(),
                                    SettingManager::HubSec(),
                                    cur->m_sText.c_str());
                if (iRet <= 0)
                {
                    return true;
                }
            }
            else
            {
                const int iRet = snprintf(sMSG.data(), szChatLen, "<%s> %s", SettingManager::HubSec(), cur->m_sText.c_str());
                if (iRet <= 0)
                {
                    return true;
                }
            }

            pUser->SendCharDelayed(sMSG.data(), szChatLen - 1);

            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

void TextFilesManager::RefreshTextFiles()
{
    if (!SettingManager::m_Ptr->m_bBools[std::to_underlying(SetBoolIds::SETBOOL_ENABLE_TEXT_FILES)])
    {
        return;
    }

    m_TextFiles.clear();

    const std::string txtdir = ServerManager::m_sPath + "/texts/";

    DIR* p_txtdir = opendir(txtdir.c_str());

    if (!p_txtdir)
    {
        return;
    }

    struct dirent* p_dirent;
    struct stat s_buf;

    while ((p_dirent = readdir(p_txtdir)))
    {
        const std::string txtfile = txtdir + p_dirent->d_name;

        if (stat(txtfile.c_str(), &s_buf) != 0 || (s_buf.st_mode & S_IFDIR) != 0 || strcasecmp(p_dirent->d_name + (strlen(p_dirent->d_name) - 4), ".txt") != 0)
        {
            continue;
        }

        FilePtr f(fopen(txtfile.c_str(), "rb"));
        if (f)
        {
            if (s_buf.st_size != 0)
            {
                auto pNewTxtFile = std::make_unique<TextFile>();

                pNewTxtFile->m_sText.resize(s_buf.st_size + 2);
                const size_t size = fread(pNewTxtFile->m_sText.data(), 1, s_buf.st_size, f.get());
                pNewTxtFile->m_sText[size] = '|';
                pNewTxtFile->m_sText[size + 1] = '\0';

                const size_t szFileName = strlen(p_dirent->d_name) - 4;
                pNewTxtFile->m_sCommand.assign(p_dirent->d_name, szFileName);

                m_TextFiles.push_front(std::move(pNewTxtFile));
            }
        }
    }

    closedir(p_txtdir);
}
//---------------------------------------------------------------------------
