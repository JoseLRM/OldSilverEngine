#include "SilverEngine/core.h"

#include <iostream>
#include <stdarg.h>

#include "SilverEngine/platform/impl.h"
#include "SilverEngine/window.h"

#ifdef SV_PLATFORM_WIN
#include <commdlg.h> // File Dialogs
#endif

namespace sv {

	///////////////////////////////////////////////// ASSERTION /////////////////////////////////////////////////

	void throw_assertion(const char* content, u32 line, const char* file)
	{
		console_notify("ASSERTION", "'%s', file: '%s', line: %u", content, file, line);
		
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

	SV_INLINE static std::string file_dialog(u32 filterCount, const char** filters, const char* filepath, bool open)
	{
		std::stringstream absFilterStream;

		for (u32 i = 0; i < filterCount; ++i) {
			absFilterStream << filters[i * 2u];
			absFilterStream << '\0';
			absFilterStream << filters[i * 2u + 1u];
			absFilterStream << '\0';
		}

		std::string absFilter = std::move(absFilterStream.str());

		SV_PARSE_FILEPATH();

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
		file.lpstrInitialDir = filepath;
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

	extern v2_f32 next_mouse_position;
	extern bool new_mouse_position;

	void set_cursor_position(Window* window, f32 x, f32 y)
	{
		x = std::max(std::min(x, 0.5f), -0.5f) + 0.5f;
		y = std::max(std::min(y, 0.5f), -0.5f) + 0.5f;
		next_mouse_position = { x, y };
		new_mouse_position = true;
	}

	void system_pause()
	{
		bool show_console = IsWindowVisible(GetConsoleWindow());

#ifdef SV_PLATFORM_WIN
		
		if (!show_console) {

			ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
		}
		
		system("pause");

		if (!show_console) {

			ShowWindow(GetConsoleWindow(), SW_HIDE);
		}
#endif

	}

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
		SV_PARSE_FILEPATH();

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
		SV_PARSE_FILEPATH();

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
		SV_PARSE_FILEPATH();

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
		SV_PARSE_FILEPATH();

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
		SV_PARSE_FILEPATH();

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
		SV_PARSE_FILEPATH();
		return (remove(filepath) != 0) ? Result_NotFound : Result_Success;
	}

	Result FileO::open(const char* filepath, bool text, bool append)
	{
		close();

		SV_PARSE_FILEPATH();

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

	Result FileI::open(const char* filepath, bool text)
	{
		close();

		SV_PARSE_FILEPATH();

		u32 f = 0u;

		if (!text) f |= std::ios::binary;

		stream.open(filepath, f);

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

	Archive::Archive() : _capacity(0u), _size(0u), _data(nullptr), _pos(0u)
	{}

	Archive::~Archive()
	{
		free();
	}

	void Archive::reserve(size_t size)
	{
		if (_size + size > _capacity) {

			size_t newSize = size_t(double(_size + size) * 1.5);

			bool write_version = false;

			if (_size == 0u) {
				newSize += sizeof(Version);
				write_version = true;
			}

			allocate(newSize);

			if (write_version) {
				memcpy(_data, &engine.version, sizeof(Version));
				_size = sizeof(Version);
			}
		}
	}

	void Archive::write(const void* data, size_t size)
	{
		reserve(size);

		memcpy(_data + _size, data, size);
		_size += size;
	}

	void Archive::read(void* data, size_t size)
	{
		if (_pos + size > _size) {
			size_t invalidSize = (_pos + size) - _size;
			SV_ZERO_MEMORY((u8*)data + size - invalidSize, invalidSize);
			size -= invalidSize;
			SV_LOG_WARNING("Archive reading, out of bounds");
		}
		memcpy(data, _data + _pos, size);
		_pos += size;
	}

	void Archive::erase(size_t size)
	{
		SV_ASSERT(size <= _size);
		_size -= size;
	}

	Result Archive::openFile(const char* filePath)
	{
		_pos = 0u;
		svCheck(file_read_binary(filePath, &_data, &_size));
		if (_size < sizeof(Version)) return Result_InvalidFormat;
		this->operator>>(version);
		return Result_Success;
	}

	Result Archive::saveFile(const char* filePath, bool append)
	{
		return file_write_binary(filePath, _data, _size, append);
	}

	void Archive::allocate(size_t size)
	{
		u8* newData = (u8*)operator new(size);

		if (_data) {
			memcpy(newData, _data, _size);
			operator delete[](_data);
		}

		_data = newData;
		_capacity = size;
	}

	void Archive::free()
	{
		if (_data) {
			operator delete[](_data);
			_data = nullptr;
			_size = 0u;
			_capacity = 0u;
			_pos = 0u;
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

	Result load_image(const char* filepath, void** pdata, u32* width, u32* height)
	{
		SV_PARSE_FILEPATH();

		int w = 0, h = 0, bits = 0;
		void* data = stbi_load(filepath, &w, &h, &bits, 4);

		* pdata = nullptr;
		*width = w;
		*height = h;

		if (!data) return Result_NotFound;
		*pdata = data;
		return Result_Success;
	}

	std::string bin_filepath(size_t hash)
	{
		return "bin/" + std::to_string(hash) + ".bin";
	}

	Result bin_read(size_t hash, std::vector<u8>& data)
	{
		std::string filepath = bin_filepath(hash);
		return file_read_binary(filepath.c_str(), data);
	}

	Result bin_read(size_t hash, Archive& archive)
	{
		std::string filepath = "bin/" + std::to_string(hash) + ".bin";
		svCheck(archive.openFile(filepath.c_str()));
		return Result_Success;
	}

	Result bin_write(size_t hash, const void* data, size_t size)
	{
		std::string filepath = bin_filepath(hash);
		return file_write_binary(filepath.c_str(), (u8*)data, size);
	}

	Result bin_write(size_t hash, Archive& archive)
	{
		std::string filepath = bin_filepath(hash);
		svCheck(archive.saveFile(filepath.c_str()));
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