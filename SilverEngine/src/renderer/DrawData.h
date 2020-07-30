#pragma once

#include "Camera.h"
#include "graphics/graphics.h"
#include "renderer/renderer_desc.h"

namespace _sv {

	struct DrawData {

		sv::Camera* pCamera;
		XMMATRIX	viewMatrix;
		XMMATRIX	projectionMatrix;
		XMMATRIX	viewProjectionMatrix;

		sv::Image* pOutput;

		std::vector<sv::RenderLayer*> renderLayers;

	};

}