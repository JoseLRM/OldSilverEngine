#pragma once

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
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <filesystem>

using namespace DirectX;

// types
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef int8_t		i8;
typedef int16_t		i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef float	f32;
typedef double	f64;

typedef wchar_t	wchar;

typedef void* WindowHandle;

typedef int		BOOL;
#define FALSE	0
#define TRUE	1

// limits

constexpr u8	u8_max	= std::numeric_limits<u8>::max();
constexpr u16	u16_max	= std::numeric_limits<u16>::max();
constexpr u32	u32_max	= std::numeric_limits<u32>::max();
constexpr u64	u64_max	= std::numeric_limits<u64>::max();

constexpr i8	i8_min		= std::numeric_limits<i8>::min();
constexpr i16	i16_min		= std::numeric_limits<i16>::min();
constexpr i32	i32_min		= std::numeric_limits<i32>::min();
constexpr i64	i64_min		= std::numeric_limits<i64>::min();

constexpr i8	i8_max = std::numeric_limits<i8>::max();
constexpr i16	i16_max = std::numeric_limits<i16>::max();
constexpr i32	i32_max = std::numeric_limits<i32>::max();
constexpr i64	i64_max = std::numeric_limits<i64>::max();

constexpr f32	f32_min = std::numeric_limits<f32>::min();
constexpr f64	f64_min = std::numeric_limits<f64>::min();

constexpr f32	f32_max = std::numeric_limits<f32>::max();
constexpr f64	f64_max = std::numeric_limits<f64>::max();

static_assert(std::numeric_limits<f32>::is_iec559, "IEEE 754 required");
constexpr f32	f32_inf = std::numeric_limits<f32>::infinity();
constexpr f32	f32_ninf = -1.f * std::numeric_limits<f32>::infinity();
static_assert(std::numeric_limits<f64>::is_iec559, "IEEE 754 required");
constexpr f64	f64_inf = std::numeric_limits<f64>::infinity();
constexpr f64	f64_ninf = -1.0 * std::numeric_limits<f64>::infinity();

// Misc

#define svCheck(x) do{ sv::Result __res__ = (x); if(sv::result_fail(__res__)) return __res__; }while(0)
#define SV_ZERO_MEMORY(dest, size) memset(dest, 0, size)
#define SV_BIT(x) 1ULL << x 
#define foreach(_it, _end) for (u32 _it = 0u; _it < u32(_end); ++_it)

#ifdef SV_PLATFORM_WIN
#	define SV_INLINE __forceinline
#else
#	define SV_INLINE inline
#endif

// Result

namespace sv {

	enum Result : u32 {
		Result_Success,
		Result_CloseRequest,
		Result_TODO,
		Result_UnknownError,
		Result_PlatformError,
		Result_GraphicsAPIError,
		Result_CompileError,
		Result_NotFound = 404u,
		Result_InvalidFormat,
		Result_InvalidUsage,
		Result_Unsupported,
		Result_UnsupportedVersion,
		Result_Duplicated,
	};

	inline bool result_okay(Result res) { return res == Result_Success; }
	inline bool result_fail(Result res) { return res != Result_Success; }
	
}

// Ptr Handle
#define SV_DEFINE_HANDLE(x) struct x { x() = delete; ~x() = delete; }

// Platform

namespace sv {

	enum MessageStyle : u32 {
		MessageStyle_None = 0u,

		MessageStyle_IconInfo = SV_BIT(0),
		MessageStyle_IconWarning = SV_BIT(1),
		MessageStyle_IconError = SV_BIT(2),

		MessageStyle_Ok = SV_BIT(3),
		MessageStyle_OkCancel = SV_BIT(4),
		MessageStyle_YesNo = SV_BIT(5),
		MessageStyle_YesNoCancel = SV_BIT(6),
	};
	typedef u32 MessageStyleFlags;

	int show_message(const wchar* title, const wchar* content, MessageStyleFlags style = MessageStyle_None);

	std::string file_dialog_open(u32 filterCount, const char** filters, const char* startPath);
	std::string file_dialog_save(u32 filterCount, const char** filters, const char* startPath);

}

