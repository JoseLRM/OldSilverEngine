#pragma once

#include "rendering/debug_renderer.h"
#include "utils/allocator.h"

namespace sv {
	
	struct DebugRenderer_internal {

		static Result initialize();
		static Result close();

	};

	struct DebugRendererQuad {
		XMMATRIX matrix;
		Color color;

		DebugRendererQuad(const XMMATRIX& matrix, Color color) : matrix(matrix), color(color) {}
	};

	struct DebugRendererLine {
		v3_f32 point0;
		v3_f32 point1;
		Color color;

		DebugRendererLine(const v3_f32& p0, const v3_f32& p1, Color color) : point0(p0), point1(p1), color(color) {}
	};

	struct DebugRendererDraw {
		u32 list;
		u32 index;
		u32 count;
		union {
			float lineWidth;
			float stroke;
			struct {
				GPUImage* pImage;
				Sampler* pSampler;
				v4_f32 texCoord;
			};
		};

		DebugRendererDraw() {}
		DebugRendererDraw(u32 list, u32 index, float lineWidth = 1.f) : list(list), index(index), count(1u), lineWidth(lineWidth) {}
		DebugRendererDraw(u32 list, u32 index, GPUImage* image, Sampler* sampler, const v4_f32& texCoord) 
			: list(list), index(index), count(1u), pImage(image), pSampler(sampler), texCoord(texCoord) {}
	};

	struct DebugRendererBatch_internal {

		FrameList<DebugRendererQuad>	quads;
		FrameList<DebugRendererLine>	lines;
		FrameList<DebugRendererQuad>	ellipses;
		FrameList<DebugRendererQuad>	sprites;

		FrameList<DebugRendererDraw> drawCalls;
		float lineWidth = 1.f;
		float stroke = 1.f;

		v4_f32 texCoord = { 0.f, 0.f, 1.f, 1.f };
		Sampler* pSampler = nullptr;
		bool sameSprite = false;

	};

}