#include "SilverEngine/core.h"

#include "renderer/renderer_internal.h"

namespace sv {

	// RESOURCES

	Result create_offscreen(u32 width, u32 height, GPUImage** pImage)
	{
		GPUImageDesc desc;
		desc.format = OFFSCREEN_FORMAT;
		desc.layout = GPUImageLayout_RenderTarget;
		desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
		desc.width = width;
		desc.height = height;

		return graphics_image_create(&desc, pImage);
	}

	Result create_depthstencil(u32 width, u32 height, GPUImage** pImage)
	{
		GPUImageDesc desc;
		desc.width = width;
		desc.height = height;
		desc.format = ZBUFFER_FORMAT;
		desc.layout = GPUImageLayout_DepthStencil;
		desc.type = GPUImageType_DepthStencil;
		return graphics_image_create(&desc, pImage);
	}

}