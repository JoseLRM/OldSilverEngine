#pragma once

#include "material_system.h"

namespace sv {

	/* SPRITE SHADER INSTRUCTIONS
		
		Input {
			float4 position : Position; // Worldspace vertex position
			float2 texCoord : TexCoord; // Sprite coord in texture atlas
			float4 color	: Color;
		}

		Output {
			float4 color : SV_Target;
		}

		Resources {
			PS t0; // Albedo texture
		}

	*/

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

	struct SpriteRendererDrawDesc {
		const SpriteInstance*	pInstances;
		ui32					count;
		GPUImage*				renderTarget;
		Material*				material;
		CameraBuffer*			cameraBuffer;
	};

	void sprite_renderer_draw(const SpriteRendererDrawDesc* desc, CommandList cmd);

}