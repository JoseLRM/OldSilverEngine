#pragma once

#include "vulkan_impl.h"

namespace _sv {

	struct StagingBuffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		ui32 size = 0u;
		void* mapData = nullptr;
	};

	class MemoryManager {
		std::vector<StagingBuffer> m_StaggingBuffers;
		std::vector<StagingBuffer> m_ActiveStaggingBuffers;
		StagingBuffer m_CurrentStagingBuffer;
		ui32 m_CurrentStagingBufferOffset = 0u;

	public:
		void GetMappingData(ui32 size, VkBuffer& buffer, void** data, ui32& offset);
		void Reset();
		void Clear();

	private:
		void ResetStagingBuffers();
		StagingBuffer CreateStagingBuffer(ui32 size);

	};

	VkResult graphics_vulkan_memory_create_stagingbuffer(VkBuffer& buffer, VmaAllocation& allocation, void** mapData, VkDeviceSize size);

}