#pragma once

#include "Panel.h"
#include "simulation/entity_system.h"

namespace sv {

	class SceneHierarchyViewport : public Panel {

	public:
		SceneHierarchyViewport();

	protected:
		bool onDisplay() override;

	private:
		void showEntity(ECS* ecs, Entity entityentity);

	private:
		Entity m_SelectedEntity = SV_ENTITY_NULL;
		bool m_Popup = false;

	};

}