#include "core.h"

#include "Camera.h"
#include "renderer.h"
#include "engine_input.h"
#include "platform/Window.h"

namespace sv {

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

	XMMATRIX OrthographicCamera::GetProjectionMatrix() const
	{
		return XMMatrixOrthographicLH(m_Dimension.x, m_Dimension.y, -100000.f, 100000.f);
	}
	XMMATRIX OrthographicCamera::GetViewMatrix() const
	{
		return XMMatrixRotationZ(-m_Rotation) * XMMatrixTranslation(-m_Position.x, m_Position.y, 0.f);
	}
	void OrthographicCamera::Adjust()
	{
		float mag = m_Dimension.Mag();
		m_Dimension = vec2(window_get_width(), window_get_height());
		m_Dimension.Normalize();
		m_Dimension *= mag;
	}
	float OrthographicCamera::GetAspect() const noexcept
	{
		return m_Dimension.x / m_Dimension.y;
	}
	vec2 OrthographicCamera::GetMousePos()
	{
		vec2 pos = input_mouse_position_get();
		return pos * m_Dimension + m_Position;
	}
	void OrthographicCamera::SetPosition(vec2 position) noexcept
	{
		m_Position = position;
	}
	void OrthographicCamera::SetDimension(vec2 dimension) noexcept
	{
		m_Dimension = dimension;
	}
	void OrthographicCamera::SetZoom(float zoom) noexcept
	{
		if (zoom < 0.9999f) zoom = 0.9999f;
		m_Dimension.Normalize();
		m_Dimension *= zoom;
	}
	void OrthographicCamera::SetRotation(float rotation) noexcept
	{
		m_Rotation = rotation;
	}
	void OrthographicCamera::SetAspect(float aspect) noexcept
	{
		float mag = m_Dimension.Mag();
		m_Dimension.x = aspect;
		m_Dimension.y = 1.f;
		m_Dimension.Normalize();
		m_Dimension *= mag;
	}

}