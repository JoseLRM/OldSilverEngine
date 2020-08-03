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
	private:
		std::unique_ptr<_sv::Offscreen> m_Offscreen;

	public:
		CameraProjection projection;

		CameraComponent();
		CameraComponent(SV_REND_CAMERA_TYPE projectionType);

		CameraComponent(const CameraComponent& other);
		CameraComponent(CameraComponent&& other) noexcept;
		CameraComponent& operator=(const CameraComponent& other);
		CameraComponent& operator=(CameraComponent&& other) noexcept;

		bool CreateOffscreen(ui32 width, ui32 height);
		bool HasOffscreen() const noexcept;
		_sv::Offscreen* GetOffscreen() const noexcept;
		bool DestroyOffscreen();
		
		void Adjust(float width, float height);

	};

}