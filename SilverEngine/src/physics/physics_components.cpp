#include "core.h"

#include "physics_components.h"

namespace sv {

	RigidBody2DComponent::RigidBody2DComponent()
	{
		SV_PHY2D_BODY_DESC desc;
		desc.position = { 0.f, 0.f };
		desc.velocity = { 0.f, 0.f };
		desc.rotation = 0.f;
		desc.angularVelocity = 1.f;
		desc.dynamic = false;
		desc.fixedRotation = true;

		m_Body = physics_2d_body_create(&desc, scene_physics_world_get());

		SetBoxCollider(1.f, 1.f);
	}

	RigidBody2DComponent::RigidBody2DComponent(SV_PHY2D_COLLIDER_TYPE colliderType)
	{
		SV_PHY2D_BODY_DESC desc;
		desc.position = { 0.f, 0.f };
		desc.velocity = { 0.f, 0.f };
		desc.rotation = 0.f;
		desc.angularVelocity = 1.f;
		desc.dynamic = true;
		desc.fixedRotation = false;

		m_Body = physics_2d_body_create(&desc, scene_physics_world_get());

		if (colliderType == SV_PHY2D_COLLIDER_TYPE_BOX) SetBoxCollider(1.f, 1.f);
		else SetCircleCollider(1.f);
	}

	RigidBody2DComponent& RigidBody2DComponent::operator=(const RigidBody2DComponent& other)
	{
		if (other.m_Body && other.m_Collider) {

			SV_PHY2D_BODY_DESC desc;
			desc.position = { 0.f, 0.f };
			desc.velocity = { 0.f, 0.f };
			desc.rotation = 0.f;
			desc.angularVelocity = 1.f;
			desc.dynamic = other.IsDynamic();
			desc.fixedRotation = false;

			m_Body = physics_2d_body_create(&desc, scene_physics_world_get());

			float density = other.GetDensity();

			if (other.GetColliderType() == SV_PHY2D_COLLIDER_TYPE_BOX) {
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
		physics_2d_body_destroy(m_Body);
	}

	void RigidBody2DComponent::SetDensity(float density) const noexcept
	{
		physics_2d_collider_set_density(density, m_Collider);
	}

	float RigidBody2DComponent::GetDensity() const noexcept
	{
		return physics_2d_collider_get_density(m_Collider);
	}

	void RigidBody2DComponent::SetDynamic(bool dynamic) const noexcept
	{
		physics_2d_body_set_dynamic(dynamic, m_Body);
	}
	bool RigidBody2DComponent::IsDynamic() const noexcept
	{
		return physics_2d_body_get_dynamic(m_Body);
	}

	void RigidBody2DComponent::SetFixedRotation(bool fixedRotation) const noexcept
	{
		physics_2d_body_set_fixed_rotation(fixedRotation, m_Body);
	}
	bool RigidBody2DComponent::IsFixedRotation() const noexcept
	{
		return physics_2d_body_get_fixed_rotation(m_Body);
	}

	SV_PHY2D_COLLIDER_TYPE RigidBody2DComponent::GetColliderType() const noexcept
	{
		return physics_2d_collider_get_type(m_Collider);
	}

	void RigidBody2DComponent::SetBoxCollider(float width, float height)
	{
		SV_PHY2D_COLLIDER_DESC desc;
		desc.density = 1.f;
		desc.colliderType = SV_PHY2D_COLLIDER_TYPE_BOX;
		desc.body = m_Body;
		desc.box.width = width;
		desc.box.height = height;

		physics_2d_collider_destroy(m_Collider);
		m_Collider = physics_2d_collider_create(&desc);
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
		SV_PHY2D_COLLIDER_DESC desc;
		desc.density = GetDensity();
		desc.colliderType = SV_PHY2D_COLLIDER_TYPE_CIRCLE;
		desc.body = m_Body;
		desc.circle.radius = radius;

		physics_2d_collider_destroy(m_Collider);
		m_Collider = physics_2d_collider_create(&desc);
	}
	float RigidBody2DComponent::CircleCollider_GetRadius()
	{
		return 0.f;
	}
	void RigidBody2DComponent::CircleCollider_SetRadius(float radius)
	{

	}

}