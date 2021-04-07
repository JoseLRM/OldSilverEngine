#ifndef _GRAPHICS_VULKAN
#define _GRAPHICS_VULKAN

#include "..//graphics_internal.h"

namespace sv {

    // DEVICE FUNCTIONS

    void graphics_vulkan_device_prepare(GraphicsDevice& device);

    Result	graphics_vulkan_initialize();
    Result	graphics_vulkan_close();
    void* graphics_vulkan_get();

    Result graphics_vulkan_create(GraphicsPrimitiveType type, const void* desc, Primitive_internal* res);
    Result graphics_vulkan_destroy(Primitive_internal* primitive);

    CommandList graphics_vulkan_commandlist_begin();
    CommandList graphics_vulkan_commandlist_last();
    u32		graphics_vulkan_commandlist_count();

    void graphics_vulkan_renderpass_begin(CommandList);
    void graphics_vulkan_renderpass_end(CommandList);

    void graphics_vulkan_swapchain_resize();

    void graphics_vulkan_gpu_wait();

    void graphics_vulkan_frame_begin();
    void graphics_vulkan_frame_end();

    void graphics_vulkan_draw(u32, u32, u32, u32, CommandList);
    void graphics_vulkan_draw_indexed(u32, u32, u32, u32, u32, CommandList);

    void graphics_vulkan_image_clear(GPUImage*, GPUImageLayout, GPUImageLayout, Color, float, u32, CommandList);
    void graphics_vulkan_image_blit(GPUImage*, GPUImage*, GPUImageLayout, GPUImageLayout, u32, const GPUImageBlit*, SamplerFilter, CommandList);
    void graphics_vulkan_buffer_update(GPUBuffer*, const void*, u32, u32, CommandList);
    void graphics_vulkan_barrier(const GPUBarrier*, u32, CommandList);

    void graphics_vulkan_event_begin(const char* name, CommandList cmd);
    void graphics_vulkan_event_mark(const char* name, CommandList cmd);
    void graphics_vulkan_event_end(CommandList cmd);

}

#endif

#ifdef SV_VULKAN_IMPLEMENTATION
#ifndef _VULKAN_IMPLEMENTATION
#define _VULKAN_IMPLEMENTATION

/*
  TODO list:
  - Per frame and per commandlist GPU allocator
  - Remove per buffer memory allocators
  - Reset uniform descriptors after updating a dynamic buffer
  - At the beginning of the frame reuse memory and free unused memory if it has

*/

#if SV_PLATFORM_WIN
#define VK_USE_PLATFORM_WIN32_KHR
#elif SV_PLATFORM_LINUX
#define VK_USE_PLATFORM_XCB_KHR
#endif

#pragma warning(push, 0)
#pragma warning(disable : 4701)
#pragma warning(disable : 4703)
#include "external/vk_mem_alloc.h"
#pragma warning(pop)

#if SV_SLOW
#define vkAssert(x) if((x) != VK_SUCCESS) SV_ASSERT(!#x)
#else
#define vkAssert(x) x
#endif

#define vkCheck(x) if((x) != VK_SUCCESS) return sv::Result_UnknownError
#define vkExt(x) do{ VkResult res = (x); if(res != VK_SUCCESS) return res; }while(0)

#undef CreateSemaphore

#include "external/sprv/spirv_reflect.hpp"

#include "graphics_vulkan_conversions.h"

namespace sv {

    struct Graphics_vk;

    Graphics_vk& graphics_vulkan_device_get();

    constexpr u32	VULKAN_MAX_DESCRIPTOR_SETS = 100u;
    constexpr u32	VULKAN_MAX_DESCRIPTOR_TYPES = 32u;
    constexpr u32	VULKAN_DESCRIPTOR_ALLOC_COUNT = 10u;
    constexpr Time	VULKAN_UNUSED_OBJECTS_TIMECHECK = 30.0;
    constexpr Time	VULKAN_UNUSED_OBJECTS_LIFETIME = 10.0;

    // MEMORY

    struct StagingBuffer {
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	void* data = nullptr;
    };

    struct VulkanGPUAllocator {
		
	struct Buffer {
	    StagingBuffer staging_buffer;
	    u8* current = nullptr;
	    u8* end = nullptr;
	};

	List<Buffer> buffers;
    };

    struct DynamicAllocation {
	VkBuffer buffer = VK_NULL_HANDLE;
	void* data = nullptr;
	u32 offset = 0u;

	SV_INLINE bool isValid() const noexcept { return buffer != VK_NULL_HANDLE && data != nullptr; }
    };

    // DESCRIPTORS

    struct VulkanDescriptorPool {
	VkDescriptorPool pool;
	u32 count[3];
	u32 sets;
    };

