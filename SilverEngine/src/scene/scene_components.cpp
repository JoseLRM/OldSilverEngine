#include "core.h"

#include "scene_internal.h"

#include "physics_impl.h"

namespace sv {

	// CAMERA COMPONENT

	CameraComponent::CameraComponent()
	{
		settings.active = true;
		SV_ASSERT(renderer_offscreen_create(1920u, 1080u, offscreen) == Result_Success);
	}

	CameraComponent::CameraComponent(ui32 width, ui32 height)
	{
		settings.active = true;
		SV_ASSERT(renderer_offscreen_create(width, height, offscreen) == Result_Success);
	}

	CameraComponent::~CameraComponent()
	{
		SV_ASSERT(renderer_offscreen_destroy(offscreen) == Result_Success);
	}

	CameraComponent::CameraComponent(const CameraComponent& other)
	{
		settings = other.settings;
		SV_ASSERT(renderer_offscreen_create(other.offscreen.GetWidth(), other.offscreen.GetHeight(), offscreen) == Result_Success);
	}
	CameraComponent::CameraComponent(CameraComponent&& other) noexcept
	{
		settings = other.settings;
		offscreen = std::move(other.offscreen);
	}

	void CameraComponent::Adjust(float width, float height)
	{
		renderer_projection_aspect_set(settings.projection, width / height);
	}

	// RIGID BODY 2D

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
		if (comp->sprite.texture.Get()) {
			archive << comp->sprite.texture->hashCode;
		}
		else archive << (size_t)0u;
	}

	void scene_component_serialize_CameraComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		archive << comp->settings;
		archive << comp->offscreen.GetWidth();
		archive << comp->offscreen.GetHeight();
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

	void scene_component_serialize_MeshComponent(BaseComponent* comp, ArchiveO& archive)
	{
	}

	void scene_component_serialize_LightComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		LightComponent* comp = reinterpret_cast<LightComponent*>(comp_);
		archive << comp->lightType << comp->intensity << comp->range << comp->smoothness << comp->color;
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
			if (assets_load_texture(hash, comp->sprite.texture) != Result_Success) {
				sv::console_log("Texture not found, hashcode: %u", hash);
			}
		}
	}

	void scene_component_deserialize_CameraComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		archive >> comp->settings;

		ui32 width;
		ui32 height;
		archive >> width;
		archive >> height;

		SV_ASSERT(renderer_offscreen_create(width, height, comp->offscreen) == Result_Success);
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

	void scene_component_deserialize_MeshComponent(BaseComponent* comp, ArchiveI& archive)
	{
	}

	void scene_component_deserialize_LightComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		LightComponent* comp = reinterpret_cast<LightComponent*>(comp_);
		archive >> comp->lightType >> comp->intensity >> comp->range >> comp->smoothness >> comp->color;
	}

}