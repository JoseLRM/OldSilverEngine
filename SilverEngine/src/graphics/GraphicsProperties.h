#pragma once

#include "core.h"

namespace SV {

	struct GraphicsProperties {
		bool transposedMatrices;
	};

	namespace Graphics {
		const GraphicsProperties& GetProperties();

		namespace _internal {
			void SetProperties(const GraphicsProperties& props);
		}
	}
}