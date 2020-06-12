#include "core.h"

#include "Renderer2DComponents.h"

namespace SV {

#ifdef SV_IMGUI
	void SpriteComponent::ShowInfo(SV::Scene& scene)
	{
		SV::Color4f c = SVColor4b::ToColor4f(color);
		ImGui::ColorPicker4("Color", &c.x);
		color = SVColor4f::ToColor4b(c);

		const char* actualLayer;

		if (spriteLayer != SV_INVALID_ENTITY) {
			actualLayer = "Unnamed";
			NameComponent* nameComp = scene.GetComponent<NameComponent>(spriteLayer);
			if (nameComp) {
				actualLayer = nameComp->GetName().c_str();
			}
		}
		else {
			actualLayer = "Default";
		}

		if (ImGui::BeginCombo("Layers", actualLayer)) {

			if (ImGui::Button("Default")) spriteLayer = SV_INVALID_ENTITY;

			auto& layers = scene.GetComponentsList(SpriteLayerComponent::ID);
			ui32 compSize = SpriteLayerComponent::SIZE;
			for (ui32 i = 0; i < layers.size(); i += compSize) {
				SpriteLayerComponent* layer = reinterpret_cast<SpriteLayerComponent*>(&layers[i]);
				NameComponent* nameComp = scene.GetComponent<NameComponent>(layer->entity);

				const char* name = nameComp ? nameComp->GetName().c_str() : "Unnamed";

				if (ImGui::Button(name)) {
					spriteLayer = layer->entity;
				}
			}
			ImGui::EndCombo();
		}
	}
#endif

	void SpriteLayerComponent::ShowInfo(SV::Scene& scene)
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

}