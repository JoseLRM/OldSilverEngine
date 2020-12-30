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
			XMMATRIX inverseViewMatrix;
			XMMATRIX inverseProjectionMatrix;
			XMMATRIX inverseViewProjectionMatrix;
			vec3f position;
			float padding;
			vec4f direction;
		} data;

		// Fill gpu data
		data.viewMatrix = XMMatrixTranspose(viewMatrix);
		data.inverseViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, viewMatrix));
		data.projectionMatrix = XMMatrixTranspose(projectionMatrix);
		data.inverseProjectionMatrix= XMMatrixTranspose(XMMatrixInverse(nullptr, projectionMatrix));
		data.viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
		data.inverseViewProjectionMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, data.viewProjectionMatrix));
		data.viewProjectionMatrix = XMMatrixTranspose(data.viewProjectionMatrix);
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