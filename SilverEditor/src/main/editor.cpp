#include "core_editor.h"

#include "engine.h"
#include "editor.h"

#include "imGuiDevice/ImGuiDevice.h"
#include "scene_editor.h"
#include "simulation.h"
#include "viewports.h"

namespace sve {
	
	static std::unique_ptr<ImGuiDevice> g_Device;
	static bool g_Gamemode = false;

	void editor_run()
	{
		sv::InitializationDesc desc;
		desc.callbacks.initialize = editor_initialize;
		desc.callbacks.update = editor_update;
		desc.callbacks.render = editor_render;
		desc.callbacks.close = editor_close;

		desc.sceneDesc.assetsFolderPath = "assets/";
		desc.minThreadsCount = 2;
		desc.consoleDesc.show = true;
		desc.consoleDesc.logFolder = "logs/";
		desc.rendererDesc.presentOffscreen = false;
		desc.rendererDesc.resolutionWidth = 1920u;
		desc.rendererDesc.resolutionHeight = 1080u;
		desc.windowDesc.fullscreen = false;
		desc.windowDesc.x = 0u;
		desc.windowDesc.y = 0u;
		desc.windowDesc.width = 1280u;
		desc.windowDesc.height = 720u;
		desc.windowDesc.title = L"SilverEngine";

		if (sv::engine_initialize(&desc) != sv::Result_Success) return;

		g_Device = device_create();
		g_Device->Initialize();

		// Add style
		{
			auto& style = ImGui::GetStyle();
			sv::BinFile file;
			if (file.OpenR("ImGuiStyle")) {
				file.Read(&style, sizeof(ImGuiStyle));
				file.Close();
			}
			else {
				sv::log_error("ImGui Style", "Style not found");
			}
		}

		while (true) {
			
			if (sv::engine_loop() == sv::Result_CloseRequest) break;

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

			ImGui::PushStyleColor(ImGuiCol_PopupBg, { 0.1f, 0.1f, 0.1f, 1.f });

			if (ImGui::BeginMenu("Viewports")) {

				//ImGui::Checkbox("Game", &viewport_manager_get_show("Game"));
				//ImGui::Checkbox("Scene Hierarchy", &viewport_manager_get_show("Scene Hierarchy"));
				//ImGui::Checkbox("Scene Entity", &viewport_manager_get_show("Scene Entity"));
				//ImGui::Checkbox("Scene Editor", &viewport_manager_get_show("Scene Editor"));
				//ImGui::Checkbox("Assets", &viewport_manager_get_show("Assets"));
				//ImGui::Checkbox("Console", &viewport_manager_get_show("Console"));
				//ImGui::Checkbox("Renderer2D", &viewport_manager_get_show("Renderer2D"));

				ImGui::EndMenu();
			}

			ImGui::PopStyleColor();

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

	sv::Result editor_initialize()
	{
		svCheck(simulation_initialize());
		svCheck(scene_editor_initialize());
		viewports_initialize();

		return sv::Result_Success;
	}
	void editor_update(float dt)
	{
		simulation_update(dt);
		scene_editor_update(dt);

		if (sv::input_key_pressed(SV_KEY_F11)) {
			editor_gamemode_set(!editor_gamemode_get());
		}

		g_Device->ResizeSwapChain();
	}
	void editor_render()
	{
		simulation_render();

		if (!g_Gamemode) {
			scene_editor_render();

			g_Device->BeginFrame();

			MainMenu();

			DisplayDocking();

			viewports_display();
			
			g_Device->EndFrame();
		}
	}
	sv::Result editor_close()
	{
		svCheck(simulation_close());
		svCheck(scene_editor_close());

		return sv::Result_Success;
	}

	ImGuiDevice& editor_device_get()
	{
		return *g_Device.get();
	}

	void editor_gamemode_set(bool gamemode)
	{
		if (gamemode) {
			if (simulation_running() && !simulation_paused()) {
				g_Gamemode = true;
				sv::renderer_offscreen_set_present(true);
			}
		}
		else {
			g_Gamemode = false;
			sv::renderer_offscreen_set_present(false);
		}
	}

	bool editor_gamemode_get()
	{
		return g_Gamemode;
	}

}