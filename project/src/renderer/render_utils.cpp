#include "SilverEngine/core.h"

#include "renderer/renderer_internal.h"

namespace sv {

	// RESOURCES

	Result offscreen_create(u32 width, u32 height, GPUImage** pImage)
	{
		GPUImageDesc desc;
		desc.format = OFFSCREEN_FORMAT;
		desc.layout = GPUImageLayout_RenderTarget;
		desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
		desc.width = width;
		desc.height = height;

		return graphics_image_create(&desc, pImage);
	}

	Result zbuffer_create(u32 width, u32 height, GPUImage** pImage)
	{
		GPUImageDesc desc;
		desc.width = width;
		desc.height = height;
		desc.format = ZBUFFER_FORMAT;
		desc.layout = GPUImageLayout_DepthStencil;
		desc.type = GPUImageType_DepthStencil;
		return graphics_image_create(&desc, pImage);
	}

	Result camerabuffer_create(CameraBuffer* camera_buffer)
	{
		SV_ASSERT(camera_buffer);

		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Constant;
		desc.usage = ResourceUsage_Default;
		desc.CPUAccess = CPUAccess_Write;
		desc.size = sizeof(CameraBuffer_GPU);

		return graphics_buffer_create(&desc, &camera_buffer->buffer);
	}

	void camerabuffer_update(CameraBuffer* camera_buffer, CommandList cmd)
	{
		CameraBuffer_GPU data;
		data.view_matrix			= XMMatrixTranspose(camera_buffer->view_matrix);
		data.projection_matrix		= XMMatrixTranspose(camera_buffer->projection_matrix);
		data.view_projection_matrix = XMMatrixTranspose(camera_buffer->view_matrix * camera_buffer->projection_matrix);
		data.position				= camera_buffer->position.getVec4(0.f);
		data.rotation				= camera_buffer->rotation;

		graphics_buffer_update(camera_buffer->buffer, &data, sizeof(CameraBuffer_GPU), 0u, cmd);
	}

	// CAMERA PROJECTION

	void projection_adjust(CameraProjection& projection, f32 aspect)
	{
		if (projection.width / projection.height == aspect) return;
		v2_f32 res = { projection.width, projection.height };
		f32 mag = res.length();
		res.x = aspect;
		res.y = 1.f;
		res.normalize();
		res *= mag;
		projection.width = res.x;
		projection.height = res.y;
	}

	f32 projection_length_get(CameraProjection& projection)
	{
		return math_sqrt(projection.width * projection.width + projection.height * projection.height);
	}

	void projection_length_set(CameraProjection& projection, f32 length)
	{
		f32 currentLength = projection_length_get(projection);
		if (currentLength == length) return;
		projection.width = projection.width / currentLength * length;
		projection.height = projection.height / currentLength * length;
	}

	void projection_update_matrix(CameraProjection& projection)
	{
#ifndef SV_DIST
		if (projection.near >= projection.far) {
			SV_LOG_WARNING("Computing the projection matrix. The far must be grater than near");
		}

		switch (projection.projection_type)
		{
		case ProjectionType_Orthographic:
			break;

		case ProjectionType_Perspective:
			if (projection.near <= 0.f) {
				SV_LOG_WARNING("In perspective projection, near must be greater to 0");
			}
			break;
		}
#endif

		switch (projection.projection_type)
		{
		case ProjectionType_Orthographic:
			projection.projection_matrix = XMMatrixOrthographicLH(projection.width, projection.height, projection.near, projection.far);
			break;

		case ProjectionType_Perspective:
			projection.projection_matrix = XMMatrixPerspectiveLH(projection.width, projection.height, projection.near, projection.far);
			break;

		default:
			projection.projection_matrix = XMMatrixIdentity();
			break;
		}
	}

}