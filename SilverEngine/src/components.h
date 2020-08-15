#pragma once

#include "scene.h"
#include "renderer.h"
#include "physics.h"

namespace sv {

	// NAME COMPONENT

#if SV_ECS_NAME_COMPONENT

	SV_COMPONENT(NameComponent) {
	private:
		std::string m_Name;
	
	public:
		NameComponent() : m_Name("Unnamed") {}
		NameComponent(const char* name) : m_Name(name) {}
	
		inline void SetName(const char* name) noexcept { m_Name = name; }
		inline void SetName(const std::string & name) noexcept { m_Name = name; }
	
		inline const std::string& GetName() const noexcept { return m_Name; }

	};

#endif

	// SPRITE COMPONENT

	SV_COMPONENT(SpriteComponent) {

		ui32 renderLayer;
		Sprite sprite;
		Color color = SV_COLOR_WHITE;

		SpriteComponent() : renderLayer(0u) {}
		SpriteComponent(Color col) : color(col), renderLayer(0u) {}
		SpriteComponent(Sprite spr) : sprite(spr), renderLayer(0u) {}
		SpriteComponent(Sprite spr, Color col) : sprite(spr), color(col), renderLayer(0u) {}

	};

	// CAMERA COMPONENT

	SV_COMPONENT(CameraComponent) {
	private:
		std::unique_ptr<Offscreen> m_Offscreen;
	
	public:
		CameraProjection	projection;
		CameraSettings		settings;
	
		CameraComponent();
		CameraComponent(CameraType projectionType);
	
		CameraComponent(const CameraComponent & other);
		CameraComponent(CameraComponent && other) noexcept;
		CameraComponent& operator=(const CameraComponent & other);
		CameraComponent& operator=(CameraComponent && other) noexcept;
	
		bool CreateOffscreen(ui32 width, ui32 height);
		bool HasOffscreen() const noexcept;
		Offscreen* GetOffscreen() const noexcept;
		bool DestroyOffscreen();
	
		void Adjust(float width, float height);
	
	};

	// RIGID BODY 2D COMPONENT

	SV_COMPONENT(RigidBody2DComponent) {
	private:
		Body2D m_Body = nullptr;
		Collider2D m_Collider = nullptr;
	
	public:
		vec2 offset;
	
		RigidBody2DComponent();
		RigidBody2DComponent(Collider2DType colliderType);
	
		RigidBody2DComponent& operator=(const RigidBody2DComponent & other);
		RigidBody2DComponent& operator=(RigidBody2DComponent && other) noexcept;
	
		~RigidBody2DComponent();
	
		void SetDensity(float density) const noexcept;
		float GetDensity() const noexcept;
		void SetDynamic(bool dynamic) const noexcept;
		bool IsDynamic() const noexcept;
		void SetFixedRotation(bool fixedRotation) const noexcept;
		bool IsFixedRotation() const noexcept;
	
		inline Body2D GetBody() const noexcept { return m_Body; }
		inline Collider2D GetCollider() const noexcept { return m_Collider; }
		Collider2DType GetColliderType() const noexcept;
	
		void SetBoxCollider(float width, float height);
		float BoxCollider_GetWidth();
		float BoxCollider_GetHeight();
		void BoxCollider_SetSize(float width, float height);
	
		void SetCircleCollider(float radius);
		float CircleCollider_GetRadius();
		void CircleCollider_SetRadius(float radius);

	};

	SV_COMPONENT(MeshComponent) {

		Mesh mesh;
		Material material;

	};

}