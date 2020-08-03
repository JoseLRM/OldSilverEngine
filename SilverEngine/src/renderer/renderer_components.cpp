#include "core.h"

#include "renderer_components.h"
#include "renderer.h"

namespace sv {

	CameraComponent::CameraComponent() 
	{}

	CameraComponent::CameraComponent(SV_REND_CAMERA_TYPE type)
	{
		projection = CameraProjection(type);
	}

	CameraComponent::CameraComponent(const CameraComponent& other)
	{
		this->operator=(other);
	}
	CameraComponent::CameraComponent(CameraComponent&& other) noexcept
	{
		this->operator=(std::move(other));
	}
	CameraComponent& CameraComponent::operator=(const CameraComponent& other)
	{
		projection = other.projection;
		if (other.HasOffscreen()) {
			_sv::Offscreen& offscreen = *other.GetOffscreen();
			CreateOffscreen(offscreen.GetWidth(), offscreen.GetHeight());
		}
		return *this;
	}
	CameraComponent& CameraComponent::operator=(CameraComponent&& other) noexcept
	{
		projection = other.projection;
		if (other.HasOffscreen()) {
			m_Offscreen = std::move(other.m_Offscreen);
		}
		return *this;
	}

	void CameraComponent::Adjust(float width, float height)
	{
		switch (projection.GetCameraType())
		{
		case SV_REND_CAMERA_TYPE_ORTHOGRAPHIC:
			projection.Orthographic_SetAspect(width / height);
			break;
		case SV_REND_CAMERA_TYPE_PERSPECTIVE:
			sv::log_error("TODO");
			break;
		}
	}

	bool CameraComponent::CreateOffscreen(ui32 width, ui32 height)
	{
		if (HasOffscreen()) {
			DestroyOffscreen();
		}
		m_Offscreen = std::make_unique<_sv::Offscreen>();
		return _sv::renderer_offscreen_create(width, height, *m_Offscreen.get());
	}

	bool CameraComponent::HasOffscreen() const noexcept
	{
		return m_Offscreen.get() != nullptr;
	}

	_sv::Offscreen* CameraComponent::GetOffscreen() const noexcept
	{
		return m_Offscreen.get();
	}

	bool CameraComponent::DestroyOffscreen()
	{
		if (HasOffscreen())
			return _sv::renderer_offscreen_destroy(*GetOffscreen());
		else
			return true;
	}

}