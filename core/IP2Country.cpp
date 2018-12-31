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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "IP2Country.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "SettingManager.h"
#include "ServerManager.h"
#include "utility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
IpP2Country * IpP2Country::m_Ptr = nullptr;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define COUNTRY_COUNT   254 // alex82 ... Число стран в списке

static const char * CountryNames[COUNTRY_COUNT] =
{
	"Andorra", "United Arab Emirates", "Afghanistan", "Antigua and Barbuda", "Anguilla", "Albania", "Armenia", "Angola",
	"Argentina", "American Samoa", "Austria", "Australia", "Aruba", "Aland Islands", "Azerbaijan", "Bosnia and Herzegovina",
	"Barbados", "Bangladesh", "Belgium", "Burkina Faso", "Bulgaria", "Bahrain", "Burundi", "Benin", "Saint Barthelemy",
	"Bermuda", "Brunei Darussalam", "Bolivia", "Brazil", "Bahamas", "Bhutan", "Bouvet Island", "Botswana", "Belarus",
	"Belize", "Canada", "Cocos (Keeling) Islands", "The Democratic Republic of the Congo", "Central African Republic",
	"Congo", "Switzerland", "Cote D'Ivoire", "Cook Islands", "Chile", "Cameroon", "China", "Colombia", "Costa Rica",
	"Serbia and Montenegro", "Cuba", "Cape Verde", "Curacao", "Christmas Island", "Cyprus", "Czech Republic", "Germany",
	"Djibouti", "Denmark", "Dominica", "Dominican Republic", "Algeria", "Ecuador", "Estonia", "Egypt", "Western Sahara",
	"Eritrea", "Spain", "Ethiopia", "Finland", "Fiji", "Falkland Islands (Malvinas)", "Micronesia", "Faroe Islands",
	"France", "Gabon", "United Kingdom", "Grenada", "Georgia", "French Guiana", "Guernsey", "Ghana", "Gibraltar", "Greenland",
	"Gambia", "Guinea", "Guadeloupe", "Equatorial Guinea", "Greece", "South Georgia and the South Sandwich Islands",
	"Guatemala", "Guam", "Guinea-Bissau", "Guyana", "Hong Kong", "Heard Island and McDonald Islands", "Honduras", "Croatia",
	"Haiti", "Hungary", "Indonesia", "Ireland", "Israel", "Isle of Man", "India", "British Indian Ocean Territory", "Iraq",
	"Iran", "Iceland", "Italy", "Jersey", "Jamaica", "Jordan", "Japan", "Kenya", "Kyrgyzstan", "Cambodia", "Kiribati",
	"Comoros", "Saint Kitts and Nevis", "Democratic People's Republic of Korea", "Republic of Korea", "Kuwait",
	"Cayman Islands", "Kazakhstan", "Lao People's Democratic Republic", "Lebanon", "Saint Lucia", "Liechtenstein",
	"Sri Lanka", "Liberia", "Lesotho", "Lithuania", "Luxembourg", "Latvia", "Libyan Arab Jamahiriya", "Morocco", "Monaco",
	"Moldova", "Montenegro", "Saint Martin", "Madagascar", "Marshall Islands", "Macedonia", "Mali", "Myanmar", "Mongolia",
	"Macao", "Northern Mariana Islands", "Martinique", "Mauritania", "Montserrat", "Malta", "Mauritius", "Maldives", "Malawi",
	"Mexico", "Malaysia", "Mozambique", "Namibia", "New Caledonia", "Niger", "Norfolk Island", "Nigeria", "Nicaragua",
	"Netherlands", "Norway", "Nepal", "Nauru", "Niue", "New Zealand", "Oman", "Panama", "Peru", "French Polynesia",
	"Papua New Guinea", "Philippines", "Pakistan", "Poland", "Saint Pierre and Miquelon", "Pitcairn", "Puerto Rico",
	"Palestinian Territory", "Portugal", "Palau", "Paraguay", "Qatar", "Reunion", "Romania", "Serbia", "Russian Federation",
	"Rwanda", "Saudi Arabia", "Solomon Islands", "Seychelles", "Sudan", "Sweden", "Singapore", "Saint Helena", "Slovenia",
	"Svalbard and Jan Mayen", "Slovakia", "Sierra Leone", "San Marino", "Senegal", "Somalia", "Suriname", "South Sudan",
	"Sao Tome and Principe", "El Salvador", "Sint Maarten (Dutch Part)", "Syrian Arab Republic", "Swaziland",
	"Turks and Caicos Islands", "Chad", "French Southern Territories", "Togo", "Thailand", "Tajikistan", "Tokelau",
	"Timor-Leste", "Turkmenistan", "Tunisia", "Tonga", "Turkey", "Trinidad and Tobago", "Tuvalu", "Taiwan", "Tanzania",
	"Ukraine", "Uganda", "United States Minor Outlying Islands", "United States", "Uruguay", "Uzbekistan",
	"Holy See (Vatican City State)", "Saint Vincent and the Grenadines", "Venezuela", "Virgin Islands, British",
	"Virgin Islands, U.S.", "Viet Nam", "Vanuatu", "Wallis and Futuna", "Samoa", "Yemen", "Mayotte", "South Africa",
	"Zambia", "Zimbabwe", "Netherlands Antilles", "Unknown (Asia-Pacific)", "Unknown (Local network)",
	"Unknown (European Union)", "Unknown (Reserved)", "Unknown"
};

