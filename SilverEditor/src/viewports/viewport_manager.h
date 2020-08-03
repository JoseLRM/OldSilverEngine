#pragma once

#include "core.h"

namespace sve {

	typedef bool(*DisplayFunction)();

	struct Viewport {
		DisplayFunction displayFn;
		bool show;
	};

	void viewport_manager_add(const char* name, DisplayFunction displayFn);
	void viewport_manager_display();
	void viewport_manager_show(const char* name);
	void viewport_manager_hide(const char* name);

	sv::uvec2 viewport_game_get_size();

	bool viewport_game_display();
	bool viewport_scene_hierarchy_display();
	bool viewport_scene_entity_display();

}