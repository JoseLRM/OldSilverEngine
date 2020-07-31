#include "core.h"

#include "graphics/graphics.h"
#include "graphics/graphics_properties.h"
#include "renderer.h"
#include "renderer_components.h"

using namespace sv;

namespace _sv {

	// Render Layers

	static std::vector<std::unique_ptr<RenderLayer>> g_RenderLayers;
	static RenderLayer* g_DefRenderLayer;

	// Sprite Primitives

	static GraphicsPipeline g_SpriteOpaquePipeline;
	static GraphicsPipeline g_SpriteTransparentPipeline;
	static RenderPass g_SpriteOpaqueRenderPass;
	static RenderPass g_SpriteTransparentRenderPass;

	static Shader g_SpriteVertexShader;
	static Shader g_SpritePixelShader;
	static Buffer g_SpriteIndexBuffer;
	static Buffer g_SpriteVertexBuffer;
	static Image g_SpriteWhiteTexture;
	static Sampler g_SpriteDefSampler;

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
		// Create Default RenderLayer
		g_DefRenderLayer = reinterpret_cast<RenderLayer*>(renderer_layer_create(0, SV_REND_SORT_MODE_NONE, false));

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
			SV_GFX_RENDERPASS_DESC desc;
			desc.attachments.resize(2);

			desc.attachments[0].loadOp = SV_GFX_LOAD_OP_LOAD;
			desc.attachments[0].storeOp = SV_GFX_STORE_OP_STORE;
			desc.attachments[0].stencilLoadOp = SV_GFX_LOAD_OP_DONT_CARE;
			desc.attachments[0].stencilStoreOp = SV_GFX_STORE_OP_DONT_CARE;
			desc.attachments[0].format = SV_REND_OFFSCREEN_FORMAT;
			desc.attachments[0].initialLayout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
			desc.attachments[0].layout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
			desc.attachments[0].finalLayout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
			desc.attachments[0].type = SV_GFX_ATTACHMENT_TYPE_RENDER_TARGET;
			
			desc.attachments[1].loadOp = SV_GFX_LOAD_OP_DONT_CARE;
			desc.attachments[1].storeOp = SV_GFX_STORE_OP_DONT_CARE;
			desc.attachments[1].stencilLoadOp = SV_GFX_LOAD_OP_LOAD;
			desc.attachments[1].stencilStoreOp = SV_GFX_STORE_OP_STORE;
			desc.attachments[1].format = SV_GFX_FORMAT_D24_UNORM_S8_UINT;
			desc.attachments[1].initialLayout = SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL;
			desc.attachments[1].layout = SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL;
			desc.attachments[1].finalLayout = SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL;
			desc.attachments[1].type = SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL;

			svCheck(graphics_renderpass_create(&desc, g_SpriteOpaqueRenderPass));

			desc.attachments[1].stencilStoreOp = SV_GFX_STORE_OP_DONT_CARE;
			desc.attachments[1].initialLayout = SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY;
			desc.attachments[1].layout = SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY;
			desc.attachments[1].finalLayout = SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY;

