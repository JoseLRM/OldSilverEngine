#pragma once

#include "Panel.h"

namespace sv {

	class RendererPanel : public Panel {

	public:
		RendererPanel();

	protected:
		bool onDisplay() override;
		void onClose() override;

	private:
		u32 m_RenderLayerSelected = u32_max;

	};

}