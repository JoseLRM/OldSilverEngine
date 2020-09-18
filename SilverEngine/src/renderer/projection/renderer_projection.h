#pragma once

#include "core.h"

namespace sv {

	enum CameraType : ui32 {
		CameraType_Clip,
		CameraType_Orthographic,
		CameraType_Perspective,
	};

	struct CameraProjection {
		CameraType cameraType = CameraType_Orthographic;
		float width = 1.f;
		float height = 1.f;
		float near = -10000.0f;
		float far = 10000.f;
	};

	XMMATRIX	renderer_projection_matrix(const CameraProjection& projection);
	float		renderer_projection_aspect_get(const CameraProjection& projection);
	void		renderer_projection_aspect_set(CameraProjection& projection, float aspect);
	vec2f		renderer_projection_position(const CameraProjection& projection, const vec2f& point); // The point must be in range { -0.5 - 0.5 }
	float		renderer_projection_zoom_get(const CameraProjection& projection);
	void		renderer_projection_zoom_set(CameraProjection& projection, float zoom);

}