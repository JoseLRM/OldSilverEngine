#include "core.h"

#include "renderer/renderer_internal.h"

namespace sv {

	struct MeshData {
		XMMATRIX modelViewMatrix;
	};

	struct CameraBufferData {
		XMMATRIX projectionMatrix;
	};

	struct LightBufferData {
		LightInstance light[SV_REND_FORWARD_LIGHT_COUNT];
	};

	static GPUBuffer g_InstanceBuffer;
	static GPUBuffer g_CameraBuffer;
	static GPUBuffer g_LightBuffer;

	// TODO: not global
	static MeshData g_MeshData[SV_REND_MESH_INSTANCE_COUNT];

	static RenderPass			g_ForwardRenderPass;
	static GraphicsPipeline		g_ForwardPipeline;
	static Shader				g_ForwardVertexShader;
	static Shader				g_ForwardPixelShader;
	static InputLayoutState		g_ForwardInputLayoutState;
	static DepthStencilState	g_ForwardDepthStencilState;
	static RasterizerState		g_ForwardRasterizerState;

	Result renderer_mesh_initialize()
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
		// Light Buffer
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(LightBufferData);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, g_LightBuffer));
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

		return Result_Success;
	}

	Result renderer_mesh_close()
	{

		return Result_Success;
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

		graphics_vertexbuffer_bind(vBuffers, offsets, 2u, 0u, cmd);
		graphics_indexbuffer_bind(mesh->GetIndexBuffer(), 0u, cmd);

		GPUBuffer* cBuffers[] = {
			&g_CameraBuffer, nullptr
		};

		graphics_constantbuffer_bind(cBuffers, 1u, 0u, ShaderType_Vertex, cmd);

		cBuffers[0] = &material->GetConstantBuffer();
		cBuffers[1] = &g_LightBuffer;

		graphics_constantbuffer_bind(cBuffers, 2u, 0u, ShaderType_Pixel, cmd);

		graphics_draw_indexed(mesh->GetIndexCount(), size, 0u, 0u, 0u, cmd);
	}

	void renderer_mesh_forward_rendering(const ForwardRenderingDesc* desc, CommandList cmd)
	{
		ui32 instance = 0u;

		const MeshInstance* instances = desc->pInstances;
		ui32 count = desc->count;
		const ui32* indices = desc->pIndices;
		XMMATRIX VM = *desc->pViewMatrix;
		XMMATRIX PM = *desc->pProjectionMatrix;
		ui32 lightCount = desc->lightCount;

		GPUImage* att[] = {
			desc->pRenderTarget, desc->pDepthStencil
		};

		// Set CameraData
		{
			CameraBufferData camData;
			camData.projectionMatrix = XMMatrixTranspose(PM);
			graphics_buffer_update(g_CameraBuffer, &camData, sizeof(CameraBufferData), 0u, cmd);
		}

		while (lightCount > 0) {

			ui32 drawLightCount = std::min(SV_REND_FORWARD_LIGHT_COUNT, lightCount);
			
			// Set Light Data
			{
				LightBufferData lightData{};
				memcpy(lightData.light, desc->lights, drawLightCount * sizeof(LightInstance));
				graphics_buffer_update(g_LightBuffer, &lightData, sizeof(LightBufferData), 0u, cmd);
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

					g_MeshData[i].modelViewMatrix = XMMatrixTranspose(mesh.modelViewMatrix);

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

				const ui32* beginIndex = indices;
				const ui32* endIndex = indices + batchSize;
				ui32 offset = 0u;

				while (indices != endIndex) {

					const MeshInstance& mesh = instances[*indices];
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

			lightCount -= drawLightCount;
		}

	}

}