    struct VulkanDescriptorSet {
	std::vector<VkDescriptorSet>	sets;
	u32							used = 0u;
    };

    struct DescriptorPool {
	std::vector<VulkanDescriptorPool>						pools;
	std::map<VkDescriptorSetLayout, VulkanDescriptorSet>	sets;
    };

    struct ShaderResourceBinding {
	u32				vulkanBinding;
	u32				userBinding;
    };
    struct ShaderDescriptorSetLayout {
	VkDescriptorSetLayout setLayout;
	std::vector<ShaderResourceBinding> bindings;
	u32 count[3];
    };

    // PIPELINE

    struct PipelineDescriptorSetLayout {
	ShaderDescriptorSetLayout layouts[ShaderType_GraphicsCount];
    };

    struct VulkanPipeline {
	VulkanPipeline& operator=(const VulkanPipeline& other)
	    {
		semanticNames = other.semanticNames;
		layout = other.layout;
		pipelines = other.pipelines;
		setLayout = other.setLayout;
		return *this;
	    }

	std::mutex						mutex;
	std::mutex						creationMutex;

	std::map<std::string, u32>		semanticNames;

	VkPipelineLayout				layout = VK_NULL_HANDLE;
	std::map<size_t, VkPipeline>	pipelines;
	PipelineDescriptorSetLayout		setLayout;
	VkDescriptorSet					descriptorSets[ShaderType_GraphicsCount] = {};
	Time							lastUsage;
    };

    struct Shader_vk;

    size_t graphics_vulkan_pipeline_compute_hash(const GraphicsState& state);
    bool graphics_vulkan_pipeline_create(VulkanPipeline& pipeline, Shader_vk* pVertexShader, Shader_vk* pPixelShader, Shader_vk* pGeometryShader);
    bool graphics_vulkan_pipeline_destroy(VulkanPipeline& pipeline);
    VkPipeline graphics_vulkan_pipeline_get(VulkanPipeline& pipeline, GraphicsState& state, size_t hash);

    // PRIMITIVES

    // Buffer
    struct Buffer_vk : public GPUBuffer_internal {
	VkBuffer				buffer;
	VmaAllocation			allocation;
	VkDescriptorBufferInfo	buffer_info;
	DynamicAllocation		dynamic_allocation[GraphicsLimit_CommandList];
    };
    // Image
    struct Image_vk : public GPUImage_internal {
	VmaAllocation			allocation;
	VkImage					image = VK_NULL_HANDLE;
	VkImageView				renderTargetView = VK_NULL_HANDLE;
	VkImageView				depthStencilView = VK_NULL_HANDLE;
	VkImageView				shaderResouceView = VK_NULL_HANDLE;
	u32						layers = 1u;
	u64						ID;
	VkDescriptorImageInfo	image_info;
    };
    // Sampler
    struct Sampler_vk : public Sampler_internal {
	VkSampler				sampler = VK_NULL_HANDLE;
	VkDescriptorImageInfo	image_info;
    };
    // Shader
    struct Shader_vk : public Shader_internal {
	VkShaderModule							module = VK_NULL_HANDLE;
	std::unordered_map<std::string, u32>	semanticNames;
	ShaderDescriptorSetLayout				layout;
	u64									ID;
    };
    // RenderPass
    struct RenderPass_vk : public RenderPass_internal {
	VkRenderPass									renderPass;
	std::vector<std::pair<size_t, VkFramebuffer>>	frameBuffers;
	VkRenderPassBeginInfo							beginInfo;
	std::mutex										mutex;
    };
    // InputLayoutState
    struct InputLayoutState_vk : public InputLayoutState_internal {
	size_t hash;
    };
    // BlendState
    struct BlendState_vk : public BlendState_internal {
	size_t hash;
    };
    // DepthStencilState
    struct DepthStencilState_vk : public DepthStencilState_internal {
	size_t hash;
    };
    // RasterizerState
    struct RasterizerState_vk : public RasterizerState_internal {
	size_t hash;
    };

    // API STRUCTS

    struct Frame {
	VkCommandPool		commandPool;
	VkCommandBuffer		commandBuffers[GraphicsLimit_CommandList];
	VkCommandPool		transientCommandPool;
	VkFence				fence;
	DescriptorPool		descPool[GraphicsLimit_CommandList];
	VulkanGPUAllocator	allocator[GraphicsLimit_CommandList];
    };

    struct SwapChain_vk {
	    
	VkSurfaceKHR						surface = VK_NULL_HANDLE;
	VkSwapchainKHR						swapChain = VK_NULL_HANDLE;

	VkSemaphore							semAcquireImage = VK_NULL_HANDLE;
	VkSemaphore							semPresent = VK_NULL_HANDLE;