// Assertion

namespace sv {
	void throw_assertion(const char* content, u32 line, const char* file);
}

#ifdef SV_ENABLE_ASSERTION
#define SV_ASSERT(x) do{ if (!(x)) { sv::throw_assertion(#x, __LINE__, __FILE__); } } while(0)
#else
#define SV_ASSERT(x)
#endif

// Logging

#ifdef SV_ENABLE_LOGGING

namespace sv {
	
	void __internal__do_not_call_this_please_or_you_will_die__console_log(u32 id, const char* s, ...);
	void __internal__do_not_call_this_please_or_you_will_die__console_clear();

}

#define SV_LOG_CLEAR() sv::__internal__do_not_call_this_please_or_you_will_die__console_clear()
#define SV_LOG_SEPARATOR() sv::__internal__do_not_call_this_please_or_you_will_die__console_log(0u, "----------------------------------------------------")
#define SV_LOG(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(0u, x, __VA_ARGS__)
#define SV_LOG_INFO(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(1u, "[INFO] " x, __VA_ARGS__)
#define SV_LOG_WARNING(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(2u, "[WARNING] " x, __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(3u, "[ERROR] " x, __VA_ARGS__)


#else

#define SV_LOG_CLEAR() do{}while(false)
#define SV_LOG_SEPARATOR() do{}while(false)
#define SV_LOG(x, ...) do{}while(false)
#define SV_LOG_INFO(x, ...) do{}while(false)
#define SV_LOG_WARNING(x, ...) do{}while(false)
#define SV_LOG_ERROR(x, ...) do{}while(false)

#endif

// Debug profiler

#ifdef SV_ENABLE_PROFILER

#define SV_PROFILER_SCALAR float

namespace sv {

	void				__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_begin(const char* name);
	void				__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_end(const char* name);
	float				__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(const char* name);

	void				__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_set(const char* name, SV_PROFILER_SCALAR value);
	void				__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(const char* name, SV_PROFILER_SCALAR value);
	void				__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(const char* name, SV_PROFILER_SCALAR value);
	SV_PROFILER_SCALAR	__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_get(const char* name);

}

#define SV_PROFILER(content) content

#define SV_PROFILER_CHRONO_BEGIN(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_begin(x)
#define SV_PROFILER_CHRONO_END(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_end(x)
#define SV_PROFILER_CHRONO_GET(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(x)

#ifdef SV_ENABLE_LOGGING
#define SV_PROFILER_CHRONO_LOG(x) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(5u, "[PROFILER] %s: %fms", x, sv::__internal__do_not_call_this_please_or_you_will_die__profiler_chrono_get(x))
#else
#define SV_PROFILER_CHRONO_LOG(x)
#endif

#define SV_PROFILER_SCALAR_SET(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_set(x, value)
#define SV_PROFILER_SCALAR_ADD(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(x, value)
#define SV_PROFILER_SCALAR_SUB(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_add(x, -(value))
#define SV_PROFILER_SCALAR_MUL(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(x, value)
#define SV_PROFILER_SCALAR_DIV(x, value) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_mul(x, 1.0/(value))
#define SV_PROFILER_SCALAR_GET(x) sv::__internal__do_not_call_this_please_or_you_will_die__profiler_scalar_get(x)

#else

#define SV_PROFILER(content) do{}while(false)

#define SV_PROFILER_CHRONO_BEGIN(x) do{}while(false)
#define SV_PROFILER_CHRONO_END(x) do{}while(false)
#define SV_PROFILER_CHRONO_GET(x) 0.0
#define SV_PROFILER_CHRONO_LOG(x) do{}while(false)

#define SV_PROFILER_SCALAR_SET(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_ADD(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_SUB(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_MUL(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_DIV(x, value) do{}while(false)
#define SV_PROFILER_SCALAR_GET(x) 0.0

#endif

// SilverEngine Includes

#include "core/utils/helper.h"
#include "core/utils/timer.h"
#include "core/utils/math.h"
