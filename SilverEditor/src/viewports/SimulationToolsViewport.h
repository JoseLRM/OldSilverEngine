#pragma once

#include "Viewport.h"

namespace sve {

	class SimulationToolsViewport : public Viewport {

	public:
		SimulationToolsViewport();

	protected:
		void beginDisplay() override;
		bool onDisplay() override;
		void endDisplay() override;

		ImGuiWindowFlags getWindowFlags() override;

	};

}