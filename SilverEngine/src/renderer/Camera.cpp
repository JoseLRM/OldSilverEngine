#include "core.h"

#include "Camera.h"
#include "Input.h"
#include "graphics/Window.h"

namespace SV {

	XMMATRIX OrthographicCamera::GetProjectionMatrix() const
	{
		return XMMatrixOrthographicLH(m_Dimension.x, m_Dimension.y, -100000.f, 100000.f);
	}
	XMMATRIX OrthographicCamera::GetViewMatrix() const
	{
		return XMMatrixRotationZ(-m_Rotation) * XMMatrixTranslation(-m_Position.x, -m_Position.y, 0.f);
	}
	void OrthographicCamera::Adjust()
	{
		float mag = m_Dimension.Mag();
		m_Dimension = vec2(Window::GetWidth(), Window::GetHeight());
		m_Dimension.Normalize();
		m_Dimension *= mag;
	}
	float OrthographicCamera::GetAspect() const noexcept
	{
		return m_Dimension.x / m_Dimension.y;
	}
	SV::vec2 OrthographicCamera::GetMousePos()
	{
		SV::vec2 pos = Engine::GetMousePos();
		return pos * m_Dimension + m_Position;
	}
	void OrthographicCamera::SetPosition(SV::vec2 position) noexcept
	{
		m_Position = position;
	}
	void OrthographicCamera::SetDimension(SV::vec2 dimension) noexcept
	{
		m_Dimension = dimension;
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