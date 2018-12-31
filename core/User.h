/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
#ifndef UserH
#define UserH
//---------------------------------------------------------------------------

#include <string>

//---------------------------------------------------------------------------
struct User; // needed for next struct, and next struct must be defined before user :-/
//---------------------------------------------------------------------------

struct DcCommand
{
	User * m_pUser;
	
	char * m_sCommand;
	
	uint32_t m_ui32CommandLen;
};
//---------------------------------------------------------------------------

struct UserBan
{
	UserBan();
	~UserBan();
	
	char * m_sMessage;
	
	uint32_t m_ui32Len, m_ui32NickHash;
	
	static UserBan * CreateUserBan(const char * sMess, const uint32_t ui32MessLen, const uint32_t ui32Hash);
	
	DISALLOW_COPY_AND_ASSIGN(UserBan);
};
//---------------------------------------------------------------------------

struct LoginLogout
{
	LoginLogout();
	~LoginLogout();
	void Clean();
	
	uint64_t m_ui64LogonTick, m_ui64IPv4CheckTick;
	
	UserBan * m_pBan;
	
	char * m_pBuffer;
	
	uint32_t m_ui32ToCloseLoops, m_ui32UserConnectedLen;
	DISALLOW_COPY_AND_ASSIGN(LoginLogout);
};
//---------------------------------------------------------------------------

struct PrcsdUsrCmd
{
	PrcsdUsrCmd() : m_pNext(NULL), m_pPtr(NULL), m_sCommand(NULL), m_ui32Len(0), m_ui8Type(0)
#ifdef FLYLINKDC_USE_STAT_RELOCATION
		, m_ui32LenMax(0), m_ui32CountRelocation(0)
#endif
	{
#ifdef _PtokaX_TESTING_
		// printf("PrcsdUsrCmd()\r\n");
#endif
	}
	~PrcsdUsrCmd()
	{
#ifdef FLYLINKDC_USE_STAT_RELOCATION
		if (m_ui32CountRelocation > 1)
		{
#ifndef _WIN32
			syslog(LOG_NOTICE, "~PrcsdUsrCmd this = %p m_ui32LenMax = %u, count = %u ui8Type = %d", this, m_ui32LenMax, m_ui32CountRelocation, int(ui8Type));
#else
			printf("~PrcsdUsrCmd this = %p m_ui32LenMax = %u, count = %u ui8Type = %d\r\n", this, m_ui32LenMax, m_ui32CountRelocation, int(ui8Type));
#endif
		}
#endif
	}
	enum PrcsdCmdsIds
	{
		CTM_MCTM_RCTM_SR_TO,
		SUPPORTS,
		LOGINHELLO,
		GETPASS,
		CHAT,
		TO_OP_CHAT
	};
	
	PrcsdUsrCmd * m_pNext;
	
	void * m_pPtr;
	
	char * m_sCommand;
	
	uint32_t m_ui32Len;
	uint8_t m_ui8Type;
	
#ifdef FLYLINKDC_USE_STAT_RELOCATION
	uint32_t m_ui32LenMax;
	uint32_t m_ui32CountRelocation;
	void stat()
	{
		if (m_ui32LenMax < ui32Len)
		{
			m_ui32LenMax = ui32Len;
		}
		m_ui32CountRelocation++;
	}
#else
	void stat() {}
#endif
	
	DISALLOW_COPY_AND_ASSIGN(PrcsdUsrCmd);
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
struct User; // needed for next struct, and next struct must be defined before user :-/
//---------------------------------------------------------------------------

struct PrcsdToUsrCmd
{
	PrcsdToUsrCmd() : m_pNext(NULL), m_pToUser(NULL), m_sCommand(NULL), m_ui32Len(0), m_ui32PmCount(0), m_ui32Loops(0) { }
	
	PrcsdToUsrCmd * m_pNext;
	
	User * m_pToUser;
	
	char * m_sCommand;
	std::string m_nick;
	
