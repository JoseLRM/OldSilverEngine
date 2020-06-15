#pragma once

#include "core.h"
#include "Scene.h"
#include "Renderer2D.h"
#include "TileMap.h"

namespace SV {

	class Renderer;

	struct SpriteComponent : public SV::Component<SpriteComponent> {
		Sprite sprite;
		SV::Color4b color = SVColor4b::WHITE;
		SV::Entity spriteLayer = SV_INVALID_ENTITY;

		SpriteComponent() : sprite() {}
		SpriteComponent(const Sprite& spr) : sprite(spr) {}
		SpriteComponent(const Sprite& spr, SV::Color4b color) : sprite(spr), color(color) {}

#ifdef SV_IMGUI
		void ShowInfo() override;
#endif

	};
	SVDefineComponent(SpriteComponent);

	struct SpriteLayerComponent : public SV::Component<SpriteLayerComponent> {
		std::unique_ptr<RenderLayer> renderLayer;
		bool sortByValue = false;
		bool defaultRendering = true;

		SpriteLayerComponent() : renderLayer(std::make_unique<RenderLayer>()) {}
		SpriteLayerComponent(i32 value, bool transparent) : renderLayer(std::make_unique<RenderLayer>(value, transparent)) {}

#ifdef SV_IMGUI
		void ShowInfo() override;
#endif
	};
	SVDefineComponent(SpriteLayerComponent);

	struct TileMapComponent : public SV::Component<SV::TileMapComponent> {

		std::unique_ptr<TileMap> tileMap;

		SV::ivec2 GetTilePos(SV::vec2 position);

		TileMapComponent() : tileMap(std::make_unique<TileMap>()) {}
	};
	SVDefineComponent(TileMapComponent);

}