#pragma once

#include "simulation/entity_system.h"
#include "asset_system.h"
#include "utils/Version.h"
#include "utils/io.h"
#include "platform/graphics.h"

namespace sv {

	constexpr Version SCENE_MINIMUM_SUPPORTED_VERSION = { 0u, 0u, 0u };

	enum ForceType : ui32 {
		ForceType_Force,
		ForceType_Impulse,
	};

	enum Collider2DType : ui32 {
		Collider2DType_Null,
		Collider2DType_Box,
		Collider2DType_Circle,
	};

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

	// Rendering

	enum ProjectionType : ui32 {
		ProjectionType_Clip,
		ProjectionType_Orthographic,
		ProjectionType_Perspective
	};

	// This class only contains information about projection and some images used during rendering (offscreen)
	class Camera {
	public:
		Camera();
		~Camera();
		Camera(const Camera& other);
		Camera(Camera&& other) noexcept;

		void clear();

		Result serialize(ArchiveO& archive);
		Result deserialize(ArchiveI& archive);

		inline void activate() noexcept { m_Active = true; }
		inline void deactivate() noexcept { m_Active = false; }
		bool isActive() const noexcept; //TODO: while implementing check if the resolution is defined

		// Projection

		inline float			getWidth() const noexcept { return m_Projection.width; }
		inline float			getHeight() const noexcept { return m_Projection.height; }
		inline float			getNear() const noexcept { return m_Projection.near; }
		inline float			getFar() const noexcept { return m_Projection.far; }
		inline ProjectionType	getProjectionType() const noexcept { return m_Projection.type; }
		
		void setWidth(float width) noexcept;
		void setHeight(float height) noexcept;
		void setNear(float near) noexcept;
		void setFar(float far) noexcept;
		void setProjectionType(ProjectionType type) noexcept;

		Viewport	getViewport() const noexcept;
		Scissor		getScissor() const noexcept;

		void adjust(ui32 width, ui32 height) noexcept;
		void adjust(float aspect) noexcept;

		float	getProjectionLength() const noexcept;
		void	setProjectionLength(float length) noexcept;

		

		const XMMATRIX& getProjectionMatrix() noexcept;

		// Resolution

		Result	setResolution(ui32 width, ui32 height);
		ui32	getResolutionWidth() const noexcept;
		ui32	getResolutionHeight() const noexcept;
		vec2u	getResolution() const noexcept;

		// Offscreen getters
		
		inline GPUImage* getOffscreenRT() const noexcept { return m_OffscreenRT; }
		inline GPUImage* getOffscreenDS() const noexcept { return m_OffscreenDS; }

	private:
		GPUImage*	m_OffscreenRT = nullptr;
		GPUImage*	m_OffscreenDS = nullptr;

		struct {
			ProjectionType type;
			float width = 1.f;
			float height = 1.f;
			float near = -1000.f;
			float far = 1000.f;
			XMMATRIX matrix = XMMatrixIdentity();
			bool modified = true;
		} m_Projection;

		bool m_Active = true;

	};

	struct Sprite {
		TextureAsset	texture;
		vec4f			texCoord = { 0.f, 0.f, 1.f, 1.f };
	};

	class Scene {

	public:
		Scene();
		~Scene();
		Scene(const Scene& scene) = delete;

		void create();
		void destroy();
		void clear();

		Result	serialize(const char* filePath);
		Result	serialize(ArchiveO& archive);
		Result	deserialize(const char* filePath);
		Result	deserialize(ArchiveI& archive);

		// entity system
	public:
		inline ECS* getECS() { return m_ECS; }
		inline operator ECS* () const noexcept { return m_ECS; }

		void	setMainCamera(Entity camera);
		Entity	getMainCamera();

		// physics
	public:
		void physicsSimulate(float dt);

		inline void		setTimestep(float timestep) noexcept { m_TimeStep = timestep; }
		inline float	getTimestep() const noexcept { return m_TimeStep; }
		
		void	setGravity2D(const vec2f& gravity) noexcept;
		vec2f	getGravity2D() const noexcept;

	private:
		void createPhysics();
		void destroyPhysics();

		// rendering
	public:

		// Is the most high level draw call. Takes the ECS data and the main camera and render everything. (OPTIONAL) Presents to screen
		void draw(bool present = true);

		// Is the second high level draw call, save the result in the camera
		void drawCamera(Camera* pCamera, const vec3f& position, const vec3f& rotation);

	private:
		void createRendering();
		void destroyRendering();

		// attributes
	private:
		float m_TimeStep = 1.f;
		Entity m_MainCamera = SV_ENTITY_NULL;
		ECS* m_ECS = nullptr;
		void* m_pPhysics = nullptr;
		void* m_pRendering = nullptr;

	};

	// COMPONENTS

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
		Camera camera;

		CameraComponent() = default;
		CameraComponent(ui32 width, ui32 heigth);

	};

	// Rigid body 2d component

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
		RigidBody2DComponent(const RigidBody2DComponent& other);
		RigidBody2DComponent(RigidBody2DComponent&& other) noexcept;
	};

}