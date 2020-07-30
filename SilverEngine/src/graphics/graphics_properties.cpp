#include "core.h"

#include "graphics_properties.h"

using namespace sv;

namespace _sv {

	static GraphicsProperties g_Properties;

	void graphics_properties_set(const sv::GraphicsProperties& props)
	{
		g_Properties = props;
	}

}

namespace sv {

	using namespace _sv;

	GraphicsProperties graphics_properties_get()
	{
		return g_Properties;
	}

}

