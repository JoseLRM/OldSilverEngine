#pragma once

#include "core.h"

namespace sv {

	struct GraphicsProperties {
		bool transposedMatrices;
	};

	GraphicsProperties graphics_properties_get();

}

namespace _sv {

	void graphics_properties_set(const sv::GraphicsProperties& props);

}