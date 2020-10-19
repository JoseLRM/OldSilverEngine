#include "core.h"

#include "logging_internal.h"

#include <iostream>
#include <stdarg.h>

#include "platform/platform_impl.h"

namespace sv {

	int show_message(const wchar* title, const wchar* content, MessageStyleFlags style)
	{
#ifdef SV_PLATFORM_WIN
		UINT flags = 0u;

		if (style & MessageStyle_IconInfo) flags |= MB_ICONINFORMATION;
		else if (style & MessageStyle_IconWarning) flags |= MB_ICONWARNING;
		else if (style & MessageStyle_IconError) flags |= MB_ICONERROR;

		if (style & MessageStyle_Ok) flags |= MB_OK;
		else if (style & MessageStyle_OkCancel) flags |= MB_OKCANCEL;
		else if (style & MessageStyle_YesNo) flags |= MB_YESNO;
		else if (style & MessageStyle_YesNoCancel) flags |= MB_YESNOCANCEL;

		int res = MessageBox(0, content, title, flags);

		if (style & MessageStyle_Ok) {
			if (res == IDOK) return 0;
			else return -1;
		}
		else if (style & MessageStyle_OkCancel) {
			switch (res)
			{
			case IDOK:
				return 0;
			case IDCANCEL:
				return 1;
			default:
				return -1;
			}
		}
		else if (style & MessageStyle_YesNo) {
			switch (res)
			{
			case IDYES:
				return 0;
			case IDNO:
				return 1;
			default:
				return -1;
			}
		}
		else if (style & MessageStyle_YesNoCancel) {
			switch (res)
			{
			case IDYES:
				return 0;
			case IDNO:
				return 1;
			case IDCANCEL:
				return 2;
			default:
				return -1;
			}
		}

		return res;
#endif
	}

#ifndef SV_ENABLE_LOGGING

	Result logging_initialize()
	{
#ifdef SV_PLATFORM_WIN
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		return Result_Success;
	}
	
	Result logging_close()
	{
		return Result_Success;
	}


#else
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

	Result logging_initialize()
	{
#ifdef SV_PLATFORM_WIN
		ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
#endif

		std::string logFolder = "logs/";
#ifdef SV_RES_PATH
		logFolder = SV_RES_PATH + logFolder;
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

		std::string logFile;
		logFile = '[' + day + '-' + month + '-' + year + "][" + hour + '-' + minute + '-' + second + "].log";

		std::string absPath = logFolder + logFile;

		g_LogFile.open(absPath.c_str());
		if (!g_LogFile.is_open()) {

			// Create logs folder
			if (std::filesystem::create_directories(logFolder)) return Result_UnknownError;

			g_LogFile.open(absPath.c_str());

			if (!g_LogFile.is_open()) 
				return Result_NotFound;
		}

		return Result_Success;
	}

	Result logging_close()
	{
		g_LogFile.close();
		return Result_Success;
	}

	void __internal__do_not_call_this_please_or_you_will_die__console_clear()
	{
		std::lock_guard<std::mutex> lock(g_LogMutex);
		system("CLS");
	}

	void log(const char* title, const char* s1, va_list args, ui32 id)
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

#ifdef SV_PLATFORM_WIN
		ui32 platformFlags = 0u;

		switch (id)
		{
		case 0u:
			platformFlags |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;

		case 1u:
			platformFlags |= FOREGROUND_GREEN;
			break;

		case 2u:
			platformFlags |= FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;

		case 3u:
			platformFlags |= FOREGROUND_RED;
			break;

		case 4u:
			platformFlags |= FOREGROUND_RED | FOREGROUND_BLUE;
			break;
		}
#endif

		std::lock_guard<std::mutex> lock(g_LogMutex);

#ifdef SV_PLATFORM_WIN
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), platformFlags);
#endif

		std::cout << logBuffer;
		g_LogFile << logBuffer;
	}

	void __internal__do_not_call_this_please_or_you_will_die__console_log(ui32 id, const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		log("", s, args, id);

		va_end(args);
	}

#endif

}