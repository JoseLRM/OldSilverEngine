#include "core.h"

#include "components.h"

namespace sv {

	CameraComponent::CameraComponent()
	{}

	CameraComponent::CameraComponent(CameraType type)
	{
		projection.cameraType = type;
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
			Offscreen& offscreen = *other.GetOffscreen();
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
		switch (projection.cameraType)
		{
		case CameraType_Orthographic:
			renderer_compute_projection_aspect_set(projection, width / height);
			break;
		case CameraType_Perspective:
			sv::log_error("TODO");
			break;
		}
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

	RigidBody2DComponent::RigidBody2DComponent()
	{
		Body2DDesc desc;
		desc.position = { 0.f, 0.f };
		desc.velocity = { 0.f, 0.f };
		desc.rotation = 0.f;
		desc.angularVelocity = 1.f;
		desc.dynamic = false;
		desc.fixedRotation = true;

		m_Body = body2D_create(&desc);

		SetBoxCollider(1.f, 1.f);
	}

	RigidBody2DComponent::RigidBody2DComponent(Collider2DType colliderType)
	{
		Body2DDesc desc;
		desc.position = { 0.f, 0.f };
		desc.velocity = { 0.f, 0.f };
		desc.rotation = 0.f;
		desc.angularVelocity = 1.f;
		desc.dynamic = true;
		desc.fixedRotation = false;

		m_Body = body2D_create(&desc);

		if (colliderType == Collider2DType_Box) SetBoxCollider(1.f, 1.f);
		else SetCircleCollider(1.f);
	}

	RigidBody2DComponent& RigidBody2DComponent::operator=(const RigidBody2DComponent& other)
	{
		if (other.m_Body && other.m_Collider) {

			Body2DDesc desc;
			desc.position = { 0.f, 0.f };
			desc.velocity = { 0.f, 0.f };
			desc.rotation = 0.f;
			desc.angularVelocity = 1.f;
			desc.dynamic = other.IsDynamic();
			desc.fixedRotation = false;

			m_Body = body2D_create(&desc);

			float density = other.GetDensity();

			if (other.GetColliderType() == Collider2DType_Box) {
				SetBoxCollider(1.f, 1.f);
			}
			else {
				SetCircleCollider(1.f);
			}

		}
		return *this;
	}

	RigidBody2DComponent& RigidBody2DComponent::operator=(RigidBody2DComponent&& other) noexcept
	{
		m_Body = other.m_Body;
		m_Collider = other.m_Collider;
		other.m_Body = nullptr;
		other.m_Collider = nullptr;
		return *this;
	}

	RigidBody2DComponent::~RigidBody2DComponent()
	{
		body2D_destroy(m_Body);
	}

	void RigidBody2DComponent::SetDensity(float density) const noexcept
	{
		collider2D_density_set(density, m_Collider);
	}

	float RigidBody2DComponent::GetDensity() const noexcept
	{
		return collider2D_density_get(m_Collider);
	}

	void RigidBody2DComponent::SetDynamic(bool dynamic) const noexcept
	{
		body2D_dynamic_set(dynamic, m_Body);
	}
	bool RigidBody2DComponent::IsDynamic() const noexcept
	{
		return body2D_dynamic_get(m_Body);
	}

	void RigidBody2DComponent::SetFixedRotation(bool fixedRotation) const noexcept
	{
		body2D_fixedRotation_set(fixedRotation, m_Body);
	}
	bool RigidBody2DComponent::IsFixedRotation() const noexcept
	{
		return body2D_fixedRotation_get(m_Body);
	}

	Collider2DType RigidBody2DComponent::GetColliderType() const noexcept
	{
		return collider2D_type_get(m_Collider);
	}

	void RigidBody2DComponent::SetBoxCollider(float width, float height)
	{
		Collider2DDesc desc;
		desc.density = 1.f;
		desc.colliderType = Collider2DType_Box;
		desc.body = m_Body;
		desc.box.width = width;
		desc.box.height = height;

		collider2D_destroy(m_Collider);
		m_Collider = collider2D_create(&desc);
	}
	float RigidBody2DComponent::BoxCollider_GetWidth()
	{
		return 0.f;
	}
	float RigidBody2DComponent::BoxCollider_GetHeight()
	{
		return 0.f;
	}
	void RigidBody2DComponent::BoxCollider_SetSize(float width, float height)
	{
	}

	void RigidBody2DComponent::SetCircleCollider(float radius)
	{
		Collider2DDesc desc;
		desc.density = GetDensity();
		desc.colliderType = Collider2DType_Circle;
		desc.body = m_Body;
		desc.circle.radius = radius;

		collider2D_destroy(m_Collider);
		m_Collider = collider2D_create(&desc);
	}
	float RigidBody2DComponent::CircleCollider_GetRadius()
	{
		return 0.f;
	}
	void RigidBody2DComponent::CircleCollider_SetRadius(float radius)
	{

	}

}