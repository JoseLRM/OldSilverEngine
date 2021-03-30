#pragma once

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

typedef wchar_t	wchar;

typedef void* WindowHandle;

typedef int		BOOL;
#define FALSE	0
#define TRUE	1

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

#define svCheck(x) do{ sv::Result __res__ = (x); if(sv::result_fail(__res__)) return __res__; }while(0)
#define SV_ZERO_MEMORY(dest, size) memset(dest, 0, size)
#define SV_BIT(x) (1ULL << x) 
#define foreach(_it, _end) for (u32 _it = 0u; _it < u32(_end); ++_it)
#define SV_MIN(a, b) ((a < b) ? a : b)
#define SV_MAX(a, b) ((a > b) ? a : b)
#define SV_AUX SV_INLINE static
#define internal static

#define SV_USER extern "C" __declspec(dllexport)

#if SV_SILVER_ENGINE
#define SV_API __declspec(dllexport)
#else
#define SV_API
#endif

#if SV_PLATFORM_WIN
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

    SV_INLINE bool result_okay(Result res) { return res == Result_Success; }
    SV_INLINE bool result_fail(Result res) { return res != Result_Success; }
    SV_INLINE const char* result_str(Result res)
    {
	switch (res)
	{
	case sv::Result_Success:
	    return "Success";
	case sv::Result_CloseRequest:
	    return "Close request";
	case sv::Result_TODO:
	    return "TODO";
	case sv::Result_UnknownError:
	    return "Unknown error";
	case sv::Result_PlatformError:
	    return "Platform error";
	case sv::Result_GraphicsAPIError:
	    return "Graphics API error";
	case sv::Result_CompileError:
	    return "Compile error";
	case sv::Result_NotFound:
	    return "Not found";
	case sv::Result_InvalidFormat:
	    return "Invalid format";
	case sv::Result_InvalidUsage:
	    return "Invalid usage";
	case sv::Result_Unsupported:
	    return "Unsupported";
	case sv::Result_UnsupportedVersion:
	    return "Unsupported version";
	case sv::Result_Duplicated:
	    return "Duplicated";
	default:
	    return "Unknown result...";
	}
    }

}

// Assertion

namespace sv {
    SV_API void throw_assertion(const char* content, u32 line, const char* file);
}

#if SV_SLOW
#define SV_ASSERT(x) do{ if (!(x)) { sv::throw_assertion(#x, __LINE__, __FILE__); } } while(0)
#else
#define SV_ASSERT(x)
#endif

// Console

#if SV_SLOW

namespace sv {
	
    typedef Result(*CommandFn)(const char** args, u32 argc);
    
    SV_API Result execute_command(const char* command);
    SV_API Result register_command(const char* name, CommandFn command_fn);

    SV_API void console_print(const char* str, ...);
    SV_API void console_notify(const char* title, const char* str, ...);
    SV_API void console_clear();

}

#define SV_LOG_CLEAR() sv::console_clear()
#define SV_LOG_SEPARATOR() sv::console_print("----------------------------------------------------\n")
#define SV_LOG(x, ...) sv::console_print(x "\n", __VA_ARGS__)
#define SV_LOG_INFO(x, ...) sv::console_notify("INFO", x, __VA_ARGS__)
#define SV_LOG_WARNING(x, ...) sv::console_notify("WARNING", x, __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) sv::console_notify("ERROR", x, __VA_ARGS__)

#else

#define SV_LOG_CLEAR() do{}while(0)
#define SV_LOG_SEPARATOR() do{}while(0)
#define SV_LOG(x, ...) do{}while(0)
#define SV_LOG_INFO(x, ...) do{}while(0)
#define SV_LOG_WARNING(x, ...) printf("[WARNING] " x "\n", __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) printf("[ERROR] " x "\n", __VA_ARGS__)

#endif

// Hash functions

namespace sv {

