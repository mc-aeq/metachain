/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "functions.h"
#include <string>
#include <atomic>
#include <exception>
#include <openssl/crypto.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

#include "defines.h"
#include "tinyformat.h"

// thread renaming
void RenameThread(const char* name)
{
#if defined(PR_SET_NAME)
	// Only the first 15 characters are used (16 - NUL terminator)
	::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
	pthread_set_name_np(pthread_self(), name);

#elif defined(MAC_OSX)
	pthread_setname_np(name);
#else
	// Prevent warnings for unused parameters...
	(void)name;
#endif
}

void memory_cleanse(void *ptr, size_t len)
{
	OPENSSL_cleanse(ptr, len);
}

static std::atomic<int64_t> nMockTime(0); //!< For unit testing

// formatting exceptions
std::string FormatException(const std::exception* pex, const char* pszThread)
{
#ifdef _WIN32
	char pszModule[MAX_PATH] = "";
	GetModuleFileNameA(NULL, pszModule, sizeof(pszModule));
#else
	const char* pszModule = "TCT";
#endif
	if (pex)
		return strprintf(
			"EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
	else
		return strprintf(
			"UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
}

#ifdef _WIN32
std::string NetworkErrorString(int err)
{
	char buf[256];
	buf[0] = 0;
	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), NULL))
	{
		return strprintf("%s (%d)", buf, err);
	}
	else
	{
		return strprintf("Unknown error (%d)", err);
	}
}
#else
std::string NetworkErrorString(int err)
{
	char buf[256];
	buf[0] = 0;
	/* Too bad there are two incompatible implementations of the
	* thread-safe strerror. */
	const char *s;
#ifdef STRERROR_R_CHAR_P /* GNU variant can return a pointer outside the passed buffer */
	s = strerror_r(err, buf, sizeof(buf));
#else /* POSIX variant always returns message in buffer */
	s = buf;
	if (strerror_r(err, buf, sizeof(buf)))
		buf[0] = 0;
#endif
	return strprintf("%s (%d)", s, err);
}
#endif


bool SetSocketNonBlocking(SOCKET& hSocket, bool fNonBlocking)
{
	if (fNonBlocking)
	{
#ifdef _WIN32
		u_long nOne = 1;
		if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
		{
#else
		int fFlags = fcntl(hSocket, F_GETFL, 0);
		if (fcntl(hSocket, F_SETFL, fFlags | O_NONBLOCK) == SOCKET_ERROR)
		{
#endif
			return false;
		}
		}
	else
	{
#ifdef _WIN32
		u_long nZero = 0;
		if (ioctlsocket(hSocket, FIONBIO, &nZero) == SOCKET_ERROR)
		{
#else
		int fFlags = fcntl(hSocket, F_GETFL, 0);
		if (fcntl(hSocket, F_SETFL, fFlags & ~O_NONBLOCK) == SOCKET_ERROR)
		{
#endif
			return false;
		}
		}

	return true;
}

bool CloseSocket(SOCKET& hSocket)
{
	if (hSocket == INVALID_SOCKET)
		return false;
#ifdef _WIN32
	int ret = closesocket(hSocket);
#else
	int ret = close(hSocket);
#endif
	hSocket = INVALID_SOCKET;
	return ret != SOCKET_ERROR;
}

struct timeval MillisToTimeval(int64_t nTimeout)
{
	struct timeval timeout;
	timeout.tv_sec = nTimeout / 1000;
	timeout.tv_usec = (nTimeout % 1000) * 1000;
	return timeout;
}

bool SetSocketNoDelay(SOCKET& hSocket)
{
	int set = 1;
	int rc = setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&set, sizeof(int));
	return rc == 0;
}

int64_t GetTime()
{
	int64_t mocktime = nMockTime.load(std::memory_order_relaxed);
	if (mocktime) return mocktime;

	time_t now = time(NULL);
	assert(now > 0);
	return now;
}

int64_t GetTimeMillis()
{
	int64_t now = (boost::posix_time::microsec_clock::universal_time() -
		boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))).total_milliseconds();
	assert(now > 0);
	return now;
}

int64_t GetTimeMicros()
{
	int64_t now = (boost::posix_time::microsec_clock::universal_time() -
		boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))).total_microseconds();
	assert(now > 0);
	return now;
}

int64_t GetSystemTimeInSeconds()
{
	return GetTimeMicros() / 1000000;
}

void MilliSleep(int64_t n)
{

	/**
	* Boost's sleep_for was uninterruptible when backed by nanosleep from 1.50
	* until fixed in 1.52. Use the deprecated sleep method for the broken case.
	* See: https://svn.boost.org/trac/boost/ticket/7238
	*/
#if defined(HAVE_WORKING_BOOST_SLEEP_FOR)
	boost::this_thread::sleep_for(boost::chrono::milliseconds(n));
#elif defined(HAVE_WORKING_BOOST_SLEEP)
	boost::this_thread::sleep(boost::posix_time::milliseconds(n));
#else
	//should never get here
#error missing boost sleep implementation
#endif
}
