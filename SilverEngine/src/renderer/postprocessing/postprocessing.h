#pragma once

#include "graphics.h"

namespace sv {

	struct PostProcessing_Default {
		sv::RenderPass renderPass;
	};

	bool postprocessing_initialize();
	bool postprocessing_close();
		 
	bool postprocessing_default_create(Format dstFormat, GPUImageLayout initialLayout, GPUImageLayout finalLayout, PostProcessing_Default& pp);
	bool postprocessing_default_destroy(PostProcessing_Default& pp);
	void postprocessing_default_render(PostProcessing_Default& pp, GPUImage& src, GPUImage& dst, CommandList cmd);

}