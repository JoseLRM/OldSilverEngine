#include "SilverEngine/core.h"

#include "SilverEngine/render_utils/debug_renderer.h"
#include "SilverEngine/utils/allocators/FrameList.h"
#include "render_utils/render_utils_internal.h"

namespace sv {

	struct DebugRendererQuad {
		XMMATRIX matrix;
		Color color;

		DebugRendererQuad(const XMMATRIX& matrix, Color color) : matrix(matrix), color(color) {}
	};

	struct DebugRendererLine {
		v3_f32 point0;
		v3_f32 point1;
		Color color;

		DebugRendererLine(const v3_f32& p0, const v3_f32& p1, Color color) : point0(p0), point1(p1), color(color) {}
	};

	struct DebugRendererDraw {
		u32 list;
		u32 index;
		u32 count;
		union {
			float lineWidth;
			float stroke;
			struct {
				GPUImage* pImage;
				Sampler* pSampler;
				v4_f32 texCoord;
			};
		};

		DebugRendererDraw() {}
		DebugRendererDraw(u32 list, u32 index, float lineWidth = 1.f) : list(list), index(index), count(1u), lineWidth(lineWidth) {}
		DebugRendererDraw(u32 list, u32 index, GPUImage* image, Sampler* sampler, const v4_f32& texCoord)
			: list(list), index(index), count(1u), pImage(image), pSampler(sampler), texCoord(texCoord) {}
	};

	struct DebugRendererBatch_internal {

		FrameList<DebugRendererQuad>	quads;
		FrameList<DebugRendererLine>	lines;
		FrameList<DebugRendererQuad>	ellipses;
		FrameList<DebugRendererQuad>	sprites;

		FrameList<DebugRendererDraw> drawCalls;
		float lineWidth = 1.f;
		float stroke = 1.f;

		v4_f32 texCoord = { 0.f, 0.f, 1.f, 1.f };
		Sampler* pSampler = nullptr;
		bool sameSprite = false;

	};

	static constexpr u32 BATCH_COUNT = 10000u;

	static const char* QUAD_VERTEX_SHADER_SRC = 
	"#include \"core.hlsl\"\n"
	"struct Input {\n"
	"float4 position : Position; \n"
	"float2 texCoord : TexCoord;\n"
	"float stroke : Stroke;\n"
	"float4 color : Color;\n"
	"}; \n"
	"struct Output {\n"
	"	float4 color : PxColor;\n"
	"	float2 texCoord : PxTexCoord;\n"
	"	float2 stroke : PxStroke;\n"
	"	float4 position : SV_Position;\n"
	"};\n"
	"\n"
	"Output main(Input input)\n"
	"{\n"
	"	Output output;\n"
	"	output.position = input.position;\n"
	"	output.color = input.color;\n"
	"	float2 halfSize = abs(input.texCoord);\n"
	"	output.texCoord = input.texCoord;\n"
	"	output.stroke = halfSize - (input.stroke * min(halfSize.x, halfSize.y));\n"
	"	return output;\n"
	"}";

	static const char* QUAD_PIXEL_SHADER_SRC = 
		"#include \"core.hlsl\"\n"
		"struct Input {\n"
		"float4 color : PxColor; \n"
		"float2 texCoord : PxTexCoord; \n"
		"float2 stroke : PxStroke; \n"
		"};\n"

		"struct Output {\n"
		"	float4 color : SV_Target;\n"
		"};\n"
		"Output main(Input input)\n"
		"{\n"
		"	Output output;\n"
		"	if (abs(input.texCoord.x) < input.stroke.x && abs(input.texCoord.y) < input.stroke.y) discard;\n"
		"	output.color = input.color;\n"
		"	return output;\n"
		"};";

	static const char* ELLIPSE_VERTEX_SHADER_SRC = 
		"#include \"core.hlsl\"\n"
		"struct Input {\n"
		"	float4 position : Position;\n"

		"	float2 texCoord : TexCoord;\n"
		"	float stroke : Stroke;\n"
		"	float4 color : Color;\n"
		"};\n"
		"struct Output {\n"
		"	float2 texCoord : PxTexCoord;\n"
		"	float4 color : PxColor;\n"
		"	float stroke : PxStroke;\n"
		"	float4 position : SV_Position;\n"
		"};\n"
		"Output main(Input input)\n"
		"{\n"
		"	Output output;\n"
		"	output.position = input.position;\n"
		"	output.color = input.color;\n"
		"	output.texCoord = input.texCoord;\n"
		"	float2 halfSize = abs(input.texCoord);\n"
		"	float s = min(halfSize.x, halfSize.y);\n"
		"	output.stroke = s - (input.stroke * s);\n"
		"	return output;\n"
		"}";

