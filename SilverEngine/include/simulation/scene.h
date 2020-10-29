#pragma once

#include "simulation/entity_system.h"
#include "asset_system.h"
#include "utils/Version.h"
#include "utils/io.h"
#include "platform/graphics.h"

#include "rendering/scene_renderer.h"

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
		void drawCamera(Camera* pCamera, const vec3f& position, const vec4f& directionQuat);

		// attributes
	private:
		float m_TimeStep = 1.f;
		Entity m_MainCamera = SV_ENTITY_NULL;
		ECS* m_ECS = nullptr;
		void* m_pPhysics = nullptr;

		SceneRenderer m_Renderer;

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