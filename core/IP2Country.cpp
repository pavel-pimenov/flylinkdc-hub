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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <algorithm>
#include <fstream>
#include <string>
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "IP2Country.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "SettingManager.h"
#include "ServerManager.h"
#include "utility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// IP address byte shift constants for building uint32 from individual bytes
static constexpr uint32_t IP_BYTE3_SHIFT = 16777216; // 2^24
static constexpr uint32_t IP_BYTE2_SHIFT = 65536;    // 2^16
static constexpr uint32_t IP_BYTE1_SHIFT = 256;      // 2^8
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::unique_ptr<IpP2Country> IpP2Country::m_Ptr;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static constexpr size_t COUNTRY_COUNT = 254; // alex82 ... ����� ����� � ������

static const char* CountryNames[COUNTRY_COUNT] = {"Andorra", // NOLINT(modernize-avoid-c-arrays)
                                                  "United Arab Emirates",
                                                  "Afghanistan",
                                                  "Antigua and Barbuda",
                                                  "Anguilla",
                                                  "Albania",
                                                  "Armenia",
                                                  "Angola",
                                                  "Argentina",
                                                  "American Samoa",
                                                  "Austria",
                                                  "Australia",
                                                  "Aruba",
                                                  "Aland Islands",
                                                  "Azerbaijan",
                                                  "Bosnia and Herzegovina",
                                                  "Barbados",
                                                  "Bangladesh",
                                                  "Belgium",
                                                  "Burkina Faso",
                                                  "Bulgaria",
                                                  "Bahrain",
                                                  "Burundi",
                                                  "Benin",
                                                  "Saint Barthelemy",
                                                  "Bermuda",
                                                  "Brunei Darussalam",
                                                  "Bolivia",
                                                  "Brazil",
                                                  "Bahamas",
                                                  "Bhutan",
                                                  "Bouvet Island",
                                                  "Botswana",
                                                  "Belarus",
                                                  "Belize",
                                                  "Canada",
                                                  "Cocos (Keeling) Islands",
                                                  "The Democratic Republic of the Congo",
                                                  "Central African Republic",
                                                  "Congo",
                                                  "Switzerland",
                                                  "Cote D'Ivoire",
                                                  "Cook Islands",
                                                  "Chile",
                                                  "Cameroon",
                                                  "China",
                                                  "Colombia",
                                                  "Costa Rica",
                                                  "Serbia and Montenegro",
                                                  "Cuba",
                                                  "Cape Verde",
                                                  "Curacao",
                                                  "Christmas Island",
                                                  "Cyprus",
                                                  "Czech Republic",
                                                  "Germany",
                                                  "Djibouti",
                                                  "Denmark",
                                                  "Dominica",
                                                  "Dominican Republic",
                                                  "Algeria",
                                                  "Ecuador",
                                                  "Estonia",
                                                  "Egypt",
                                                  "Western Sahara",
                                                  "Eritrea",
                                                  "Spain",
                                                  "Ethiopia",
                                                  "Finland",
                                                  "Fiji",
                                                  "Falkland Islands (Malvinas)",
                                                  "Micronesia",
                                                  "Faroe Islands",
                                                  "France",
                                                  "Gabon",
                                                  "United Kingdom",
                                                  "Grenada",
                                                  "Georgia",
                                                  "French Guiana",
                                                  "Guernsey",
                                                  "Ghana",
                                                  "Gibraltar",
                                                  "Greenland",
                                                  "Gambia",
                                                  "Guinea",
                                                  "Guadeloupe",
                                                  "Equatorial Guinea",
                                                  "Greece",
                                                  "South Georgia and the South Sandwich Islands",
                                                  "Guatemala",
                                                  "Guam",
                                                  "Guinea-Bissau",
                                                  "Guyana",
                                                  "Hong Kong",
                                                  "Heard Island and McDonald Islands",
                                                  "Honduras",
                                                  "Croatia",
                                                  "Haiti",
                                                  "Hungary",
                                                  "Indonesia",
                                                  "Ireland",
                                                  "Israel",
                                                  "Isle of Man",
                                                  "India",
                                                  "British Indian Ocean Territory",
                                                  "Iraq",
                                                  "Iran",
                                                  "Iceland",
                                                  "Italy",
                                                  "Jersey",
                                                  "Jamaica",
                                                  "Jordan",
                                                  "Japan",
                                                  "Kenya",
                                                  "Kyrgyzstan",
                                                  "Cambodia",
                                                  "Kiribati",
                                                  "Comoros",
                                                  "Saint Kitts and Nevis",
                                                  "Democratic People's Republic of Korea",
                                                  "Republic of Korea",
                                                  "Kuwait",
                                                  "Cayman Islands",
                                                  "Kazakhstan",
                                                  "Lao People's Democratic Republic",
                                                  "Lebanon",
                                                  "Saint Lucia",
                                                  "Liechtenstein",
                                                  "Sri Lanka",
                                                  "Liberia",
                                                  "Lesotho",
                                                  "Lithuania",
                                                  "Luxembourg",
                                                  "Latvia",
                                                  "Libyan Arab Jamahiriya",
                                                  "Morocco",
                                                  "Monaco",
                                                  "Moldova",
                                                  "Montenegro",
                                                  "Saint Martin",
                                                  "Madagascar",
                                                  "Marshall Islands",
                                                  "Macedonia",
                                                  "Mali",
                                                  "Myanmar",
                                                  "Mongolia",
                                                  "Macao",
                                                  "Northern Mariana Islands",
                                                  "Martinique",
                                                  "Mauritania",
                                                  "Montserrat",
                                                  "Malta",
                                                  "Mauritius",
                                                  "Maldives",
                                                  "Malawi",
                                                  "Mexico",
                                                  "Malaysia",
                                                  "Mozambique",
                                                  "Namibia",
                                                  "New Caledonia",
                                                  "Niger",
                                                  "Norfolk Island",
                                                  "Nigeria",
                                                  "Nicaragua",
                                                  "Netherlands",
                                                  "Norway",
                                                  "Nepal",
                                                  "Nauru",
                                                  "Niue",
                                                  "New Zealand",
                                                  "Oman",
                                                  "Panama",
                                                  "Peru",
                                                  "French Polynesia",
                                                  "Papua New Guinea",
                                                  "Philippines",
                                                  "Pakistan",
                                                  "Poland",
                                                  "Saint Pierre and Miquelon",
                                                  "Pitcairn",
                                                  "Puerto Rico",
                                                  "Palestinian Territory",
                                                  "Portugal",
                                                  "Palau",
                                                  "Paraguay",
                                                  "Qatar",
                                                  "Reunion",
                                                  "Romania",
                                                  "Serbia",
                                                  "Russian Federation",
                                                  "Rwanda",
                                                  "Saudi Arabia",
                                                  "Solomon Islands",
                                                  "Seychelles",
                                                  "Sudan",
                                                  "Sweden",
                                                  "Singapore",
                                                  "Saint Helena",
                                                  "Slovenia",
                                                  "Svalbard and Jan Mayen",
                                                  "Slovakia",
                                                  "Sierra Leone",
                                                  "San Marino",
                                                  "Senegal",
                                                  "Somalia",
                                                  "Suriname",
                                                  "South Sudan",
                                                  "Sao Tome and Principe",
                                                  "El Salvador",
                                                  "Sint Maarten (Dutch Part)",
                                                  "Syrian Arab Republic",
                                                  "Swaziland",
                                                  "Turks and Caicos Islands",
                                                  "Chad",
                                                  "French Southern Territories",
                                                  "Togo",
                                                  "Thailand",
                                                  "Tajikistan",
                                                  "Tokelau",
                                                  "Timor-Leste",
                                                  "Turkmenistan",
                                                  "Tunisia",
                                                  "Tonga",
                                                  "Turkey",
                                                  "Trinidad and Tobago",
                                                  "Tuvalu",
                                                  "Taiwan",
                                                  "Tanzania",
                                                  "Ukraine",
                                                  "Uganda",
                                                  "United States Minor Outlying Islands",
                                                  "United States",
                                                  "Uruguay",
                                                  "Uzbekistan",
                                                  "Holy See (Vatican City State)",
                                                  "Saint Vincent and the Grenadines",
                                                  "Venezuela",
                                                  "Virgin Islands, British",
                                                  "Virgin Islands, U.S.",
                                                  "Viet Nam",
                                                  "Vanuatu",
                                                  "Wallis and Futuna",
                                                  "Samoa",
                                                  "Yemen",
                                                  "Mayotte",
                                                  "South Africa",
                                                  "Zambia",
                                                  "Zimbabwe",
                                                  "Netherlands Antilles",
                                                  "Unknown (Asia-Pacific)",
                                                  "Unknown (Local network)",
                                                  "Unknown (European Union)",
                                                  "Unknown (Reserved)",
                                                  "Unknown"};

