#pragma once

#include "graphics.h"
#include "vulkan_impl.h"

namespace sv {

	class MemoryManager {

		struct StagingBuffer {
			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;
			void* mapData = nullptr;
			ui32 frame = 0u;
		};

		std::vector<StagingBuffer> m_StaggingBuffers;
		std::vector<StagingBuffer> m_ActiveStaggingBuffers;
		StagingBuffer m_CurrentStagingBuffer;
		ui32 m_CurrentStagingBufferOffset = 0u;
		ui64 m_LastFrame = UINT64_MAX;
		size_t m_BufferSize;

	public:
		void Create(size_t size);
		void GetMappingData(ui32 size, VkBuffer& buffer, void** data, ui32& offset);
		void Clear();

	private:
		void Reset(ui32 frame);
		StagingBuffer CreateStagingBuffer(ui32 currentFrame);

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