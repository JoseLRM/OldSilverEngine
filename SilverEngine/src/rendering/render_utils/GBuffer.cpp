#include "core.h"

#include "rendering/render_utils.h"

namespace sv {

	GBuffer::~GBuffer()
	{
		destroy();
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

		// Diffuse
		desc.format = FORMAT_DIFFUSE;
		svCheck(graphics_image_create(&desc, &diffuse));

		// Normal
		desc.format = FORMAT_NORMAL;
		svCheck(graphics_image_create(&desc, &normal));

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
		svCheck(graphics_destroy(diffuse));
		svCheck(graphics_destroy(normal));
		svCheck(graphics_destroy(depthStencil));

		return Result_Success;
	}

}