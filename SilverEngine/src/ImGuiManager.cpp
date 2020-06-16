#include "core.h"

#include "Graphics_dx11.h"
#include "Window.h"
#include "Scene.h"
#include "Input.h"
#include "Camera.h"
#include "Renderer.h"

#include "external/ImGui/imgui_impl_dx11.h"
#include "external/ImGui/imgui_impl_win32.h"
#include "ImGuiManager.h"

#ifdef SV_IMGUI

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace SV;

namespace SVImGui {

	ImGuiContext* g_ImGuiContext = nullptr;

	namespace _internal {
		bool Initialize(SV::Window& window, SV::Graphics& graphics)
		{
			ImGui_ImplWin32_EnableDpiAwareness();

			SV::DirectX11Device& dx11 = *reinterpret_cast<SV::DirectX11Device*>(&graphics);

			IMGUI_CHECKVERSION();
			g_ImGuiContext = ImGui::CreateContext();

			if (g_ImGuiContext == nullptr) {
				SV::LogE("Can't create ImGui Context");
				return false;
			}

			auto& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DpiEnableScaleViewports;;

			if (!ImGui_ImplWin32_Init(window.GetWindowHandle())) {
				SV::LogE("Can't initialize ImGui Windows Impl");
				Close();
				return false;
			}
			if (!ImGui_ImplDX11_Init(dx11.device.Get(), dx11.immediateContext.Get())) {
				SV::LogE("Can't initialize ImGui DirectX11 Impl");
				Close();
				return false;
			}

			return true;
		}
		void BeginFrame()
		{
			if (g_ImGuiContext == nullptr) return;

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
		}
		void EndFrame()
		{
			if (g_ImGuiContext == nullptr) return;

			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			ImGui::EndFrame();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		i64 UpdateWindowProc(void* handle, ui32 msg, i64 wParam, i64 lParam)
		{
			return ImGui_ImplWin32_WndProcHandler(reinterpret_cast<HWND>(handle), msg, wParam, lParam);
		}
		bool Close()
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext(g_ImGuiContext);
			return true;
		}
	}
	
	void ShowGraphics(SV::Graphics& graphics, bool* open)
	{
		if (ImGui::Begin("Graphics")) {

		}
		ImGui::End();
	}

	void ImGuiAddEntity(SV::Entity entity, SV::Scene& scene, SV::Entity& selectedEntity)
	{
		auto& entities = scene.GetEntityList();
		auto& entityData = scene.GetEntityDataList();

		EntityData& ed = entityData[entity];

		bool empty = ed.childsCount == 0;
		auto treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | (empty ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_AllowItemOverlap);

		bool active;
		NameComponent* nameComponent = (NameComponent*)scene.GetComponent(entity, NameComponent::ID);
		if (nameComponent) {
			active = ImGui::TreeNodeEx((nameComponent->GetName() + "[" + std::to_string(entity) + "]").c_str(), treeFlags);
		}
		else {
			active = ImGui::TreeNodeEx(("Entity[" + std::to_string(entity) + "]").c_str(), treeFlags);
		}

		if (ImGui::IsItemClicked()) {
			if (selectedEntity != entity) {
				selectedEntity = entity;
			}
		}
		if (active) {
			if (!empty) {
				for (ui32 i = 0; i < ed.childsCount; ++i) {
					Entity e = entities[ed.handleIndex + i + 1];
					i += entityData[e].childsCount;
					ImGuiAddEntity(e, scene, selectedEntity);
				}
				ImGui::TreePop();
			}
			else ImGui::TreePop();
		}
	}
	
