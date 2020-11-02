#include "core_editor.h"

#include "panel_manager.h"
#include "panels/SimulationPanel.h"
#include "simulation.h"
#include "simulation/scene.h"
#include "platform/window.h"
#include "simulation/animator.h"
#include "gui.h"

namespace sv {

	static bool g_Running = false;
	static bool g_Paused = false;
	static bool g_Gamemode = false;

	static bool g_RunningRequest = false;
	static bool g_StopRequest = false;

	static Scene g_Scene;
	static std::string g_ScenePath;

	Result simulation_initialize(const char* sceneFilePath)
	{
		g_Scene.create();

		if (g_Scene.deserialize(sceneFilePath) != Result_Success) {
			g_Scene.destroy();
			g_Scene.create();
		}

		g_ScenePath = sceneFilePath;
		engine_animations_disable();

		return Result_Success;
	}

	Result simulation_close()
	{
		g_Scene.destroy();
		return Result_Success;
	}

	void simulation_update(float dt)
	{
		if (g_RunningRequest) {
			g_Running = true;
			g_Paused = false;
			gui_style_simulation();

			g_Scene.serialize(g_ScenePath.c_str());
			g_RunningRequest = false;
		}

		if (g_StopRequest) {
			g_Running = false;
			g_Paused = false;
			gui_style_editor();

			g_Scene.deserialize(g_ScenePath.c_str());
			g_StopRequest = false;
		}

		// Adjust camera
		{
			SimulationPanel* sim = (SimulationPanel*)panel_manager_get("Simulation");
			if (sim) {
				if (g_Scene.getMainCamera() != SV_ENTITY_NULL) {
					CameraComponent* camera = ecs_component_get<CameraComponent>(g_Scene, g_Scene.getMainCamera());
					if (camera) {
						vec2u size = g_Gamemode ? window_size_get() : sim->get_screen_size();
						camera->camera.adjust(size.x, size.y);
					}
				}
			}
		}

		if (g_Running && !g_Paused) {

			g_Scene.physicsSimulate(dt);

		}
	}

	void simulation_render()
	{
		SimulationPanel* sim = (SimulationPanel*)panel_manager_get("Simulation");
		if (sim == nullptr) return;

		if (!simulation_running() && !sim->isVisible()) {
			return;
		}

		g_Scene.draw(g_Gamemode);
	}

	void simulation_run()
	{
		if (g_Running) return;
		g_RunningRequest = true;
		engine_animations_enable();
	}

	void simulation_continue()
	{
		if (!g_Running || !g_Paused) return;
	
		g_Paused = false;
		gui_style_simulation();
		engine_animations_enable();
	}

	void simulation_pause()
	{
		if (!g_Running || g_Paused) return;
		
		g_Paused = true;
		gui_style_editor();
		engine_animations_disable();
	}

	void simulation_stop()
	{
		if (!g_Running) return;
		g_StopRequest = true;
		engine_animations_disable();
	}

	void simulation_gamemode_set(bool gamemode)
	{
		if (gamemode) {
			if (g_Running && !g_Paused) {
				g_Gamemode = true;
				window_style_set(WindowStyle_Fullscreen);
			}
		}
		else {
			g_Gamemode = false;
			window_style_set(WindowStyle_Default);
		}
	}

	bool simulation_running()
	{
		return g_Running;
	}

	bool simulation_paused()
	{
		return g_Paused;
	}

	bool simulation_gamemode_get()
	{
		return g_Gamemode;
	}

	Scene& simulation_scene_get()
	{
		return g_Scene;
	}

}