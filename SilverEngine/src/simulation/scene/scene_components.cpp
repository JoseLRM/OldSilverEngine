#include "core.h"

#include "scene_internal.h"

#include "physics_impl.h"

namespace sv {

	// CAMERA COMPONENT

	CameraComponent::CameraComponent(ui32 width, ui32 height)
	{
		camera.setResolution(width, height);
	}

	Collider2D::Collider2D() : type(Collider2DType_Box), box({{1.f, 1.f}, 0.f})
	{}

	RigidBody2DComponent::RigidBody2DComponent(const RigidBody2DComponent& other)
	{
		dynamic = other.dynamic;
		fixedRotation = other.fixedRotation;
		velocity = other.velocity;
		angularVelocity = other.angularVelocity;
		pInternal = nullptr;
		memcpy(colliders, other.colliders, other.collidersCount * sizeof(Collider2D));
		collidersCount = other.collidersCount;
		for (ui32 i = 0; i < collidersCount; ++i) colliders[i].pInternal = nullptr;
	}

	RigidBody2DComponent::RigidBody2DComponent(RigidBody2DComponent&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		dynamic = other.dynamic;
		fixedRotation = other.fixedRotation;
		velocity = other.velocity;
		angularVelocity = other.angularVelocity;
		memcpy(colliders, other.colliders, other.collidersCount * sizeof(Collider2D));
		collidersCount = other.collidersCount;
	}

	RigidBody2DComponent::~RigidBody2DComponent()
	{
		if (pInternal) {

			b2Body* body = reinterpret_cast<b2Body*>(pInternal);
			b2World& world = *body->GetWorld();
			world.DestroyBody(body);
			pInternal = nullptr;
			collidersCount = 0u;

		}
	}

	// SERIALIZATION

	void scene_component_serialize_NameComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		NameComponent* comp = reinterpret_cast<NameComponent*>(comp_);
		archive << comp->name;
	}

	void scene_component_serialize_SpriteComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		SpriteComponent* comp = reinterpret_cast<SpriteComponent*>(comp_);
		archive << comp->color;
		archive << comp->sprite.texCoord;
		archive << comp->sprite.texture.getHashCode();
		archive << comp->material.getHashCode();
	}

	void scene_component_serialize_AnimatedSpriteComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		AnimatedSpriteComponent* comp = reinterpret_cast<AnimatedSpriteComponent*>(comp_);
		archive << comp->color;
		archive << comp->sprite.getState();
		archive << comp->material.getHashCode();
	}

	void scene_component_serialize_CameraComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		comp->camera.serialize(archive);
	}

	void scene_component_serialize_RigidBody2DComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		RigidBody2DComponent* comp = reinterpret_cast<RigidBody2DComponent*>(comp_);

		archive << comp->dynamic << comp->fixedRotation << comp->velocity << comp->angularVelocity;

		archive << comp->collidersCount;
		for (ui32 i = 0; i < comp->collidersCount; ++i) {
			Collider2D& collider = comp->colliders[i];
			
			archive << collider.offset << collider.density << collider.friction << collider.restitution << collider.type;
			
			switch (collider.type)
			{
			case Collider2DType_Box:
				archive << collider.box.angularOffset << collider.box.size;
				break;

			case Collider2DType_Circle:
				archive << collider.circle.radius;
				break;
			}

		}
	}

	// DESERIALIZATION

	void scene_component_deserialize_NameComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		NameComponent* comp = reinterpret_cast<NameComponent*>(comp_);
		archive >> comp->name;
	}

	void scene_component_deserialize_SpriteComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		SpriteComponent* comp = reinterpret_cast<SpriteComponent*>(comp_);
		archive >> comp->color;
		archive >> comp->sprite.texCoord;
		size_t hash;
		archive >> hash;

		if (hash != 0u) {
			if (comp->sprite.texture.load(hash) != Result_Success) {
				SV_LOG_ERROR("Texture not found, hashcode: %u", hash);
			}
		}

		archive >> hash;
		if (hash != 0u) {
			if (comp->material.load(hash) != Result_Success) {
				SV_LOG_ERROR("Material not found, hashcode: %u", hash);
			}
		}
	}

	void scene_component_deserialize_AnimatedSpriteComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		AnimatedSpriteComponent* comp = reinterpret_cast<AnimatedSpriteComponent*>(comp_);
		archive >> comp->color;

		AnimatedSprite::State sprState;
		archive >> sprState;
		if (result_fail(comp->sprite.setState(sprState))) {
			SV_LOG_ERROR("Sprite Animation not found");
		}

		size_t hash;
		archive >> hash;
		if (hash != 0u) {
			if (comp->material.load(hash) != Result_Success) {
				SV_LOG_ERROR("Material not found, hashcode: %u", hash);
			}
		}
	}

	void scene_component_deserialize_CameraComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		comp->camera.deserialize(archive);
	}

	void scene_component_deserialize_RigidBody2DComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		RigidBody2DComponent* comp = reinterpret_cast<RigidBody2DComponent*>(comp_);

		archive >> comp->dynamic >> comp->fixedRotation >> comp->velocity >> comp->angularVelocity;

		archive >> comp->collidersCount;
		for (ui32 i = 0; i < comp->collidersCount; ++i) {
			Collider2D& collider = comp->colliders[i];
			archive >> collider.offset >> collider.density >> collider.friction >> collider.restitution >> collider.type;

			switch (collider.type)
			{
			case Collider2DType_Box:
				archive >> collider.box.angularOffset >> collider.box.size;
				break;

			case Collider2DType_Circle:
				archive >> collider.circle.radius;
				break;
			}
		}
	}

}