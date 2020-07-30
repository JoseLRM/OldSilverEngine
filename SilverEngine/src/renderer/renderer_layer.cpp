#include "core.h"

#include "graphics/graphics.h"
#include "graphics/graphics_properties.h"
#include "renderer.h"
#include "scene/Scene.h"
#include "DrawData.h"

using namespace sv;

namespace _sv {

	// Sprite Primitives

	static GraphicsPipeline g_SpriteOpaquePipeline;
	static RenderPass g_SpriteFirstRenderPass;
	static RenderPass g_SpriteRenderPass;
	static Shader g_SpriteVertexShader;
	static Shader g_SpritePixelShader;
	static Buffer g_SpriteIndexBuffer;
	static Buffer g_SpriteVertexBuffer;


	static RenderLayer g_DefRenderLayer;

	struct SpriteVertex {
		vec4 position;
		vec2 texCoord;
		Color color;
	};

	static SpriteVertex g_SpriteData[SV_REND_LAYER_BATCH_COUNT * 4u];

	constexpr void ComputeIndexData(ui32* indices)
	{
		ui32 indexCount = SV_REND_LAYER_BATCH_COUNT * 6u;
		ui32 index = 0u;
		for (ui32 i = 0; i < indexCount; ) {
			
			indices[i++] = index;
			indices[i++] = index + 1;
			indices[i++] = index + 2;
			indices[i++] = index + 1;
			indices[i++] = index + 3;
			indices[i++] = index + 2;

			index += 4u;
		}
	}

	bool renderer_layer_initialize()
	{
		sv::Image& offscreen = renderer_offscreen_get();

		// Sprite Buffers
		{
			SV_GFX_BUFFER_DESC desc;
			desc.bufferType = SV_GFX_BUFFER_TYPE_VERTEX;
			desc.CPUAccess = SV_GFX_CPU_ACCESS_WRITE;
			desc.usage = SV_GFX_USAGE_DEFAULT;
			desc.pData = nullptr;
			desc.size = SV_REND_LAYER_BATCH_COUNT * sizeof(SpriteVertex) * 4u;

			svCheck(graphics_buffer_create(&desc, g_SpriteVertexBuffer));

			// Index Buffer
			ui32 indexData[SV_REND_LAYER_BATCH_COUNT * 6u];
			ComputeIndexData(indexData);
			
			desc.bufferType = SV_GFX_BUFFER_TYPE_INDEX;
			desc.CPUAccess = SV_GFX_CPU_ACCESS_NONE;
			desc.usage = SV_GFX_USAGE_STATIC;
			desc.pData = indexData;
			desc.size = SV_REND_LAYER_BATCH_COUNT * 6u * sizeof(ui32);
			desc.indexType = SV_GFX_INDEX_TYPE_32_BITS;

			svCheck(graphics_buffer_create(&desc, g_SpriteIndexBuffer));

		}
		// Sprite Shaders
		{
			SV_GFX_SHADER_DESC desc;
			desc.filePath = "shaders/SpriteVertex.shader";
			desc.shaderType = SV_GFX_SHADER_TYPE_VERTEX;

			svCheck(graphics_shader_create(&desc, g_SpriteVertexShader));

			desc.filePath = "shaders/SpritePixel.shader";
			desc.shaderType = SV_GFX_SHADER_TYPE_PIXEL;

			svCheck(graphics_shader_create(&desc, g_SpritePixelShader));
		}
		// Sprite RenderPass
		{
			SV_GFX_ATTACHMENT_DESC att;
			att.loadOp = SV_GFX_LOAD_OP_LOAD;
			att.storeOp = SV_GFX_STORE_OP_STORE;
			att.stencilLoadOp = SV_GFX_LOAD_OP_DONT_CARE;
			att.stencilStoreOp = SV_GFX_STORE_OP_DONT_CARE;
			att.format = offscreen->GetFormat();
			att.initialLayout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
			att.layout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
			att.finalLayout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
			att.type = SV_GFX_ATTACHMENT_TYPE_RENDER_TARGET;

			SV_GFX_RENDERPASS_DESC desc;
			desc.attachments.push_back(att);

			svCheck(graphics_renderpass_create(&desc, g_SpriteRenderPass));
			desc.attachments[0].loadOp = SV_GFX_LOAD_OP_CLEAR;
			svCheck(graphics_renderpass_create(&desc, g_SpriteFirstRenderPass));
		}
		// Sprite Opaque Pipeline
		{
			SV_GFX_INPUT_LAYOUT_DESC inputLayout;
			inputLayout.slots.push_back({ 0u, sizeof(SpriteVertex), false });

			inputLayout.elements.push_back({ 0u, "Position", 0u, 0u, SV_GFX_FORMAT_R32G32B32A32_FLOAT });
			inputLayout.elements.push_back({ 0u, "TexCoord", 0u, 4u * sizeof(float), SV_GFX_FORMAT_R32G32_FLOAT });
			inputLayout.elements.push_back({ 0u, "Color", 0u, 6u * sizeof(float), SV_GFX_FORMAT_R8G8B8A8_UNORM });

			SV_GFX_GRAPHICS_PIPELINE_DESC desc;
			desc.pVertexShader = &g_SpriteVertexShader;
			desc.pPixelShader = &g_SpritePixelShader;
			desc.pGeometryShader = nullptr;
			desc.pInputLayout = &inputLayout;
			desc.pBlendState = nullptr;
			desc.pRasterizerState = nullptr;
			desc.pDepthStencilState = nullptr;
			desc.topology = SV_GFX_TOPOLOGY_TRIANGLES;

			svCheck(graphics_pipeline_create(&desc, g_SpriteOpaquePipeline));
		}

		return true;
	}

