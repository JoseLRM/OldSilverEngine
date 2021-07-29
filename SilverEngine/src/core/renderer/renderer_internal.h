#pragma once

#include "core/renderer.h"

#include "shared_headers/core.h"

namespace sv {

    struct GPU_MeshInstanceData {
		XMMATRIX model_view_matrix;
		XMMATRIX inv_model_view_matrix;
    };

	struct GPU_TerrainInstanceData {
		XMMATRIX model_view_matrix;
		XMMATRIX inv_model_view_matrix;
		u32 size_x;
		u32 size_z;
		f32 _padding0;
		f32 _padding1;
    };

    struct GPU_MaterialData {
		v3_f32 diffuse_color;
		u32    flags;
		v3_f32 specular_color;
		f32    shininess;
		v3_f32 emissive_color;
    };

#define MAT_FLAG_NORMAL_MAPPING SV_BIT(0u)
#define MAT_FLAG_SPECULAR_MAPPING SV_BIT(1u)
#define MAT_FLAG_EMISSIVE_MAPPING SV_BIT(2u)

	struct GPU_ShadowMappingData {
		XMMATRIX tm;
	};
	
    constexpr u32 TEXT_BATCH_COUNT = 1000u; // Num of letters
    constexpr u32 SPRITE_BATCH_COUNT = 1000u;

    struct TextVertex {
		v4_f32	position;
		v2_f32	texCoord;
		Color	color;
    };

    struct GPU_TextData {
		TextVertex vertices[TEXT_BATCH_COUNT * 4u];
    };

    struct SpriteVertex {
		v4_f32 position;
		v2_f32 texCoord;
		Color color;
    };

    struct SpriteData {
		SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
    };

    struct GaussianBlurData {
		f32 intensity;
		u32 horizontal;
		v2_f32 padding;
    };

    struct EnvironmentData {
		v3_f32 ambient_light;
    };

    struct GraphicsObjects {

		GPUImage* image_white;
		GPUImage* image_aux0;
		GPUImage* image_aux1;
		GPUImage* image_ssao_aux;
		Sampler* sampler_def_linear;
		Sampler* sampler_def_nearest;
		GPUBuffer* vbuffer_batch[GraphicsLimit_CommandList];
		DepthStencilState* dss_default_depth;
		DepthStencilState* dss_read_depth;
		DepthStencilState* dss_write_depth;
		RasterizerState* rs_back_culling;
		RasterizerState* rs_front_culling;
		RasterizerState* rs_wireframe;
		RasterizerState* rs_wireframe_back_culling;
		RasterizerState* rs_wireframe_front_culling;
		GPUBuffer* cbuffer_camera;
		GPUBuffer* cbuffer_environment;
		RenderPass* renderpass_off;
		RenderPass* renderpass_world;
		RenderPass* renderpass_gbuffer;
		BlendState* bs_transparent;

		// GBUFFER

		GPUImage* offscreen;
		GPUImage* gbuffer_normal;
		GPUImage* gbuffer_emission;
		GPUImage* gbuffer_depthstencil;
		GPUImage* gbuffer_ssao;

		// TEXT

		Shader* vs_text;
		Shader* ps_text;
		GPUBuffer* ibuffer_text;
		InputLayoutState* ils_text;

		// SPRITE

		Shader* vs_sprite;
		Shader* ps_sprite;
		InputLayoutState* ils_sprite;
		GPUBuffer* ibuffer_sprite;

		// MESH

		Shader* vs_mesh_default;
		Shader* ps_mesh_default;
		InputLayoutState* ils_mesh;
		BlendState* bs_mesh;
		GPUBuffer* cbuffer_material;
		GPUBuffer* cbuffer_mesh_instance;
		GPUBuffer* cbuffer_light_instances;

		// TERRAIN

		Shader* vs_terrain;
		Shader* ps_terrain;
		InputLayoutState* ils_terrain;
		GPUBuffer* cbuffer_terrain_instance;

		// SHADOW MAPPING

		Shader* vs_shadow;
		GPUBuffer* cbuffer_shadow_mapping;
		GPUBuffer* cbuffer_shadow_data;
		RenderPass* renderpass_shadow_mapping;

		// PP

		Shader* cs_addition;
		Shader* cs_gaussian_blur_float4;
		Shader* cs_gaussian_blur_float;
		GPUBuffer* cbuffer_gaussian_blur;
		GPUBuffer* cbuffer_bloom_threshold;
		Sampler* sampler_blur;
		Shader* cs_bloom_threshold;
		BlendState* bs_addition;
		Shader* cs_ssao;
		Shader* cs_ssao_add;
		GPUBuffer* cbuffer_ssao;

		// SKY

		Shader* vs_sky;
		Shader* ps_sky;
		GPUBuffer* vbuffer_skybox;
		GPUBuffer* cbuffer_skybox;
		InputLayoutState* ils_sky;

    };

	struct ShadowMapRef {
		Entity entity;
		GPUImage* image[4u];
	};
    
    struct RendererState {

		GraphicsObjects gfx = {};

		u8* batch_data[GraphicsLimit_CommandList] = {};

		List<TextVertex> text_vertices[GraphicsLimit_CommandList] = {};

		Font font_opensans;
		Font font_console;

		List<ShadowMapRef> shadow_maps;
    };

    extern RendererState* renderer;

    SV_INLINE GPUBuffer* get_batch_buffer(u32 size, CommandList cmd)
    {
		// TODO: Dynamic update
		GPUBuffer*& buffer = renderer->gfx.vbuffer_batch[cmd];

		if (buffer == nullptr || graphics_buffer_info(buffer).size < size) {

			if (buffer) {
				graphics_destroy(buffer);
				SV_FREE_MEMORY(renderer->batch_data[cmd]);
			}
			
			GPUBufferDesc desc;
			desc.buffer_type = GPUBufferType_Vertex;
			desc.usage = ResourceUsage_Default;
			desc.cpu_access = CPUAccess_Write;
			desc.size = size;
			desc.data = nullptr;

			graphics_buffer_create(&desc, &buffer);
			graphics_name_set(buffer, "BatchBuffer");

			renderer->batch_data[cmd] = (u8*)SV_ALLOCATE_MEMORY(size, "Renderer");
		}

		return buffer;
    }

}
