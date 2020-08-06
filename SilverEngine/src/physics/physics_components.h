#pragma once

#include "core.h"
#include "../scene/Scene.h"

namespace sv {

	SV_COMPONENT(RigidBody2DComponent) {
	private:
		Body2D m_Body = nullptr;
		Collider2D m_Collider = nullptr;
		
	public:
		vec2 offset;

		RigidBody2DComponent();
		RigidBody2DComponent(SV_PHY2D_COLLIDER_TYPE colliderType);

		RigidBody2DComponent& operator=(const RigidBody2DComponent& other);
		RigidBody2DComponent& operator=(RigidBody2DComponent&& other) noexcept;

		~RigidBody2DComponent();

		void SetDensity(float density) const noexcept;
		float GetDensity() const noexcept;
		void SetDynamic(bool dynamic) const noexcept;
		bool IsDynamic() const noexcept;
		void SetFixedRotation(bool fixedRotation) const noexcept;
		bool IsFixedRotation() const noexcept;

		inline Body2D GetBody() const noexcept { return m_Body; }
		inline Collider2D GetCollider() const noexcept { return m_Collider; }
		SV_PHY2D_COLLIDER_TYPE GetColliderType() const noexcept;
		
		void SetBoxCollider(float width, float height);
		float BoxCollider_GetWidth();
		float BoxCollider_GetHeight();
		void BoxCollider_SetSize(float width, float height);

		void SetCircleCollider(float radius);
		float CircleCollider_GetRadius();
		void CircleCollider_SetRadius(float radius);

	};

}