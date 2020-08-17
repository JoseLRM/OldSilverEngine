#pragma once

#include "..//graphics_internal.h"
#include "vulkan_impl.h"
#include "graphics_vulkan_conversions.h"
#include "graphics_vulkan_memory.h"

namespace sv {

	class Graphics_vk;

	Graphics_vk& graphics_vulkan_device_get();

	//////////////////////////////////////////////////////////// ADAPTER ////////////////////////////////////////////////////////////
	class Adapter_vk : public Adapter {
		VkPhysicalDevice					m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties			m_Properties;
		VkPhysicalDeviceFeatures			m_Features;
		VkPhysicalDeviceMemoryProperties	m_MemoryProps;

		struct {
			ui32 graphics = UINT32_MAX;

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
	struct Buffer_vk : public GPUBuffer_internal {
		VkBuffer				buffer;
		VmaAllocation			allocation;
		VkDescriptorBufferInfo	buffer_info;
		MemoryManager			memory;
	};
	// Image
	struct Image_vk : public GPUImage_internal {
		VmaAllocation			allocation;
		VkImage					image = VK_NULL_HANDLE;
		VkImageView				renderTargetView = VK_NULL_HANDLE;
		VkImageView				depthStencilView = VK_NULL_HANDLE;
		VkImageView				shaderResouceView = VK_NULL_HANDLE;
		ui64					ID;
		VkDescriptorImageInfo	image_info;
	};
	// Sampler
	struct Sampler_vk : public Sampler_internal {
		VkSampler				sampler = VK_NULL_HANDLE;
		VkDescriptorImageInfo	image_info;
	};
	// Shader
	struct Shader_vk : public Shader_internal {
		VkShaderModule								module = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding>	bindings;
		std::map<std::string, ui32>					semanticNames;
		std::map<ui32, ui32>						bindingsLocation;
	};
	// RenderPass
	struct RenderPass_vk : public RenderPass_internal {
		VkRenderPass					renderPass;
		std::map<size_t, VkFramebuffer> frameBuffers;
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

	//////////////////////////////////////////////////////////// CONSTRUCTOR & DESTRUCTOR ////////////////////////////////////////////////////////////

	void* VulkanConstructor(GraphicsPrimitiveType type, const void* desc);
	bool VulkanDestructor(Primitive& primitive);

	//////////////////////////////////////////////////////////// GRAPHICS API ////////////////////////////////////////////////////////////
	struct VulkanPipeline {
		VulkanPipeline& operator=(const VulkanPipeline& other)
		{
			semanticNames = other.semanticNames;
			bindingsLocation = other.bindingsLocation;
			layout = other.layout;
			pipelines = other.pipelines;
			descriptors = other.descriptors;
			bindings = other.bindings;
			return *this;
		}

		std::mutex									mutex;
		std::mutex									creationMutex;

		std::map<std::string, ui32>					semanticNames;
		std::vector<ui32>						bindingsLocation;

		VkPipelineLayout							layout = VK_NULL_HANDLE;
		std::map<size_t, VkPipeline>				pipelines;
		DescriptorsManager							descriptors;

		std::vector<VkDescriptorSetLayoutBinding>	bindings;
	};

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
		Image_vk							backBuffer;
		Primitive						backBufferImage;
	};

	class Graphics_vk : public GraphicsDevice {

		VkInstance					m_Instance = VK_NULL_HANDLE;
		VkDevice					m_Device = VK_NULL_HANDLE;
		SwapChain					m_SwapChain;
		VmaAllocator				m_Allocator;

#if SV_GFX_VK_VALIDATION_LAYERS
		VkDebugUtilsMessengerEXT	m_Debug = VK_NULL_HANDLE;
#endif

		std::vector<const char*>	m_Extensions;
		std::vector<const char*>	m_ValidationLayers;
		std::vector<const char*>	m_DeviceExtensions;
		std::vector<const char*>	m_DeviceValidationLayers;

		VkQueue						m_QueueGraphics = VK_NULL_HANDLE;

		// Frame Members

		std::vector<Frame>		m_Frames;
		std::vector<VkFence>	m_ImageFences;
		ui32					m_FrameCount;
		ui32					m_CurrentFrame = 0u;
		std::mutex				m_MutexCMD;
		ui32					m_ActiveCMDCount = 0u;

		ui32					m_ImageIndex;

