#pragma once

#include "rendering/scene_renderer.h"
#include "rendering/sprite_renderer.h"
#include "rendering/mesh_renderer.h"
#include "utils/allocator.h"

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