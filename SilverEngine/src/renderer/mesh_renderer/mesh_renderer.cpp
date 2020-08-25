#include "core.h"

#include "mesh_renderer.h"
#include "scene/scene_internal.h"
#include "components.h"

namespace sv {

	struct MeshData {
		XMMATRIX modelViewMatrix;
	};

	struct CameraBufferData {
		XMMATRIX projectionMatrix;
	};

	static GPUBuffer g_InstanceBuffer;
	static GPUBuffer g_CameraBuffer;

	// TODO: not global
	static MeshData g_MeshData[SV_REND_MESH_INSTANCE_COUNT];

	static RenderPass			g_ForwardRenderPass;
	static GraphicsPipeline		g_ForwardPipeline;
	static Shader				g_ForwardVertexShader;
	static Shader				g_ForwardPixelShader;
	static InputLayoutState		g_ForwardInputLayoutState;
	static DepthStencilState	g_ForwardDepthStencilState;
	static RasterizerState		g_ForwardRasterizerState;

	bool mesh_renderer_initialize()
	{
		// Instance Buffer
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = SV_REND_MESH_INSTANCE_COUNT * sizeof(MeshData);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, g_InstanceBuffer));
		}
		// Camera Buffer
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(CameraBufferData);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, g_CameraBuffer));
		}
		// Forward technique
		{
			RenderPassDesc desc;
			desc.attachments.resize(2u);

			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[0].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[0].format = SV_REND_OFFSCREEN_FORMAT;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			desc.attachments[1].loadOp = AttachmentOperation_Load;
			desc.attachments[1].storeOp = AttachmentOperation_Store;
			desc.attachments[1].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[1].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[1].format = Format_D24_UNORM_S8_UINT;
			desc.attachments[1].initialLayout = GPUImageLayout_DepthStencil;
			desc.attachments[1].layout = GPUImageLayout_DepthStencil;
			desc.attachments[1].finalLayout = GPUImageLayout_DepthStencil;
			desc.attachments[1].type = AttachmentType_DepthStencil;

			svCheck(graphics_renderpass_create(&desc, g_ForwardRenderPass));
		}
		{
			svCheck(renderer_shader_create("Forward", ShaderType_Vertex, g_ForwardVertexShader));
			svCheck(renderer_shader_create("Forward", ShaderType_Pixel, g_ForwardPixelShader));
		}
		{
			InputLayoutStateDesc desc;
			desc.slots.resize(2u);

			desc.slots[0].instanced = false;
			desc.slots[0].slot = 0u;
			desc.slots[0].stride = sizeof(MeshVertex);

			desc.slots[1].instanced = true;
			desc.slots[1].slot = 1u;
			desc.slots[1].stride = sizeof(MeshData);

			desc.elements.resize(7u);

			desc.elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };
			desc.elements[1] = { "Normal", 0u, 0u, 3 * sizeof(float), Format_R32G32B32_FLOAT };
			desc.elements[2] = { "TexCoord", 0u, 0u, 6 * sizeof(float), Format_R32G32_FLOAT };
			
			desc.elements[3] = { "ModelViewMatrix", 0u, 1u, 0u * sizeof(float), Format_R32G32B32A32_FLOAT };
			desc.elements[4] = { "ModelViewMatrix", 1u, 1u, 4u * sizeof(float), Format_R32G32B32A32_FLOAT };
			desc.elements[5] = { "ModelViewMatrix", 2u, 1u, 8u * sizeof(float), Format_R32G32B32A32_FLOAT };
			desc.elements[6] = { "ModelViewMatrix", 3u, 1u, 12u * sizeof(float), Format_R32G32B32A32_FLOAT };

			svCheck(graphics_inputlayoutstate_create(&desc, g_ForwardInputLayoutState));
		}
		{
			RasterizerStateDesc desc;
			desc.wireframe = false;
			desc.lineWidth = 1.f;
			desc.cullMode = RasterizerCullMode_Back;
			desc.clockwise = true;

			svCheck(graphics_rasterizerstate_create(&desc, g_ForwardRasterizerState));
		}
		{
			DepthStencilStateDesc desc;
			desc.depthTestEnabled = true;
			desc.depthWriteEnabled = true;
			desc.depthCompareOp = CompareOperation_LessOrEqual;
			desc.stencilTestEnabled = false;
			desc.readMask = 0xFF;
			desc.writeMask = 0xFF;

			svCheck(graphics_depthstencilstate_create(&desc, g_ForwardDepthStencilState));
		}

		g_ForwardPipeline.pVertexShader = &g_ForwardVertexShader;
		g_ForwardPipeline.pPixelShader = &g_ForwardPixelShader;
		g_ForwardPipeline.pGeometryShader = nullptr;
		g_ForwardPipeline.pInputLayoutState = &g_ForwardInputLayoutState;
		g_ForwardPipeline.pBlendState = nullptr;
		g_ForwardPipeline.pRasterizerState = &g_ForwardRasterizerState;
		g_ForwardPipeline.pDepthStencilState = &g_ForwardDepthStencilState;
		g_ForwardPipeline.topology = GraphicsTopology_Triangles;

		return true;
	}

	bool mesh_renderer_close()
	{

		return true;
	}

	void mesh_renderer_scene_draw_meshes(CommandList cmd)
	{
		EntityView<MeshComponent> meshes;
		std::vector<MeshInstance> instances(meshes.size());

		// Create instances
		for (ui32 i = 0; i < meshes.size(); ++i) {
			MeshComponent& mesh = meshes[i];
			Transform trans = scene_ecs_entity_get_transform(mesh.entity);

			instances[i] = { trans.GetWorldMatrix(), mesh.mesh, mesh.material };
		}

		// TODO: Frustum culling
		std::vector<ui32> indices(instances.size());
		{
			for (ui32 i = 0; i < indices.size(); ++i) indices[i] = i;
		}

		DrawData& drawData = renderer_drawData_get();
		mesh_renderer_mesh_render_forward(instances.data(), indices.data(), indices.size(), false, drawData.viewMatrix, 
			drawData.projectionMatrix, drawData.pOffscreen->renderTarget, drawData.pOffscreen->depthStencil, cmd);
	}

	void mesh_renderer_render(Mesh* mesh, Material* material, ui32 offset, ui32 size, CommandList cmd) {

		GPUBuffer* vBuffers[] = {
			&mesh->GetVertexBuffer(), &g_InstanceBuffer
		};

		ui32 offsets[] = {
			0u, offset * sizeof(MeshData)
		};

		ui32 strides[] = {
			mesh->GetVertexCount() * sizeof(MeshVertex), size * sizeof(MeshData)
		};

		graphics_vertexbuffer_bind(vBuffers, offsets, strides, 2u, cmd);
		graphics_indexbuffer_bind(mesh->GetIndexBuffer(), 0u, cmd);

		GPUBuffer* cBuffers[] = {
			&g_CameraBuffer
		};

		graphics_constantbuffer_bind(cBuffers, 1u, ShaderType_Vertex, cmd);

		cBuffers[0] = &material->GetConstantBuffer();

		graphics_constantbuffer_bind(cBuffers, 1u, ShaderType_Pixel, cmd);

		graphics_draw_indexed(mesh->GetIndexCount(), size, 0u, 0u, 0u, cmd);
	}

	void mesh_renderer_mesh_render_forward(MeshInstance* instances, ui32* indices, ui32 count, bool transparent, const XMMATRIX& VM, const XMMATRIX& PM, GPUImage& renderTarget, GPUImage& depthStencil, CommandList cmd)
	{
		ui32 instance = 0u;

		GPUImage* att[] = {
			&renderTarget, &depthStencil
		};

		// Set CameraData
		{
			CameraBufferData camData;
			camData.projectionMatrix = XMMatrixTranspose(PM);
			graphics_buffer_update(g_CameraBuffer, &camData, sizeof(CameraBufferData), 0u, cmd);
		}

		while (instance < count) {

			ui32 batchSize = count;
			if (batchSize > SV_REND_MESH_INSTANCE_COUNT) {
				batchSize = SV_REND_MESH_INSTANCE_COUNT;
			}

			// Fill instance data
			for (ui32 i = 0; i < batchSize; ++i) {

				ui32 index = indices[i];
				auto& mesh = instances[index];

				g_MeshData[i].modelViewMatrix = XMMatrixTranspose(mesh.tm * VM);

			}

			// Update Material buffers
			{
				Material* currentMaterial = instances[indices[0]].pMaterial;
				for (ui32 i = 1; i < batchSize; ++i) {
					Material* mat = instances[indices[0]].pMaterial;
					if (currentMaterial != mat) {
						if (currentMaterial != nullptr) {
							currentMaterial->UpdateBuffers(cmd);
						}
						currentMaterial = mat;
					}
				}
				if (currentMaterial != nullptr) {
					currentMaterial->UpdateBuffers(cmd);
				}
			}

			// Copy instance data to GPU
			graphics_buffer_update(g_InstanceBuffer, g_MeshData, batchSize * sizeof(MeshData), 0u, cmd);

			// Begin rendering
			graphics_renderpass_begin(g_ForwardRenderPass, att, nullptr, 0.f, 0u, cmd);
			graphics_pipeline_bind(g_ForwardPipeline, cmd);

			Mesh* currentMesh = instances[*indices].pMesh;
			Material* currentMaterial = instances[*indices].pMaterial;

			ui32* beginIndex = indices;
			ui32* endIndex = indices + batchSize;
			ui32 offset = 0u;

			while (indices != endIndex) {

				MeshInstance& mesh = instances[*indices];
				if (mesh.pMesh != currentMesh || mesh.pMaterial != currentMaterial) {

					ui32 batchPos = indices - beginIndex;
					mesh_renderer_render(currentMesh, currentMaterial, offset, batchPos - offset, cmd);

					currentMesh = mesh.pMesh;
					currentMaterial = mesh.pMaterial;
					offset = batchPos;
				}

				indices++;
			}
			mesh_renderer_render(currentMesh, currentMaterial, offset, batchSize - offset, cmd);

			graphics_renderpass_end(cmd);
			// End rendering

			instance += batchSize;
		}
	}

}