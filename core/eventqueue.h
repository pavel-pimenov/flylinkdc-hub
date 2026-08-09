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
#ifndef eventqueueH
#define eventqueueH
//---------------------------------------------------------------------------
#include "CriticalSection.h"
#include <array>
#include <list>
#include <string>

class EventQueue
{
public:
    enum class EventType : uint8_t
    {
        RESTART,
        RSTSCRIPTS,
        RSTSCRIPT,
        STOPSCRIPT,
        STOP_SCRIPTING,
        SHUTDOWN,
        REGSOCK_MSG,
        SRVTHREAD_MSG,
        UDP_SR
    };

private:
    struct Event
    {
        std::string m_sMsg;

        std::array<uint8_t, 16> m_ui128IpHash{};
        EventType m_eId = EventType::RESTART;

        explicit Event(const char* p_message);

        Event(const Event&) = delete;

        auto operator=(const Event&) -> Event& = delete;
    };

    CriticalSection m_csEventQueue;

public:
    static std::unique_ptr<EventQueue> m_Ptr;

    std::list<std::unique_ptr<Event>> m_NormalEvents;
    std::list<std::unique_ptr<Event>> m_ThreadEvents;

    uint32_t m_ui32NormalDepth = 0;

    EventQueue(const EventQueue&) = delete;
    auto operator=(const EventQueue&) -> EventQueue& = delete;

    EventQueue() = default;
    ~EventQueue();

    void AddNormal(EventType eId, const char* sMsg);
    void AddThread(EventType eId, const char* sMsg, const sockaddr_storage* sas = nullptr);
    void ProcessEvents();
};
//---------------------------------------------------------------------------

#endif
