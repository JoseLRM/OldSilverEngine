#pragma once

#include "SilverEngine/renderer.h"
#include "SilverEngine/gui.h"
#include "SilverEngine/entity_system.h"

namespace sv {

	// PANELS

	enum EditorPanelType : u32 {
		EditorPanelType_None,
		EditorPanelType_Console,
		EditorPanelType_Runtime,
	};

	struct EditorPanel {

		GuiWindow* window = nullptr;
		bool close_request = false;

		const EditorPanelType type;

		EditorPanel(EditorPanelType type) : type(type) {}
		virtual ~EditorPanel() {}
	};

	struct ConsolePanel : public EditorPanel {
		ConsolePanel() : EditorPanel(EditorPanelType_Console) {}
	};

	struct RuntimePanel : public EditorPanel {
		RuntimePanel() : EditorPanel(EditorPanelType_Runtime) {}
	};

	// EDITOR

	SV_DEFINE_HANDLE(Editor);

	Editor* editor_create(GUI* gui);
	void	editor_destroy(Editor* editor);
	void	editor_update(Editor* editor);

	ConsolePanel* editor_console_create(Editor* editor);
	RuntimePanel* editor_runtime_create(Editor* editor);

	std::pair<EditorPanel**, u32> editor_panels(Editor* editor);

	// MISC

	void editor_key_shortcuts(Editor* editor);

	void editor_camera_controller2D(v2_f32& position, CameraProjection& projection);
	void editor_camera_controller3D(v3_f32& position, v2_f32& rotation, CameraProjection& projection);

}