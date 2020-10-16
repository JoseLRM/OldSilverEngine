#pragma once

#include "Viewport.h"
#include "simulation/entity_system.h"

namespace sve {

	class SceneHierarchyViewport : public Viewport {

	public:
		SceneHierarchyViewport();

	protected:
		bool onDisplay() override;

	private:
		void showEntity(sv::ECS* ecs, sv::Entity entityentity);

	private:
		sv::Entity m_SelectedEntity = SV_ENTITY_NULL;
		bool m_Popup = false;

	};

}