#pragma once

#include "rendering/material_system.h"

namespace sv {

	struct PostProcessing {

		// BLUR EFFECTS

		static void blurBox(GPUImage* img, GPUImageLayout imgLayout, float range, u32 samples, bool vertical, bool horizontal, CommandList cmd);
		static void blurGaussian();
		static void blurSolid();

		// BLOOM EFFECT

		static void bloom(GPUImage* src);

	};

}