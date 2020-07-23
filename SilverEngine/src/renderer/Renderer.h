#pragma once

#include "core.h"
#include "Camera.h"
#include "graphics/Graphics.h"

struct SV_RENDERER_INITIALIZATION_DESC {
	ui32 resolutionWidth;
	ui32 resolutionHeight;
};

namespace SV {
	namespace Renderer {

		namespace _internal {
			bool Initialize(const SV_RENDERER_INITIALIZATION_DESC& desc);
			bool Close();

			void BeginFrame();
			void Render();
			void EndFrame();
		}

		void SetResolution(ui32 width, ui32 height);

		uvec2 GetResolution() noexcept;
		ui32 GetResolutionWidth() noexcept;
		ui32 GetResolutionHeight() noexcept;
		float GetResolutionAspect() noexcept;

	}
}