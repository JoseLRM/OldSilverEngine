#include "core.h"

#include "postprocessing_internal.h"

namespace sv {

	struct BloomThresholdData {
		f32 threshold;
		v3_f32 padding;
	};

	struct BloomBlurData {
		f32 range;
		v3_f32 padding;
	};

	// THRESHOLD PASS

	static RenderPass*	g_RenderPass_Threshold = nullptr;
	static BlendState*	g_BS_Threshold = nullptr;
	static Shader*		g_PS_Threshold = nullptr;
	static GPUBuffer*	g_Buffer_Threshold = nullptr;
	static Sampler*		g_Sampler_Threshold = nullptr;

	// EMISSIVE PASS

	static RenderPass*	g_RenderPass_Emissive = nullptr;
	static BlendState*	g_BS_Emissive = nullptr;
	static Sampler*		g_Sampler_Emissive = nullptr;

	// BLUR PASS

	static RenderPass*	g_RenderPass_Blur = nullptr;
	static BlendState*	g_BS_Blur = nullptr;
	static Shader*		g_PS_Blur = nullptr;
	static GPUBuffer*	g_Buffer_Blur = nullptr;
	static Sampler*		g_Sampler_Blur = nullptr;

	// ADDBLOOM PASS

	static RenderPass* g_RenderPass_AddBloom = nullptr;
	static BlendState* g_BS_AddBloom = nullptr;


	Result pp_bloomInitialize()
	{
		svCheck(graphics_shader_compile_fastbin_from_string("PS_BloomThreshold", ShaderType_Pixel, &g_PS_Threshold, R"(
#include "core.hlsl"

struct Input {
	float2 texCoord : FragTexCoord;
};

struct Output {
	float4 color : SV_Target0;
};

SV_TEXTURE(Tex, t0);
SV_CONSTANT_BUFFER(BloomBuffer, b0) {
	f32 threshold;
};
SV_SAMPLER(Sam, s0);

Output main(Input input) 
{
	Output output;
	
	output.color = Tex.Sample(Sam, input.texCoord);
	
	// TODO: Should use the alpha??
	float intensity = dot(output.color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
	if (intensity < threshold) discard;

	return output;
}
		)", true));

		svCheck(graphics_shader_compile_fastbin_from_string("PS_BloomBlur", ShaderType_Pixel, &g_PS_Blur, R"(
#include "core.hlsl"

struct Input {
	float2 texCoord : FragTexCoord;
};

struct Output {
	float4 color : SV_Target0;
};

SV_TEXTURE(Tex, t0);
SV_CONSTANT_BUFFER(BlurBuffer, b0) {
	f32 range;
};
SV_SAMPLER(Sam, s0);

static const u32 SAMPLES = 17u;	

static const f32 weights[] = {

	0.08714879632386216,
	0.08509506327289246,
	0.07922052979655275,
	0.07031660076677806,
	0.059506661727330365,
	0.048013354392469045,
	0.03693577601198269,
	0.027090706403713718,
	0.018944381870605587,
	0.012630687290649084,
	0.008028971914424055,
	0.004866086962193581,
	0.00281182201322326,
	0.0015491206978279991,
	0.0008137178183738435,
	0.0004075259886298812,
	0.00019459491042249266

};

Output main(Input input) 
{
	Output output;

	f32 stepRange = abs(range) / f32(SAMPLES);

	output.color.rgb = Tex.Sample(Sam, input.texCoord).rgb * weights[0];
	output.color.a = 1.f;
	
	// Vertical
	if (range > 0.f) {

		for (i32 i = 1; i < SAMPLES; ++i) {
			output.color.xyz += Tex.Sample(Sam, float2(input.texCoord.x, input.texCoord.y + stepRange * i)).xyz * weights[i];
			output.color.xyz += Tex.Sample(Sam, float2(input.texCoord.x, input.texCoord.y - stepRange * i)).xyz * weights[i];
		}
	}
	// Horizontal
	else {
		
		for (i32 i = 1; i < SAMPLES; ++i) {
			output.color.xyz += Tex.Sample(Sam, float2(input.texCoord.x + stepRange * i, input.texCoord.y)).xyz * weights[i];
			output.color.xyz += Tex.Sample(Sam, float2(input.texCoord.x - stepRange * i, input.texCoord.y)).xyz * weights[i];
		}
	}

	return output;
}
		)", true));

		graphics_name_set(g_PS_Threshold, "PS_BloomThreshold");
		graphics_name_set(g_PS_Blur, "PS_BloomBlur");

		// Create Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(BloomThresholdData);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, &g_Buffer_Threshold));
			graphics_name_set(g_Buffer_Threshold, "CBuffer_BloomThreshold");

			desc.size = sizeof(BloomBlurData);

			svCheck(graphics_buffer_create(&desc, &g_Buffer_Blur));
			graphics_name_set(g_Buffer_Blur, "CBuffer_BloomBlur");
		}

		// Create Samplers
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Linear;
			desc.magFilter = SamplerFilter_Linear;

			svCheck(graphics_sampler_create(&desc, &g_Sampler_Threshold));
			graphics_name_set(g_Sampler_Threshold, "Sampler_BloomThreshold");

			svCheck(graphics_sampler_create(&desc, &g_Sampler_Emissive));
			graphics_name_set(g_Sampler_Emissive, "Sampler_BloomEmissive");

			desc.addressModeU = SamplerAddressMode_Mirror;
			desc.addressModeV = SamplerAddressMode_Mirror;
			desc.addressModeW = SamplerAddressMode_Mirror;

			svCheck(graphics_sampler_create(&desc, &g_Sampler_Blur));
			graphics_name_set(g_Sampler_Blur, "Sampler_BloomBlur");
		}

		// Create RenderPasses
		{
			RenderPassDesc desc;
			desc.attachments.resize(1u);

			desc.attachments[0].loadOp = AttachmentOperation_Clear;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].format = GBuffer::FORMAT_OFFSCREEN;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Threshold));
			graphics_name_set(g_RenderPass_Threshold, "RenderPass_BloomThreshold");

