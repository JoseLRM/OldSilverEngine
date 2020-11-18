#pragma once

#include "rendering/mesh_renderer.h"

namespace sv {

	constexpr Format GBUFFER_DIFFUSE_FORMAT = Format_R8G8B8A8_SRGB;
	constexpr Format GBUFFER_NORMAL_FORMAT = Format_R16G16_FLOAT;

	Result mesh_renderer_initialize();
	Result mesh_renderer_close();

}