#include "core_editor.h"

#include "viewport_manager.h"
#include "viewports/SimulationViewport.h"
#include "simulation.h"
#include "high_level/scene.h"
#include "platform/window.h"

namespace sve {

	static bool g_Running = false;
	static bool g_Paused = false;
	static bool g_Gamemode = false;

	static bool g_RunningRequest = false;
	static bool g_StopRequest = false;

	static sv::Scene g_Scene;
	static std::string g_ScenePath;

	sv::Result simulation_initialize(const char* sceneFilePath)
	{
		g_Scene.create();

		if (g_Scene.deserialize(sceneFilePath) != sv::Result_Success) {
			g_Scene.destroy();
			g_Scene.create();
		}

		g_ScenePath = sceneFilePath;

		return sv::Result_Success;
	}

	sv::Result simulation_close()
	{
		g_Scene.destroy();
		return sv::Result_Success;
	}

	void simulation_update(float dt)
	{
		if (g_RunningRequest) {
			g_Running = true;
			g_Paused = false;
			ImGui::GetStyle().Alpha = 0.2f;

			g_Scene.serialize(g_ScenePath.c_str());
			g_RunningRequest = false;
		}

		if (g_StopRequest) {
			g_Running = false;
			g_Paused = false;
			ImGui::GetStyle().Alpha = 1.f;

			g_Scene.deserialize(g_ScenePath.c_str());
			g_StopRequest = false;
		}

		// Adjust camera
		{
			SimulationViewport* sim = (SimulationViewport*)viewport_get("Simulation");
			if (sim) {
				sv::CameraComponent& camera = *sv::ecs_component_get<sv::CameraComponent>(g_Scene, g_Scene.getMainCamera());
				sv::vec2u size = g_Gamemode ? sv::window_size_get() : sim->get_screen_size();
				camera.camera.adjust(size.x, size.y);
			}
		}

		if (g_Running && !g_Paused) {

			g_Scene.physicsSimulate(dt);

		}
	}

	void simulation_render()
	{
		SimulationViewport* sim = (SimulationViewport*)viewport_get("Simulation");
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
	}

	void simulation_continue()
	{
		if (!g_Running || !g_Paused) return;
	
		g_Paused = false;
		ImGui::GetStyle().Alpha = 0.2f;
	}

	void simulation_pause()
	{
		if (!g_Running || g_Paused) return;
		
		g_Paused = true;
		ImGui::GetStyle().Alpha = 1.f;
	}

	void simulation_stop()
	{
		if (!g_Running) return;
		g_StopRequest = true;
	}

	void simulation_gamemode_set(bool gamemode)
	{
		if (gamemode) {
			if (g_Running && !g_Paused) {
				g_Gamemode = true;
				sv::window_style_set(sv::WindowStyle_Fullscreen);
			}
		}
		else {
			g_Gamemode = false;
			sv::window_style_set(sv::WindowStyle_Default);
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

	sv::Scene& simulation_scene_get()
	{
		return g_Scene;
	}

}