	uint32_t m_ui32Len, m_ui32PmCount, m_ui32Loops;
	DISALLOW_COPY_AND_ASSIGN(PrcsdToUsrCmd);
};
//---------------------------------------------------------------------------
struct QzBuf; // for send queue
//---------------------------------------------------------------------------
#ifdef USE_FLYLINKDC_EXT_JSON
struct User;
class ExtJSONInfo
{
		std::string m_ExtJSON;
		std::string m_ExtJSONOriginal;
		uint64_t    m_iLastExtJSONSendTick;
	public:
		ExtJSONInfo() : m_iLastExtJSONSendTick(0)
		{
		}
		explicit ExtJSONInfo(const char* p_info) : m_iLastExtJSONSendTick(0)
		{
			if (p_info)
			{
				m_ExtJSON = p_info;
			}
		}
		uint64_t   getLastExtJSONSendTick() const
		{
			return m_iLastExtJSONSendTick;
		}
		void   setLastExtJSONSendTick(uint64_t p_tick)
		{
			m_iLastExtJSONSendTick = p_tick;
		}
		bool ComparExtJSON(const char * sNewExtJSON, const uint16_t ui16NewExtJSONLen) const
		{
			if (!m_ExtJSONOriginal.empty())
				return m_ExtJSONOriginal == std::string(sNewExtJSON, ui16NewExtJSONLen);
			else
				return m_ExtJSON == std::string(sNewExtJSON, ui16NewExtJSONLen);
		}
		void SetJSONOriginal(const std::string& p_json)
		{
			m_ExtJSON = p_json;
			m_ExtJSONOriginal = p_json;
		}
		const std::string& GetExtJSONCommand() const
		{
			return m_ExtJSON;
		}
};
#endif

struct User
{
	bool m_is_proxy_user;
	bool m_is_bad_len_port;
	bool m_is_bad_len_number_myinfo;
	bool m_is_bad_port;
	bool m_is_ddos_udp;
	bool m_is_max_int8;
	bool m_is_max_ip_len;
	bool m_is_invalid_json;
	
	bool m_is_json_user;
	
	bool isBlockSearch() const
	{
		return m_is_bad_len_port || m_is_ddos_udp || m_is_bad_len_number_myinfo; // TODO || m_is_max_int8 + m_is_max_ip_len;
	}
	
	uint64_t m_ui64SharedSize, m_ui64ChangedSharedSizeShort, m_ui64ChangedSharedSizeLong;
	uint64_t m_ui64GetNickListsTick, m_ui64MyINFOsTick, m_ui64SearchsTick, m_ui64ChatMsgsTick;
	uint64_t m_ui64PMsTick, m_ui64SameSearchsTick, m_ui64SamePMsTick, m_ui64SameChatsTick;
	uint64_t m_ui64LastMyINFOSendTick, m_ui64LastNicklist, m_ui64ReceivedPmTick, m_ui64ChatMsgsTick2;
	uint64_t m_ui64PMsTick2, m_ui64SearchsTick2, m_ui64MyINFOsTick2, m_ui64CTMsTick;
	uint64_t m_ui64CTMsTick2, m_ui64RCTMsTick, m_ui64RCTMsTick2, m_ui64SRsTick;
	uint64_t m_ui64SRsTick2, m_ui64RecvsTick, m_ui64RecvsTick2, m_ui64ChatIntMsgsTick;
	uint64_t m_ui64PMsIntTick, m_ui64SearchsIntTick;
	
	time_t m_tLoginTime;
	LoginLogout m_LogInOut;
	
	PrcsdToUsrCmd * m_pCmdToUserStrt, * m_pCmdToUserEnd;
	
	PrcsdUsrCmd * m_pCmdStrt, * m_pCmdEnd,
	            * m_pCmdActive4Search, *m_pCmdActive6Search, *m_pCmdPassiveSearch;
	            
#ifdef USE_FLYLINKDC_EXT_JSON
	void SendCharDelayedExtJSON();
	void SetExtJSONOriginal(const char * sNewExtJSON, const uint16_t ui16NewExtJSONLen);
	bool ComparExtJSON(const char * sNewExtJSON, const uint16_t ui16NewExtJSONLen) const
	{
		if (m_user_ext_info == NULL)
			return true;
		else
			return m_user_ext_info->ComparExtJSON(sNewExtJSON, ui16NewExtJSONLen);
	}
	bool isSupportExtJSON() const
	{
		return m_is_json_user; // (m_ui32SupportBits & SUPPORTBIT_EXTJSON2) == SUPPORTBIT_EXTJSON2;
	}
#endif
	










