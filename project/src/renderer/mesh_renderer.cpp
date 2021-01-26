#include "SilverEngine/core.h"

#include "SilverEngine/mesh.h"
#include "renderer/renderer_internal.h"
#include "SilverEngine/renderer.h"

namespace sv {

	SV_INLINE static GPUBuffer* get_instance_buffer(CommandList cmd)
	{
		if (gfx.cbuffer_mesh_instance[cmd] == nullptr) {

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.size = sizeof(XMMATRIX);

			graphics_buffer_create(&desc, &gfx.cbuffer_mesh_instance[cmd]);
		}
		return gfx.cbuffer_mesh_instance[cmd];
	}

	void draw_mesh(const Mesh* mesh, const XMMATRIX& transform_matrix, CommandList cmd)
	{
		SV_ASSERT_OFFSCREEN();
		SV_ASSERT_ZBUFFER();
		SV_ASSERT_CAMERA_BUFFER();

		RenderingContext& ctx = render_context[cmd];

		// Prepare state
		graphics_shader_bind(gfx.vs_mesh, cmd);
		graphics_shader_bind(gfx.ps_mesh, cmd);
		graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
		graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
		graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
		graphics_blendstate_bind(gfx.bs_mesh, cmd);

		// Bind resources
		GPUBuffer* instance_buffer = get_instance_buffer(cmd);

		graphics_constantbuffer_bind(instance_buffer, 0u, ShaderType_Vertex, cmd);
		graphics_constantbuffer_bind(ctx.camera_buffer->buffer, 1u, ShaderType_Vertex, cmd);
		graphics_vertexbuffer_bind(mesh->vbuffer, 0u, 0u, cmd);
		graphics_indexbuffer_bind(mesh->ibuffer, 0u, cmd);

		// Update instance data
		graphics_buffer_update(instance_buffer, &XMMatrixTranspose(transform_matrix), sizeof(XMMATRIX), 0u, cmd);

		// Begin renderpass
		GPUImage* att[] = { ctx.offscreen, ctx.zbuffer };
		graphics_renderpass_begin(gfx.renderpass_mesh, att, cmd);
		
		// Draw
		graphics_draw_indexed(u32(mesh->indices.size()), 1u, 0u, 0u, 0u, cmd);

		graphics_renderpass_end(cmd);
	}

}