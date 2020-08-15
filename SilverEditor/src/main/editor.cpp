#include "core_editor.h"

#include "engine.h"
#include "editor.h"

#include "imGuiDevice/ImGuiDevice.h"
#include "scene_editor.h"
#include "simulation.h"
#include "viewport_manager.h"

namespace sve {
	
	static std::unique_ptr<ImGuiDevice> g_Device;
	static bool g_Simulation = false;

	void editor_run()
	{
		sv::InitializationDesc desc;
		desc.callbacks.initialize = editor_initialize;
		desc.callbacks.update = editor_update;
		desc.callbacks.render = editor_render;
		desc.callbacks.close = editor_close;

		desc.minThreadsCount = 2;
		desc.showConsole = true;
		desc.rendererDesc.resolutionWidth = 1280;
		desc.rendererDesc.resolutionHeight = 720;
		desc.windowDesc.fullscreen = false;
		desc.windowDesc.x = 0u;
		desc.windowDesc.y = 0u;
		desc.windowDesc.width = 1280u;
		desc.windowDesc.height = 720u;
		desc.windowDesc.title = L"SilverEngine";

		if (!sv::engine_initialize(&desc)) return;

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

			ImGui::PushStyleColor(ImGuiCol_PopupBg, { 0.1f, 0.1f, 0.1f, 1.f });

			if (ImGui::BeginMenu("Viewports")) {

				ImGui::Checkbox("Game", &viewport_manager_get_show("Game"));
				ImGui::Checkbox("Scene Hierarchy", &viewport_manager_get_show("Scene Hierarchy"));
				ImGui::Checkbox("Scene Entity", &viewport_manager_get_show("Scene Entity"));
				ImGui::Checkbox("Scene Editor", &viewport_manager_get_show("Scene Editor"));
				ImGui::Checkbox("Console", &viewport_manager_get_show("Console"));
				ImGui::Checkbox("Renderer2D", &viewport_manager_get_show("Renderer2D"));

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

	bool editor_initialize()
	{
		// Add viewports
		viewport_manager_add("Game", viewport_game_display);
		viewport_manager_add("Scene Hierarchy", viewport_scene_hierarchy_display);
		viewport_manager_add("Scene Entity", viewport_scene_entity_display);
		viewport_manager_add("Scene Editor", viewport_scene_editor_display);
		viewport_manager_add("Console", viewport_console_display);
		viewport_manager_add("Renderer2D", viewport_renderer2D_display);

		// Show viewports
		viewport_manager_show("Game");
		viewport_manager_show("Scene Hierarchy");
		viewport_manager_show("Scene Entity");
		viewport_manager_show("Scene Editor");
		viewport_manager_show("Console");
		viewport_manager_show("Renderer2D");

		svCheck(simulation_initialize());
		svCheck(scene_editor_initialize());

		return true;
	}
	void editor_update(float dt)
	{
		simulation_update(dt);
		scene_editor_update(dt);

		if (sv::input_key_pressed(SV_KEY_F11)) {
			sv::window_fullscreen_set(!sv::window_fullscreen_get());
		}

		g_Device->ResizeSwapChain();
	}
	void editor_render()
	{
		simulation_render();
		scene_editor_render();

		g_Device->BeginFrame();

		MainMenu();

		DisplayDocking();
		
		viewport_manager_display();
		
		g_Device->EndFrame();
	}
	bool editor_close()
	{
		svCheck(simulation_close());
		svCheck(scene_editor_close());

		return true;
	}

	ImGuiDevice& editor_device_get()
	{
		return *g_Device.get();
	}

}