#include "SilverEngine/core.h"

#include <iostream>
#include <stdarg.h>

#include "SilverEngine/platform/impl.h"
#include "SilverEngine/window.h"
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

		HWND hwnd = (HWND)window_handle(engine.window);

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

		g_LogFile.open(absPath.c_str(), true);
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

	///////////////////////////////////////////////// TIMER /////////////////////////////////////////////////

	static std::chrono::steady_clock::time_point g_InitialTime = std::chrono::steady_clock::now();

	static std::chrono::steady_clock::time_point timer_now_chrono()
	{
		return std::chrono::steady_clock::now();
	}

	Time timer_now()
	{
		return Time(std::chrono::duration<float>(timer_now_chrono() - g_InitialTime).count());
	}

	Date timer_date()
	{
		__time64_t t;
		_time64(&t);
		tm time;
		_localtime64_s(&time, &t);

		Date date;
		date.year = time.tm_year + 1900;
		date.month = time.tm_mon + 1;
		date.day = time.tm_mday;
		date.hour = time.tm_hour;
		date.minute = time.tm_min;
		date.second = time.tm_sec;

		return date;
	}

	///////////////////////////////////////////////// FILE MANAGEMENT /////////////////////////////////////////////////

#if defined(SV_RES_PATH) && defined(SV_SYS_PATH)

#define PARSE_FILEPATH() std::string filepath_str; \
	if (!sv::path_is_absolute(filepath)) { \
		if (filepath[0] == '$') { \
			filepath_str = SV_SYS_PATH; \	
			++filepath \
		} \
		else \
			filepath_str = SV_RES_PATH; \	
		filepath_str += filepath; \
		filepath = filepath_str.c_str(); \
	} 

#elif defined(SV_SYS_PATH) 

#define PARSE_FILEPATH() std::string filepath_str; \
	if (!sv::path_is_absolute(filepath)) { \
		if (filepath[0] == '$') { \
			filepath_str = SV_SYS_PATH; \	
			filepath_str += filepath + 1u; \
			filepath = filepath_str.c_str(); \
		} \
	} 

#elif defined(SV_RES_PATH)

#define PARSE_FILEPATH() std::string filePathStr; \
	if (!sv::path_is_absolute(filePath) && *filepath != '$') { \
		filePathStr = SV_RES_PATH; \
		filePathStr += filePath; \
		filePath = filePathStr.c_str(); \
	} 

#else

#define PARSE_FILEPATH() if (!sv::path_is_absolute(filepath)) { \
		if (*filepath == '$') ++filepath; \
	} \

