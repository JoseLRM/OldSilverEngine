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

// Misc

#define svCheck(x) do{ sv::Result __res__ = (x); if(sv::result_fail(__res__)) return __res__; }while(0)
#define svZeroMemory(dest, size) memset(dest, 0, size)
#define SV_BIT(x) 1ULL << x 

// Result

namespace sv {

	enum Result : ui32 {
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

	enum MessageStyle : ui32 {
		MessageStyle_None = 0u,

		MessageStyle_IconInfo = SV_BIT(0),
		MessageStyle_IconWarning = SV_BIT(1),
		MessageStyle_IconError = SV_BIT(2),

		MessageStyle_Ok = SV_BIT(3),
		MessageStyle_OkCancel = SV_BIT(4),
		MessageStyle_YesNo = SV_BIT(5),
		MessageStyle_YesNoCancel = SV_BIT(6),
	};
	typedef ui32 MessageStyleFlags;

	int show_message(const wchar* title, const wchar* content, MessageStyleFlags style = MessageStyle_None);

}

// Assertion

namespace sv {
	void throw_assertion(const char* content, ui32 line, const char* file);
}

#ifdef SV_ENABLE_ASSERTION
#define SV_ASSERT(x) do{ if (!(x)) { sv::throw_assertion(#x, __LINE__, __FILE__); } } while(0)
#else
#define SV_ASSERT(x)
#endif

// Logging

#ifdef SV_ENABLE_LOGGING

namespace sv {
	
	void __internal__do_not_call_this_please_or_you_will_die__console_log(ui32 id, const char* s, ...);
	void __internal__do_not_call_this_please_or_you_will_die__console_clear();

}

#define SV_LOG_CLEAR() sv::__internal__do_not_call_this_please_or_you_will_die__console_clear()
#define SV_LOG_SEPARATOR() sv::__internal__do_not_call_this_please_or_you_will_die__console_log(0u, "----------------------------------------------------")
#define SV_LOG(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(0u, x, __VA_ARGS__)
#define SV_LOG_INFO(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(1u, "[INFO] " x, __VA_ARGS__)
#define SV_LOG_WARNING(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(2u, "[WARNING] " x, __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(3u, "[ERROR] " x, __VA_ARGS__)


#else

#define SV_LOG_CLEAR()
#define SV_LOG_SEPARATOR()
#define SV_LOG()
#define SV_LOG_INFO()
#define SV_LOG_WARNING()
#define SV_LOG_ERROR()

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

#define SV_PROFILER(content)

#define SV_PROFILER_CHRONO_BEGIN(x)
#define SV_PROFILER_CHRONO_END(x)
#define SV_PROFILER_CHRONO_GET(x)
#define SV_PROFILER_CHRONO_LOG(x)

#define SV_PROFILER_SCALAR_SET(x, value)
#define SV_PROFILER_SCALAR_ADD(x, value)
#define SV_PROFILER_SCALAR_SUB(x, value)
#define SV_PROFILER_SCALAR_MUL(x, value)
#define SV_PROFILER_SCALAR_DIV(x, value)
#define SV_PROFILER_SCALAR_GET(x)

#endif

// SilverEngine Includes

#include "utils/helper.h"
#include "utils/timer.h"
#include "utils/math.h"
#include "utils/Version.h"
#include "platform/window.h"

// Engine callbacks

namespace sv {

	struct ApplicationCallbacks {
		Result(*initialize)();
		void(*update)(float);
		void(*render)();
		Result(*close)();
	};

	struct InitializationDesc {

		ApplicationCallbacks	callbacks;
		vec4u					windowBounds;
		const wchar*			windowTitle;
		WindowStyle				windowStyle;
		const wchar*			iconFilePath;
		ui32					minThreadsCount;
		const char*				assetsFolderPath;

	};

	Result engine_initialize(const InitializationDesc* desc);
	Result engine_loop();
	Result engine_close();

	Version	engine_version_get() noexcept;
	float	engine_deltatime_get() noexcept;
	ui64	engine_frame_count() noexcept;

	void engine_animations_enable();
	void engine_animations_disable();
	bool engine_animations_is_enabled();

}