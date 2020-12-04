#pragma once

#include "core.h"

#define ToRadians(x) x * 0.0174533f
#define ToDegrees(x) x / 0.0174533f
#define PI 3.14159f

namespace sv {

	inline float math_sqrt(float n)
	{
		int i;
		float x, y;
		x = n * 0.5f;
		y = n;
		i = *(int*)& y;
		i = 0x5f3759df - (i >> 1);
		y = *(float*)& i;
		y = y * (1.5f - (x * y * y));
		y = y * (1.5f - (x * y * y));
		return n * y;
	}

	template<typename T>
	constexpr T math_gauss(T x, T sigma) noexcept
	{
		return ((T)1.f / math_sqrt((T)2.f * (T)PI * sigma * sigma)) * exp(-(x * x) / ((T)2.f * sigma * sigma));
	}

	// Vector Structs

	template<typename T>
	struct Vector2D;
	template<typename T>
	struct Vector3D;
	template<typename T>
	struct Vector4D;

	template<typename T>
	struct Vector2D {

		T x;
		T y;

		using vec = Vector2D<T>;
		using vec3 = Vector3D<T>;
		using vec4 = Vector4D<T>;

		constexpr Vector2D() : x(), y() {}
		constexpr Vector2D(T n) : x(n), y(n) {}
		constexpr Vector2D(T x, T y) : x(x), y(y) {}
		explicit constexpr Vector2D(const XMVECTOR& v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)) {}

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
		constexpr float length() const noexcept
		{
			return math_sqrt(x * x + y * y);
		}
		inline void normalize() noexcept
		{
			float m = length();
			x /= m;
			y /= m;
		}
		inline vec vec_to(const vec& other) const noexcept
		{
			return other - *this;
		}
		inline float distance_to(const vec& other) const noexcept
		{
			return vec_to(other).length();
		}
		inline float angle() const noexcept
		{
			float res = atan2(y, x);
			if (res < 0) {
				res = (2.f * PI) + res;
			}
			return res;
		}
		inline void setAngle(float angle) noexcept
		{
			float mag = length();
			x = cos(angle);
			x *= mag;
			y = sin(angle);
			y *= mag;
		}

		// setters
		inline void set(T x, T y) noexcept
		{
			*this = { x, y };
		}
		inline void set_vec3(const vec3& v) noexcept
		{
			*this = { v.x, v.y };
		}
		inline void set_vec4(const vec4& v) noexcept
		{
			*this = { v.x, v.y };
		}
		inline void set_dx(const XMVECTOR& v) noexcept
		{
			*this = { XMVectorGetX(v), XMVectorGetY(v) };
		}

		// getters
		inline vec3 getVec3() const noexcept
		{
			return { x, y, 0.f };
		}
		inline vec4 get_vec4() const noexcept
		{
			return { x, y, 0.f, 0.f };
		}
		inline XMVECTOR get_dx() const noexcept
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
		using vec2 = Vector2D<T>;
		using vec4 = Vector4D<T>;

