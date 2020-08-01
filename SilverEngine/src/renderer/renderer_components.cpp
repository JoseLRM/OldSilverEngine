#include "core.h"

#include "renderer_components.h"
#include "Camera.h"

namespace sv {

	CameraComponent::CameraComponent() 
	{
		camera = std::make_unique<Camera>();
	}

	CameraComponent::CameraComponent(SV_REND_CAMERA_TYPE type)
	{
		camera = std::make_unique<Camera>(type);
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
		camera = std::make_unique<Camera>();
		// TODO: Camera copy constructor
		active = other.active;
		return *this;
	}
	CameraComponent& CameraComponent::operator=(CameraComponent&& other) noexcept
	{
		camera = std::move(other.camera);
		active = other.active;
		return *this;
	}

	void CameraComponent::Adjust(float width, float height)
	{
		switch (camera->GetCameraType())
		{
		case SV_REND_CAMERA_TYPE_ORTHOGRAPHIC:
			camera->Orthographic_SetAspect(width / height);
			break;
		case SV_REND_CAMERA_TYPE_PERSPECTIVE:
			sv::log_error("TODO");
			break;
		}
	}

}