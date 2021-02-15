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
			desc.size = sizeof(MeshData);
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;

			graphics_buffer_create(&desc, &gfx.cbuffer_mesh_instance[cmd]);
		}
		return gfx.cbuffer_mesh_instance[cmd];
	}

	SV_INLINE static GPUBuffer* get_material_buffer(CommandList cmd)
	{
		if (gfx.cbuffer_material[cmd] == nullptr) {

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.size = sizeof(Material);
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;

			graphics_buffer_create(&desc, &gfx.cbuffer_material[cmd]);
		}
		return gfx.cbuffer_material[cmd];
	}

	SV_INLINE static GPUBuffer* get_light_instances_buffer(CommandList cmd)
	{
		if (gfx.cbuffer_light_instances[cmd] == nullptr) {

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.size = sizeof(LightData) * LIGHT_COUNT;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;

			graphics_buffer_create(&desc, &gfx.cbuffer_light_instances[cmd]);
		}
		return gfx.cbuffer_light_instances[cmd];
	}

	void draw_meshes(const MeshInstance* meshes, u32 mesh_count, const LightInstance* lights, u32 light_count, CommandList cmd)
	{
		SV_ASSERT_OFFSCREEN();
		SV_ASSERT_ZBUFFER();
		SV_ASSERT_CAMERA_BUFFER();

		/* TODO LIST: 
			- Undefined lights
			- Begin render pass once
			- Dinamic material and instance buffer
		*/

		RenderingContext& ctx = render_context[cmd];

		// Prepare state
		graphics_shader_bind(gfx.vs_mesh, cmd);
		graphics_shader_bind(gfx.ps_mesh, cmd);
		graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
		graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
		graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
		graphics_blendstate_bind(gfx.bs_mesh, cmd);
		graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

		// Bind resources
		GPUBuffer* instance_buffer			= get_instance_buffer(cmd);
		GPUBuffer* material_buffer			= get_material_buffer(cmd);
		GPUBuffer* light_instances_buffer	= get_light_instances_buffer(cmd);

		graphics_constantbuffer_bind(instance_buffer, 0u, ShaderType_Vertex, cmd);
		graphics_constantbuffer_bind(ctx.camera_buffer->buffer, 1u, ShaderType_Vertex, cmd);
		graphics_constantbuffer_bind(material_buffer, 0u, ShaderType_Pixel, cmd);
		graphics_constantbuffer_bind(light_instances_buffer, 1u, ShaderType_Pixel, cmd);

		light_count = std::max(LIGHT_COUNT, light_count);

		// Send light data
		{
			LightData light_data[LIGHT_COUNT];

			XMMATRIX view_matrix = ctx.camera_buffer->view_matrix;

			foreach(i, light_count) {

				LightData& l0 = light_data[i];
				const LightInstance& l1 = lights[i];

				l0.type = l1.type;
				l0.color = l1.color;
				l0.intensity = l1.intensity;
				
				switch (l1.type)
				{
				case LightType_Point:
				        l0.position = v3_f32(XMVector3Normalize(XMVector4Transform(l1.point.position.getDX(1.f), view_matrix)));
					l0.range = l1.point.range;
					l0.smoothness = l1.point.smoothness;
					break;

				case LightType_Direction:
					l0.position = v3_f32(XMVector4Transform(l1.direction.getDX(1.f), view_matrix));
					break;
				}
			}

			graphics_buffer_update(light_instances_buffer, light_data, sizeof(LightData) * light_count, 0u, cmd);
		}

		foreach(i, mesh_count) {

			const MeshInstance& inst = meshes[i];

			graphics_vertexbuffer_bind(inst.mesh->vbuffer, 0u, 0u, cmd);
			graphics_indexbuffer_bind(inst.mesh->ibuffer, 0u, cmd);

			// Update instance and material data
			MeshData mesh_data;
			mesh_data.model_view_matrix = inst.transform_matrix * ctx.camera_buffer->view_matrix;
			mesh_data.inv_model_view_matrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mesh_data.model_view_matrix));

			MaterialData material_data;
			material_data.diffuse_color = inst.material->diffuse_color;
			material_data.specular_color = inst.material->specular_color;
			material_data.shininess = inst.material->shininess;

			graphics_buffer_update(instance_buffer, &mesh_data, sizeof(MeshData), 0u, cmd);
			graphics_buffer_update(material_buffer, &material_data, sizeof(MaterialData), 0u, cmd);

			// Bind material resources
			GPUImage* diffuse_map = inst.material->diffuse_map.get();

			graphics_image_bind(diffuse_map ? diffuse_map : gfx.image_white, 0u, ShaderType_Pixel, cmd);

			// Begin renderpass
			GPUImage* att[] = { ctx.offscreen, ctx.zbuffer };
			graphics_renderpass_begin(gfx.renderpass_mesh, att, cmd);

			// Draw
			graphics_draw_indexed(u32(inst.mesh->indices.size()), 1u, 0u, 0u, 0u, cmd);

			graphics_renderpass_end(cmd);
		}
	}

}