#endif

	bool path_is_absolute(const char* filepath)
	{
		return filepath[0] != '\0' && filepath[1] == ':';
	}
	void path_clear(char* path)
	{
		while (*path != '\0') {

			switch (*path)
			{
			case '//':
				*path = '/';
				break;

			case '\\':
				*path = '/';
				break;
			}

			++path;
		}
	}

	bool path_is_absolute(const wchar* filepath)
	{
		return filepath[0] != L'\0' && filepath[1] == L':';
	}

	Result file_read_binary(const char* filepath, u8** pData, size_t* pSize)
	{
		PARSE_FILEPATH();

		std::ifstream stream;
		stream.open(filepath, std::ios::binary | std::ios::ate);
		if (!stream.is_open())
			return Result_NotFound;

		*pSize = stream.tellg();
		*pData = new u8[*pSize];

		stream.seekg(0u);

		stream.read((char*)* pData, *pSize);

		stream.close();

		return Result_Success;
	}

	Result file_read_binary(const char* filepath, std::vector<u8>& data)
	{
		PARSE_FILEPATH();

		std::ifstream stream;
		stream.open(filepath, std::ios::binary | std::ios::ate);
		if (!stream.is_open())
			return Result_NotFound;

		data.resize(stream.tellg());

		stream.seekg(0u);

		stream.read((char*)data.data(), data.size());

		stream.close();

		return Result_Success;
	}

	Result file_read_text(const char* filepath, std::string& str)
	{
		PARSE_FILEPATH();

		std::ifstream stream;
		stream.open(filepath, std::ios::ate);
		if (!stream.is_open())
			return Result_NotFound;

		str.resize(stream.tellg());

		stream.seekg(0u);

		stream.read(str.data(), str.size());

		stream.close();

		return Result_Success;
	}

	Result file_write_binary(const char* filepath, const u8* data, size_t size, bool append)
	{
		PARSE_FILEPATH();

		std::ofstream stream;
		stream.open(filepath, std::ios::binary | (append ? std::ios::app : 0u));
		if (!stream.is_open())
			return Result_NotFound;

		stream.write((const char*)data, size);

		stream.close();

		return Result_Success;
	}

	Result file_write_text(const char* filepath, const char* str, size_t size, bool append)
	{
		PARSE_FILEPATH();

		std::ofstream stream;
		stream.open(filepath, (append ? std::ios::app : 0u));
		if (!stream.is_open())
			return Result_NotFound;

		stream.write(str, size);

		stream.close();

		return Result_Success;
	}

	Result file_remove(const char* filepath)
	{
		PARSE_FILEPATH();
		return (remove(filepath) != 0) ? Result_NotFound : Result_Success;
	}

	Result FileO::open(const char* filepath, bool text, bool append)
	{
		close();

		PARSE_FILEPATH();

		u32 f = 0u;

		if (append) f |= std::ios::app;
		if (!text) f |= std::ios::binary;

		stream.open(filepath, f);

		if (stream.is_open()) {
			return Result_Success;
		}
		else return Result_NotFound;
	}

	void FileO::close()
	{
		stream.close();
	}

	bool FileO::isOpen()
	{
		return stream.is_open();
	}

	void FileO::write(const u8* data, size_t size)
	{
		stream.write((const char*)data, size);
	}

	void FileO::writeLine(const char* str)
	{
		stream.write(str, strlen(str));
	}

	void FileO::writeLine(const std::string& str)
	{
		stream.write(str.data(), str.size());
	}

	Result FileI::open(const char* filePath, bool text)
	{
		close();

		PARSE_FILEPATH();

		u32 f = 0u;

		if (!text) f |= std::ios::binary;

		stream.open(filePath, f);

		if (stream.is_open()) {
			return Result_Success;
		}
		else return Result_NotFound;
	}

	void FileI::close()
	{
		stream.close();
	}

	bool FileI::isOpen()
	{
		return stream.is_open();
	}

	void FileI::setPos(size_t pos)
	{
		stream.seekg(pos);
	}

	size_t FileI::getPos()
	{
		return stream.tellg();
	}

	size_t FileI::getSize()
	{
		size_t pos = stream.tellg();
		stream.seekg(stream.end);
		size_t size = stream.tellg();
		stream.seekg(pos);
		return size;
	}

	void FileI::read(u8* data, size_t size)
	{
		stream.read((char*)data, size);
	}

	bool FileI::readLine(char* buf, size_t bufSize)
	{
		return (bool)stream.getline((char*)buf, bufSize);
	}

	bool FileI::readLine(std::string& str)
	{
		return (bool)std::getline(stream, str);
	}

	ArchiveO::ArchiveO() : m_Capacity(0u), m_Size(0u), m_Data(nullptr)
	{}

	ArchiveO::~ArchiveO()
	{
		free();
	}

	void ArchiveO::reserve(size_t size)
	{
		if (m_Size + size > m_Capacity) {

			size_t newSize = size_t(double(m_Size + size) * 1.5);
			allocate(newSize);

		}
	}

	void ArchiveO::write(const void* data, size_t size)
	{
		reserve(size);

		memcpy(m_Data + m_Size, data, size);
		m_Size += size;
	}

	void ArchiveO::erase(size_t size)
	{
		SV_ASSERT(size <= m_Size);
		m_Size -= size;
	}

	void ArchiveO::clear()
	{
		m_Size = 0u;
	}

	Result ArchiveO::save_file(const char* filePath, bool append)
	{
		return file_write_binary(filePath, m_Data, m_Size, append);
	}

	void ArchiveO::allocate(size_t size)
	{
		u8* newData = (u8*)operator new(size);
		if (m_Data) {
			memcpy(newData, m_Data, m_Size);
			operator delete[](m_Data);
		}
		m_Capacity = size;
		m_Data = newData;
	}

	void ArchiveO::free()
	{
		if (m_Data) {
			operator delete[](m_Data);
			m_Data = nullptr;
			m_Size = 0u;
			m_Capacity = 0u;
		}
	}

	// INPUT

	ArchiveI::ArchiveI() : m_Size(0u), m_Pos(0u), m_Data(nullptr)
	{
	}

	ArchiveI::~ArchiveI()
	{
		clear();
	}

	Result ArchiveI::open_file(const char* filePath)
	{
		return file_read_binary(filePath, &m_Data, &m_Size);
	}

	void ArchiveI::read(void* data, size_t size)
	{
		if (m_Pos + size > m_Size) {
			size_t invalidSize = (m_Pos + size) - m_Size;
			SV_ZERO_MEMORY((u8*)data + size - invalidSize, invalidSize);
			size -= invalidSize;
			SV_LOG_WARNING("Archive reading, out of bounds");
		}
		memcpy(data, m_Data + m_Pos, size);
		m_Pos += size;
	}

	void ArchiveI::clear()
	{
		if (m_Data) {
			operator delete[](m_Data);
			m_Data = nullptr;
			m_Size = 0u;
			m_Pos = 0u;
		}
	}

}

