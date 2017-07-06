#pragma once

// thread renaming
void RenameThread(const char* name);

// Attempt to overwrite data in the specified memory span.
void memory_cleanse(void *ptr, size_t len);

// get current time
int64_t GetTime();

extern std::string FormatException(const std::exception* pex, const char* pszThread);

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