#pragma once

#include "core.h"
#include "scene/Scene.h"
#include "renderer_desc.h"

namespace sv {

	SV_COMPONENT(SpriteComponent) {

		Color color = SV_COLOR_WHITE;
		Entity renderLayer = SV_INVALID_ENTITY;

		SpriteComponent() = default;
		SpriteComponent(Color col) : color(col) {}

	};

	SV_COMPONENT(RenderLayerComponent) {

		RenderLayer renderLayer;

	};

}