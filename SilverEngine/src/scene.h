#pragma once

#include "entity_system.h"
#include "renderer.h"

namespace sv {

	constexpr Version SCENE_MINIMUM_SUPPORTED_VERSION = { 0u, 0u, 0u };

	enum ForceType : ui32 {
		ForceType_Force,
		ForceType_Impulse,
	};

	enum Collider2DType : ui32 {
		Collider2DType_Box,
		Collider2DType_Circle,
	};

	// Rendering

	struct CameraSettings {
		bool active = true;
		CameraProjection projection;
	};

}

#include "scene/scene_components.h"

namespace sv {

	struct InitializationSceneDesc {
		const char* assetsFolderPath;
	};

	struct SceneDesc {
		vec3		gravity;
	};

	typedef void Scene;

	struct CameraDrawDesc {
		Offscreen*				pOffscreen;
		const CameraSettings*	pSettings;
		vec3					position;
		vec3					rotation;
	};

	// Main functions

	Result	scene_create(const SceneDesc* desc, Scene** scene);
	Result	scene_destroy(Scene* scene);
	void	scene_clear(Scene* scene);

	Result	scene_serialize(Scene* scene, const char* filePath);
	Result	scene_serialize(Scene* scene, ArchiveO& archive);
	Result	scene_deserialize(Scene* scene, const char* filePath);
	Result	scene_deserialize(Scene* scene, ArchiveI& archive);

	// Getters

	ECS* scene_ecs_get(Scene* scene);
	void scene_camera_set(Scene* scene, Entity camera);
	Entity scene_camera_get(Scene* scene);

	// Asset Management

	Result	scene_assets_refresh();
	const std::map<std::string, SharedRef<Texture>>& scene_assets_texturesmap_get();

	Result	scene_assets_update(Scene* scene, float dt);

	Result scene_assets_load_texture(Scene* scene, const char* filePath, SharedRef<Texture>& ref);

	// Rendering

	void scene_renderer_draw(Scene* scene);
	void scene_renderer_camera_draw(Scene* scene, const CameraDrawDesc* desc);

	// Physics

	void scene_physics_simulate(Scene* scene, float dt);

}