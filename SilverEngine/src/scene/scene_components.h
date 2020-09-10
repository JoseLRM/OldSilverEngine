#pragma once

namespace sv {

	// Name component
#if SV_SCENE_NAME_COMPONENT

	struct NameComponent : public Component<NameComponent> {
	
		std::string name;

		NameComponent() = default;
		NameComponent(const char* name) : name(name) {}
		NameComponent(const std::string& name) : name(name) {}
		NameComponent(std::string&& name) : name(std::move(name)) {}

	};

#endif

	// Sprite Component

	struct SpriteComponent : public Component<SpriteComponent> {

		Sprite sprite;
		Color color = SV_COLOR_WHITE;

		SpriteComponent() {}
		SpriteComponent(Color col) : color(col) {}
		SpriteComponent(Sprite spr) : sprite(spr) {}
		SpriteComponent(Sprite spr, Color col) : sprite(spr), color(col) {}

	};

	// Camera Component

	struct CameraComponent : public Component<CameraComponent> {
		Offscreen		offscreen;
		CameraSettings	settings;
	
		CameraComponent();
		CameraComponent(ui32 width, ui32 heigth);
		~CameraComponent();
	
		CameraComponent(const CameraComponent & other);
		CameraComponent(CameraComponent && other) noexcept;
	
		void Adjust(float width, float height);

	};

	// Rigid body 2d component

	struct Box2DCollider {
		void* pInternal;
		vec2 size = { 1.f, 1.f };
		vec2 offset;
		float angularOffset = 0.f;
		float density = 10.f;
		float friction = 0.3f;
		float restitution = 0.3f;
	};

	struct RigidBody2DComponent : public Component<RigidBody2DComponent> {

		void* pInternal = nullptr;
		bool dynamic = true;
		bool fixedRotation = false;
		vec2 velocity;
		float angularVelocity = 0.f;

		Box2DCollider boxColliders[8];
		ui32 boxCollidersCount = 1u;

		RigidBody2DComponent() = default;
		~RigidBody2DComponent();
		RigidBody2DComponent(const RigidBody2DComponent & other);
		RigidBody2DComponent(RigidBody2DComponent && other) noexcept;
	};

	// Mesh component

	struct MeshComponent : public Component<MeshComponent> {

		SharedRef<Mesh>		mesh;
		SharedRef<Material> material;

	};

	// Light component

	struct LightComponent : public Component<LightComponent> {

		LightType	lightType	= LightType_Point;
		float		intensity	= 1.f;
		float		range		= 5.f;
		float		smoothness	= 0.8f;
		Color3f		color		= SV_COLOR3F_WHITE;

	};

}