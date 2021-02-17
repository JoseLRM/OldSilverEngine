#pragma once

#include "SilverEngine/scene.h"
#include "SilverEngine/gui.h"

namespace sv {

	void key_shortcuts();

	void camera_controller2D(ECS* ecs, CameraComponent& camera, f32 max_projection_length = f32_max);
	void camera_controller3D(ECS* ecs, CameraComponent& camera);

	struct Editor_ECS {
	    GUI* gui;
	    ECS* ecs;
	    GuiContainer* container;

	    Entity selected_entity = SV_ENTITY_NULL;
	    GuiContainer* container_hierarchy;
	    GuiContainer* container_entity;

	    std::vector<GuiButton*> button_entity;

		GuiDrag* drag_coord[3] = {};
		GuiDrag* drag_rotation[3] = {};
		GuiDrag* drag_scale[3] = {};
	};
	
	Editor_ECS*    create_editor_ecs(ECS* ecs, GUI* gui, GuiContainer* parent);
	void update_editor_ecs(Editor_ECS* editor_ecs);
	void destroy_editor_ecs(Editor_ECS* editor_ecs);

}
