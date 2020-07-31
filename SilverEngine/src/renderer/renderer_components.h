#pragma once

#include "..//scene/Scene.h"
#include "renderer_desc.h"

namespace sv {

	SV_COMPONENT(SpriteComponent) {

		RenderLayerID renderLayer;
		Sprite sprite;
		Color color = SV_COLOR_WHITE;

		SpriteComponent(RenderLayerID renderLayer = SV_RENDER_LAYER_DEFAULT) : renderLayer(renderLayer) {}
		SpriteComponent(Color col, RenderLayerID renderLayer = SV_RENDER_LAYER_DEFAULT) : color(col), renderLayer(renderLayer) {}
		SpriteComponent(Sprite spr, RenderLayerID renderLayer = SV_RENDER_LAYER_DEFAULT) : sprite(spr), renderLayer(renderLayer) {}
		SpriteComponent(Sprite spr, Color col, RenderLayerID renderLayer = SV_RENDER_LAYER_DEFAULT) : sprite(spr), color(col), renderLayer(renderLayer) {}

	};

}