static const char * CountryNamesRussian[COUNTRY_COUNT] =
{
	"Андорра", "ОАЭ", "Афганистан", "Антигуа и Барбуда", "Ангилья", "Албания", "Армения", "Ангола", "Аргентина",
	"Американское Самоа", "Австрия", "Австралия", "Аруба", "Аландские острова", "Азербайджан", "Босния и Герцеговина",
	"Барбадос", "Бангладеш", "Бельгия", "Буркина Фасо", "Болгария", "Бахрейн", "Бурунди", "Бенин", "Сен-Бартельми",
	"Бермуды", "Бруней", "Боливия", "Бразилия", "Багамы", "Бутан", "Остров Буве", "Ботсвана", "Беларусь", "Белиз",
	"Канада", "Кокосовые острова", "ДР Конго", "ЦАР", "Республика Конго", "Швейцария", "Кот-д’Ивуар", "Острова Кука",
	"Чили", "Камерун", "Китай", "Колумбия", "Коста-Рика", "Сербия и Черногория", "Куба", "Кабо-Верде", "Кюрасао",
	"Остров Рождества", "Кипр", "Чехия", "Германия", "Джибути", "Дания", "Доминика", "Доминиканская Республика", "Алжир",
	"Эквадор", "Эстония", "Египет", "Западная Сахара", "Эритрея", "Испания", "Эфиопия", "Финляндия", "Фиджи",
	"Фолклендские острова", "Микронезия", "Фарерские острова", "Франция", "Габон", "Великобритания", "Гренада",
	"Грузия", "Французская Гвиана", "Гернси", "Гана", "Гибралтар", "Гренландия", "Гамбия", "Гвинея", "Гваделупа",
	"Экваториальная Гвинея", "Греция", "Южная Георгия и Южные Сандвичевы острова", "Гватемала", "Гуам", "Гвинея-Бисау",
	"Гайана", "Гонконг", "Херд и Макдональд", "Гондурас", "Хорватия", "Гаити", "Венгрия", "Индонезия", "Ирландия",
	"Израиль", "Остров Мэн", "Индия", "Британские территории в Индийском океане", "Ирак", "Иран", "Исландия", "Италия",
	"Джерси", "Ямайка", "Иордания", "Япония", "Кения", "Киргизия", "Камбоджа", "Кирибати", "Коморские Острова",
	"Сент-Киттс и Невис", "КНДР", "Республика Корея", "Кувейт", "Каймановы острова", "Казахстан", "Лаос", "Ливан",
	"Сент-Люсия", "Лихтенштейн", "Шри-Ланка", "Либерия", "Лесото", "Литва", "Люксембург", "Латвия", "Ливия", "Марокко",
	"Монако", "Молдова", "Черногория", "Сен-Мартен", "Мадагаскар", "Маршалловы Острова", "Македония", "Мали", "Мьянма",
	"Монголия", "Аомынь", "Северные Марианские острова", "Мартиника", "Мавритания", "Монтсеррат", "Мальта", "Маврикий",
	"Мальдивы", "Малави", "Мексика", "Малайзия", "Мозамбик", "Намибия", "Новая Каледония", "Нигер", "Остров Норфолк",
	"Нигерия", "Никарагуа", "Нидерланды", "Норвегия", "Непал", "Науру", "Ниуэ", "Новая Зеландия", "Оман", "Панама", "Перу",
	"Французская Полинезия", "Папуа — Новая Гвинея", "Филиппины", "Пакистан", "Польша", "Сен-Пьер и Микелон",
	"Острова Питкэрн", "Пуэрто-Рико", "Палестина", "Португалия", "Палау", "Парагвай", "Катар", "Реюньон", "Румыния",
	"Сербия", "Россия", "Руанда", "Саудовская Аравия", "Соломоновы Острова", "Сейшельские Острова", "Судан", "Швеция",
	"Сингапур", "Остров Святой Елены", "Словения", "Шпицберген и Ян-Майен", "Словакия", "Сьерра-Леоне", "Сан-Марино",
	"Сенегал", "Сомали", "Суринам", "Южный Судан", "Сан-Томе и Принсипи", "Сальвадор", "Синт-Мартен", "Сирия", "Свазиленд",
	"Тёркс и Кайкос", "Чад", "Французские Южные и Антарктические Территории", "Того", "Таиланд", "Таджикистан", "Токелау",
	"Восточный Тимор", "Туркмения", "Тунис", "Тонга", "Турция", "Тринидад и Тобаго", "Тувалу", "Тайвань", "Танзания",
	"Украина", "Уганда", "Внешние малые острова (США)", "США", "Уругвай", "Узбекистан", "Ватикан", "Сент-Винсент и Гренадины",
	"Венесуэла", "Британские Виргинские острова", "Американские Виргинские острова", "Вьетнам", "Вануату", "Уоллис и Футуна",
	"Самоа", "Йемен", "Майотта", "ЮАР", "Замбия", "Зимбабве", "Нидерландские Антильские острова",
	"Неизвестно (Азиатско-Тихоокеанский регион)", "Неизвестно (Локальная сеть)", "Неизвестно (Европейский Союз)",
	"Неизвестно (Зарезервировано)", "Неизвестно"
};
// alex82 ... last updated 23 dec 2014
static const char * CountryCodes[COUNTRY_COUNT] =
{
	"AD", "AE", "AF", "AG", "AI", "AL", "AM", "AO", "AR", "AS", "AT", "AU", "AW", "AX", "AZ", "BA", "BB", "BD", "BE", "BF",
	"BG", "BH", "BI", "BJ", "BL", "BM", "BN", "BO", "BR", "BS", "BT", "BV", "BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG",
	"CH", "CI", "CK", "CL", "CM", "CN", "CO", "CR", "CS", "CU", "CV", "CW", "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO",
	"DZ", "EC", "EE", "EG", "EH", "ER", "ES", "ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB", "GD", "GE", "GF", "GG",
	"GH", "GI", "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", "HM", "HN", "HR", "HT", "HU", "ID",
	"IE", "IL", "IM", "IN", "IO", "IQ", "IR", "IS", "IT", "JE", "JM", "JO", "JP", "KE", "KG", "KH", "KI", "KM", "KN", "KP",
	"KR", "KW", "KY", "KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT", "LU", "LV", "LY", "MA", "MC", "MD", "ME", "MF",
	"MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP", "MQ", "MR", "MS", "MT", "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC",
	"NE", "NF", "NG", "NI", "NL", "NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN",
	"PR", "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU", "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", "SI", "SJ",
	"SK", "SL", "SM", "SN", "SO", "SR", "SS", "ST", "SV", "SX", "SY", "SZ", "TC", "TD", "TF", "TG", "TH", "TJ", "TK", "TL",
	"TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", "UM", "US", "UY", "UZ", "VA", "VC", "VE", "VG", "VI", "VN",
	"VU", "WF", "WS", "YE", "YT", "ZA", "ZM", "ZW", "AN", "AP", "LN", "EU", "ZZ", "??"
};
static const char* TranslateCountry(unsigned p_index)
{
	if (p_index < sizeof(CountryNamesRussian) / sizeof(CountryNamesRussian[0]))
	{
		if (SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE] != NULL && strcasecmp(SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE], "Russian") == 0)
			return CountryNamesRussian[p_index];
		else
			return CountryNames[p_index];
	}
	else
	{
		return "Error country index";
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::LoadIPv4()
{
	if (ServerManager::m_bUseIPv4 == false)
	{
		return;
	}
	
#ifdef _WIN32
	FILE * ip2country = fopen((ServerManager::m_sPath + "\\cfg\\IpToCountry.csv").c_str(), "r");
#else
	FILE * ip2country = fopen((ServerManager::m_sPath + "/cfg/IpToCountry.csv").c_str(), "r");
#endif
	
	if (ip2country == NULL)
	{
		return;
	}
	
	if (m_ui32Size == 0)
	{
		m_ui32Size = 131072;
		
		if (m_ui32RangeFrom == NULL)
		{
			m_ui32RangeFrom = (uint32_t *)calloc(m_ui32Size, sizeof(uint32_t));
			
			if (m_ui32RangeFrom == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui32RangeFrom\n");
				fclose(ip2country);
				m_ui32Size = 0;
				return;
			}
		}
		
		if (m_ui32RangeTo == NULL)
		{
			m_ui32RangeTo = (uint32_t *)calloc(m_ui32Size, sizeof(uint32_t));
			
			if (m_ui32RangeTo == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui32RangeTo\n");
				fclose(ip2country);
				m_ui32Size = 0;
				return;
			}
		}
		
		if (m_ui8RangeCI == NULL)
		{
			m_ui8RangeCI = (uint8_t *)calloc(m_ui32Size, sizeof(uint8_t));
			
			if (m_ui8RangeCI == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui8RangeCI\n");
				fclose(ip2country);
				m_ui32Size = 0;
				return;
			}
		}
	}
	
	char sLine[1024];
	
	while (fgets(sLine, 1024, ip2country) != NULL)
	{
		if (sLine[0] != '\"')
		{
			continue;
		}
		
		if (m_ui32Count == m_ui32Size)
		{
			m_ui32Size += 512;
			void * oldbuf = m_ui32RangeFrom;
			m_ui32RangeFrom = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
			if (m_ui32RangeFrom == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui32RangeFrom\n", m_ui32Size);
				m_ui32RangeFrom = (uint32_t *)oldbuf;
				fclose(ip2country);
				return;
			}
			
			oldbuf = m_ui32RangeTo;
			m_ui32RangeTo = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
			if (m_ui32RangeTo == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui32RangeTo\n", m_ui32Size);
				m_ui32RangeTo = (uint32_t *)oldbuf;
				fclose(ip2country);
				return;
			}
			
			oldbuf = m_ui8RangeCI;
			m_ui8RangeCI = (uint8_t *)realloc(oldbuf, m_ui32Size * sizeof(uint8_t));
			if (m_ui8RangeCI == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui8RangeCI\n", m_ui32Size);
				m_ui8RangeCI = (uint8_t *)oldbuf;
				fclose(ip2country);
				return;
			}
		}
		
		char * sStart = sLine + 1;
		uint8_t ui8d = 0;
		
		size_t szLineLen = strlen(sLine);
		
		for (size_t szi = 1; szi < szLineLen; szi++)
		{
			if (sLine[szi] == '\"')
			{
				sLine[szi] = '\0';
				if (ui8d == 0)
				{
					m_ui32RangeFrom[m_ui32Count] = strtoul(sStart, NULL, 10);
				}
				else if (ui8d == 1)
				{
					m_ui32RangeTo[m_ui32Count] = strtoul(sStart, NULL, 10);
				}
				else if (ui8d == 4)
				{
					for (uint8_t ui8i = 0; ui8i < COUNTRY_COUNT - 1; ui8i++)
					{
						if (*((uint16_t *)CountryCodes[ui8i]) == *((uint16_t *)sStart))
						{
							m_ui8RangeCI[m_ui32Count] = ui8i;
							m_ui32Count++;
							break;
						}
					}
					
					break;
				}
				
				ui8d++;
				szi += (uint16_t)2;
				sStart = sLine + szi + 1;
				
			}
		}
	}
	
	fclose(ip2country);
	
	if (m_ui32Count < m_ui32Size)
	{
		m_ui32Size = m_ui32Count;
		
		void * oldbuf = m_ui32RangeFrom;
		m_ui32RangeFrom = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
		if (m_ui32RangeFrom == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui32RangeFrom\n", m_ui32Size);
			m_ui32RangeFrom = (uint32_t *)oldbuf;
		}
		
		oldbuf = m_ui32RangeTo;
		m_ui32RangeTo = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
		if (m_ui32RangeTo == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui32RangeTo\n", m_ui32Size);
			m_ui32RangeTo = (uint32_t *)oldbuf;
		}
		
		oldbuf = m_ui8RangeCI;
		m_ui8RangeCI = (uint8_t *)realloc(oldbuf, m_ui32Size * sizeof(uint8_t));
		if (m_ui8RangeCI == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui8RangeCI\n", m_ui32Size);
			m_ui8RangeCI = (uint8_t *)oldbuf;
		}
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::LoadIPv6()
{
#ifdef FLYLINKDC_USE_IP_TO_COUNTRY_V6
	if (ServerManager::m_bUseIPv6 == false)
	{
		return;
	}
	
#ifdef _WIN32
	FILE * ip2country = fopen((ServerManager::m_sPath + "\\cfg\\IpToCountry.6R.csv").c_str(), "r");
#else
	FILE * ip2country = fopen((ServerManager::m_sPath + "/cfg/IpToCountry.6R.csv").c_str(), "r");
#endif
	
	if (ip2country == NULL)
	{
		return;
	}
	
	if (m_ui32IPv6Size == 0)
	{
		m_ui32IPv6Size = 16384;
		
		if (m_ui128IPv6RangeFrom == NULL)
		{
			ui128IPv6RangeFrom = (uint8_t *)calloc(ui32IPv6Size, sizeof(uint8_t) * 16);
			
			if (m_ui128IPv6RangeFrom == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui128IPv6RangeFrom\n");
				fclose(ip2country);
				ui32IPv6Size = 0;
				return;
			}
		}
		
		if (m_ui128IPv6RangeTo == NULL)
		{
			ui128IPv6RangeTo = (uint8_t *)calloc(ui32IPv6Size, sizeof(uint8_t) * 16);
			
			if (m_ui128IPv6RangeTo == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui128IPv6RangeTo\n");
				fclose(ip2country);
				m_ui32IPv6Size = 0;
				return;
			}
		}
		
		if (m_ui8IPv6RangeCI == NULL)
		{
			ui8IPv6RangeCI = (uint8_t *)calloc(ui32IPv6Size, sizeof(uint8_t));
			
			if (m_ui8IPv6RangeCI == NULL)
			{
				AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui8IPv6RangeCI\n");
				fclose(ip2country);
				m_ui32IPv6Size = 0;
				return;
			}
		}
	}
	
	char sLine[1024];
	
	while (fgets(sLine, 1024, ip2country) != NULL)
	{
		if (sLine[0] == '#' || sLine[0] < 32)
		{
			continue;
		}
		
		if (m_ui32IPv6Count == m_ui32IPv6Size)
		{
			ui32IPv6Size += 512;
			void * oldbuf = ui128IPv6RangeFrom;
			ui128IPv6RangeFrom = (uint8_t *)realloc(oldbuf, ui32IPv6Size * (sizeof(uint8_t) * 16));
			if (ui128IPv6RangeFrom == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui128IPv6RangeFrom\n", m_ui32IPv6Size);
				m_ui128IPv6RangeFrom = (uint8_t *)oldbuf;
				fclose(ip2country);
				return;
			}
			
			oldbuf = ui128IPv6RangeTo;
			ui128IPv6RangeTo = (uint8_t *)realloc(oldbuf, ui32IPv6Size * (sizeof(uint8_t) * 16));
			if (ui128IPv6RangeTo == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui128IPv6RangeTo\n", ui32IPv6Size);
				ui128IPv6RangeTo = (uint8_t *)oldbuf;
				fclose(ip2country);
				return;
			}
			
			oldbuf = ui8IPv6RangeCI;
			ui8IPv6RangeCI = (uint8_t *)realloc(oldbuf, ui32IPv6Size * sizeof(uint8_t));
			if (ui8IPv6RangeCI == NULL)
			{
				AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui8IPv6RangeCI\n", ui32IPv6Size);
				ui8IPv6RangeCI = (uint8_t *)oldbuf;
				fclose(ip2country);
				return;
			}
		}
		
		char * sStart = sLine;
		uint8_t ui8d = 0;
		
		size_t szLineLen = strlen(sLine);
		
		for (size_t szi = 0; szi < szLineLen; szi++)
		{
			if (ui8d == 0 && sLine[szi] == '-')
			{
				sLine[szi] = '\0';
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
				win_inet_pton(sStart, m_ui128IPv6RangeFrom + (m_ui32IPv6Count * 16));
#else
				inet_pton(AF_INET6, sStart, m_ui128IPv6RangeFrom + (m_ui32IPv6Count * 16));
#endif
			}
			else if (sLine[szi] == ',')
			{
				sLine[szi] = '\0';
				if (ui8d == 1)
				{
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
					win_inet_pton(sStart, m_ui128IPv6RangeTo + (m_ui32IPv6Count * 16));
#else
					inet_pton(AF_INET6, sStart, m_ui128IPv6RangeTo + (m_ui32IPv6Count * 16));
#endif
				}
				else
				{
					for (uint8_t ui8i = 0; ui8i < COUNTRY_COUNT - 1; ui8i++)
					{
						if (*((uint16_t *)CountryCodes[ui8i]) == *((uint16_t *)sStart))
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
			sStart = sLine + szi + 1;
		}
	}
	
	fclose(ip2country);
	
	if (m_ui32IPv6Count < m_ui32IPv6Size)
	{
		m_ui32IPv6Size = m_ui32IPv6Count;
		
		void * oldbuf = ui128IPv6RangeFrom;
		ui128IPv6RangeFrom = (uint8_t *)realloc(oldbuf, ui32IPv6Size * (sizeof(uint8_t) * 16));
		if (ui128IPv6RangeFrom == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui128IPv6RangeFrom\n", m_ui32IPv6Size);
			m_ui128IPv6RangeFrom = (uint8_t *)oldbuf;
		}
		
		oldbuf = ui128IPv6RangeTo;
		ui128IPv6RangeTo = (uint8_t *)realloc(oldbuf, ui32IPv6Size * (sizeof(uint8_t) * 16));
		if (ui128IPv6RangeTo == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui128IPv6RangeTo\n", m_ui32IPv6Size);
			m_ui128IPv6RangeTo = (uint8_t *)oldbuf;
		}
		
		oldbuf = ui8IPv6RangeCI;
		ui8IPv6RangeCI = (uint8_t *)realloc(oldbuf, ui32IPv6Size * sizeof(uint8_t));
		if (ui8IPv6RangeCI == NULL)
		{
			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui8IPv6RangeCI\n", ui32IPv6Size);
			ui8IPv6RangeCI = (uint8_t *)oldbuf;
		}
	}
#else
	AppendDebugLogFormat("[SYS] Disable load IpToCountry.6R.csv\n");
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

IpP2Country::IpP2Country() : m_ui32RangeFrom(NULL), m_ui32RangeTo(NULL), m_ui8RangeCI(NULL), m_ui8IPv6RangeCI(NULL), m_ui128IPv6RangeFrom(NULL), m_ui128IPv6RangeTo(NULL), m_ui32Size(0), m_ui32IPv6Size(0), m_ui32Count(0), m_ui32IPv6Count(0)
{
	LoadIPv4();
	LoadIPv6();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

IpP2Country::~IpP2Country()
{
	free(m_ui32RangeFrom);
	free(m_ui32RangeTo);
	free(m_ui8RangeCI);
	free(m_ui128IPv6RangeFrom);
	free(m_ui128IPv6RangeTo);
	free(m_ui8IPv6RangeCI);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const char * IpP2Country::Find(const uint8_t * ui128IpHash, const bool bCountryName)
{
	bool bIPv4 = false;
	uint32_t ui32IpHash = 0;
	
	if(ServerManager::m_bUseIPv6 == false || IN6_IS_ADDR_V4MAPPED((in6_addr *)ui128IpHash))
	{
		bIPv4 = true;
		
		ui32IpHash = 16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15];
	}
	
	if (bIPv4 == false)
	{
		if(ui128IpHash[0] == 32 && ui128IpHash[1] == 2)   // 6to4 tunnel
		{
			bIPv4 = true;
			
			ui32IpHash = 16777216 * ui128IpHash[2] + 65536 * ui128IpHash[3] + 256 * ui128IpHash[4] + ui128IpHash[5];
		}
		else if(ui128IpHash[0] == 32 && ui128IpHash[1] == 1 && ui128IpHash[2] == 0 && ui128IpHash[3] == 0)     // teredo tunnel
		{
			bIPv4 = true;
			
			ui32IpHash = (16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15]) ^ 0xffffffff;
		}
	}
	
	if (bIPv4 == true)
	{
		for (uint32_t ui32i = 0; ui32i < m_ui32Count; ui32i++)
		{
			if (m_ui32RangeFrom[ui32i] <= ui32IpHash && m_ui32RangeTo[ui32i] >= ui32IpHash)
			{
				if (bCountryName == false)
				{
					return CountryCodes[m_ui8RangeCI[ui32i]];
				}
				else
				{
					return TranslateCountry(m_ui8RangeCI[ui32i]);
				}
			}
		}
	}
	else
	{
		for (uint32_t ui32i = 0; ui32i < m_ui32IPv6Count; ui32i++)
		{
			if(memcmp(m_ui128IPv6RangeFrom+(ui32i*16), ui128IpHash, 16) <= 0 && memcmp(m_ui128IPv6RangeTo+(ui32i*16), ui128IpHash, 16) >= 0)
			{
				if (bCountryName == false)
				{
					return CountryCodes[m_ui8IPv6RangeCI[ui32i]];
				}
				else
				{
					return CountryNames[m_ui8IPv6RangeCI[ui32i]];
				}
			}
		}
	}
	
	if (bCountryName == false)
	{
		return CountryCodes[COUNTRY_COUNT - 1];
	}
	else
	{
		return TranslateCountry(COUNTRY_COUNT - 1);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint8_t IpP2Country::Find(const uint8_t * ui128IpHash)
{
	bool bIPv4 = false;
	uint32_t ui32IpHash = 0;
	
	if(ServerManager::m_bUseIPv6 == false || IN6_IS_ADDR_V4MAPPED((in6_addr *)ui128IpHash))
	{
		bIPv4 = true;
		
		ui32IpHash = 16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15];
	}
	
	if (bIPv4 == false)
	{
		if(ui128IpHash[0] == 32 && ui128IpHash[1] == 2)   // 6to4 tunnel
		{
			bIPv4 = true;
			
			ui32IpHash = 16777216 * ui128IpHash[2] + 65536 * ui128IpHash[3] + 256 * ui128IpHash[4] + ui128IpHash[5];
		}
		else if(ui128IpHash[0] == 32 && ui128IpHash[1] == 1 && ui128IpHash[2] == 0 && ui128IpHash[3] == 0)     // teredo tunnel
		{
			bIPv4 = true;
			
			ui32IpHash = (16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15]) ^ 0xffffffff;
		}
	}
	
	if (bIPv4 == true)
	{
		for (uint32_t ui32i = 0; ui32i < m_ui32Count; ui32i++)
		{
			if (m_ui32RangeFrom[ui32i] <= ui32IpHash && m_ui32RangeTo[ui32i] >= ui32IpHash)
			{
				return m_ui8RangeCI[ui32i];
			}
		}
	}
	else
	{
		for (uint32_t ui32i = 0; ui32i < m_ui32IPv6Count; ui32i++)
		{
			if(memcmp(m_ui128IPv6RangeFrom+(ui32i*16), ui128IpHash, 16) <= 0 && memcmp(m_ui128IPv6RangeTo+(ui32i*16), ui128IpHash, 16) >= 0)
			{
				return m_ui8IPv6RangeCI[ui32i];
			}
		}
	}
	
	return COUNTRY_COUNT - 1;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const char * IpP2Country::GetCountry(const uint8_t ui8dx, const bool bCountryName)
{
	if (bCountryName == false)
	{
		return CountryCodes[ui8dx];
	}
	else
	{
		return TranslateCountry(ui8dx);
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// alex82 ... Определяем страну по коду
const char * IpP2Country::GetCountryName(const char * sCode)
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