#define STBI_ASSERT(x) SV_ASSERT(x)
#define STBI_MALLOC(size) malloc(size)
#define STBI_REALLOC(ptr, size) realloc(ptr, size)
#define STBI_FREE(ptr) free(ptr)
#define STB_IMAGE_IMPLEMENTATION

#include "external/stbi_lib.h"

namespace sv {

	Result load_image(const char* filePath, void** pdata, u32* width, u32* height)
	{
		int w = 0, h = 0, bits = 0;

#ifdef SV_RES_PATH
		std::string filePathStr = SV_RES_PATH;
		filePathStr += filePath;
		void* data = stbi_load(filePathStr.c_str(), &w, &h, &bits, 4);
#else
		void* data = stbi_load(filePath, &w, &h, &bits, 4);
#endif

		* pdata = nullptr;
		*width = w;
		*height = h;

		if (!data) return Result_NotFound;
		*pdata = data;
		return Result_Success;
	}

	std::string bin_filepath(size_t hash)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		return filePath;
	}

	Result bin_read(size_t hash, std::vector<u8>& data)
	{
		std::string filePath = bin_filepath(hash);
		return file_read_binary(filePath.c_str(), data);
	}

	Result bin_read(size_t hash, ArchiveI& archive)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		svCheck(archive.open_file(filePath.c_str()));
		return Result_Success;
	}

	Result bin_write(size_t hash, const void* data, size_t size)
	{
		std::string filePath = bin_filepath(hash);
		return file_write_binary(filePath.c_str(), (u8*)data, size);
	}

	Result bin_write(size_t hash, ArchiveO& archive)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		svCheck(archive.save_file(filePath.c_str()));
		return Result_Success;
	}

	// CHARACTER UTILS

	std::wstring parse_wstring(const char* c)
	{
		std::wstring str;
		str.resize(strlen(c));
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, c, int(str.size()), &str[0], int(str.size()));
		return str;
	}

	std::string parse_string(const wchar* c)
	{
		size_t size = wcslen(c) + 1u;
		std::string str;
		str.resize(size);
		size_t i;
		wcstombs_s(&i, str.data(), size, c, size);
		return str;
	}

}