#pragma once

#include "entity_system.h"
#include "renderer.h"

namespace sv {

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

	enum AssetType : ui32 {
		AssetType_Unknown,
		AssetType_Texture,
	};

	struct SceneDesc {
		vec3		gravity;
	};

	struct Scene {
		ECS		ecs;
		Entity	mainCamera;
		float	timeStep;
		void*	pWorld2D;

		struct {
			ui32 refreshCount;
			std::vector<std::pair<std::string, SharedRef<Texture>>> textures;
		} assets;
	};

	struct CameraDrawDesc {
		Offscreen*				pOffscreen;
		const CameraSettings*	pSettings;
		vec3					position;
		vec3					rotation;
	};

	Result	scene_create(const SceneDesc* desc, Scene& scene);
	Result	scene_destroy(Scene& scene);
	void	scene_clear(Scene& scene);

	// Asset Management

	Result	scene_assets_refresh();
	const std::map<std::string, SharedRef<Texture>>& scene_assets_texturesmap_get();

	Result	scene_assets_update(Scene& scene, float dt);

	Result scene_assets_load_texture(Scene& scene, const char* filePath, SharedRef<Texture>& ref);

	// Rendering

	void scene_renderer_draw(Scene& scene);
	void scene_renderer_camera_draw(const CameraDrawDesc* desc, Scene& scene);

	// Physics

	void scene_physics_simulate(Scene& scene, float dt);

}