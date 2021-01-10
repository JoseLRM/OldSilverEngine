#include "core.h"

#include <iostream>
#include <stdarg.h>

#include "core/platform/impl.h"
#include "core/window.h"
#include "core/utils/io.h"
#include "core_internal.h"

#ifdef SV_PLATFORM_WIN

#include <commdlg.h> // File Dialogs

#endif

namespace sv {

	///////////////////////////////////////////////// PLATFORM /////////////////////////////////////////////////

#ifdef SV_PLATFORM_WIN

	int show_message(const wchar* title, const wchar* content, MessageStyleFlags style)
	{
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
	}

	inline std::string file_dialog(u32 filterCount, const char** filters, const char* startPath, bool open)
	{
		std::stringstream absFilterStream;

		for (u32 i = 0; i < filterCount; ++i) {
			absFilterStream << filters[i * 2u];
			absFilterStream << '\0';
			absFilterStream << filters[i * 2u + 1u];
			absFilterStream << '\0';
		}

		std::string absFilter = std::move(absFilterStream.str());

#ifdef SV_RES_PATH
		std::string startPathStr = SV_RES_PATH;
		startPathStr += startPath;
		startPath = startPathStr.c_str();
#endif

		HWND hwnd = (HWND)window_handle_get();

		OPENFILENAMEA file;
		SV_ZERO_MEMORY(&file, sizeof(OPENFILENAMEA));

		constexpr size_t FILE_MAX_SIZE = 300u;
		char lpstrFile[FILE_MAX_SIZE] = {};

		file.lStructSize = sizeof(OPENFILENAMEA);
		file.hwndOwner = hwnd;
		file.lpstrFilter = absFilter.c_str();
		file.nFilterIndex = 1u;
		file.lpstrFile = lpstrFile;
		file.lpstrInitialDir = startPath;
		file.nMaxFile = FILE_MAX_SIZE;
		file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		BOOL result;

		if (open) result = GetOpenFileNameA(&file);
		else result = GetSaveFileNameA(&file);

		if (result == TRUE) {
			path_clear(lpstrFile);
			return lpstrFile;
		}
		else return std::string();
	}

	std::string file_dialog_open(u32 filterCount, const char** filters, const char* startPath)
	{
		return file_dialog(filterCount, filters, startPath, true);
	}

	std::string file_dialog_save(u32 filterCount, const char** filters, const char* startPath)
	{
		return file_dialog(filterCount, filters, startPath, false);
	}

#endif

	///////////////////////////////////////////////// ASSERTION /////////////////////////////////////////////////

	void throw_assertion(const char* content, u32 line, const char* file)
	{
#ifdef SV_ENABLE_LOGGING
		__internal__do_not_call_this_please_or_you_will_die__console_log(4u, "[ASSERTION] '%s', file: '%s', line: %u", content, file, line);
#endif
		std::wstringstream ss;
		ss << L"'";
		ss << parse_wstring(content);
		ss << L"'. File: '";
		ss << parse_wstring(file);
		ss << L"'. Line: " << line;
		ss << L". Do you want to continue?";

		if (sv::show_message(L"ASSERTION!!", ss.str().c_str(), sv::MessageStyle_IconError | sv::MessageStyle_YesNo) == 1)
		{
			exit(1u);
		}
	}

	///////////////////////////////////////////////// LOGGING /////////////////////////////////////////////////

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
	static FileO g_LogFile;

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

		g_LogFile.open(absPath.c_str(), FileOpen_Text);
		if (!g_LogFile.isOpen())
			return Result_NotFound;

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

	void log(const char* title, const char* s1, va_list args, u32 id)
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
		u32 platformFlags = 0u;

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

		case 5u:
			platformFlags |= FOREGROUND_RED | FOREGROUND_GREEN;
			break;
		}
#endif

		std::lock_guard<std::mutex> lock(g_LogMutex);

#ifdef SV_PLATFORM_WIN
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), platformFlags);
#endif

		std::cout << logBuffer;
		g_LogFile.writeLine(logBuffer);
	}

	void __internal__do_not_call_this_please_or_you_will_die__console_log(u32 id, const char* s, ...)
	{
		va_list args;
		va_start(args, s);

		log("", s, args, id);

		va_end(args);
	}

#endif

	///////////////////////////////////////////////// PROFILER /////////////////////////////////////////////////

#ifdef SV_ENABLE_PROFILER

	struct ChronoProfile {
		f32 beginTime;
		f32 endTime;
		f32 resTime;
	};

	struct ScalarProfile {
		SV_PROFILER_SCALAR value;
	};

	static std::unordered_map<std::string, ChronoProfile>		g_ProfilerChrono;
	static std::shared_mutex									g_ProfilerChronoMutex;
	static std::unordered_map<const char*, ScalarProfile>		g_ProfilerScalar;
	static std::mutex											g_ProfilerScalarMutex;

	void __internal__do_not_call_this_please_or_you_will_die__profiler_chrono_begin(const char* name)
	{
		std::scoped_lock lock(g_ProfilerChronoMutex);
		g_ProfilerChrono[name].beginTime = timer_now();
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_chrono_end(const char* name)
	{
		std::scoped_lock lock(g_ProfilerChronoMutex);
		auto& prof = g_ProfilerChrono[name];
		prof.endTime = timer_now();
		prof.resTime = prof.endTime - prof.beginTime;
	}

	float __internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(const char* name)
	{
		std::shared_lock lock(g_ProfilerChronoMutex);
		auto& prof = g_ProfilerChrono[name];
		return prof.resTime;
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_set(const char* name, SV_PROFILER_SCALAR value)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		g_ProfilerScalar[name].value = value;
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(const char* name, SV_PROFILER_SCALAR value)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		g_ProfilerScalar[name].value += value;
	}

	void __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(const char* name, SV_PROFILER_SCALAR value)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		g_ProfilerScalar[name].value *= value;
	}

	SV_PROFILER_SCALAR __internal__do_not_call_this_please_or_you_will_die__profiler_scalar_get(const char* name)
	{
		std::scoped_lock lock(g_ProfilerScalarMutex);
		return g_ProfilerScalar[name].value;
	}

#endif
}
