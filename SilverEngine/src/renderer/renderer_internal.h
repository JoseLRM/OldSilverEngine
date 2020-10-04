#pragma once

#include "renderer.h"

#define svThrow(x) SV_THROW("RENDERER_ERROR", x)
#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[RENDERER] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[RENDERER_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[RENDERER_ERROR] "#x, __VA_ARGS__)

#include "postprocessing/renderer_postprocessing_internal.h"
#include "mesh/renderer_mesh_internal.h"
#include "sprite/renderer_sprite_internal.h"
#include "debug/renderer_debug_internal.h"

namespace sv {

	Result renderer_initialize(const InitializationRendererDesc& desc);
	Result renderer_close();

	void renderer_frame_begin();
	void renderer_frame_end();

	struct MeshVertex {
		vec3f position;
		vec3f normal;
		vec2f texCoord;
	};

}