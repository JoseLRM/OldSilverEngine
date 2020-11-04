#pragma once

#include "simulation/entity_system.h"

namespace sv {

	enum ForceType : ui32 {
		ForceType_Force,
		ForceType_Impulse,
	};

	enum Collider2DType : ui32 {
		Collider2DType_Null,
		Collider2DType_Box,
		Collider2DType_Circle,
	};

	class ScenePhysics {
	public:
		~ScenePhysics();

		void create(ECS* ecs);
		void destroy();

		void simulate(float dt, ECS* ecs);

		void	setGravity2D(const vec2f& gravity) noexcept;
		vec2f	getGravity2D() const noexcept;

	private:
		void* pInternal = nullptr;

	};

	// Rigid body 2D component

	struct RigidBody2DComponent : public Component<RigidBody2DComponent> {

		// Modifiers

		//void applyForce(ForceType type);

		// Setters
		void setDynamic(bool dynamic) noexcept;
		void setFixedRotation(bool fixedRotation) noexcept;
		void setVelocity(const vec2f& velocity) noexcept;
		void setAngularVelocity(float velocity) noexcept;

		// Getters
		bool isDynamic() const noexcept;
		bool isFixedRotation() const noexcept;
		const vec2f& getVelocity() const noexcept;
		float getAngularVelocity() const noexcept;

		friend ScenePhysics;

	private:
		void* pInternal = nullptr;
		void* boxInternal = nullptr;
		void* circleInternal = nullptr;

	};

	// Box Collider 2D Component

	struct BoxCollider2DComponent : public Component<BoxCollider2DComponent> {

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent& other);

		// Setters
		void setFriction(float friction) noexcept;
		void setRestitution(float restitution) noexcept;
		void setDensity(float density) noexcept;
		void setSize(const vec2f& size) noexcept;
		void setOffset(const vec2f& offset) noexcept;

		// Getters
		inline float getFriction() const noexcept { return friction; }
		inline float getRestitution() const noexcept { return restitution; }
		inline float getDensity() const noexcept { return density; }
		inline const vec2f& getSize() const noexcept { return size; }
		inline const vec2f& getOffset() const noexcept { return offset; }

		friend ScenePhysics;

	private:
		bool update = true;
		float friction = 0.5f;
		float restitution = 0.1f;
		float density = 5.f;
		vec2f size = { 0.5f, 0.5f };
		vec2f offset = { 0.f, 0.f };

	};

	// Circle Collider 2D Component

	struct CircleCollider2DComponent : public Component<CircleCollider2DComponent> {

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent& other);

		// Setters
		void setFriction(float friction) noexcept;
		void setRestitution(float restitution) noexcept;
		void setDensity(float density) noexcept;
		void setRadius(float radius) noexcept;
		void setOffset(const vec2f& offset) noexcept;

		// Getters
		inline float getFriction() const noexcept { return friction; }
		inline float getRestitution() const noexcept { return restitution; }
		inline float getDensity() const noexcept { return density; }
		inline float getRadius() const noexcept { return radius; }
		inline const vec2f& getOffset() const noexcept { return offset; }

		friend ScenePhysics;

	private:
		bool update = true;
		float friction = 0.5f;
		float restitution = 0.1f;
		float density = 5.f;
		float radius = 0.5f;
		vec2f offset = { 0.f, 0.f };

	};

}