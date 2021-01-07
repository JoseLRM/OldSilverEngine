#include "core.h"

#include "mesh_renderer_internal.h"
#include "simulation/task_system.h"
#include "utils/io.h"

namespace sv {

	static MeshRendererContext g_Context[GraphicsLimit_CommandList];
	
	static ShaderLibrary* g_DefShaderLibrary = nullptr;

	static RasterizerState* g_RasterizerState = nullptr;
	static DepthStencilState* g_DepthStencilState = nullptr;

	// GEOMETRY PASS

	static RenderPass* g_RenderPass_Geometry = nullptr;
	static InputLayoutState* g_InputLayoutState_Geometry = nullptr;
	static BlendState* g_BS_Geometry = nullptr;

	static GPUBuffer* g_InstanceBuffer = nullptr;

	static SubShaderID g_SubShader_GeometryVertexShader;
	static SubShaderID g_SubShader_GeometryPixelShader;

	// LIGHTING PASS

	static RenderPass* g_RenderPass_Lighting = nullptr;

	static Shader* g_VS_Lighting = nullptr;
	static Shader* g_PS_Lighting = nullptr;
	static GPUBuffer* g_CBuffer_Lighting = nullptr;

	Result MeshRenderer_internal::initialize()
	{
		// Initialize Context
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				MeshRendererContext& ctx = g_Context[i];
				ctx.meshData = new MeshData[MESH_BATCH_COUNT];

			}
		}

		// Individual Vertex Shader
		{
			graphics_shader_include_write("mesh_vertex", R"(
#name MeshVertex
#shadertype VertexShader

struct VertexInput {
	float3 position : Position;
	float3 normal : Normal;
};

struct InstanceInput {
	matrix tm;
};

struct VertexOutput {
	float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(InstanceInputBuffer, b0) {
	InstanceInput instanceInput;
};

// Free slots
#define FREE_CBUFFER_SLOT0 b1
#define FREE_CBUFFER_SLOT1 b2
#define FREE_CBUFFER_SLOT2 b3
#define FREE_CBUFFER_SLOT3 b4
#define FREE_CBUFFER_SLOT4 b5
#define FREE_CBUFFER_SLOT5 b6
#define FREE_CBUFFER_SLOT6 b7

#userblock MeshVertex

UserVertexOutput main(VertexInput vertexInput)
{
	return meshVertex(vertexInput, instanceInput);
}
			)");
		}

		// Surface Shader
		{
			graphics_shader_include_write("mesh_surface", R"(
#name MeshSurface
#shadertype PixelShader

struct SurfaceOutput {
	float3 diffuse : SV_Target0;
	float3 normal : SV_Target1;
};

#userblock MeshSurface

SurfaceOutput main(UserSurfaceInput input)
{
	return meshSurface(input);
}
			)");
		}

		// Lighting shaders
		{
			svCheck(graphics_shader_compile_fastbin("MeshLightingVertex", ShaderType_Vertex, &g_VS_Lighting, R"(
#include "core.hlsl"
struct Output {
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;
};

Output main(u32 id : SV_VertexID)
{
	Output output;
	
	switch (id)
	{
	case 0:
		output.position = float4(-1.f, -1.f, 0.f, 1.f);
		break;
	case 1:
		output.position = float4( 1.f, -1.f, 0.f, 1.f);
		break;
	case 2:
		output.position = float4(-1.f,  1.f, 0.f, 1.f);
		break;
	case 3:
		output.position = float4( 1.f,  1.f, 0.f, 1.f);
		break;
	}
	
	output.texCoord = (output.position.xy + 1.f) / 2.f;

	return output;
}
			)"));

			svCheck(graphics_shader_compile_fastbin("MeshLightingPixel", ShaderType_Pixel, &g_PS_Lighting, R"(
#include "core.hlsl"
struct Input {
	float2 texCoord : FragTexCoord;
};

struct Output {
	float4 color : SV_Target;
};

struct Light {
	u32 type;
	float3 position;
	float intensity;
	float radius;
	float smoothness;
	float padding;
};

SV_CONSTANT_BUFFER(LightingBuffer, b0) {
	uint2 resolution;
	u32 lightCount;
	Light lights[20];
};

SV_DEFINE_CAMERA(b1);

SV_TEXTURE(DiffuseMap, t0);
SV_TEXTURE(NormalMap, t1);
SV_TEXTURE(DepthMap, t2);

Output main(Input input)
{
	Output output;
	
	// Get geometry data

	uint3 screenPos = uint3(input.texCoord * (float2)resolution, 0);

	float depth = DepthMap.Load(screenPos);
	if (depth == 1.f) discard;
	float3 diffuse = DiffuseMap.Load(screenPos).rgb;
	float3 normal = NormalMap.Load(screenPos).rgb;

	// Compute frag position

	float4 view = float4((input.texCoord - 0.5f) * 2.f, depth, 1.f);
	view = mul(view, camera.ipm);
	view.xyz /= view.w;
	
	float3 pos = view.xyz;
	
	output.color.rgb = float3(0.f, 0.f, 0.f);
	for (u32 i = 0; i < lightCount; ++i) {
	
		Light l = lights[i];
		
		float3 toLight = pos - l.position;
		toLight = normalize(toLight);
	
		float d = max(dot(toLight, normal), 0.f);
		output.color.rgb += diffuse * d;
	}

	
	
	
	output.color.a = 1.f;

	// TEST
	//output.color.rgb = normal + 1.f - depth - diffuse;
	

	return output;
}
			)"));
		}
		// Register ShaderLibrary Type
		{
			ShaderLibraryTypeDesc desc;
			desc.name = "Mesh";
			desc.subShaderCount = 2u;
			desc.subshaderIncludeNames[0] = "mesh_vertex";
			desc.subshaderIncludeNames[1] = "mesh_surface";

			svCheck(matsys_shaderlibrary_type_register(&desc));

			g_SubShader_GeometryVertexShader = matsys_subshader_get("Mesh", "MeshVertex");
			g_SubShader_GeometryPixelShader = matsys_subshader_get("Mesh", "MeshSurface");

			if (g_SubShader_GeometryVertexShader == u32_max) return Result_UnknownError;
			if (g_SubShader_GeometryPixelShader == u32_max) return Result_UnknownError;
		}
		// Create Render Passes
		{
			RenderPassDesc desc;
			desc.attachments.resize(3u);
			
			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].format = GBuffer::FORMAT_DIFFUSE;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			desc.attachments[1].loadOp = AttachmentOperation_Load;
			desc.attachments[1].storeOp = AttachmentOperation_Store;
			desc.attachments[1].format = GBuffer::FORMAT_NORMAL;
			desc.attachments[1].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[1].layout = GPUImageLayout_RenderTarget;
			desc.attachments[1].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[1].type = AttachmentType_RenderTarget;

			desc.attachments[2].loadOp = AttachmentOperation_Load;
			desc.attachments[2].storeOp = AttachmentOperation_Store;
			desc.attachments[2].stencilLoadOp = AttachmentOperation_Load;
			desc.attachments[2].stencilStoreOp = AttachmentOperation_Store;
			desc.attachments[2].format = GBuffer::FORMAT_DEPTHSTENCIL;
			desc.attachments[2].initialLayout = GPUImageLayout_DepthStencil;
			desc.attachments[2].layout = GPUImageLayout_DepthStencil;
			desc.attachments[2].finalLayout = GPUImageLayout_DepthStencil;
			desc.attachments[2].type = AttachmentType_DepthStencil;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Geometry));

			desc.attachments.resize(1u);

			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].format = GBuffer::FORMAT_OFFSCREEN;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Lighting));
		}
		// Create Rasterizer State
		{
			RasterizerStateDesc desc;
			desc.wireframe = false;
			desc.cullMode = RasterizerCullMode_Back;
			desc.clockwise = true;

			svCheck(graphics_rasterizerstate_create(&desc, &g_RasterizerState));
		}
		// Create DepthStencil State
		{
			DepthStencilStateDesc desc;
			desc.depthTestEnabled = true;
			desc.depthWriteEnabled = true;
			desc.depthCompareOp = CompareOperation_Less;
			desc.stencilTestEnabled = false;

			svCheck(graphics_depthstencilstate_create(&desc, &g_DepthStencilState));
		}
		// Create Blend State
		{
			BlendStateDesc desc;
			desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			desc.attachments.resize(2u);
			desc.attachments[0].blendEnabled = false;
			desc.attachments[1].blendEnabled = false;
			desc.attachments[0].colorWriteMask = ColorComponent_All;
			desc.attachments[1].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&desc, &g_BS_Geometry));
			graphics_name_set(g_BS_Geometry, "BS_MeshGeometry");
		}
		// Create Input Layout State
		{
			InputLayoutStateDesc desc;

			desc.slots.resize(1u);
			desc.slots[0].instanced = false;
			desc.slots[0].slot = 0u;
			desc.slots[0].stride = sizeof(MeshVertex);

			desc.elements.resize(2u);
			desc.elements[0u] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };
			desc.elements[1u] = { "Normal", 0u, 0u, sizeof(v3_f32), Format_R32G32B32_FLOAT };
			
			svCheck(graphics_inputlayoutstate_create(&desc, &g_InputLayoutState_Geometry));
		}
		// Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(MeshData);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, &g_InstanceBuffer));

			desc.size = sizeof(LightingData);

			svCheck(graphics_buffer_create(&desc, &g_CBuffer_Lighting));

			graphics_name_set(g_InstanceBuffer, "Mesh Instance Buffer");
			graphics_name_set(g_CBuffer_Lighting, "Mesh Lighting Buffer");
		}

		// Default shader library
		{
			// TEMP
			if (result_fail(matsys_shaderlibrary_create_from_binary(hash_string("SilverEngine/DefaultMesh"), &g_DefShaderLibrary))) {

				const char* src = R"(

#name SilverEngine/DefaultMesh
#type Mesh

#begin MeshVertex

struct UserVertexOutput : VertexOutput {
	float3 normal : FragNormal;
};

SV_DEFINE_CAMERA(FREE_CBUFFER_SLOT0);

UserVertexOutput meshVertex(VertexInput vertex, InstanceInput instance)
{
	UserVertexOutput output;
	
	matrix mvpMatrix = mul(instance.tm, camera.vpm);
	output.position = mul(float4(vertex.position, 1.f), mvpMatrix);
	output.normal = mul(vertex.normal, (float3x3)instance.tm);

	return output;
}

#end

#begin MeshSurface

struct UserSurfaceInput {
	float3 normal : FragNormal;
};

SurfaceOutput meshSurface(UserSurfaceInput input)
{
	SurfaceOutput output;

	output.diffuse = float3(0.7f, 0.7f, 0.7f);
	output.normal = input.normal;

	return output;
}

#end
				)";

				svCheck(matsys_shaderlibrary_create_from_string(src, &g_DefShaderLibrary));
			}
		}

		return Result_Success;
	}

	Result MeshRenderer_internal::close()
	{
		// Deallocate context
		for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i)
			delete[] g_Context[i].meshData;

		svCheck(matsys_shaderlibrary_destroy(g_DefShaderLibrary));

		svCheck(graphics_destroy(g_RenderPass_Geometry));
		svCheck(graphics_destroy(g_RenderPass_Lighting));
		svCheck(graphics_destroy(g_InputLayoutState_Geometry));
		svCheck(graphics_destroy(g_BS_Geometry));
		svCheck(graphics_destroy(g_CBuffer_Lighting));
		svCheck(graphics_destroy(g_DepthStencilState));
		svCheck(graphics_destroy(g_InstanceBuffer));
		svCheck(graphics_destroy(g_RasterizerState));
		svCheck(graphics_destroy(g_VS_Lighting));
		svCheck(graphics_destroy(g_PS_Lighting));

		return Result_Success;
	}

	void MeshRenderer::drawMeshes(GPUImage* renderTarget, GBuffer& gBuffer, CameraBuffer& cameraBuffer, FrameList<MeshInstanceGroup>& meshes, FrameList<LightInstance>& lights, bool optimizeLists, bool depthTest, CommandList cmd)
	{
		if (meshes.empty()) return;

		// Optimize lists
		if (optimizeLists) {
			for (MeshInstanceGroup& group : meshes) {

				FrameList<MeshInstance>& m = *group.pInstances;

				std::sort(m.data(), m.data() + m.size(), [](const MeshInstance& i0, const MeshInstance& i1)
				{
					float toCameraDiference = abs(i0.toCameraDistance - i1.toCameraDistance);
					if (toCameraDiference > 3.f) {
						return i0.material < i1.material;
					}
					else return i0.toCameraDistance < i1.toCameraDistance;
				});
			}
		}

		// GEOMETRY PASS
		{
			MeshRendererContext& ctx = g_Context[cmd];

			graphics_state_unbind(cmd);
			graphics_event_begin("Mesh_DeferredGeometryPass", cmd);

			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_constantbuffer_bind(g_InstanceBuffer, 0u, ShaderType_Vertex, cmd);
			graphics_blendstate_bind(g_BS_Geometry, cmd);
			graphics_inputlayoutstate_bind(g_InputLayoutState_Geometry, cmd);

			if (depthTest)
				graphics_depthstencilstate_bind(g_DepthStencilState, cmd);
			else
				graphics_depthstencilstate_unbind(cmd);

			for (const MeshInstanceGroup& group : meshes) {

				// Prepare
				{
					// TODO: Face culling
					graphics_rasterizerstate_bind(g_RasterizerState, cmd);
					
					
				}

				// Default binding
				Material* material = nullptr;
				Mesh* mesh = nullptr;
				MeshData meshData;

				matsys_shaderlibrary_bind_camerabuffer(g_DefShaderLibrary, cameraBuffer, cmd);
				matsys_shaderlibrary_bind_subshader(g_DefShaderLibrary, g_SubShader_GeometryVertexShader, cmd);
				matsys_shaderlibrary_bind_subshader(g_DefShaderLibrary, g_SubShader_GeometryPixelShader, cmd);


				// TODO: Instanced Rendering
				for (const MeshInstance& instance : *group.pInstances) {

					if (instance.pMesh == nullptr)
						continue;

					// Bind Mesh
					if (instance.pMesh != mesh) {
						mesh = instance.pMesh;

						graphics_vertexbuffer_bind(mesh->vertexBuffer, 0u, 0u, cmd);
						graphics_indexbuffer_bind(mesh->indexBuffer, 0u, cmd);
					}

					// Bind Material
					if (instance.material != material) {
						graphics_shader_unbind_commandlist(cmd);

						if (instance.material) {

							ShaderLibrary* lib = matsys_material_get_shaderlibrary(instance.material);
							matsys_shaderlibrary_bind_subshader(lib, g_SubShader_GeometryVertexShader, cmd);
							matsys_shaderlibrary_bind_subshader(lib, g_SubShader_GeometryPixelShader, cmd);

							matsys_shaderlibrary_bind_camerabuffer(lib, cameraBuffer, cmd);

							matsys_material_bind(instance.material, g_SubShader_GeometryVertexShader, cmd);
							matsys_material_bind(instance.material, g_SubShader_GeometryPixelShader, cmd);
						}
						else {

							matsys_shaderlibrary_bind_camerabuffer(g_DefShaderLibrary, cameraBuffer, cmd);
							matsys_shaderlibrary_bind_subshader(g_DefShaderLibrary, g_SubShader_GeometryVertexShader, cmd);
							matsys_shaderlibrary_bind_subshader(g_DefShaderLibrary, g_SubShader_GeometryPixelShader, cmd);
						}
					}

					// Instance Data
					meshData.tm = XMMatrixTranspose(instance.tm);
					graphics_buffer_update(g_InstanceBuffer, &meshData, sizeof(MeshData), 0u, cmd);

					GPUImage* att[] = {
						gBuffer.diffuse,
						gBuffer.normal,
						gBuffer.depthStencil
					};

					graphics_renderpass_begin(g_RenderPass_Geometry, att, nullptr, 0.f, 0u, cmd);

					graphics_draw_indexed((u32)mesh->indices.size(), 1u, 0u, 0u, 0u, cmd);

					graphics_renderpass_end(cmd);

				}
			}

			graphics_event_end(cmd);
		}

		// LIGHTING PASS
		{
			graphics_event_begin("Mesh_DeferredLightingPass", cmd);

			// Change gBuffer layout to Shader Resources
			GPUBarrier barriers[3u];

			barriers[0] = GPUBarrier::Image(gBuffer.diffuse, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
			barriers[1] = GPUBarrier::Image(gBuffer.normal, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
			barriers[2] = GPUBarrier::Image(gBuffer.depthStencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencilReadOnly);
			graphics_barrier(barriers, 3u, cmd);

			// Prepare
			graphics_depthstencilstate_unbind(cmd);
			graphics_inputlayoutstate_unbind(cmd);
			graphics_blendstate_unbind(cmd);
			
			graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);

			//TODO: If I call this, it crashes in draw call :(
			//graphics_resources_unbind(cmd);
			GPUImage* textures[] = {
				gBuffer.diffuse,
				gBuffer.normal,
				gBuffer.depthStencil
			};
			graphics_image_bind_array(textures, 3u, 0u, ShaderType_Pixel, cmd);
			graphics_constantbuffer_bind(g_CBuffer_Lighting, 0u, ShaderType_Pixel, cmd);
			graphics_constantbuffer_bind(cameraBuffer.gpuBuffer, 1u, ShaderType_Pixel, cmd);

			graphics_shader_bind(g_VS_Lighting, cmd);
			graphics_shader_bind(g_PS_Lighting, cmd);

			GPUImage* att[] = {
				renderTarget
			};

			// Light data
			LightingData data;
			v2_u32 res = gBuffer.getResolution();
			data.resolutionWidth = res.x;
			data.resolutionHeight = res.y;

			data.lightCount = std::min((u32)lights.size(), LIGHT_COUNT);
			for (u32 i = 0u; i < data.lightCount; ++i) {
				MeshLightData& ld = data.lights[i];
				const LightInstance& li = lights[i];

				ld.type = li.type;
				ld.intensity = li.intensity;
				
				switch (li.type)
				{
				case LightType_Point:
					ld.positionOrDirection = li.point.position + cameraBuffer.position;
					ld.radius = li.point.range;
					ld.smoothness = li.point.smoothness;
					break;
				}
			}

			graphics_buffer_update(g_CBuffer_Lighting, &data, sizeof(LightingData), 0u, cmd);

			graphics_renderpass_begin(g_RenderPass_Lighting, att, nullptr, 1.f, 0u, cmd);

			graphics_draw(4u, 1u, 0u, 0u, cmd);

			graphics_renderpass_end(cmd);

			// Change gBuffer layout to Render Targets
			barriers[0] = GPUBarrier::Image(gBuffer.diffuse, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
			barriers[1] = GPUBarrier::Image(gBuffer.normal, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
			barriers[2] = GPUBarrier::Image(gBuffer.depthStencil, GPUImageLayout_DepthStencilReadOnly, GPUImageLayout_DepthStencil);
			graphics_barrier(barriers, 3u, cmd);

			graphics_event_end(cmd);
		}
	}

}