	void ShowScene(SV::Scene& scene, SV::Entity* pSelectedEntity, bool* open)
	{
		bool result = true;
		SV::Entity& selectedEntity = *pSelectedEntity;

		auto& entities = scene.GetEntityList();
		auto& entityData = scene.GetEntityDataList();

		if (ImGui::Begin("Entities")) {

			ImGui::BeginChild("Entities");
			ImGui::Columns(2);
			{
				ImGui::BeginChild("Entities");

				for (size_t i = 1; i < entities.size(); ++i) {

					Entity entity = entities[i];
					EntityData& ed = entityData[entity];

					if (ed.parent == SV_INVALID_ENTITY) {
						ImGuiAddEntity(entity, scene, selectedEntity);
						i += ed.childsCount;
					}

				}

				if (selectedEntity < 0 || selectedEntity >= entityData.size()) selectedEntity = SV_INVALID_ENTITY;

				ImGui::EndChild();
			}

			ImGui::NextColumn();

			{
				ImGui::BeginChild("Components");

				if (selectedEntity != SV_INVALID_ENTITY) {

					NameComponent* nameComponent = (NameComponent*)scene.GetComponent(selectedEntity, NameComponent::ID);
					if (nameComponent) {
						ImGui::Text("%s[%u]", nameComponent->GetName().c_str(), selectedEntity);
					}
					else {
						ImGui::Text("Entity[%u]", selectedEntity);
					}

					if (ImGui::Button("Destroy")) {
						scene.DestroyEntity(selectedEntity);
						selectedEntity = SV_INVALID_ENTITY;
					}
					else {

						if (ImGui::BeginCombo("Add", "Add Component")) {

							for (ui16 ID = 0; ID < ECS::GetComponentsCount(); ++ID) {
								const char* NAME = ECS::GetComponentName(ID);
								if (scene.GetComponent(selectedEntity, ID) != nullptr) continue;
								size_t SIZE = ECS::GetComponentSize(ID);
								if (ImGui::Button(NAME)) {
									scene.AddComponent(selectedEntity, ID, SIZE);
								}
							}

							ImGui::EndCombo();
						}
						if (ImGui::BeginCombo("Rmv", "Remove Component")) {

							EntityData& ed = entityData[selectedEntity];
							for (auto& it : ed.indices) {
								ui16 ID = it.first;
								const char* NAME = ECS::GetComponentName(ID);
								size_t SIZE = ECS::GetComponentSize(ID);
								if (ImGui::Button(NAME)) {
									scene.RemoveComponent(selectedEntity, ID, SIZE);
									break;
								}
							}

							ImGui::EndCombo();
						}
						if (ImGui::Button("Duplicate")) {
							scene.DuplicateEntity(selectedEntity);
						}
						if (ImGui::Button("Create Child")) {
							scene.CreateEntity(selectedEntity);
						}

						EntityData& ed = entityData[selectedEntity];

						// Show Transform Data
						SV::Transform& trans = ed.transform;

						SV::vec3 position = trans.GetLocalPosition();
						SV::vec3 rotation = ToDegrees(trans.GetLocalRotation());
						SV::vec3 scale = trans.GetLocalScale();

						ImGui::DragFloat3("Position", &position.x, 1.f);
						ImGui::DragFloat3("Rotation", &rotation.x, 0.1f);
						ImGui::DragFloat3("Scale", &scale.x, 1.f);

						trans.SetPosition(position);
						trans.SetRotation(ToRadians(rotation));
						trans.SetScale(scale);

						ImGui::Separator();
						ImGui::Separator();

						ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

						for (auto& it : ed.indices) {

							ui16 compID = it.first;
							size_t index = it.second;

							std::vector<ui8>& componentList = scene.GetComponentsList(compID);
							BaseComponent* comp = (BaseComponent*)(&componentList[index]);

							ImGui::Separator();
							bool remove = false;
							if (ImGui::TreeNodeEx(ECS::GetComponentName(compID), flags)) {
								comp->ShowInfo();
								ImGui::TreePop();
							}

							if (remove) {
								scene.RemoveComponent(selectedEntity, compID, ECS::GetComponentSize(compID));
								break;
							}
							ImGui::Separator();
						}
					}
				}

				ImGui::EndChild();
			}

			ImGui::NextColumn();

			ImGui::EndChild();
			if (ImGui::Button("Create")) {
				scene.CreateEntity();
			}

		}
		ImGui::End();
		if(open) *open = result;
	}

