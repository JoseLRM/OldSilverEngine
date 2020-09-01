#include "core_editor.h"

#include "viewports/viewport_assets.h"
#include "viewports/viewport_scene_editor.h"
#include "viewports/viewport_simulation.h"

#include "viewports.h"

namespace sve {

	constexpr static ui32 g_ViewportsCount = 5u;
	static Viewport g_Viewports[g_ViewportsCount];


	void viewports_initialize()
	{
		g_Viewports[SVE_VIEWPORT_SIMULATION] = { false, "Simulation" };
		g_Viewports[SVE_VIEWPORT_ASSETS] = { false, "Assets" };
		g_Viewports[SVE_VIEWPORT_SCENE_HIERARCHY] = { false, "Scene Hierarchy" };
		g_Viewports[SVE_VIEWPORT_SCENE_EDITOR] = { false, "Scene Editor" };
		g_Viewports[SVE_VIEWPORT_ENTITY_INSPECTOR] = { false, "Entity Inspector" };

		viewports_show(SVE_VIEWPORT_SIMULATION);
		viewports_show(SVE_VIEWPORT_ASSETS);
		viewports_show(SVE_VIEWPORT_SCENE_HIERARCHY);
		viewports_show(SVE_VIEWPORT_SCENE_EDITOR);
		viewports_show(SVE_VIEWPORT_ENTITY_INSPECTOR);
	}

	void viewports_close()
	{

	}

#define hideCheck(x) do { if (!(x)) { viewports_hide(i); }; } while(0)
	void viewports_display()
	{

		for (ui32 i = 0; i < g_ViewportsCount; ++i) {

			Viewport& viewport = g_Viewports[i];

			switch (i)
			{
			
			case SVE_VIEWPORT_SIMULATION:
				hideCheck(viewport_simulation_display());
				break;

			case SVE_VIEWPORT_ASSETS:
				hideCheck(viewport_assets_display());
				break;

			case SVE_VIEWPORT_SCENE_HIERARCHY:
				hideCheck(viewport_scene_hierarchy_display());
				break;

			case SVE_VIEWPORT_SCENE_EDITOR:
				hideCheck(viewport_scene_editor_display());
				break;

			case SVE_VIEWPORT_ENTITY_INSPECTOR:
				hideCheck(viewport_scene_entity_inspector_display());
				break;

			}
		}

	}

	void viewports_show(ui32 index)
	{
		sv::log_info("Showing viewport: %s", g_Viewports[index].name);
		g_Viewports[index].active = true;
	}

	void viewports_hide(ui32 index)
	{
		sv::log_info("Hiding viewport: %s", g_Viewports[index].name);
		g_Viewports[index].active = false;
	}

	bool viewports_is_active(ui32 index)
	{
		return g_Viewports[index].active;
	}

	const char* viewports_get_name(ui32 index)
	{
		return g_Viewports[index].name;
	}

}