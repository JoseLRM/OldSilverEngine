#include "core.h"

#include "editor.h"

#include "imGuiDevice/ImGuiDevice.h"
#include "EditorState.h"
#include "EditorApplication.h"

namespace sve {
	
	std::unique_ptr<ImGuiDevice> g_Device;
	static EditorApplication g_Application;

	void editor_run()
	{
		SV_ENGINE_INITIALIZATION_DESC desc;
		desc.minThreadsCount = 2;
		desc.showConsole = true;
		desc.rendererDesc.resolutionWidth = 1280;
		desc.rendererDesc.resolutionHeight = 720;
		desc.rendererDesc.outputMode = SV_REND_OUTPUT_MODE_OFFSCREEN;
		desc.windowDesc.fullscreen = false;
		desc.windowDesc.x = 0u;
		desc.windowDesc.y = 0u;
		desc.windowDesc.width = 1280u;
		desc.windowDesc.height = 720u;
		desc.windowDesc.title = L"SilverEngine";

		sv::engine_initialize(&g_Application, &desc);

		g_Device = device_create();
		g_Device->Initialize();

		// Add style
		{
			auto& style = ImGui::GetStyle();
			sv::BinFile file;
			if (!file.OpenR("ImGuiStyle")) {
				SV_THROW("ImGui Style", "Style not found");
			}

			file.Read(&style, sizeof(ImGuiStyle));
			file.Close();
		}

		while (true) {
			
			if (!sv::engine_loop()) break;

		}

		g_Device->Close();

		sv::engine_close();
	}

	void StyleEditor()
	{
		ImGui::ShowStyleEditor();

		if (sv::input_key_pressed('S')) {

			sv::BinFile file;
			file.OpenW("ImGuiStyle");

			file.Write(&ImGui::GetStyle(), sizeof(ImGuiStyle));

			file.Close();

			sv::log("Saved");
		}
	}

	void MainMenu()
	{
		if (ImGui::BeginMainMenuBar()) {

			if (ImGui::Button("Exit")) {
				sv::engine_request_close();
			}

			ImGui::EndMainMenuBar();
		}
	}

	void DisplayDocking()
	{
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->GetWorkPos());
			ImGui::SetNextWindowSize(viewport->GetWorkSize());
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", 0, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}


		ImGui::End();
	}

	void editor_initialize()
	{
		// Add viewports
		viewport_manager_add("Game", viewport_game_display);
		viewport_manager_add("Scene Hierarchy", viewport_scene_hierarchy_display);
		viewport_manager_add("Scene Entity", viewport_scene_entity_display);
		viewport_manager_add("Scene Editor", viewport_scene_editor_display);
		viewport_manager_add("Console", viewport_console_display);

		// Show viewports
		viewport_manager_show("Game");
		viewport_manager_show("Scene Hierarchy");
		viewport_manager_show("Scene Entity");
		viewport_manager_show("Scene Editor");
		viewport_manager_show("Console");

		engine_state_load(new sve::EditorState(), nullptr);
	}
	void editor_update(float dt)
	{

		if (sv::input_key_pressed(SV_KEY_F11)) {
			sv::window_fullscreen_set(!sv::window_fullscreen_get());
		}

		g_Device->ResizeSwapChain();

	}
	void editor_render()
	{
		g_Device->BeginFrame();

		MainMenu();

		DisplayDocking();

		viewport_manager_display();

		g_Device->EndFrame();
	}
	void editor_close()
	{
	}

	ImGuiDevice& editor_device_get()
	{
		return *g_Device.get();
	}

	EditorState& editor_state_get()
	{
		return *reinterpret_cast<EditorState*>(sv::engine_state_get_state());
	}

	sv::Scene& editor_scene_get()
	{
		return editor_state_get().GetScene();
	}

}