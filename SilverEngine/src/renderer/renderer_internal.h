#pragma once

#include "renderer.h"
#include "renderer2D/renderer2D_internal.h"
#include "postprocessing/postprocessing.h"

// Draw data

namespace sv {

	bool renderer_initialize(const InitializationRendererDesc& desc);
	bool renderer_close();

	void renderer_frame_begin();
	void renderer_frame_end();

	struct Camera_DrawData {
		CameraProjection	projection;
		Offscreen*			pOffscreen;
		XMMATRIX			viewMatrix;
	};

	struct DrawData {

		std::vector<Camera_DrawData>				cameras;
		Camera_DrawData								currentCamera;
		XMMATRIX									viewMatrix;
		XMMATRIX									projectionMatrix;
		XMMATRIX									viewProjectionMatrix;
		sv::RenderList<SpriteInstance>				sprites;
		//sv::RenderList<MeshInstance>				meshes;

	};

	DrawData& renderer_drawData_get();



	// Scene struct
	struct RenderWorld {
		std::vector<RenderLayer> renderLayers;
	};

}