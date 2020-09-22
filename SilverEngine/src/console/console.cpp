#include "core.h"

#include "console_internal.h"

#include <iostream>
#include <stdarg.h>

#include "window/platform_impl.h"

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

	void log(const char* title, const char* s1, va_list args, ui32 flags, std::wstring* saveOutput = nullptr)
	{
		char logBuffer[1001];
		size_t offset = 0u;

		// Date
		{
			std::string dateStr = date_string(timer_date());
			memcpy(logBuffer, dateStr.data(), dateStr.size());
			offset += dateStr.size();
		}

		// Title
		{
			size_t titleSize = strlen(title);
			if (titleSize != 0u) {
				logBuffer[offset++] = '[';
				logBuffer[offset + titleSize] = ']';
				memcpy(logBuffer + offset, title, titleSize);
				offset += titleSize + 1u;
			}
		}

		// Content
		vsnprintf(logBuffer + offset, 1000 - offset, s1, args);

		size_t size = strlen(logBuffer);
		logBuffer[size] = '\n';
		logBuffer[size + 1u] = '\0';

		if (saveOutput) {
			*saveOutput = utils_wstring_parse(logBuffer + offset);
		}

#ifdef SV_PLATFORM_WINDOWS
		ui32 platformFlags = 0u;

		if (flags & LoggingStyle_Red)				platformFlags |= FOREGROUND_RED;
		if (flags & LoggingStyle_Green)				platformFlags |= FOREGROUND_GREEN;
		if (flags & LoggingStyle_Blue)				platformFlags |= FOREGROUND_BLUE;
		if (flags & LoggingStyle_BackgroundRed)		platformFlags |= BACKGROUND_RED;
		if (flags & LoggingStyle_BackgroundGreen)	platformFlags |= BACKGROUND_GREEN;
		if (flags & LoggingStyle_BackgroundBlue)	platformFlags |= BACKGROUND_BLUE;
#endif

		std::lock_guard<std::mutex> lock(g_LogMutex);

#ifdef SV_PLATFORM_WINDOWS
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), platformFlags);
#endif

		std::cout << logBuffer;
		g_LogFile << logBuffer;
	}

	void console_log_separator()
	{
		console_log("----------------------------------------------------");
	}

	void console_log(const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		log("", s, args, LoggingStyle_Red | LoggingStyle_Green | LoggingStyle_Blue);

		va_end(args);
	}

	void console_log(LoggingStyleFlags style, const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		log("", s, args, style);

		va_end(args);
	}

	void console_log_error(bool fatal, const char* title, const char* content, ...)
	{
		std::wstring contentStr;

		va_list args;
		va_start(args, content);

		log(title, content, args, LoggingStyle_Red, &contentStr);

		va_end(args);

#ifdef SV_PLATFORM_WINDOWS
		std::wstring titleStr = utils_wstring_parse(title);
		

		UINT flags = 0u;

		if (fatal) flags |= MB_OK | MB_ICONERROR;
		else flags |= MB_OK | MB_ICONWARNING;
			
		MessageBox(0, contentStr.c_str(), titleStr.c_str(), flags);
#endif
	}

}