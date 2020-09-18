#include "core.h"

#include "renderer/renderer_internal.h"

namespace sv {

	// Common Buffers

	static GPUBuffer		g_VertexBuffer;

	Result renderer_postprocessing_initialize()
	{
		// Create Common Buffers
		{
			vec2f vertices[] = {
				{-1.f, -1.f},
				{ 1.f, -1.f},
				{-1.f,  1.f},
				{ 1.f,  1.f},
			};

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.CPUAccess = CPUAccess_None;
			desc.pData = vertices;
			desc.size = 4 * sizeof(vec2f);
			desc.usage = ResourceUsage_Static;

			svCheck(graphics_buffer_create(&desc, g_VertexBuffer));
		}

		return Result_Success;
	}

	Result renderer_postprocessing_close()
	{
		svCheck(graphics_destroy(g_VertexBuffer));
		return Result_Success;
	}

}