static const char* CountryNamesRussian[COUNTRY_COUNT] = {"�������", // NOLINT(modernize-avoid-c-arrays)
                                                         "���",
                                                         "����������",
                                                         "������� � �������",
                                                         "�������",
                                                         "�������",
                                                         "�������",
                                                         "������",
                                                         "���������",
                                                         "������������ �����",
                                                         "�������",
                                                         "���������",
                                                         "�����",
                                                         "��������� �������",
                                                         "�����������",
                                                         "������ � �����������",
                                                         "��������",
                                                         "���������",
                                                         "�������",
                                                         "������� ����",
                                                         "��������",
                                                         "�������",
                                                         "�������",
                                                         "�����",
                                                         "���-���������",
                                                         "�������",
                                                         "������",
                                                         "�������",
                                                         "��������",
                                                         "������",
                                                         "�����",
                                                         "������ ����",
                                                         "��������",
                                                         "��������",
                                                         "�����",
                                                         "������",
                                                         "��������� �������",
                                                         "�� �����",
                                                         "���",
                                                         "���������� �����",
                                                         "���������",
                                                         "���-������",
                                                         "������� ����",
                                                         "����",
                                                         "�������",
                                                         "�����",
                                                         "��������",
                                                         "�����-����",
                                                         "������ � ����������",
                                                         "����",
                                                         "����-�����",
                                                         "�������",
                                                         "������ ���������",
                                                         "����",
                                                         "�����",
                                                         "��������",
                                                         "�������",
                                                         "�����",
                                                         "��������",
                                                         "������������� ����������",
                                                         "�����",
                                                         "�������",
                                                         "�������",
                                                         "������",
                                                         "�������� ������",
                                                         "�������",
                                                         "�������",
                                                         "�������",
                                                         "���������",
                                                         "�����",
                                                         "������������ �������",
                                                         "����������",
                                                         "��������� �������",
                                                         "�������",
                                                         "�����",
                                                         "��������������",
                                                         "�������",
                                                         "������",
                                                         "����������� ������",
                                                         "������",
                                                         "����",
                                                         "���������",
                                                         "����������",
                                                         "������",
                                                         "������",
                                                         "���������",
                                                         "�������������� ������",
                                                         "������",
                                                         "����� ������� � ����� ���������� �������",
                                                         "���������",
                                                         "����",
                                                         "������-�����",
                                                         "������",
                                                         "�������",
                                                         "���� � ����������",
                                                         "��������",
                                                         "��������",
                                                         "�����",
                                                         "�������",
                                                         "���������",
                                                         "��������",
                                                         "�������",
                                                         "������ ���",
                                                         "�����",
                                                         "���������� ���������� � ��������� ������",
                                                         "����",
                                                         "����",
                                                         "��������",
                                                         "������",
                                                         "������",
                                                         "������",
                                                         "��������",
                                                         "������",
                                                         "�����",
                                                         "��������",
                                                         "��������",
                                                         "��������",
                                                         "��������� �������",
                                                         "����-����� � �����",
                                                         "����",
                                                         "���������� �����",
                                                         "������",
                                                         "��������� �������",
                                                         "���������",
                                                         "����",
                                                         "�����",
                                                         "����-�����",
                                                         "�����������",
                                                         "���-�����",
                                                         "�������",
                                                         "������",
                                                         "�����",
                                                         "����������",
                                                         "������",
                                                         "�����",
                                                         "�������",
                                                         "������",
                                                         "�������",
                                                         "����������",
                                                         "���-������",
                                                         "����������",
                                                         "���������� �������",
                                                         "���������",
                                                         "����",
                                                         "������",
                                                         "��������",
                                                         "������",
                                                         "�������� ���������� �������",
                                                         "���������",
                                                         "����������",
                                                         "����������",
                                                         "������",
                                                         "��������",
                                                         "��������",
                                                         "������",
                                                         "�������",
                                                         "��������",
                                                         "��������",
                                                         "�������",
                                                         "����� ���������",
                                                         "�����",
                                                         "������ �������",
                                                         "�������",
                                                         "���������",
                                                         "����������",
                                                         "��������",
                                                         "�����",
                                                         "�����",
                                                         "����",
                                                         "����� ��������",
                                                         "����",
                                                         "������",
                                                         "����",
                                                         "����������� ���������",
                                                         "����� � ����� ������",
                                                         "���������",
                                                         "��������",
                                                         "������",
                                                         "���-���� � �������",
                                                         "������� �������",
                                                         "������-����",
                                                         "���������",
                                                         "����������",
                                                         "�����",
                                                         "��������",
                                                         "�����",
                                                         "�������",
                                                         "�������",
                                                         "������",
                                                         "������",
                                                         "������",
                                                         "���������� ������",
                                                         "���������� �������",
                                                         "����������� �������",
                                                         "�����",
                                                         "������",
                                                         "��������",
                                                         "������ ������ �����",
                                                         "��������",
                                                         "���������� � ��-�����",
                                                         "��������",
                                                         "������-�����",
                                                         "���-������",
                                                         "�������",
                                                         "������",
                                                         "�������",
                                                         "����� �����",
                                                         "���-���� � ��������",
                                                         "���������",
                                                         "����-������",
                                                         "�����",
                                                         "���������",
                                                         "Ҹ��� � ������",
                                                         "���",
                                                         "����������� ����� � �������������� ����������",
                                                         "����",
                                                         "�������",
                                                         "�����������",
                                                         "�������",
                                                         "��������� �����",
                                                         "���������",
                                                         "�����",
                                                         "�����",
                                                         "������",
                                                         "�������� � ������",
                                                         "������",
                                                         "�������",
                                                         "��������",
                                                         "�������",
                                                         "������",
                                                         "������� ����� ������� (���)",
                                                         "���",
                                                         "�������",
                                                         "����������",
                                                         "�������",
                                                         "����-������� � ���������",
                                                         "���������",
                                                         "���������� ���������� �������",
                                                         "������������ ���������� �������",
                                                         "�������",
                                                         "�������",
                                                         "������ � ������",
                                                         "�����",
                                                         "�����",
                                                         "�������",
                                                         "���",
                                                         "������",
                                                         "��������",
                                                         "������������� ���������� �������",
                                                         "���������� (��������-������������� ������)",
                                                         "���������� (��������� ����)",
                                                         "���������� (����������� ����)",
                                                         "���������� (���������������)",
                                                         "����������"};
