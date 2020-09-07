#pragma once

#include "core_editor.h"
#include "scene.h"

namespace sve {

	sv::Result simulation_initialize(const char* sceneFilePath);
	sv::Result simulation_close();
	void simulation_update(float dt);
	void simulation_render();

	void simulation_run();		// Start/Reset the simulation
	void simulation_continue(); // If it was paused, continue
	void simulation_pause();	// If it was running, pause but not stop the simulation
	void simulation_stop();		// Destroy the simulation
	void simulation_gamemode_set(bool gamemode);

	bool simulation_running();	// Return true if the simulation is running or paused
	bool simulation_paused();	// Return true if the simulation is paused
	bool simulation_gamemode_get();

	sv::Scene*	simulation_scene_get();

}