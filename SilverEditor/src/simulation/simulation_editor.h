#pragma once

#include "simulation/entity_system.h"

namespace sv {

	Result simulation_editor_initialize();
	Result simulation_editor_close();

	void simulation_editor_render();
	void simulation_editor_camera_controller_2D(const vec2f mousePos, float dt);

	enum TransformMode : ui32 {

		TransformMode_Translation,
		TransformMode_Scale,
		TransformMode_Rotation,

	};

	void simulation_editor_update_action(float dt);

	bool simulation_editor_is_entity_in_point(ECS* ecs, Entity entity, const vec2f& point);
	std::vector<Entity> simulation_editor_entities_in_point(const vec2f& point);
	
	extern TransformMode g_TransformMode;
	extern bool g_DebugRendering_Collisions;
	extern bool g_DebugRendering_Grid;

}