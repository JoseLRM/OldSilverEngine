#pragma once

#include "core.h"
#include "RenderList.h"
#include "TextureAtlas.h"

enum SV_REND_SORT_MODE : ui8 {
	SV_REND_SORT_MODE_NONE,
	SV_REND_SORT_MODE_COORD_X,
	SV_REND_SORT_MODE_COORD_Y,
	SV_REND_SORT_MODE_COORD_Z,
};

enum SV_REND_CAMERA_TYPE : ui8 {
	SV_REND_CAMERA_TYPE_NONE,
	SV_REND_CAMERA_TYPE_ORTHOGRAPHIC,
	SV_REND_CAMERA_TYPE_PERSPECTIVE
};

namespace sv {
	class Camera;
}

namespace _sv {

	struct Offscreen {
		sv::Image renderTarget;
		sv::Image depthStencil;
		inline sv::Viewport GetViewport() const noexcept { return renderTarget->GetViewport(); }
		inline sv::Scissor GetScissor() const noexcept { return renderTarget->GetScissor(); }
		inline ui32 GetWidth() const noexcept { return renderTarget->GetWidth(); }
		inline ui32 GetHeight() const noexcept { return renderTarget->GetHeight(); }
	};

	struct SpriteInstance {
		XMMATRIX tm;
		sv::Sprite sprite;
		sv::Color color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, sv::Sprite sprite, sv::Color color) : tm(m), sprite(sprite), color(color) {}
	};

	struct RenderLayer {
		sv::RenderList<SpriteInstance>	sprites;
		SV_REND_SORT_MODE				sortMode;
		i16								sortValue;
		bool							transparent;

		void Reset()
		{
			sprites.Reset();
		}
	};

	struct PostProcessing_Default {
		sv::RenderPass renderPass;
	};

	struct DrawData {

		std::vector<std::pair<sv::Camera*, XMMATRIX>>	cameras;
		sv::Camera*										currentCamera;
		Offscreen*										pOutput;
		XMMATRIX										viewMatrix;
		XMMATRIX										projectionMatrix;
		XMMATRIX										viewProjectionMatrix;

	};

}

namespace sv {

	typedef void* RenderLayerID;

}

constexpr sv::RenderLayerID SV_RENDER_LAYER_DEFAULT = nullptr;