// alex82 ... last updated 23 dec 2014
static const char* CountryCodes[COUNTRY_COUNT] = { // NOLINT(modernize-avoid-c-arrays)
    "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AO", "AR", "AS", "AT", "AU", "AW", "AX", "AZ", "BA", "BB", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BL", "BM",
    "BN", "BO", "BR", "BS", "BT", "BV", "BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG", "CH", "CI", "CK", "CL", "CM", "CN", "CO", "CR", "CS", "CU", "CV", "CW",
    "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO", "DZ", "EC", "EE", "EG", "EH", "ER", "ES", "ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB", "GD", "GE",
    "GF", "GG", "GH", "GI", "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", "HM", "HN", "HR", "HT", "HU", "ID", "IE", "IL", "IM", "IN",
    "IO", "IQ", "IR", "IS", "IT", "JE", "JM", "JO", "JP", "KE", "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", "KZ", "LA", "LB", "LC", "LI", "LK", "LR",
    "LS", "LT", "LU", "LV", "LY", "MA", "MC", "MD", "ME", "MF", "MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP", "MQ", "MR", "MS", "MT", "MU", "MV", "MW", "MX",
    "MY", "MZ", "NA", "NC", "NE", "NF", "NG", "NI", "NL", "NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN", "PR", "PS",
    "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU", "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", "SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "SS", "ST",
    "SV", "SX", "SY", "SZ", "TC", "TD", "TF", "TG", "TH", "TJ", "TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", "UM", "US", "UY", "UZ",
    "VA", "VC", "VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YT", "ZA", "ZM", "ZW", "AN", "AP", "LN", "EU", "ZZ", "??"};
