#pragma once

#include "core.h"
#include "Scene.h"
#include "Renderer2D.h"

namespace SV {

	struct SpriteComponent : public SV::Component<SpriteComponent> {
		Sprite sprite;
		SV::Color4b color = SVColor4b::WHITE;
		SV::Entity spriteLayer = SV_INVALID_ENTITY;

		SpriteComponent() : sprite() {}
		SpriteComponent(const Sprite& spr) : sprite(spr) {}

#ifdef SV_IMGUI
		void ShowInfo(SV::Scene& scene) override;
#endif

	};
	SVDefineComponent(SpriteComponent);

	struct SpriteLayerComponent : public SV::Component<SpriteLayerComponent> {
		std::unique_ptr<RenderLayer> renderLayer;
		bool sortByValue = false;
		bool defaultRendering = true;

		SpriteLayerComponent() : renderLayer(std::make_unique<RenderLayer>()) {}
		SpriteLayerComponent(i32 value, bool transparent) : renderLayer(std::make_unique<RenderLayer>(value, transparent)) {}

		void ShowInfo(SV::Scene& scene) override;
	};
	SVDefineComponent(SpriteLayerComponent);

}