#include "core.h"

#include <iostream>
#include <stdarg.h>

#ifdef SV_PLATFORM_WINDOWS
#include "platform/windows/windows_impl.h"
#endif

namespace _sv {

	void console_show()
	{
		ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
	}
	void console_hide()
	{
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}

}

namespace sv {

	static std::mutex g_LogMutex;

	void _Log(const char* s0, const char* s1, va_list args)
	{
		char logBuffer[1000];

		vsnprintf(logBuffer, 1000, s1, args);

		std::string out = s0;
		out += logBuffer;

		std::lock_guard<std::mutex> lock(g_LogMutex);
		std::cout << out << std::endl;
	}

	void log_separator()
	{
		log("----------------------------------------------------");
	}

	void log(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("", s, args);

		va_end(args);
	}
	void log_info(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[INFO] ", s, args);
		
		va_end(args);
	}
	void log_warning(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[WARNING] ", s, args);

		va_end(args);
	}
	void log_error(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[ERROR] ", s, args);

		va_end(args);
	}
}