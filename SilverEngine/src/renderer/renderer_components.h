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

	SV_COMPONENT(CameraComponent) {

		std::unique_ptr<Camera> camera;
		bool active = true;

		CameraComponent();
		CameraComponent(SV_REND_CAMERA_TYPE type);

		CameraComponent(const CameraComponent& other);
		CameraComponent(CameraComponent&& other) noexcept;
		CameraComponent& operator=(const CameraComponent& other);
		CameraComponent& operator=(CameraComponent&& other) noexcept;

		void Adjust(float width, float height);

	};

}