	User * m_pPrev, * m_pNext, * m_pHashTablePrev, * m_pHashTableNext, * m_pHashIpTablePrev, * m_pHashIpTableNext;
	
	char * m_sNick;
	
#ifdef FLYLINKDC_USE_VERSION
    char * m_sVersion;
#endif
	char * m_sMyInfoOriginal, * m_sMyInfoShort, * m_sMyInfoLong;
	char * m_sDescription, * m_sTag, * m_sConnection, * m_sEmail;
	char * m_sClient, * m_sTagVersion;
	char * m_sLastChat, * m_sLastPM, * m_sLastSearch;
	char * m_pSendBuf, * m_pRecvBuf, * m_pSendBufHead;
	char * m_sChangedDescriptionShort, * m_sChangedDescriptionLong, * m_sChangedTagShort, * m_sChangedTagLong;
	char * m_sChangedConnectionShort, * m_sChangedConnectionLong, * m_sChangedEmailShort, * m_sChangedEmailLong;
	
	uint32_t m_ui32Recvs, m_ui32Recvs2;
	
	uint32_t m_ui32Hubs, m_ui32Slots, m_ui32OLimit, m_ui32LLimit, m_ui32DLimit, m_ui32NormalHubs, m_ui32RegHubs, m_ui32OpHubs;
	uint32_t m_ui32SendCalled, m_ui32RecvCalled, m_ui32ReceivedPmCount, m_ui32SR, m_ui32DefloodWarnings;
	uint32_t m_ui32BoolBits, m_ui32InfoBits, m_ui32SupportBits;
	
	uint32_t m_ui32SendBufLen, m_ui32RecvBufLen, m_ui32SendBufDataLen, m_ui32RecvBufDataLen;
	
	uint32_t m_ui32NickHash;
	
	int32_t m_i32Profile;

	std::string m_LastSearch;
	
#ifdef _WIN32
	SOCKET m_Socket;
#else
	int m_Socket;
#endif
	
	uint16_t m_ui16MyInfoOriginalLen, m_ui16MyInfoShortLen, m_ui16MyInfoLongLen;
	uint16_t m_ui16GetNickLists, m_ui16MyINFOs, m_ui16Searchs, m_ui16ChatMsgs, m_ui16PMs;
	uint16_t m_ui16SameSearchs, m_ui16LastSearchLen, m_ui16SamePMs, m_ui16LastPMLen;
	uint16_t m_ui16SameChatMsgs, m_ui16LastChatLen, m_ui16LastPmLines, m_ui16SameMultiPms;
	uint16_t m_ui16LastChatLines, m_ui16SameMultiChats, m_ui16ChatMsgs2, m_ui16PMs2;
	uint16_t m_ui16Searchs2, m_ui16MyINFOs2, m_ui16CTMs, m_ui16CTMs2;
	uint16_t m_ui16RCTMs, m_ui16RCTMs2, m_ui16SRs, m_ui16SRs2;
	uint16_t m_ui16ChatIntMsgs, m_ui16PMsInt, m_ui16SearchsInt;
	uint16_t m_ui16IpTableIdx;
	
	uint8_t m_ui8MagicByte;
	
	uint8_t m_ui8NickLen;
	uint8_t m_ui8IpLen, m_ui8ConnectionLen, m_ui8DescriptionLen, m_ui8EmailLen, m_ui8TagLen, m_ui8ClientLen, m_ui8TagVersionLen;
	uint8_t m_ui8Country, m_ui8State, m_ui8IPv4Len;
	uint8_t m_ui8ChangedDescriptionShortLen, m_ui8ChangedDescriptionLongLen, m_ui8ChangedTagShortLen, m_ui8ChangedTagLongLen;
	uint8_t m_ui8ChangedConnectionShortLen, m_ui8ChangedConnectionLongLen, m_ui8ChangedEmailShortLen, m_ui8ChangedEmailLongLen;

	uint64_t m_last_recv_tick;
	
	uint8_t m_ui128IpHash[16];
	
	char m_sIP[40], m_sIPv4[16];
	
	char m_sModes[3];
	
	enum UserStates
	{
		STATE_SOCKET_ACCEPTED,
		STATE_KEY_OR_SUP,
		STATE_VALIDATE,
		STATE_VERSION_OR_MYPASS,
		STATE_GETNICKLIST_OR_MYINFO,
		STATE_IPV4_CHECK,
		STATE_ADDME,
		STATE_ADDME_1LOOP,
		STATE_ADDME_2LOOP,
		STATE_ADDED,
		STATE_CLOSING,
		STATE_REMME
	};
	
