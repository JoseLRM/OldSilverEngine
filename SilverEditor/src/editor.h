#pragma once

#include "core.h"
#include "imGuiDevice/ImGuiDevice.h"
#include "viewports/viewport_manager.h"
#include "EditorState.h"

namespace sve {

	void editor_run();

	void editor_initialize();
	void editor_update(float dt);
	void editor_render();
	void editor_close();

	ImGuiDevice& editor_device_get();
	EditorState& editor_state_get();
}