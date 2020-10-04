#include "core_editor.h"

#include "viewports.h"
#include "viewports/viewport_materials.h"
#include "asset_system.h"

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

				sv::Material* mat = g_SelectedMaterial.get_material();
				sv::ShaderLibrary* lib = sv::matsys_material_shader_get(mat);
				const auto& attributes = sv::matsys_shaderlibrary_attributes_get(lib);

				for (const sv::ShaderAttribute& attr : attributes) {

					switch (attr.type)
					{
					case sv::ShaderAttributeType_Float:
					{
						float f = 0.f;
						sv::matsys_material_get(mat, attr.name.c_str(), &f, attr.type);
						if (ImGui::DragFloat(attr.name.c_str(), &f, 0.1f)) {
							sv::matsys_material_set(mat, attr.name.c_str(), &f, attr.type);
						}
					}
						break;
					case sv::ShaderAttributeType_Float2:
						break;
					case sv::ShaderAttributeType_Float3:
						break;
					case sv::ShaderAttributeType_Float4:
					{
						sv::vec4f f = 0.f;
						sv::matsys_material_get(mat, attr.name.c_str(), &f, attr.type);
						if (ImGui::DragFloat4(attr.name.c_str(), &f.x, 0.1f)) {
							sv::matsys_material_set(mat, attr.name.c_str(), &f, attr.type);
						}
					}
						break;
					case sv::ShaderAttributeType_Half:
						break;
					case sv::ShaderAttributeType_Double:
						break;
					case sv::ShaderAttributeType_Boolean:
					{
						bool f = false;
						sv::matsys_material_get(mat, attr.name.c_str(), &f, attr.type);
						if (ImGui::Checkbox(attr.name.c_str(), &f)) {
							sv::matsys_material_set(mat, attr.name.c_str(), &f, attr.type);
						}
					}
						break;
					case sv::ShaderAttributeType_UInt32:
					{
						ui32 f0 = false;
						sv::matsys_material_get(mat, attr.name.c_str(), &f0, attr.type);
						int f = f0;
						if (ImGui::DragInt(attr.name.c_str(), &f)) {
							f0 = f;
							sv::matsys_material_set(mat, attr.name.c_str(), &f0, attr.type);
						}
					}
						break;
					case sv::ShaderAttributeType_UInt64:
						break;
					case sv::ShaderAttributeType_Int32:
						break;
					case sv::ShaderAttributeType_Int64:
						break;
					case sv::ShaderAttributeType_Char:
						break;
					case sv::ShaderAttributeType_Mat3:
						break;
					case sv::ShaderAttributeType_Mat4:
						break;
					case sv::ShaderAttributeType_Texture:
						break;
					default:
						break;
					}

				}
			}

		}
		ImGui::End();

		return true;
	}

}