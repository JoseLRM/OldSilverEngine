#pragma once

#include "core/rendering/material_system.h"

namespace sv {

	struct PostProcessing {

		// Prefered Img Layout: GPUImageLayout_ShaderResource
		static void bloom(GPUImage* img, GPUImageLayout imgLayout, GPUImageLayout newLayout, f32 threshold, f32 range, u32 iterations, CommandList cmd);

		// Prefered Img Layout: GPUImageLayout_ShaderResource
		static void toneMapping(GPUImage* img, GPUImageLayout imgLayout, GPUImageLayout newLayout, f32 exposure, CommandList cmd);

	};

}