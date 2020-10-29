#pragma once

#include "core_editor.h"
#include "simulation/asset_system.h"
#include "simulation/entity_system.h"
#include "platform/graphics.h"
#include "rendering/material_system.h"

namespace sv {

	Result gui_initialize();
	Result gui_close();

	void gui_begin();
	void gui_end();

	// Assets

	void gui_asset_picker_open(AssetType type);
	const char* gui_asset_picker_show(AssetType type, bool* showing = nullptr);

	// ECS

	bool gui_transform_show(Transform& transform);

	bool gui_component_show(ECS* ecs, CompID compID, BaseComponent* comp, const std::function<void(CompID compID, BaseComponent* comp)>& fn);

	void gui_component_item_begin();
	void gui_component_item_next(const char* name);
	void gui_component_item_end();
	bool gui_component_item_color_picker(Color& color);
	bool gui_component_item_color_picker(Color4f& color);
	bool gui_component_item_color_picker(Color3f& color);
	bool gui_component_item_texture(TextureAsset& texture);
	void gui_component_item_material(MaterialAsset& material);
	bool gui_component_item_string(std::string& str);
	bool gui_component_item_mat4(XMMATRIX& matrix);
	bool gui_component_item_mat4(XMFLOAT4X4& matrix);

}