#pragma once

#include "core.h"

#define ToRadians(x) x * 0.0174533f
#define ToDegrees(x) x / 0.0174533f
#define PI 3.14159f

namespace sv {

	template<typename T>
	struct Vector2D {

		T x;
		T y;

		using vec = Vector2D<T>;

		constexpr Vector2D() : x(), y() {}
		constexpr Vector2D(T n) : x(n), y(n) {}
		constexpr Vector2D(T x, T y) : x(x), y(y) {}

		// sum
		inline void operator+=(const vec& v) noexcept
		{
			x += v.x;
			y += v.y;
		}
		inline vec operator+(const vec& v) const noexcept
		{
			return vec(x + v.x, y + v.y);
		}
		inline void operator+=(const T v) noexcept
		{
			x += v;
			y += v;
		}
		inline vec operator+(const T v) const noexcept
		{
			return vec(x + v, y + v);
		}

		// substract
		inline void operator-=(const vec& v) noexcept
		{
			x -= v.x;
			y -= v.y;
		}
		inline vec operator-(const vec& v) const noexcept
		{
			return vec(x - v.x, y - v.y);
		}
		inline void operator-=(const T v) noexcept
		{
			x -= v;
			y -= v;
		}
		inline vec operator-(const T v) const noexcept
		{
			return vec(x - v, y - v);
		}

		// multipication
		inline void operator*=(const vec& v) noexcept
		{
			x *= v.x;
			y *= v.y;
		}
		inline vec operator*(const vec& v) const noexcept
		{
			return vec(x * v.x, y * v.y);
		}
		inline void operator*=(const T v) noexcept
		{
			x *= v;
			y *= v;
		}
		inline vec operator*(const T v) const noexcept
		{
			return vec(x * v, y * v);
		}

		// divide
		inline void operator/=(const vec& v) noexcept
		{
			x /= v.x;
			y /= v.y;
		}
		inline vec operator/(const vec& v) const noexcept
		{
			return vec(x / v.x, y / v.y);
		}
		inline void operator/=(const T v) noexcept
		{
			x /= v;
			y /= v;
		}
		inline vec operator/(const T v) const noexcept
		{
			return vec(x / v, y / v);
		}

		// methods
		constexpr float Mag() const noexcept
		{
			return sqrt(x * x + y * y);
		}
		inline void Normalize() noexcept
		{
			float m = Mag();
			x /= m;
			y /= m;
		}
		inline vec VecTo(const vec& other)
		{
			return other - *this;
		}
		inline float DistanceTo(const vec& other)
		{
			return VecTo(other).Mag();
		}

		// directx methods
		Vector2D(const XMVECTOR& dxVec) : x(XMVectorGetX(dxVec)), y(XMVectorGetY(dxVec)) {}

		inline operator XMVECTOR() const noexcept
		{
			return XMVectorSet(x, y, 0.f, 0.f);
		}

	};

	template<typename T>
	struct Vector3D {

		T x;
		T y;
		T z;

		using vec = Vector3D<T>;

		constexpr Vector3D() : x(), y(), z() {}
		constexpr Vector3D(T n) : x(n), y(n), z(n) {}
		constexpr Vector3D(T x, T y, T z) : x(x), y(y), z(z) {}

		// sum
		inline void operator+=(const vec& v) noexcept
		{
			x += v.x;
			y += v.y;
			z += v.z;
		}
		inline vec operator+(const vec& v) const noexcept
		{
			return vec(x + v.x, y + v.y, z + v.z);
		}
		inline void operator+=(const T v) noexcept
		{
			x += v;
			y += v;
			z += v;
		}
		inline vec operator+(const T v) const noexcept
		{
			return vec(x + v, y + v, z + v);
		}

		// substract
		inline void operator-=(const vec& v) noexcept
		{
			x -= v.x;
			y -= v.y;
			z -= v.z;
		}
		inline vec operator-(const vec& v) const noexcept
		{
			return vec(x - v.x, y - v.y, z - v.z);
		}
		inline void operator-=(const T v) noexcept
		{
			x -= v;
			y -= v;
			z -= v;
		}
		inline vec operator-(const T v) const noexcept
		{
			return vec(x - v, y - v, z - v);
		}

