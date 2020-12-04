#pragma once

#include "rendering/mesh_renderer.h"
#include "utils/allocator.h"

namespace sv {

	constexpr ui32 MESH_BATCH_COUNT = 1000u;

	constexpr Format GBUFFER_DIFFUSE_FORMAT = Format_R8G8B8A8_SRGB;
	constexpr Format GBUFFER_NORMAL_FORMAT = Format_R32G32B32A32_FLOAT; //TEMP
	constexpr Format GBUFFER_DEPTHSTENCIL_FORMAT = Format_D24_UNORM_S8_UINT;

	struct MeshData {
		XMMATRIX tm;
	};

	struct LightingData {
		ui32 resolutionWidth;
		ui32 resolutionHeight;
		vec2f padding;
	};

	struct MeshRendererContext {

		MeshData*		meshData;

	};

	struct MeshRenderer_internal {

		static Result initialize();
		static Result close();

	};

}