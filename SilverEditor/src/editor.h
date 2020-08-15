#pragma once

#include "core_editor.h"
#include "main/imGuiDevice/ImGuiDevice.h"

namespace sve {

	void editor_run();

	bool editor_initialize();
	void editor_update(float dt);
	void editor_render();
	bool editor_close();

	ImGuiDevice& editor_device_get();

}