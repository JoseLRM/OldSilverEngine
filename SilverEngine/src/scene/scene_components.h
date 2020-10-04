#pragma once

namespace sv {

	// Name component
	struct NameComponent : public Component<NameComponent> {
	
		std::string name;

		NameComponent() = default;
		NameComponent(const char* name) : name(name) {}
		NameComponent(const std::string& name) : name(name) {}
		NameComponent(std::string&& name) : name(std::move(name)) {}

	};

	// Sprite Component

	struct SpriteComponent : public Component<SpriteComponent> {

		MaterialAsset material;
		Sprite sprite;
		Color color = Color::White();

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

	struct Collider2D {
		
		Collider2DType type = Collider2DType_Null;
		void* pInternal = nullptr;
		vec2f offset;
		float density = 10.f;
		float friction = 0.3f;
		float restitution = 0.3f;

		union {
			// BOX 2D COLLIDER
			struct {

				vec2f size;
				float angularOffset;
				
			} box;

			// CIRCLE 2D COLLIDER
			struct {

				float radius;

			} circle;

		};

		Collider2D();

	};

	struct RigidBody2DComponent : public Component<RigidBody2DComponent> {

		void* pInternal = nullptr;
		bool dynamic = true;
		bool fixedRotation = false;
		vec2f velocity;
		float angularVelocity = 0.f;

		Collider2D colliders[8];
		ui32 collidersCount = 1u;

		RigidBody2DComponent() = default;
		~RigidBody2DComponent();
		RigidBody2DComponent(const RigidBody2DComponent & other);
		RigidBody2DComponent(RigidBody2DComponent && other) noexcept;
	};

	// Mesh component

	struct MeshComponent : public Component<MeshComponent> {

		//SharedRef<Mesh>		mesh;
		Material* material;

	};

	// Light component

	struct LightComponent : public Component<LightComponent> {

		LightType	lightType	= LightType_Point;
		float		intensity	= 1.f;
		float		range		= 5.f;
		float		smoothness	= 0.8f;
		Color3f		color		= Color3f::White();

	};

}