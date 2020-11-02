#include "core_editor.h"

#include "platform/input.h"
#include "editor.h"

#include "imGuiDevice/ImGuiDevice.h"
#include "scene_editor.h"
#include "simulation.h"
#include "panel_manager.h"
#include "gui.h"

namespace sv {
	
	static std::unique_ptr<ImGuiDevice> g_Device;

	void editor_run()
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
			return;
		}

		g_Device = device_create();
		g_Device->Initialize();

		gui_initialize();

		while (true) {
			
			if (engine_loop() != Result_Success) break;

		}

		gui_close();
		g_Device->Close();

		engine_close();
	}

	void StyleEditor()
	{
		ImGui::ShowStyleEditor();

		if (input_key_pressed('S')) {

			std::ofstream file;

			file.open("ImGuiStyle", std::ios::binary);

			file.seekp(0u);
			file.write((const char*)&ImGui::GetStyle(), sizeof(ImGuiStyle));

			file.close();

			SV_LOG_INFO("Saved");
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

		auto& style = ImGui::GetStyle();
		float minWidth = style.WindowMinSize.x;
		style.WindowMinSize.x = 300.f;

		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		style.WindowMinSize.x = minWidth;

		ImGui::End();
	}

	Result editor_initialize()
	{
		svCheck(simulation_initialize("assets/scenes/Test.scene"));
		svCheck(scene_editor_initialize());
		svCheck(panel_manager_initialize());

		return Result_Success;
	}

	void editor_update(float dt)
	{
		simulation_update(dt);
		scene_editor_update(dt);

		if (input_key_pressed(SV_KEY_F11)) {
			simulation_gamemode_set(!simulation_gamemode_get());
		}
		if (input_key(SV_KEY_CONTROL) && input_key_pressed('S')) {
			Scene& scene = simulation_scene_get();
			Result res = scene.serialize("assets/scenes/Test.scene");
			SV_ASSERT(result_okay(res));
		}

		g_Device->ResizeSwapChain();

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
		static ui32 fpsCount = 0u;
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
			
			scene_editor_render();

			g_Device->BeginFrame();

			GPUImage* simulationOffscreen = nullptr;
			GPUImage* editorOffscreen = scene_editor_camera_get().camera.getOffscreenRT();

			Scene& scene = simulation_scene_get();
			Entity cameraEntity = scene.getMainCamera();

			if (ecs_entity_exist(scene, cameraEntity)) {

				CameraComponent* camera = ecs_component_get<CameraComponent>(scene, cameraEntity);
				if (camera) {
					simulationOffscreen = camera->camera.getOffscreenRT();
				}
			}

			GPUBarrier barriers[2];
			barriers[0] = GPUBarrier::Image(editorOffscreen, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
			if (simulationOffscreen) barriers[1] = GPUBarrier::Image(simulationOffscreen, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
			graphics_barrier(barriers, simulationOffscreen ? 2u : 1u, g_Device->GetCMD());
			
			gui_begin();
			DisplayDocking();
			//StyleEditor();
			panel_manager_display();
			gui_end();

			g_Device->EndFrame();

			barriers[0] = GPUBarrier::Image(editorOffscreen, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
			if (simulationOffscreen) barriers[1] = GPUBarrier::Image(simulationOffscreen, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
			graphics_barrier(barriers, simulationOffscreen ? 2u : 1u, g_Device->GetCMD());
		}
	}
	Result editor_close()
	{
		svCheck(simulation_close());
		svCheck(scene_editor_close());
		svCheck(panel_manager_close());

		return Result_Success;
	}

	ImGuiDevice& editor_device_get()
	{
		return *g_Device.get();
	}

}