#pragma once

#include "..//config.h"

#ifndef SV_DEBUG
#define NDEBUG
#endif

// std includes
#include <math.h>
#include <DirectXMath.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <array>
#include <stdint.h>
#include <functional>
#include <mutex>
#include <atomic>
#include <fstream>

using namespace DirectX;

// types
typedef uint8_t		ui8;
typedef uint16_t	ui16;
typedef uint32_t	ui32;
typedef uint64_t	ui64;

typedef int8_t		i8;
typedef int16_t		i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef wchar_t		wchar;

typedef void*		WindowHandle;

typedef int			BOOL;
#define FALSE		0
#define TRUE		1

// Exception

namespace sv {
	struct Exception {
		std::string type, desc, file;
		ui32 line;
		Exception(const char* type, const char* desc, const char* file, ui32 line)
			: type(type), desc(desc), file(file), line(line) {}
	};
}
#define SV_THROW(type, desc) throw sv::Exception(type, desc, __FILE__, __LINE__)

// Console

namespace _sv {
	void console_show();
	void console_hide();
}

// macros

#ifdef SV_DEBUG
#define SV_ASSERT(x) do{ if((x) == false) SV_THROW("Assertion Failed!!", #x); }while(0)
#else
#define SV_ASSERT(x) x
#endif

#define svCheck(x) if(!(x)) do{ sv::log_error(#x); return false; }while(0)
#define svZeroMemory(dest, size) memset(dest, 0, size)
#define SV_BIT(x) 1ULL << x 

// logging

namespace sv {
	void log_separator();
	void log(const char* s, ...);
	void log_info(const char* s, ...);
	void log_warning(const char* s, ...);
	void log_error(const char* s, ...);
}

// SilverEngine Includes

#include "utils.h"