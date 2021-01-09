#pragma once

#include "scene/scene.h"
#include "scene/scene_renderer.h"
#include "sprite/sprite_renderer.h"
#include "mesh/mesh_renderer.h"
#include "core/utils/allocator.h"

namespace sv {

	Result scene_initialize();
	Result scene_close();

}



namespace sv {

	struct SceneRenderer_internal {

		static Result initialize();
		static Result close();

	};

	struct SceneRendererContext {

		FrameList<SpriteInstance>		spritesInstances;

		FrameList<MeshInstance>			meshInstances;
		FrameList<MeshInstanceGroup>	meshGroups;

	};

}