    template<typename T>
    constexpr void hash_combine(size_t& seed, const T& v)
    {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    SV_INLINE size_t hash_string(const char* str)
    {
	// TEMPORAL
	size_t res = 0u;
	size_t strLength = strlen(str);
	hash_combine(res, strLength);
	while (*str != '\0') {
	    hash_combine(res, (const size_t)* str);
	    ++str;
	}
	return res;
    }

// Timer

    class Time {
	f64 m_Time;
    public:
	constexpr Time() : m_Time(0.0) {}
	constexpr Time(f64 t) : m_Time(t) {}

	SV_INLINE operator f64() const noexcept { return m_Time; }

	SV_INLINE Time timeSince(Time time) const noexcept { return Time(time.m_Time - m_Time); }

	SV_INLINE f64 toSeconds_f64() const noexcept { return m_Time; }
	SV_INLINE u32 toSeconds_u32() const noexcept { return u32(m_Time); }
	SV_INLINE u64 toSeconds_u64() const noexcept { return u64(m_Time); }
	SV_INLINE f64 toMillis_f64() const noexcept { return m_Time * 1000.0; }
	SV_INLINE u32 toMillis_u32() const noexcept { return u32(m_Time * 1000.0); }
	SV_INLINE u64 toMillis_u64() const noexcept { return u64(m_Time * 1000.0); }
    };

    struct Date {
	u32 year;
	u32 month;
	u32 day;
	u32 hour;
	u32 minute;
	u32 second;
    };

    SV_API Time timer_now();
    SV_API Date timer_date();

// Version

    struct Version {
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

#include "utils/List.h"

// Math

#define ToRadians(x) x * 0.0174533f
#define ToDegrees(x) x / 0.0174533f
#define PI 3.14159f

namespace sv {

    SV_INLINE f32 math_sqrt(f32 n)
    {
	i32 i;
	f32 x, y;
	x = n * 0.5f;
	y = n;
	i = *(i32*)& y;
	i = 0x5f3759df - (i >> 1);
	y = *(f32*)& i;
	y = y * (1.5f - (x * y * y));
	y = y * (1.5f - (x * y * y));
	return n * y;
    }

    SV_INLINE f64 math_sqrt(f64 n)
    {
	i64 i;
	f64 x, y;
	x = n * 0.5;
	y = n;
	i = *(i64*)& y;
	i = 0x5f3759df - (i >> 1);
	y = *(f64*)& i;
	y = y * (1.5 - (x * y * y));
	y = y * (1.5 - (x * y * y));
	return n * y;
    }

    template<typename T>
    constexpr T math_gauss(T x, T sigma) noexcept
    {
	return ((T)1.f / math_sqrt((T)2.f * (T)PI * sigma * sigma)) * exp(-(x * x) / ((T)2.f * sigma * sigma));
    }

    // Vector Structs

    template<typename T, typename floatType>
    struct Vector2D;
    template<typename T, typename floatType>
    struct Vector3D;
    template<typename T, typename floatType>
    struct Vector4D;

    template<typename T, typename floatType>
    struct Vector2D {

	T x;
	T y;

	using vec = Vector2D<T, floatType>;
	using vec3 = Vector3D<T, floatType>;
	using vec4 = Vector4D<T, floatType>;

	constexpr Vector2D() : x(), y() {}
	constexpr Vector2D(T n) : x(n), y(n) {}
	constexpr Vector2D(T x, T y) : x(x), y(y) {}
	explicit constexpr Vector2D(const XMVECTOR& v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)) {}

	SV_INLINE bool operator==(const vec& v) const noexcept
	    {
		return x == v.x && y == v.y;
	    }
	SV_INLINE bool operator!=(const vec& v) const noexcept
	    {
		return x != v.x && y != v.y;
	    }

	SV_INLINE void operator+=(const vec& v) noexcept
	    {
		x += v.x;
		y += v.y;
	    }

	SV_INLINE void operator+=(const T v) noexcept
	    {
		x += v;
		y += v;
	    }

	SV_INLINE void operator-=(const vec& v) noexcept
	    {
		x -= v.x;
		y -= v.y;
	    }

	SV_INLINE void operator-=(const T v) noexcept
	    {
		x -= v;
		y -= v;
	    }

	SV_INLINE void operator*=(const vec& v) noexcept
	    {
		x *= v.x;
		y *= v.y;
	    }

	SV_INLINE void operator*=(const T v) noexcept
	    {
		x *= v;
		y *= v;
	    }

	SV_INLINE void operator/=(const vec& v) noexcept
	    {
		x /= v.x;
		y /= v.y;
	    }

	SV_INLINE void operator/=(const T v) noexcept
	    {
		x /= v;
		y /= v;
	    }

	// methods

	constexpr floatType length() const noexcept
	    {
		return math_sqrt(floatType(x) * floatType(x) + floatType(y) * floatType(y));
	    }

	SV_INLINE void normalize() noexcept
	    {
		floatType m = length();
		x /= m;
		y /= m;
	    }

	SV_INLINE vec vecTo(const vec& other) const noexcept
	    {
		return other - *this;
	    }

	SV_INLINE floatType distanceTo(const vec& other) const noexcept
	    {
		return vecTo(other).length();
	    }

	SV_INLINE floatType angle() const noexcept
	    {
		floatType res = atan2(floatType(y), floatType(x));
		if (res < 0.0) {
		    res = (2.0 * PI) + res;
		}
		return res;
	    }

	SV_INLINE void setAngle(floatType angle) noexcept
	    {
		floatType mag = length();
		x = cos(angle);
		x *= mag;
		y = sin(angle);
		y *= mag;
	    }

	SV_INLINE floatType dot(const vec& v) const noexcept
	    {
		return x * v.x + y * v.y;
	    }

	SV_INLINE vec perpendicular(bool clockwise = true) const noexcept
	    {
		if (clockwise) {
		    return { y, -x };
		}
		else {
		    return { -y, x };
		}
	    }

	// normal must be normalized
	SV_INLINE vec reflection(const vec& normal) const noexcept
	    {
		SV_ASSERT(normal.length() == 1.f);
		return *this - 2.0 * dot(normal) * normal;
	    }

	// setters
	SV_INLINE void set(T x, T y) noexcept
	    {
		*this = { x, y };
	    }
	SV_INLINE void setVec3(const vec3& v) noexcept
	    {
		*this = { v.x, v.y };
	    }
	SV_INLINE void setVec4(const vec4& v) noexcept
	    {
		*this = { v.x, v.y };
	    }
	SV_INLINE void setDX(const XMVECTOR& v) noexcept
	    {
		*this = { XMVectorGetX(v), XMVectorGetY(v) };
	    }

	// getters
	SV_INLINE vec3 getVec3(T z = 0) const noexcept
	    {
		return { x, y, z };
	    }
	SV_INLINE vec4 getVec4(T z = 0, T w = 0) const noexcept
	    {
		return { x, y, z, w };
	    }
	SV_INLINE XMVECTOR getDX(T z = 0, T w = 0) const noexcept
	    {
		return XMVectorSet(x, y, z, w);
	    }

	SV_INLINE vec _xx() const noexcept { return vec{ x, x }; }
	//SV_INLINE vec _xy() const noexcept { return vec{ x, y }; }
	SV_INLINE vec _yx() const noexcept { return vec{ y, x }; }
	SV_INLINE vec _yy() const noexcept { return vec{ y, y }; }

	static SV_INLINE vec up() { return { 0, 1 }; }
	static SV_INLINE vec down() { static_assert(std::numeric_limits<T>::is_signed(), "This method doesn't support unsigned types"); return { 0, -1 }; }
	static SV_INLINE vec left() { static_assert(std::numeric_limits<T>::is_signed(), "This method doesn't support unsigned types"); return { -1, 0 }; }
	static SV_INLINE vec right() { return { 1, 0 }; }
	static SV_INLINE vec zero() { return { 0, 0 }; }
	static SV_INLINE vec one() { return { 1, 1 }; }
	static SV_INLINE vec infinity() { return vec{ std::numeric_limits<floatType>::infinity() }; }
	static SV_INLINE vec negativeInfinity() { static_assert(std::numeric_limits<T>::is_signed(), "This method doesn't support unsigned types"); return vec{ -1. * std::numeric_limits<floatType>::infinity() }; }

    };

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator+(const Vector2D<T, floatType>& v0, const Vector2D<T, floatType>& v1) noexcept
    {
	return Vector2D<T, floatType>(v0.x + v1.x, v0.y + v1.y);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator+(const Vector2D<T, floatType>& v, const T n) noexcept
    {
	return Vector2D<T, floatType>(v.x + n, v.y + n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator+(const T n, const Vector2D<T, floatType>& v) noexcept
    {
	return Vector2D<T, floatType>(v.x + n, v.y + n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator-(const Vector2D<T, floatType>& v0, const Vector2D<T, floatType>& v1) noexcept
    {
	return Vector2D<T, floatType>(v0.x - v1.x, v0.y - v1.y);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator-(const Vector2D<T, floatType>& v, const T n) noexcept
    {
	return Vector2D<T, floatType>(v.x - n, v.y - n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator-(const T n, const Vector2D<T, floatType>& v) noexcept
    {
	return Vector2D<T, floatType>(v.x - n, v.y - n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator*(const Vector2D<T, floatType>& v0, const Vector2D<T, floatType>& v1) noexcept
    {
	return Vector2D<T, floatType>(v0.x * v1.x, v0.y * v1.y);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator*(const Vector2D<T, floatType>& v, const T n) noexcept
    {
	return Vector2D<T, floatType>(v.x * n, v.y * n);
    }
    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator*(const T n, const Vector2D<T, floatType>& v) noexcept
    {
	return Vector2D<T, floatType>(v.x * n, v.y * n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator/(const Vector2D<T, floatType>& v0, const Vector2D<T, floatType>& v1) noexcept
    {
	return Vector2D<T, floatType>(v0.x / v1.x, v0.y / v1.y);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator/(const Vector2D<T, floatType>& v, const T n) noexcept
    {
	return Vector2D<T, floatType>(v.x / n, v.y / n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector2D<T, floatType> operator/(const T n, const Vector2D<T, floatType>& v) noexcept
    {
	return Vector2D<T, floatType>(v.x / n, v.y / n);
    }


    // VECTOR 3D
    template<typename T, typename floatType>
    struct Vector3D {

	T x;
	T y;
	T z;

	using vec = Vector3D<T, floatType>;
	using vec2 = Vector2D<T, floatType>;
	using vec4 = Vector4D<T, floatType>;

	constexpr Vector3D() : x(), y(), z() {}
	constexpr Vector3D(T n) : x(n), y(n), z(n) {}
	constexpr Vector3D(T x, T y, T z) : x(x), y(y), z(z) {}
	explicit constexpr Vector3D(const XMVECTOR& v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)), z(XMVectorGetZ(v)) {}

	SV_INLINE void operator+=(const vec& v) noexcept
	    {
		x += v.x;
		y += v.y;
		z += v.z;
	    }

	SV_INLINE void operator+=(const T v) noexcept
	    {
		x += v;
		y += v;
		z += v;
	    }

	SV_INLINE void operator-=(const vec& v) noexcept
	    {
		x -= v.x;
		y -= v.y;
		z -= v.z;
	    }

	SV_INLINE void operator-=(const T v) noexcept
	    {
		x -= v;
		y -= v;
		z -= v;
	    }

	SV_INLINE void operator*=(const vec& v) noexcept
	    {
		x *= v.x;
		y *= v.y;
		z *= v.z;
	    }

	SV_INLINE void operator*=(const T v) noexcept
	    {
		x *= v;
		y *= v;
		z *= v;
	    }

	SV_INLINE void operator/=(const vec& v) noexcept
	    {
		x /= v.x;
		y /= v.y;
		z /= v.z;
	    }

	SV_INLINE void operator/=(const T v) noexcept
	    {
		x /= v;
		y /= v;
		z /= v;
	    }

	constexpr floatType length() const noexcept
	    {
		return math_sqrt(x * x + y * y + z * z);
	    }

	SV_INLINE void normalize() noexcept
	    {
		floatType m = length();
		x /= m;
		y /= m;
		z /= m;
	    }
	SV_INLINE vec vecTo(const vec& v) const noexcept
	    {
		return v - *this;
	    }
	SV_INLINE floatType distanceTo(const vec& v) const noexcept
	    {
		return vecTo(v).length();
	    }

	SV_INLINE floatType dot(const vec& v) const noexcept
	    {
		return x * v.x + y * v.y + z * v.z;
	    }

	SV_INLINE vec cross(const vec& v) const noexcept
	    {
		vec r;
		r.x = y * v.z - z * v.y;
		r.y = z * v.x - x * v.z;
		r.z = x * v.y - y * v.x;
		return r;
	    }

	// normal should be normalized
	SV_INLINE vec reflection(const vec& normal) const noexcept
	    {
		SV_ASSERT(normal.length() == 1.f);
		return 2 * dot(normal) * normal - *this;
	    }

	// setters
	SV_INLINE void set(T x, T y, T z) noexcept
	    {
		*this = { x, y, z };
	    }
	SV_INLINE void setVec2(const vec2& v, T z = 0) noexcept
	    {
		*this = { v.x, v.y, z };
	    }
	SV_INLINE void setVec4(const vec4& v) noexcept
	    {
		*this = { v.x, v.y, v.z };
	    }
	SV_INLINE void setDX(const XMVECTOR& v) noexcept
	    {
		*this = { XMVectorGetX(v), XMVectorGetY(v), XMVectorGetZ(v) };
	    }

	// getters
	SV_INLINE vec2 getVec2() const noexcept
	    {
		return { x, y };
	    }
	SV_INLINE vec4 getVec4(T w = 0) const noexcept
	    {
		return { x, y, z, w };
	    }
	SV_INLINE XMVECTOR getDX(T w = 0) const noexcept
	    {
		return XMVectorSet(x, y, z, w);
	    }

	SV_INLINE vec2 _xx() const noexcept { return vec2{ x, x }; }
	SV_INLINE vec2 _xy() const noexcept { return vec2{ x, y }; }
	SV_INLINE vec2 _xz() const noexcept { return vec2{ x, z }; }
	SV_INLINE vec2 _yx() const noexcept { return vec2{ y, x }; }
	SV_INLINE vec2 _yy() const noexcept { return vec2{ y, y }; }
	SV_INLINE vec2 _yz() const noexcept { return vec2{ y, z }; }
	SV_INLINE vec2 _zx() const noexcept { return vec2{ z, x }; }
	SV_INLINE vec2 _zy() const noexcept { return vec2{ z, y }; }
	SV_INLINE vec2 _zz() const noexcept { return vec2{ z, z }; }

	SV_INLINE vec _xxx() const noexcept { return vec{ x, x, x }; }
	SV_INLINE vec _xxy() const noexcept { return vec{ x, x, y }; }
	SV_INLINE vec _xxz() const noexcept { return vec{ x, x, z }; }
	SV_INLINE vec _xyx() const noexcept { return vec{ x, y, x }; }
	SV_INLINE vec _xyy() const noexcept { return vec{ x, y, y }; }
	// SV_INLINE vec _xyz() const noexcept { return vec{ x, y, z }; }
	SV_INLINE vec _xzx() const noexcept { return vec{ x, z, x }; }
	SV_INLINE vec _xzy() const noexcept { return vec{ x, z, y }; }
	SV_INLINE vec _xzz() const noexcept { return vec{ x, z, z }; }

	SV_INLINE vec _yxx() const noexcept { return vec{ y, x, x }; }
	SV_INLINE vec _yxy() const noexcept { return vec{ y, x, y }; }
	SV_INLINE vec _yxz() const noexcept { return vec{ y, x, z }; }
	SV_INLINE vec _yyx() const noexcept { return vec{ y, y, x }; }
	SV_INLINE vec _yyy() const noexcept { return vec{ y, y, y }; }
	SV_INLINE vec _yyz() const noexcept { return vec{ y, y, z }; }
	SV_INLINE vec _yzx() const noexcept { return vec{ y, z, x }; }
	SV_INLINE vec _yzy() const noexcept { return vec{ y, z, y }; }
	SV_INLINE vec _yzz() const noexcept { return vec{ y, z, z }; }

	SV_INLINE vec _zxx() const noexcept { return vec{ z, x, x }; }
	SV_INLINE vec _zxy() const noexcept { return vec{ z, x, y }; }
	SV_INLINE vec _zxz() const noexcept { return vec{ z, x, z }; }
	SV_INLINE vec _zyx() const noexcept { return vec{ z, y, x }; }
	SV_INLINE vec _zyy() const noexcept { return vec{ z, y, y }; }
	SV_INLINE vec _zyz() const noexcept { return vec{ z, y, z }; }
	SV_INLINE vec _zzx() const noexcept { return vec{ z, z, x }; }
	SV_INLINE vec _zzy() const noexcept { return vec{ z, z, y }; }
	SV_INLINE vec _zzz() const noexcept { return vec{ z, z, z }; }

	static SV_INLINE vec up() { return { 0, 1, 0 }; }
	static SV_INLINE vec down() { static_assert(std::numeric_limits<T>::is_signed(), "This method doesn't support unsigned types"); return { 0, -1, 0 }; }
	static SV_INLINE vec left() { static_assert(std::numeric_limits<T>::is_signed(), "This method doesn't support unsigned types"); return { -1, 0, 0 }; }
	static SV_INLINE vec right() { return { 1, 0, 0 }; }
	static SV_INLINE vec forward() { return { 0, 0, 1 }; }
	static SV_INLINE vec back() { static_assert(std::numeric_limits<T>::is_signed(), "This method doesn't support unsigned types"); return { 0, 0, -1 }; }
	static SV_INLINE vec zero() { return { 0, 0, 0 }; }
	static SV_INLINE vec one() { return { 1, 1, 1 }; }
	static SV_INLINE vec infinity() { return vec{ std::numeric_limits<floatType>::infinity() }; }
	static SV_INLINE vec negativeInfinity() { static_assert(std::numeric_limits<T>::is_signed(), "This method doesn't support unsigned types"); return vec{ -1. * std::numeric_limits<floatType>::infinity() }; }

    };

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator+(const Vector3D<T, floatType>& v0, const Vector3D<T, floatType>& v1) noexcept
    {
	return Vector3D<T, floatType>(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator+(const Vector3D<T, floatType>& v, const T n) noexcept
    {
	return Vector3D<T, floatType>(v.x + n, v.y + n, v.z + n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator+(const T n, const Vector3D<T, floatType>& v) noexcept
    {
	return Vector3D<T, floatType>(v.x + n, v.y + n, v.z + n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator-(const Vector3D<T, floatType>& v0, const Vector3D<T, floatType>& v1) noexcept
    {
	return Vector3D<T, floatType>(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator-(const Vector3D<T, floatType>& v, const T n) noexcept
    {
	return Vector3D<T, floatType>(v.x - n, v.y - n, v.z - n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator-(const T n, const Vector3D<T, floatType>& v) noexcept
    {
	return Vector3D<T, floatType>(v.x - n, v.y - n, v.z - n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator*(const Vector3D<T, floatType>& v0, const Vector3D<T, floatType>& v1) noexcept
    {
	return Vector3D<T, floatType>(v0.x * v1.x, v0.y * v1.y, v0.z * v1.z);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator*(const Vector3D<T, floatType>& v, const T n) noexcept
    {
	return Vector3D<T, floatType>(v.x * n, v.y * n, v.z * n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator*(const T n, const Vector3D<T, floatType>& v) noexcept
    {
	return Vector3D<T, floatType>(v.x * n, v.y * n, v.z * n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator/(const Vector3D<T, floatType>& v0, const Vector3D<T, floatType>& v1) noexcept
    {
	return Vector3D<T, floatType>(v0.x / v1.x, v0.y / v1.y, v0.z / v1.z);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator/(const Vector3D<T, floatType>& v, const T n) noexcept
    {
	return Vector3D<T, floatType>(v.x / n, v.y / n, v.z / n);
    }

    template<typename T, typename floatType>
    SV_INLINE Vector3D<T, floatType> operator/(const T n, const Vector3D<T, floatType>& v) noexcept
    {
	return Vector3D<T, floatType>(v.x / n, v.y / n, v.z / n);
    }

    // VECTOR 4D

    // TODO: Vector4 are incomplete
    template<typename T, typename floatType>
    struct Vector4D {

	T x;
	T y;
	T z;
	T w;

	using vec = Vector4D<T, floatType>;
	using vec2 = Vector2D<T, floatType>;
	using vec3 = Vector3D<T, floatType>;

	constexpr Vector4D() : x(), y(), z(), w() {}
	constexpr Vector4D(T n) : x(n), y(n), z(n), w(n) {}
	constexpr Vector4D(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
	explicit constexpr Vector4D(const XMVECTOR& v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)), z(XMVectorGetZ(v)), w(XMVectorGetW(v)) {}

	// sum
	inline void operator+=(const vec& v) noexcept
	    {
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
	    }
	inline vec operator+(const vec& v) const noexcept
	    {
		return vec(x + v.x, y + v.y, z + v.z, w + v.w);
	    }
	inline void operator+=(const T v) noexcept
	    {
		x += v;
		y += v;
		z += v;
		w += v;
	    }
	inline vec operator+(const T v) const noexcept
	    {
		return vec(x + v, y + v, z + v, w + v);
	    }

	// substract
	inline void operator-=(const vec& v) noexcept
	    {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
	    }
	inline vec operator-(const vec& v) const noexcept
	    {
		return vec(x - v.x, y - v.y, z - v.z, w - v.w);
	    }
	inline void operator-=(const float v) noexcept
	    {
		x -= v;
		y -= v;
		z -= v;
		w -= v;
	    }
	inline vec operator-(const T v) const noexcept
	    {
		return vec(x - v, y - v, z - v, w - v);
	    }

	// multipication
	inline void operator*=(const vec& v) noexcept
	    {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		w *= v.w;
	    }
	inline vec operator*(const vec& v) const noexcept
	    {
		return vec(x * v.x, y * v.y, z * v.z, w * v.w);
	    }
	inline void operator*=(const T v) noexcept
	    {
		x *= v;
		y *= v;
		z *= v;
		w *= v;
	    }
	inline vec operator*(const T v) const noexcept
	    {
		return vec(x * v, y * v, z * v, w * v);
	    }

	// divide
	inline void operator/=(const vec& v) noexcept
	    {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		w /= v.w;
	    }
	inline vec operator/(const vec& v) const noexcept
	    {
		return vec(x / v.x, y / v.y, z / v.z, w / v.w);
	    }
	inline void operator/=(const T v) noexcept
	    {
		x /= v;
		y /= v;
		z /= v;
		w /= v;
	    }
	inline vec operator/(const T v) const noexcept
	    {
		return vec(x / v, y / v, z / v, w / v);
	    }

	// methods
	constexpr float length() const noexcept
	    {
		return math_sqrt(x * x + y * y + z * z + w * w);
	    }
	inline void normalize() noexcept
	    {
		float m = length();
		x /= m;
		y /= m;
		z /= m;
		w /= m;
	    }
	inline vec vec_to(const vec& other)
	    {
		return other - *this;
	    }
	inline float distance_to(const vec& other)
	    {
		return vec_to(other).length();
	    }

	// setters
	inline void set(T x, T y, T z, T w) noexcept
	    {
		*this = { x, y, z, w };
	    }
	inline void set_vec2(const vec2& v) noexcept
	    {
		*this = { v.x, v.y, z, w };
	    }
	inline void set_vec3(const vec3& v) noexcept
	    {
		*this = { v.x, v.y, v.z, w };
	    }
	inline void set_dx(const XMVECTOR& v) noexcept
	    {
		*this = { XMVectorGetX(v), XMVectorGetY(v), XMVectorGetZ(v), XMVectorGetW(v) };
	    }

	// getters
	inline vec2 get_vec2() const noexcept
	    {
		return { x, y };
	    }
	inline vec3 get_vec3() const noexcept
	    {
		return { x, y, z };
	    }
	inline XMVECTOR get_dx() const noexcept
	    {
		return XMVectorSet(x, y, z, w);
	    }
    };

    typedef Vector2D<f32, f32> v2_f32;
    typedef Vector3D<f32, f32> v3_f32;
    typedef Vector4D<f32, f32> v4_f32;

    typedef Vector2D<f64, f64> v2_f64;
    typedef Vector3D<f64, f64> v3_f64;
    typedef Vector4D<f64, f64> v4_f64;

    typedef Vector2D<u32, f32> v2_u32;
    typedef Vector3D<u32, f32> v3_u32;
    typedef Vector4D<u32, f32> v4_u32;

    typedef Vector2D<i32, f32> v2_i32;
    typedef Vector3D<i32, f32> v3_i32;
    typedef Vector4D<i32, f32> v4_i32;

    typedef Vector2D<u64, f64> v2_u64;
    typedef Vector3D<u64, f64> v3_u64;
    typedef Vector4D<u64, f64> v4_u64;

    typedef Vector2D<i64, f64> v2_i64;
    typedef Vector3D<i64, f64> v3_i64;
    typedef Vector4D<i64, f64> v4_i64;

    typedef Vector2D<bool, f32> v2_bool;
    typedef Vector3D<bool, f32> v3_bool;
    typedef Vector4D<bool, f32> v4_bool;

    // Color

    struct Color {
	u8 r, g, b, a;

	constexpr static Color Red(u8 a = 255u) { return { 255u, 0u, 0u, a }; }
	constexpr static Color Green(u8 a = 255u) { return { 0u	, 128u, 0u, a }; }
	constexpr static Color Blue(u8 a = 255u) { return { 0u, 0u, 255u, a }; }
	constexpr static Color Orange(u8 a = 255u) { return { 255u, 69u, 0u, a }; }
	constexpr static Color Black(u8 a = 255u) { return { 0u, 0u, 0u, a }; }
	constexpr static Color Gray(u8 v, u8 a = 255u) { return { v, v, v, a }; }
	constexpr static Color White(u8 a = 255u) { return { 255u, 255u, 255u, a }; }
	constexpr static Color Silver(u8 a = 255u) { return { 192u, 192u, 192u, a }; }
	constexpr static Color Maroon(u8 a = 255u) { return { 128u, 0u, 0u, a }; }
	constexpr static Color Yellow(u8 a = 255u) { return { 255u, 255u, 0u, a }; }
	constexpr static Color Olive(u8 a = 255u) { return { 128u, 128u, 0u, a }; }
	constexpr static Color Lime(u8 a = 255u) { return { 0u, 255u, 0u, a }; }
	constexpr static Color Aqua(u8 a = 255u) { return { 0u, 255u, 255u, a }; }
	constexpr static Color Teal(u8 a = 255u) { return { 0u, 128u, 128u, a }; }
	constexpr static Color Navy(u8 a = 255u) { return { 0u, 0u, 128u, a }; }
	constexpr static Color Fuchsia(u8 a = 255u) { return { 255u, 0u, 255u, a }; }
	constexpr static Color Purple(u8 a = 255u) { return { 128u, 0u, 128u, a }; }
	constexpr static Color IndianRed(u8 a = 255u) { return { 205u, 92u, 92u, a }; }
	constexpr static Color LightCoral(u8 a = 255u) { return { 240u, 128u, 128u, a }; }
	constexpr static Color Salmon(u8 a = 255u) { return { 250u, 128u, 114u, a }; }
	constexpr static Color DarkSalmon(u8 a = 255u) { return { 233u, 150u, 122u, a }; }
	constexpr static Color LightSalmon(u8 a = 255u) { return { 255u, 160u, 122u, a }; }

	SV_INLINE bool operator==(const Color& o) { return o.r == r && o.g == g && o.b == b; }
	SV_INLINE bool operator!=(const Color& o) { return o.r != r || o.g != g || o.b != b; }
    };

    struct Color4f {
	float r, g, b, a;

	constexpr static Color4f Red() { return { 1.f	, 0.f	, 0.f	, 1.f }; }
	constexpr static Color4f Green() { return { 0.f	, 1.f	, 0.f	, 1.f }; }
	constexpr static Color4f Blue() { return { 0.f	, 0.f	, 1.f	, 1.f }; }
	constexpr static Color4f Black() { return { 0.f	, 0.f	, 0.f	, 1.f }; }
	constexpr static Color4f Gray(float v) { return { v	, v		, v		, 1.f }; }
	constexpr static Color4f White() { return { 1.f	, 1.f	, 1.f	, 1.f }; }

    };

    struct Color3f {
	float r, g, b;

	constexpr static Color3f Red() { return { 1.f	, 0.f	, 0.f }; }
	constexpr static Color3f Green() { return { 0.f	, 1.f	, 0.f }; }
	constexpr static Color3f Blue() { return { 0.f	, 0.f	, 1.f }; }
	constexpr static Color3f Black() { return { 0.f	, 0.f	, 0.f }; }
	constexpr static Color3f Gray(float v) { return { v	, v		, v }; }
	constexpr static Color3f White() { return { 1.f	, 1.f	, 1.f }; }

    };

    // Random

    // return value from 0u to (u32_max / 2u)
    SV_INLINE u32 math_random_u32(u32 seed)
    {
	seed = (seed << 13) ^ seed;
	return ((seed * (seed * seed * 15731u * 789221u) + 1376312589u) & 0x7fffffffu);
    }

    SV_INLINE f32 math_random_f32(u32 seed)
    {
	return (f32(math_random_u32(seed)) * (1.f / f32(u32_max / 2u)));
    }
    SV_INLINE f32 math_random_f32(u32 seed, f32 max)
    {
	return math_random_f32(seed) * max;
    }
    SV_INLINE f32 math_random_f32(u32 seed, f32 min, f32 max)
    {
	SV_ASSERT(min <= max);
	return min + math_random_f32(seed) * (max - min);
    }

    SV_INLINE u32 math_random_u32(u32 seed, u32 max)
    {
	if (max > 8u)
	    return math_random_u32(seed) % max;
	else
	    return u32(math_random_f32(++seed, f32(max)));
    }
    SV_INLINE u32 math_random_u32(u32 seed, u32 min, u32 max)
    {
	if (max - min > 8u)
	    return min + (math_random_u32(seed) % (max - min));
	else
	    return u32(math_random_f32(++seed, f32(min), f32(max)));
    }

    struct Random {

	// return value from 0u to (u32_max / 2u)
	SV_INLINE u32 gen_u32() { return math_random_u32(++seed); }
	SV_INLINE u32 gen_u32(u32 max) { return math_random_u32(++seed, max); }
	SV_INLINE u32 gen_u32(u32 min, u32 max) { return math_random_u32(++seed, min, max); }

	SV_INLINE f32 gen_f32() { return math_random_f32(++seed); }
	SV_INLINE f32 gen_f32(f32 max) { return math_random_f32(++seed, max); }
	SV_INLINE f32 gen_f32(f32 min, f32 max) { return math_random_f32(++seed, min, max); }

	u32 seed = 0u;
    };

    SV_INLINE f32 math_perlin_noise(u32 seed, f32 n)
    {
	i32 offset = (n < 0.f) ? (i32(n) - 1) : i32(n);

	f32 p0 = f32(offset) + math_random_f32(seed * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));
	f32 h0 = math_random_f32(seed * 24332 * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

	f32 p1;
	if (n < p0) {

	    --offset;
	    p1 = f32(offset) + math_random_f32(seed * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

	    f32 pos = (n - p1) / (p0 - p1);
	    pos = cos(PI - pos * PI) * 0.5f + 0.5f;

	    f32 h1 = math_random_f32(seed * 24332 * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

	    return h1 + pos * (h0 - h1);
	}
	else {

	    ++offset;
	    p1 = f32(offset) + math_random_f32(seed * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

	    f32 pos = (n - p0) / (p1 - p0);
	    pos = cos(PI - pos * -PI) * 0.5f + 0.5f;

	    f32 h1 = math_random_f32(seed * 24332 * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

	    return h0 + pos * (h1 - h0);
	}
    }

    // Matrix

    XMMATRIX math_matrix_view(const v3_f32& position, const v4_f32& directionQuat);

    // Intersection

    struct Ray {
	v3_f32 origin;
	v3_f32 direction;
    };
	
    SV_INLINE bool intersect_ray_vs_traingle(const Ray& ray,
					     const v3_f32& v0,
					     const v3_f32& v1,
					     const v3_f32& v2,
					     v3_f32& outIntersectionPoint)
    {
	const f32 EPSILON = 0.0000001f;
	v3_f32 edge1, edge2, h, s, q;
	f32 a, f, u, v;
	edge1 = v1 - v0;
	edge2 = v2 - v0;
	h = ray.direction.cross(edge2);
	a = edge1.dot(h);
	if (a > -EPSILON && a < EPSILON)
	    return false;    // This ray is parallel to this triangle.
	f = 1.f / a;
	s = ray.origin - v0;
	u = f * s.dot(h);
	if (u < 0.0 || u > 1.0)
	    return false;
	q = s.cross(edge1);
	v = f * ray.direction.dot(q);
	if (v < 0.0 || u + v > 1.0)
	    return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * edge2.dot(q);
	if (t > EPSILON) // ray intersection
	{
	    outIntersectionPoint = ray.origin + ray.direction * t;
	    return true;
	}
	else // This means that there is a line intersection but not a ray intersection.
	    return false;
    }


    // Character utils
    
    std::wstring parse_wstring(const char* c);
    std::string  parse_string(const wchar* c);
    
    constexpr bool char_is_letter(char c) 
    {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    
    constexpr bool char_is_number(char c)
    {
	return c >= '0' && c <= '9';
    }

    struct Archive {

	Archive();
	~Archive();

	void reserve(size_t size);
	void write(const void* data, size_t size);
	void read(void* data, size_t size);
	void erase(size_t size);
		
	SV_INLINE size_t size() const noexcept { return _size; }
	SV_INLINE void clear() noexcept { if (_size) _size = sizeof(Version); }
	SV_INLINE void startRead() noexcept { _pos = sizeof(Version); }

	Result openFile(const char* filePath);
	Result saveFile(const char* filePath, bool append = false);

	SV_INLINE Archive& operator<<(char n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(bool n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u8 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u16 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u32 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(u64 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i8 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i16 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i32 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(i64 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(f32 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(f64 n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const Version& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMFLOAT2& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMFLOAT3& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMFLOAT4& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const XMMATRIX& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const Color& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const Color3f& n) { write(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator<<(const Color4f& n) { write(&n, sizeof(n)); return *this; }

	template<typename T, typename F>
	SV_INLINE Archive& operator<<(const Vector2D<T, F>& n) { write(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	SV_INLINE Archive& operator<<(const Vector3D<T, F>& n) { write(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	SV_INLINE Archive& operator<<(const Vector4D<T, F>& n) { write(&n, sizeof(n)); return *this; }

	template<typename T>
	inline Archive& operator<<(const std::vector<T>& vec)
	    {
		size_t size = vec.size();
		write(&size, sizeof(size_t));

		for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
		    this->operator<<(*it);
		}
		return *this;
	    }

	inline Archive& operator<<(const char* txt)
	    {
		size_t txtLength = strlen(txt);
		write(&txtLength, sizeof(size_t));
		write(txt, txtLength);
		return *this;
	    }

	inline Archive& operator<<(const wchar* txt)
	    {
		size_t txtLength = wcslen(txt);
		write(&txtLength, sizeof(size_t));
		write(txt, txtLength * sizeof(wchar));
		return *this;
	    }

	inline Archive& operator<<(const std::string& str)
	    {
		size_t txtLength = str.size();
		write(&txtLength, sizeof(size_t));
		write(str.data(), txtLength);
		return *this;
	    }

	inline Archive& operator<<(const std::wstring& str)
	    {
		size_t txtLength = str.size();
		write(&txtLength, sizeof(size_t));
		write(str.data(), txtLength * sizeof(wchar));
		return *this;
	    }

	// READ

	SV_INLINE Archive& operator>>(bool& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(char& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u8& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u16& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u32& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(u64& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i8& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i16& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i32& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(i64& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(f32& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(f64& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(Version& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMFLOAT2& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMFLOAT3& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMFLOAT4& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(XMMATRIX& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(Color& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(Color3f& n) { read(&n, sizeof(n)); return *this; }
	SV_INLINE Archive& operator>>(Color4f& n) { read(&n, sizeof(n)); return *this; }

	template<typename T, typename F>
	SV_INLINE Archive& operator>>(Vector2D<T, F>& n) { read(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	SV_INLINE Archive& operator>>(Vector3D<T, F>& n) { read(&n, sizeof(n)); return *this; }
	template<typename T, typename F>
	SV_INLINE Archive& operator>>(Vector4D<T, F>& n) { read(&n, sizeof(n)); return *this; }

	template<typename T>
	Archive& operator>>(std::vector<T>& vec)
	    {
		size_t size;
		read(&size, sizeof(size_t));
		size_t lastSize = vec.size();
		vec.resize(lastSize + size);
		read(vec.data() + lastSize, sizeof(T) * size);
		return *this;
	    }

	Archive& operator>>(std::string& str)
	    {
		size_t txtLength;
		read(&txtLength, sizeof(size_t));
		str.resize(txtLength);
		read((void*)str.data(), txtLength);
		return *this;
	    }

	Archive& operator>>(std::wstring& str)
	    {
		size_t txtLength;
		read(&txtLength, sizeof(size_t));
		str.resize(txtLength);
		read((void*)str.data(), txtLength * sizeof(wchar));
		return *this;
	    }

	size_t _pos;
	size_t _size;
	size_t _capacity;
	u8* _data;
	Version version;

    private:
	void allocate(size_t size);
	void free();

    };
    
}

