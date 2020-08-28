#pragma once

#include "scene.h"

namespace sv {

	void scene_physics_create(const SceneDesc* desc, Scene& scene);
	void scene_physics_destroy(Scene& scene);

}