    //  u->ui32BoolBits |= BIT_PRCSD_MYINFO;   <- set to 1
    //  u->ui32BoolBits &= ~BIT_PRCSD_MYINFO;  <- set to 0
    //  (u->ui32BoolBits & BIT_PRCSD_MYINFO) == BIT_PRCSD_MYINFO    <- test if is 1/true
    enum UserBits {
		BIT_HASHED                     = 0x1,
		BIT_ERROR                      = 0x2,
		BIT_OPERATOR                   = 0x4,
		BIT_GAGGED                     = 0x8,
		BIT_GETNICKLIST                = 0x10,
		BIT_IPV4_ACTIVE                = 0x20,
		BIT_OLDHUBSTAG                 = 0x40,
		BIT_TEMP_OPERATOR              = 0x80,
		BIT_PINGER                     = 0x100,
		BIT_BIG_SEND_BUFFER            = 0x200,
		BIT_HAVE_SUPPORTS              = 0x400,
		BIT_HAVE_BADTAG                = 0x800,
		BIT_HAVE_GETNICKLIST           = 0x1000,
		BIT_HAVE_BOTINFO               = 0x2000,
		BIT_HAVE_KEY                   = 0x4000,
		BIT_HAVE_SHARECOUNTED          = 0x8000,
		BIT_PRCSD_MYINFO               = 0x10000,
		BIT_RECV_FLOODER               = 0x20000,
		BIT_QUACK_SUPPORTS             = 0x400000,
		BIT_IPV6                       = 0x800000,
		BIT_IPV4                       = 0x1000000,
		BIT_WAITING_FOR_PASS           = 0x2000000,
		BIT_WARNED_WRONG_IP            = 0x4000000,
		BIT_IPV6_ACTIVE                = 0x8000000,
		BIT_CHAT_INSERT                = 0x10000000,
#ifdef USE_FLYLINKDC_EXT_JSON
		BIT_PRCSD_EXT_JSON             = 0x20000000
#endif
	};
	
	enum UserInfoBits
	{
		INFOBIT_DESCRIPTION_CHANGED        = 0x1,
		INFOBIT_TAG_CHANGED                = 0x2,
		INFOBIT_CONNECTION_CHANGED         = 0x4,
		INFOBIT_EMAIL_CHANGED              = 0x8,
		INFOBIT_SHARE_CHANGED              = 0x10,
		INFOBIT_DESCRIPTION_SHORT_PERM     = 0x20,
		INFOBIT_DESCRIPTION_LONG_PERM      = 0x40,
		INFOBIT_TAG_SHORT_PERM             = 0x80,
		INFOBIT_TAG_LONG_PERM              = 0x100,
		INFOBIT_CONNECTION_SHORT_PERM      = 0x200,
		INFOBIT_CONNECTION_LONG_PERM       = 0x400,
		INFOBIT_EMAIL_SHORT_PERM           = 0x800,
		INFOBIT_EMAIL_LONG_PERM            = 0x1000,
		INFOBIT_SHARE_SHORT_PERM           = 0x2000,
		INFOBIT_SHARE_LONG_PERM            = 0x4000,
		// alex82 ... HideUser / Скрытие юзера
		INFOBIT_HIDDEN                     = 0x8000,
		// alex82 ... NoQuit / Подавляем $Quit для юзера
		INFOBIT_NO_QUIT                    = 0x10000,
		// alex82 ... HideUserKey / Прячем ключ юзера
		INFOBIT_HIDE_KEY                   = 0x20000,
		// alex82 feature...
	};
	
