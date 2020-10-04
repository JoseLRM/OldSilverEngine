#pragma once

#include "Mesh.h"

namespace sv {

	enum LightType : ui32 {
		LightType_None,
		LightType_Point,
		LightType_Spot,
		LightType_Directional,
	};

	struct alignas(16) LightInstance {
		LightType	lightType;
		vec3f		position;
		vec3f		direction;
		float		intensity;
		float		range;
		Color3f		color;
		float		smoothness;
		vec3f		padding;

		LightInstance() = default;
		LightInstance(LightType type, const vec3f & position, const vec3f & direction, float intensity, float range, float smoothness, const Color3f & color)
			: lightType(type), position(position), direction(direction), intensity(intensity), range(range), smoothness(smoothness), color(color) {}
	};

}