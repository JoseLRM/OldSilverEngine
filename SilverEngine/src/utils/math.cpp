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

	// MATRIX

	XMMATRIX math_matrix_view(const vec3f& position, const vec4f& directionQuat)
	{
		XMVECTOR direction, pos, target;
		
		direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		pos = position.get_dx();

		direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(directionQuat.get_dx()));

		target = XMVectorAdd(pos, direction);
		return XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
	}

}