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

	// Intersection

	bool BoundingBox3D::intersects_point(const vec3f& point) const noexcept
	{
		if (point.x < min.x) return false;
		if (point.x > max.x) return false;
		if (point.y < min.y) return false;
		if (point.y > max.y) return false;
		if (point.z < min.z) return false;
		if (point.z > max.z) return false;
		return true;
	}

	bool BoundingBox3D::intersects_ray3D(const Ray3D& ray) const noexcept
	{
		if (intersects_point(ray.origin)) return true;

		float tx1 = (min.x - ray.origin.x) * ray.directionInverse.x;
		float tx2 = (max.x - ray.origin.x) * ray.directionInverse.x;

		float tmin = std::min(tx1, tx2);
		float tmax = std::max(tx1, tx2);

		float ty1 = (min.y - ray.origin.y) * ray.directionInverse.y;
		float ty2 = (max.y - ray.origin.y) * ray.directionInverse.y;

		tmin = std::max(tmin, std::min(ty1, ty2));
		tmax = std::min(tmax, std::max(ty1, ty2));

		float tz1 = (min.z - ray.origin.z) * ray.directionInverse.z;
		float tz2 = (max.z - ray.origin.z) * ray.directionInverse.z;

		tmin = std::max(tmin, std::min(tz1, tz2));
		tmax = std::min(tmax, std::max(tz1, tz2));

		return tmax >= tmin;
	}

	bool BoundingBox2D::intersects_point(const vec2f& point) const noexcept
	{
		if (point.x < min.x) return false;
		if (point.x > max.x) return false;
		if (point.y < min.y) return false;
		if (point.y > max.y) return false;
		return true;
	}

	bool BoundingBox2D::intersects_circle(const Circle2D& circle) const noexcept
	{
		vec2f boxHalf = (max - min) / 2.f;
		vec2f boxCenter = min + boxHalf;

		vec2f distance = circle.position - boxCenter;
		vec2f distanceInsideBox = distance;
		distanceInsideBox.x = std::max(std::min(distance.x, boxHalf.x), -boxHalf.x);
		distanceInsideBox.y = std::max(std::min(distance.y, boxHalf.y), -boxHalf.y);

		return (distance.length() - distanceInsideBox.length()) <= circle.radius;
	}

	bool FrustumOthographic::intersects_circle(const Circle2D& circle) const noexcept
	{
		vec2f distance = circle.position - position;
		vec2f distanceInsideBox = distance;
		distanceInsideBox.x = std::max(std::min(distance.x, halfSize.x), -halfSize.x);
		distanceInsideBox.y = std::max(std::min(distance.y, halfSize.y), -halfSize.y);

		return (distance.length() - distanceInsideBox.length()) <= circle.radius;
	}

}