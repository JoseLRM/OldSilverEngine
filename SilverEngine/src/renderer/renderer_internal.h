#pragma once

#include "renderer.h"

namespace sv {

	Result renderer_initialize(const InitializationRendererDesc& desc);
	Result renderer_close();

	Result renderer_mesh_initialize(const InitializationRendererDesc& desc);
	Result renderer_sprite_initialize(const InitializationRendererDesc& desc);
	Result renderer_postprocessing_initialize(const InitializationRendererDesc& desc);

	Result renderer_mesh_close();
	Result renderer_sprite_close();
	Result renderer_postprocessing_close();

	void renderer_frame_begin();
	void renderer_frame_end();

	struct MeshVertex {
		vec3 position;
		vec3 normal;
		vec2 texCoord;
	};

	// Shader utils

	Result renderer_shader_create(const char* name, ShaderType type, Shader& shader);

}