#pragma once

#include "renderer_debug.h"

namespace sv {

	Result renderer_debug_initialize();
	Result renderer_debug_close();

	struct RendererDebugQuad {
		XMMATRIX matrix;
		Color color;

		RendererDebugQuad(const XMMATRIX& matrix, Color color) : matrix(matrix), color(color) {}
	};

	struct RendererDebugLine {
		vec3 point0;
		vec3 point1;
		Color color;

		RendererDebugLine(const vec3& p0, const vec3& p1, Color color) : point0(p0), point1(p1), color(color) {}
	};

	struct RendererDebugDraw {
		ui32 list;
		ui32 index;
		ui32 count;
		union {
			float lineWidth;
			float stroke;
			struct {
				GPUImage* pImage;
				Sampler* pSampler;
				vec4 texCoord;
			};
		};

		RendererDebugDraw() {}
		RendererDebugDraw(ui32 list, ui32 index, float lineWidth = 1.f) : list(list), index(index), count(1u), lineWidth(lineWidth) {}
		RendererDebugDraw(ui32 list, ui32 index, GPUImage& image, Sampler* sampler, const vec4& texCoord) 
			: list(list), index(index), count(1u), pImage(&image), pSampler(sampler), texCoord(texCoord) {}
	};

	struct RendererDebugBatch_internal {

		std::vector<RendererDebugQuad>	quads;
		std::vector<RendererDebugLine>	lines;
		std::vector<RendererDebugQuad>	ellipses;
		std::vector<RendererDebugQuad>	sprites;

		std::vector<RendererDebugDraw> drawCalls;
		float lineWidth = 1.f;
		float stroke = 1.f;

		vec4 texCoord = { 0.f, 0.f, 1.f, 1.f };
		Sampler* pSampler = nullptr;
		bool sameSprite = false;

	};

}