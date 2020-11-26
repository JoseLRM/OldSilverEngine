#include "core.h"

#include "material_system_internal.h"

namespace sv {

	CameraBuffer::~CameraBuffer()
	{
		clear();
	}

	Result CameraBuffer::updateGPU(CommandList cmd)
	{
		// Define GPU Data structure with padding
		struct BufferData {
			XMMATRIX viewMatrix;
			XMMATRIX projectionMatrix;
			XMMATRIX viewProjectionMatrix;
			vec3f position;
			float padding;
			vec4f direction;
		} data;

		// Fill gpu data
		data.viewMatrix = viewMatrix;
		data.projectionMatrix = projectionMatrix;
		data.viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
		data.position = position;
		data.direction = direction;

		// Create GPU Buffer
		if (gpuBuffer == nullptr) {

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(BufferData);
			desc.pData = &data;

			svCheck(graphics_buffer_create(&desc, &gpuBuffer));

		}
		// Update GPU Buffer
		else graphics_buffer_update(gpuBuffer, &data, sizeof(BufferData), 0u, cmd);

		return Result_Success;
	}

	Result CameraBuffer::clear()
	{
		if (gpuBuffer) {
			svCheck(graphics_destroy(gpuBuffer));
			gpuBuffer = nullptr;
		}
		return Result_Success;
	}

}