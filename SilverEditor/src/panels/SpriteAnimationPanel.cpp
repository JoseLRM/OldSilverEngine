#include "core_editor.h"

#include "SpriteAnimationPanel.h"
#include "gui.h"

namespace sv {
	
	SpriteAnimationPanel::SpriteAnimationPanel() : Panel("Sprite Animation")
	{}

	SpriteAnimationPanel::~SpriteAnimationPanel()
	{
		m_SelectedAnimation.serialize();
	}

	bool SpriteAnimationPanel::onDisplay()
	{
		const char* selectedAnimationStr = m_SelectedAnimation.getFilePath();
		if (ImGui::BeginCombo("Select Animation", selectedAnimationStr ? selectedAnimationStr : "None")) {

			if (m_SelectedAnimation.hasReference()) {
				if (ImGui::MenuItem("None")) {
					m_SelectedAnimation.unload();
					m_SelectedAnimation.serialize();
				}
			}

			auto& assets = asset_map_get();

			AssetType sprType = asset_type_get("Sprite Animation");

			for (auto& asset : assets) {
				
				if (asset.second.assetType != sprType) continue;
				if (selectedAnimationStr && strcmp(asset.first.c_str(), selectedAnimationStr) == 0) continue;

				if (ImGui::MenuItem(asset.first.c_str())) {
					m_SelectedAnimation.serialize();
					m_SelectedAnimation.load(asset.first.c_str());
				}
			}

			ImGui::EndCombo();
		}


		// Edit Animation

		if (m_SelectedAnimation.hasReference()) {

			std::vector<Sprite>& sprites = m_SelectedAnimation.get()->sprites;

			ImGui::Separator();

			for (auto it = sprites.begin(); it != sprites.end(); ) {

				ImGui::PushID(&(*it));

				gui_component_item_begin();

				gui_component_item_next("Texture");
				gui_component_item_texture(it->texture);

				gui_component_item_next("Coords");
				ImGui::DragFloat4("##TexCoord", &it->texCoord.x, 0.01f);

				gui_component_item_end();

				if (ImGui::Button("Remove")) {
					it = sprites.erase(it);
				}
				else {
					++it;
				}

				ImGui::PopID();

				ImGui::Separator();

			}


			if (ImGui::Button("Add Sprite") || sprites.empty()) {
				sprites.emplace_back();
			}

		}

		return true;
	}

}