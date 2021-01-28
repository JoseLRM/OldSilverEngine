#pragma once

#include "SilverEngine/renderer.h"

// CONTEXT ASSERTIONS

#define SV_ASSERT_OFFSCREEN() SV_ASSERT(sv::render_context[cmd].offscreen)
#define SV_ASSERT_ZBUFFER() SV_ASSERT(sv::render_context[cmd].zbuffer)
#define SV_ASSERT_CAMERA_BUFFER() SV_ASSERT(sv::render_context[cmd].camera_buffer && sv::render_context[cmd].camera_buffer->buffer)

namespace sv {

	constexpr u32 LIGHT_COUNT = 10u;

	struct LightData {
		v3_f32		position;
		LightType	type;
		f32			range;
		f32			intensity;
		f32			smoothness;
		f32 padding0;
		Color3f		color;
		f32 padding1;
	};

	struct MeshData {
		XMMATRIX model_view_matrix;
		XMMATRIX inv_model_view_matrix;
	};

	struct CameraBuffer_GPU {
		XMMATRIX	view_matrix;
		XMMATRIX	projection_matrix;
		XMMATRIX	view_projection_matrix;
		v4_f32		position;
		v4_f32		rotation;
	};

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

		GPUImage*			image_white;
		Sampler*			sampler_def_linear;
		Sampler*			sampler_def_nearest;
		GPUBuffer*			vbuffer_batch[GraphicsLimit_CommandList];
		DepthStencilState*	dss_default_depth;
		RasterizerState*	rs_back_culling;

		// DEBUG

		Shader*				vs_debug_quad;
		Shader*				ps_debug_quad;
		Shader*				vs_debug_ellipse;
		Shader*				ps_debug_ellipse;
		Shader*				vs_debug_sprite;
		Shader*				ps_debug_sprite;
		RenderPass*			renderpass_debug;
		InputLayoutState*	ils_debug;
		BlendState*			bs_debug;

		// TEXT

		Shader*				vs_text;
		Shader*				ps_text;
		RenderPass*			renderpass_text;
		GPUBuffer*			ibuffer_text;
		InputLayoutState*	ils_text;

		// SPRITE

		Shader*				vs_sprite;
		Shader*				ps_sprite;
		RenderPass*			renderpass_sprite;
		InputLayoutState*	ils_sprite;
		BlendState*			bs_sprite;
		GPUBuffer*			ibuffer_sprite;

		// MESH

		Shader*				vs_mesh;
		Shader*				ps_mesh;
		RenderPass*			renderpass_mesh;
		InputLayoutState*	ils_mesh;
		BlendState*			bs_mesh;
		GPUBuffer*			cbuffer_material[GraphicsLimit_CommandList];
		GPUBuffer*			cbuffer_mesh_instance[GraphicsLimit_CommandList];
		GPUBuffer*			cbuffer_light_instances[GraphicsLimit_CommandList];

	};
	
	struct RenderingUtils {
		u8* batch_data;
	};

	extern GraphicsObjects gfx;
	extern RenderingUtils rend_utils[GraphicsLimit_CommandList];

	Result	renderer_initialize();
	Result	renderer_close();
	void	renderer_begin_frame();

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