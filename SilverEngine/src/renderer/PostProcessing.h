#pragma once

#include "core.h"
#include "graphics/Graphics.h"

namespace SV {

	namespace Renderer {
		namespace _internal {
			bool InitializePostProcessing();
			bool ClosePostProcessing();
		}
	}
	
	class DefaultPostProcessing {
		SV::RenderPass			m_RenderPass;

	public:
		bool Create(SV_GFX_FORMAT dstFormat, SV_GFX_IMAGE_LAYOUT initialLayout, SV_GFX_IMAGE_LAYOUT finalLayout);
		bool Destroy();

		void PostProcess(SV::Image& src, SV::Image& dst, CommandList cmd);

	};

}