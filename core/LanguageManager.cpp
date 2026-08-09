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
#include "LanguageXml.h"
#include "LanguageStrings.h"
#include "LanguageManager.h"
//---------------------------------------------------------------------------
#include "SettingManager.h"
#include "ServerManager.h"
#include "utility.h"
#include <tinyxml2.h>
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

std::unique_ptr<LanguageManager> LanguageManager::m_Ptr;
//---------------------------------------------------------------------------

LanguageManager::LanguageManager()
{
    for (size_t szi = 0; szi < std::to_underlying(LangIds::LANG_IDS_END); szi++)
    {
        m_sTexts[szi] = LangStr[szi];
    }
}
//---------------------------------------------------------------------------

// LanguageManager destructor is = default in header
//---------------------------------------------------------------------------

void LanguageManager::Load()
{
    if (SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_LANGUAGE)].empty())
    {
        for (size_t szi = 0; szi < std::to_underlying(LangIds::LANG_IDS_END); szi++)
        {
            m_sTexts[szi] = LangStr[szi];
        }
    }
    else
    {
        const std::string sLanguageFile = ServerManager::m_sPath + "/language/" + SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_LANGUAGE)] + ".xml";

        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(sLanguageFile.c_str()) != tinyxml2::XML_SUCCESS)
        {
            if (doc.ErrorID() != tinyxml2::XML_ERROR_FILE_NOT_FOUND && doc.ErrorID() != tinyxml2::XML_ERROR_EMPTY_DOCUMENT)
            {
                LogXmlError((SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_LANGUAGE)] + ".xml").c_str(), doc.ErrorStr(), 0, 0);
            }
        }
        else
        {
            tinyxml2::XMLHandle cfg(&doc);
            tinyxml2::XMLNode* language = cfg.FirstChildElement("Language").ToNode();
            if (language)
            {
                tinyxml2::XMLElement* text = language->FirstChildElement();
                while (text)
                {
                    const char* sName = text->Attribute("Name");
                    const char* sText = text->GetText();
                    const size_t szLen = (sText ? strlen(sText) : 0);
                    if (szLen != 0 && szLen < 129)
                    {
                        for (size_t szi = 0; szi < std::to_underlying(LangIds::LANG_IDS_END); szi++) // NOLINT(modernize-loop-convert) index used in body
                        {
                            if (strcmp(LangXmlStr[szi], sName) == 0)
                            {
                                m_sTexts[szi].assign(sText, szLen);
                                break;
                            }
                        }
                    }
                    text = text->NextSiblingElement();
                }
            }
        }
    }
}

void LanguageManager::GenerateXmlExample()
{
    tinyxml2::XMLDocument xmldoc;
    xmldoc.InsertEndChild(xmldoc.NewDeclaration("version=\"1.0\" encoding=\"windows-1252\" standalone=\"yes\""));
    tinyxml2::XMLElement* xmllanguage = xmldoc.NewElement("Language");
    xmllanguage->SetAttribute("Name", "Example English Language");
    xmllanguage->SetAttribute("Author", "PtokaX");
    xmllanguage->SetAttribute("Version", PtokaXVersionString " build " BUILD_NUMBER);

    for (int i = 0; i < std::to_underlying(LangIds::LANG_IDS_END); i++)
    {
        tinyxml2::XMLElement* xmlstring = xmldoc.NewElement("String");
        xmlstring->SetAttribute("Name", LangXmlStr[i]);
        xmlstring->InsertEndChild(xmldoc.NewText(LangStr[i]));
        xmllanguage->InsertEndChild(xmlstring);
    }

    xmldoc.InsertEndChild(xmllanguage);
    xmldoc.SaveFile("English.xml.example");
}
//---------------------------------------------------------------------------
