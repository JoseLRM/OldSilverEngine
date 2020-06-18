#pragma once

#include "core.h"
#include "Scene.h"
#include "Renderer2D.h"
#include "TileMap.h"

namespace SV {

	class Renderer;

	SV_COMPONENT(SpriteComponent) {
		Sprite sprite;
		SV::Color4b color = SVColor4b::WHITE;
		SV::Entity spriteLayer = SV_INVALID_ENTITY;

		SpriteComponent() : sprite() {}
		SpriteComponent(const Sprite& spr) : sprite(spr) {}
		SpriteComponent(const Sprite& spr, SV::Color4b color) : sprite(spr), color(color) {}

	};

	SV_COMPONENT(SpriteLayerComponent) {
		std::unique_ptr<RenderLayer> renderLayer;
		bool sortByValue = false;
		bool defaultRendering = true;

		SpriteLayerComponent() : renderLayer(std::make_unique<RenderLayer>()) {}
		SpriteLayerComponent(i32 value, bool transparent) : renderLayer(std::make_unique<RenderLayer>(value, transparent)) {}

		SpriteLayerComponent(const SpriteLayerComponent& other) { this->operator=(other); }
		SpriteLayerComponent(SpriteLayerComponent&& other) noexcept { this->operator=(std::move(other)); }
		SpriteLayerComponent& operator=(const SpriteLayerComponent& other);
		SpriteLayerComponent& operator=(SpriteLayerComponent&& other) noexcept;

	};

	SV_COMPONENT(TileMapComponent) {

		std::unique_ptr<TileMap> tileMap;

		SV::ivec2 GetTilePos(SV::vec2 position);

		TileMapComponent() : tileMap(std::make_unique<TileMap>()) {}
		TileMapComponent(const TileMapComponent& other) { this->operator=(other); }
		TileMapComponent(TileMapComponent&& other) noexcept { this->operator=(std::move(other)); }
		TileMapComponent& operator=(const TileMapComponent& other);
		TileMapComponent& operator=(TileMapComponent&& other) noexcept;
	};

}