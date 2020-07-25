#pragma once

#include "..//config.h"

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

namespace SV {
	namespace _internal {
		struct Exception {
			std::string type, desc, file;
			ui32 line;
			Exception(const char* type, const char* desc, const char* file, ui32 line)
				: type(type), desc(desc), file(file), line(line) {}
		};
	}
}
#define SV_THROW(type, desc) throw SV::_internal::Exception(type, desc, __FILE__, __LINE__)

// Console

namespace SV {
	namespace _internal {
		void ShowConsole();
		void HideConsole();
	}
}

// macros

#ifdef SV_DEBUG
#define SV_ASSERT(x) do{ if((x) == false) SV_THROW("Assertion Failed!!", #x); }while(0)
#else
#define SV_ASSERT(x) x
#endif

#define svCheck(x) if(!(x)) do{ SV::LogE(#x); return false; }while(0)
#define svZeroMemory(dest, size) memset(dest, 0, size)
#define SV_BIT(x) 1 << x

// logging

namespace SV {
	void LogSeparator();
	void Log(const char* s, ...);
	void LogI(const char* s, ...);
	void LogW(const char* s, ...);
	void LogE(const char* s, ...);
}

// Helper Functions

namespace SV {

	template<typename T>
	constexpr void HashCombine(size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	constexpr void HashString(size_t& seed, const char* str)
	{

	}

}

// SilverEngine Includes

#include "utils/Timer.h"
#include "utils/BinFile.h"
#include "utils/TxtFile.h"
#include "utils/SVMath.h"
#include "utils/Utils.h"
#include "utils/safe_queue.h"
#include "TaskSystem.h"
