#include "core.h"

#include "scene_internal.h"

#include "physics_impl.h"

namespace sv {

	CameraComponent::CameraComponent()
	{
		settings.active = true;
	}

	CameraComponent::CameraComponent(CameraType type)
	{
		settings.projection.cameraType = type;
		settings.active = true;
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
		settings = other.settings;
		if (other.HasOffscreen()) {
			Offscreen& offscreen = *other.GetOffscreen();
			CreateOffscreen(offscreen.GetWidth(), offscreen.GetHeight());
		}
		return *this;
	}
	CameraComponent& CameraComponent::operator=(CameraComponent&& other) noexcept
	{
		settings = other.settings;
		if (other.HasOffscreen()) {
			m_Offscreen = std::move(other.m_Offscreen);
		}
		return *this;
	}

	void CameraComponent::Adjust(float width, float height)
	{
		renderer_projection_aspect_set(settings.projection, width / height);
	}

	bool CameraComponent::CreateOffscreen(ui32 width, ui32 height)
	{
		if (HasOffscreen()) {
			DestroyOffscreen();
		}
		m_Offscreen = std::make_unique<Offscreen>();
		return renderer_offscreen_create(width, height, *m_Offscreen.get());
	}

	bool CameraComponent::HasOffscreen() const noexcept
	{
		return m_Offscreen.get() != nullptr;
	}

	Offscreen* CameraComponent::GetOffscreen() const noexcept
	{
		return m_Offscreen.get();
	}

	bool CameraComponent::DestroyOffscreen()
	{
		if (HasOffscreen())
			return renderer_offscreen_destroy(*GetOffscreen());
		else
			return true;
	}

	RigidBody2DComponent::RigidBody2DComponent() : pInternal(nullptr)
	{}

	RigidBody2DComponent& RigidBody2DComponent::operator=(const RigidBody2DComponent& other)
	{
		dynamic = other.dynamic;
		fixedRotation = other.fixedRotation;
		velocity = other.velocity;
		angularVelocity = other.angularVelocity;

		return *this;
	}

	RigidBody2DComponent& RigidBody2DComponent::operator=(RigidBody2DComponent&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		dynamic = other.dynamic;
		fixedRotation = other.fixedRotation;
		velocity = other.velocity;
		angularVelocity = other.angularVelocity;
		return *this;
	}

	RigidBody2DComponent::~RigidBody2DComponent()
	{
		if (pInternal) {
			b2Body* body = reinterpret_cast<b2Body*>(pInternal);
			b2World& world = *body->GetWorld();
			world.DestroyBody(body);
		}
		pInternal = nullptr;
	}

	// QUAD COMPONENT

	QuadComponent::QuadComponent() : pInternal(nullptr)
	{}

	QuadComponent::~QuadComponent()
	{
		if (pInternal) {

			b2Fixture* fixture = reinterpret_cast<b2Fixture*>(pInternal);
			b2Body* body = fixture->GetBody();
			body->DestroyFixture(fixture);
			pInternal = nullptr;

		}
	}

	QuadComponent& QuadComponent::operator=(const QuadComponent& other)
	{
		density = other.density;
		friction = other.friction;
		angularOffset = other.angularOffset;
		offset = other.offset;
		restitution = other.restitution;
		size = other.size;
		return *this;
	}

	QuadComponent& QuadComponent::operator=(QuadComponent&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		density = other.density;
		friction = other.friction;
		angularOffset = other.angularOffset;
		offset = other.offset;
		restitution = other.restitution;
		size = other.size;
		return *this;
	}

}