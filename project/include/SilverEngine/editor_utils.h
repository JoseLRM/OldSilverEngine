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
	    std::vector<GuiButton*> button_entity;
	    Entity selected_entity = SV_ENTITY_NULL;
	};

	struct Editor_Entity {
	    GUI* gui;
	    ECS* ecs;
	    Entity entity = SV_ENTITY_NULL;
	    
	    GuiContainer* container;

	    // TODO: Use drag widget
	    GuiButton* button_coord[3] = {};
	    GuiButton* drag_coord[3] = {};

	    GuiButton* button_rotation[3] = {};
	    GuiButton* drag_rotation[3] = {};

	    GuiButton* button_scale[3] = {};
	    GuiButton* drag_scale[3] = {};
	};
	
	Editor_ECS*    create_editor_ecs(ECS* ecs, GUI* gui, GuiContainer* parent);
	Editor_Entity* create_editor_entity(ECS* ecs, GUI* gui, GuiContainer* parent);
	
	void update_editor_ecs(Editor_ECS* editor_ecs);
	void update_editor_entity(Editor_Entity* editor_entity);

	void destroy_editor_ecs(Editor_ECS* editor_ecs);
	void destroy_editor_entity(Editor_Entity* editor_entity);

}
