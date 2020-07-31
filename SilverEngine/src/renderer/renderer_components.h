#pragma once

#include "..//scene/Scene.h"
#include "renderer_desc.h"

namespace sv {

	SV_COMPONENT(SpriteComponent) {

		RenderLayerID renderLayer = SV_RENDER_LAYER_DEFAULT;
		Sprite sprite;
		Color color = SV_COLOR_WHITE;

		SpriteComponent() = default;
		SpriteComponent(Color col) : color(col) {}
		SpriteComponent(Sprite spr) : sprite(spr) {}
		SpriteComponent(Sprite spr, Color col) : sprite(spr), color(col) {}

	};

}