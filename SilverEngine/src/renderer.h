#pragma once

#include "core.h"

#include "graphics.h"

#include "renderer/postprocessing/renderer_postprocessing.h"
#include "renderer/projection/renderer_projection.h"
#include "renderer/offscreen/renderer_offscreen.h"
#include "renderer/mesh/renderer_mesh.h"
#include "renderer/sprite/renderer_sprite_internal.h"
#include "renderer/debug/renderer_debug.h"
#include "renderer/objects/renderer_objects.h"

namespace sv {	

	struct InitializationRendererDesc {
	};

	void renderer_present(GPUImage* image, const GPUImageRegion& region, GPUImageLayout layout, CommandList cmd);

}