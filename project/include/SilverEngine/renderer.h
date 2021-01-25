#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	struct Mesh;

	void draw_mesh(const Mesh* mesh, const XMMATRIX& transform_matrix, GPUImage* zbuffer, GPUImage* offscreen, CommandList cmd);

}