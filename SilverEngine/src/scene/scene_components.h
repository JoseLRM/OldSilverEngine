#pragma once

namespace sv {

	// Name component
#if SV_SCENE_NAME_COMPONENT

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

	// Sprite Component

	SV_COMPONENT(SpriteComponent) {

		ui32 renderLayer;
		Sprite sprite;
		Color color = SV_COLOR_WHITE;

		SpriteComponent() : renderLayer(0u) {}
		SpriteComponent(Color col) : color(col), renderLayer(0u) {}
		SpriteComponent(Sprite spr) : sprite(spr), renderLayer(0u) {}
		SpriteComponent(Sprite spr, Color col) : sprite(spr), color(col), renderLayer(0u) {}

	};

	// Camera Component

	SV_COMPONENT(CameraComponent) {
	private:
		std::unique_ptr<Offscreen> m_Offscreen;
	
	public:
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

	// Rigid body 2d component

	SV_COMPONENT(RigidBody2DComponent) {
		void* pInternal;

		bool dynamic = true;
		bool fixedRotation = false;
		vec2 velocity;
		float angularVelocity = 0.f;

		RigidBody2DComponent();
		~RigidBody2DComponent();
		RigidBody2DComponent& operator=(const RigidBody2DComponent & other);
		RigidBody2DComponent& operator=(RigidBody2DComponent && other) noexcept;
	};

	// Quad Collider

	SV_COMPONENT(QuadComponent) {
		void* pInternal;

		vec2 size = { 1.f, 1.f };
		vec2 offset;
		float angularOffset = 0.f;
		float density = 10.f;
		float friction = 0.3f;
		float restitution = 0.3f;

		QuadComponent();
		~QuadComponent();
		QuadComponent& operator=(const QuadComponent & other);
		QuadComponent& operator=(QuadComponent && other) noexcept;

	};

	// Mesh component

	SV_COMPONENT(MeshComponent) {

		SharedRef<Mesh>		mesh;
		SharedRef<Material> material;

	};

	// Light component

	SV_COMPONENT(LightComponent) {

		LightType	lightType	= LightType_Point;
		float		intensity	= 1.f;
		float		range		= 5.f;
		float		smoothness	= 0.8f;
		Color3f		color		= SV_COLOR3F_WHITE;

	};

}