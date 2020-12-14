#pragma once

#include "material_system.h"
#include "render_utils.h"

namespace sv {

	struct SpriteInstance {
		XMMATRIX	tm;
		vec4f		texCoord;
		Material*	material;
		GPUImage*	image;
		Color		color;

		SpriteInstance() = default;
		SpriteInstance(const SpriteInstance& other) 
			: tm(other.tm), texCoord(other.texCoord), material(other.material), image(other.image), color(other.color) {}
		SpriteInstance(const XMMATRIX& m, const vec4f& texCoord, Material* material, GPUImage* image, Color color)
			: tm(m), texCoord(texCoord), material(material), image(image), color(color) {}
	};

	struct SpriteRendererDrawDesc {

		GPUImage*				renderTarget;
		GBuffer*				pGBuffer;
		CameraBuffer*			pCameraBuffer;
		const SpriteInstance*	pSprites;
		u32						spriteCount;
		const LightInstance*	pLights;
		u32						lightCount;
		float					lightMult;
		Color3f					ambientLight;
		//bool			depthTest;
		//bool			transparent;

	};

	struct SpriteRenderer {

		static void drawSprites(const SpriteRendererDrawDesc* desc, CommandList cmd);

	};

}