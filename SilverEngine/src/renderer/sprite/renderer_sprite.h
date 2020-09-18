#pragma once

#include "renderer/objects/renderer_objects.h"

namespace sv {

	struct SpriteInstance {
		XMMATRIX tm;
		vec4f texCoord;
		Texture* pTexture;
		Color color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, const vec4f& texCoord, Texture* pTex, sv::Color color) : tm(m), texCoord(texCoord), pTexture(pTex), color(color) {}
	};

	struct SpriteRenderingDesc {
		const SpriteInstance* pInstances;
		ui32					count;
		const XMMATRIX* pViewProjectionMatrix;
		GPUImage* pRenderTarget;
	};

	void renderer_sprite_rendering(const SpriteRenderingDesc* desc, CommandList cmd);

}