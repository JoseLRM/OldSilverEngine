#pragma once

#include "core/rendering/material_system.h"
#include "core/rendering/render_utils.h"

namespace sv {

	struct SpriteSimpleInstance {

		XMMATRIX	tm;
		Color		color;

		SpriteSimpleInstance() = default;
		SpriteSimpleInstance(const SpriteSimpleInstance& other)
			: tm(other.tm), color(other.color) {}
		SpriteSimpleInstance(const XMMATRIX& m, Color color)
			: tm(m), color(color) {}
	};

	struct SpriteInstance {
		XMMATRIX	tm;
		v4_f32		texCoord;
		Material*	material;
		GPUImage*	image;
		Color		color;

		SpriteInstance() = default;
		SpriteInstance(const SpriteInstance& other) 
			: tm(other.tm), texCoord(other.texCoord), material(other.material), image(other.image), color(other.color) {}
		SpriteInstance(const XMMATRIX& m, const v4_f32& texCoord, Material* material, GPUImage* image, Color color)
			: tm(m), texCoord(texCoord), material(material), image(image), color(color) {}
	};

	struct SpriteRendererDrawDesc {

		CameraData*				pCameraData;
		const SpriteInstance*	pSprites;
		u32						spriteCount;
		const LightInstance*	pLights;
		u32						lightCount;
		float					lightMult;
		Color3f					ambientLight;
		bool					depthTest;
		bool					transparent;

	};

	struct SpriteRendererSameDrawDesc {

		CameraData*					pCameraData;
		const SpriteSimpleInstance*	pSprites;
		u32							spriteCount;
		GPUImage*					image;
		v4_f32						texCoord;
		Material*					material;
		const LightInstance*		pLights;
		u32							lightCount;
		float						lightMult;
		Color3f						ambientLight;
		bool						depthTest;
		bool						transparent;

	};

	struct SpriteRenderer {

		static void drawSprites(const SpriteRendererDrawDesc* desc, CommandList cmd);
		static void drawSameSprites(const SpriteRendererSameDrawDesc* desc, CommandList cmd);

	};

}