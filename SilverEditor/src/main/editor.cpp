#include "core_editor.h"

#include "main/engine.h"
#include "platform/input.h"
#include "editor.h"

#include "imGuiDevice/ImGuiDevice.h"
#include "scene_editor.h"
#include "simulation.h"
#include "viewport_manager.h"

namespace sve {
	
	static std::unique_ptr<ImGuiDevice> g_Device;

	void editor_run()
	{
		sv::InitializationDesc desc;
		desc.callbacks.initialize = editor_initialize;
		desc.callbacks.update = editor_update;
		desc.callbacks.render = editor_render;
		desc.callbacks.close = editor_close;

		desc.assetsFolderPath = "assets/";
		desc.minThreadsCount = 2;
		desc.iconFilePath = L"icon.ico";
		desc.windowStyle = sv::WindowStyle_Default;
		desc.windowBounds.x = 0u;
		desc.windowBounds.y = 0u;
		desc.windowBounds.z = 1280u;
		desc.windowBounds.w = 720u;
		desc.windowTitle = L"SilverEngine";

		if (sv::engine_initialize(&desc) != sv::Result_Success) {
			return;
		}

		g_Device = device_create();
		g_Device->Initialize();

		// Add style
		{
			auto& style = ImGui::GetStyle();
			
			std::ifstream file;

			std::string path = "ImGuiStyle";
#ifdef SV_RES_PATH
			path = SV_RES_PATH + path;
#endif

			file.open(path, std::ios::binary);

			if (file.is_open()) {
				file.seekg(0u);
				file.read((char*)&style, sizeof(ImGuiStyle));
				file.close();
			}
			else {
				SV_LOG_ERROR("ImGui Style", "Style not found");
			}
		}

		while (true) {
			
			if (sv::engine_loop() != sv::Result_Success) break;

		}

		g_Device->Close();

		sv::engine_close();
	}

	void StyleEditor()
	{
		ImGui::ShowStyleEditor();

		if (sv::input_key_pressed('S')) {

			std::ofstream file;

			file.open("ImGuiStyle", std::ios::binary);

			file.seekp(0u);
			file.write((const char*)&ImGui::GetStyle(), sizeof(ImGuiStyle));

			file.close();

			SV_LOG_INFO("Saved");
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
		svCheck(simulation_initialize("assets/scenes/Test.scene"));
		svCheck(scene_editor_initialize());
		svCheck(viewport_initialize());

		return sv::Result_Success;
	}

	void editor_update(float dt)
	{
		simulation_update(dt);
		scene_editor_update(dt);

		if (sv::input_key_pressed(SV_KEY_F11)) {
			simulation_gamemode_set(!simulation_gamemode_get());
		}
		if (sv::input_key(SV_KEY_CONTROL) && sv::input_key_pressed('S')) {
			sv::Scene& scene = simulation_scene_get();
			sv::Result res = scene.serialize("assets/scenes/Test.scene");
			SV_ASSERT(sv::result_okay(res));
		}

		g_Device->ResizeSwapChain();

		// Update Assets
		static float assetTimeCount = 0.f;

		if (!simulation_gamemode_get()) {
			assetTimeCount += dt;
			if (assetTimeCount >= 1.f) {

				sv::assets_refresh();

				assetTimeCount--;
			}
		}

		// FPS count
		static float fpsTime = 0.f;
		static ui32 fpsCount = 0u;
		fpsTime += dt;
		fpsCount++;

		if (fpsTime >= 0.25f) {

			std::wstring title = L"SilverEditor | FPS: ";
			title += std::to_wstring(fpsCount * 4u);

			sv::window_title_set(title.c_str());

			fpsTime -= 0.25f;
			fpsCount = 0u;
		}
	}

	void editor_render()
	{
		simulation_render();

		if (!simulation_gamemode_get()) {		
			scene_editor_render();

			g_Device->BeginFrame();

			sv::GPUImage* simulationOffscreen = nullptr;
			sv::GPUImage* editorOffscreen = scene_editor_camera_get().camera.getOffscreenRT();

			sv::Scene& scene = simulation_scene_get();
			sv::Entity cameraEntity = scene.getMainCamera();

			if (sv::ecs_entity_exist(scene, cameraEntity)) {

				sv::CameraComponent* camera = sv::ecs_component_get<sv::CameraComponent>(scene, cameraEntity);
				if (camera) {
					simulationOffscreen = camera->camera.getOffscreenRT();
				}
			}

			sv::GPUBarrier barriers[2];
			barriers[0] = sv::GPUBarrier::Image(editorOffscreen, sv::GPUImageLayout_RenderTarget, sv::GPUImageLayout_ShaderResource);
			if (simulationOffscreen) barriers[1] = sv::GPUBarrier::Image(simulationOffscreen, sv::GPUImageLayout_RenderTarget, sv::GPUImageLayout_ShaderResource);
			sv::graphics_barrier(barriers, simulationOffscreen ? 2u : 1u, g_Device->GetCMD());

			
			MainMenu();
			DisplayDocking();
			viewport_display();
			g_Device->EndFrame();

			barriers[0] = sv::GPUBarrier::Image(editorOffscreen, sv::GPUImageLayout_ShaderResource, sv::GPUImageLayout_RenderTarget);
			if (simulationOffscreen) barriers[1] = sv::GPUBarrier::Image(simulationOffscreen, sv::GPUImageLayout_ShaderResource, sv::GPUImageLayout_RenderTarget);
			sv::graphics_barrier(barriers, simulationOffscreen ? 2u : 1u, g_Device->GetCMD());
		}
	}
	sv::Result editor_close()
	{
		svCheck(simulation_close());
		svCheck(scene_editor_close());
		svCheck(viewport_close());

		return sv::Result_Success;
	}

	ImGuiDevice& editor_device_get()
	{
		return *g_Device.get();
	}

}