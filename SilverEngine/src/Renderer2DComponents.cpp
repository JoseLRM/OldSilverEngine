#include "core.h"

#include "Renderer2DComponents.h"

namespace SV {

#ifdef SV_IMGUI
	void SpriteComponent::ShowInfo()
	{
		SV::Color4f c = SVColor4b::ToColor4f(color);
		ImGui::ColorPicker4("Color", &c.x);
		color = SVColor4f::ToColor4b(c);

		const char* actualLayer;

		if (spriteLayer != SV_INVALID_ENTITY) {
			actualLayer = "Unnamed";
			NameComponent* nameComp = pScene->GetComponent<NameComponent>(spriteLayer);
			if (nameComp) {
				actualLayer = nameComp->GetName().c_str();
			}
		}
		else {
			actualLayer = "Default";
		}

		if (ImGui::BeginCombo("Layers", actualLayer)) {

			if (ImGui::Button("Default")) spriteLayer = SV_INVALID_ENTITY;

			auto& layers = pScene->GetComponentsList(SpriteLayerComponent::ID);
			ui32 compSize = SpriteLayerComponent::SIZE;
			for (ui32 i = 0; i < layers.size(); i += compSize) {
				SpriteLayerComponent* layer = reinterpret_cast<SpriteLayerComponent*>(&layers[i]);
				NameComponent* nameComp = pScene->GetComponent<NameComponent>(layer->entity);

				const char* name = nameComp ? nameComp->GetName().c_str() : "Unnamed";

				if (ImGui::Button(name)) {
					spriteLayer = layer->entity;
				}
			}
			ImGui::EndCombo();
		}
	}

	void SpriteLayerComponent::ShowInfo()
	{
		i32 value = renderLayer->GetValue();
		ImGui::DragInt("Value", &value);
		renderLayer->SetValue(value);

		bool trans = renderLayer->IsTransparent();
		ImGui::Checkbox("Transparent", &trans);
		renderLayer->SetTransparent(trans);

		ImGui::Checkbox("Sorting", &sortByValue);
		ImGui::Checkbox("Default Rendering", &defaultRendering);
	}
#endif

	SpriteLayerComponent& SpriteLayerComponent::operator=(const SpriteLayerComponent& other)
	{
		this->defaultRendering = other.defaultRendering;
		this->entity = other.entity;
		this->pScene = other.pScene;
		this->sortByValue = other.sortByValue;
		this->renderLayer = std::make_unique<RenderLayer>(*other.renderLayer.get());
		return *this;
	}
	SpriteLayerComponent& SpriteLayerComponent::operator=(SpriteLayerComponent&& other) noexcept
	{
		this->defaultRendering = other.defaultRendering;
		this->entity = other.entity;
		this->pScene = other.pScene;
		this->sortByValue = other.sortByValue;
		this->renderLayer = std::move(other.renderLayer);
		return *this;
	}

	SV::ivec2 TileMapComponent::GetTilePos(SV::vec2 position)
	{
		SV::vec2 pos = SV::vec2(pScene->GetTransform(entity).GetWorldPosition());
		SV::vec2 scale = SV::vec2(pScene->GetTransform(entity).GetWorldScale());

		SV::vec2 size = SV::vec2(tileMap->GetWidth(), tileMap->GetHeight()) * scale;

		pos = (position - pos + (size / 2.f)) / size;

		pos *= SV::vec2(tileMap->GetWidth(), tileMap->GetHeight());

		SV::ivec2 res = SV::ivec2(pos.x, pos.y);
		if (pos.x < 0.f) res.x = -(res.x + 1);
		if (pos.y < 0.f) res.y = -(res.y + 1);
		return res;
	}

	TileMapComponent& TileMapComponent::operator=(const TileMapComponent& other)
	{
		this->entity = other.entity;
		this->pScene = other.pScene;
		this->tileMap = std::make_unique<TileMap>();
		return *this;
	}
	TileMapComponent& TileMapComponent::operator=(TileMapComponent&& other) noexcept
	{
		this->entity = other.entity;
		this->pScene = other.pScene;
		this->tileMap = std::move(other.tileMap);
		return *this;
	}

}
