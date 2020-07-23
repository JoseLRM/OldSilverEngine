#pragma once

#include "graphics/Graphics.h"
#include "Vulkan.h"
#include "Conversions_vk.h"

namespace SV {

	class Graphics_vk;

	//////////////////////////////////////////////////////////// ADAPTER ////////////////////////////////////////////////////////////
	class Adapter_vk : public Adapter {
		VkPhysicalDevice					m_PhysicalDevice	= VK_NULL_HANDLE;
		VkPhysicalDeviceProperties			m_Properties;
		VkPhysicalDeviceFeatures			m_Features;
		VkPhysicalDeviceMemoryProperties	m_MemoryProps;

		struct {
			ui32 graphics	= UINT32_MAX;

			inline bool IsComplete() const noexcept { return graphics != UINT32_MAX; }

		} m_FamilyIndex;

	public:
		Adapter_vk() {}
		Adapter_vk(VkPhysicalDevice device);

		inline VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_PhysicalDevice; }
		inline const VkPhysicalDeviceProperties& GetProperties() const noexcept { return m_Properties; }
		inline const VkPhysicalDeviceFeatures& GetFeatures() const noexcept { return m_Features; }
		inline const VkPhysicalDeviceMemoryProperties GetMemoryProperties() const noexcept { return m_MemoryProps; }

		inline auto GetFamilyIndex() const noexcept { return m_FamilyIndex; }

	};

	//////////////////////////////////////////////////////////// PRIMITIVES ////////////////////////////////////////////////////////////

	// Buffer
	struct Buffer_vk : public _internal::Buffer_internal {
		VkBuffer				buffer;
		VmaAllocation			allocation;
		VkDescriptorBufferInfo	buffer_info;
		VkBuffer				stagingBuffer;
		VmaAllocation			stagingAllocation;
	};
	// Image
	struct Image_vk : public _internal::Image_internal {
		VmaAllocation			allocation;
		VkImage					image				= VK_NULL_HANDLE;
		VkImageView				renderTargetView	= VK_NULL_HANDLE;
		VkImageView				depthStencilView	= VK_NULL_HANDLE;
		VkImageView				shaderResouceView	= VK_NULL_HANDLE;
		ui64					ID;
		VkDescriptorImageInfo	image_info;
	};
	// Sampler
	struct Sampler_vk : public _internal::Sampler_internal {
		VkSampler				sampler		= VK_NULL_HANDLE;
		VkDescriptorImageInfo	image_info;
	};
	// Shader
	struct Shader_vk : public _internal::Shader_internal {
		VkShaderModule module = VK_NULL_HANDLE;
		std::vector<ui8> sprvCode;
	};
	// RenderPass
	struct RenderPass_vk : public _internal::RenderPass_internal {
		VkRenderPass					renderPass;
		std::map<size_t, VkFramebuffer> frameBuffers;
	};
	// GraphicsPipeline
	struct GraphicsPipeline_vk : public _internal::GraphicsPipeline_internal {
		std::map<std::string, ui32>						semanticNames;
		std::vector<ui32>								bindingsLocation;
		std::map<size_t, VkPipeline>					pipelines;
		std::mutex										mutex;
		VkDescriptorPool								descriptorPool;
		std::vector<VkDescriptorSet>					descriptorSets;
		std::vector<VkWriteDescriptorSet>				writeDescriptors;
		std::vector<VkDescriptorSetLayoutBinding>		setLayoutBindings;
		VkDescriptorSetLayout							setLayout			= VK_NULL_HANDLE;
		VkPipelineLayout								layout				= VK_NULL_HANDLE;
		size_t											ID					= 0u;
	};

	//////////////////////////////////////////////////////////// CONSTRUCTOR & DESTRUCTOR ////////////////////////////////////////////////////////////

	void* VulkanConstructor(SV_GFX_PRIMITIVE type, const void* desc);
	bool VulkanDestructor(Primitive& primitive);

	//////////////////////////////////////////////////////////// GRAPHICS API ////////////////////////////////////////////////////////////
	struct Frame {
		VkCommandPool		commandPool;
		VkCommandBuffer		commandBuffers[SV_GFX_COMMAND_LIST_COUNT];
		VkCommandPool		transientCommandPool;
		VkFence				fence;
	};

	struct SwapChain {

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

		struct Image {
			VkImage image;
			VkImageView view;
			ui64 ID;
		};
		std::vector<Image>					images;
		SV::Image_vk						backBuffer;
		SV::Primitive						backBufferImage;
	};

	class Graphics_vk : public Graphics::_internal::GraphicsDevice {

		VkInstance					m_Instance								= VK_NULL_HANDLE;
		VkDevice					m_Device								= VK_NULL_HANDLE;
		SwapChain					m_SwapChain;
		VmaAllocator				m_Allocator;

#if SV_GFX_VK_VALIDATION_LAYERS
		VkDebugUtilsMessengerEXT	m_Debug									= VK_NULL_HANDLE;
#endif

		std::vector<const char*>	m_Extensions;
		std::vector<const char*>	m_ValidationLayers;
		std::vector<const char*>	m_DeviceExtensions;
		std::vector<const char*>	m_DeviceValidationLayers;

		VkQueue						m_QueueGraphics							= VK_NULL_HANDLE;

		// Frame Members

		std::vector<Frame>		m_Frames;
		std::vector<VkFence>	m_ImageFences;
		ui32					m_FrameCount;
		ui32					m_CurrentFrame = 0u;
		std::mutex				m_MutexCMD;
		ui32					m_ActiveCMDCount = 0u;

		ui32					m_ImageIndex;

		inline Frame& GetFrame() noexcept { return m_Frames[m_CurrentFrame]; }
		inline VkCommandBuffer GetCMD(CommandList cmd) { return GetFrame().commandBuffers[cmd]; }

		// Binding Members

		bool m_ActiveRenderPass[SV_GFX_COMMAND_LIST_COUNT] = {};

	public:
		bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) override;
		bool Close() override;

	private:
		// Initialization Methods

