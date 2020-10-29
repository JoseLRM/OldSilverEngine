#include "core_editor.h"

#include "MaterialPanel.h"
#include "editor.h"
#include "gui.h"

namespace sv {
	
	MaterialPanel::MaterialPanel() : Panel("Material")
	{
	}

	bool MaterialPanel::onDisplay()
	{
		// Material selection
		const char* selectedName = m_SelectedMaterial.getFilePath();

		AssetType matType = asset_type_get("Material");

		if (ImGui::BeginCombo("Select material", (selectedName == nullptr) ? "None" : selectedName)) {

			const std::unordered_map<std::string, AssetFile>& assetMap = asset_map_get();
			
			if (selectedName) if (ImGui::Button("None"))
				m_SelectedMaterial.unload();
			
			for (auto& mat : assetMap) {
				if (mat.second.assetType == matType) {
					if (mat.first.c_str() == selectedName) continue;
					if (ImGui::Button(mat.first.c_str())) {
						m_SelectedMaterial.load(mat.first.c_str());
					}
				}
			}

			ImGui::EndCombo();
		}

		// Material inspector

		if (m_SelectedMaterial.hasReference()) {
		
			bool serialize = false;

			Material& mat = *m_SelectedMaterial.get();
			ShaderLibrary& lib = *mat.getShaderLibrary();
			const auto& attributes = lib.getAttributes();

			gui_component_item_begin();
			
			for (const ShaderLibraryAttribute& attr : attributes) {
			
				ImGui::PushID(attr.name.c_str());

				bool add = false;
				XMMATRIX data;
				mat.getRaw(attr.name.c_str(), attr.type, &data);

				gui_component_item_next(attr.name.c_str());
			
				switch (attr.type)
				{
				case ShaderAttributeType_Float:
				{
					float& f = *reinterpret_cast<float*>(&data);
					add = ImGui::DragFloat("##value", &f, 0.1f);
				}
				break;
				case ShaderAttributeType_Float2:
				{
					vec2f& v = *reinterpret_cast<vec2f*>(&data);
					add = ImGui::DragFloat2("##value", &v.x, 0.1f);
				}
				break;
				case ShaderAttributeType_Float3:
				{
					vec3f& v = *reinterpret_cast<vec3f*>(&data);
					add = ImGui::DragFloat3("##value", &v.x, 0.1f);
				}
				break;
				case ShaderAttributeType_Float4:
				{
					vec4f& v = *reinterpret_cast<vec4f*>(&data);
					add = ImGui::DragFloat4("##value", &v.x, 0.1f);
				}
				break;
				case ShaderAttributeType_Half:
					break;
				case ShaderAttributeType_Double:
				{
					double& d = *reinterpret_cast<double*>(&data);
					float f = float(d);
					add = ImGui::DragFloat("##value", &f, 0.1f);
					d = double(f);
				}
					break;
				case ShaderAttributeType_Boolean:
					break;
				case ShaderAttributeType_UInt32:
				{
					ui32& ui = *reinterpret_cast<ui32*>(&data);
					int aux = ui;
					add = ImGui::DragInt("##value", &aux, 1);
					if (aux < 0) aux = 0;
					ui = aux;
				}
				break;
				case ShaderAttributeType_UInt64:
					break;
				case ShaderAttributeType_Int32:
				{
					i32& ui = *reinterpret_cast<i32*>(&data);
					int aux = ui;
					add = ImGui::DragInt("##value", &aux, 1);
					ui = aux;
				}
				break;
				case ShaderAttributeType_Int64:
				{
					i64& ui = *reinterpret_cast<i64*>(&data);
					int aux = int(ui);
					add = ImGui::DragInt("##value", &aux, 1);
					ui = i64(aux);
				}
				break;
				case ShaderAttributeType_Char:
				{
					char& c = *reinterpret_cast<char*>(&data);
					add = ImGui::InputText("##value", &c, 1u);
				}
				break;
				case ShaderAttributeType_Mat3:
					break;
				case ShaderAttributeType_Mat4:
				{
					XMMATRIX& f = *reinterpret_cast<XMMATRIX*>(&data);
					add = gui_component_item_mat4(f);
				}
					break;
				case ShaderAttributeType_Other:
				{
					TextureAsset tex;
					m_SelectedMaterial->getTexture(attr.name.c_str(), tex);
					if (gui_component_item_texture(tex)) {
						m_SelectedMaterial->setTexture(attr.name.c_str(), tex);
					}
				}
				break;
				default:
					break;
				}
			
				if (add) {
					mat.setRaw(attr.name.c_str(), attr.type, &data);
					serialize = true;
				}

				ImGui::PopID();
			
			}

			if (serialize) {
				m_SelectedMaterial.serialize();
			}

			gui_component_item_end();
		}

		return true;
	}
}