#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H

// typedef boost::recursive_mutex  CriticalSection;
// typedef boost::lock_guard<boost::recursive_mutex> Lock;
class CriticalSection
{
	public:
		void lock()
		{
#ifdef _WIN32
			//assert(cs.RecursionCount == 0 || (cs.RecursionCount > 0 && tryLock() == true));
			EnterCriticalSection(&cs);
#else
			pthread_mutex_lock(&mutex);
#endif
		}
		void unlock()
		{
#ifdef _WIN32
			LeaveCriticalSection(&cs);
			//assert(cs.RecursionCount == 0 || (cs.RecursionCount > 0 && tryLock() == true));
#else
			pthread_mutex_unlock(&mutex);
#endif
		}
		bool tryLock()
		{
#ifdef _WIN32
			if (TryEnterCriticalSection(&cs) != FALSE)
				return true;
			else
				return false;
#else
			const int rc = pthread_mutex_trylock(&mutex);
			if (rc == EBUSY)
				return false;
			return true;
#endif
		}
		explicit CriticalSection()
		{
#ifdef _WIN32
			InitializeCriticalSectionAndSpinCount(&cs, 2000);
#else
			pthread_mutex_init(&mutex, NULL);
#endif
		}
		~CriticalSection()
		{
#ifdef _WIN32
			DeleteCriticalSection(&cs);
#else
			pthread_mutex_destroy(&mutex);
#endif
		}
	private:
#ifdef _WIN32
		CRITICAL_SECTION cs;
#else
		pthread_mutex_t mutex;
#endif
};
#if 0
class FastCriticalSection
{
		static void yield()
		{
			::SleepEx(0, TRUE);
		}
		static long safeExchange(volatile long& target, long value)
		{
			return InterlockedExchange(&target, value);
		}
		static bool failStateLock(volatile long& state)
		{
			return safeExchange(state, 1) == 1;
		}
		static void unlockState(volatile long& state)
		{
			assert(state == 1);
			safeExchange(state, 0);
		}
		static void waitLockState(volatile long& state)
		{
			while (failStateLock(state))
			{
				yield();
			}
		}
	public:
		explicit FastCriticalSection() : state(0)
		{
		}
		void lock()
		{
			waitLockState(state);
		}
		void unlock()
		{
			unlockState(state);
		}
	private:
		volatile long state;
};
#endif

template<class T>
class LockBase
{
	public:
		explicit LockBase(T& aCs) : cs(aCs)
		{
			cs.lock();
		}
		~LockBase()
		{
			cs.unlock();
		}
	private:
		T& cs;
		DISALLOW_COPY_AND_ASSIGN(LockBase);
};
typedef LockBase<CriticalSection> Lock;
// typedef LockBase<FastCriticalSection> FastLock;

#endif // !defined(CRITICAL_SECTION_H)