	public:
		inline Frame& GetFrame() noexcept { return m_Frames[m_CurrentFrame]; }
		inline VkCommandBuffer GetCMD(CommandList cmd) { return GetFrame().commandBuffers[cmd]; }

	private:
		// Binding Members

		VkRenderPass						m_ActiveRenderPass[SV_GFX_COMMAND_LIST_COUNT];
		std::map<size_t, VulkanPipeline>	m_Pipelines;
		std::mutex							m_PipelinesMutex;

	public:
		bool Initialize(const InitializationGraphicsDesc& desc) override;
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

		void UpdateGraphicsState(CommandList cmd);
		size_t ComputeVulkanPipelineHash(const GraphicsState& state);
		bool CreateVulkanPipeline(VulkanPipeline& p, Shader_internal* pVertexShader, Shader_internal* pPixelShader, Shader_internal* pGeometryShader);

	public:
		// Getters
		inline VkInstance GetInstance() const noexcept { return m_Instance; }
		inline VkDevice GetDevice() const noexcept { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const noexcept;
		inline VmaAllocator GetAllocator() const noexcept { return m_Allocator; }
		inline ui32 GetCurrentFrame() const noexcept { return m_CurrentFrame; }
		inline VkQueue GetGraphicsQueue() const noexcept { return m_QueueGraphics; }
		inline SwapChain& GetSwapChain() noexcept { return m_SwapChain; }

		// Primitive Creation
		bool CreateBuffer(Buffer_vk& buffer, const GPUBufferDesc& desc);
		bool CreateImage(Image_vk& image, const GPUImageDesc& desc);
		bool CreateSampler(Sampler_vk& sampler, const SamplerDesc& desc);
		bool CreateShader(Shader_vk& shader, const ShaderDesc& desc);
		bool CreateRenderPass(RenderPass_vk& renderPass, const RenderPassDesc& desc);
		bool CreateInputLayoutState(InputLayoutState_vk& inputLayoutState, const InputLayoutStateDesc& desc);
		bool CreateBlendState(BlendState_vk& blendState, const BlendStateDesc& desc);
		bool CreateDepthStencilState(DepthStencilState_vk& depthStencilState, const DepthStencilStateDesc& desc);
		bool CreateRasterizerState(RasterizerState_vk& rasterizerState, const RasterizerStateDesc& desc);

		ui64 m_IDCount = 0u;
		std::mutex m_IDMutex;
		inline ui64 GetID() noexcept { std::lock_guard<std::mutex> lock(m_IDMutex); return m_IDCount++; }

		// Primitive Destuction
		bool DestroyBuffer(Buffer_vk& buffer);
		bool DestroyImage(Image_vk& image);
		bool DestroySampler(Sampler_vk& sampler);
		bool DestroyShader(Shader_vk& shader);
		bool DestroyRenderPass(RenderPass_vk& renderPass);

		// Device Methods

		CommandList BeginCommandList() override;
		CommandList GetLastCommandList() override;
		ui32 GetCommandListCount() override;

		void BeginRenderPass(CommandList cmd) override;
		void EndRenderPass(CommandList cmd) override;

		void ResizeSwapChain() override;
		GPUImage& AcquireSwapChainImage() override;
		void WaitGPU() override;

		void BeginFrame() override;
		void SubmitCommandLists() override;
		void Present() override;

		void Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, CommandList cmd) override;
		void DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd) override;

		void ClearImage(GPUImage& image, GPUImageLayout oldLayout, GPUImageLayout newLayout, const Color4f& clearColor, float depth, ui32 stencil, CommandList cmd) override;
		void UpdateBuffer(GPUBuffer& buffer, void* pData, ui32 size, ui32 offset, CommandList cmd) override;
		void Barrier(const GPUBarrier* barriers, ui32 count, CommandList cmd) override;

		// CommandBuffers

		VkResult BeginSingleTimeCMD(VkCommandBuffer* pCmd);
		VkResult EndSingleTimeCMD(VkCommandBuffer cmd);

		// Memory Funtions

		void CopyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);

		VkResult CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, ui32 layerCount, VkImageView& view);

		// Sync Functions

		VkSemaphore CreateSemaphore();
		VkFence CreateFence(bool signaled);

		// Shader Layout

		void LoadSpirv_SemanticNames(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, std::map<std::string, ui32>& semanticNames);
		void LoadSpirv_Samplers(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, ShaderType shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings);
		void LoadSpirv_Images(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, ShaderType shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings);
		void LoadSpirv_Uniforms(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, ShaderType shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings);

	};

}