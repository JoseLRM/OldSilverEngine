#pragma once

#include "rendering/mesh_renderer.h"
#include "utils/allocator.h"

namespace sv {

	constexpr u32 MESH_BATCH_COUNT = 1000u;
	constexpr u32 LIGHT_COUNT = 20u;

	struct MeshData {
		XMMATRIX tm;
	};

	struct MeshLightData {
		LightType type;
		v3_f32 positionOrDirection;
		float intensity;
		float radius;
		float smoothness;
		float padding;
	};

	struct LightingData {
		u32 resolutionWidth;
		u32 resolutionHeight;
		u32 lightCount;
		float padding;

		MeshLightData lights[LIGHT_COUNT];
	};

	struct MeshRendererContext {

		MeshData*		meshData;

	};

	struct MeshRenderer_internal {

		static Result initialize();
		static Result close();

	};

}