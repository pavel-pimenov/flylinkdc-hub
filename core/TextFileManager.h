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
#ifndef TextFileManagerH
#define TextFileManagerH
//---------------------------------------------------------------------------
#include <list>
#include <memory>
#include <string>
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class TextFilesManager
{
private:
    struct TextFile
    {
        std::string m_sCommand, m_sText;

        TextFile() = default;
        ~TextFile() = default;

        TextFile(const TextFile&) = delete;

        auto operator=(const TextFile&) -> TextFile& = delete;
    };

    std::list<std::unique_ptr<TextFile>> m_TextFiles;

public:
    static std::unique_ptr<TextFilesManager> m_Ptr;

    TextFilesManager(const TextFilesManager&) = delete;
    auto operator=(const TextFilesManager&) -> TextFilesManager& = delete;

    TextFilesManager() = default;
    ~TextFilesManager();

    [[nodiscard]] auto ProcessTextFilesCmd(User* pUser, const char* sCommand, bool bFromPM = false) const -> bool;
    void RefreshTextFiles();
};
//---------------------------------------------------------------------------

#endif
