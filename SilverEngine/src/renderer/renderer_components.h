#pragma once

#include "..//scene/Scene.h"
#include "renderer_desc.h"

namespace sv {

	SV_COMPONENT(SpriteComponent) {

		Entity renderLayer = SV_INVALID_ENTITY;
		Sprite sprite;
		Color color = SV_COLOR_WHITE;

		SpriteComponent() = default;
		SpriteComponent(Color col) : color(col) {}
		SpriteComponent(Sprite spr) : sprite(spr) {}
		SpriteComponent(Sprite spr, Color col) : sprite(spr), color(col) {}

	};

	SV_COMPONENT(RenderLayerComponent) {

		RenderLayer renderLayer;

	};

}