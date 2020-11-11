#pragma once

#include "core_editor.h"
#include "simulation/scene.h"

namespace sv {

	struct DebugCamera {
		Camera camera;
		vec3f position;
		vec4f rotation;
	};

	Result simulation_initialize(const char* sceneFilePath);
	Result simulation_close();
	void simulation_update(float dt);
	void simulation_render();
	void simulation_display();

	void simulation_run();		// Start/Reset the simulation
	void simulation_continue(); // If it was paused, continue
	void simulation_pause();	// If it was running, pause but not stop the simulation
	void simulation_stop();		// Destroy the simulation
	void simulation_gamemode_set(bool gamemode);

	bool simulation_running();	// Return true if the simulation is running or paused
	bool simulation_paused();	// Return true if the simulation is paused
	bool simulation_gamemode_get();

	extern DebugCamera g_DebugCamera;
	extern Scene g_Scene;
	extern vec2f g_SimulationMousePos;
	extern bool g_SimulationMouseInCamera;
	extern Entity g_SelectedEntity;

}