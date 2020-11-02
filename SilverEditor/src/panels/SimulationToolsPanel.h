#pragma once

#include "Panel.h"

namespace sv {

	class SimulationToolsPanel : public Panel {

	public:
		SimulationToolsPanel();

	protected:
		void beginDisplay() override;
		bool onDisplay() override;
		void endDisplay() override;

		ImGuiWindowFlags getWindowFlags() override;

	};

}