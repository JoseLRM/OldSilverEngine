#pragma once

#include "simulation/entity_system.h"
#include "asset_system.h"
#include "utils/Version.h"
#include "utils/io.h"
#include "platform/graphics.h"

#include "rendering/scene_renderer.h"
#include "physics/scene_physics.h"

namespace sv {

	constexpr Version SCENE_MINIMUM_SUPPORTED_VERSION = { 0u, 0u, 0u };

	enum SceneType : ui32 {

		SceneType_Invalid,
		SceneType_2D,
		SceneType_3D

	};

	class Scene {

	public:
		Scene();
		~Scene();
		Scene(const Scene& scene) = delete;

		void create(SceneType sceneType);
		void destroy();
		void clear();

		Result	serialize(const char* filePath);
		Result	serialize(ArchiveO& archive);
		Result	deserialize(const char* filePath);
		Result	deserialize(ArchiveI& archive);

		inline SceneType getSceneType() const noexcept { return m_SceneType; }

		// entity system
	public:
		inline ECS* getECS() { return m_ECS; }
		inline operator ECS* () const noexcept { return m_ECS; }

		void	setMainCamera(Entity camera);
		Entity	getMainCamera();

		// physics
	public:
		void physicsSimulate(float dt);

		inline ScenePhysics& getPhysics() noexcept { return m_Physics; }

		// rendering
	public:

		// Is the most high level draw call. Takes the ECS data and the main camera and render everything. (OPTIONAL) Presents to screen
		void draw(bool present = true);

		// Is the second high level draw call, save the result in the camera
		void drawCamera(Camera* pCamera, const vec3f& position, const vec4f& directionQuat);

		inline SceneRenderer& getRenderer() noexcept { return m_Renderer; }

		// attributes
	private:
		SceneType m_SceneType = SceneType_Invalid;
		float m_TimeStep = 1.f;
		Entity m_MainCamera = SV_ENTITY_NULL;
		ECS* m_ECS = nullptr;

		SceneRenderer	m_Renderer;
		ScenePhysics	m_Physics;

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

}