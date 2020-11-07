#pragma once

#include "Panel.h"
#include "simulation/scene.h"

namespace sv {

	class EntityInspectorPanel : public Panel {

	public:
		EntityInspectorPanel();

	protected:
		bool onDisplay() override;

	};

}