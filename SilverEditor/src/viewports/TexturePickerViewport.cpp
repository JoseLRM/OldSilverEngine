#include "core_editor.h"

#include "TexturePickerViewport.h"

namespace sve {
	
	TexturePickerViewport::TexturePickerViewport()
	{}

	bool TexturePickerViewport::getTextureName(const char** pName)
	{
		bool res = true;
		*pName = nullptr;

		if (ImGui::Begin("TexturePicker", 0, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar)) {

			auto& assets = sv::assets_registers_get();
			for (auto it = assets.begin(); it != assets.end(); ++it) {

				if (it->second.assetType == sv::AssetType_Texture) {
					if (ImGui::Button(it->first.c_str())) {

						*pName = it->first.c_str();

						res = false;
						break;
					}
				}
			}

			if (!ImGui::IsWindowFocused()) res = false;

		}

		ImGui::End();
		return res;
	}

}