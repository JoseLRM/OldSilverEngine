#pragma once

#include "core.h"

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

		constexpr static Color Red(u8 a = 255u)			{ return { 255u	, 0u	, 0u	, a }; }
		constexpr static Color Green(u8 a = 255u)			{ return { 0u	, 255u	, 0u	, a }; }
		constexpr static Color Blue(u8 a = 255u)			{ return { 0u	, 0u	, 255u	, a }; }
		constexpr static Color Orange(u8 a = 255u)			{ return { 255u	, 69u	, 0u	, a }; }
		constexpr static Color Black(u8 a = 255u)			{ return { 0u	, 0u	, 0u	, a }; }
		constexpr static Color Gray(u8 v, u8 a = 255u)	{ return { v	, v		, v		, a }; }
		constexpr static Color White(u8 a = 255u)			{ return { 255u	, 255u	, 255u	, a }; }

	};

	struct Color4f {
		float r, g, b, a;

		constexpr static Color4f Red()			{ return { 1.f	, 0.f	, 0.f	, 1.f }; }
		constexpr static Color4f Green()		{ return { 0.f	, 1.f	, 0.f	, 1.f }; }
		constexpr static Color4f Blue()			{ return { 0.f	, 0.f	, 1.f	, 1.f }; }
		constexpr static Color4f Black()		{ return { 0.f	, 0.f	, 0.f	, 1.f }; }
		constexpr static Color4f Gray(float v)	{ return { v	, v		, v		, 1.f }; }
		constexpr static Color4f White()		{ return { 1.f	, 1.f	, 1.f	, 1.f }; }

	};

	struct Color3f {
		float r, g, b;

		constexpr static Color3f Red()			{ return { 1.f	, 0.f	, 0.f }; }
		constexpr static Color3f Green()		{ return { 0.f	, 1.f	, 0.f }; }
		constexpr static Color3f Blue()			{ return { 0.f	, 0.f	, 1.f }; }
		constexpr static Color3f Black()		{ return { 0.f	, 0.f	, 0.f }; }
		constexpr static Color3f Gray(float v)	{ return { v	, v		, v	 }; }
		constexpr static Color3f White()		{ return { 1.f	, 1.f	, 1.f }; }

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

		SV_INLINE void	setSeed(u32 seed) noexcept { this->seed = seed; }
		SV_INLINE u32	getSeed() const noexcept { return seed; }

		// return value from 0u to (u32_max / 2u)
		SV_INLINE u32 gen_u32() { return math_random_u32(++seed); }
		SV_INLINE u32 gen_u32(u32 max) { return math_random_u32(++seed, max); }
		SV_INLINE u32 gen_u32(u32 min, u32 max) { return math_random_u32(++seed, min, max); }

		SV_INLINE f32 gen_f32() { return math_random_f32(++seed); }
		SV_INLINE f32 gen_f32(f32 max) { return math_random_f32(++seed, max); }
		SV_INLINE f32 gen_f32(f32 min, f32 max) { return math_random_f32(++seed, min, max); }

	private:
		u32 seed = 0u;
	};

	// Matrix

	XMMATRIX math_matrix_view(const v3_f32& position, const v4_f32& directionQuat);

	// MOSTLY INSPIRED IN WICKED ENGINE !!!!!!!!!!
	// Intersection

	struct Ray3D;

	struct BoundingBox3D {

		v3_f32 min, max;

		SV_INLINE void init_center(const v3_f32& center, const v3_f32& dimensions)
		{
			v3_f32 half = dimensions / 2.f;
			min = center - half;
			max = center + half;
		}

		SV_INLINE void init_minmax(const v3_f32& min, const v3_f32& max)
		{
			this->min = min;
			this->max = max;
		}

		SV_INLINE v3_f32 getCenter() const noexcept { return min + (getDimensions() / 2.f); }
		SV_INLINE v3_f32 getDimensions() const noexcept { return max - min; }

		bool intersects_point(const v3_f32& point) const noexcept;
		bool intersects_ray3D(const Ray3D& ray) const noexcept;

	};

	struct Ray3D {

		v3_f32 origin, direction, directionInverse;

		inline void init_line(const v3_f32& point0, const v3_f32& point1)
		{
			origin = point0;
			direction = point1 - origin;
			direction.normalize();
			directionInverse = { 1.f / direction.x, 1.f / direction.y, 1.f / direction.z };
		}

		inline void init_direction(const v3_f32& origin, const v3_f32& direction)
		{
			this->origin = origin;
			this->direction = direction;
		}

		inline bool intersects_boundingBox3D(const BoundingBox3D& aabb) const noexcept { return aabb.intersects_ray3D(*this); }

	};

	// 2D INTERSECTIONS

	struct Circle2D;

	struct BoundingBox2D {

		v2_f32 min, max;

		inline void init_center(const v2_f32& center, const v2_f32& dimensions)
		{
			v2_f32 half = dimensions / 2.f;
			min = center - half;
			max = center + half;
		}

		inline void init_minmax(const v2_f32& min, const v2_f32& max)
		{
			this->min = min;
			this->max = max;
		}

		bool intersects_point(const v2_f32& point) const noexcept;
		bool intersects_circle(const Circle2D& circle) const noexcept;

	};

	struct FrustumOthographic {

		v2_f32 position;
		v2_f32 halfSize;

		inline void init_center(const v2_f32& center, const v2_f32& dimensions)
		{
			halfSize = dimensions / 2.f;
			position = center;
		}

		bool intersects_circle(const Circle2D& circle) const noexcept;

	};

	struct Circle2D {

		v2_f32 position;
		float radius;

	};

}