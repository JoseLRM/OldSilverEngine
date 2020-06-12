#include "core.h"

#include "Graphics_dx11.h"
#include "Window.h"
#include "Scene.h"

#include "external/ImGui/imgui_impl_dx11.h"
#include "external/ImGui/imgui_impl_win32.h"
#include "ImGuiManager.h"

#ifdef SV_IMGUI

using namespace SV;

namespace SVImGui {

	ImGuiContext* g_ImGuiContext = nullptr;

	namespace _internal {
		bool Initialize(SV::Window& window, SV::Graphics& graphics)
		{
			SV::DirectX11Device& dx11 = *reinterpret_cast<SV::DirectX11Device*>(&graphics);

			IMGUI_CHECKVERSION();
			g_ImGuiContext = ImGui::CreateContext();

			if (g_ImGuiContext == nullptr) {
				SV::LogE("Can't create ImGui Context");
				return false;
			}

			if (!ImGui_ImplWin32_Init(GetDesktopWindow())) {
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
	
	void ShowGraphics(const SV::Graphics& graphics, bool* open)
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
								comp->ShowInfo(scene);
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

}

#endif