	static const char* ELLIPSE_PIXEL_SHADER_SRC = 
		"#include \"core.hlsl\"\n"
		"struct Input {\n"
		"	float2 texCoord : PxTexCoord; \n"
		"	float4 color : PxColor; \n"
		"	float stroke : PxStroke; \n"
		"};\n"
		"struct Output {\n"
		"	float4 color : SV_Target;\n"
		"};\n"
		"Output main(Input input)\n"
		"{\n"
		"	float distance = length(input.texCoord);\n"
		"	if (distance > 0.5f || distance < input.stroke) discard;\n"
		"	Output output;\n"
		"	output.color = input.color;\n"
		"	return output;\n"
		"}";

static const char* SPRITE_VERTEX_SHADER_SRC = 
		"#include \"core.hlsl\"\n"
		"struct Input {\n"
		"	float4 position : Position;\n"
		"	float2 texCoord : TexCoord;\n"
		"	float stroke : Stroke;\n"
		"	float4 color : Color;\n"
		"};\n"
		"struct Output {\n"
		"	float4 color : PxColor;\n"
		"	float2 texCoord : PxTexCoord;\n"
		"	float4 position : SV_Position;\n"
		"};\n"
		"Output main(Input input)\n"
		"{\n"
		"	Output output;\n"
		"	output.position = input.position;\n"
		"	output.color = input.color;\n"
		"	output.texCoord = input.texCoord;\n"
		"	return output;\n"
		"}";

static const char* SPRITE_PIXEL_SHADER_SRC = 
		"#include \"core.hlsl\"\n"
		"struct Input {\n"
		"	float4 color : PxColor;\n"
		"	float2 texCoord : PxTexCoord;\n"
		"};\n"
		"struct Output {\n"
		"	float4 color : SV_Target;\n"
		"};\n"
		"SV_TEXTURE(tex, t0);\n"
		"SV_SAMPLER(sam, s0);\n"
		"Output main(Input input)\n"
		"{\n"
		"	Output output;\n"
		"	output.color = input.color * tex.Sample(sam, input.texCoord);\n"
		"	return output;\n"
		"}";

#define parseBatch() sv::DebugRendererBatch_internal& batch = *reinterpret_cast<sv::DebugRendererBatch_internal*>(pInternal)

	struct DebugData {
		v4_f32 position;
		v2_f32 texCoord;
		float stroke;
		Color color;
	};

	static GPUBuffer* g_VertexBuffer[GraphicsLimit_CommandList] = {};

	static RenderPass*			g_RenderPass;
	static InputLayoutState*	g_InputLayout;
	static BlendState*			g_BS_Geometry;
	static Sampler*				g_DefSampler;

	// Shaders

	static Shader*			g_QuadVertexShader;
	static Shader*			g_QuadPixelShader;

	static Shader*			g_EllipseVertexShader;
	static Shader*			g_EllipsePixelShader;
	
	static Shader*			g_SpriteVertexShader;
	static Shader*			g_SpritePixelShader;

	// MAIN FUNCTIONS

