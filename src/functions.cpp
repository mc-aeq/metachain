#include "../stdafx.h"

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
int64_t GetTime()
{
	int64_t mocktime = nMockTime.load(std::memory_order_relaxed);
	if (mocktime) return mocktime;

	time_t now = time(NULL);
	assert(now > 0);
	return now;
}

// formatting exceptions
std::string FormatException(const std::exception* pex, const char* pszThread)
{
#ifdef _WINDOWS
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

