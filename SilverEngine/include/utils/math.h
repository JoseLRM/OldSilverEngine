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

    SV_INLINE f32 math_clamp(f32 min, f32 n, f32 max)
    {
		if (n < min) n = min;
		else if (n > max) n = max;
		return n;
    }

    SV_INLINE u32 math_clamp(u32 min, u32 n, u32 max)
    {
		if (n < min) n = min;
		else if (n > max) n = max;
		return n;
    }

    SV_INLINE f32 math_clamp01(f32 n)
    {
		return math_clamp(0.f, n, 1.f);
    }

    template<typename T>
	constexpr T math_gauss(T x, T sigma) noexcept
    {
		return ((T)1.f / math_sqrt((T)2.f * (T)PI * sigma * sigma)) * exp(-(x * x) / ((T)2.f * sigma * sigma));
    }

    // Vector Structs

	struct v2_f32 {
		
		f32 x = 0.f;
		f32 y = 0.f;

		constexpr v2_f32() = default;
		constexpr v2_f32(f32 x, f32 y) : x(x), y(y) {}
		constexpr v2_f32(f32 n) : x(n), y(n) {}
		v2_f32(XMVECTOR v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)) {}

		f32& operator[](u32 index) {
			return (index == 0u) ? x : y;
		}
		const f32& operator[](u32 index) const {
			return (index == 0u) ? x : y;
		}

		bool operator==(v2_f32 v) const {
			return x == v.x && y == v.y;
		}
	    bool operator!=(v2_f32 v) const {
			return x != v.x && y != v.y;
		}

	    void operator+=(v2_f32 v) {
			x += v.x;
			y += v.y;
		}

	    void operator+=(f32 v) {
			x += v;
			y += v;
		}

	    void operator-=(v2_f32 v) {
			x -= v.x;
			y -= v.y;
		}

	    void operator-=(f32 v) {
			x -= v;
			y -= v;
		}

	    void operator*=(v2_f32 v) {
			x *= v.x;
			y *= v.y;
		}

	    void operator*=(f32 v) {
			x *= v;
			y *= v;
		}

	    void operator/=(v2_f32 v) {
			x /= v.x;
			y /= v.y;
		}

	    void operator/=(f32 v) {
			x /= v;
			y /= v;
		}

		static v2_f32 up() { return { 0.f, 1.f }; }
	    static v2_f32 down() { return { 0.f, -1.f }; }
	    static v2_f32 left() { return { -1.f, 0.f }; }
	    static v2_f32 right() { return { 1.f, 0.f }; }
	    static v2_f32 zero() { return { 0.f, 0 }; }
	    static v2_f32 one() { return { 1.f, 1.f }; }
	};

	SV_INLINE v2_f32 operator+(v2_f32 v0, v2_f32 v1)
    {
		return v2_f32{ v0.x + v1.x, v0.y + v1.y };
    }
	SV_INLINE v2_f32 operator+(v2_f32 v, f32 n)
    {
		return v2_f32{ v.x + n, v.y + n };
    }
	SV_INLINE v2_f32 operator+(f32 n, v2_f32 v)
    {
		return v2_f32{ v.x + n, v.y + n };
    }
	SV_INLINE v2_f32 operator-(v2_f32 v0, v2_f32 v1)
    {
		return v2_f32{ v0.x - v1.x, v0.y - v1.y };
    }
	SV_INLINE v2_f32 operator-(v2_f32 v, f32 n)
    {
		return v2_f32{ v.x - n, v.y - n };
    }
	SV_INLINE v2_f32 operator-(f32 n, v2_f32 v)
    {
		return v2_f32{ n - v.x, n - v.y };
    }
	SV_INLINE v2_f32 operator*(v2_f32 v0, v2_f32 v1)
    {
		return v2_f32{ v0.x * v1.x, v0.y * v1.y };
    }
	SV_INLINE v2_f32 operator*(v2_f32 v, f32 n)
    {
		return v2_f32{ v.x * n, v.y * n };
    }
	SV_INLINE v2_f32 operator*(f32 n, v2_f32 v)
    {
		return v2_f32{ n * v.x, n * v.y };
    }
	SV_INLINE v2_f32 operator/(v2_f32 v0, v2_f32 v1)
    {
		return v2_f32{ v0.x / v1.x, v0.y / v1.y };
    }
	SV_INLINE v2_f32 operator/(v2_f32 v, f32 n)
    {
		return v2_f32{ v.x / n, v.y / n };
    }
	SV_INLINE v2_f32 operator/(f32 n, v2_f32 v)
    {
		return v2_f32{ n / v.x, n / v.y };
    }
    
	SV_INLINE f32 vec2_length(v2_f32 v)
	{
		return math_sqrt(v.x * v.x + v.y * v.y);
	}

	SV_INLINE v2_f32 vec2_normalize(v2_f32 v)
	{
		f32 mag = vec2_length(v);
		v.x /= mag;
		v.y /= mag;
		return v;
	}

	SV_INLINE v2_f32 vec2_to(v2_f32 from, v2_f32 to)
	{
		return to - from;
	}

	SV_INLINE f32 vec2_distance(v2_f32 from, v2_f32 to)
	{
		return vec2_length(vec2_to(from, to));
	}
	
	SV_INLINE f32 vec2_angle(v2_f32 v)
	{
		f32 res = atan2f(v.y, v.x);
		if (res < 0.0) {
			res = (2.f * PI) + res;
		}
		return res;
	}
	
	SV_INLINE v2_f32 vec2_set_angle(v2_f32 v, f32 angle)
	{
		f32 mag = vec2_length(v);
		v.x = cosf(angle);
		v.x *= mag;
		v.y = sinf(angle);
		v.y *= mag;
		return v;
	}
	
	SV_INLINE f32 vec2_dot(v2_f32 v0, v2_f32 v1)
	{
		return v0.x * v1.x + v0.y * v1.y;
	}
	
	SV_INLINE v2_f32 vec2_perpendicular(v2_f32 v, bool clockwise = true)
	{
		return clockwise ? v2_f32{ v.y, -v.x } : v2_f32{ -v.y, v.x };
	}
	
	// NOTE: normal must be normalized
	SV_INLINE v2_f32 vec2_reflection(v2_f32 v, v2_f32 normal)
	{
		SV_ASSERT(vec2_length(normal) == 1.f);
		return v - 2.f * vec2_dot(v, normal) * normal;
	}

	struct v2_u32 {

		u32 x = 0u;
		u32 y = 0u;
		
	};

	struct v2_i32 {

		i32 x = 0u;
		i32 y = 0u;
		
	};

    struct v3_f32 {

		f32 x = 0.f;
		f32 y = 0.f;
		f32 z = 0.f;

		constexpr v3_f32() = default;
		constexpr v3_f32(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
		constexpr v3_f32(f32 n) : x(n), y(n), z(n) {}
		v3_f32(XMVECTOR v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)), z(XMVectorGetZ(v)) {}

		const f32& operator[](u32 index) const {
			switch (index) {
			case 0:
				return x;
			case 1:
				return y;
			default:
				return z;
			}
		}
		f32& operator[](u32 index) {
			switch (index) {
			case 0:
				return x;
			case 1:
				return y;
			default:
				return z;
			}
		}

	    bool operator==(v3_f32 v) const {
			return x == v.x && y == v.y && z == v.z;
		}
	    bool operator!=(v3_f32 v) const {
			return x != v.x && y != v.y && z != v.z;
		}

	    void operator+=(v3_f32 v) {
			x += v.x;
			y += v.y;
			z += v.z;
		}

	    void operator+=(f32 v) {
			x += v;
			y += v;
			z += v;
		}

	    void operator-=(v3_f32 v) {
			x -= v.x;
			y -= v.y;
			z -= v.z;
		}

	    void operator-=(f32 v) {
			x -= v;
			y -= v;
			z -= v;
		}

	    void operator*=(v3_f32 v) {
			x *= v.x;
			y *= v.y;
			z *= v.z;
		}

	    void operator*=(f32 v) {
			x *= v;
			y *= v;
			z *= v;
		}

	    void operator/=(v3_f32 v) {
			x /= v.x;
			y /= v.y;
			z /= v.z;
		}

	    void operator/=(f32 v) {
			x /= v;
			y /= v;
			z /= v;
		}

		static v3_f32 up() { return { 0.f, 1.f, 0.f }; }
	    static v3_f32 down() { return { 0.f, -1.f, 0.f }; }
	    static v3_f32 left() { return { -1.f, 0.f, 0.f }; }
	    static v3_f32 right() { return { 1.f, 0.f, 0.f }; }
	    static v3_f32 forward() { return { 0.f, 0.f, 1.f }; }
	    static v3_f32 back() { return { 0.f, 0.f, -1.f }; }
	    static v3_f32 zero() { return { 0.f, 0.f, 0.f }; }
	    static v3_f32 one() { return { 1.f, 1.f, 1.f }; }
		
	};
	
	SV_INLINE v3_f32 operator+(v3_f32 v0, v3_f32 v1)
    {
		return v3_f32{ v0.x + v1.x, v0.y + v1.y, v0.z + v1.z };
    }
	SV_INLINE v3_f32 operator+(v3_f32 v, f32 n)
    {
		return v3_f32{ v.x + n, v.y + n, v.z + n };
    }
	SV_INLINE v3_f32 operator+(f32 n, v3_f32 v)
    {
		return v3_f32{ v.x + n, v.y + n, v.z + n };
    }
	SV_INLINE v3_f32 operator-(v3_f32 v0, v3_f32 v1)
    {
		return v3_f32{ v0.x - v1.x, v0.y - v1.y, v0.z - v1.z };
    }
	SV_INLINE v3_f32 operator-(v3_f32 v, f32 n)
    {
		return v3_f32{ v.x - n, v.y - n, v.z - n };
    }
	SV_INLINE v3_f32 operator-(f32 n, v3_f32 v)
    {
		return v3_f32{ n - v.x, n - v.y, n - v.z };
    }
	SV_INLINE v3_f32 operator*(v3_f32 v0, v3_f32 v1)
    {
		return v3_f32{ v0.x * v1.x, v0.y * v1.y, v0.z * v1.z };
    }
	SV_INLINE v3_f32 operator*(v3_f32 v, f32 n)
    {
		return v3_f32{ v.x * n, v.y * n, v.z * n };
    }
	SV_INLINE v3_f32 operator*(f32 n, v3_f32 v)
    {
		return v3_f32{ n * v.x, n * v.y, n * v.z };
    }
	SV_INLINE v3_f32 operator/(v3_f32 v0, v3_f32 v1)
    {
		return v3_f32{ v0.x / v1.x, v0.y / v1.y, v0.z / v1.z };
    }
	SV_INLINE v3_f32 operator/(v3_f32 v, f32 n)
    {
		return v3_f32{ v.x / n, v.y / n, v.z / n };
    }
	SV_INLINE v3_f32 operator/(f32 n, v3_f32 v)
    {
		return v3_f32{ n / v.x, n / v.y, n / v.z };
    }
    
	SV_INLINE f32 vec3_length(v3_f32 v)
	{
		return math_sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	SV_INLINE v3_f32 vec3_normalize(v3_f32 v)
	{
		f32 mag = vec3_length(v);
		v.x /= mag;
		v.y /= mag;
		v.z /= mag;
		return v;
	}

	SV_INLINE v3_f32 vec3_to(v3_f32 from, v3_f32 to)
	{
		return to - from;
	}

	SV_INLINE f32 vec3_distance(v3_f32 from, v3_f32 to)
	{
		return vec3_length(vec3_to(from, to));
	}
	
	SV_INLINE f32 vec3_dot(v3_f32 v0, v3_f32 v1)
	{
		return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
	}
	
	SV_INLINE v3_f32 vec3_cross(v3_f32 v0, v3_f32 v1)
	{
		v3_f32 r;
		r.x = v0.y * v1.z - v0.z * v1.y;
		r.y = v0.z * v1.x - v0.x * v1.z;
		r.z = v0.x * v1.y - v0.y * v1.x;
		return r;
	}
	
	// NOTE: normal must be normalized
	SV_INLINE v3_f32 vec3_reflection(v3_f32 v, v3_f32 normal)
	{
		SV_ASSERT(vec3_length(normal) == 1.f);
		return 2.f * vec3_dot(v, normal) * normal - v;
	}

	struct v3_u32 {

		u32 x = 0u;
		u32 y = 0u;
		u32 z = 0u;
		
	};

	struct v3_i32 {

		i32 x = 0u;
		i32 y = 0u;
		i32 z = 0u;
		
	};

	struct v4_f32 {

		f32 x = 0.f;
		f32 y = 0.f;
		f32 z = 0.f;
		f32 w = 0.f;

		constexpr v4_f32() = default;
		constexpr v4_f32(f32 n) : x(n), y(n), z(n), w(n) {}
		constexpr v4_f32(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
		v4_f32(XMVECTOR v) : x(XMVectorGetX(v)), y(XMVectorGetY(v)), z(XMVectorGetZ(v)), w(XMVectorGetW(v)) {}

		const f32& operator[](u32 index) const {
			switch (index) {
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
			default:
				return w;
			}
		}
		f32& operator[](u32 index) {
			switch (index) {
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
			default:
				return w;
			}
		}

		bool operator==(v4_f32 v) const {
			return x == v.x && y == v.y && z == v.z && w == v.w;
		}
		bool operator!=(v4_f32 v) const {
			return x != v.x && y != v.y && z != v.z && w != v.w;
		}
		
	    void operator+=(v4_f32 v) {
			x += v.x;
			y += v.y;
			z += v.z;
			w += v.w;
		}
	    void operator+=(f32 v) {
			x += v;
			y += v;
			z += v;
			w += v;
		}
	    void operator-=(v4_f32 v) {
			x -= v.x;
			y -= v.y;
			z -= v.z;
			w -= v.w;
		}
	    void operator-=(f32 v) {
			x -= v;
			y -= v;
			z -= v;
			w -= v;
		}
	    void operator*=(v4_f32 v) {
			x *= v.x;
			y *= v.y;
			z *= v.z;
			w *= v.w;
		}
	    void operator*=(f32 v) {
			x *= v;
			y *= v;
			z *= v;
			w *= v;
		}
	    void operator/=(v4_f32 v) {
			x /= v.x;
			y /= v.y;
			z /= v.z;
			w /= v.w;
		}
	    void operator/=(f32 v) {
			x /= v;
			y /= v;
			z /= v;
			w /= v;
		}
		
	};

	SV_INLINE v4_f32 operator+(v4_f32 v0, v4_f32 v1)
    {
		return v4_f32{ v0.x + v1.x, v0.y + v1.y, v0.z + v1.z, v0.w + v1.w };
    }
	SV_INLINE v4_f32 operator+(v4_f32 v, f32 n)
    {
		return v4_f32{ v.x + n, v.y + n, v.z + n, v.w + n };
    }
	SV_INLINE v4_f32 operator+(f32 n, v4_f32 v)
    {
		return v4_f32{ v.x + n, v.y + n, v.z + n, v.w + n };
    }
	SV_INLINE v4_f32 operator-(v4_f32 v0, v4_f32 v1)
    {
		return v4_f32{ v0.x - v1.x, v0.y - v1.y, v0.z - v1.z, v0.w - v1.w };
    }
	SV_INLINE v4_f32 operator-(v4_f32 v, f32 n)
    {
		return v4_f32{ v.x - n, v.y - n, v.z - n, v.w - n };
    }
	SV_INLINE v4_f32 operator-(f32 n, v4_f32 v)
    {
		return v4_f32{ n - v.x, n - v.y, n - v.z, n - v.w };
    }
	
	SV_INLINE f32 vec4_length(v4_f32 v)
	{
		return math_sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	}
	
	SV_INLINE v4_f32 vec4_normalize(v4_f32 v)
	{
		f32 m = vec4_length(v);
		v.x /= m;
		v.y /= m;
		v.z /= m;
		v.w /= m;
		return v;
	}

	SV_INLINE v4_f32 vec4_to(v4_f32 from, v4_f32 to)
	{
		return to - from;
	}

	SV_INLINE f32 vec4_distance(v4_f32 from, v4_f32 to)
	{
		return vec4_length(vec4_to(from, to));
	}

	SV_INLINE v3_f32 vec2_to_vec3(v2_f32 v, f32 z = 0.f)
	{
		return v3_f32{ v.x, v.y, z };
	}
	SV_INLINE v4_f32 vec2_to_vec4(v2_f32 v, f32 z = 0.f, f32 w = 0.f)
	{
		return v4_f32{ v.x, v.y, z, w };
	}
	SV_INLINE XMVECTOR vec2_to_dx(v2_f32 v, f32 z = 0.f, f32 w = 0.f)
	{
		return XMVectorSet(v.x, v.y, z, w);
	}
	SV_INLINE v2_f32 vec3_to_vec2(v3_f32 v)
	{
		return v2_f32{ v.x, v.y };
	}
	SV_INLINE v4_f32 vec3_to_vec4(v3_f32 v, f32 w = 0.f)
	{
		return v4_f32{ v.x, v.y, v.z, w };
	}
	SV_INLINE XMVECTOR vec3_to_dx(v3_f32 v, f32 w = 0.f)
	{
		return XMVectorSet(v.x, v.y, v.z, w);
	}
	SV_INLINE v2_f32 vec4_to_vec2(v4_f32 v)
	{
		return v2_f32{ v.x, v.y };
	}
	SV_INLINE v3_f32 vec4_to_vec3(v4_f32 v)
	{
		return v3_f32{ v.x, v.y, v.z };
	}
	SV_INLINE XMVECTOR vec4_to_dx(v4_f32 v)
	{
		return XMVectorSet(v.x, v.y, v.z, v.w);
	}

    // Color

    struct SV_API Color {

		union {
			struct {
				u8 r, g, b, a;
			};
			u32 rgba;
		};

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

		bool operator==(const Color& o) { return o.r == r && o.g == g && o.b == b; }
		bool operator!=(const Color& o) { return o.r != r || o.g != g || o.b != b; }
	
    };

	constexpr Color color_float(f32 r, f32 g, f32 b, f32 a = 1.f)
	{
		return { u8(r * 255.f), u8(g * 255.f), u8(b * 255.f), u8(a * 255.f) };
	}

	constexpr v3_f32 color_to_vec3(Color c)
	{
		constexpr f32 mult = (1.f / 255.f);
		return { f32(c.r) * mult, f32(c.g) * mult, f32(c.b) * mult };
	}
	
	constexpr v4_f32 color_to_vec4(Color c)
	{
		constexpr f32 mult = (1.f / 255.f);
		return { f32(c.r) * mult, f32(c.g) * mult, f32(c.b) * mult, f32(c.a) * mult };
	}

    SV_INLINE Color color_blend(Color c0, Color c1)
    {
		Color c;
		c.r = (c0.r / 2u) + (c1.r / 2u);
		c.g = (c0.g / 2u) + (c1.g / 2u);
		c.b = (c0.b / 2u) + (c1.b / 2u);
		c.a = (c0.a / 2u) + (c1.a / 2u);
		return c;
    }

    SV_INLINE Color color_interpolate(Color c0, Color c1, f32 n)
    {
		Color c;
		c.r = u8(f32(c0.r) * (1.f - n)) + u8(f32(c1.r) * n);
		c.g = u8(f32(c0.g) * (1.f - n)) + u8(f32(c1.g) * n);
		c.b = u8(f32(c0.b) * (1.f - n)) + u8(f32(c1.b) * n);
		c.a = u8(f32(c0.a) * (1.f - n)) + u8(f32(c1.a) * n);
		return c;
    }

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
			pos = (f32)cos(PI - pos * PI) * 0.5f + 0.5f;

			f32 h1 = math_random_f32(seed * 24332 * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

			return h1 + pos * (h0 - h1);
		}
		else {

			++offset;
			p1 = f32(offset) + math_random_f32(seed * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

			f32 pos = (n - p0) / (p1 - p0);
			pos = (f32)cos(PI - pos * -PI) * 0.5f + 0.5f;

			f32 h1 = math_random_f32(seed * 24332 * (u32(offset) + 0x9e3779b9 + (u32(offset) << 6) + (u32(offset) >> 2)));

			return h0 + pos * (h1 - h0);
		}
    }

    // Matrix

    XMMATRIX math_matrix_view(const v3_f32& position, const v4_f32& directionQuat);

	// Quaternion

	// TODO: WTF is going on
	SV_INLINE v3_f32 quaternion_to_euler_angles(v4_f32 quat)
	{
		v3_f32 euler;

		// roll (x-axis rotation)

		XMFLOAT4 q;
		q.x = quat.x;
		q.y = quat.y;
		q.z = quat.z;
		q.w = quat.w;
		float sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
		float cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
		euler.x = atan2f(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float sinp = 2.f * (q.w * q.y - q.z * q.x);
		if (abs(sinp) >= 1.f)
			euler.y = copysignf(PI / 2.f, sinp); // use 90 degrees if out of range
		else
			euler.y = asinf(sinp);

		// yaw (z-axis rotation)
		float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
		float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
		euler.z = atan2f(siny_cosp, cosy_cosp);

		if (euler.x < 0.f) {
			euler.x = 2.f * PI + euler.x;
		}
		if (euler.y < 0.f) {
			euler.y = 2.f * PI + euler.y;
		}
		if (euler.z < 0.f) {
			euler.z = 2.f * PI + euler.z;
		}

		return euler;
    }

	// TODO: WTF is going on 2
	SV_INLINE v4_f32 quaternion_from_euler_angles(v3_f32 euler)
	{
		float cy = cosf(euler.z * 0.5f);
		float sy = sinf(euler.z * 0.5f);
		float cp = cosf(euler.y * 0.5f);
		float sp = sinf(euler.y * 0.5f);
		float cr = cosf(euler.x * 0.5f);
		float sr = sinf(euler.x * 0.5f);

		v4_f32 q;
		q.w = cr * cp * cy + sr * sp * sy;
		q.x = sr * cp * cy - cr * sp * sy;
		q.y = cr * sp * cy + sr * cp * sy;
		q.z = cr * cp * sy - sr * sp * cy;

		return q;
	}

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
		h = vec3_cross(ray.direction, edge2);
		a = vec3_dot(edge1, h);
		if (a > -EPSILON && a < EPSILON)
			return false;    // This ray is parallel to this triangle.
		f = 1.f / a;
		s = ray.origin - v0;
		u = f * vec3_dot(s, h);
		if (u < 0.0 || u > 1.0)
			return false;
		q = vec3_cross(s, edge1);
		v = f * vec3_dot(ray.direction, q);
		if (v < 0.0 || u + v > 1.0)
			return false;
		// At this stage we can compute t to find out where the intersection point is on the line.
		float t = f * vec3_dot(edge2, q);
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
