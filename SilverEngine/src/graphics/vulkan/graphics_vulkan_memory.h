#pragma once

#include "graphics/graphics.h"
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

	class DescriptorsManager {

		struct Frame {
			std::vector<VkDescriptorSet> descriptors;
			ui32 activeDescriptors;
		};

		struct DescCMD {
			std::vector<Frame> frames;
			std::vector<VkDescriptorPool> pools;
		};

		DescCMD m_Descriptors[SV_GFX_COMMAND_LIST_COUNT];
		VkDescriptorSetLayout m_SetLayout;

		ui32 m_ImagesCount;
		ui32 m_SamplersCount;
		ui32 m_UniformsCount;


	public:
		void Create(VkDescriptorSetLayout setLayout, ui32 frameCount, ui32 imagesCount, ui32 samplersCount, ui32 uniformsCount);
		VkDescriptorSet GetDescriptorSet(ui32 currentFrame, sv::CommandList cmd);
		void Reset(ui32 frame);
		void Clear();

	};

	VkResult graphics_vulkan_memory_create_stagingbuffer(VkBuffer& buffer, VmaAllocation& allocation, void** mapData, VkDeviceSize size);

}