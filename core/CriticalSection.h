#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H

#include <mutex>

using CriticalSection = std::mutex;
using Lock = std::scoped_lock<std::mutex>;

#endif // !defined(CRITICAL_SECTION_H)