namespace {
const char* TranslateCountry(unsigned p_index)
{
    if (p_index < std::size(CountryNamesRussian))
    {
        if (!SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_LANGUAGE)].empty() &&
            iequals(SettingManager::m_Ptr->m_sTexts[std::to_underlying(SetTxtIds::SETTXT_LANGUAGE)], "Russian"))
        {
            return CountryNamesRussian[p_index];
        }
        return CountryNames[p_index];
    }
    return "Error country index";
}
} // namespace
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::LoadIPv4()
{
    if (!ServerManager::m_bUseIPv4)
    {
        return;
    }

    std::ifstream ip2country(ServerManager::m_sPath + "/cfg/IpToCountry.csv");

    if (!ip2country.is_open())
    {
        return;
    }

    if (m_ui32RangeFrom.empty())
    {
        m_ui32RangeFrom.resize(PTOKAX_GLOBAL_BUFF_SIZE);
        m_ui32RangeTo.resize(PTOKAX_GLOBAL_BUFF_SIZE);
        m_ui8RangeCI.resize(PTOKAX_GLOBAL_BUFF_SIZE);
    }

    std::string sLine;

    while (std::getline(ip2country, sLine))
    {
        if (sLine.empty() || sLine[0] != '\"')
        {
            continue;
        }

        if (m_ui32Count == static_cast<uint32_t>(m_ui32RangeFrom.size()))
        {
            const auto szNewSize = m_ui32RangeFrom.size() + 512;
            m_ui32RangeFrom.resize(szNewSize);
            m_ui32RangeTo.resize(szNewSize);
            m_ui8RangeCI.resize(szNewSize);
        }

        const char* sStart = sLine.data() + 1;
        uint8_t ui8d = 0;

        for (size_t szi = 1; szi < sLine.size(); szi++)
        {
            if (sLine[szi] == '\"')
            {
                sLine[szi] = '\0';
                if (ui8d == 0)
                {
                    std::from_chars(sStart, sStart + strlen(sStart), m_ui32RangeFrom[m_ui32Count]);
                }
                else if (ui8d == 1)
                {
                    std::from_chars(sStart, sStart + strlen(sStart), m_ui32RangeTo[m_ui32Count]);
                }
                else if (ui8d == 4)
                {
                    for (uint8_t ui8i = 0; ui8i < COUNTRY_COUNT - 1; ui8i++)
                    {
                        if (memcmp(CountryCodes[ui8i], sStart, 2) == 0)
                        {
                            m_ui8RangeCI[m_ui32Count] = ui8i;
                            m_ui32Count++;
                            break;
                        }
                    }

                    break;
                }

                ui8d++;
                szi += 2;
                sStart = sLine.data() + szi + 1;
            }
        }
    }

    if (m_ui32Count < static_cast<uint32_t>(m_ui32RangeFrom.size()))
    {
        m_ui32RangeFrom.resize(m_ui32Count);
        m_ui32RangeTo.resize(m_ui32Count);
        m_ui8RangeCI.resize(m_ui32Count);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::LoadIPv6()
{
#ifdef FLYLINKDC_USE_IP_TO_COUNTRY_V6
    if (!ServerManager::m_bUseIPv6)
    {
        return;
    }

    std::ifstream ip2country(ServerManager::m_sPath + "/cfg/IpToCountry.6R.csv");

    if (!ip2country.is_open())
    {
        return;
    }

    if (m_ui128IPv6RangeFrom.empty())
    {
        try
        {
            m_ui128IPv6RangeFrom.resize(16384 * 16, 0);
            m_ui128IPv6RangeTo.resize(16384 * 16, 0);
            m_ui8IPv6RangeCI.resize(16384, 0);
        }
        catch (const std::bad_alloc&)
        {
            LogDbg("[MEM] Cannot create IpP2Country IPv6 buffers");
            return;
        }
    }

    std::string sLine;

    while (std::getline(ip2country, sLine))
    {
        if (sLine.empty() || sLine.starts_with('#') || sLine[0] < 32)
        {
            continue;
        }

        if (m_ui32IPv6Count == static_cast<uint32_t>(m_ui8IPv6RangeCI.size()))
        {
            const auto szNewEntries = m_ui8IPv6RangeCI.size() + 512;
            try
            {
                m_ui128IPv6RangeFrom.resize(szNewEntries * 16);
                m_ui128IPv6RangeTo.resize(szNewEntries * 16);
                m_ui8IPv6RangeCI.resize(szNewEntries);
            }
            catch (const std::bad_alloc&)
            {
                LogDbgErr("[MEM] Cannot reallocate IPv6 buffers in IpP2Country for {} entries", static_cast<unsigned>(szNewEntries));
                return;
            }
        }

        const char* sStart = sLine.data();
        uint8_t ui8d = 0;

        for (size_t szi = 0; szi < sLine.size(); szi++)
        {
            if (ui8d == 0 && sLine[szi] == '-')
            {
                sLine[szi] = '\0';
                inet_pton(AF_INET6, sStart, m_ui128IPv6RangeFrom.data() + (m_ui32IPv6Count * 16));
            }
            else if (sLine[szi] == ',')
            {
                sLine[szi] = '\0';
                if (ui8d == 1)
                {
                    inet_pton(AF_INET6, sStart, m_ui128IPv6RangeTo.data() + (m_ui32IPv6Count * 16));
                }
                else
                {
                    for (uint8_t ui8i = 0; ui8i < COUNTRY_COUNT - 1; ui8i++)
                    {
                        if (memcmp(CountryCodes[ui8i], sStart, 2) == 0)
                        {
                            m_ui8IPv6RangeCI[m_ui32IPv6Count] = ui8i;
                            m_ui32IPv6Count++;

                            break;
                        }
                    }

                    break;
                }
            }
            else
            {
                continue;
            }

            ui8d++;
            sStart = sLine.data() + szi + 1;
        }
    }

    if (m_ui32IPv6Count < static_cast<uint32_t>(m_ui8IPv6RangeCI.size()))
    {
        m_ui128IPv6RangeFrom.resize(m_ui32IPv6Count * 16);
        m_ui128IPv6RangeTo.resize(m_ui32IPv6Count * 16);
        m_ui8IPv6RangeCI.resize(m_ui32IPv6Count);
    }
#else
    LogDbg("[SYS] Disable load IpToCountry.6R.csv");
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

IpP2Country::IpP2Country()
{
    LoadIPv4();
    LoadIPv6();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// IpP2Country destructor is = default in header
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const char* IpP2Country::Find(const uint8_t* ui128IpHash, const bool bCountryName) const
{
    bool bIPv4 = false;
    uint32_t ui32IpHash = 0;

    if (!ServerManager::m_bUseIPv6 || IsV4Mapped(ui128IpHash))
    {
        bIPv4 = true;

        ui32IpHash = IP_BYTE3_SHIFT * ui128IpHash[12] + IP_BYTE2_SHIFT * ui128IpHash[13] + IP_BYTE1_SHIFT * ui128IpHash[14] + ui128IpHash[15];
    }

    if (!bIPv4)
    {
        if (ui128IpHash[0] == 32 && ui128IpHash[1] == 2) // 6to4 tunnel
        {
            bIPv4 = true;

            ui32IpHash = IP_BYTE3_SHIFT * ui128IpHash[2] + IP_BYTE2_SHIFT * ui128IpHash[3] + IP_BYTE1_SHIFT * ui128IpHash[4] + ui128IpHash[5];
        }
        else if (ui128IpHash[0] == 32 && ui128IpHash[1] == 1 && ui128IpHash[2] == 0 && ui128IpHash[3] == 0) // teredo tunnel
        {
            bIPv4 = true;

            ui32IpHash =
                (IP_BYTE3_SHIFT * ui128IpHash[12] + IP_BYTE2_SHIFT * ui128IpHash[13] + IP_BYTE1_SHIFT * ui128IpHash[14] + ui128IpHash[15]) ^ 0xffffffff;
        }
    }

    if (bIPv4)
    {
        // Binary search: find the last range where RangeFrom <= ui32IpHash
        const auto it = std::upper_bound(m_ui32RangeFrom.begin(), m_ui32RangeFrom.begin() + m_ui32Count, ui32IpHash);
        if (it != m_ui32RangeFrom.begin())
        {
            const auto idx = static_cast<uint32_t>(it - m_ui32RangeFrom.begin() - 1);
            if (m_ui32RangeTo[idx] >= ui32IpHash)
            {
                if (!bCountryName)
                {
                    return CountryCodes[m_ui8RangeCI[idx]];
                }
                return TranslateCountry(m_ui8RangeCI[idx]);
            }
        }
    }
    else
    {
        // Binary search for IPv6 ranges (128-bit addresses stored as flat byte arrays)
        uint32_t lo = 0, hi = m_ui32IPv6Count;
        while (lo < hi)
        {
            const uint32_t mid = lo + (hi - lo) / 2;
            if (memcmp(m_ui128IPv6RangeFrom.data() + (mid * 16), ui128IpHash, 16) <= 0)
                lo = mid + 1;
            else
                hi = mid;
        }
        if (lo > 0)
        {
            const auto idx = lo - 1;
            if (memcmp(m_ui128IPv6RangeTo.data() + (idx * 16), ui128IpHash, 16) >= 0)
            {
                if (!bCountryName)
                {
                    return CountryCodes[m_ui8IPv6RangeCI[idx]];
                }
                return CountryNames[m_ui8IPv6RangeCI[idx]];
            }
        }
    }

    if (!bCountryName)
    {
        return CountryCodes[COUNTRY_COUNT - 1];
    }
    return TranslateCountry(COUNTRY_COUNT - 1);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint8_t IpP2Country::Find(const uint8_t* ui128IpHash) const
{
    bool bIPv4 = false;
    uint32_t ui32IpHash = 0;

    in6_addr addr;
    memcpy(&addr, ui128IpHash, sizeof(in6_addr));
    if (!ServerManager::m_bUseIPv6 || IN6_IS_ADDR_V4MAPPED(&addr))
    {
        bIPv4 = true;

        ui32IpHash = IP_BYTE3_SHIFT * ui128IpHash[12] + IP_BYTE2_SHIFT * ui128IpHash[13] + IP_BYTE1_SHIFT * ui128IpHash[14] + ui128IpHash[15];
    }

    if (!bIPv4)
    {
        if (ui128IpHash[0] == 32 && ui128IpHash[1] == 2) // 6to4 tunnel
        {
            bIPv4 = true;

            ui32IpHash = IP_BYTE3_SHIFT * ui128IpHash[2] + IP_BYTE2_SHIFT * ui128IpHash[3] + IP_BYTE1_SHIFT * ui128IpHash[4] + ui128IpHash[5];
        }
        else if (ui128IpHash[0] == 32 && ui128IpHash[1] == 1 && ui128IpHash[2] == 0 && ui128IpHash[3] == 0) // teredo tunnel
        {
            bIPv4 = true;

            ui32IpHash =
                (IP_BYTE3_SHIFT * ui128IpHash[12] + IP_BYTE2_SHIFT * ui128IpHash[13] + IP_BYTE1_SHIFT * ui128IpHash[14] + ui128IpHash[15]) ^ 0xffffffff;
        }
    }

    if (bIPv4)
    {
        const auto it = std::upper_bound(m_ui32RangeFrom.begin(), m_ui32RangeFrom.begin() + m_ui32Count, ui32IpHash);
        if (it != m_ui32RangeFrom.begin())
        {
            const auto idx = static_cast<uint32_t>(it - m_ui32RangeFrom.begin() - 1);
            if (m_ui32RangeTo[idx] >= ui32IpHash)
            {
                return m_ui8RangeCI[idx];
            }
        }
    }
    else
    {
        // Binary search for IPv6 ranges
        uint32_t lo = 0, hi = m_ui32IPv6Count;
        while (lo < hi)
        {
            const uint32_t mid = lo + (hi - lo) / 2;
            if (memcmp(m_ui128IPv6RangeFrom.data() + (mid * 16), ui128IpHash, 16) <= 0)
                lo = mid + 1;
            else
                hi = mid;
        }
        if (lo > 0)
        {
            const auto idx = lo - 1;
            if (memcmp(m_ui128IPv6RangeTo.data() + (idx * 16), ui128IpHash, 16) >= 0)
            {
                return m_ui8IPv6RangeCI[idx];
            }
        }
    }

    return COUNTRY_COUNT - 1;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const char* IpP2Country::GetCountry(const uint8_t ui8dx, const bool bCountryName)
{
    if (!bCountryName)
    {
        return CountryCodes[ui8dx];
    }
    return TranslateCountry(ui8dx);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// alex82 ... ���������� ������ �� ����
const char* IpP2Country::GetCountryName(const char* sCode)
{
    for (uint8_t ui8i = 0; ui8i < COUNTRY_COUNT; ui8i++)
    {
        if (strcasecmp(CountryCodes[ui8i], sCode) == 0)
        {
            return TranslateCountry(ui8i);
        }
    }
    return TranslateCountry(COUNTRY_COUNT - 1);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::Reload()
{
    m_ui32Count = 0;
    LoadIPv4();

    m_ui32IPv6Count = 0;
    LoadIPv6();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
