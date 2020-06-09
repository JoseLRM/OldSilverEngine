#pragma once

#include "..//config.h"

// std includes
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <queue>
#include <stdint.h>
#include <functional>
#include <mutex>
#include <atomic>
#include <math.h>
#include <DirectXMath.h>

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

// macros
#ifdef SV_DEBUG
#define SV_ASSERT(x) do{ if((x) == false) SV_THROW("Assertion Failed!!", #x); }while(false)
#else
#define SV_ASSERT(x)
#endif

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

// SilverEngine Includes
#include "ImGuiManager.h"
#include "Math.h"
#include "Utils.h"
#include "Scene.h"
#include "safe_queue.h"
#include "TaskSystem.h"
#include "Engine.h"