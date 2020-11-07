#pragma once

#include "Panel.h"
#include "simulation/entity_system.h"

namespace sv {

	class SceneHierarchyPanel : public Panel {

	public:
		SceneHierarchyPanel();

	protected:
		bool onDisplay() override;

	private:
		void showEntity(ECS* ecs, Entity entityentity);

	private:
		bool m_Popup = false;

	};

}