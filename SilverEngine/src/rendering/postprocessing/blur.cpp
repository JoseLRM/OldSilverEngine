#include "core.h"

#include "postprocessing_internal.h"
#include "rendering/render_utils.h"

namespace sv {

	struct BlurBoxData {
		u32 srcWidth;
		u32 srcHeight;
		u32 steps;
		f32 stepRange;
	};

	static Shader* g_VS_Default = nullptr;

	// BOX BLUR

	static Shader* g_PS_BoxBlur = nullptr;
	static BlendState* g_BS_AddBlur = nullptr;
	static GPUBuffer* g_BufferBoxBlur = nullptr;
	static RenderPass* g_RenderPass_BoxBlur = nullptr;
	static Sampler* g_SamplerBlur = nullptr;

	Result pp_blurInitialize()
	{
		// Create Shaders
		svCheck(graphics_shader_compile_fastbin("PPShader_Default", ShaderType_Vertex, &g_VS_Default, R"(
#include "core.hlsl"

struct Output {
	
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;

};

Output main(u32 id : SV_VertexID)
{
	Output output;

	//output.texCoord = float2((id << 1) & 2, id & 2);
	//output.position = float4(output.texCoord * float2(2, -2) + float2(-1, 1), 0, 1);

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

		svCheck(graphics_shader_compile_fastbin("PPShader_BoxBlur", ShaderType_Pixel, &g_PS_BoxBlur, R"(
#include "core.hlsl"

struct Input {
	
	float2 texCoord : FragTexCoord;

};

struct Output {
	
	float4 color : SV_Target0;

};

SV_CONSTANT_BUFFER(BlurBuffer, b0) {
	u32 srcWidth;
	u32 srcHeight;
	u32 steps;
	f32 stepRange;
};

SV_TEXTURE(Src, t0);
SV_SAMPLER(Sam, s0);

Output main(Input input)
{
	Output output;
	
	output.color = 0.f;

	float range = steps * stepRange;
	
	// VERTICAL
	if (stepRange > 0.f) {
	
		f32 i = input.texCoord.y - (range / 2.f);
		f32 end = i + range;
		
		for (; i < end; i += stepRange) {		
			output.color += Src.Sample(Sam, float2(input.texCoord.x, i));
		}
	}
	// HORIZONTAL
	else {

		// Set stepRange and range positive
		range = -range;
		f32 stepRange_ = -stepRange;

		f32 i = input.texCoord.x - (range / 2.f);
		f32 end = i + range;
		
		for (; i < end; i += stepRange_) {		
			output.color += Src.Sample(Sam, float2(i, input.texCoord.y));
		}
	}

	output.color /= steps;

	return output;
}
		)"));

		// Create RenderPasses
		{
			RenderPassDesc desc;
			desc.attachments.resize(1u);
			
			desc.attachments[0].loadOp = AttachmentOperation_DontCare;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].format = OFFSCREEN_FORMAT;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_BoxBlur));
			graphics_name_set(g_RenderPass_BoxBlur, "RenderPass_BoxBlur");
		}

		// Create Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(BlurBoxData);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, &g_BufferBoxBlur));
			graphics_name_set(g_BufferBoxBlur, "CBuffer_BoxBlurData");
		}

		// Create Blur Sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Mirror;
			desc.addressModeV = SamplerAddressMode_Mirror;
			desc.addressModeW = SamplerAddressMode_Mirror;
			desc.minFilter = SamplerFilter_Linear;
			desc.magFilter = SamplerFilter_Linear;

			svCheck(graphics_sampler_create(&desc, &g_SamplerBlur));
			graphics_name_set(g_SamplerBlur, "Sampler_Blur");
		}

