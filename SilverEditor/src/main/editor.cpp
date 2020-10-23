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
			ImVec4* colors = ImGui::GetStyle().Colors;
			colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
			colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.54f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.43f, 0.43f, 0.43f, 0.40f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.67f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.00f, 0.00f, 0.51f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.98f, 0.32f, 1.00f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.88f, 0.27f, 1.00f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.28f, 0.98f, 0.26f, 1.00f);
			colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.24f, 0.54f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.43f, 0.43f, 0.40f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.67f);
			colors[ImGuiCol_Header] = ImVec4(0.77f, 0.77f, 0.77f, 0.31f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.66f, 0.66f, 0.66f, 0.43f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.63f, 0.63f, 0.63f, 0.63f);
			colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
			colors[ImGuiCol_SeparatorHovered] = ImVec4(0.58f, 0.58f, 0.58f, 0.78f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(0.56f, 0.56f, 0.56f, 0.25f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.48f, 0.48f, 0.48f, 0.67f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.50f, 0.50f, 0.95f);
			colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
			colors[ImGuiCol_TabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.30f, 0.30f, 0.30f, 0.44f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
			colors[ImGuiCol_DockingPreview] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.98f, 0.37f, 0.17f);
			colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
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
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", 0, window_flags);
		ImGui::PopStyleVar();

		ImGui::PopStyleVar(2);

		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

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
			//StyleEditor();
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