	bool renderer_layer_close()
	{
		svCheck(graphics_destroy(g_SpriteOpaquePipeline));
		svCheck(graphics_destroy(g_SpriteRenderPass));
		svCheck(graphics_destroy(g_SpriteFirstRenderPass));
		svCheck(graphics_destroy(g_SpriteVertexShader));
		svCheck(graphics_destroy(g_SpritePixelShader));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));

		return true;
	}

	void ResetRenderLayersSystem(Scene& scene, Entity entity, BaseComponent** comp, float dt)
	{
		RenderLayerComponent& rlComp = *reinterpret_cast<RenderLayerComponent*>(comp[0]);
		rlComp.renderLayer.Reset();
	}

	void CreateSpritesInstancesSystem(Scene& scene, Entity entity, BaseComponent** comp, float dt)
	{
		SpriteComponent& sprComp = *reinterpret_cast<SpriteComponent*>(comp[0]);
		Transform trans = scene.GetTransform(entity);

		if (sprComp.renderLayer == SV_INVALID_ENTITY) {
			g_DefRenderLayer.sprites.Emplace(trans.GetWorldMatrix(), sprComp.color);
		}
		else {
			RenderLayerComponent& rlComp = *reinterpret_cast<RenderLayerComponent*>(scene.GetComponent<RenderLayerComponent>(sprComp.renderLayer));
			rlComp.renderLayer.sprites.Emplace(trans.GetWorldMatrix(), sprComp.color);
		}
	}

	void GetRenderLayers(Scene& scene, Entity entity, BaseComponent** comp, float dt)
	{
		RenderLayerComponent& rlComp = *reinterpret_cast<RenderLayerComponent*>(comp[0]);
		renderer_drawdata_get().renderLayers.push_back(&rlComp.renderLayer);
	}

	void renderer_layer_begin()
	{
		g_DefRenderLayer.Reset();
	}

	void renderer_layer_end()
	{
		DrawData& drawData = renderer_drawdata_get();
		auto& layers = drawData.renderLayers;

		// Sort RenderLayers
		std::sort(layers.begin(), layers.end(), [](const RenderLayer* rl0, const RenderLayer* rl1) {
			return (rl0->sortValue == rl1->sortValue) ? rl0->sortValue < rl1->sortValue : rl0 < rl1;
		});
	}

	void renderer_layer_prepare_scene(sv::Scene& scene)
	{
		// Reset RenderLayers, Create Sprites Instances and Get RenderLayers

		SV_ECS_SYSTEM_DESC systems[3];

		CompID reqComp0[] = {
			RenderLayerComponent::ID
		};
		CompID reqComp1[] = {
			SpriteComponent::ID
		};
		CompID reqComp2[] = {
			RenderLayerComponent::ID
		};

		systems[0].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		systems[0].system = ResetRenderLayersSystem;
		systems[0].pRequestedComponents = reqComp0;
		systems[0].requestedComponentsCount = 1u;
		systems[0].optionalComponentsCount = 0u;

		systems[1].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		systems[1].system = CreateSpritesInstancesSystem;
		systems[1].pRequestedComponents = reqComp1;
		systems[1].requestedComponentsCount = 1u;
		systems[1].optionalComponentsCount = 0u;

		systems[2].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		systems[2].system = GetRenderLayers;
		systems[2].pRequestedComponents = reqComp2;
		systems[2].requestedComponentsCount = 1u;
		systems[2].optionalComponentsCount = 0u;

		scene.ExecuteSystems(systems, 3u, 0.f);

		renderer_drawdata_get().renderLayers.push_back(&g_DefRenderLayer);

	}

	void renderer_layer_render(CommandList cmd)
	{
		DrawData& drawData = renderer_drawdata_get();
		auto& layers = drawData.renderLayers;

		// TODO: frustum culling

		Image* att[] = {
			drawData.pOutput
		};

		Buffer* vBuffers[] = {
			&g_SpriteVertexBuffer,
		};
		ui32 offsets[] = {
			0u,
		};
		ui32 strides[] = {
			0u,
		};

		bool firstRenderPass = true;

		for (auto it = layers.begin(); it != layers.end(); ++it) {
			auto& sprites = (*it)->sprites;

			ui32 i = 0u;

			while (i < sprites.Size()) {

				ui32 j = 0u;
				ui32 batchSize = SV_REND_LAYER_BATCH_COUNT;
				if (i + batchSize > sprites.Size()) {
					batchSize = sprites.Size() - i;
				}

				while (j < batchSize) {

					SpriteInstance& spr = sprites[i + j];

					// Compute Matrices form WorldSpace to ScreenSpace
					XMMATRIX matrix = spr.tm * drawData.viewProjectionMatrix;

					XMVECTOR pos0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
					XMVECTOR pos1 = XMVectorSet( 0.5f, -0.5f, 0.f, 1.f);
					XMVECTOR pos2 = XMVectorSet(-0.5f,  0.5f, 0.f, 1.f);
					XMVECTOR pos3 = XMVectorSet( 0.5f,  0.5f, 0.f, 1.f);

					pos0 = XMVector4Transform(pos0, matrix);
					pos1 = XMVector4Transform(pos1, matrix);
					pos2 = XMVector4Transform(pos2, matrix);
					pos3 = XMVector4Transform(pos3, matrix);

					ui32 index = j * 4u;
					g_SpriteData[index + 0] = { pos0, {0.f, 0.f}, spr.color };
					g_SpriteData[index + 1] = { pos1, {0.f, 0.f}, spr.color };
					g_SpriteData[index + 2] = { pos2, {0.f, 0.f}, spr.color };
					g_SpriteData[index + 3] = { pos3, {0.f, 0.f}, spr.color };

					j++;
				}

				strides[0] = batchSize * sizeof(SpriteVertex) * 4u;

				graphics_buffer_update(g_SpriteVertexBuffer, g_SpriteData, batchSize * sizeof(SpriteVertex) * 4u, 0u, cmd);

				if (firstRenderPass) {
					firstRenderPass = false;
					graphics_renderpass_begin(g_SpriteFirstRenderPass, att, nullptr, 1.f, 0u, cmd);
				}
				else
					graphics_renderpass_begin(g_SpriteRenderPass, att, nullptr, 1.f, 0u, cmd);

				graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
				graphics_vertexbuffer_bind(vBuffers, offsets, strides, 1u, cmd);
				graphics_pipeline_bind(g_SpriteOpaquePipeline, cmd);

				graphics_draw_indexed(batchSize * 6u, 1u, 0u, 0u, 0u, cmd);

				graphics_renderpass_end(cmd);

				i += j;
			}
		}
		
	}

}