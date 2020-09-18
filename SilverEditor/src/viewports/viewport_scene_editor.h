#pragma once

#include "core_editor.h"

namespace sve {

	bool viewport_scene_hierarchy_display();
	bool viewport_scene_entity_inspector_display();
	bool viewport_scene_editor_display();

	bool viewport_scene_editor_visible();
	sv::vec2u viewport_scene_editor_size();
	bool viewport_scene_editor_has_focus();

}