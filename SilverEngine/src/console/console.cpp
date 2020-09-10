#include "core.h"

#include "console_internal.h"

#include <iostream>
#include <stdarg.h>

#include "platform/platform_impl.h"

namespace sv {

	static std::mutex g_LogMutex;
	std::ofstream g_LogFile;

	std::string date_string(const Date& date)
	{
		std::stringstream stream;
		stream << '[';
		if (date.hour < 10u) stream << '0';
		stream << date.hour << ':';
		if (date.minute < 10u) stream << '0';
		stream << date.minute << ':';
		if (date.second < 10u) stream << '0';
		stream << date.second << ']';
		std::string res = stream.str();
		return res;
	}

	Result console_initialize(const InitializationConsoleDesc& desc)
	{
		if (desc.show) console_show();
		else console_hide();

		std::string logFile = desc.logFolder;
#ifdef SV_SRC_PATH
		logFile = SV_SRC_PATH + logFile;
#endif
		Date date = timer_date();

		std::string day;
		if (date.day < 10) day = '0';
		day += std::to_string(date.day);

		std::string month;
		if (date.month < 10) month = '0';
		month += std::to_string(date.month);

		std::string year = std::to_string(date.year);

		std::string hour;
		if (date.hour < 10) hour = '0';
		hour += std::to_string(date.hour);

		std::string minute;
		if (date.minute < 10) minute = '0';
		minute += std::to_string(date.minute);

		std::string second;
		if (date.second < 10) second = '0';
		second += std::to_string(date.second);

		logFile += '[' + day + '-' + month + '-' + year + "][" + hour + '-' + minute + '-' + second + "].log";

		g_LogFile.open(logFile.c_str());
		if (!g_LogFile.is_open()) return Result_NotFound;

		return Result_Success;
	}

	Result console_close()
	{
		g_LogFile.close();
		return Result_Success;
	}

	void console_show()
	{
		ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
	}
	void console_hide()
	{
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}

	void _Log(const char* s0, const char* s1, va_list args, ui32 flags)
	{
		std::string dateStr = date_string(timer_date());

		char logBuffer[1001];
		memcpy(logBuffer, dateStr.data(), dateStr.size());

		size_t s0Size = strlen(s0);
		memcpy(logBuffer + dateStr.size(), s0, s0Size);

		vsnprintf(logBuffer + dateStr.size() + s0Size, 1000 - dateStr.size() - s0Size, s1, args);

		size_t size = strlen(logBuffer);
		logBuffer[size] = '\n';
		logBuffer[size + 1u] = '\0';

#ifdef SV_PLATFORM_WINDOWS
		ui32 platformFlags = 0u;

		if (flags & LoggingFlags_Red)				platformFlags |= FOREGROUND_RED;
		if (flags & LoggingFlags_Green)				platformFlags |= FOREGROUND_GREEN;
		if (flags & LoggingFlags_Blue)				platformFlags |= FOREGROUND_BLUE;
		if (flags & LoggingFlags_BackgroundRed)		platformFlags |= BACKGROUND_RED;
		if (flags & LoggingFlags_BackgroundGreen)	platformFlags |= BACKGROUND_GREEN;
		if (flags & LoggingFlags_BackgroundBlue)	platformFlags |= BACKGROUND_BLUE;
#endif

		std::lock_guard<std::mutex> lock(g_LogMutex);

#ifdef SV_PLATFORM_WINDOWS
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), platformFlags);
#endif

		std::cout << logBuffer;
		g_LogFile << logBuffer;
	}

	void log_separator()
	{
		log("----------------------------------------------------");
	}

	void log(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("", s, args, LoggingFlags_Red | LoggingFlags_Green | LoggingFlags_Blue);

		va_end(args);
	}
	void log_ex(const char* s, ui32 flags, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("", s, args, flags);

		va_end(args);
	}
	void log_info(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[INFO] ", s, args, LoggingFlags_Green);

		va_end(args);
	}
	void log_warning(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[WARNING] ", s, args, LoggingFlags_Green | LoggingFlags_Blue);

		va_end(args);
	}
	void log_error(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		_Log("[ERROR] ", s, args, LoggingFlags_Red);

		va_end(args);
	}

}