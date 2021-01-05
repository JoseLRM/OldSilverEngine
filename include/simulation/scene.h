#pragma once

#include "simulation/entity_system.h"
#include "asset_system.h"
#include "utils/Version.h"
#include "utils/io.h"
#include "platform/graphics.h"

#include "rendering/scene_renderer.h"

namespace sv {

	constexpr Version SCENE_MINIMUM_SUPPORTED_VERSION = { 0u, 0u, 0u };

	class Scene {

	public:
		Scene();
		~Scene();

		Scene(const Scene& other) = delete;

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

		// rendering
	private:
		LightSceneData	lightData;
		GBuffer			gBuffer;

	public:

		SV_INLINE GBuffer& getGBuffer() noexcept { return gBuffer; }

		/*
			Is the most high level draw call. Takes the ECS data and the main camera and render everything.
			This method doesn't present anything to the swapchain
		*/
		void draw();

		// attributes
	private:
		float m_TimeStep = 1.f;
		Entity m_MainCamera = SV_ENTITY_NULL;
		ECS* m_ECS = nullptr;

	};

	// Scene Asset
	class SceneAsset : public Asset {
	public:
		inline Scene* get() const noexcept { return reinterpret_cast<Scene*>(m_Ref.get()); }
		inline Scene* operator->() const noexcept { return get(); }

		Result createFile(const char* filePath);
		inline Result save() const noexcept { return get()->serialize((asset_folderpath_get() + getFilePath()).c_str()); }
	};

	// COMPONENTS

	// Name component

	struct NameComponent : public Component<NameComponent> {

		std::string name;

		NameComponent() = default;
		NameComponent(const char* name) : name(name) {}
		NameComponent(const std::string& name) : name(name) {}
		NameComponent(std::string&& name) : name(std::move(name)) {}

		void serialize(ArchiveO& file);
		void deserialize(ArchiveI& file);

	};

}