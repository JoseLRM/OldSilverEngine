#include "core.h"

#include "GraphicsProperties.h"

namespace SV {

	GraphicsProperties g_Properties;

	namespace Graphics {
		const GraphicsProperties& GetProperties()
		{
			return g_Properties;
		}

		namespace _internal {
			void SetProperties(const GraphicsProperties& props)
			{
				g_Properties = props;
			}
		}
	}
}