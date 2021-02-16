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
	};
	
	Editor_ECS* create_editor_ecs(ECS* ecs, GUI* gui, GuiContainer* parent);
	void        update_editor_ecs(Editor_ECS* editor_ecs);

}
