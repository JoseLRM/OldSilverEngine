#pragma once

#include "core.h"

namespace sve {

	typedef bool(*DisplayFunction)();

	struct ViewportProperties {
		ui32 x, y;
		ui32 width, height;
		bool focus;
	};

	struct Viewport {
		DisplayFunction displayFn;
		bool show;
		ViewportProperties properties;
	};

	void viewport_manager_add(const char* name, DisplayFunction displayFn);
	void viewport_manager_display();
	void viewport_manager_show(const char* name);
	void viewport_manager_hide(const char* name);
	ViewportProperties viewport_manager_properties_get(const char* name);

	bool viewport_game_display();
	bool viewport_scene_hierarchy_display();
	bool viewport_scene_entity_display();
	bool viewport_scene_editor_display();
	bool viewport_console_display();

}