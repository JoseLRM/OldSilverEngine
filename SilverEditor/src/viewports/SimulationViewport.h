#pragma once

#include "Viewport.h"

namespace sve {

	class SimulationViewport : public Viewport {
	public:
		SimulationViewport();

		inline const sv::vec2u& get_screen_size() const noexcept { return m_ScreenSize; }

	protected:
		void beginDisplay() override;
		bool onDisplay() override;
		void endDisplay() override;

		ImGuiWindowFlags getWindowFlags() override;

	private:
		sv::vec2u m_ScreenSize = { 1u, 1u };

	};

}