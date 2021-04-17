#pragma once

#include "debug/console.h"

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

	constexpr static Color Transparent() { return { 0u, 0u, 0u, 0u }; }
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

	SV_INLINE v4_f32 toVec4() const { return { f32(r) * (1.f / 255.f), f32(g) * (1.f / 255.f), f32(b) * (1.f / 255.f), f32(a) * (1.f / 255.f) }; }
	SV_INLINE v3_f32 toVec3() const { return { f32(r) * (1.f / 255.f), f32(g) * (1.f / 255.f), f32(b) * (1.f / 255.f) }; }

	SV_INLINE void setFloat(f32 _r, f32 _g, f32 _b, f32 _a = 1.f)
	    {
		r = u8(_r * 255.f);
		g = u8(_g * 255.f);
		b = u8(_b * 255.f);
		a = u8(_a * 255.f);
	    }
	
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

    // Hash functions

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
}
