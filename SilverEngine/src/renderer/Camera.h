#pragma once

#include "..//core.h"
#include "renderer_desc.h"

namespace sv {

	class Camera {
		std::unique_ptr<_sv::Offscreen> m_Offscreen;
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
		Camera();
		Camera(SV_REND_CAMERA_TYPE type);

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

		// Offscreen

		bool CreateOffscreen(ui32 width, ui32 height);
		bool HasOffscreen() const noexcept;
		_sv::Offscreen& GetOffscreen() noexcept;
		bool DestroyOffscreen();

		inline SV_REND_CAMERA_TYPE GetCameraType() const noexcept { return m_CameraType; }

	};

}