#include "core.h"

#include "renderer.h"

namespace sv {

	XMMATRIX renderer_compute_projection_matrix(const CameraProjection& projection)
	{
		XMMATRIX matrix = XMMatrixIdentity();

		switch (projection.cameraType)
		{
		case CameraType_Orthographic:
			matrix = XMMatrixOrthographicLH(projection.orthographic.width, -projection.orthographic.height, -100000.f, 100000.f);
			break;
		case CameraType_Perspective:
			break;
		}

		return matrix;
	}

	float renderer_compute_projection_aspect_get(const CameraProjection& projection)
	{
		switch (projection.cameraType)
		{
		case CameraType_Orthographic:
			return projection.orthographic.width / projection.orthographic.height;
		case CameraType_Perspective:
			log_warning("Perspective aspect: TODO");
			return 1.f;
		default:
			return 1.f;
		}
	}

	void renderer_compute_projection_aspect_set(CameraProjection& projection, float aspect)
	{
		projection.orthographic.width = projection.orthographic.height * aspect;
	}

	vec2 renderer_compute_orthographic_position(const CameraProjection& projection, const vec2& point)
	{
		return point * vec2(projection.orthographic.width, projection.orthographic.height);
	}

	float renderer_compute_orthographic_zoom_get(const CameraProjection& projection)
	{
		return sqrt(projection.orthographic.width * projection.orthographic.width + projection.orthographic.height * projection.orthographic.height);
	}

	void renderer_compute_orthographic_zoom_set(CameraProjection& projection, float zoom)
	{
		float currentZoom = renderer_compute_orthographic_zoom_get(projection);
		if (zoom < 0.0001f) zoom = 0.0001f;
		projection.orthographic.width = (projection.orthographic.width / currentZoom) * zoom;
		projection.orthographic.height = (projection.orthographic.height / currentZoom) * zoom;
	}

}