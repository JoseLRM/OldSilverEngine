#pragma once

#include "Panel.h"
#include "simulation/scene.h"

namespace sv {

	class EntityInspectorViewport : public Panel {

	public:
		EntityInspectorViewport();

		void setEntity(Entity entity);

	protected:
		bool onDisplay() override;

	private:
		Entity m_Entity = SV_ENTITY_NULL;

	};

}