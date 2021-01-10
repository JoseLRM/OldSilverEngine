#include "core.h"

#include "core/rendering/render_utils.h"

namespace sv {

	GBuffer::~GBuffer()
	{
		destroy();
	}

	Viewport GBuffer::getViewport() const noexcept
	{
		v2_u32 res = getResolution();
		return { 0.f, 0.f, f32(res.x), f32(res.y), 0.f, 1.f };
	}

	Scissor GBuffer::getScissor() const noexcept
	{
		v2_u32 res = getResolution();
		return { 0u, 0u, res.x, res.y };
	}

	Result GBuffer::create(u32 width, u32 height)
	{
		GPUImageDesc desc;
		desc.pData = nullptr;
		desc.size = 0u;
		desc.layout = GPUImageLayout_RenderTarget;
		desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
		desc.usage = ResourceUsage_Static;
		desc.CPUAccess = CPUAccess_None;
		desc.dimension = 2u;
		desc.width = width;
		desc.height = height;
		desc.depth = 1u;
		desc.layers = 1u;

		// Offscreen
		desc.format = FORMAT_OFFSCREEN;
		svCheck(graphics_image_create(&desc, &offscreen));

		// Diffuse
		desc.format = FORMAT_DIFFUSE;
		svCheck(graphics_image_create(&desc, &diffuse));

		// Normal
		desc.format = FORMAT_NORMAL;
		svCheck(graphics_image_create(&desc, &normal));

		// Emissive
		desc.format = FORMAT_EMISSIVE;
		svCheck(graphics_image_create(&desc, &emissive));

		// Depth Stencil
		desc.format = FORMAT_DEPTHSTENCIL;
		desc.layout = GPUImageLayout_DepthStencil;
		desc.type = GPUImageType_DepthStencil | GPUImageType_ShaderResource;
		svCheck(graphics_image_create(&desc, &depthStencil));

		return Result_Success;
	}
	
	Result GBuffer::resize(u32 width, u32 height)
	{
		SV_LOG_ERROR("TODO: Resize GBuffer");
		return Result_TODO;
	}

	Result GBuffer::destroy()
	{
		svCheck(graphics_destroy(offscreen));
		svCheck(graphics_destroy(diffuse));
		svCheck(graphics_destroy(normal));
		svCheck(graphics_destroy(emissive));
		svCheck(graphics_destroy(depthStencil));

		return Result_Success;
	}

}