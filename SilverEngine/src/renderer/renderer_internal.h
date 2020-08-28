#pragma once

#include "renderer.h"

namespace sv {

	bool renderer_initialize(const InitializationRendererDesc& desc);
	bool renderer_close();

	bool renderer_mesh_initialize(const InitializationRendererDesc& desc);
	bool renderer_sprite_initialize(const InitializationRendererDesc& desc);
	bool renderer_postprocessing_initialize(const InitializationRendererDesc& desc);

	bool renderer_mesh_close();
	bool renderer_sprite_close();
	bool renderer_postprocessing_close();

	void renderer_frame_begin();
	void renderer_frame_end();

	struct MeshVertex {
		vec3 position;
		vec3 normal;
		vec2 texCoord;
	};

	// Shader utils

	bool renderer_shader_create(const char* name, ShaderType type, Shader& shader);

}