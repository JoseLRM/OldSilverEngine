#pragma once

#include "renderer.h"
#include "renderer2D/renderer2D.h"
#include "postprocessing/postprocessing.h"
#include "mesh_renderer/mesh_renderer.h"

// Draw data

namespace sv {

	bool renderer_initialize(const InitializationRendererDesc& desc);
	bool renderer_close();

	void renderer_frame_begin();
	void renderer_frame_end();

	struct DrawData {
		Offscreen*			pOffscreen;
		CameraProjection	projection;
		CameraSettings		settings;
		XMMATRIX			viewMatrix;
		XMMATRIX			projectionMatrix;
		XMMATRIX			viewProjectionMatrix;
	};

	DrawData& renderer_drawData_get();

	// Scene struct
	struct RenderWorld {
		std::vector<RenderLayer> renderLayers;
	};

}