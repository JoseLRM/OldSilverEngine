#include "core.h"

namespace sv {

	// MATRIX

	XMMATRIX math_matrix_view(const v3_f32& position, const v4_f32& directionQuat)
	{
		XMVECTOR direction, pos, target;
		
		direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		pos = position.getDX();

		direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(directionQuat.get_dx()));

		target = XMVectorAdd(pos, direction);
		return XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
	}

	// Intersection

	bool BoundingBox3D::intersects_point(const v3_f32& point) const noexcept
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

	bool BoundingBox2D::intersects_point(const v2_f32& point) const noexcept
	{
		if (point.x < min.x) return false;
		if (point.x > max.x) return false;
		if (point.y < min.y) return false;
		if (point.y > max.y) return false;
		return true;
	}

	bool BoundingBox2D::intersects_circle(const Circle2D& circle) const noexcept
	{
		v2_f32 boxHalf = (max - min) / 2.f;
		v2_f32 boxCenter = min + boxHalf;

		v2_f32 distance = circle.position - boxCenter;
		v2_f32 distanceInsideBox = distance;
		distanceInsideBox.x = std::max(std::min(distance.x, boxHalf.x), -boxHalf.x);
		distanceInsideBox.y = std::max(std::min(distance.y, boxHalf.y), -boxHalf.y);

		return (distance.length() - distanceInsideBox.length()) <= circle.radius;
	}

	bool FrustumOthographic::intersects_circle(const Circle2D& circle) const noexcept
	{
		v2_f32 distance = circle.position - position;
		v2_f32 distanceInsideBox = distance;
		distanceInsideBox.x = std::max(std::min(distance.x, halfSize.x), -halfSize.x);
		distanceInsideBox.y = std::max(std::min(distance.y, halfSize.y), -halfSize.y);

		return (distance.length() - distanceInsideBox.length()) <= circle.radius;
	}

}