	VkSurfaceCapabilitiesKHR			capabilities;
	std::vector<VkPresentModeKHR>		presentModes;
	std::vector<VkSurfaceFormatKHR>		formats;

	VkFormat							currentFormat;
	VkColorSpaceKHR						currentColorSpace;
	VkPresentModeKHR					currentPresentMode;
	VkExtent2D							currentExtent;

	std::vector<VkFence>				imageFences;
	u32									imageIndex;

	struct Image {
	    VkImage image;
	    VkImageView view;
	};
	std::vector<Image> images;
    };

    struct Graphics_vk {

	struct {
			
	    VkPhysicalDevice		     physicalDevice = VK_NULL_HANDLE;
	    VkPhysicalDeviceProperties	     properties;
	    VkPhysicalDeviceFeatures	     features;
	    VkPhysicalDeviceMemoryProperties memoryProps;

	    struct {
		u32 graphics = UINT32_MAX;

		SV_INLINE bool IsComplete() const noexcept { return graphics != u32_max; }

	    } familyIndex;

	} card;

	VkInstance   instance = VK_NULL_HANDLE;
	VkDevice     device = VK_NULL_HANDLE;
	VmaAllocator allocator;

	SwapChain_vk swapchain;

#if SV_GFX
	VkDebugUtilsMessengerEXT	debug = VK_NULL_HANDLE;
#endif

	List<const char*> extensions;
	List<const char*> validationLayers;
	List<const char*> deviceExtensions;
	List<const char*> deviceValidationLayers;

	VkQueue						queueGraphics = VK_NULL_HANDLE;

	Time						lastTime = 0.f;

	// Frame Members

	List<Frame> frames;
	u32	    frameCount;
	u32	    currentFrame = 0u;
	std::mutex  mutexCMD;
	u32	    activeCMDCount = 0u;

	SV_INLINE Frame& GetFrame() noexcept { return frames[currentFrame]; }
	SV_INLINE VkCommandBuffer GetCMD(CommandList cmd) { return frames[currentFrame].commandBuffers[cmd]; }

	// Binding Members

	VkRenderPass                     activeRenderPass[GraphicsLimit_CommandList];
	std::map<size_t, VulkanPipeline> pipelines;
	std::mutex			 pipelinesMutex;

    private:
	u64 IDCount = 0u;
	std::mutex IDMutex;
    public:
	inline u64 GetID() noexcept { std::lock_guard<std::mutex> lock(IDMutex); return IDCount++; }

    };

    // API FUNCTIONS

    VkResult graphics_vulkan_singletimecmb_begin(VkCommandBuffer* pCmd);
    VkResult graphics_vulkan_singletimecmb_end(VkCommandBuffer cmd);

    void graphics_vulkan_buffer_copy(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
    VkResult graphics_vulkan_imageview_create(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, u32 layerCount, VkImageView& view);

    VkSemaphore graphics_vulkan_semaphore_create();
    VkFence graphics_vulkan_fence_create(bool sign);

    void graphics_vulkan_acquire_image();
    void graphics_vulkan_submit_commandbuffers();
    void graphics_vulkan_present();

    Result graphics_vulkan_buffer_create(Buffer_vk& buffer, const GPUBufferDesc& desc);
    Result graphics_vulkan_image_create(Image_vk& image, const GPUImageDesc& desc);
    Result graphics_vulkan_sampler_create(Sampler_vk& sampler, const SamplerDesc& desc);
    Result graphics_vulkan_shader_create(Shader_vk& shader, const ShaderDesc& desc);
    Result graphics_vulkan_renderpass_create(RenderPass_vk& renderPass, const RenderPassDesc& desc);
    Result graphics_vulkan_inputlayoutstate_create(InputLayoutState_vk& inputLayoutState, const InputLayoutStateDesc& desc);
    Result graphics_vulkan_blendstate_create(BlendState_vk& blendState, const BlendStateDesc& desc);
    Result graphics_vulkan_depthstencilstate_create(DepthStencilState_vk& depthStencilState, const DepthStencilStateDesc& desc);
    Result graphics_vulkan_rasterizerstate_create(RasterizerState_vk& rasterizerState, const RasterizerStateDesc& desc);
    Result graphics_vulkan_swapchain_create();

    Result graphics_vulkan_buffer_destroy(Buffer_vk& buffer);
    Result graphics_vulkan_image_destroy(Image_vk& image);
    Result graphics_vulkan_sampler_destroy(Sampler_vk& sampler);
    Result graphics_vulkan_shader_destroy(Shader_vk& shader);
    Result graphics_vulkan_renderpass_destroy(RenderPass_vk& renderPass);
    Result graphics_vulkan_swapchain_destroy( bool resizing);

}

#endif
#endif
