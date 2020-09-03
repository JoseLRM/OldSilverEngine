#pragma once

#include "scene.h"

namespace sv {

	Result scene_initialize(const InitializationSceneDesc& desc);
	Result scene_close();

	Result scene_assets_initialize(const char* assetsFolderPath);
	Result scene_assets_close();

	Result scene_assets_create(const SceneDesc* desc, Scene& scene);
	Result scene_assets_destroy(Scene& scene);

	Result scene_physics_create(const SceneDesc* desc, Scene& scene);
	Result scene_physics_destroy(Scene& scene);

}