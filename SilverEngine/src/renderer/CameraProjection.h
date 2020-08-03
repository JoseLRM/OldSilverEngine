#pragma once

#include "..//core.h"

enum SV_REND_CAMERA_TYPE : ui8 {
	SV_REND_CAMERA_TYPE_NONE,
	SV_REND_CAMERA_TYPE_ORTHOGRAPHIC,
	SV_REND_CAMERA_TYPE_PERSPECTIVE
};

namespace sv {

	class CameraProjection {
		SV_REND_CAMERA_TYPE m_CameraType;

		struct OrthographicCamera {
			float width = 1.f;
			float height = 1.f;
		};
		struct PerspectiveCamera {

		};

		union {
			OrthographicCamera m_Orthographic;
			PerspectiveCamera m_Perspective;
		};

	public:
		CameraProjection();
		CameraProjection(SV_REND_CAMERA_TYPE type);

		// Orthographic

		void Orthographic_SetDimension(ui32 width, ui32 height);
		void Orthographic_SetZoom(float zoom);
		void Orthographic_SetAspect(float aspect);

		uvec2 Orthographic_GetDimension();
		float Orthographic_GetZoom();
		float Orthographic_GetAspect();
		vec2 Orthographic_GetMousePos();

		// Perspective

		// General

		XMMATRIX GetProjectionMatrix() const;

		inline SV_REND_CAMERA_TYPE GetCameraType() const noexcept { return m_CameraType; }

	};

}