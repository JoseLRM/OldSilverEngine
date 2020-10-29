#pragma once

#include "core_editor.h"
#include "main/imGuiDevice/ImGuiDevice.h"

namespace sv {

	void editor_run();

	Result editor_initialize();
	void editor_update(float dt);
	void editor_render();
	Result editor_close();

	ImGuiDevice& editor_device_get();

}