	enum UserSupportBits
	{
		SUPPORTBIT_NOGETINFO               = 0x1,
		SUPPORTBIT_USERCOMMAND             = 0x2,
		SUPPORTBIT_NOHELLO                 = 0x4,
		SUPPORTBIT_QUICKLIST               = 0x8,
		SUPPORTBIT_USERIP2                 = 0x10,
		SUPPORTBIT_ZPIPE                   = 0x20,
		SUPPORTBIT_IP64                    = 0x40,
		SUPPORTBIT_IPV4                    = 0x80,
		SUPPORTBIT_TLS2                    = 0x100,
		SUPPORTBIT_ZPIPE0                  = 0x200,
#ifdef USE_FLYLINKDC_EXT_JSON
		// Bug SUPPORTBIT_EXTJSON2                  = 0x400,
#endif
		SUPPORTBIT_HUBURL = 0x800
	};
	bool isSupportZpipe() const
	{
		return (m_ui32SupportBits & SUPPORTBIT_ZPIPE) == SUPPORTBIT_ZPIPE;
	}
	void SendTextDelayed(const string & sText);
	
	void logInvalidUser(int p_id, const char * p_buffer, int p_len, bool p_is_send);

#ifdef USE_FLYLINKDC_EXT_JSON
	ExtJSONInfo * m_user_ext_info;
	
	void initExtJSON(const char* p_json)
	{
		if (m_user_ext_info == NULL)
		{
			m_user_ext_info = new ExtJSONInfo(p_json);
			m_ui32BoolBits |= User::BIT_PRCSD_EXT_JSON;
		}
	}
	
	uint64_t   getLastExtJSONSendTick() const
	{
		return m_user_ext_info ? m_user_ext_info->getLastExtJSONSendTick() : 0;
	}
	void  setLastExtJSONSendTick(uint64_t p_tick)
	{
		if (m_user_ext_info)
		{
			m_user_ext_info->setLastExtJSONSendTick(p_tick);
		}
	}
#endif
	// alex82 ... Запретили юзерам с одинаковыми профилями глушить друг друга
	bool CheckBanProfile(const User* pOtherUser) const
	{
		return pOtherUser->m_i32Profile != -1 && m_i32Profile != 0 && m_i32Profile >= pOtherUser->m_i32Profile;
	}
	User();
	~User();
	
	bool MakeLock();
	bool DoRecv();
	
	void SendChar(const char * sText, const size_t szTextLen);
	void SendCharDelayed(const std::string& p_Text)
	{
		if (!p_Text.empty())
		{
			SendCharDelayed(p_Text.c_str(), p_Text.size());
		}
	}
	
	void SendCharDelayed(const char * sText, const size_t szTextLen);
	void SendFormat(const char * sFrom, const bool bDelayed, const char * sFormatMsg, ...);
	void SendFormatCheckPM(const char * sFrom, const char * sOtherNick, const bool bDelayed, const char * sFormatMsg, ...);
	
	bool PutInSendBuf(const char * sText, const size_t szTxtLen);
	bool Try2Send();
	
	void SetIP(const char * sIP);
	void SetNick(const char * sNick, const uint8_t ui8NickLen);
	void SetMyInfoOriginal(const char * sMyInfo, const uint16_t ui16MyInfoLen);
	void SetVersion(const char * sVersion);
	void SetLastChat(const char * sData, const size_t szLen);
	void SetLastPM(const char * sData, const size_t szLen);
	void SetLastSearch(const char * sData, const size_t szLen);
	void SetBuffer(const char * sKickMsg, size_t szLen = 0);
	void FreeBuffer();
	
	void Close(const bool bNoQuit = false);
	
	void Add2Userlist();
	void AddUserList();
	
	bool GenerateMyInfoLong();
	bool GenerateMyInfoShort();
	
	inline static void FreeInfo(char *& sInfo, uint8_t& p_len)
	{
		if (sInfo)
		{
			free(sInfo);
			sInfo = nullptr;
		}
		p_len = 0;
	}
	
	void HasSuspiciousTag();
	
	bool ProcessRules();
	
	void AddPrcsdCmd(const uint8_t ui8Type, const char * sCommand, const size_t szCommandLen, User * pToUser, const bool bIsPm = false);
	
	void AddMeOrIPv4Check();
	
	static char * SetUserInfo(char * sOldData, uint8_t &ui8OldDataLen, const char * sNewData, const size_t szNewDataLen, const char * sDataName);
	
	void RemFromSendBuf(const char * sData, const uint32_t ui32Len, const uint32_t ui32SendBufLen);
	
	static void DeletePrcsdUsrCmd(PrcsdUsrCmd *& pCommand); //[+]FlylinkDC++
	
	DISALLOW_COPY_AND_ASSIGN(User);
};
//---------------------------------------------------------------------------

#endif
