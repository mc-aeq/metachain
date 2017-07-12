#pragma once

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

#ifdef __clang__
// TL;DR Add GUARDED_BY(mutex) to member variables. The others are
// rarely necessary. Ex: int nFoo GUARDED_BY(cs_foo);
//
// See http://clang.llvm.org/docs/LanguageExtensions.html#threadsafety
// for documentation.  The clang compiler can do advanced static analysis
// of locking when given the -Wthread-safety option.
#define LOCKABLE __attribute__((lockable))
#define SCOPED_LOCKABLE __attribute__((scoped_lockable))
#define GUARDED_BY(x) __attribute__((guarded_by(x)))
#define GUARDED_VAR __attribute__((guarded_var))
#define PT_GUARDED_BY(x) __attribute__((pt_guarded_by(x)))
#define PT_GUARDED_VAR __attribute__((pt_guarded_var))
#define ACQUIRED_AFTER(...) __attribute__((acquired_after(__VA_ARGS__)))
#define ACQUIRED_BEFORE(...) __attribute__((acquired_before(__VA_ARGS__)))
#define EXCLUSIVE_LOCK_FUNCTION(...) __attribute__((exclusive_lock_function(__VA_ARGS__)))
#define SHARED_LOCK_FUNCTION(...) __attribute__((shared_lock_function(__VA_ARGS__)))
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) __attribute__((exclusive_trylock_function(__VA_ARGS__)))
#define SHARED_TRYLOCK_FUNCTION(...) __attribute__((shared_trylock_function(__VA_ARGS__)))
#define UNLOCK_FUNCTION(...) __attribute__((unlock_function(__VA_ARGS__)))
#define LOCK_RETURNED(x) __attribute__((lock_returned(x)))
#define LOCKS_EXCLUDED(...) __attribute__((locks_excluded(__VA_ARGS__)))
#define EXCLUSIVE_LOCKS_REQUIRED(...) __attribute__((exclusive_locks_required(__VA_ARGS__)))
#define SHARED_LOCKS_REQUIRED(...) __attribute__((shared_locks_required(__VA_ARGS__)))
#define NO_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))
#else
#define LOCKABLE
#define SCOPED_LOCKABLE
#define GUARDED_BY(x)
#define GUARDED_VAR
#define PT_GUARDED_BY(x)
#define PT_GUARDED_VAR
#define ACQUIRED_AFTER(...)
#define ACQUIRED_BEFORE(...)
#define EXCLUSIVE_LOCK_FUNCTION(...)
#define SHARED_LOCK_FUNCTION(...)
#define EXCLUSIVE_TRYLOCK_FUNCTION(...)
#define SHARED_TRYLOCK_FUNCTION(...)
#define UNLOCK_FUNCTION(...)
#define LOCK_RETURNED(x)
#define LOCKS_EXCLUDED(...)
#define EXCLUSIVE_LOCKS_REQUIRED(...)
#define SHARED_LOCKS_REQUIRED(...)
#define NO_THREAD_SAFETY_ANALYSIS
#endif // __GNUC__

#ifdef DEBUG_LOCKORDER
void EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false);
void LeaveCritical();
std::string LocksHeld();
void AssertLockHeldInternal(const char* pszName, const char* pszFile, int nLine, void* cs);
void DeleteLock(void* cs);
#else
void static inline EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false) {}
void static inline LeaveCritical() {}
void static inline AssertLockHeldInternal(const char* pszName, const char* pszFile, int nLine, void* cs) {}
void static inline DeleteLock(void* cs) {}
#endif
#define AssertLockHeld(cs) AssertLockHeldInternal(#cs, __FILE__, __LINE__, &cs)

/**
* Template mixin that adds -Wthread-safety locking
* annotations to a subset of the mutex API.
*/
template <typename PARENT>
class LOCKABLE AnnotatedMixin : public PARENT
{
public:
	void lock() EXCLUSIVE_LOCK_FUNCTION()
	{
		PARENT::lock();
	}

	void unlock() UNLOCK_FUNCTION()
	{
		PARENT::unlock();
	}

	bool try_lock() EXCLUSIVE_TRYLOCK_FUNCTION(true)
	{
		return PARENT::try_lock();
	}
};

// cCriticalSection Class directly from boost::recursive_mutex
class cCriticalSection : public AnnotatedMixin<boost::recursive_mutex>
{
public:
	~cCriticalSection() {
		DeleteLock((void*)this);
	}
};

/** Wrapper around boost::unique_lock<Mutex> */
template <typename Mutex>
class SCOPED_LOCKABLE cMutexLock
{
private:
	boost::unique_lock<Mutex> lock;

	void Enter(const char* pszName, const char* pszFile, int nLine)
	{
		EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()));
#ifdef DEBUG_LOCKCONTENTION
		if (!lock.try_lock()) {
			PrintLockContention(pszName, pszFile, nLine);
#endif
			lock.lock();
#ifdef DEBUG_LOCKCONTENTION
		}
#endif
	}

	bool TryEnter(const char* pszName, const char* pszFile, int nLine)
	{
		EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()), true);
		lock.try_lock();
		if (!lock.owns_lock())
			LeaveCritical();
		return lock.owns_lock();
	}

public:
	cMutexLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine, bool fTry = false) EXCLUSIVE_LOCK_FUNCTION(mutexIn) : lock(mutexIn, boost::defer_lock)
	{
		if (fTry)
			TryEnter(pszName, pszFile, nLine);
		else
			Enter(pszName, pszFile, nLine);
	}

	cMutexLock(Mutex* pmutexIn, const char* pszName, const char* pszFile, int nLine, bool fTry = false) EXCLUSIVE_LOCK_FUNCTION(pmutexIn)
	{
		if (!pmutexIn) return;

		lock = boost::unique_lock<Mutex>(*pmutexIn, boost::defer_lock);
		if (fTry)
			TryEnter(pszName, pszFile, nLine);
		else
			Enter(pszName, pszFile, nLine);
	}

	~cMutexLock() UNLOCK_FUNCTION()
	{
		if (lock.owns_lock())
			LeaveCritical();
	}

	operator bool()
	{
		return lock.owns_lock();
	}
};

// defines for usage
typedef cMutexLock<cCriticalSection> cCriticalBlock;
#define PASTE(x, y) x ## y
#define PASTE2(x, y) PASTE(x, y)

#define LOCK(cs) cCriticalBlock PASTE2(criticalblock, __COUNTER__)(cs, #cs, __FILE__, __LINE__)
#define LOCK2(cs1, cs2) cCriticalBlock criticalblock1(cs1, #cs1, __FILE__, __LINE__), criticalblock2(cs2, #cs2, __FILE__, __LINE__)
#define TRY_LOCK(cs, name) cCriticalBlock name(cs, #cs, __FILE__, __LINE__, true)