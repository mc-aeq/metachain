#pragma once

// thread renaming
void RenameThread(const char* name);

// Attempt to overwrite data in the specified memory span.
void memory_cleanse(void *ptr, size_t len);

// format exceptions
extern std::string FormatException(const std::exception* pex, const char* pszThread);

// format network errors
extern std::string NetworkErrorString(int err);

// set the socket to non blocking
extern bool SetSocketNonBlocking(SOCKET& hSocket, bool fNonBlocking);

// function to close a socket
extern bool CloseSocket(SOCKET& hSocket);

// function that converts milli seconds to timeval struct
extern struct timeval MillisToTimeval(int64_t nTimeout);

// function to set a socket to non delay operation mode
extern bool SetSocketNoDelay(SOCKET& hSocket);

// some time and date functions
int64_t GetTime();
int64_t GetTimeMillis();
int64_t GetTimeMicros();
int64_t GetSystemTimeInSeconds(); // Like GetTime(), but not mockable
void MilliSleep(int64_t n);

// thread tracing
template <typename Callable> void TraceThread(const char* name, Callable func)
{
	string s = "TCT-" + string(name);
	RenameThread(s.c_str());
	try
	{
#ifdef _DEBUG
		LOG_DEBUG("thread start - " + s, "TraceThread");
#endif
		func();
#ifdef _DEBUG
		LOG_DEBUG("thread exit - " + s, "TraceThread");
#endif
	}
	catch (const boost::thread_interrupted&)
	{
#ifdef _DEBUG
		LOG_DEBUG("thread interrupt - " + s, "TraceThread");
#endif
		throw;
	}
	catch (const std::exception& e) {
		LOG_ERROR(FormatException(&e, name), "TraceThread");
		throw;
	}
	catch (...) {
		LOG_ERROR(FormatException(NULL, name), "TraceThread");
		throw;
	}
}

// selectable socket - always true for windows
bool static inline IsSelectableSocket(SOCKET s) {
#ifdef _WINDOWS
	return true;
#else
	return (s < FD_SETSIZE);
#endif
}