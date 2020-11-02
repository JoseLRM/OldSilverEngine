#pragma once

#include "Panel.h"

namespace sv {

	class SceneEditorPanel : public Panel {
	public:
		SceneEditorPanel();

		inline const vec2u& get_screen_size() const noexcept { return m_ScreenSize; }

	protected:
		void beginDisplay() override;
		bool onDisplay() override;
		void endDisplay() override;

		ImGuiWindowFlags getWindowFlags() override;

	private:
		vec2u m_ScreenSize = { 1u, 1u };

	};

}