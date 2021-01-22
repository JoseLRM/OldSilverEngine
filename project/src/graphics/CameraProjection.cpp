#include "SilverEngine/core.h"

#include "SilverEngine/graphics.h"

namespace sv {

	void CameraProjection::adjust(u32 width, u32 height) noexcept
	{
		adjust(f32(width) / f32(height));
	}

	void CameraProjection::adjust(f32 aspect) noexcept
	{
		if (width / height == aspect) return;
		v2_f32 res = { width, height };
		f32 mag = res.length();
		res.x = aspect;
		res.y = 1.f;
		res.normalize();
		res *= mag;
		width = res.x;
		height = res.y;
	}

	f32 CameraProjection::getProjectionLength() const noexcept
	{
		return math_sqrt(width * width + height * height);
	}

	void CameraProjection::setProjectionLength(f32 length) noexcept
	{
		f32 currentLength = getProjectionLength();
		if (currentLength == length) return;
		width = width / currentLength * length;
		height = height / currentLength * length;
	}

	void CameraProjection::updateMatrix()
	{
#ifndef SV_DIST
		if (near >= far) {
			SV_LOG_WARNING("Computing the projection matrix. The far must be grater than near");
		}

		switch (projectionType)
		{
		case ProjectionType_Orthographic:
			break;

		case ProjectionType_Perspective:
			if (near <= 0.f) {
				SV_LOG_WARNING("In perspective projection, near must be greater to 0");
			}
			break;
		}
#endif

		switch (projectionType)
		{
		case ProjectionType_Orthographic:
			projectionMatrix = XMMatrixOrthographicLH(width, height, near, far);
			break;

		case ProjectionType_Perspective:
			projectionMatrix = XMMatrixPerspectiveLH(width, height, near, far);
			break;

		default:
			projectionMatrix = XMMatrixIdentity();
			break;
		}
	}

}