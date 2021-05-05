#pragma once

#define SV_VERSION_MAJOR 0
#define SV_VERSION_MINOR 0
#define SV_VERSION_REVISION 0

// std includes
#include <math.h>
#include <DirectXMath.h>
#include <string>
#include <map>
#include <unordered_map>
#include <queue>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <fstream>
#include <algorithm>

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

// limits

constexpr u8	 u8_max	        = std::numeric_limits<u8>::max();
constexpr u16	 u16_max	= std::numeric_limits<u16>::max();
constexpr u32	 u32_max	= std::numeric_limits<u32>::max();
constexpr u64	 u64_max	= std::numeric_limits<u64>::max();
constexpr size_t size_t_max	= std::numeric_limits<size_t>::max();

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

constexpr u32 FILEPATH_SIZE = 259u;
constexpr u32 FILENAME_SIZE = 50u;
constexpr u32 SCENENAME_SIZE = 50u;

#define SV_CHECK(exp) if ((exp) == false) return false;
#define SV_ZERO_MEMORY(dest, size) memset(dest, 0, size)
#define SV_BIT(x) (1ULL << x) 
#define foreach(_it, _end) for (u32 _it = 0u; _it < u32(_end); ++_it)
#define SV_MIN(a, b) ((a < b) ? a : b)
#define SV_MAX(a, b) ((a > b) ? a : b)
#define SV_AUX SV_INLINE static
#define SV_INTERNAL static

#define SV_USER extern "C" __declspec(dllexport)

#if SV_SILVER_ENGINE
#define SV_API __declspec(dllexport)
#else
#define SV_API __declspec(dllimport)
#endif

#if SV_PLATFORM_WIN
#define SV_INLINE __forceinline
#else
#define SV_INLINE inline
#endif

namespace sv {

    // Memory (Platform specific)
    
    SV_API void* _allocate_memory(size_t size);
    SV_API void _free_memory(void* ptr);

#define SV_ALLOCATE_MEMORY(size) sv::_allocate_memory(size)
#define SV_FREE_MEMORY(ptr) sv::_free_memory(ptr)
    
#define SV_ALLOCATE_STRUCT(type) new(SV_ALLOCATE_MEMORY(sizeof(type))) type()
#define SV_ALLOCATE_STRUCT_ARRAY(type, count) new(SV_ALLOCATE_MEMORY(sizeof(type) * size_t(count))) type[size_t(count)]

    template <typename T>
    SV_INLINE void destruct(T& t) { t.~T(); }
    
#define SV_FREE_STRUCT(ptr) do { destruct(*ptr); SV_FREE_MEMORY(ptr); } while(0)
#define SV_FREE_STRUCT_ARRAY(ptr, count) do { foreach(i, count) destruct(ptr[i]); SV_FREE_MEMORY(ptr); } while(0)

    // Version

    struct SV_API Version {
	u32 major = 0u;
	u32 minor = 0u;
	u32 revision = 0u;

	constexpr Version() = default;
	constexpr Version(u32 major, u32 minor, u32 revision)
	: major(major), minor(minor), revision(revision) {}

	u64 getVersion() const noexcept
	{
	    return u64(major) * 1000000L + u64(minor) * 1000L + u64(revision);
	}

	bool operator== (const Version& other) const noexcept
	{
	    return major == other.major && minor == other.minor && revision == other.revision;
	}
	bool operator!= (const Version& other) const noexcept
	{
	    return !this->operator==(other);
	}
	bool operator< (const Version& other) const noexcept
	{
	    if (major != other.major) return major < other.major;
	    else if (minor != other.minor) return minor < other.minor;
	    else return revision < other.revision;
	}
	bool operator> (const Version& other) const noexcept
	{
	    if (major != other.major) return major > other.major;
	    else if (minor != other.minor) return minor > other.minor;
	    else return revision > other.revision;
	}
	bool operator<= (const Version& other) const noexcept {
	    return this->operator<(other) || this->operator==(other);
	}
	bool operator>= (const Version& other) const noexcept {
	    return this->operator>(other) || this->operator==(other);
	}

    };

    
}
