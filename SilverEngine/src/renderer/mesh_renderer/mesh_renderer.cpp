#include "core.h"

#include "mesh_renderer_internal.h"
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

	static RenderPass		g_ForwardRenderPass;
	static GraphicsPipeline g_ForwardPipeline;
	static Shader			g_ForwardVertexShader;
	static Shader			g_ForwardPixelShader;

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
			ShaderDesc desc;
			desc.filePath = "shaders/ForwardVertex.shader";
			desc.shaderType = ShaderType_Vertex;

			svCheck(graphics_shader_create(&desc, g_ForwardVertexShader));

			desc.filePath = "shaders/ForwardPixel.shader";
			desc.shaderType = ShaderType_Pixel;

			svCheck(graphics_shader_create(&desc, g_ForwardPixelShader));
		}
		{
			InputLayoutDesc inputLayout;
			inputLayout.slots.resize(2u);

			inputLayout.slots[0].instanced = false;
			inputLayout.slots[0].slot = 0u;
			inputLayout.slots[0].stride = sizeof(MeshVertex);

			inputLayout.slots[1].instanced = true;
			inputLayout.slots[1].slot = 1u;
			inputLayout.slots[1].stride = sizeof(MeshData);

			inputLayout.elements.resize(7u);

			inputLayout.elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };
			inputLayout.elements[1] = { "Normal", 0u, 0u, 3 * sizeof(float), Format_R32G32B32_FLOAT };
			inputLayout.elements[2] = { "TexCoord", 0u, 0u, 6 * sizeof(float), Format_R32G32_FLOAT };
			
			inputLayout.elements[3] = { "ModelViewMatrix", 0u, 1u, 0u * sizeof(float), Format_R32G32B32A32_FLOAT };
			inputLayout.elements[4] = { "ModelViewMatrix", 1u, 1u, 4u * sizeof(float), Format_R32G32B32A32_FLOAT };
			inputLayout.elements[5] = { "ModelViewMatrix", 2u, 1u, 8u * sizeof(float), Format_R32G32B32A32_FLOAT };
			inputLayout.elements[6] = { "ModelViewMatrix", 3u, 1u, 12u * sizeof(float), Format_R32G32B32A32_FLOAT };

			RasterizerStateDesc rasterizerState;
			rasterizerState.wireframe = false;
			rasterizerState.lineWidth = 1.f;
			rasterizerState.cullMode = RasterizerCullMode_Back;
			rasterizerState.clockwise = true;

			DepthStencilStateDesc depthStencilState;
			depthStencilState.depthTestEnabled = true;
			depthStencilState.depthWriteEnabled = true;
			depthStencilState.depthCompareOp = CompareOperation_LessOrEqual;
			depthStencilState.stencilTestEnabled = false;
			depthStencilState.readMask = 0xFF;
			depthStencilState.writeMask = 0xFF;

			GraphicsPipelineDesc desc;
			desc.pVertexShader = &g_ForwardVertexShader;
			desc.pPixelShader = &g_ForwardPixelShader;
			desc.pGeometryShader = nullptr;
			desc.pInputLayout = &inputLayout;
			desc.pBlendState = nullptr;
			desc.pRasterizerState = &rasterizerState;
			desc.pDepthStencilState = &depthStencilState;
			desc.topology = GraphicsTopology_Triangles;

			svCheck(graphics_pipeline_create(&desc, g_ForwardPipeline));
		}

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

			instances[i] = { trans.GetWorldMatrix(), reinterpret_cast<Mesh_internal*>(mesh.mesh), reinterpret_cast<Material_internal*>(mesh.material) };
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

	void mesh_renderer_render(Mesh_internal* mesh, Material_internal* material, ui32 offset, ui32 size, CommandList cmd) {

		GPUBuffer* vBuffers[] = {
			&mesh->vertexBuffer, &g_InstanceBuffer
		};

		ui32 offsets[] = {
			0u, offset * sizeof(MeshData)
		};

		ui32 strides[] = {
			mesh->vertexCount * sizeof(MeshVertex), size * sizeof(MeshData)
		};

		graphics_vertexbuffer_bind(vBuffers, offsets, strides, 2u, cmd);
		graphics_indexbuffer_bind(mesh->indexBuffer, 0u, cmd);

		GPUBuffer* cBuffers[] = {
			&g_CameraBuffer
		};

		graphics_constantbuffer_bind(cBuffers, 1u, ShaderType_Vertex, cmd);

		cBuffers[0] = &material->buffer;

		graphics_constantbuffer_bind(cBuffers, 1u, ShaderType_Pixel, cmd);

		graphics_draw_indexed(mesh->indexCount, size, 0u, 0u, 0u, cmd);
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
			camData.projectionMatrix = PM;

			if (graphics_properties_get().transposedMatrices) 
				camData.projectionMatrix = XMMatrixTranspose(camData.projectionMatrix);

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

				g_MeshData[i].modelViewMatrix = mesh.tm * VM;

			}

			// Transpose matrices
			if (graphics_properties_get().transposedMatrices) {
				for(ui32 i = 0; i < batchSize; ++i)
					g_MeshData[i].modelViewMatrix = XMMatrixTranspose(g_MeshData[i].modelViewMatrix);
			}

			// Copy instance data to GPU
			graphics_buffer_update(g_InstanceBuffer, g_MeshData, batchSize * sizeof(MeshData), 0u, cmd);

			// Begin rendering
			graphics_renderpass_begin(g_ForwardRenderPass, att, nullptr, 0.f, 0u, cmd);
			graphics_pipeline_bind(g_ForwardPipeline, cmd);

			Mesh_internal* currentMesh = instances[*indices].pMesh;
			Material_internal* currentMaterial = instances[*indices].pMaterial;

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

	// Mesh

	bool mesh_create(ui32 vertexCount, ui32 indexCount, Mesh* pMesh)
	{
		if (vertexCount == 0u && indexCount == 0u) return false;

		Mesh_internal& mesh = *new Mesh_internal();
		
		mesh.positions.resize(vertexCount, { 0.f, 0.f, 0.f });
		mesh.normals.resize(vertexCount, { 0.f, 1.f, 0.f });
		mesh.texCoords.resize(vertexCount, { 0.f, 0.f });

		mesh.indices.resize(indexCount, 0u);

		mesh.vertexCount = vertexCount;
		mesh.indexCount = indexCount;

		*pMesh = &mesh;
		return true;
	}

	bool mesh_destroy(Mesh mesh_)
	{
		if (mesh_ == nullptr) return true;

		Mesh_internal& mesh = *reinterpret_cast<Mesh_internal*>(mesh_);

		graphics_destroy(mesh.vertexBuffer);
		graphics_destroy(mesh.indexBuffer);

		delete &mesh;
		return true;
	}

	void mesh_positions_set(Mesh mesh_, void* positions, ui32 offset, ui32 count)
	{
		SV_ASSERT(mesh_ != nullptr);
		Mesh_internal& mesh = *reinterpret_cast<Mesh_internal*>(mesh_);

		SV_ASSERT(!mesh.vertexBuffer.IsValid() && !mesh.indexBuffer.IsValid()); 
		SV_ASSERT(offset + count <= mesh.vertexCount);
		
		memcpy(mesh.positions.data() + offset, positions, count * sizeof(vec3));
	}

	void mesh_normals_set(Mesh mesh_, void* normals, ui32 offset, ui32 count)
	{
		SV_ASSERT(mesh_ != nullptr);
		Mesh_internal& mesh = *reinterpret_cast<Mesh_internal*>(mesh_);

		SV_ASSERT(!mesh.vertexBuffer.IsValid() && !mesh.indexBuffer.IsValid());
		SV_ASSERT(offset + count <= mesh.vertexCount);

		memcpy(mesh.normals.data() + offset, normals, count * sizeof(vec3));
	}

	void mesh_texCoord_set(Mesh mesh_, void* texCoord, ui32 offset, ui32 count)
	{
		SV_ASSERT(mesh_ != nullptr);
		Mesh_internal& mesh = *reinterpret_cast<Mesh_internal*>(mesh_);

		SV_ASSERT(!mesh.vertexBuffer.IsValid() && !mesh.indexBuffer.IsValid());
		SV_ASSERT(offset + count <= mesh.vertexCount);

		memcpy(mesh.texCoords.data() + offset, texCoord, count * sizeof(vec2));
	}

	void mesh_indices_set(Mesh mesh_, ui32* indices, ui32 offset, ui32 count)
	{
		SV_ASSERT(mesh_ != nullptr);
		Mesh_internal& mesh = *reinterpret_cast<Mesh_internal*>(mesh_);

		SV_ASSERT(!mesh.vertexBuffer.IsValid() && !mesh.indexBuffer.IsValid());
		SV_ASSERT(offset + count <= mesh.indexCount);

		memcpy(mesh.indices.data() + offset, indices, count * sizeof(ui32));
	}

	bool mesh_initialize(Mesh mesh_)
	{
		SV_ASSERT(mesh_ != nullptr);
		Mesh_internal& mesh = *reinterpret_cast<Mesh_internal*>(mesh_);

		// Vertex Buffer
		MeshVertex* vertices = new MeshVertex[mesh.vertexCount];
		
		for (ui32 i = 0; i < mesh.vertexCount; ++i) {
			vertices[i].position = mesh.positions[i];
		}
		for (ui32 i = 0; i < mesh.vertexCount; ++i) {
			vertices[i].normal = mesh.normals[i];
		}
		for (ui32 i = 0; i < mesh.vertexCount; ++i) {
			vertices[i].texCoord = mesh.texCoords[i];
		}

		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Vertex;
		desc.usage = ResourceUsage_Static;
		desc.CPUAccess = CPUAccess_None;
		desc.size = sizeof(MeshVertex) * mesh.vertexCount;
		desc.pData = vertices;

		svCheck(graphics_buffer_create(&desc, mesh.vertexBuffer));

		delete[] vertices;

		// Index Buffer
		desc.bufferType = GPUBufferType_Index;
		desc.size = sizeof(ui32) * mesh.indexCount;
		desc.pData = mesh.indices.data();
		desc.indexType = IndexType_32;

		svCheck(graphics_buffer_create(&desc, mesh.indexBuffer));
		
		return true;
	}

	// Material

	bool material_create(MaterialData& initialData, Material* pMaterial)
	{
		// Create
		Material_internal& material = *new Material_internal();
		material.data = initialData;
		material.modified = false;
		
		// Create Constant Shader
		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Constant;
		desc.usage = ResourceUsage_Default;
		desc.CPUAccess = CPUAccess_Write;
		desc.size = sizeof(MaterialData);
		desc.pData = &initialData;

		svCheck(graphics_buffer_create(&desc, material.buffer));

		*pMaterial = &material;
		return true;
	}

	bool material_destroy(Material material_)
	{
		if (material_ == nullptr) return true;

		Material_internal& material = *reinterpret_cast<Material_internal*>(material_);

		svCheck(graphics_destroy(material.buffer));

		delete &material;
		return true;
	}

	void material_data_set(Material material_, const MaterialData& data)
	{
		SV_ASSERT(material_ != nullptr);
		Material_internal& material = *reinterpret_cast<Material_internal*>(material_);
		material.data = data;
		material.modified = true;
	}

	const MaterialData& material_data_get(Material material_)
	{
		SV_ASSERT(material_ != nullptr);
		Material_internal& material = *reinterpret_cast<Material_internal*>(material_);
		return material.data;
	}

}