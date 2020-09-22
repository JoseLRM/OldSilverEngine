#include "core.h"

#include "renderer/renderer_internal.h"

namespace sv {

#define parseBatch() sv::RendererDebugBatch_internal& batch = *reinterpret_cast<sv::RendererDebugBatch_internal*>(batch_)

	struct DebugData {
		vec4f position;
		vec2f texCoord;
		float stroke;
		Color color;
	};

	static GPUBuffer g_VertexBuffer[SV_GFX_COMMAND_LIST_COUNT];

	static RenderPass		g_RenderPass;
	static InputLayoutState g_InputLayout;
	static BlendState		g_BlendState;
	static Sampler			g_DefSampler;

	// Shaders

	static Shader			g_QuadVertexShader;
	static Shader			g_QuadPixelShader;

	static Shader			g_EllipseVertexShader;
	static Shader			g_EllipsePixelShader;
	
	static Shader			g_SpriteVertexShader;
	static Shader			g_SpritePixelShader;

	// MAIN FUNCTIONS

	Result renderer_debug_initialize()
	{
		// Create Shaders
		{
			svCheck(renderer_shader_create("Debug_quad", ShaderType_Vertex, g_QuadVertexShader));
			svCheck(renderer_shader_create("Debug_quad", ShaderType_Pixel, g_QuadPixelShader));

			svCheck(renderer_shader_create("Debug_ellipse", ShaderType_Vertex, g_EllipseVertexShader));
			svCheck(renderer_shader_create("Debug_ellipse", ShaderType_Pixel, g_EllipsePixelShader));
			
			svCheck(renderer_shader_create("Debug_sprite", ShaderType_Vertex, g_SpriteVertexShader));
			svCheck(renderer_shader_create("Debug_sprite", ShaderType_Pixel, g_SpritePixelShader));
		}

		// Create RenderPass
		{
			RenderPassDesc desc;
			AttachmentDesc& att = desc.attachments.emplace_back();

			att.loadOp = AttachmentOperation_Load;
			att.storeOp = AttachmentOperation_Store;
			att.stencilLoadOp = AttachmentOperation_DontCare;
			att.stencilStoreOp = AttachmentOperation_DontCare;
			att.format = SV_REND_OFFSCREEN_FORMAT;
			att.initialLayout = GPUImageLayout_RenderTarget;
			att.layout = GPUImageLayout_RenderTarget;
			att.finalLayout = GPUImageLayout_RenderTarget;
			att.type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, g_RenderPass));
		}

		// Create Input layout
		{
			InputLayoutStateDesc desc;
			InputSlotDesc& slot = desc.slots.emplace_back();
			slot.instanced = false;
			slot.slot = 0u;
			slot.stride = sizeof(DebugData);

			desc.elements = { 
				{ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT },
				{ "TexCoord", 0u, 0u, 4u * sizeof(float), Format_R32G32_FLOAT },
				{ "Stroke", 0u, 0u, 6u * sizeof(float), Format_R32_FLOAT },
				{ "Color", 0u, 0u, 7u * sizeof(float), Format_R8G8B8A8_UNORM },
			};

			svCheck(graphics_inputlayoutstate_create(&desc, g_InputLayout));
		}
		
		// Create Blend State
		{
			BlendStateDesc desc;
			desc.attachments.resize(1);
			desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			desc.attachments[0].blendEnabled = true;
			desc.attachments[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			desc.attachments[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			desc.attachments[0].colorBlendOp = BlendOperation_Add;
			desc.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].alphaBlendOp = BlendOperation_Add;
			desc.attachments[0].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&desc, g_BlendState));
		}

		// Create Def sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Nearest;
			desc.magFilter = SamplerFilter_Nearest;

			svCheck(graphics_sampler_create(&desc, g_DefSampler));
		}

		return Result_Success;
	}

	Result renderer_debug_close()
	{
		return Result_Success;
	}

	Result renderer_debug_create_buffer(CommandList cmd)
	{
		if (!g_VertexBuffer[cmd].IsValid()) {

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = SV_REND_BATCH_COUNT * sizeof(DebugData);
			desc.pData = nullptr;

			return graphics_buffer_create(&desc, g_VertexBuffer[cmd]);

		}

		return Result_Success;
	}

	// BATCH MANGEMENT

	Result renderer_debug_batch_create(RendererDebugBatch** pBatch)
	{
		*pBatch = new RendererDebugBatch_internal();
		renderer_debug_batch_reset(*pBatch);
		return Result_Success;
	}

	Result renderer_debug_batch_destroy(RendererDebugBatch* batch_)
	{
		if (batch_ == nullptr) return Result_Success;
		parseBatch();

		delete &batch;
		return Result_Success;
	}

	// BEGIN - END

	void renderer_debug_batch_reset(RendererDebugBatch* batch_)
	{
		parseBatch();
		batch.quads.clear();
		batch.lines.clear();
		batch.ellipses.clear();
		batch.sprites.clear();
		batch.drawCalls.resize(1u);
		batch.drawCalls.back().list = ui32_max;
	}

	constexpr ui32 renderer_debug_vertex_count(ui32 list)
	{
		switch (list)
		{
		case 0:
			return 6u;
		case 1:
			return 2u;
		case 2:
			return 6u;
		case 3:
			return 6u;
		default:
			svLogError("Unknown list: %u", list);
			return 0u;
		}
	}

	void renderer_debug_draw_call(ui32 batchOffset, RendererDebugDraw& draw, ui32 vertexCount, GPUBuffer& buffer, CommandList cmd)
	{
		switch (draw.list)
		{
		case 0u:
			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_shader_bind(g_QuadVertexShader, cmd);
			graphics_shader_bind(g_QuadPixelShader, cmd);
			break;

		case 1u:
			graphics_topology_set(GraphicsTopology_Lines, cmd);
			graphics_shader_bind(g_QuadVertexShader, cmd);
			graphics_shader_bind(g_QuadPixelShader, cmd);
			graphics_line_width_set(draw.lineWidth, cmd);
			break;

		case 2u:
			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_shader_bind(g_EllipseVertexShader, cmd);
			graphics_shader_bind(g_EllipsePixelShader, cmd);
			break;

		case 3u:
			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_shader_bind(g_SpriteVertexShader, cmd);
			graphics_shader_bind(g_SpritePixelShader, cmd);

			graphics_image_bind(*draw.pImage, 0u, ShaderType_Pixel, cmd);

			if (draw.pSampler) {
				graphics_sampler_bind(*draw.pSampler, 0u, ShaderType_Pixel, cmd);
			}
			else {
				graphics_sampler_bind(g_DefSampler, 0u, ShaderType_Pixel, cmd);
			}

			break;

		}

		graphics_draw(vertexCount, 1u, batchOffset, 0u, cmd);
	}

	void renderer_debug_draw_batch(RendererDebugDraw* begin, ui32 beginIndex, RendererDebugDraw* end, ui32 endIndex, ui32 batchCount, DebugData* batchData, GPUImage& renderTarget, CommandList cmd)
	{
		GPUBuffer& buffer = g_VertexBuffer[cmd];

		graphics_buffer_update(buffer, batchData, batchCount * sizeof(DebugData), 0u, cmd);

		GPUImage* attachments[] = {
			&renderTarget
		};

		graphics_renderpass_begin(g_RenderPass, attachments, nullptr, 1.f, 0u, cmd);

		graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);

		ui32 batchOffset;
		{
			ui32 count = begin->count - (beginIndex - begin->index);
			ui32 vertexCount = renderer_debug_vertex_count(begin->list);
			renderer_debug_draw_call(0u, *begin, vertexCount * count, buffer, cmd);

			batchOffset = vertexCount * count;
		}

		if (begin != end) {

			RendererDebugDraw* it = begin + 1u;
			while (it != end) {
				ui32 vertexCount = renderer_debug_vertex_count(it->list);
				renderer_debug_draw_call(batchOffset, *it, vertexCount * it->count, buffer, cmd);
				batchOffset += vertexCount * it->count;
				++it;
			}

			ui32 vertexCount = renderer_debug_vertex_count(end->list);
			renderer_debug_draw_call(batchOffset, *end, vertexCount * end->count - (endIndex - end->index), buffer, cmd);

		}

		graphics_renderpass_end(cmd);

	}

	void renderer_debug_batch_render(RendererDebugBatch* batch_, GPUImage& renderTarget, const Viewport& viewport, const Scissor& scissor, const XMMATRIX& viewProjectionMatrix, CommandList cmd)
	{
		parseBatch();
		if (batch.drawCalls.size() <= 1u)
			return;

		SV_ASSERT(renderer_debug_create_buffer(cmd) == Result_Success);

		graphics_mode_set(GraphicsPipelineMode_Graphics, cmd);
		graphics_state_unbind(cmd);

		graphics_viewport_set(&viewport, 1u, cmd);
		graphics_scissor_set(&scissor, 1u, cmd);

		graphics_inputlayoutstate_bind(g_InputLayout, cmd);
		graphics_blendstate_bind(g_BlendState, cmd);

		RendererDebugDraw* end = batch.drawCalls.data() + batch.drawCalls.size();
		RendererDebugDraw* it = batch.drawCalls.data() + 1u;

		// Draw Data
		RendererDebugDraw* beginIt = it;
		ui32 beginIndex = it->index;
		ui32 currentIndex = ui32_max;

		// Update Data
		DebugData batchData[SV_REND_BATCH_COUNT];
		DebugData* itBatch = batchData;

		while (it != end) {
			const RendererDebugDraw& draw = *it;

			// Current index
			ui32 currentIndex;

			if (beginIndex == ui32_max) {

				beginIt = it;

				if (currentIndex == ui32_max) beginIndex = draw.index;
				else beginIndex = currentIndex;

				currentIndex = beginIndex;
			}
			else currentIndex = draw.index;

			ui32 vertexCount = renderer_debug_vertex_count(draw.list);

			// Fill vertex buffer
			DebugData* endBatch = itBatch + SV_REND_BATCH_COUNT;
			ui32 batchCount = ui32(endBatch - itBatch) / vertexCount;
			ui32 c = std::min(batchCount, draw.count - (currentIndex - draw.index)) * vertexCount;
			endBatch = itBatch + ui64(c);

			switch (draw.list)
			{
			case 0:
			case 2:
			case 3:
			{
				while (itBatch != endBatch) {

					XMVECTOR p0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
					XMVECTOR p1 = XMVectorSet( 0.5f, -0.5f, 0.f, 1.f);
					XMVECTOR p2 = XMVectorSet(-0.5f,  0.5f, 0.f, 1.f);
					XMVECTOR p3 = XMVectorSet( 0.5f,  0.5f, 0.f, 1.f);

					XMMATRIX mvpMatrix;
					Color color;
					vec4f texCoord;
					float stroke = 1.f;

					switch (draw.list)
					{
					case 0:
					{
						RendererDebugQuad& quad = batch.quads[currentIndex];

						mvpMatrix = quad.matrix;
						color = quad.color;
						stroke = batch.stroke;
						texCoord.x = 69.f;
					}

						break;

					case 2:
					{
						RendererDebugQuad& ellipse = batch.ellipses[currentIndex];

						mvpMatrix = ellipse.matrix;
						color = ellipse.color;
						stroke = batch.stroke;
						texCoord.x = 69.f;
					}
						break;

					case 3:
					{
						RendererDebugQuad& spr = batch.sprites[currentIndex];

						mvpMatrix = spr.matrix;
						color = spr.color;
						texCoord = batch.texCoord;
					}
						break;
					}

					// Compute size
					if (texCoord.x == 69.f) {
						XMFLOAT4X4 m;
						XMStoreFloat4x4(&m, mvpMatrix);

						float width = XMVectorGetX(XMVector3Length(XMVectorSet(m._11, m._12, m._13, 0.f)));
						float height = XMVectorGetX(XMVector3Length(XMVectorSet(m._21, m._22, m._23, 0.f)));

						texCoord.z = width / 2.f;
						texCoord.w = height / 2.f;
						texCoord.x = -texCoord.z;
						texCoord.y = -texCoord.w;
					}

					mvpMatrix *= viewProjectionMatrix;

					p0 = XMVector3Transform(p0, mvpMatrix);
					p1 = XMVector3Transform(p1, mvpMatrix);
					p2 = XMVector3Transform(p2, mvpMatrix);
					p3 = XMVector3Transform(p3, mvpMatrix);

					itBatch->position = vec4f(p0);
					itBatch->texCoord = { texCoord.x, texCoord.y };
					itBatch->stroke = stroke;
					itBatch->color = color;
					++itBatch;

					itBatch->position = vec4f(p1);
					itBatch->texCoord = { texCoord.z, texCoord.y };
					itBatch->stroke = stroke;
					itBatch->color = color;
					++itBatch;

					itBatch->position = vec4f(p2);
					itBatch->texCoord = { texCoord.x, texCoord.w };
					itBatch->stroke = stroke;
					itBatch->color = color;
					++itBatch;

					itBatch->position = vec4f(p1);
					itBatch->texCoord = { texCoord.z, texCoord.y };
					itBatch->stroke = stroke;
					itBatch->color = color;
					++itBatch;

					itBatch->position = vec4f(p3);
					itBatch->texCoord = { texCoord.z, texCoord.w };
					itBatch->stroke = stroke;
					itBatch->color = color;
					++itBatch;

					itBatch->position = vec4f(p2);
					itBatch->texCoord = { texCoord.x, texCoord.w };
					itBatch->stroke = stroke;
					itBatch->color = color;
					++itBatch;
					
					++currentIndex;
				}
			}
				break;

			case 1:
			{
				while (itBatch != endBatch) {

					RendererDebugLine& line = batch.lines[currentIndex];

					XMVECTOR p0 = XMVector4Transform(XMVectorSet(line.point0.x, line.point0.y, line.point0.z, 1.f), viewProjectionMatrix);
					XMVECTOR p1 = XMVector4Transform(XMVectorSet(line.point1.x, line.point1.y, line.point1.z, 1.f), viewProjectionMatrix);

					itBatch->position = vec4f(p0);
					itBatch->color = line.color;
					itBatch->stroke = 1.f;
					++itBatch;

					itBatch->position = vec4f(p1);
					itBatch->color = line.color;
					itBatch->stroke = 1.f;
					++itBatch;

					++currentIndex;
				}
			}
				break;

			}

			// Update & draw
			if (batchCount <= draw.count) {

				renderer_debug_draw_batch(beginIt, beginIndex, it, currentIndex, itBatch - batchData, batchData, renderTarget, cmd);

				// prepare next batch
				beginIndex = ui32_max;
				if (currentIndex == draw.count) {
					currentIndex = ui32_max;
					++it;
				}
			}
			else {
				++it;
			}
		}

		ui32 batchCount = itBatch - batchData;
		renderer_debug_draw_batch(beginIt, beginIndex, it - 1u, currentIndex, batchCount, batchData, renderTarget, cmd);

	}

	// DRAW

	void renderer_debug_draw_quad(RendererDebugBatch* batch_, const XMMATRIX& matrix, Color color)
	{
		parseBatch();

		if (batch.drawCalls.back().list == 0u && batch.drawCalls.back().stroke == batch.stroke) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(0u, ui32(batch.quads.size()), batch.stroke);
		}

		batch.quads.emplace_back(matrix, color);
	}

	void renderer_debug_draw_line(RendererDebugBatch* batch_, const vec3f& p0, const vec3f& p1, Color color)
	{
		parseBatch();
		
		if (batch.drawCalls.back().list == 1u && batch.drawCalls.back().lineWidth == batch.lineWidth) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(1u, ui32(batch.lines.size()), batch.lineWidth);
		}

		batch.lines.emplace_back(p0, p1, color);
	}

	void renderer_debug_draw_ellipse(RendererDebugBatch* batch_, const XMMATRIX& matrix, Color color)
	{
		parseBatch();
		
		if (batch.drawCalls.back().list == 2u && batch.drawCalls.back().stroke == batch.stroke) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(2u, ui32(batch.ellipses.size()), batch.stroke);
		}

		batch.ellipses.emplace_back(matrix, color);
	}

	void renderer_debug_draw_sprite(RendererDebugBatch* batch_, const XMMATRIX& matrix, Color color, GPUImage& image)
	{
		parseBatch();
		
		if (batch.sameSprite && batch.drawCalls.back().list == 3u && batch.drawCalls.back().pImage == &image) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(3u, ui32(batch.sprites.size()), image, batch.pSampler, batch.texCoord);
			batch.sameSprite = true;
		}

		batch.sprites.emplace_back(matrix, color);
	}


	void renderer_debug_draw_quad(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		renderer_debug_draw_quad(batch, tm, color);
	}

	void renderer_debug_draw_quad(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec3f& rotation, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		renderer_debug_draw_quad(batch, tm, color);
	}

	void renderer_debug_draw_ellipse(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		renderer_debug_draw_ellipse(batch, tm, color);
	}

	void renderer_debug_draw_ellipse(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec3f& rotation, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		renderer_debug_draw_ellipse(batch, tm, color);
	}

	void renderer_debug_draw_sprite(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, Color color, GPUImage& image)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		renderer_debug_draw_sprite(batch, tm, color, image);
	}

	void renderer_debug_draw_sprite(RendererDebugBatch* batch, const vec3f& position, const vec2f& size, const vec3f& rotation, Color color, GPUImage& image)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		renderer_debug_draw_sprite(batch, tm, color, image);
	}

	void renderer_debug_linewidth_set(RendererDebugBatch* batch_, float lineWidth)
	{
		parseBatch();
		batch.lineWidth = lineWidth;
	}

	float renderer_debug_linewidth_get(RendererDebugBatch* batch_)
	{
		parseBatch();
		return batch.lineWidth;
	}

	void renderer_debug_stroke_set(RendererDebugBatch* batch_, float stroke)
	{
		parseBatch();
		batch.stroke = stroke;
	}

	float renderer_debug_stroke_get(RendererDebugBatch* batch_)
	{
		parseBatch();
		return batch.stroke;
	}

	void renderer_debug_texcoord_set(RendererDebugBatch* batch_, const vec4f& texCoord)
	{
		parseBatch();
		batch.texCoord = texCoord;
		batch.sameSprite = false;
	}

	vec4f renderer_debug_texcoord_get(RendererDebugBatch* batch_)
	{
		parseBatch();
		return batch.texCoord;
	}

	void renderer_debug_sampler_set_default(RendererDebugBatch* batch_)
	{
		parseBatch();
		batch.pSampler = nullptr;
	}

	void renderer_debug_sampler_set(RendererDebugBatch* batch_, Sampler& sampler)
	{
		parseBatch();
		batch.pSampler = &sampler;
	}

}