#include "core.h"

#include "renderer2D_internal.h"
#include "..//renderer_internal.h"
#include "scene/scene_internal.h"
#include "components.h"

namespace sv {

	// Sprite Primitives

	static GraphicsPipeline g_SpritePipeline;
	static RenderPass g_SpriteRenderPass;

	static Shader g_SpriteVertexShader;
	static Shader g_SpritePixelShader;
	static GPUBuffer g_SpriteIndexBuffer;
	static GPUBuffer g_SpriteVertexBuffer;
	static GPUImage g_SpriteWhiteTexture;
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

	bool renderer2D_initialize()
	{
		// Sprite Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.CPUAccess = CPUAccess_Write;
			desc.usage = ResourceUsage_Default;
			desc.pData = nullptr;
			desc.size = SV_REND_LAYER_BATCH_COUNT * sizeof(SpriteVertex) * 4u;

			svCheck(graphics_buffer_create(&desc, g_SpriteVertexBuffer));

			// Index Buffer
			ui32 indexData[SV_REND_LAYER_BATCH_COUNT * 6u];
			ComputeIndexData(indexData);

			desc.bufferType = GPUBufferType_Index;
			desc.CPUAccess = CPUAccess_None;
			desc.usage = ResourceUsage_Static;
			desc.pData = indexData;
			desc.size = SV_REND_LAYER_BATCH_COUNT * 6u * sizeof(ui32);
			desc.indexType = IndexType_32;

			svCheck(graphics_buffer_create(&desc, g_SpriteIndexBuffer));

		}
		// Sprite Shaders
		{
			ShaderDesc desc;
			desc.filePath = "shaders/SpriteVertex.shader";
			desc.shaderType = ShaderType_Vertex;

			svCheck(graphics_shader_create(&desc, g_SpriteVertexShader));

			desc.filePath = "shaders/SpritePixel.shader";
			desc.shaderType = ShaderType_Pixel;

			svCheck(graphics_shader_create(&desc, g_SpritePixelShader));
		}
		// Sprite RenderPass
		{
			RenderPassDesc desc;
			desc.attachments.resize(1);

			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[0].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[0].format = SV_REND_OFFSCREEN_FORMAT;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, g_SpriteRenderPass));
		}
		// Sprite Pipelines
		{
			InputLayoutDesc inputLayout;
			inputLayout.slots.push_back({ 0u, sizeof(SpriteVertex), false });

			inputLayout.elements.push_back({ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT });
			inputLayout.elements.push_back({ "TexCoord", 0u, 0u, 4u * sizeof(float), Format_R32G32_FLOAT });
			inputLayout.elements.push_back({ "Color", 0u, 0u, 6u * sizeof(float), Format_R8G8B8A8_UNORM });

			BlendStateDesc blendState;
			blendState.attachments.resize(1);
			blendState.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			blendState.attachments[0].blendEnabled = true;
			blendState.attachments[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			blendState.attachments[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			blendState.attachments[0].colorBlendOp = BlendOperation_Add;
			blendState.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].alphaBlendOp = BlendOperation_Add;
			blendState.attachments[0].colorWriteMask = ColorComponent_All;

			GraphicsPipelineDesc desc;
			desc.pVertexShader = &g_SpriteVertexShader;
			desc.pPixelShader = &g_SpritePixelShader;
			desc.pGeometryShader = nullptr;
			desc.pInputLayout = &inputLayout;
			desc.pBlendState = &blendState;
			desc.pRasterizerState = nullptr;
			desc.pDepthStencilState = nullptr;
			desc.topology = GraphicsTopology_Triangles;

			svCheck(graphics_pipeline_create(&desc, g_SpritePipeline));
		}
		// Sprite White Image
		{
			ui8 bytes[4];
			for (ui32 i = 0; i < 4; ++i) bytes[i] = 255u;

			GPUImageDesc desc;
			desc.pData = bytes;
			desc.size = sizeof(ui8) * 4u;
			desc.format = Format_R8G8B8A8_UNORM;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.dimension = 2u;
			desc.width = 1u;
			desc.height = 1u;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, g_SpriteWhiteTexture));
		}
		// Sprite Default Sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Nearest;
			desc.magFilter = SamplerFilter_Nearest;

			svCheck(graphics_sampler_create(&desc, g_SpriteDefSampler));
		}

		return true;
	}

	bool renderer2D_close()
	{
		svCheck(graphics_destroy(g_SpritePipeline));
		svCheck(graphics_destroy(g_SpriteRenderPass));
		svCheck(graphics_destroy(g_SpriteVertexShader));
		svCheck(graphics_destroy(g_SpritePixelShader));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));
		svCheck(graphics_destroy(g_SpriteIndexBuffer));
		svCheck(graphics_destroy(g_SpriteWhiteTexture));
		svCheck(graphics_destroy(g_SpriteDefSampler));

		return true;
	}

	void renderer2D_sprite_sort(SpriteInstance* buffer, ui32 count, const RenderLayer& renderLayer) 
	{
		SpriteInstance* begin = buffer;
		SpriteInstance* end = buffer + count;

		switch (renderLayer.sortMode)
		{
		case RenderLayerSortMode_none:
			std::sort(begin, end, [](const SpriteInstance& s0, const SpriteInstance& s1) {
				return s0.sprite.textureAtlas < s1.sprite.textureAtlas;
			});
			break;

		case RenderLayerSortMode_coordX:
			std::sort(begin, end, [](const SpriteInstance& s0, const SpriteInstance& s1) {
				if (s0.position.x == s1.position.x) return s0.sprite.textureAtlas < s1.sprite.textureAtlas;
				return s0.position.x < s1.position.x;
			});
			break;

		case RenderLayerSortMode_coordY:
			std::sort(begin, end, [](const SpriteInstance& s0, const SpriteInstance& s1) {
				if (s0.position.y == s1.position.y) return s0.sprite.textureAtlas < s1.sprite.textureAtlas;
				return s0.position.y < s1.position.y;
			});
			break;

		case RenderLayerSortMode_coordZ:
			std::sort(begin, end, [](const SpriteInstance& s0, const SpriteInstance& s1) {
				if (s0.position.z == s1.position.z) return s0.sprite.textureAtlas < s1.sprite.textureAtlas;
				return s0.position.z < s1.position.z;
			});
			break;

		}
	}

	void renderer2D_scene_draw_sprites(CommandList cmd)
	{
		EntityView<SpriteComponent> sprites;
		std::vector<SpriteInstance> instances(sprites.size());

		// Create Instances
		{
			for (ui32 i = 0; i < sprites.size(); ++i) {
				SpriteComponent& sprite = sprites[i];
				// TODO: frustum culling
				Transform trans = scene_ecs_entity_get_transform(sprite.entity);
				instances.emplace_back(trans.GetWorldMatrix(), sprite.sprite, sprite.color, sprite.renderLayer, trans.GetWorldPosition());
			}
		}

		// Sort by layer
		std::sort(instances.begin(), instances.end(), [](const SpriteInstance& i0, const SpriteInstance& i1) {
			return i0.layerID < i1.layerID;
		});

		// Sort layer by sort mode and texture
		{
			auto& renderLayers = scene_renderWorld_get().renderLayers;

			ui32 rl = 0u;
			SpriteInstance* offset = instances.data();

			if (!instances.empty()) {
				for (auto it = instances.begin(); it != instances.end(); ++it) {
					if (rl != it->layerID) {
						
						SpriteInstance* pos = &*it;
						
						renderer2D_sprite_sort(offset, pos - offset, renderLayers[rl]);

						offset = pos;
						rl = it->layerID;
					}
				}
				renderer2D_sprite_sort(offset, instances.size() - (offset - instances.data()), renderLayers[rl]);
			}
		}

		// Render
		DrawData& drawData = renderer_drawData_get();
		renderer2D_sprite_render(instances.data(), ui32(instances.size()), drawData.viewProjectionMatrix, drawData.pOffscreen->renderTarget, cmd);
	}

	void RenderSpriteBatch(ui32 offset, ui32 size, TextureAtlas_internal* texture, CommandList cmd)
	{
		GPUBuffer* vBuffers[] = {
			&g_SpriteVertexBuffer,
		};
		ui32 offsets[] = {
			0u,
		};
		ui32 strides[] = {
			size * 4u * sizeof(SpriteVertex),
		};
		GPUImage* images[1];
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

		graphics_image_bind(images, 1u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(samplers, 1u, ShaderType_Pixel, cmd);

		graphics_draw_indexed(size * 6u, 1u, offset * 6u, 0u, 0u, cmd);

	}

	void renderer2D_sprite_render(SpriteInstance* buffer, ui32 count, const XMMATRIX& viewProjectionMatrix, GPUImage& renderTarget, CommandList cmd)
	{
		GPUImage* att[] = {
			&renderTarget
		};

		TextureAtlas_internal* texture = nullptr;
		auto& renderLayers = scene_renderWorld_get().renderLayers;

		SpriteInstance* initialPtr = buffer;

		while (buffer < initialPtr + count) {

			ui32 j = 0u;
			ui32 currentInstance = buffer - initialPtr;
			ui32 batchSize = SV_REND_LAYER_BATCH_COUNT;
			if (currentInstance + batchSize > count) {
				batchSize = count - currentInstance;
			}

			// Fill Vertex Buffer
			while (j < batchSize) {

				SpriteInstance& spr = buffer[j];

				// Compute Matrices form WorldSpace to ScreenSpace
				XMMATRIX matrix = spr.tm * viewProjectionMatrix;

				XMVECTOR pos0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR pos1 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR pos2 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR pos3 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);

				pos0 = XMVector4Transform(pos0, matrix);
				pos1 = XMVector4Transform(pos1, matrix);
				pos2 = XMVector4Transform(pos2, matrix);
				pos3 = XMVector4Transform(pos3, matrix);

				vec4 texCoord;
				if (spr.sprite.textureAtlas)
					texCoord = reinterpret_cast<TextureAtlas_internal*>(spr.sprite.textureAtlas)->sprites[spr.sprite.index];

				ui32 index = j * 4u;
				g_SpriteData[index + 0] = { pos0, {texCoord.x, texCoord.y}, spr.color };
				g_SpriteData[index + 1] = { pos1, {texCoord.z, texCoord.y}, spr.color };
				g_SpriteData[index + 2] = { pos2, {texCoord.x, texCoord.w}, spr.color };
				g_SpriteData[index + 3] = { pos3, {texCoord.z, texCoord.w}, spr.color };

				j++;
			}

			graphics_buffer_update(g_SpriteVertexBuffer, g_SpriteData, batchSize * sizeof(SpriteVertex) * 4u, 0u, cmd);

			// Begin rendering
			graphics_renderpass_begin(g_SpriteRenderPass, att, nullptr, 1.f, 0u, cmd);

			graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
			graphics_pipeline_bind(g_SpritePipeline, cmd);

			SpriteInstance* beginBuffer = buffer;
			SpriteInstance* endBuffer;

			while (buffer < beginBuffer + batchSize) {

				endBuffer = buffer + batchSize;

				texture = reinterpret_cast<TextureAtlas_internal*>(buffer->sprite.textureAtlas);
				
				ui32 offset = buffer - beginBuffer;
				while (buffer != endBuffer) {

					if (buffer->sprite.textureAtlas != texture) {
						ui32 batchPos = buffer - beginBuffer;
						RenderSpriteBatch(offset, batchPos - offset, texture, cmd);
						offset = batchPos;
						texture = reinterpret_cast<TextureAtlas_internal*>(buffer->sprite.textureAtlas);
					}

					buffer++;
				}

				ui32 batchPos = buffer - beginBuffer;
				RenderSpriteBatch(offset, batchPos - offset, texture, cmd);

			}

			graphics_renderpass_end(cmd);
		}

	}

	// Render layers

	void renderLayer_count_set(ui32 count)
	{
		scene_renderWorld_get().renderLayers.resize(count);
	}

	ui32 renderLayer_count_get()
	{
		return ui32(scene_renderWorld_get().renderLayers.size());
	}

	void renderLayer_sortMode_set(ui32 layer, RenderLayerSortMode sortMode)
	{
		RenderLayer& rl = scene_renderWorld_get().renderLayers[layer];
		rl.sortMode = sortMode;
	}

	RenderLayerSortMode	renderLayer_sortMode_get(ui32 layer)
	{
		return scene_renderWorld_get().renderLayers[layer].sortMode;
	}

	// Texture Atlas

	bool textureAtlas_create_from_file(const char* filePath, bool linearFilter, SamplerAddressMode addressMode, TextureAtlas* pTextureAtlas)
	{
		// Get file data
		void* data;
		ui32 width;
		ui32 height;
		svCheck(utils_loader_image(filePath, &data, &width, &height));

		TextureAtlas_internal* res = new TextureAtlas_internal();

		// Create Image
		{
			GPUImageDesc desc;

			desc.pData = data;
			desc.size = width * height * 4u;
			desc.format = Format_R8G8B8A8_UNORM;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.dimension = 2u;
			desc.width = width;
			desc.height = height;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, res->image));
		}
		// Create Sampler
		{
			SamplerDesc desc;

			desc.addressModeU = addressMode;
			desc.addressModeV = addressMode;
			desc.addressModeW = addressMode;
			desc.minFilter = linearFilter ? SamplerFilter_Linear : SamplerFilter_Nearest;
			desc.magFilter = desc.minFilter;

			svCheck(graphics_sampler_create(&desc, res->sampler));
		}

		*pTextureAtlas = res;
		return true;
	}

	bool textureAtlas_destroy(TextureAtlas textureAtlas)
	{
		TextureAtlas_internal& t = *reinterpret_cast<TextureAtlas_internal*>(textureAtlas);
		if (t.image.IsValid()) {
			svCheck(graphics_destroy(t.image));
			svCheck(graphics_destroy(t.sampler));
			t.sprites.clear();
		}

		delete &t;
		return true;
	}

	Sprite textureAtlas_sprite_add(TextureAtlas textureAtlas, float x, float y, float w, float h)
	{
		TextureAtlas_internal& t = *reinterpret_cast<TextureAtlas_internal*>(textureAtlas);
		SV_ASSERT(t.image.IsValid());
		Sprite res = { textureAtlas, ui32(t.sprites.size()) };
		t.sprites.emplace_back(x, y, x + w, y + h);
		return res;
	}

	ui32 textureAtlas_sprite_count(TextureAtlas textureAtlas)
	{
		if (textureAtlas == nullptr) return 0u;
		TextureAtlas_internal& t = *reinterpret_cast<TextureAtlas_internal*>(textureAtlas);
		return ui32(t.sprites.size());
	}

	vec4 textureAtlas_sprite_get(TextureAtlas textureAtlas, ui32 index)
	{
		TextureAtlas_internal& t = *reinterpret_cast<TextureAtlas_internal*>(textureAtlas);
		return t.sprites[index];
	}

}