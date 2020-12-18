#include "core.h"

#include "postprocessing_internal.h"

namespace sv {

	// TONE MAPPING PASS

	static RenderPass*	g_RenderPass_ToneMapping = nullptr;
	static Shader*		g_PS_ToneMapping = nullptr;
	static Sampler*		g_Sampler_ToneMapping = nullptr;

	Result pp_toneMappingInitialize()
	{
		svCheck(graphics_shader_compile_fastbin("PS_ToneMapping", ShaderType_Pixel, &g_PS_ToneMapping, R"(
#include "core.hlsl"

struct Input {
	float2 texCoord : FragTexCoord;
};

struct Output {
	float4 color : SV_Target0;
};

//SV_CONSTANT_BUFFER(ToneMappingBuffer, b0) {
//	f32 exposure;
//};
SV_TEXTURE(Tex, t0);
SV_SAMPLER(Sam, s0);

Output main(Input input) 
{
	Output output;
	
	output.color = Tex.Sample(Sam, input.texCoord);

	// HDR to LDR
	output.color.rgb = 1.f - exp(-output.color.rgb * 0.3f);
	
	return output;
}
		)", true));

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

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_ToneMapping));
			graphics_name_set(g_RenderPass_ToneMapping, "RenderPass_ToneMapping");
		}

		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Linear;
			desc.magFilter = SamplerFilter_Linear;

			svCheck(graphics_sampler_create(&desc, &g_Sampler_ToneMapping));
			graphics_name_set(g_Sampler_ToneMapping, "Sampler_ToneMapping");
		}

		return Result_Success;
	}

	Result pp_toneMappingClose()
	{
		return Result_Success;
	}

	void PostProcessing::toneMapping(GPUImage* img, GPUImageLayout imgLayout, GPUImageLayout newLayout, f32 exposure, CommandList cmd)
	{
		GPUImage* aux;
		size_t auxIndex;

		// Get aux image
		{
			u64 id = auximg_id(graphics_image_get_width(img), graphics_image_get_height(img), OFFSCREEN_FORMAT, GPUImageType_ShaderResource | GPUImageType_RenderTarget);

			auto res = auximg_push(id, GPUImageLayout_RenderTarget, cmd);

			aux = res.first;
			auxIndex = res.second;

			if (aux == nullptr) {
				SV_LOG_ERROR("Can't do tone mapping without auxiliar images");
				return;
			}
		}

		if (imgLayout != GPUImageLayout_ShaderResource) {
			GPUBarrier barrier = GPUBarrier::Image(img, imgLayout, GPUImageLayout_ShaderResource);
			graphics_barrier(&barrier, 1u, cmd);
		}

		graphics_state_unbind(cmd);

		graphics_viewport_set(img, 0u, cmd);
		graphics_scissor_set(img, 0u, cmd);

		graphics_shader_bind(g_VS_DefPostProcessing, cmd);
		graphics_shader_bind(g_PS_ToneMapping, cmd);

		GPUImage* att[1u];
		att[0] = aux;

		graphics_sampler_bind(g_Sampler_ToneMapping, 0u, ShaderType_Pixel, cmd);
		graphics_image_bind(img, 0u, ShaderType_Pixel, cmd);

		graphics_renderpass_begin(g_RenderPass_ToneMapping, att, nullptr, 0.f, 0u, cmd);
		graphics_draw(3u, 1u, 0u, 0u, cmd);
		graphics_renderpass_end(cmd);

		GPUImageBlit blit;
		blit.srcRegion.offset = 0u;
		blit.srcRegion.size = { graphics_image_get_width(img), graphics_image_get_height(img), 1u };
		blit.dstRegion = blit.srcRegion;

		graphics_image_blit(aux, img, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource, 1u, &blit, SamplerFilter_Nearest, cmd);

		if (newLayout != GPUImageLayout_ShaderResource) {
			GPUBarrier barrier = GPUBarrier::Image(img, GPUImageLayout_ShaderResource, newLayout);
			graphics_barrier(&barrier, 1u, cmd);
		}

		auximg_pop(auxIndex, GPUImageLayout_RenderTarget, cmd);
	}

}