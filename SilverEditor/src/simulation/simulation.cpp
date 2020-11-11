#include "core_editor.h"

#include "panel_manager.h"
#include "simulation.h"
#include "simulation/scene.h"
#include "platform/window.h"
#include "simulation/animator.h"
#include "gui.h"
#include "platform/input.h"
#include "simulation_editor.h"

namespace sv {

	static bool g_Running = false;
	static bool g_Paused = false;
	static bool g_Gamemode = false;

	static bool g_RunningRequest = false;
	static bool g_StopRequest = false;

	Scene g_Scene;
	static std::string g_ScenePath;

	DebugCamera g_DebugCamera;

	vec2f g_SimulationMousePos;
	bool g_SimulationMouseInCamera;
	static vec4f g_ScreenBounds = { 0.f, 0.f, 1.f, 1.f };
	static bool g_ShowDebug = true;

	Entity g_SelectedEntity = SV_ENTITY_NULL;

	Result simulation_initialize(const char* sceneFilePath)
	{
		g_Scene.create();

		if (g_Scene.deserialize(sceneFilePath) != Result_Success) {
			g_Scene.destroy();
			g_Scene.create();
		}

		g_ScenePath = sceneFilePath;
		engine_animations_disable();

		// Create debug camera
		{
			g_DebugCamera.position = { 0.f, 0.f, -10.f };
			svCheck(g_DebugCamera.camera.setResolution(1920u, 1080u));
			g_DebugCamera.camera.setProjectionType(ProjectionType_Orthographic);
			g_DebugCamera.camera.setWidth(10.f);
			g_DebugCamera.camera.setHeight(10.f);
		}

		svCheck(simulation_editor_initialize());

		return Result_Success;
	}

	Result simulation_close()
	{
		svCheck(simulation_editor_close());
		g_DebugCamera.camera.clear();
		g_Scene.destroy();
		return Result_Success;
	}

	void simulation_update(float dt)
	{
		// Compute Mouse Position
		if (g_Gamemode) {
			g_SimulationMousePos = input_mouse_position_get();
			g_SimulationMouseInCamera = true;
		}
		else {

			g_SimulationMousePos = input_mouse_position_get();
			
			vec2f winSize = vec2f(float(window_size_get().x), float(window_size_get().y));
			vec2f screenPos = vec2f{ g_ScreenBounds.x, g_ScreenBounds.y };

			g_SimulationMousePos += { 0.5f, 0.5f };
			g_SimulationMousePos *= winSize;

			g_SimulationMousePos -= screenPos;
			g_SimulationMousePos /= vec2f{ g_ScreenBounds.z, g_ScreenBounds.w };
			g_SimulationMousePos -= { 0.5f, 0.5f };

			g_SimulationMouseInCamera = g_SimulationMousePos.x > -0.5f && g_SimulationMousePos.x < 0.5f && g_SimulationMousePos.y > -0.5f && g_SimulationMousePos.y < 0.5f;

			if (g_ShowDebug) {
				g_SimulationMousePos *= { g_DebugCamera.camera.getWidth(), g_DebugCamera.camera.getHeight() };
				g_SimulationMousePos += g_DebugCamera.position.getVec2();
			}
			else {
				// TODO
			}
		}

		// RUN - STOP Simulation
		{
			if (g_RunningRequest) {
				g_Running = true;
				g_Paused = false;
				gui_style_simulation();

				g_Scene.serialize(g_ScenePath.c_str());
				g_RunningRequest = false;
			}

			if (g_StopRequest) {
				g_Running = false;
				g_Paused = false;
				gui_style_editor();

				g_Scene.deserialize(g_ScenePath.c_str());
				g_StopRequest = false;
			}
		}

		// Adjust cameras
		{
			if (g_Scene.getMainCamera() != SV_ENTITY_NULL) {
				CameraComponent* camera = ecs_component_get<CameraComponent>(g_Scene, g_Scene.getMainCamera());
				if (camera) {
					vec2u panelSize;
					panelSize.x = g_ScreenBounds.z;
					panelSize.y = g_ScreenBounds.w;

					vec2u size = g_Gamemode ? window_size_get() : panelSize;
					camera->camera.adjust(size.x, size.y);
				}
			}

			if (g_ShowDebug) {
				g_DebugCamera.camera.adjust(g_ScreenBounds.z / g_ScreenBounds.w);
			}
		}

		// Debug
		if (g_ShowDebug) {
			simulation_editor_camera_controller_2D(g_SimulationMousePos, dt);
			simulation_editor_update_action(dt);
		}

		if (g_Running && !g_Paused) {

			g_Scene.physicsSimulate(dt);

		}
	}

