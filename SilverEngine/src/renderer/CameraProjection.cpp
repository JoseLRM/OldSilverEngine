#include "core.h"

#include "renderer.h"
#include "engine_input.h"
#include "platform/Window.h"

namespace sv {

	CameraProjection::CameraProjection() : m_CameraType(SV_REND_CAMERA_TYPE_NONE)
	{
	}
	CameraProjection::CameraProjection(SV_REND_CAMERA_TYPE type) : m_CameraType(type)
	{
		if (type == SV_REND_CAMERA_TYPE_ORTHOGRAPHIC) {
			m_Orthographic.width = 1.f;
			m_Orthographic.height = 1.f;
		}
	}

	// Orthographic

	void CameraProjection::Orthographic_SetDimension(ui32 width, ui32 height)
	{
		m_Orthographic.width = width;
		m_Orthographic.height = height;
	}

	void CameraProjection::Orthographic_SetZoom(float zoom)
	{
		float currentZoom = Orthographic_GetZoom();
		m_Orthographic.width = (m_Orthographic.width / currentZoom) * zoom;
		m_Orthographic.height = (m_Orthographic.height / currentZoom) * zoom;
	}

	void CameraProjection::Orthographic_SetAspect(float aspect)
	{
		m_Orthographic.width = m_Orthographic.height * aspect;
	}

	uvec2 CameraProjection::Orthographic_GetDimension()
	{
		return uvec2();
	}
	float CameraProjection::Orthographic_GetZoom()
	{
		return sqrt(m_Orthographic.width * m_Orthographic.width + m_Orthographic.height * m_Orthographic.height);
	}

	float CameraProjection::Orthographic_GetAspect()
	{
		return m_Orthographic.width / m_Orthographic.height;
	}

	vec2 CameraProjection::Orthographic_GetMousePos()
	{
		return input_mouse_position_get() * vec2(m_Orthographic.width, m_Orthographic.height);
	}

	// General

	XMMATRIX CameraProjection::GetProjectionMatrix() const
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

}