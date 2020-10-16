#include "core.h"

namespace sv {

	// RANDOM

	// 0u - (ui32_max / 2u)
	inline ui32 math_random_inline(ui32 seed)
	{
		seed = (seed << 13) ^ seed;
		return ((seed * (seed * seed * 15731u * 789221u) + 1376312589u) & 0x7fffffffu);
	}

	ui32 math_random(ui32 seed)
	{
		return math_random_inline(seed);
	}

	ui32 math_random(ui32 seed, ui32 max)
	{
		return math_random_inline(seed) % max;
	}

	ui32 math_random(ui32 seed, ui32 min, ui32 max)
	{
		SV_ASSERT(min <= max);
		return min + (math_random_inline(seed) % (max - min));
	}

	float math_randomf(ui32 seed)
	{
		return (float(math_random(seed)) / float(ui32_max / 2u));
	}

	float math_randomf(ui32 seed, float max)
	{
		return float(math_random_inline(seed) / float(ui32_max / 2u)) * max;
	}

	float math_randomf(ui32 seed, float min, float max)
	{
		SV_ASSERT(min <= max);
		return min + float(math_random_inline(seed) / float(ui32_max / 2u)) * (max - min);
	}

	// QUATERNION

	void math_quaternion_to_euler(sv::vec4f* q, sv::vec3f* e)
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

	// MATRIX

	XMMATRIX math_matrix_view(const vec3f& position, const vec3f& rotation)
	{
		XMVECTOR direction, pos, target;
		
		direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		pos = position.get_dx();

		direction = XMVector3Transform(direction, XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, 0.f));

		target = XMVectorAdd(pos, direction);
		return XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
	}

}