		// multipication
		inline void operator*=(const vec& v) noexcept
		{
			x *= v.x;
			y *= v.y;
			z *= v.z;
		}
		inline vec operator*(const vec& v) const noexcept
		{
			return vec(x * v.x, y * v.y, z * v.z);
		}
		inline void operator*=(const T v) noexcept
		{
			x *= v;
			y *= v;
			z *= v;
		}
		inline vec operator*(const T v) const noexcept
		{
			return vec(x * v, y * v, z * v);
		}

		// divide
		inline void operator/=(const vec& v) noexcept
		{
			x /= v.x;
			y /= v.y;
			z /= v.z;
		}
		inline vec operator/(const vec& v) const noexcept
		{
			return vec(x / v.x, y / v.y, z / v.z);
		}
		inline void operator/=(const T v) noexcept
		{
			x /= v;
			y /= v;
			z /= v;
		}
		inline vec operator/(const T v) const noexcept
		{
			return vec(x / v, y / v, z / v);
		}

		// methods
		constexpr float Mag() const noexcept
		{
			return sqrt(x * x + y * y + z * z);
		}
		inline void Normalize() noexcept
		{
			float m = Mag();
			x /= m;
			y /= m;
			z /= m;
		}
		inline vec VecTo(const vec& other)
		{
			return other - *this;
		}
		inline float DistanceTo(const vec& other)
		{
			return VecTo(other).Mag();
		}

		// directx methods
		Vector3D(const XMVECTOR& dxVec) : x(XMVectorGetX(dxVec)), y(XMVectorGetY(dxVec)), z(XMVectorGetZ(dxVec)) {}

		inline operator XMVECTOR() const noexcept
		{
			return XMVectorSet(x, y, z, 0.f);
		}
	};

	template<typename T>
	struct Vector4D {

		T x;
		T y;
		T z;
		T w;

		using vec = Vector4D<T>;

		constexpr Vector4D() : x(), y(), z(), w() {}
		constexpr Vector4D(T n) : x(n), y(n), z(n), w(n) {}
		constexpr Vector4D(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

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
		constexpr float Mag() const noexcept
		{
			return sqrt(x * x + y * y + z * z + w * w);
		}
		inline void Normalize() noexcept
		{
			float m = Mag();
			x /= m;
			y /= m;
			z /= m;
			w /= m;
		}
		inline vec VecTo(const vec& other)
		{
			return other - *this;
		}
		inline float DistanceTo(const vec& other)
		{
			return VecTo(other).Mag();
		}

		// directx methods
		Vector4D(const XMVECTOR& dxVec) : x(XMVectorGetX(dxVec)), y(XMVectorGetY(dxVec)), z(XMVectorGetZ(dxVec)), w(XMVectorGetW(dxVec)) {}

		inline operator XMVECTOR() const noexcept
		{
			return XMVectorSet(x, y, z, w);
		}
	};

	typedef Vector2D<float>  vec2;
	typedef Vector2D<i32>	 ivec2;
	typedef Vector2D<ui32>	 uvec2;

	typedef Vector3D<float>  vec3;
	typedef Vector3D<i32>	 ivec3;
	typedef Vector3D<ui32>	 uvec3;

	typedef Vector4D<float>  vec4;
	typedef Vector4D<i32>	 ivec4;
	typedef Vector4D<ui32>	 uvec4;

	typedef Vector4D<ui8>	Color;
	typedef Vector3D<float>	Color3f;
	typedef Vector4D<float>	Color4f;

}

namespace sv {

	void math_quaternion_to_euler(sv::vec4* q, sv::vec3* e);
	XMMATRIX math_matrix_view(vec3 position, vec3 rotation);

	template<typename T>
	constexpr T math_gauss(T x, T sigma) noexcept
	{
		return ((T)1.f / sqrt((T)2.f * (T)PI * sigma * sigma)) * exp(-(x * x) / ((T)2.f * sigma * sigma));
	}

	constexpr float math_color_byte_to_float(ui8 c) { return float(c) / 255.f; }
	constexpr ui8 math_color_float_to_byte(float c) { return ui8(c * 255.f); }

}

#define SV_COLOR_GRAY(x)	sv::Color(x   , x   , x   , 255u)
#define SV_COLOR_RED		sv::Color(255u, 0u  , 0u  , 255u)
#define SV_COLOR_GREEN		sv::Color(0u  , 255u, 0u  , 255u)
#define SV_COLOR_BLUE		sv::Color(0u  , 0u  , 255u, 255u)
#define SV_COLOR_BLACK		sv::Color(0u  , 0u  , 0u  , 255u)
#define SV_COLOR_WHITE		sv::Color(255u, 255u, 255u, 255u)