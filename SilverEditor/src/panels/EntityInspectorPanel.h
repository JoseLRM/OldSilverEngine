#pragma once

#include "Panel.h"
#include "simulation/scene.h"

namespace sv {

	class EntityInspectorPanel : public Panel {

	public:
		EntityInspectorPanel();

		void setEntity(Entity entity);

	protected:
		bool onDisplay() override;

	private:
		Entity m_Entity = SV_ENTITY_NULL;

	};

}