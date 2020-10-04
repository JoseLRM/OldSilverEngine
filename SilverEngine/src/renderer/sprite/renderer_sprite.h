#pragma once

#include "renderer/objects/renderer_objects.h"
#include "material_system.h"

namespace sv {

	struct SpriteInstance {
		XMMATRIX	tm;
		vec4f		texCoord;
		GPUImage*	pTexture;
		Color		color;
		Material*	material;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, const vec4f& texCoord, GPUImage* pTex, Color color, Material* material) 
			: tm(m), texCoord(texCoord), pTexture(pTex), color(color), material(material) {}
	};

	struct SpriteRenderingDesc {
		const SpriteInstance* pInstances;
		ui32					count;
		const XMMATRIX* pViewProjectionMatrix;
		GPUImage* pRenderTarget;
	};

	void renderer_sprite_rendering(const SpriteRenderingDesc* desc, CommandList cmd);

}