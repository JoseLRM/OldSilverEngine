#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	constexpr u32 TEXT_BATCH_COUNT = 100u; // Num of letters
	constexpr u32 DEBUG_VERTEX_COUNT = 1000u;
	constexpr u32 SPRITE_BATCH_COUNT = 1000u;

	struct TextVertex {
		v4_f32	position;
		v2_f32	texCoord;
		Color	color;
	};

	struct TextData {
		TextVertex vertices[TEXT_BATCH_COUNT * 4u];
	};

	struct DebugVertex {
		v4_f32	position;
		v2_f32	texCoord;
		f32		stroke;
		Color	color;
	};

	struct DebugData {
		DebugVertex vertices[DEBUG_VERTEX_COUNT];
	};

	struct SpriteVertex {
		v4_f32 position;
		v2_f32 texCoord;
		Color color;
		Color emissionColor;
	};

	struct SpriteData {
		SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
	};

	constexpr u32 compute_max_batch_data()
	{
		size_t memory = 0u;
		memory = std::max(memory, sizeof(TextData));
		memory = std::max(memory, sizeof(DebugData));
		memory = std::max(memory, sizeof(SpriteData));
		return memory;
	}
	constexpr u32 MAX_BATCH_DATA = compute_max_batch_data();

	struct GraphicsObjects {

		GPUImage* image_white;
		Sampler* sampler_def_linear;
		Sampler* sampler_def_nearest;
		GPUBuffer* vbuffer_batch[GraphicsLimit_CommandList];

		// DEBUG

		Shader* vs_debug_quad;
		Shader* ps_debug_quad;
		Shader* vs_debug_ellipse;
		Shader* ps_debug_ellipse;
		Shader* vs_debug_sprite;
		Shader* ps_debug_sprite;
		RenderPass* renderpass_debug;
		InputLayoutState* ils_debug;
		BlendState* bs_debug;

		// TEXT

		Shader* vs_text;
		Shader* ps_text;
		RenderPass* renderpass_text;
		GPUBuffer* ibuffer_text;
		InputLayoutState* ils_text;

		// SPRITE

		Shader* vs_sprite;
		Shader* ps_sprite;
		RenderPass* renderpass_sprite;
		InputLayoutState* ils_sprite;
		BlendState* bs_sprite;
		GPUBuffer* ibuffer_sprite;

	};
	
	struct RenderingUtils {

		u8* batch_data;

	};

	extern GraphicsObjects gfx;
	extern RenderingUtils rend_utils[GraphicsLimit_CommandList];

	Result rendering_initialize();
	Result rendering_close();

	SV_INLINE GPUBuffer* get_batch_buffer(CommandList cmd)
	{
		if (gfx.vbuffer_batch[cmd] == nullptr) {

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = MAX_BATCH_DATA;
			desc.pData = nullptr;

			graphics_buffer_create(&desc, &gfx.vbuffer_batch[cmd]);
			graphics_name_set(gfx.vbuffer_batch[cmd], "BatchVertexBuffer");
		}
		return gfx.vbuffer_batch[cmd];
	}

}