		constexpr Vector3D() : x(), y(), z() {}
		constexpr Vector3D(T n) : x(n), y(n), z(n) {}
		constexpr Vector3D(T x, T y, T z) : x(x), y(y), z(z) {}
		explicit constexpr Vector3D(const XMVECTOR& v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)), z(XMVectorGetZ(v)) {}

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
		constexpr float length() const noexcept
		{
			return math_sqrt(x * x + y * y + z * z);
		}
		inline void normalize() noexcept
		{
			float m = length();
			x /= m;
			y /= m;
			z /= m;
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
		inline void set(T x, T y, T z) noexcept
		{
			*this = { x, y, z };
		}
		inline void set_vec2(const vec2& v) noexcept
		{
			*this = { v.x, v.y, z };
		}
		inline void set_vec4(const vec4& v) noexcept
		{
			*this = { v.x, v.y, v.z };
		}
		inline void setDX(const XMVECTOR& v) noexcept
		{
			*this = { XMVectorGetX(v), XMVectorGetY(v), XMVectorGetZ(v) };
		}

		// getters
		inline vec2 getVec2() const noexcept
		{
			return { x, y };
		}
		inline vec4 get_vec4() const noexcept
		{
			return { x, y, z, 0.f };
		}
		inline XMVECTOR getDX() const noexcept
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
		using vec2 = Vector2D<T>;
		using vec3 = Vector3D<T>;

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

	typedef Vector2D<float> vec2f;
	typedef Vector3D<float> vec3f;
	typedef Vector4D<float> vec4f;

	typedef Vector2D<ui32> vec2u;
	typedef Vector3D<ui32> vec3u;
	typedef Vector4D<ui32> vec4u;

	typedef Vector2D<i32> vec2i;
	typedef Vector3D<i32> vec3i;
	typedef Vector4D<i32> vec4i;

	// Color

	struct Color {
		ui8 r, g, b, a;

		constexpr static Color Red(ui8 a = 255u)			{ return { 255u	, 0u	, 0u	, a }; }
		constexpr static Color Green(ui8 a = 255u)			{ return { 0u	, 255u	, 0u	, a }; }
		constexpr static Color Blue(ui8 a = 255u)			{ return { 0u	, 0u	, 255u	, a }; }
		constexpr static Color Orange(ui8 a = 255u)			{ return { 255u	, 153u	, 51u	, a }; }
		constexpr static Color Black(ui8 a = 255u)			{ return { 0u	, 0u	, 0u	, a }; }
		constexpr static Color Gray(ui8 v, ui8 a = 255u)	{ return { v	, v		, v		, a }; }
		constexpr static Color White(ui8 a = 255u)			{ return { 255u	, 255u	, 255u	, a }; }

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

	ui32 math_random(ui32 seed);
	ui32 math_random(ui32 seed, ui32 max);
	ui32 math_random(ui32 seed, ui32 min, ui32 max);

	float math_randomf(ui32 seed);
	float math_randomf(ui32 seed, float max);
	float math_randomf(ui32 seed, float min, float max);

	// Matrix

	XMMATRIX math_matrix_view(const vec3f& position, const vec4f& directionQuat);

	// MOSTLY INSPIRED IN WICKED ENGINE !!!!!!!!!!
	// Intersection

	struct Ray3D;

	struct BoundingBox3D {

		vec3f min, max;

		inline void init_center(const vec3f& center, const vec3f& dimensions)
		{
			vec3f half = dimensions / 2.f;
			min = center - half;
			max = center + half;
		}

		inline void init_minmax(const vec3f& min, const vec3f& max)
		{
			this->min = min;
			this->max = max;
		}

		bool intersects_point(const vec3f& point) const noexcept;
		bool intersects_ray3D(const Ray3D& ray) const noexcept;

	};

	struct Ray3D {

		vec3f origin, direction, directionInverse;

		inline void init_line(const vec3f& point0, const vec3f& point1)
		{
			origin = point0;
			direction = point1 - origin;
			direction.normalize();
			directionInverse = { 1.f / direction.x, 1.f / direction.y, 1.f / direction.z };
		}

		inline void init_direction(const vec3f& origin, const vec3f& direction)
		{
			this->origin = origin;
			this->direction = direction;
		}

		inline bool intersects_boundingBox3D(const BoundingBox3D& aabb) const noexcept { return aabb.intersects_ray3D(*this); }

	};

	// 2D INTERSECTIONS

	struct Circle2D;

	struct BoundingBox2D {

		vec2f min, max;

		inline void init_center(const vec2f& center, const vec2f& dimensions)
		{
			vec2f half = dimensions / 2.f;
			min = center - half;
			max = center + half;
		}

		inline void init_minmax(const vec2f& min, const vec2f& max)
		{
			this->min = min;
			this->max = max;
		}

		bool intersects_point(const vec2f& point) const noexcept;
		bool intersects_circle(const Circle2D& circle) const noexcept;

	};

	struct FrustumOthographic {

		vec2f position;
		vec2f halfSize;

		inline void init_center(const vec2f& center, const vec2f& dimensions)
		{
			halfSize = dimensions / 2.f;
			position = center;
		}

		bool intersects_circle(const Circle2D& circle) const noexcept;

	};

	struct Circle2D {

		vec2f position;
		float radius;

	};

}