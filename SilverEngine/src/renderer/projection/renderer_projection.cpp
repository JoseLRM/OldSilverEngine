#include "core.h"

#include "renderer/renderer_internal.h"

namespace sv {

	XMMATRIX renderer_projection_matrix(const CameraProjection& projection)
	{
		XMMATRIX matrix = XMMatrixIdentity();

		switch (projection.cameraType)
		{
		case CameraType_Orthographic:
			matrix = XMMatrixOrthographicLH(projection.width, projection.height, projection.near, projection.far);
			break;
		case CameraType_Perspective:
			matrix = XMMatrixPerspectiveLH(projection.width, projection.height, projection.near, projection.far);
			break;
		}

		return matrix;
	}

	float renderer_projection_aspect_get(const CameraProjection& projection)
	{
		return projection.width / projection.height;
	}

	void renderer_projection_aspect_set(CameraProjection& projection, float aspect)
	{
		projection.width = projection.height * aspect;
	}

	vec2 renderer_projection_position(const CameraProjection& projection, const vec2& point)
	{
		return point * vec2(projection.width, projection.height);
	}

	float renderer_projection_zoom_get(const CameraProjection& projection)
	{
		return sqrt(projection.width * projection.width + projection.height * projection.height);
	}

	void renderer_projection_zoom_set(CameraProjection& projection, float zoom)
	{
		float currentZoom = renderer_projection_zoom_get(projection);
		if (zoom < 0.0001f) zoom = 0.0001f;
		projection.width = (projection.width / currentZoom) * zoom;
		projection.height = (projection.height / currentZoom) * zoom;
	}

}