#pragma once

#include "renderer.h"

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
		vec3 position;
		vec3 normal;
		vec2 texCoord;
	};

	// Shader utils

	Result renderer_shader_create(const char* name, ShaderType type, Shader& shader, std::pair<const char*, const char*>* macros = nullptr, ui32 macrosCount = 0u);

}