#include "core.h"

#include <iostream>
#include <stdarg.h>

namespace SV {
	void _Log(const char* s0, const char* s1, va_list args)
	{
		char logBuffer[1000];

		vsnprintf(logBuffer, 1000, s1, args);

		std::string out = s0;
		out += logBuffer;

		std::cout << out << std::endl;
	}

	void Log(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("", s, args);

		va_end(args);
	}
	void LogI(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[INFO] ", s, args);
		
		va_end(args);
	}
	void LogW(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[WARNING] ", s, args);

		va_end(args);
	}
	void LogE(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[ERROR] ", s, args);

		va_end(args);
	}
}