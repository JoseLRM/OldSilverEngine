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
		ui32 m_RenderLayerSelected = ui32_max;

	};

}