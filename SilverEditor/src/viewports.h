#pragma once

namespace sve {

#define SVE_VIEWPORT_SIMULATION 0
#define SVE_VIEWPORT_ASSETS 1
#define SVE_VIEWPORT_SCENE_HIERARCHY 2
#define SVE_VIEWPORT_SCENE_EDITOR 3
#define SVE_VIEWPORT_ENTITY_INSPECTOR 4
#define SVE_VIEWPORT_SIMULATION_TOOLS 5
#define SVE_VIEWPORT_MATERIALS 6

	struct Viewport {
		bool		active;
		const char* name;
	};

	void viewports_initialize();
	void viewports_close();

	void viewports_display();

	void viewports_show(ui32 index);
	void viewports_hide(ui32 index);
	bool viewports_is_active(ui32 index);
	const char* viewports_get_name(ui32 index);

}