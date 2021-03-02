#include "SilverEngine/core.h"

#include "SilverEngine/utils/allocators/FrameList.h"
#include "renderer/renderer_internal.h"

namespace sv {

	struct DebugQuad {
		XMMATRIX matrix;
		Color color;
		
		DebugQuad(const XMMATRIX& matrix, Color color) : matrix(matrix), color(color) {}
	};

	struct DebugLine {
		v3_f32 p0, p1;
		Color color;
		
		DebugLine(const v3_f32& p0, const v3_f32& p1, Color color) : point0(p0), point1(p1), color(color) {}
	};

	struct DebugRendererBatch {

		FrameList<DebugQuad> quads;
		FrameList<DebugLine> lines;
		
	};

#define SV_PARSE_BATCH() sv::DebugRendererBatch& batch = sv::debug_batches[cmd]
	
	static DebugRendererBatch debug_batches[GraphicsLimit_CommandList];

	void begin_debug_batch(CommandList cmd)
	{
		SV_PARSE_BATCH();

		batch.quads.reset();
		batch.lines.reset();
	}

	constexpr u32 
	
	void end_debug_batch(GPUImage* offscreen, GPUImage* depthstencil, const XMMATRIX& viewProjectionMatrix, CommandList cmd)
	{
		SV_PARSE_BATCH();
		
		// DRAW QUADS
		if (batch.quads.size()) {

			for (const DebugQuad& q : batch.quads) {
				
			}
		}

		// DRAW LINES
		if (batch.lines.size()) {

			for (const DebugLine& l : batch.lines) {
				
			}
		}

		GPUImage* att[2u];
		att[0] = offscreen;
		att[1] = depthstencil;
		
		graphics_renderpass_begin(gfx.renderpass_world, att, nullptr, 1.f, 0u, cmd);

		graphics_renderpass_end(cmd);
	}

	void draw_debug_quad(const XMMATRIX& matrix, Color color, CommandList cmd)
	{
		SV_PARSE_BATCH();
		batch.quads.emplace_back(matrix, color);
	}
	
	void draw_debug_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd)
	{
		SV_PARSE_BATCH();
		batch.lines.emplace_back(p0, p1, color, cmd);
	}

	// HELPER
	
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		draw_debug_quad(tm, color, cmd);
	}

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		draw_debug_quad(tm, color, cmd);
	}

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationQuaternion(rotationQuat.get_dx()) * XMMatrixTranslation(position.x, position.y, position.z);
		draw_debug_quad(tm, color, cmd);
	}
	
	void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd)
	{
		DEF_BATCH();

		v2_f32 begin = position - offset - size / 2.f;
		v2_f32 end = begin + size;

		for (f32 y = i32(begin.y / gridSize.y) * gridSize.y; y < end.y; y += gridSize.y) {
			draw_debug_line({ begin.x + offset.x, y + offset.y, 0.f }, { end.x + offset.x, y + offset.y, 0.f }, color, cmd);
		}
		for (f32 x = i32(begin.x / gridSize.x) * gridSize.x; x < end.x; x += gridSize.x) {
			draw_debug_line({ x + offset.x, begin.y + offset.y, 0.f }, { x + offset.x, end.y + offset.y, 0.f }, color, cmd);
		}
	}
}