#if SV_GFX_VK_VALIDATION_LAYERS
		bool CreateDebugMessenger();
#endif

		void AdjustAllocator();
		void SetProperties();
		bool CreateInstance();
		bool CreateAdapters();
		bool CreateLogicalDevice();
		bool CreateAllocator();
		bool CreateFrames();
		bool CreateSwapChain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

		bool DestroySwapChain(bool resizing = true);

		// State Methods
		
		void UpdateGraphicsState(SV::CommandList cmd);

	public:
		// Getters
		inline VkInstance GetInstance() const noexcept { return m_Instance; }
		inline VkDevice GetDevice() const noexcept { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const noexcept;

		// Primitive Creation
		bool CreateBuffer			(Buffer_vk& buffer,						const SV_GFX_BUFFER_DESC& desc);
		bool CreateImage			(Image_vk& image,						const SV_GFX_IMAGE_DESC& desc);
		bool CreateSampler			(Sampler_vk& sampler,					const SV_GFX_SAMPLER_DESC& desc);
		bool CreateShader			(Shader_vk& shader,						const SV_GFX_SHADER_DESC& desc);
		bool CreateRenderPass		(RenderPass_vk& renderPass,				const SV_GFX_RENDERPASS_DESC& desc);
		bool CreateGraphicsPipeline	(GraphicsPipeline_vk& graphicsPipeline, const SV_GFX_GRAPHICS_PIPELINE_DESC& desc);

		ui64 m_IDCount = 0u;
		std::mutex m_IDMutex;
		inline ui64 GetID() noexcept { std::lock_guard<std::mutex> lock(m_IDMutex); return m_IDCount++; }

		// Primitive Destuction
		bool DestroyBuffer				(Buffer_vk& buffer);
		bool DestroyImage				(Image_vk& image);
		bool DestroySampler				(Sampler_vk& sampler);
		bool DestroyShader				(Shader_vk& shader);
		bool DestroyRenderPass			(RenderPass_vk& renderPass);
		bool DestroyGraphicsPipeline	(GraphicsPipeline_vk& graphicsPipeline);

		// Device Methods

		CommandList BeginCommandList() override;

		void ResizeSwapChain() override;
		SV::Image& GetSwapChainBackBuffer() override;

		void BeginFrame() override;
		void SubmitCommandLists() override;
		void Present() override;

		void Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, SV::CommandList cmd) override;
		void DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, SV::CommandList cmd) override;

		void UpdateBuffer(SV::Buffer& buffer, void* pData, ui32 size, ui32 offset, SV::CommandList cmd) override;

		// CommandBuffers

		VkResult BeginSingleTimeCMD(VkCommandBuffer* pCmd);
		VkResult EndSingleTimeCMD(VkCommandBuffer cmd);

		// Memory Funtions

		VmaMemoryUsage GetMemoryUsage(SV_GFX_USAGE usage, SV::CPUAccessFlags cpuAccess);

		void CopyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
		VkResult MapMemoryUsingStagingBuffer(VkCommandBuffer cmd, VkImage image, VmaAllocation memory, void* data, VkDeviceSize size, ui32 width, ui32 height);

		VkResult CreateStagingBuffer(VkBuffer& buffer, VmaAllocation& memory, void* data, VkDeviceSize size);
		VkResult CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, ui32 layerCount, VkImageView& view);
		VkResult ImageMemoryBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspect, ui32 layers);

		// Sync Functions

		VkSemaphore CreateSemaphore();
		VkFence CreateFence(bool signaled);

		// Shader Layout

		void LoadSpirv_SemanticNames(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, std::map<std::string, ui32>& semanticNames);
		void LoadSpirv_Samplers(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, SV_GFX_SHADER_TYPE shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings);
		void LoadSpirv_Images(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, SV_GFX_SHADER_TYPE shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings);
		void LoadSpirv_Uniforms(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, SV_GFX_SHADER_TYPE shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings);

	};

}