	Result debug_renderer_initialize()
	{
		// Create Shaders
		svCheck(graphics_shader_compile_fastbin_from_string("DefaultRenderer_QuadVertex", ShaderType_Vertex, &g_QuadVertexShader, QUAD_VERTEX_SHADER_SRC));
		svCheck(graphics_shader_compile_fastbin_from_string("DefaultRenderer_QuadPixel", ShaderType_Pixel, &g_QuadPixelShader, QUAD_PIXEL_SHADER_SRC));

		svCheck(graphics_shader_compile_fastbin_from_string("DefaultRenderer_EllipseVertex", ShaderType_Vertex, &g_EllipseVertexShader, ELLIPSE_VERTEX_SHADER_SRC));
		svCheck(graphics_shader_compile_fastbin_from_string("DefaultRenderer_EllipsePixel", ShaderType_Pixel, &g_EllipsePixelShader, ELLIPSE_PIXEL_SHADER_SRC));

		svCheck(graphics_shader_compile_fastbin_from_string("DefaultRenderer_SpriteVertex", ShaderType_Vertex, &g_SpriteVertexShader, SPRITE_VERTEX_SHADER_SRC));
		svCheck(graphics_shader_compile_fastbin_from_string("DefaultRenderer_SpritePixel", ShaderType_Pixel, &g_SpritePixelShader, SPRITE_PIXEL_SHADER_SRC));

		// Create RenderPass
		{
			AttachmentDesc att;
			att.loadOp = AttachmentOperation_Load;
			att.storeOp = AttachmentOperation_Store;
			att.stencilLoadOp = AttachmentOperation_DontCare;
			att.stencilStoreOp = AttachmentOperation_DontCare;
			att.format = OFFSCREEN_FORMAT;
			att.initialLayout = GPUImageLayout_RenderTarget;
			att.layout = GPUImageLayout_RenderTarget;
			att.finalLayout = GPUImageLayout_RenderTarget;
			att.type = AttachmentType_RenderTarget;

			RenderPassDesc desc;
			desc.attachmentCount = 1u;
			desc.pAttachments = &att;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass));
		}

		// Create Input layout
		{
			InputSlotDesc slot;
			slot.instanced = false;
			slot.slot = 0u;
			slot.stride = sizeof(DebugData);

			InputElementDesc elements[] = 
			{ 
				{ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT },
				{ "TexCoord", 0u, 0u, 4u * sizeof(float), Format_R32G32_FLOAT },
				{ "Stroke", 0u, 0u, 6u * sizeof(float), Format_R32_FLOAT },
				{ "Color", 0u, 0u, 7u * sizeof(float), Format_R8G8B8A8_UNORM },
			};

			InputLayoutStateDesc desc;
			desc.elementCount = 4u;
			desc.slotCount = 1u;
			desc.pElements = elements;
			desc.pSlots = &slot;

			svCheck(graphics_inputlayoutstate_create(&desc, &g_InputLayout));
		}
		
		// Create Blend State
		{
			BlendAttachmentDesc att;
			att.blendEnabled = true;
			att.srcColorBlendFactor = BlendFactor_SrcAlpha;
			att.dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			att.colorBlendOp = BlendOperation_Add;
			att.srcAlphaBlendFactor = BlendFactor_One;
			att.dstAlphaBlendFactor = BlendFactor_One;
			att.alphaBlendOp = BlendOperation_Add;
			att.colorWriteMask = ColorComponent_All;


			BlendStateDesc desc;
			desc.attachmentCount = 1u;
			desc.pAttachments = &att;
			desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };

			svCheck(graphics_blendstate_create(&desc, &g_BS_Geometry));
		}

		// Create Def sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Nearest;
			desc.magFilter = SamplerFilter_Nearest;

			svCheck(graphics_sampler_create(&desc, &g_DefSampler));
		}

		return Result_Success;
	}

	Result debug_renderer_close()
	{
		for (u32 i = 0; i < GraphicsLimit_CommandList; ++i) graphics_destroy(g_VertexBuffer[i]);
		graphics_destroy(g_RenderPass);
		graphics_destroy(g_InputLayout);
		graphics_destroy(g_BS_Geometry);
		graphics_destroy(g_DefSampler);
		graphics_destroy(g_QuadVertexShader);
		graphics_destroy(g_QuadPixelShader);
		graphics_destroy(g_EllipseVertexShader);
		graphics_destroy(g_EllipsePixelShader);
		graphics_destroy(g_SpriteVertexShader);
		graphics_destroy(g_SpritePixelShader);

		return Result_Success;
	}

	Result debug_renderer_create_buffer(CommandList cmd)
	{
		if (g_VertexBuffer[cmd] == nullptr) {

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = BATCH_COUNT * sizeof(DebugData);
			desc.pData = nullptr;

			return graphics_buffer_create(&desc, &g_VertexBuffer[cmd]);

		}

		return Result_Success;
	}

	// BEGIN - END

	constexpr u32 debug_renderer_vertex_count(u32 list)
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
			SV_LOG_ERROR("Unknown list: %u", list);
			return 0u;
		}
	}

	void debug_renderer_draw_call(u32 batchOffset, const DebugRendererDraw& draw, u32 vertexCount, GPUBuffer* buffer, CommandList cmd)
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

			graphics_image_bind(draw.pImage, 0u, ShaderType_Pixel, cmd);

			if (draw.pSampler) {
				graphics_sampler_bind(draw.pSampler, 0u, ShaderType_Pixel, cmd);
			}
			else {
				graphics_sampler_bind(g_DefSampler, 0u, ShaderType_Pixel, cmd);
			}

			break;

		}

		graphics_draw(vertexCount, 1u, batchOffset, 0u, cmd);
	}

	void debug_renderer_draw_batch(const DebugRendererDraw* begin, u32 beginIndex, const DebugRendererDraw* end, u32 endIndex, u32 batchCount, DebugData* batchData, GPUImage* renderTarget, CommandList cmd)
	{
		GPUBuffer* buffer = g_VertexBuffer[cmd];

		graphics_buffer_update(buffer, batchData, batchCount * sizeof(DebugData), 0u, cmd);

		GPUImage* attachments[] = {
			renderTarget
		};

		graphics_renderpass_begin(g_RenderPass, attachments, nullptr, 1.f, 0u, cmd);

		graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);

		u32 batchOffset = 0u;
		
		if (begin != end) {

			{
				u32 count = begin->count - (beginIndex - begin->index);
				u32 vertexCount = debug_renderer_vertex_count(begin->list);
				debug_renderer_draw_call(0u, *begin, vertexCount * count, buffer, cmd);

				batchOffset =+ vertexCount * count;
			}

			const DebugRendererDraw* it = begin + 1u;
			while (it != end) {
				u32 vertexCount = debug_renderer_vertex_count(it->list);
				debug_renderer_draw_call(batchOffset, *it, vertexCount * it->count, buffer, cmd);
				batchOffset += vertexCount * it->count;
				++it;
			}

		}
		
		u32 vertexCount = debug_renderer_vertex_count(end->list);
		debug_renderer_draw_call(batchOffset, *end, vertexCount * (endIndex - end->index), buffer, cmd);

		graphics_renderpass_end(cmd);

	}

	DebugRenderer::~DebugRenderer()
	{
		destroy();
	}

	Result DebugRenderer::create()
	{
		pInternal = new DebugRendererBatch_internal();
		reset();
		return Result_Success;
	}

	Result DebugRenderer::destroy()
	{
		if (pInternal == nullptr) return Result_Success;
		parseBatch();

		delete& batch;
		pInternal = nullptr;
		return Result_Success;
	}

	void DebugRenderer::reset()
	{
		parseBatch();
		batch.quads.reset();
		batch.lines.reset();
		batch.ellipses.reset();
		batch.sprites.reset();
		batch.drawCalls.reset();
		batch.drawCalls.emplace_back().list = u32_max;
	}

	void DebugRenderer::render(GPUImage* renderTarget, const XMMATRIX& viewProjectionMatrix, CommandList cmd)
	{
		parseBatch();
		if (batch.drawCalls.size() <= 1u)
			return;

		sv::Result result = debug_renderer_create_buffer(cmd);
		SV_ASSERT(result_okay(result));

		graphics_mode_set(GraphicsPipelineMode_Graphics, cmd);

		graphics_inputlayoutstate_bind(g_InputLayout, cmd);
		graphics_blendstate_bind(g_BS_Geometry, cmd);

		const DebugRendererDraw* end = batch.drawCalls.data() + batch.drawCalls.size();
		const DebugRendererDraw* it = batch.drawCalls.data() + 1u;

		// Draw Data
		const DebugRendererDraw* beginIt = it;
		u32 beginIndex = u32_max;

		// Update Data
		DebugData batchData[BATCH_COUNT];
		DebugData* itBatch = batchData;
		DebugData* endBatch = batchData + BATCH_COUNT;

		// Vertex vectors
		XMVECTOR v0;
		XMVECTOR v1;
		XMVECTOR v2;
		XMVECTOR v3;

		v0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
		v1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
		v2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
		v3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

		XMVECTOR p0;
		XMVECTOR p1;
		XMVECTOR p2;
		XMVECTOR p3;

		while (it != end) {

			DebugRendererDraw draw = *it;

			u32 vertexCount = debug_renderer_vertex_count(draw.list);
			u32 currentIndex = draw.index;

			if (beginIndex == u32_max) {
				beginIndex = draw.index;
				beginIt = it;
			}

			while (draw.count) {

				u32 capacity = u32(endBatch - itBatch) / vertexCount;

				u32 batchUsage = std::min(capacity, draw.count);

				draw.count -= batchUsage;

				DebugData* itBatchEnd = itBatch + (batchUsage * vertexCount);

				switch (draw.list)
				{
				case 0:
				case 2:
				case 3:
				{
					while (itBatch != itBatchEnd) {

						XMMATRIX mvpMatrix;
						Color color;
						v4_f32 texCoord;
						float stroke = 1.f;

						switch (draw.list)
						{
						case 0:
						{
							DebugRendererQuad& quad = batch.quads[currentIndex];

							mvpMatrix = quad.matrix;
							color = quad.color;
							stroke = draw.stroke;
							texCoord.x = 69.f;
						}

						break;

						case 2:
						{
							DebugRendererQuad& ellipse = batch.ellipses[currentIndex];

							mvpMatrix = ellipse.matrix;
							color = ellipse.color;
							stroke = draw.stroke;
							texCoord.x = 69.f;
						}
						break;

						case 3:
						{
							DebugRendererQuad& spr = batch.sprites[currentIndex];

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

						p0 = v0;
						p0 = XMVector3Transform(p0, mvpMatrix);
						
						p1 = v1;
						p1 = XMVector3Transform(p1, mvpMatrix);
						
						p2 = v2;
						p2 = XMVector3Transform(p2, mvpMatrix);
						
						p3 = v3;
						p3 = XMVector3Transform(p3, mvpMatrix);

						itBatch->position = v4_f32(p0);
						itBatch->texCoord = { texCoord.x, texCoord.y };
						itBatch->stroke = stroke;
						itBatch->color = color;
						++itBatch;

						itBatch->position = v4_f32(p1);
						itBatch->texCoord = { texCoord.z, texCoord.y };
						itBatch->stroke = stroke;
						itBatch->color = color;
						++itBatch;

						itBatch->position = v4_f32(p2);
						itBatch->texCoord = { texCoord.x, texCoord.w };
						itBatch->stroke = stroke;
						itBatch->color = color;
						++itBatch;

						itBatch->position = v4_f32(p1);
						itBatch->texCoord = { texCoord.z, texCoord.y };
						itBatch->stroke = stroke;
						itBatch->color = color;
						++itBatch;

						itBatch->position = v4_f32(p3);
						itBatch->texCoord = { texCoord.z, texCoord.w };
						itBatch->stroke = stroke;
						itBatch->color = color;
						++itBatch;

						itBatch->position = v4_f32(p2);
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
					while (itBatch != itBatchEnd) {

						DebugRendererLine& line = batch.lines[currentIndex];

						p0 = XMVector4Transform(XMVectorSet(line.point0.x, line.point0.y, line.point0.z, 1.f), viewProjectionMatrix);
						p1 = XMVector4Transform(XMVectorSet(line.point1.x, line.point1.y, line.point1.z, 1.f), viewProjectionMatrix);

						itBatch->position = v4_f32(p0);
						itBatch->color = line.color;
						itBatch->stroke = 1.f;
						++itBatch;

						itBatch->position = v4_f32(p1);
						itBatch->color = line.color;
						itBatch->stroke = 1.f;
						++itBatch;

						++currentIndex;
					}
				}
				break;

				}

				// Update & draw
				if ((capacity - batchUsage) * vertexCount < 6u) { // 6 because is the max vertex count

					debug_renderer_draw_batch(beginIt, beginIndex, it, currentIndex, u32(itBatch - batchData), batchData, renderTarget, cmd);

					itBatch = batchData;

					if (draw.count) {
						beginIt = it;
						beginIndex = currentIndex;
					}
					else {
						beginIndex = u32_max;
					}
				}

			}

			++it;
		}

		if (beginIndex != u32_max) {
			--it;
			debug_renderer_draw_batch(beginIt, beginIndex, it, it->index + it->count, u32(itBatch - batchData), batchData, renderTarget, cmd);
		}
	}

	void DebugRenderer::drawQuad(const XMMATRIX& matrix, Color color)
	{
		parseBatch();

		if (batch.drawCalls.back().list == 0u && batch.drawCalls.back().stroke == batch.stroke) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(0u, u32(batch.quads.size()), batch.stroke);
		}

		batch.quads.emplace_back(matrix, color);
	}

	void DebugRenderer::drawLine(const v3_f32& p0, const v3_f32& p1, Color color)
	{
		parseBatch();

		if (batch.drawCalls.back().list == 1u && batch.drawCalls.back().lineWidth == batch.lineWidth) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(1u, u32(batch.lines.size()), batch.lineWidth);
		}

		batch.lines.emplace_back(p0, p1, color);
	}

	void DebugRenderer::drawEllipse(const XMMATRIX& matrix, Color color)
	{
		parseBatch();

		if (batch.drawCalls.back().list == 2u && batch.drawCalls.back().stroke == batch.stroke) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(2u, u32(batch.ellipses.size()), batch.stroke);
		}

		batch.ellipses.emplace_back(matrix, color);
	}

	void DebugRenderer::drawSprite(const XMMATRIX& matrix, Color color, GPUImage* image)
	{
		parseBatch();

		if (batch.sameSprite && batch.drawCalls.back().list == 3u && batch.drawCalls.back().pImage == image) ++batch.drawCalls.back().count;
		else {
			batch.drawCalls.emplace_back(3u, u32(batch.sprites.size()), image, batch.pSampler, batch.texCoord);
			batch.sameSprite = true;
		}

		batch.sprites.emplace_back(matrix, color);
	}

	void DebugRenderer::drawQuad(const v3_f32& position, const v2_f32& size, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		drawQuad(tm, color);
	}

	void DebugRenderer::drawQuad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		drawQuad(tm, color);
	}

	void DebugRenderer::drawQuad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationQuaternion(rotationQuat.get_dx()) * XMMatrixTranslation(position.x, position.y, position.z);
		drawQuad(tm, color);
	}

	void DebugRenderer::drawEllipse(const v3_f32& position, const v2_f32& size, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		drawEllipse(tm, color);
	}

	void DebugRenderer::drawEllipse(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		drawEllipse(tm, color);
	}

	void DebugRenderer::drawEllipse(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationQuaternion(rotationQuat.get_dx()) * XMMatrixTranslation(position.x, position.y, position.z);
		drawEllipse(tm, color);
	}

	void DebugRenderer::drawSprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);
		drawSprite(tm, color, image);
	}

	void DebugRenderer::drawSprite(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, GPUImage* image)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) * XMMatrixTranslation(position.x, position.y, position.z);
		drawSprite(tm, color, image);
	}

	void DebugRenderer::drawSprite(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, GPUImage* image)
	{
		XMMATRIX tm = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixRotationQuaternion(rotationQuat.get_dx()) * XMMatrixTranslation(position.x, position.y, position.z);
		drawSprite(tm, color, image);
	}

	void DebugRenderer::setlinewidth(f32 lineWidth)
	{
		parseBatch();
		batch.lineWidth = lineWidth;
	}

	f32 DebugRenderer::getlinewidth()
	{
		parseBatch();
		return batch.lineWidth;
	}

	void DebugRenderer::setStroke(f32 stroke)
	{
		parseBatch();
		batch.stroke = stroke;
	}

	f32 DebugRenderer::getStroke()
	{
		parseBatch();
		return batch.stroke;
	}

	void DebugRenderer::setTexcoord(const v4_f32& texCoord)
	{
		parseBatch();

		batch.texCoord = texCoord;
		batch.sameSprite = false;
	}

	v4_f32 DebugRenderer::getTexcoord()
	{
		parseBatch();
		return batch.texCoord;
	}

	void DebugRenderer::setSamplerDefault()
	{
		parseBatch();
		batch.pSampler = nullptr;
	}

	void DebugRenderer::setSampler(Sampler* sampler)
	{
		parseBatch();
		batch.pSampler = sampler;
	}

	void DebugRenderer::drawOrthographicGrip(const v2_f32& position, const v2_f32& size, float gridSize, Color color)
	{
		v2_f32 begin = position - size / 2.f;
		v2_f32 end = begin + size;

		for (float y = i32(begin.y / gridSize) * gridSize; y < end.y; y += gridSize) {
			drawLine({ begin.x, y, 0.f }, { end.x, y, 0.f }, color);
		}
		for (float x = i32(begin.x / gridSize) * gridSize; x < end.x; x += gridSize) {
			drawLine({ x, begin.y, 0.f }, { x, end.y, 0.f }, color);
		}
	}

}
