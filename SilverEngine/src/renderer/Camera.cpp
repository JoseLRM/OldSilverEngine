#include "core.h"

#include "Camera.h"
#include "renderer.h"
#include "engine_input.h"
#include "platform/Window.h"

namespace sv {

	Camera::Camera() : m_CameraType(SV_REND_CAMERA_TYPE_NONE)
	{
	}
	Camera::Camera(SV_REND_CAMERA_TYPE type) : m_CameraType(type)
	{
		if (type == SV_REND_CAMERA_TYPE_ORTHOGRAPHIC) {
			m_Orthographic.width = 1.f;
			m_Orthographic.height = 1.f;
		}
	}

	// Orthographic

	void Camera::Orthographic_SetDimension(ui32 width, ui32 height)
	{
		m_Orthographic.width = width;
		m_Orthographic.height = height;
	}

	void Camera::Orthographic_SetZoom(float zoom)
	{
		float currentZoom = Orthographic_GetZoom();
		m_Orthographic.width = (m_Orthographic.width / currentZoom) * zoom;
		m_Orthographic.height = (m_Orthographic.height / currentZoom) * zoom;
	}

	void Camera::Orthographic_SetAspect(float aspect)
	{
		m_Orthographic.width = m_Orthographic.height * aspect;
	}

	uvec2 Camera::Orthographic_GetDimension()
	{
		return uvec2();
	}
	float Camera::Orthographic_GetZoom()
	{
		return sqrt(m_Orthographic.width * m_Orthographic.width + m_Orthographic.height * m_Orthographic.height);
	}

	float Camera::Orthographic_GetAspect()
	{
		return m_Orthographic.width / m_Orthographic.height;
	}

	vec2 Camera::Orthographic_GetMousePos()
	{
		return input_mouse_position_get() * vec2(m_Orthographic.width, m_Orthographic.height);
	}

	// General

	XMMATRIX Camera::GetProjectionMatrix() const
	{
		XMMATRIX matrix = XMMatrixIdentity();

		switch (m_CameraType)
		{
		case SV_REND_CAMERA_TYPE_ORTHOGRAPHIC:
			matrix = XMMatrixOrthographicLH(m_Orthographic.width, m_Orthographic.height, -100000.f, 100000.f);
			break;
		case SV_REND_CAMERA_TYPE_PERSPECTIVE:
			break;
		}

		return matrix;
	}

	bool Camera::CreateOffscreen(ui32 width, ui32 height)
	{
		if (HasOffscreen()) {
			DestroyOffscreen();
		}
		m_Offscreen = std::make_unique<_sv::Offscreen>();
		return _sv::renderer_offscreen_create(width, height, *m_Offscreen.get());
	}

	bool Camera::HasOffscreen() const noexcept
	{
		return m_Offscreen.get() != nullptr;
	}

	_sv::Offscreen& Camera::GetOffscreen() noexcept
	{
		SV_ASSERT(HasOffscreen());
		return *m_Offscreen.get();
	}

	bool Camera::DestroyOffscreen()
	{
		SV_ASSERT(HasOffscreen());
		return _sv::renderer_offscreen_destroy(GetOffscreen());
	}

}