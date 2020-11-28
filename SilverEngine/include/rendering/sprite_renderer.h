#pragma once

#include "material_system.h"

namespace sv {

	struct SpriteInstance {
		XMMATRIX	tm;
		vec4f		texCoord;
		GPUImage*	pTexture;
		Color		color;

		SpriteInstance() = default;
		SpriteInstance(const SpriteInstance& other) 
			: tm(other.tm), texCoord(other.texCoord), pTexture(other.pTexture), color(other.color) {}
		SpriteInstance(const XMMATRIX& m, const vec4f& texCoord, GPUImage* pTex, Color color) 
			: tm(m), texCoord(texCoord), pTexture(pTex), color(color) {}
	};

	struct SpriteRenderer {

		static void enableDepthTest(GPUImage* depthStencil, CommandList cmd);
		static void disableDepthTest(CommandList cmd);

		static void prepare(GPUImage* renderTarget, CameraBuffer& cameraBuffer, CommandList cmd);

		static void draw(Material* material, const SpriteInstance* instances, ui32 count, CommandList cmd);

	};

}