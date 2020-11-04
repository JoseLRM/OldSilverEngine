#include "core.h"

#include "scene_physics_internal.h"

namespace sv {

	void RigidBody2DComponent::setDynamic(bool dynamic) noexcept
	{
		reinterpret_cast<b2Body*>(pInternal)->SetType(dynamic ? b2_dynamicBody : b2_staticBody);
	}

	void RigidBody2DComponent::setFixedRotation(bool fixedRotation) noexcept
	{
		reinterpret_cast<b2Body*>(pInternal)->SetFixedRotation(fixedRotation);
	}

	void RigidBody2DComponent::setVelocity(const vec2f& velocity) noexcept
	{
		reinterpret_cast<b2Body*>(pInternal)->SetLinearVelocity({ velocity.x, velocity.y });
	}

	void RigidBody2DComponent::setAngularVelocity(float velocity) noexcept
	{
		reinterpret_cast<b2Body*>(pInternal)->SetAngularVelocity(velocity);
	}

	bool RigidBody2DComponent::isDynamic() const noexcept
	{
		return reinterpret_cast<b2Body*>(pInternal)->GetType() == b2_dynamicBody;
	}

	bool RigidBody2DComponent::isFixedRotation() const noexcept
	{
		return reinterpret_cast<b2Body*>(pInternal)->IsFixedRotation();
	}

	const vec2f& RigidBody2DComponent::getVelocity() const noexcept
	{
		return *reinterpret_cast<const vec2f*>(&reinterpret_cast<b2Body*>(pInternal)->GetLinearVelocity());
	}

	float RigidBody2DComponent::getAngularVelocity() const noexcept
	{
		return reinterpret_cast<b2Body*>(pInternal)->GetAngularVelocity();
	}

	BoxCollider2DComponent::BoxCollider2DComponent(const BoxCollider2DComponent& other)
	{
		update = true;
		density = other.density;
		friction = other.friction;
		restitution = other.restitution;
		size = other.size;
		offset = other.offset;
	}

	void BoxCollider2DComponent::setFriction(float friction) noexcept
	{
		this->friction = friction;
		update = true;
	}
	void BoxCollider2DComponent::setRestitution(float restitution) noexcept
	{
		this->restitution = restitution;
		update = true;
	}
	void BoxCollider2DComponent::setDensity(float density) noexcept
	{
		this->density = density;
		update = true;
	}
	void BoxCollider2DComponent::setSize(const vec2f& size) noexcept
	{
		this->size = size;
		update = true;
	}
	void BoxCollider2DComponent::setOffset(const vec2f& offset) noexcept
	{
		this->offset = offset;
		update = true;
	}

	CircleCollider2DComponent::CircleCollider2DComponent(const CircleCollider2DComponent& other)
	{
		update = true;
		density = other.density;
		friction = other.friction;
		restitution = other.restitution;
		radius = other.radius;
		offset = other.offset;
	}
	void CircleCollider2DComponent::setFriction(float friction) noexcept
	{
		update = true;
		this->friction = friction;
	}
	void CircleCollider2DComponent::setRestitution(float restitution) noexcept
	{
		update = true;
		this->restitution = restitution;
	}
	void CircleCollider2DComponent::setDensity(float density) noexcept
	{
		update = true;
		this->density = density;
	}
	void CircleCollider2DComponent::setRadius(float radius) noexcept
	{
		update = true;
		this->radius = radius;
	}
	void CircleCollider2DComponent::setOffset(const vec2f& offset) noexcept
	{
		update = true;
		this->offset = offset;
	}

	b2BodyDef scene_physics2D_body_def(b2Body* body)
	{
		b2BodyDef def;
		def.userData = body->GetUserData();
		def.position = body->GetPosition();
		def.angle = body->GetAngle();
		def.linearVelocity = body->GetLinearVelocity();
		def.angularVelocity = body->GetAngularVelocity();
		def.linearDamping = body->GetLinearDamping();
		def.angularDamping = body->GetAngularDamping();
		def.allowSleep = body->IsSleepingAllowed();
		def.awake = body->IsAwake();
		def.fixedRotation = body->IsFixedRotation();
		def.bullet = body->IsBullet();
		def.type = body->GetType();
		def.enabled = body->IsEnabled();
		def.gravityScale = body->GetGravityScale();

		return def;
	}

}