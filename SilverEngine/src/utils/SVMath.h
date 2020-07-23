#pragma once

#include "core.h"

#define ToRadians(x) x * 0.0174533f
#define ToDegrees(x) x / 0.0174533f
#define PI 3.14159f

namespace SV {

#pragma region Vectors
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

#pragma endregion

	namespace Math {
		inline void QuaternionToEuler(SV::vec4* q, SV::vec3* e)
		{
			// roll (x-axis rotation)
			float sinr_cosp = 2.f * (q->w * q->x + q->y * q->z);
			float cosr_cosp = 1.f - 2.f * (q->x * q->x + q->y * q->y);
			e->x = std::atan2(sinr_cosp, cosr_cosp);

			// pitch (y-axis rotation)
			float sinp = 2.f * (q->w * q->y - q->z * q->x);
			if (std::abs(sinp) >= 1.f)
				e->y = std::copysign(PI / 2.f, sinp); // use 90 degrees if out of range
			else
				e->y = std::asin(sinp);

			// yaw (z-axis rotation)
			float siny_cosp = 2.f * (q->w * q->z + q->x * q->y);
			float cosy_cosp = 1.f - 2.f * (q->y * q->y + q->z * q->z);
			e->z = std::atan2(siny_cosp, cosy_cosp);
		}

		inline XMMATRIX CalculateViewMatrix(SV::vec3 position, SV::vec3 rotation)
		{
			XMVECTOR direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);

			direction = XMVector3Transform(direction, XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, 0.f));

			const auto target = XMVECTOR(position) + direction;
			return XMMatrixLookAtLH(position, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
		}

		template<typename T>
		constexpr T Gauss(T x, T sigma) noexcept
		{
			return ((T)1.f / sqrt((T)2.f * (T)PI * sigma * sigma)) * exp(-(x * x) / ((T)2.f * sigma * sigma));
		}

		constexpr float ByteColorToFloat(ui8 c) { return float(c) / 255.f; }
		constexpr ui8 FloatColorToByte(float c) { return ui8(c * 255.f); }
	}
}

/////////////////////////////////////////////////////COLORS/////////////////////////////////////////////////////////////////
namespace SV {
	typedef Vector4D<ui8>	Color4b;
	typedef Vector3D<ui8>	Color3b;
	typedef Vector4D<float> Color4f;
	typedef Vector3D<float> Color3f;
	typedef ui32			Color1u;
}

namespace SVColor4b {

	constexpr SV::Color4b BLACK(0u, 0u, 0u, 255u);
	constexpr SV::Color4b WHITE(255u);
	constexpr SV::Color4b RED(255u, 0u, 0u, 255u);
	constexpr SV::Color4b GREEN(0u, 255u, 0u, 255u);
	constexpr SV::Color4b BLUE(0u, 0u, 255u, 255u);
	constexpr SV::Color4b YELLOW(255u, 255u, 0u, 255u);
	constexpr SV::Color4b ORANGE(255u, 100u, 0u, 255u);
	constexpr SV::Color4b GREY(ui8 n) { return SV::Color4b(n, n, n, 255u); }

	constexpr SV::Color3b ToColor3b(SV::Color4b c) { return SV::Color3b(c.x, c.y, c.z); }
	constexpr SV::Color4f ToColor4f(SV::Color4b c) { return SV::Color4f(SV::Math::ByteColorToFloat(c.x), SV::Math::ByteColorToFloat(c.y), SV::Math::ByteColorToFloat(c.z), SV::Math::ByteColorToFloat(c.w)); }
	constexpr SV::Color3f ToColor3f(SV::Color4b c) { return SV::Color3f(SV::Math::ByteColorToFloat(c.x), SV::Math::ByteColorToFloat(c.y), SV::Math::ByteColorToFloat(c.z)); }

}

namespace SVColor3b {

	constexpr SV::Color3b BLACK(0u, 0u, 0u);
	constexpr SV::Color3b WHITE(255u);
	constexpr SV::Color3b RED(255u, 0u, 0u);
	constexpr SV::Color3b GREEN(0u, 255u, 0u);
	constexpr SV::Color3b BLUE(0u, 0u, 255u);
	constexpr SV::Color3b GREY(ui8 n) { return SV::Color3b(n, n, n); }

	constexpr SV::Color4b ToColor4b(SV::Color3b c) { return SV::Color4b(c.x, c.y, c.z, 255u); }
	constexpr SV::Color4f ToColor4f(SV::Color3b c) { return SV::Color4f(SV::Math::ByteColorToFloat(c.x), SV::Math::ByteColorToFloat(c.y), SV::Math::ByteColorToFloat(c.z), 1.f); }
	constexpr SV::Color3f ToColor3f(SV::Color3b c) { return SV::Color3f(SV::Math::ByteColorToFloat(c.x), SV::Math::ByteColorToFloat(c.y), SV::Math::ByteColorToFloat(c.z)); }

}

namespace SVColor4f {

	constexpr SV::Color4f BLACK(0.f, 0.f, 0.f, 1.f);
	constexpr SV::Color4f WHITE(1.f);
	constexpr SV::Color4f RED(1.f, 0.f, 0.f, 1.f);
	constexpr SV::Color4f GREEN(0.f, 1.f, 0.f, 1.f);
	constexpr SV::Color4f BLUE(0.f, 0.f, 1.f, 1.f);
	constexpr SV::Color4f GREY(float n) { return SV::Color4f(n, n, n, 1.f); }
			  
	constexpr SV::Color4b ToColor4b(SV::Color4f c) { return SV::Color4b(SV::Math::FloatColorToByte(c.x), SV::Math::FloatColorToByte(c.y), SV::Math::FloatColorToByte(c.z), SV::Math::FloatColorToByte(c.w)); }
	constexpr SV::Color3b ToColor3b(SV::Color4f c) { return SV::Color3b(SV::Math::FloatColorToByte(c.x), SV::Math::FloatColorToByte(c.y), SV::Math::FloatColorToByte(c.z)); }
	constexpr SV::Color3f ToColor3f(SV::Color4f c) { return SV::Color3f(c.x, c.y, c.z); }

}

namespace SVColor3f {

	constexpr SV::Color3f BLACK(0.f, 0.f, 0.f);
	constexpr SV::Color3f WHITE(1.f);
	constexpr SV::Color3f RED(1.f, 0.f, 0.f);
	constexpr SV::Color3f GREEN(0.f, 1.f, 0.f);
	constexpr SV::Color3f BLUE(0.f, 0.f, 1.f);
	constexpr SV::Color3f GREY(float n) { return SV::Color3f(n, n, n); }

	constexpr SV::Color4b ToColor4b(SV::Color3f c) { return SV::Color4b(SV::Math::FloatColorToByte(c.x), SV::Math::FloatColorToByte(c.y), SV::Math::FloatColorToByte(c.z), 255u); }
	constexpr SV::Color3b ToColor3b(SV::Color3f c) { return SV::Color3b(SV::Math::FloatColorToByte(c.x), SV::Math::FloatColorToByte(c.y), SV::Math::FloatColorToByte(c.z)); }
	constexpr SV::Color4f ToColor4f(SV::Color3f c) { return SV::Color4f(c.x, c.y, c.z, 1.f); }

}
