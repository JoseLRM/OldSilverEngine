#pragma once

#include "core/graphics.h"

namespace sv {

	// Prefered Img Layout: GPUImageLayout_ShaderResource
	void postprocess_bloom(
		GPUImage* img, 
		GPUImageLayout imgOldLayout, 
		GPUImageLayout imgNewLayout, 
		GPUImage* emission, 
		GPUImageLayout emissionOldLayout,
		GPUImageLayout emissionNewLayout,
		f32 threshold, 
		f32 range, 
		u32 iterations, 
		CommandList cmd
	);

		// Prefered Img Layout: GPUImageLayout_ShaderResource
	void postprocess_toneMapping(GPUImage* img, GPUImageLayout imgLayout, GPUImageLayout newLayout, f32 exposure, CommandList cmd);

}