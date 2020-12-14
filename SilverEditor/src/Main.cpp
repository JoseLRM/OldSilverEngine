#include "core_editor.h"

#include "platform/input.h"

#include "simulation.h"
#include "panel_manager.h"
#include "gui.h"
#include "engine.h"

using namespace sv;

namespace sv {

	// TEMP
	void StyleEditor()
	{
		ImGui::ShowStyleEditor();

		if (input_key_pressed('S')) {

			std::ofstream file;

			file.open("ImGuiStyle", std::ios::binary);

			file.seekp(0u);
			file.write((const char*)& ImGui::GetStyle(), sizeof(ImGuiStyle));

			file.close();

			SV_LOG_INFO("Saved");
		}
	}

	Result editor_initialize()
	{
		svCheck(simulation_initialize());
		svCheck(panel_manager_initialize());

		return Result_Success;
	}

	void editor_update(float dt)
	{
		simulation_update(dt);

		if (input_key_pressed(SV_KEY_F11)) {
			simulation_gamemode_set(!simulation_gamemode_get());
		}

		// Update Assets
		static float assetTimeCount = 0.f;

		if (!simulation_gamemode_get()) {
			assetTimeCount += dt;
			if (assetTimeCount >= 1.f) {

				asset_refresh();

				assetTimeCount--;
			}
		}

		// FPS count
		static float fpsTime = 0.f;
		static u32 fpsCount = 0u;
		fpsTime += dt;
		fpsCount++;

		if (fpsTime >= 0.25f) {

			std::wstring title = L"SilverEditor | FPS: ";
			title += std::to_wstring(fpsCount * 4u);

			window_title_set(title.c_str());

			fpsTime -= 0.25f;
			fpsCount = 0u;
		}
	}

	void editor_render()
	{
		simulation_render();

		if (!simulation_gamemode_get()) {

			gui_begin();
			//StyleEditor();
			panel_manager_display();
			simulation_display();
			gui_end();

		}
	}
	Result editor_close()
	{
		svCheck(simulation_close());
		svCheck(panel_manager_close());

		return Result_Success;
	}

}

int main()
{
	InitializationDesc desc;
	desc.callbacks.initialize = editor_initialize;
	desc.callbacks.update = editor_update;
	desc.callbacks.render = editor_render;
	desc.callbacks.close = editor_close;

	desc.assetsFolderPath = "assets/";
	desc.minThreadsCount = 2;
	desc.iconFilePath = L"icon.ico";
	desc.windowStyle = WindowStyle_Default;
	desc.windowBounds.x = 0u;
	desc.windowBounds.y = 0u;
	desc.windowBounds.z = 1280u;
	desc.windowBounds.w = 720u;
	desc.windowTitle = L"SilverEngine";

	if (engine_initialize(&desc) != Result_Success) {
		return 1;
	}

	gui_initialize();

	while (true) {
		if (engine_loop() != Result_Success) break;
	}

	gui_close();

	engine_close();

	return 0;
}