		// Create Blend State
		{
			BlendStateDesc desc;
			desc.attachments.resize(1u);
			desc.attachments[0].blendEnabled = true;
			desc.attachments[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			desc.attachments[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			desc.attachments[0].colorBlendOp = BlendOperation_Add;
			desc.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].alphaBlendOp = BlendOperation_Add;
			desc.attachments[0].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&desc, &g_BS_AddBlur));
			graphics_name_set(g_BS_AddBlur, "BlendState_AddBlur");
		}

		return Result_Success;
	}

	Result pp_blurClose()
	{
		svCheck(graphics_destroy(g_BufferBoxBlur));
		svCheck(graphics_destroy(g_BS_AddBlur));
		svCheck(graphics_destroy(g_RenderPass_BoxBlur));
		svCheck(graphics_destroy(g_SamplerBlur));
		svCheck(graphics_destroy(g_PS_BoxBlur));
		svCheck(graphics_destroy(g_VS_Default));

		return Result_Success;
	}

	SV_INLINE static void prepareBlur(CommandList cmd)
	{
		graphics_inputlayoutstate_unbind(cmd);
		graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);
	}

	SV_INLINE static void blurDrawCall(GPUImage** att, BlurBoxData& data, CommandList cmd)
	{
		graphics_buffer_update(g_BufferBoxBlur, &data, sizeof(BlurBoxData), 0u, cmd);

		graphics_renderpass_begin(g_RenderPass_BoxBlur, att, nullptr, 0.f, 0u, cmd);
		graphics_draw(4u, 1u, 0u, 0u, cmd);
		graphics_renderpass_end(cmd);
	}

	SV_INLINE static bool isSupportedFormat(Format format)
	{
		switch (format)
		{
		case OFFSCREEN_FORMAT:
			return true;
		default:
			return false;
		}
	}

	void PostProcessing::blurBox(GPUImage* img, GPUImageLayout imgLayout, f32 range, u32 samples, bool vertical, bool horizontal, CommandList cmd)
	{
		SV_ASSERT((vertical || horizontal) && range > 0.f && samples != 0u);

		u32 imgWidth = graphics_image_get_width(img);
		u32 imgHeight = graphics_image_get_height(img);
		Format format = graphics_image_get_format(img);

		// TODO: Made a unefficient way to unsoported formats
		if (!isSupportedFormat(format)) {
			SV_LOG_ERROR("Can't do a blur effect with this format");
			return;
		}

		GPUImage* aux;
		size_t auxIndex;

		// Get aux image
		{
			// TODO: Optimization, reduce aux img size to do some blur with sampling

			u64 auxID = auximg_id(imgWidth, imgHeight, format, GPUImageType_RenderTarget | GPUImageType_ShaderResource);
			auto res = auximg_push(auxID, GPUImageLayout_RenderTarget, cmd);

			aux = res.first;
			auxIndex = res.second;

			if (aux == nullptr) {
				SV_LOG_ERROR("Can't do a blur effect without auxiliar images");
				return;
			}
		}

		// Change Image Layout to ShaderResource
		if (imgLayout != GPUImageLayout_ShaderResource) {
			GPUBarrier barrier;
			barrier = GPUBarrier::Image(img, imgLayout, GPUImageLayout_ShaderResource);
			graphics_barrier(&barrier, 1u, cmd);
		}
		
		prepareBlur(cmd);
		graphics_shader_bind(g_VS_Default, cmd);
		graphics_shader_bind(g_PS_BoxBlur, cmd);
		graphics_constantbuffer_bind(g_BufferBoxBlur, 0u, ShaderType_Pixel, cmd);
		graphics_image_bind(img, 0u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(g_SamplerBlur, 0u, ShaderType_Pixel, cmd);
		graphics_blendstate_unbind(cmd);

		GPUImage* att[1u];
		att[0] = aux;

		if (samples % 2u == 0u) ++samples;

		BlurBoxData data;
		data.stepRange = range / samples;
		data.steps = samples;
		data.srcWidth = imgWidth;
		data.srcHeight = imgHeight;

		if (vertical && horizontal) {

			// Vertical rendering

			blurDrawCall(att, data, cmd);

			GPUBarrier barriers[2u];
			barriers[0] = GPUBarrier::Image(aux, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
			barriers[1] = GPUBarrier::Image(img, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
			graphics_barrier(barriers, 2u, cmd);

			graphics_blendstate_bind(g_BS_AddBlur, cmd);
			graphics_image_bind(aux, 0u, ShaderType_Pixel, cmd);

			att[0] = img;
			data.stepRange = -data.stepRange;
			blurDrawCall(att, data, cmd);

			// Change ImageLayout to the current layout
			if (imgLayout != GPUImageLayout_RenderTarget) {
				barriers[0] = GPUBarrier::Image(img, GPUImageLayout_RenderTarget, imgLayout);
				graphics_barrier(barriers, 1u, cmd);
			}

			auximg_pop(auxIndex, GPUImageLayout_ShaderResource, cmd);
		}
		else {
			
			if (horizontal) data.stepRange = -data.stepRange;

			blurDrawCall(att, data, cmd);

			// Blit image
			{
				GPUImageBlit blit;
				blit.srcRegion.offset = 0u;
				blit.srcRegion.size.x = imgWidth;
				blit.srcRegion.size.y = imgHeight;
				blit.srcRegion.size.z = 1u;
				blit.dstRegion = blit.srcRegion;

				graphics_image_blit(aux, img, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource, 1u, &blit, SamplerFilter_Nearest, cmd);
			}

			if (imgLayout != GPUImageLayout_ShaderResource) {
				GPUBarrier barrier = GPUBarrier::Image(img, GPUImageLayout_ShaderResource, imgLayout);
				graphics_barrier(&barrier, 1u, cmd);
			}

			auximg_pop(auxIndex, GPUImageLayout_RenderTarget, cmd);
		}
	}

	void PostProcessing::blurGaussian()
	{

	}

	void PostProcessing::blurSolid()
	{

	}

}