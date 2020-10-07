#include "core_editor.h"

#include "viewports.h"
#include "viewports/viewport_materials.h"
#include "viewports/viewport_assets.h"
#include "asset_system.h"
#include "editor.h"

namespace sve {

	static sv::MaterialAsset g_SelectedMaterial;

	bool viewport_materials_display()
	{
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_MATERIALS))) {

			// Material selection
			const char* selectedName = g_SelectedMaterial.get_name();
			if (ImGui::BeginCombo("Select material", (selectedName == nullptr) ? "None" : selectedName)) {

				const std::map<std::string, sv::AssetRegister>& assetMap = sv::assets_registers_get();

				if (selectedName) if (ImGui::Button("None")) 
					g_SelectedMaterial.unload();

				for (auto& mat : assetMap) {
					if (mat.second.assetType == sv::AssetType_Material) {
						if (mat.first.c_str() == selectedName) continue;
						if (ImGui::Button(mat.first.c_str())) {
							g_SelectedMaterial.load(mat.first.c_str());
						}
					}
				}
				
				ImGui::EndCombo();
			}

			// Material inspector
			if (g_SelectedMaterial.get_material()) {

				bool serialize = false;

				sv::Material* mat = g_SelectedMaterial.get_material();
				sv::ShaderLibrary* lib = sv::matsys_material_shader_get(mat);
				const auto& attributes = sv::matsys_shaderlibrary_attributes_get(lib);

				for (const sv::ShaderAttribute& attr : attributes) {

					bool add = false;
					XMMATRIX data;
					sv::matsys_material_get(mat, attr.name.c_str(), attr.type , &data);

					switch (attr.type)
					{
					case sv::ShaderAttributeType_Float:
					{
						float& f = *reinterpret_cast<float*>(&data);
						add = ImGui::DragFloat(attr.name.c_str(), &f, 0.1f);
					}
						break;
					case sv::ShaderAttributeType_Float2:
					{
						sv::vec2f& v = *reinterpret_cast<sv::vec2f*>(&data);
						add = ImGui::DragFloat2(attr.name.c_str(), &v.x, 0.1f);
					}
						break;
					case sv::ShaderAttributeType_Float3:
					{
						sv::vec3f& v = *reinterpret_cast<sv::vec3f*>(&data);
						add = ImGui::DragFloat3(attr.name.c_str(), &v.x, 0.1f);
					}
						break;
					case sv::ShaderAttributeType_Float4:
					{
						sv::vec4f& v = *reinterpret_cast<sv::vec4f*>(&data);
						add = ImGui::DragFloat4(attr.name.c_str(), &v.x, 0.1f);
					}
						break;
					case sv::ShaderAttributeType_Half:
						break;
					case sv::ShaderAttributeType_Double:
						break;
					case sv::ShaderAttributeType_Boolean:
						break;
					case sv::ShaderAttributeType_UInt32:
					{
						ui32& ui = *reinterpret_cast<ui32*>(&data);
						int aux = ui;
						add = ImGui::DragInt(attr.name.c_str(), &aux, 1);
						if (aux < 0) aux = 0;
						ui = aux;
					}
						break;
					case sv::ShaderAttributeType_UInt64:
						break;
					case sv::ShaderAttributeType_Int32:
					{
						i32& ui = *reinterpret_cast<i32*>(&data);
						int aux = ui;
						add = ImGui::DragInt(attr.name.c_str(), &aux, 1);
						ui = aux;
					}
						break;
					case sv::ShaderAttributeType_Int64:
					{
						i64& ui = *reinterpret_cast<i64*>(&data);
						int aux = ui;
						add = ImGui::DragInt(attr.name.c_str(), &aux, 1);
						ui = aux;
					}
						break;
					case sv::ShaderAttributeType_Char:
					{
						char& c = *reinterpret_cast<char*>(&data);
						add = ImGui::InputText(attr.name.c_str(), &c, 1u);
					}
						break;
					case sv::ShaderAttributeType_Mat3:
						break;
					case sv::ShaderAttributeType_Mat4:
						break;
					case sv::ShaderAttributeType_Texture:
					{
						if (ImGui::TreeNodeEx(attr.name.c_str())) {

							static bool selectTexture = false;

							sv::TextureAsset tex;
							if (g_SelectedMaterial.get_texture(attr.name.c_str(), tex) == sv::Result_Success) {

								ImGuiDevice& device = editor_device_get();
								sv::GPUImage* img = tex.get_image();
								if (img) ImGui::Image(device.ParseImage(img), { 50.f, 50.f });
								if (ImGui::Button("Select")) {
									selectTexture = true;
								}

								if (selectTexture) {
									const char* name;
									selectTexture = viewport_assets_texture_popmenu(&name);

									if (name) {
										if (tex.load(name) == sv::Result_Success) {
											g_SelectedMaterial.set_texture(attr.name.c_str(), tex);
										}
									}
								}

							}

							ImGui::TreePop();
						}
					}
						break;
					default:
						break;
					}

					if (add) {
						sv::matsys_material_set(mat, attr.name.c_str(), attr.type, &data);
						serialize = true;
					}

				}

				if (serialize) {
					g_SelectedMaterial.serialize();
				}
			}

		}
		ImGui::End();

		return true;
	}

}