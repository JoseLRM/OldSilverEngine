#include "core.h"

#define VMA_IMPLEMENTATION
#define SV_VULKAN_IMPLEMENTATION
#include "graphics_vulkan.h"
#include "core/engine.h"

namespace sv {

	void MemoryManager::Create(size_t size)
	{
		m_BufferSize = size;
	}
	void MemoryManager::GetMappingData(u32 size, VkBuffer& buffer, void** data, u32& offset)
	{
		SV_ASSERT(size <= m_BufferSize);

		u32 currentFrame =  graphics_vulkan_device_get().currentFrame;

		// Reset
		if (m_LastFrame != sv::engine_frame_count()) {
			Reset(currentFrame);
			m_LastFrame = sv::engine_frame_count();
		}

		// Get new staging buffer
		if (m_BufferSize - m_CurrentStagingBufferOffset < size) {
			m_ActiveStaggingBuffers.push_back(m_CurrentStagingBuffer);
			m_CurrentStagingBufferOffset = 0u;
			m_CurrentStagingBuffer = {};
		}

		// Create staging buffer
		if (m_CurrentStagingBuffer.buffer == VK_NULL_HANDLE) {
			
			bool find = false;
			m_CurrentStagingBufferOffset = 0u;

			if (!m_StaggingBuffers.empty()) {
				
				for (u32 i = 0; i < m_StaggingBuffers.size(); ++i) {
					if (m_StaggingBuffers[i].frame == currentFrame) {
						m_CurrentStagingBuffer = m_StaggingBuffers[i];
						m_StaggingBuffers.erase(m_StaggingBuffers.begin() +i);
						find = true;
						break;
					}
				}

			}

			if (!find) {
				m_CurrentStagingBuffer = CreateStagingBuffer(currentFrame);
			}
		}

		// Set result
		offset = m_CurrentStagingBufferOffset;
		buffer = m_CurrentStagingBuffer.buffer;
		*data = (u8*)(m_CurrentStagingBuffer.mapData) + m_CurrentStagingBufferOffset;
		m_CurrentStagingBufferOffset += size;
	}

	void MemoryManager::Reset(u32 frame)
	{
		if (m_CurrentStagingBuffer.buffer != VK_NULL_HANDLE) {
			m_ActiveStaggingBuffers.push_back(m_CurrentStagingBuffer);
			m_CurrentStagingBuffer = {};
			m_CurrentStagingBufferOffset = 0u;
		}

		if (!m_ActiveStaggingBuffers.empty()) {
			for (i64 i = m_ActiveStaggingBuffers.size() - 1u; i >= 0; --i) {
				if (m_ActiveStaggingBuffers[i].frame == frame) {
					m_StaggingBuffers.emplace_back(m_ActiveStaggingBuffers[i]);
					m_ActiveStaggingBuffers.erase(m_ActiveStaggingBuffers.begin() + i);
				}
			}
		}
	}

	void MemoryManager::Clear()
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();
		
