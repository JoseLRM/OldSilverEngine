#include "core.h"

#include "graphics_internal.h"

namespace sv {

	static GraphicsProperties g_Properties;

	void graphics_properties_set(const sv::GraphicsProperties& props)
	{
		g_Properties = props;
	}

	GraphicsProperties graphics_properties_get()
	{
		return g_Properties;
	}

}