	void ShowTileMapEditor(SV::TileMapComponent* tileMapComp, SV::Input& input, SV::OrthographicCamera& camera, RenderQueue2D& rq, RenderLayer& layer)
	{
		SV::TileMap& tileMap = *tileMapComp->tileMap.get();

		if (ImGui::Begin("TileMapEditor")) {

			// Info
			ImGui::Text("Width: %u", tileMap.GetWidth());
			ImGui::Text("Height: %u", tileMap.GetHeight());

			ImGui::Separator();

			// Resize grid
			ImGui::Text("New Size");
			static int width, height;
			ImGui::DragInt("Width", &width, 1, 0);
			ImGui::DragInt("Height", &height, 1, 0);
			if (ImGui::Button("Create Grid")) tileMap.CreateGrid(width, height);

			ImGui::Separator();
			
			// Tile Selection
			ui32 tilesCount = tileMap.GetTilesCount();
			for (ui32 i = 0; i < tilesCount; ++i) {
				SV::Sprite spr = tileMap.GetSprite(i);
				SV::vec4 texCoords = spr.pTextureAtlas->GetSpriteTexCoords(spr.ID);
				ImGui::Text("Tile0: %f,%f,%f,%f", texCoords.x, texCoords.y, texCoords.z, texCoords.w);
			}

			static SV::vec4 newTileCoords = { 0.f, 0.f, 1.f, 1.f };
			ImGui::DragFloat("X coord", &newTileCoords.x, 0.001f);
			ImGui::DragFloat("Y coord", &newTileCoords.y, 0.001f);
			ImGui::DragFloat("Spr Width", &newTileCoords.z, 0.001f);
			ImGui::DragFloat("Spr Height", &newTileCoords.w, 0.001f);
			if (ImGui::Button("Create Tile")) {
				Sprite spr;
				spr.pTextureAtlas = tileMap.GetSprite(0).pTextureAtlas;
				tileMap.CreateTile(spr.pTextureAtlas->CreateSprite(newTileCoords.x, newTileCoords.y, newTileCoords.z, newTileCoords.w));
			}

			// Edit mode
			static bool editMode = false;
			static SV::vec2 mousePressed;
			static bool bigSelection = false;
			static bool createMode = true;
			ImGui::Checkbox("Edit Mode", &editMode);

			if (editMode) {
				ui32 gridX, gridY;

				SV::vec2 mousePos = camera.GetMousePos(input);

				if (input.IsMousePressed(SV_MOUSE_LEFT) || input.IsMousePressed(SV_MOUSE_RIGHT)) {
					mousePressed = mousePos;
					if (input.IsMousePressed(SV_MOUSE_LEFT)) createMode = true;
					else createMode = false;
				}

				if (input.IsMouse(SV_MOUSE_LEFT) || input.IsMouse(SV_MOUSE_RIGHT)) {
					float dragged = (mousePos - mousePressed).Mag();
					dragged /= camera.GetDimension().Mag();

					// Big Selection
					if (dragged >= 0.01f) {
						bigSelection = true;
					}
				}

				if (input.IsMouseReleased(SV_MOUSE_LEFT) || input.IsMouseReleased(SV_MOUSE_RIGHT)) {

					if (bigSelection) {

						SV::ivec2 pos0 = tileMapComp->GetTilePos(mousePressed);
						SV::ivec2 pos1 = tileMapComp->GetTilePos(mousePos);

						if (pos0.x > pos1.x) {
							i32 aux = pos0.x;
							pos0.x = pos1.x;
							pos1.x = aux;
						}
						if (pos0.y > pos1.y) {
							i32 aux = pos0.y;
							pos0.y = pos1.y;
							pos1.y = aux;
						}

						for (i32 x = pos0.x; x <= pos1.x; ++x) {
							for (i32 y = pos0.y; y <= pos1.y; ++y) {
								if (!tileMap.InBounds(x, y)) continue;
								if (createMode) tileMap.PutTile(x, y, 1);
								else tileMap.PutTile(x, y, SV_NULL_TILE);
							}
						}

					}
					else {
						SV::ivec2 pos = tileMapComp->GetTilePos(mousePos);

						if (tileMap.InBounds(pos.x, pos.y)) {
							if(createMode) tileMap.PutTile(pos.x, pos.y, 1);
							else tileMap.PutTile(pos.x, pos.y, SV_NULL_TILE);
						}
					}

					bigSelection = false;
				}

				//Draw
				if (bigSelection) {
					SV::vec2 size = mousePos - mousePressed;
					SV::vec2 pos = mousePressed + (size / 2.f);
					size.x = abs(size.x);
					size.y = abs(size.y);
					SV::Color4b color = {0u, 150, 220, 150};
					rq.DrawQuad(pos, size, color, layer);
				}

			}

		}
		ImGui::End();
	}

}

#endif