			svCheck(graphics_renderpass_create(&desc, g_SpriteTransparentRenderPass));
		}
		// Sprite Pipelines
		{
			SV_GFX_INPUT_LAYOUT_DESC inputLayout;
			inputLayout.slots.push_back({ 0u, sizeof(SpriteVertex), false });

			inputLayout.elements.push_back({ 0u, "Position", 0u, 0u, SV_GFX_FORMAT_R32G32B32A32_FLOAT });
			inputLayout.elements.push_back({ 0u, "TexCoord", 0u, 4u * sizeof(float), SV_GFX_FORMAT_R32G32_FLOAT });
			inputLayout.elements.push_back({ 0u, "Color", 0u, 6u * sizeof(float), SV_GFX_FORMAT_R8G8B8A8_UNORM });

			//SV_GFX_DEPTHSTENCIL_STATE_DESC depthStencilState;
			//depthStencilState.depthTestEnabled = false;
			//depthStencilState.depthWriteEnabled = false;
			//depthStencilState.depthCompareOp = SV_GFX_COMPARE_OP_ALWAYS;
			//depthStencilState.stencilTestEnabled = true;

			//depthStencilState.front.failOp;
			//depthStencilState.front.passOp;
			//depthStencilState.front.depthFailOp;
			//depthStencilState.front.compareOp = SV_GFX_COMPARE_OP_NOT_EQUAL;
			//depthStencilState.front.readMask = 0xff;
			//depthStencilState.front.writeMask = 0xff;
			//depthStencilState.front.reference;

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

			desc.pVertexShader = nullptr;
			desc.pPixelShader = nullptr;
			desc.pGeometryShader = nullptr;
			svCheck(graphics_pipeline_create(&desc, g_SpriteTransparentPipeline));
		}
		// Sprite White Image
		{
			ui8 bytes[4];
			for (ui32 i = 0; i < 4; ++i) bytes[i] = 255u;

			SV_GFX_IMAGE_DESC desc;
			desc.pData = bytes;
			desc.size = sizeof(ui8) * 4u;
			desc.format = SV_GFX_FORMAT_R8G8B8A8_UNORM;
			desc.layout = SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE;
			desc.type = SV_GFX_IMAGE_TYPE_SHADER_RESOURCE;
			desc.usage = SV_GFX_USAGE_STATIC;
			desc.CPUAccess = SV_GFX_CPU_ACCESS_NONE;
			desc.dimension = 2u;
			desc.width = 1u;
			desc.height = 1u;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, g_SpriteWhiteTexture));
		}
		// Sprite Default Sampler
		{
			SV_GFX_SAMPLER_DESC desc;
			desc.addressModeU = SV_GFX_ADDRESS_MODE_WRAP;
			desc.addressModeV = SV_GFX_ADDRESS_MODE_WRAP;
			desc.addressModeW = SV_GFX_ADDRESS_MODE_WRAP;
			desc.minFilter = SV_GFX_FILTER_NEAREST;
			desc.magFilter = SV_GFX_FILTER_NEAREST;

			svCheck(graphics_sampler_create(&desc, g_SpriteDefSampler));
		}

		return true;
	}

	bool renderer_layer_close()
	{
		svCheck(graphics_destroy(g_SpriteOpaquePipeline));
		svCheck(graphics_destroy(g_SpriteOpaqueRenderPass));
		svCheck(graphics_destroy(g_SpriteVertexShader));
		svCheck(graphics_destroy(g_SpritePixelShader));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));
		svCheck(graphics_destroy(g_SpriteIndexBuffer));
		svCheck(graphics_destroy(g_SpriteWhiteTexture));
		svCheck(graphics_destroy(g_SpriteDefSampler));

		return true;
	}

	void CreateSpritesInstancesSystem(Scene& scene, Entity entity, BaseComponent** comp, float dt)
	{
		SpriteComponent& sprComp = *reinterpret_cast<SpriteComponent*>(comp[0]);
		Transform trans = scene.GetTransform(entity);

		if (sprComp.renderLayer == SV_RENDER_LAYER_DEFAULT) {
			g_DefRenderLayer->sprites.Emplace(trans.GetWorldMatrix(), sprComp.sprite, sprComp.color);
		}
		else {
			RenderLayer* rl = reinterpret_cast<RenderLayer*>(sprComp.renderLayer);
			rl->sprites.Emplace(trans.GetWorldMatrix(), sprComp.sprite, sprComp.color);
		}
	}

	void renderer_layer_begin()
	{
		// Reset Render Layers
		for (auto it = g_RenderLayers.begin(); it != g_RenderLayers.end(); ++it) {
			(*it)->Reset();
		}
	}

	void renderer_layer_end()
	{
		DrawData& drawData = renderer_drawdata_get();

		// Sort Sprites Instances

		// Sort RenderLayers
		std::sort(g_RenderLayers.begin(), g_RenderLayers.end(), [](const std::unique_ptr<RenderLayer>& rl0, const std::unique_ptr<RenderLayer>& rl1) {
			return rl0->sortValue < rl1->sortValue;
		});
	}

	void renderer_layer_prepare_scene(sv::Scene& scene)
	{
		// Create Sprites Instances

		SV_ECS_SYSTEM_DESC systems[1];

		CompID reqComp[] = {
			SpriteComponent::ID
		};

		systems[0].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		systems[0].system = CreateSpritesInstancesSystem;
		systems[0].pRequestedComponents = reqComp;
		systems[0].requestedComponentsCount = 1u;
		systems[0].optionalComponentsCount = 0u;

		scene.ExecuteSystems(systems, 1u, 0.f);

	}

	void RenderSpriteBatch(ui32 offset, ui32 size, TextureAtlas_DrawData* texture, CommandList cmd)
	{
		Buffer* vBuffers[] = {
			&g_SpriteVertexBuffer,
		};
		ui32 offsets[] = {
			0u,
		};
		ui32 strides[] = {
			size * 4u * sizeof(SpriteVertex),
		};
		Image* images[1];
		Sampler* samplers[1];

		if (texture) {
			images[0] = &texture->image;
			samplers[0] = &texture->sampler;
		}
		else {
			images[0] = &g_SpriteWhiteTexture;
			samplers[0] = &g_SpriteDefSampler;
		}

		graphics_vertexbuffer_bind(vBuffers, offsets, strides, 1u, cmd);

		graphics_image_bind(images, 1u, SV_GFX_SHADER_TYPE_PIXEL, cmd);
		graphics_sampler_bind(samplers, 1u, SV_GFX_SHADER_TYPE_PIXEL, cmd);

		graphics_draw_indexed(size * 6u, 1u, offset * 6u, 0u, 0u, cmd);

	}

	void renderer_layer_render(CommandList cmd)
	{
		DrawData& drawData = renderer_drawdata_get();

		// TODO: frustum culling

		Image* att[] = {
			&drawData.pOutput->renderTarget,
			&drawData.pOutput->depthStencil
		};

		TextureAtlas_DrawData* texture = nullptr;

		for (auto it = g_RenderLayers.begin(); it != g_RenderLayers.end(); ++it) {
			auto& sprites = (*it)->sprites;

			ui32 i = 0u;

			while (i < sprites.Size()) {

				ui32 j = 0u;
				ui32 batchSize = SV_REND_LAYER_BATCH_COUNT;
				if (i + batchSize > sprites.Size()) {
					batchSize = sprites.Size() - i;
				}

				// Fill Vertex Buffer
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

					vec4 texCoord;
					if(spr.sprite.pTexAtlas != nullptr) 
						texCoord = spr.sprite.pTexAtlas->sprites[spr.sprite.ID];

					ui32 index = j * 4u;
					g_SpriteData[index + 0] = { pos0, {texCoord.x, texCoord.y}, spr.color };
					g_SpriteData[index + 1] = { pos1, {texCoord.z, texCoord.y}, spr.color };
					g_SpriteData[index + 2] = { pos2, {texCoord.x, texCoord.w}, spr.color };
					g_SpriteData[index + 3] = { pos3, {texCoord.z, texCoord.w}, spr.color };

					j++;
				}

				graphics_buffer_update(g_SpriteVertexBuffer, g_SpriteData, batchSize * sizeof(SpriteVertex) * 4u, 0u, cmd);

				// Begin rendering

				graphics_renderpass_begin(g_SpriteOpaqueRenderPass, att, nullptr, 1.f, 0u, cmd);

				graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
				graphics_pipeline_bind(g_SpriteOpaquePipeline, cmd);

				// Draw call per textureAtlas
				j = 0u;
				ui32 offset = 0u;
				texture = sprites[j].sprite.pTexAtlas;

				while (j < batchSize) {

					if (sprites[j].sprite.pTexAtlas != texture) {
						RenderSpriteBatch(offset, j - offset, texture, cmd);
						offset += j;
						texture = sprites[j].sprite.pTexAtlas;
					}
					
					++j;
				}

				RenderSpriteBatch(offset, j - offset, texture, cmd);

				graphics_renderpass_end(cmd);

				i += j;
			}
		}
		
	}

}