		if (m_CurrentStagingBuffer.buffer != VK_NULL_HANDLE) {
			m_StaggingBuffers.push_back(m_CurrentStagingBuffer);
			m_CurrentStagingBuffer = {};
		}
		for (u32 i = 0; i < m_StaggingBuffers.size(); ++i) {
			const StagingBuffer& b = m_StaggingBuffers[i];
			vmaDestroyBuffer(gfx.allocator, b.buffer, b.allocation);
		}
		for (u32 i = 0; i < m_ActiveStaggingBuffers.size(); ++i) {
			const StagingBuffer& b = m_ActiveStaggingBuffers[i];
			vmaDestroyBuffer(gfx.allocator, b.buffer, b.allocation);
		}
		m_StaggingBuffers.clear();
		m_ActiveStaggingBuffers.clear();
	}

	MemoryManager::StagingBuffer MemoryManager::CreateStagingBuffer(u32 currentFrame)
	{
		StagingBuffer res;
		vkAssert(graphics_vulkan_memory_create_stagingbuffer(res.buffer, res.allocation, &res.mapData, m_BufferSize));
		res.frame = currentFrame;
		return res;
	}

	VkResult graphics_vulkan_memory_create_stagingbuffer(VkBuffer& buffer, VmaAllocation& allocation, void** mapData, VkDeviceSize size)
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_info.queueFamilyIndexCount = 0u;
		buffer_info.pQueueFamilyIndices = nullptr;

		VmaAllocationCreateInfo alloc_info{};
		alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		vkExt(vmaCreateBuffer(gfx.allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr));

		*mapData = allocation->GetMappedData();

		return VK_SUCCESS;
	}

	constexpr u32 graphics_vulkan_descriptors_indextype(VkDescriptorType type)
	{
		switch (type)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
			return 0u;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return 1u;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return 2u;
		default:
			return 0u;
		}
	}

	VkDescriptorSet graphics_vulkan_descriptors_allocate_sets(DescriptorPool& descPool, const ShaderDescriptorSetLayout& layout)
	{
		// Try to use allocated set
		auto it = descPool.sets.find(layout.setLayout);
		if (it != descPool.sets.end()) {
			VulkanDescriptorSet& sets = it->second;
			if (sets.used < sets.sets.size()) {
				return sets.sets[sets.used++];
			}
		}

		if (it == descPool.sets.end()) descPool.sets[layout.setLayout] = {};
		VulkanDescriptorSet& sets = descPool.sets[layout.setLayout];
		VulkanDescriptorPool* pool = nullptr;
		Graphics_vk& gfx = graphics_vulkan_device_get();

		u32 samplerIndex = graphics_vulkan_descriptors_indextype(VK_DESCRIPTOR_TYPE_SAMPLER);
		u32 imageIndex = graphics_vulkan_descriptors_indextype(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
		u32 uniformIndex = graphics_vulkan_descriptors_indextype(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		// Try to find existing pool
		if (!descPool.pools.empty()) {
			for (auto it = descPool.pools.begin(); it != descPool.pools.end(); ++it) {
				if (it->sets + VULKAN_DESCRIPTOR_ALLOC_COUNT >= VULKAN_MAX_DESCRIPTOR_SETS &&
					it->count[samplerIndex] >= layout.count[samplerIndex] * VULKAN_DESCRIPTOR_ALLOC_COUNT &&
					it->count[imageIndex] >= layout.count[imageIndex] * VULKAN_DESCRIPTOR_ALLOC_COUNT &&
					it->count[uniformIndex] >= layout.count[uniformIndex] * VULKAN_DESCRIPTOR_ALLOC_COUNT) {
					pool = it._Ptr;
					break;
				}
			}
		}

		// Create new pool if necessary
		if (pool == nullptr) {

			pool = &descPool.pools.emplace_back();

			VkDescriptorPoolSize sizes[3];

			sizes[0].descriptorCount = VULKAN_MAX_DESCRIPTOR_TYPES * VULKAN_MAX_DESCRIPTOR_SETS;
			sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
			
			sizes[1].descriptorCount = VULKAN_MAX_DESCRIPTOR_TYPES * VULKAN_MAX_DESCRIPTOR_SETS;
			sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

			sizes[2].descriptorCount = VULKAN_MAX_DESCRIPTOR_TYPES * VULKAN_MAX_DESCRIPTOR_SETS;
			sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VkDescriptorPoolCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			create_info.maxSets = VULKAN_MAX_DESCRIPTOR_SETS;
			create_info.poolSizeCount = 3u;
			create_info.pPoolSizes = sizes;

			vkAssert(vkCreateDescriptorPool(gfx.device, &create_info, nullptr, &pool->pool));

			pool->sets = VULKAN_MAX_DESCRIPTOR_SETS;
			for (u32 i = 0; i < 3; ++i)	
				pool->count[i] = VULKAN_MAX_DESCRIPTOR_TYPES;
		}

		// Allocate sets
		{
			pool->count[samplerIndex] -= layout.count[samplerIndex] * VULKAN_DESCRIPTOR_ALLOC_COUNT;
			pool->count[imageIndex] -= layout.count[imageIndex] * VULKAN_DESCRIPTOR_ALLOC_COUNT;
			pool->count[uniformIndex] -= layout.count[uniformIndex] * VULKAN_DESCRIPTOR_ALLOC_COUNT;

			size_t index = sets.sets.size();
			sets.sets.resize(index + VULKAN_DESCRIPTOR_ALLOC_COUNT);

			VkDescriptorSetLayout setLayouts[VULKAN_DESCRIPTOR_ALLOC_COUNT];
			for (u32 i = 0; i < VULKAN_DESCRIPTOR_ALLOC_COUNT; ++i)
				setLayouts[i] = layout.setLayout;

			VkDescriptorSetAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = pool->pool;
			alloc_info.descriptorSetCount = VULKAN_DESCRIPTOR_ALLOC_COUNT;
			alloc_info.pSetLayouts = setLayouts;

			vkAssert(vkAllocateDescriptorSets(gfx.device, &alloc_info, sets.sets.data() + index));

			return sets.sets[sets.used++];
		}
	}

	void graphics_vulkan_descriptors_reset(DescriptorPool& descPool)
	{
		for (auto& set : descPool.sets) {
			set.second.used = 0u;
		}
	}

	void graphics_vulkan_descriptors_clear(DescriptorPool& descPool)
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		for (auto it = descPool.pools.begin(); it != descPool.pools.end(); ++it) {
			vkDestroyDescriptorPool(gfx.device, it->pool, nullptr);
		}

		descPool.sets.clear();
		descPool.pools.clear();
	}

}