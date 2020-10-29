#pragma once

#ifdef NDEBUG
#define NDEBUG
#endif

#ifdef SV_RES_PATH
#define SV_RES_PATH_W L"" SV_RES_PATH
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
#include <shared_mutex>
#include <atomic>
#include <fstream>
#include <filesystem>

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

// limits

constexpr ui8	ui8_max		= std::numeric_limits<ui8>::max();
constexpr ui16	ui16_max	= std::numeric_limits<ui16>::max();
constexpr ui32	ui32_max	= std::numeric_limits<ui32>::max();
constexpr ui64	ui64_max	= std::numeric_limits<ui64>::max();

constexpr i8	i8_min		= std::numeric_limits<i8>::min();
constexpr i16	i16_min		= std::numeric_limits<i16>::min();
constexpr i32	i32_min		= std::numeric_limits<i32>::min();
constexpr i64	i64_min		= std::numeric_limits<i64>::min();

constexpr i8	i8_max		= std::numeric_limits<i8>::max();
constexpr i16	i16_max		= std::numeric_limits<i16>::max();
constexpr i32	i32_max		= std::numeric_limits<i32>::max();
constexpr i64	i64_max		= std::numeric_limits<i64>::max();

constexpr float	float_min = std::numeric_limits<float>::min();
constexpr float	float_max = std::numeric_limits<float>::max();

namespace sv {

	// Result

	enum Result : ui32 {
		Result_Success,
		Result_CloseRequest,
		Result_TODO,
		Result_UnknownError,
		Result_PlatformError,
		Result_GraphicsAPIError,
		Result_CompileError,
		Result_NotFound,
		Result_InvalidFormat,
		Result_InvalidUsage,
		Result_UnsupportedVersion,
		Result_Duplicated,
	};

	inline bool result_okay(Result res) { return res == Result_Success; }
	inline bool result_fail(Result res) { return res != Result_Success; }
	
	// Ptr Handle
#define SV_DEFINE_HANDLE(x) struct x { x() = delete; ~x() = delete; }
}

// macros

namespace sv {
	void throw_assertion(const char* content, ui32 line, const char* file);
}

#ifdef SV_ENABLE_ASSERTION
#define SV_ASSERT(x) do{ if (!(x)) { sv::throw_assertion(#x, __LINE__, __FILE__); } } while(0)
#else
#define SV_ASSERT(x)
#endif

#define svCheck(x) do{ sv::Result __res__ = (x); if(sv::result_fail(__res__)) return __res__; }while(0)
#define svZeroMemory(dest, size) memset(dest, 0, size)
#define SV_BIT(x) 1ULL << x 

// SilverEngine Includes

#include "utils/helper.h"
#include "utils/timer.h"
#include "utils/math.h"
#include "main/logging.h"