	void simulation_render()
	{
		if (g_ShowDebug) {
			simulation_editor_render();
		}
		else {
			g_Scene.draw(g_Gamemode);
		}
	}

	void simulation_display()
	{
		if (ImGui::Begin("Simulation")) {

			ImGui::Columns(2);

			if (g_Running) {

				if (ImGui::Button("Stop")) simulation_stop();
				ImGui::SameLine();

				if (g_Paused) {
					if (ImGui::Button("Continue")) simulation_continue();
				}
				else if (ImGui::Button("Pause")) simulation_pause();

			}
			else {
				if (ImGui::Button("Start")) simulation_run();
			}

			ImGui::NextColumn();

			ImGui::Checkbox("Debug", &g_ShowDebug);

			const ImVec4 transformButtonColor(0.2f, 0.8f, 0.2f, 1.f);

			ImGui::Separator();
			ImGui::NextColumn();

			// Rotation Button
			{
				if (g_TransformMode == TransformMode_Rotation) {
					ImGui::PushStyleColor(ImGuiCol_Button, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transformButtonColor);
				}

				

				if (ImGui::Button("R", { 20.f, 20.f })) {
					g_TransformMode = TransformMode_Rotation;
				}
				else if (g_TransformMode == TransformMode_Rotation) ImGui::PopStyleColor(3);

				ImGui::SameLine(0.f, 0.f);
			}

			// Scale Button
			{
				if (g_TransformMode == TransformMode_Scale) {
					ImGui::PushStyleColor(ImGuiCol_Button, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, transformButtonColor);
				}

				if (ImGui::Button("S", { 20.f, 20.f })) {
					g_TransformMode = TransformMode_Scale;
				}
				else if (g_TransformMode == TransformMode_Scale) ImGui::PopStyleColor(3);

				ImGui::SameLine(0.f, 0.f);
			}

			// Translation Button
			{
				if (g_TransformMode == TransformMode_Translation) {
					ImGui::PushStyleColor(ImGuiCol_Button, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transformButtonColor);
				}

				if (ImGui::Button("T", { 20.f, 20.f })) {
					g_TransformMode = TransformMode_Translation;
				}
				else if (g_TransformMode == TransformMode_Translation) ImGui::PopStyleColor(3);
			}

			ImGui::NextColumn();

			ImGui::Checkbox("Collisions", &g_DebugRendering_Collisions);
			ImGui::SameLine();
			ImGui::Checkbox("Grid", &g_DebugRendering_Grid);

			ImGui::Columns(1);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });

			ImVec2 size = ImGui::GetContentRegionAvail();
			g_ScreenBounds.z = size.x;
			g_ScreenBounds.w = size.y;

			if (g_ShowDebug) {

				ImGui::Image(gui_image_parse(g_DebugCamera.camera.getOffscreenRT()), size);

			}
			else {

				Entity entityCamera = g_Scene.getMainCamera();
				if (ecs_entity_exist(g_Scene, entityCamera)) {
					
					CameraComponent* camComp = ecs_component_get<CameraComponent>(g_Scene, entityCamera);
					if (camComp) {
						ImGui::Image(gui_image_parse(camComp->camera.getOffscreenRT()), size);
					}
				}
			}

			ImVec2 screenPos = ImGui::GetItemRectMin();
			g_ScreenBounds.x = screenPos.x;
			g_ScreenBounds.y = screenPos.y;

			ImGui::PopStyleVar();

		}
		ImGui::End();
	}

	void simulation_run()
	{
		if (g_Running) return;
		g_RunningRequest = true;
		engine_animations_enable();
	}

	void simulation_continue()
	{
		if (!g_Running || !g_Paused) return;
	
		g_Paused = false;
		gui_style_simulation();
		engine_animations_enable();
	}

	void simulation_pause()
	{
		if (!g_Running || g_Paused) return;
		
		g_Paused = true;
		gui_style_editor();
		engine_animations_disable();
	}

	void simulation_stop()
	{
		if (!g_Running) return;
		g_StopRequest = true;
		engine_animations_disable();
	}

	void simulation_gamemode_set(bool gamemode)
	{
		if (gamemode) {
			if (g_Running && !g_Paused) {
				g_Gamemode = true;
				window_style_set(WindowStyle_Fullscreen);
				g_ShowDebug = false;
			}
		}
		else {
			g_Gamemode = false;
			window_style_set(WindowStyle_Default);
			g_ShowDebug = false;
		}
	}

	bool simulation_running()
	{
		return g_Running;
	}

	bool simulation_paused()
	{
		return g_Paused;
	}

	bool simulation_gamemode_get()
	{
		return g_Gamemode;
	}

	DebugCamera& simulation_debug_camera_get()
	{
		return g_DebugCamera;
	}

}