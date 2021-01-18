#pragma once

#include "SilverEngine/render_utils/debug_renderer.h"
#include "SilverEngine/render_utils/text.h"

namespace sv {

	Result debug_renderer_initialize();
	Result debug_renderer_close();

	Result text_initialize();
	Result text_close();

	SV_INLINE Result render_utils_initialize()
	{
		svCheck(debug_renderer_initialize());
		svCheck(text_initialize());

		return Result_Success;
	}

	SV_INLINE Result render_utils_close()
	{
		svCheck(debug_renderer_close());
		svCheck(text_close());

		return Result_Success;
	}

}