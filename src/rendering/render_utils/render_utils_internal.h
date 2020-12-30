#pragma once

#include "rendering/render_utils.h"

namespace sv {

	Result render_utils_initialize();
	Result render_utils_close();
	void render_utils_update(f32 deltaTime);

	Result auximg_initialize();
	Result auximg_close();
	void auximg_update(Time now);

}