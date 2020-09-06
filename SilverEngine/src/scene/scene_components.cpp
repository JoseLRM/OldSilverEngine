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
		// TODO: Texure Asset
	}

	void scene_component_serialize_CameraComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		archive << comp->settings;
		archive << comp->HasOffscreen();
		if (comp->HasOffscreen()) {
			archive << comp->GetOffscreen()->GetWidth();
			archive << comp->GetOffscreen()->GetHeight();
		}
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
		// TODO: Texure Asset
	}

	void scene_component_deserialize_CameraComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		archive >> comp->settings;

		bool hasOffscreen;
		archive >> hasOffscreen;

		if (hasOffscreen) {
			ui32 width;
			ui32 height;
			archive >> width;
			archive >> height;

			SV_ASSERT(comp->CreateOffscreen(width, height));
		}
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