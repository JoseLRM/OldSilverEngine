#pragma once

#include "renderer_desc.h"

namespace _sv {

	bool renderer_postprocessing_initialize();
	bool renderer_postprocessing_close();

	bool renderer_postprocessing_default_create(SV_GFX_FORMAT dstFormat, SV_GFX_IMAGE_LAYOUT initialLayout, SV_GFX_IMAGE_LAYOUT finalLayout, PostProcessing_Default& pp);
	bool renderer_postprocessing_default_destroy(PostProcessing_Default& pp);
	void renderer_postprocessing_default_render(PostProcessing_Default& pp, sv::Image& src, sv::Image& dst, sv::CommandList cmd);

}