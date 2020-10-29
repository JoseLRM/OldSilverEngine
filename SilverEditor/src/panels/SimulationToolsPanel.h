#pragma once

#include "Panel.h"

namespace sv {

	class SimulationToolsViewport : public Panel {

	public:
		SimulationToolsViewport();

	protected:
		void beginDisplay() override;
		bool onDisplay() override;
		void endDisplay() override;

		ImGuiWindowFlags getWindowFlags() override;

	};

}