namespace sv {

	using namespace _sv;

	RenderLayerID renderer_layer_create(i16 sortValue, SV_REND_SORT_MODE sortMode, bool transparent)
	{
		std::unique_ptr<RenderLayer> renderLayer = std::make_unique<RenderLayer>();
		renderLayer->sortMode = sortMode;
		renderLayer->sortValue = sortValue;
		renderLayer->transparent = transparent;
		RenderLayerID res = renderLayer.get();
		g_RenderLayers.push_back(std::move(renderLayer));
		return res;
	}

	i16 renderer_layer_get_sort_value(RenderLayerID renderLayer)
	{
		return reinterpret_cast<RenderLayer*>(renderLayer)->sortValue;
	}

	SV_REND_SORT_MODE renderer_layer_get_sort_mode(RenderLayerID renderLayer)
	{
		return reinterpret_cast<RenderLayer*>(renderLayer)->sortMode;
	}

	bool renderer_layer_get_transparent(RenderLayerID renderLayer)
	{
		return reinterpret_cast<RenderLayer*>(renderLayer)->transparent;
	}

	void renderer_layer_set_sort_value(i16 value, RenderLayerID renderLayer)
	{
		reinterpret_cast<RenderLayer*>(renderLayer)->sortValue = value;
	}

	void renderer_layer_set_sort_mode(SV_REND_SORT_MODE mode, RenderLayerID renderLayer)
	{
		reinterpret_cast<RenderLayer*>(renderLayer)->sortMode = mode;
	}

	void renderer_layer_set_transparent(bool transparent, RenderLayerID renderLayer)
	{
		reinterpret_cast<RenderLayer*>(renderLayer)->transparent = transparent;
	}

	void renderer_layer_destroy(RenderLayerID renderLayer)
	{
		for (auto it = g_RenderLayers.begin(); it != g_RenderLayers.end(); ++it) {
			if (it->get() == renderLayer) {
				g_RenderLayers.erase(it);
				return;
			}
		}
	}

}