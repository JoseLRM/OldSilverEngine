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

	RigidBody2DComponent::RigidBody2DComponent() : pInternal(nullptr)
	{}

	RigidBody2DComponent::RigidBody2DComponent(const RigidBody2DComponent& other)
	{
		dynamic = other.dynamic;
		fixedRotation = other.fixedRotation;
		velocity = other.velocity;
		angularVelocity = other.angularVelocity;
		pInternal = nullptr;
	}

	RigidBody2DComponent::RigidBody2DComponent(RigidBody2DComponent&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		dynamic = other.dynamic;
		fixedRotation = other.fixedRotation;
		velocity = other.velocity;
		angularVelocity = other.angularVelocity;
	}

	RigidBody2DComponent::~RigidBody2DComponent()
	{
		if (pInternal) {

			b2Body* body = reinterpret_cast<b2Body*>(pInternal);
			b2World& world = *body->GetWorld();
			world.DestroyBody(body);
			pInternal = nullptr;

		}
	}

	// QUAD COMPONENT

	Box2DComponent::Box2DComponent() : pInternal(nullptr)
	{}

	Box2DComponent::~Box2DComponent()
	{
		if (pInternal) {

			b2Fixture* fixture = reinterpret_cast<b2Fixture*>(pInternal);
			b2Body* body = fixture->GetBody();
			body->DestroyFixture(fixture);
			pInternal = nullptr;

		}
	}

	Box2DComponent::Box2DComponent(const Box2DComponent& other)
	{
		density = other.density;
		friction = other.friction;
		angularOffset = other.angularOffset;
		offset = other.offset;
		restitution = other.restitution;
		size = other.size;
		pInternal = nullptr;
	}

	Box2DComponent::Box2DComponent(Box2DComponent&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		density = other.density;
		friction = other.friction;
		angularOffset = other.angularOffset;
		offset = other.offset;
		restitution = other.restitution;
		size = other.size;
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

	void scene_component_serialize_RigidBody2DComponent(BaseComponent* comp, ArchiveO& archive)
	{
		
	}

	void scene_component_serialize_QuadComponent(BaseComponent* comp, ArchiveO& archive)
	{
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
				log_error("Texture not found, hashcode: %u", hash);
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

	void scene_component_deserialize_RigidBody2DComponent(BaseComponent* comp, ArchiveI& archive)
	{
	}

	void scene_component_deserialize_QuadComponent(BaseComponent* comp, ArchiveI& archive)
	{
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