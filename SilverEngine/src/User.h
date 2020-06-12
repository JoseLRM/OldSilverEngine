#pragma once

#include "core.h"
#include "GraphicsDesc.h"

namespace SV {

	struct Adapter {

		struct OutputMode {
			SV_GFX_FORMAT format;
			ui32 width, height;
			struct {
				ui32 numerator;
				ui32 denominator;
			} refreshRate;

			SV_GFX_MODE_SCALING scaling;
			SV_GFX_MODE_SCANLINE_ORDER scanlineOrdering;

			std::wstring deviceName;

			uvec2 resolution;
		};

		std::vector<OutputMode> modes;
		std::wstring description;
	};

	namespace User {

		namespace _internal {
			bool Initialize();
		}

		const std::vector<Adapter>& GetAdapters();
		SV_GFX_API GetGraphicsAPI();

	}
}