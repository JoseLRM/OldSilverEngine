#include "core_editor.h"

#include "panel_manager.h"
#include "simulation.h"
#include "simulation/scene.h"
#include "platform/window.h"
#include "gui.h"
#include "platform/input.h"
#include "simulation_editor.h"

namespace sv {

	static bool g_Running = false;
	static bool g_Paused = false;
	static bool g_Gamemode = false;

	static bool g_RunningRequest = false;
	static bool g_StopRequest = false;

	SceneAsset g_Scene;

	DebugCamera g_DebugCamera;

	vec2f g_SimulationMousePos;
	bool g_SimulationMouseInCamera;
	static vec4f g_ScreenBounds = { 0.f, 0.f, 1.f, 1.f };
	static bool g_ShowDebug = true;

	Entity g_SelectedEntity = SV_ENTITY_NULL;

	Result simulation_initialize()
	{
		const char* sceneFilePath = "scenes/Test.scene";

		if (g_Scene.load(sceneFilePath) != Result_Success) {
			g_Scene.createFile("assets/scenes/Test.scene");
			g_Scene.load(sceneFilePath);
		}

		simulation_scene_open(sceneFilePath);
			
		engine_animations_disable();

		svCheck(simulation_editor_initialize());

		return Result_Success;
	}

	Result simulation_close()
	{
		svCheck(simulation_editor_close());
		g_DebugCamera.camera.clear();
		g_Scene.unload();
		return Result_Success;
	}

	void simulation_update(float dt)
	{
		if (!ecs_entity_exist(*g_Scene.get(), g_SelectedEntity)) g_SelectedEntity = SV_ENTITY_NULL;

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
			static size_t hash = hash_string("Scene aux");

			if (g_RunningRequest) {
				simulation_scene_save();

				g_Running = true;
				g_Paused = false;
				gui_style_simulation();

				ArchiveO file;
				file << g_Scene.getHashCode();
				bin_write(hash, file);
				
				g_RunningRequest = false;
			}

			if (g_StopRequest) {
				g_Running = false;
				g_Paused = false;
				gui_style_editor();

				g_SelectedEntity = SV_ENTITY_NULL;

				ArchiveI file;
				size_t sceneHashCode;
				if (result_okay(bin_read(hash, file))) {
					
					file >> sceneHashCode;
					g_Scene.unload();

					asset_free_unused(asset_type_get("Scene"));

					simulation_scene_open(asset_filepath_get(sceneHashCode));
				}
				
				g_StopRequest = false;
			}
		}

		Scene& scene = *g_Scene.get();

		// Adjust cameras
		{
			if (scene.getMainCamera() != SV_ENTITY_NULL) {
				CameraComponent* camera = ecs_component_get<CameraComponent>(scene, scene.getMainCamera());
				if (camera) {
					vec2u panelSize;
					panelSize.x = ui32(g_ScreenBounds.z);
					panelSize.y = ui32(g_ScreenBounds.w);

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
			if (g_DebugCamera.camera.getCameraType() == CameraType_2D)
				simulation_editor_camera_controller_2D(dt);
			else 
				simulation_editor_camera_controller_3D(dt);

			simulation_editor_update_action(dt);
		}

		// Control keys
		{
			if (input_key(SV_KEY_CONTROL)) {

				if (g_SelectedEntity != SV_ENTITY_NULL && input_key_pressed('D')) {
					ecs_entity_duplicate(scene, g_SelectedEntity);
				}

			}

			if (g_SelectedEntity != SV_ENTITY_NULL && input_key_pressed(SV_KEY_DELETE)) {
				ecs_entity_destroy(scene, g_SelectedEntity);
				g_SelectedEntity = SV_ENTITY_NULL;
			}
		}

		if (g_Running && !g_Paused) {

			scene.physicsSimulate(dt);

		}
	}

	void simulation_render()
	{
		if (g_ShowDebug) {
			simulation_editor_render();
		}
		else {
			g_Scene->draw(g_Gamemode);
		}
	}

	void simulation_display()
	{
		Scene& scene = *g_Scene.get();

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

			ImGui::SameLine();

			bool mode3D = g_DebugCamera.camera.getCameraType() == CameraType_3D;

			if (ImGui::Checkbox("3D", &mode3D)) {

				if (mode3D) {
					g_DebugCamera.position = { 0.f, 0.f, -10.f };
					g_DebugCamera.camera.setCameraType(CameraType_3D);
				}
				else {
					g_DebugCamera.position = { 0.f, 0.f, -10.f };
					g_DebugCamera.camera.setCameraType(CameraType_2D);
				}
			}

			const ImVec4 transformButtonColor(0.2f, 0.8f, 0.2f, 1.f);

			ImGui::Separator();
			ImGui::NextColumn();

			// Rotation Button
			{
				bool pop;

				if (g_TransformMode == TransformMode_Rotation) {
					ImGui::PushStyleColor(ImGuiCol_Button, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transformButtonColor);

					pop = true;
				}
				else pop = false;
				

				if (ImGui::Button("R", { 20.f, 20.f })) {
					g_TransformMode = TransformMode_Rotation;
				}
				
				if (pop) ImGui::PopStyleColor(3);

				ImGui::SameLine(0.f, 0.f);
			}

			// Scale Button
			{
				bool pop;

				if (g_TransformMode == TransformMode_Scale) {
					ImGui::PushStyleColor(ImGuiCol_Button, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, transformButtonColor);

					pop = true;
				}
				else pop = false;

				if (ImGui::Button("S", { 20.f, 20.f })) {
					g_TransformMode = TransformMode_Scale;
				}
				
				if (pop) ImGui::PopStyleColor(3);

				ImGui::SameLine(0.f, 0.f);
			}

			// Translation Button
			{
				bool pop;

				if (g_TransformMode == TransformMode_Translation) {
					ImGui::PushStyleColor(ImGuiCol_Button, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, transformButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transformButtonColor);

					pop = true;
				}
				else pop = false;

				if (ImGui::Button("T", { 20.f, 20.f })) {
					g_TransformMode = TransformMode_Translation;
				}
				
				if (pop) ImGui::PopStyleColor(3);
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

				Entity entityCamera = scene.getMainCamera();
				if (ecs_entity_exist(scene, entityCamera)) {
					
					CameraComponent* camComp = ecs_component_get<CameraComponent>(scene, entityCamera);
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

	Result simulation_scene_save()
	{
		if (g_Running) return Result_Success;
		return g_Scene.save();
	}

	Result simulation_scene_open(const char* filePath)
	{
		g_SelectedEntity = SV_ENTITY_NULL;
		g_Scene.unload();
		asset_free_unused(asset_type_get("Scene"));
		svCheck(g_Scene.load(filePath));
		
		// Create debug camera

		g_DebugCamera.position = { 0.f, 0.f, -10.f };
		svCheck(g_DebugCamera.camera.setResolution(1920u, 1080u));
		g_DebugCamera.camera.setCameraType(CameraType_2D);

		return Result_Success;
	}

	Result simulation_scene_new(const char* filePath)
	{
		g_Scene.createFile(filePath);
		g_SelectedEntity = SV_ENTITY_NULL;
		return g_Scene.load(filePath);
	}

	DebugCamera& simulation_debug_camera_get()
	{
		return g_DebugCamera;
	}

}