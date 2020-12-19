#pragma once

#include "Panel.h"

namespace sv {

	class ProfilerPanel : public Panel {

	public:
		ProfilerPanel();

	protected:
		bool onDisplay() override;
		void onClose() override;

	};

}