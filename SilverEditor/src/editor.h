#pragma once

#include "core_editor.h"
#include "main/imGuiDevice/ImGuiDevice.h"

namespace sve {

	void editor_run();

	sv::Result editor_initialize();
	void editor_update(float dt);
	void editor_render();
	sv::Result editor_close();

	ImGuiDevice& editor_device_get();

}