			desc.attachments[0].loadOp = AttachmentOperation_Load;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Emissive));
			graphics_name_set(g_RenderPass_Emissive, "RenderPass_BloomEmissive");

			desc.attachments[0].loadOp = AttachmentOperation_DontCare;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Blur));
			graphics_name_set(g_RenderPass_Blur, "RenderPass_BloomBlur");

			desc.attachments[0].loadOp = AttachmentOperation_Load;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_AddBloom));
			graphics_name_set(g_RenderPass_AddBloom, "RenderPass_BloomAdd");
		}

		// Create Blend States
		{
			BlendStateDesc desc;
			desc.attachments.resize(1u);
			desc.attachments[0].blendEnabled = true;
			desc.attachments[0].srcColorBlendFactor = BlendFactor_One;
			desc.attachments[0].dstColorBlendFactor = BlendFactor_One;
			desc.attachments[0].colorBlendOp = BlendOperation_Add;
			desc.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].alphaBlendOp = BlendOperation_Add;
			desc.attachments[0].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&desc, &g_BS_Threshold));
			graphics_name_set(g_BS_Threshold, "BlendState_BloomThreshold");

			svCheck(graphics_blendstate_create(&desc, &g_BS_Emissive));
			graphics_name_set(g_BS_Emissive, "BlendState_BloomEmissive");

			desc.attachments[0].dstColorBlendFactor = BlendFactor_Zero;
			desc.attachments[0].dstAlphaBlendFactor = BlendFactor_Zero;

			svCheck(graphics_blendstate_create(&desc, &g_BS_Blur));
			graphics_name_set(g_BS_Blur, "BlendState_BloomBlur");

			desc.attachments[0].srcColorBlendFactor = BlendFactor_One;
			desc.attachments[0].dstColorBlendFactor = BlendFactor_One;
			desc.attachments[0].colorBlendOp = BlendOperation_Add;
			desc.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].alphaBlendOp = BlendOperation_Add;

			svCheck(graphics_blendstate_create(&desc, &g_BS_AddBloom));
			graphics_name_set(g_BS_AddBloom, "BlendState_BloomAdd");
		}

		return Result_Success;
	}

	Result pp_bloomClose()
	{
		svCheck(graphics_destroy(g_RenderPass_AddBloom));
		svCheck(graphics_destroy(g_RenderPass_Blur));
		svCheck(graphics_destroy(g_RenderPass_Threshold));
		svCheck(graphics_destroy(g_BS_AddBloom));
		svCheck(graphics_destroy(g_BS_Threshold));
		svCheck(graphics_destroy(g_BS_Blur));
		svCheck(graphics_destroy(g_Buffer_Blur));
		svCheck(graphics_destroy(g_Buffer_Threshold));
		svCheck(graphics_destroy(g_Sampler_Blur));
		svCheck(graphics_destroy(g_Sampler_Threshold));
		svCheck(graphics_destroy(g_PS_Blur));
		svCheck(graphics_destroy(g_PS_Threshold));

		return Result_Success;
	}

	void postprocess_bloom(
		GPUImage* img,
		GPUImageLayout imgOldLayout,
		GPUImageLayout imgNewLayout,
		GPUImage* emission,
		GPUImageLayout emissionOldLayout,
		GPUImageLayout emissionNewLayout,
		f32 threshold,
		f32 range,
		u32 iterations,
		CommandList cmd
	)
	{
		SV_ASSERT(graphics_image_get_format(img) == GBuffer::FORMAT_OFFSCREEN);

		u32 imgWidth = graphics_image_get_width(img);
		u32 imgHeight = graphics_image_get_height(img);

		// Compute aux images dimension
		u32 auxWidth = imgWidth / 4u;
		u32 auxHeight = imgHeight / 4u;

		// Reduce the range
		SV_ASSERT(f32(imgWidth) / f32(imgHeight) == f32(auxWidth) / f32(auxHeight));
		range /= f32(imgWidth) / f32(auxWidth);
		// TODO: Should modify the samples??

		GPUImage* aux0;
		size_t aux0Index;

		GPUImage* aux1;
		size_t aux1Index;

		// Get aux images
		{
			u64 id = auximg_id(auxWidth, auxHeight, GBuffer::FORMAT_OFFSCREEN, GPUImageType_ShaderResource | GPUImageType_RenderTarget);

			auto res = auximg_push(id, GPUImageLayout_RenderTarget, cmd);

			aux0 = res.first;
			aux0Index = res.second;

			res = auximg_push(id, GPUImageLayout_RenderTarget, cmd);

			aux1 = res.first;
			aux1Index = res.second;

			if (aux0 == nullptr || aux1 == nullptr) {
				SV_LOG_ERROR("Can't do a bloom effect without auxiliar images");
				return;
			}
		}

		// Set img and emission as ShaderResource
		{
			u32 barrierCount = 0u;
			GPUBarrier barrier[2u];

			if (imgOldLayout != GPUImageLayout_ShaderResource) {
				barrier[barrierCount++] = GPUBarrier::Image(img, imgOldLayout, GPUImageLayout_ShaderResource);
			}

			if (emission && emissionOldLayout != GPUImageLayout_ShaderResource) {
				barrier[barrierCount++] = GPUBarrier::Image(emission, emissionOldLayout, GPUImageLayout_ShaderResource);
			}
			
			graphics_barrier(barrier, barrierCount, cmd);
		}

		graphics_state_unbind(cmd);

		graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);
		graphics_shader_bind(g_VS_DefPostProcessing, cmd);

		GPUImage* att[1u];

		// THRESHOLD PASS
		{
			Color4f attColors[] = {
				{ 0.f, 0.f, 0.f, 1.f }
			};

			att[0] = aux0;

			graphics_viewport_set(aux0, 0u, cmd);
			graphics_scissor_set(aux0, 0u, cmd);

			graphics_shader_bind(g_PS_Threshold, cmd);
			graphics_blendstate_bind(g_BS_Threshold, cmd);

			graphics_constantbuffer_bind(g_Buffer_Threshold, 0u, ShaderType_Pixel, cmd);
			graphics_image_bind(img, 0u, ShaderType_Pixel, cmd);
			graphics_sampler_bind(g_Sampler_Threshold, 0u, ShaderType_Pixel, cmd);

			BloomThresholdData data;
			data.threshold = threshold;

			graphics_buffer_update(g_Buffer_Threshold, &data, sizeof(f32), 0u, cmd);

			graphics_renderpass_begin(g_RenderPass_Threshold, att, attColors, 0.f, 0u, cmd);
			graphics_draw(3u, 1u, 0u, 0u, cmd);
			graphics_renderpass_end(cmd);
		}

		// EMISSION PASS 
		if (emission) {

			graphics_shader_bind(g_PS_DefPostProcessing, cmd);
			graphics_blendstate_bind(g_BS_Emissive, cmd);

			graphics_image_bind(emission, 0u, ShaderType_Pixel, cmd);
			graphics_sampler_bind(g_Sampler_Emissive, 0u, ShaderType_Pixel, cmd);

			graphics_renderpass_begin(g_RenderPass_Emissive, att, nullptr, 0.f, 0u, cmd);
			graphics_draw(3u, 1u, 0u, 0u, cmd);
			graphics_renderpass_end(cmd);
		}

		// BLUR PASS
		{
			GPUBarrier barriers[2u];

			graphics_shader_bind(g_PS_Blur, cmd);
			graphics_blendstate_bind(g_BS_Blur, cmd);

			graphics_constantbuffer_bind(g_Buffer_Blur, 0u, ShaderType_Pixel, cmd);
			graphics_image_bind(aux0, 0u, ShaderType_Pixel, cmd);
			graphics_sampler_bind(g_Sampler_Blur, 0u, ShaderType_Pixel, cmd);

			BloomBlurData data;

			data.range = (range / f32(iterations)) / f32(auxHeight);
			graphics_buffer_update(g_Buffer_Blur, &data, sizeof(f32), 0u, cmd);

			// Vertical Blur
			foreach(i, iterations)
			{
				if (i == 0u) {

					barriers[0] = GPUBarrier::Image(aux0, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
					// Prepare to the addPass
					barriers[1] = GPUBarrier::Image(img, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);

					graphics_barrier(barriers, 2u, cmd);

					att[0] = aux1;
					graphics_image_bind(aux0, 0u, ShaderType_Pixel, cmd);
				}
				else if (i % 2 == 1u) {
					barriers[0] = GPUBarrier::Image(aux1, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
					barriers[1] = GPUBarrier::Image(aux0, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
					graphics_barrier(barriers, 2u, cmd);

					att[0] = aux0;
					graphics_image_bind(aux1, 0u, ShaderType_Pixel, cmd);
				}
				else {
					barriers[0] = GPUBarrier::Image(aux0, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
					barriers[1] = GPUBarrier::Image(aux1, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
					graphics_barrier(barriers, 2u, cmd);

					att[0] = aux1;
					graphics_image_bind(aux0, 0u, ShaderType_Pixel, cmd);
				}

				graphics_renderpass_begin(g_RenderPass_Blur, att, nullptr, 0.f, 0u, cmd);
				graphics_draw(3u, 1u, 0u, 0u, cmd);
				graphics_renderpass_end(cmd);
			}

			data.range = -(range / f32(iterations)) / f32(auxWidth);
			graphics_buffer_update(g_Buffer_Blur, &data, sizeof(f32), 0u, cmd);

			// Horizontal Blur
			foreach(i, iterations)
			{
				if (att[0] == aux0) {
					barriers[0] = GPUBarrier::Image(aux0, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
					barriers[1] = GPUBarrier::Image(aux1, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
					graphics_barrier(barriers, (iterations - 1u == i) ? 1u : 2u, cmd);

					att[0] = aux1;
					graphics_image_bind(aux0, 0u, ShaderType_Pixel, cmd);
				}
				else {
					barriers[0] = GPUBarrier::Image(aux1, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
					barriers[1] = GPUBarrier::Image(aux0, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
					graphics_barrier(barriers, (iterations - 1u == i) ? 1u : 2u, cmd);

					att[0] = aux0;
					graphics_image_bind(aux1, 0u, ShaderType_Pixel, cmd);
				}

				if (iterations - 1u == i) {
					att[0] = img;
					graphics_viewport_set(img, 0u, cmd);
					graphics_scissor_set(img, 0u, cmd);
					graphics_blendstate_bind(g_BS_AddBloom, cmd);
					graphics_renderpass_begin(g_RenderPass_AddBloom, att, nullptr, 0.f, 0u, cmd);
				}
				else graphics_renderpass_begin(g_RenderPass_Blur, att, nullptr, 0.f, 0u, cmd);

				graphics_draw(3u, 1u, 0u, 0u, cmd);
				graphics_renderpass_end(cmd);
			}
		}

		{
			u32 barrierCount = 0u;
			GPUBarrier barrier[2u];

			if (imgNewLayout != GPUImageLayout_ShaderResource) {
				barrier[barrierCount++] = GPUBarrier::Image(img, GPUImageLayout_RenderTarget, imgNewLayout);
			}

			if (emission && emissionNewLayout != GPUImageLayout_ShaderResource) {
				barrier[barrierCount++] = GPUBarrier::Image(emission, GPUImageLayout_ShaderResource, emissionNewLayout);
			}

			graphics_barrier(barrier, barrierCount, cmd);
		}
		
		auximg_pop(aux0Index, GPUImageLayout_ShaderResource, cmd);
		auximg_pop(aux1Index, GPUImageLayout_ShaderResource, cmd);
	}
}