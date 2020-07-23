#include "core.h"

#include "Engine.h"
#include "graphics/PipelineState.h"

#define VMA_IMPLEMENTATION
#include "Graphics_vk.h"
#include "graphics/GraphicsProperties.h"

namespace SV {

	////////////////////////////////////////////DEBUG/////////////////////////////////////////////////
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
		VkDebugUtilsMessageSeverityFlagsEXT flags,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userData
	)
	{
		switch (msgSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			SV::Log("[VULKAN VERBOSE] %s\n", data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			SV::Log("[VULKAN WARNING] %s\n", data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			SV::Log("[VULKAN ERROR] %s\n", data->pMessage);
			break;
		}
		return VK_FALSE;
	}

	VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* alloc, VkDebugUtilsMessengerEXT* debug)
	{
		auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

		if (fn) return fn(instance, create_info, alloc, debug);
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
	void __stdcall vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug)
	{
		auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (fn) fn(instance, debug, nullptr);
	}

	////////////////////////////////////////////ADAPTER/////////////////////////////////////////////////
	Adapter_vk::Adapter_vk(VkPhysicalDevice device) : m_PhysicalDevice(device)
	{
		vkGetPhysicalDeviceProperties(device, &m_Properties);
		vkGetPhysicalDeviceFeatures(device, &m_Features);
		vkGetPhysicalDeviceMemoryProperties(device, &m_MemoryProps);

		// Choose FamilyQueueIndices
		{
			ui32 count = 0u;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
			std::vector<VkQueueFamilyProperties> props(count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

			for (ui32 i = 0; i < props.size(); ++i) {
				const VkQueueFamilyProperties& prop = props[i];

				bool hasGraphics = prop.queueFlags & VK_QUEUE_GRAPHICS_BIT;
				
				if(hasGraphics)	m_FamilyIndex.graphics = i;
			}
		}

		// Suitability
		bool valid = true;
		m_Suitability++;

		if (m_Properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) valid = false;

		if (!valid) m_Suitability = 0u;
	}
	
	////////////////////////////////////////////API/////////////////////////////////////////////////
	bool Graphics_vk::Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc)
	{
		// Instance extensions and validation layers
#if SV_GFX_VK_VALIDATION_LAYERS
		m_ValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
		m_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		m_Extensions.push_back("VK_KHR_surface");
		m_Extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

		// Device extensions and validation layers
#if SV_GFX_VK_VALIDATION_LAYERS
		m_DeviceValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
		m_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		AdjustAllocator();

		svCheck(CreateInstance());

#if SV_GFX_VK_VALIDATION_LAYERS
		svCheck(CreateDebugMessenger());
#endif

		svCheck(CreateAdapters());
		svCheck(CreateLogicalDevice());
		svCheck(CreateAllocator());
		svCheck(CreateFrames());
		svCheck(CreateSwapChain());

		SetProperties();

		return true;
	}

	bool Graphics_vk::Close()
	{
		vkDeviceWaitIdle(m_Device);

		// Destroy Allocator
		vmaDestroyAllocator(m_Allocator);

		// Destroy SwapChain
		svCheck(DestroySwapChain());

		// Destroy command pools
		for (ui32 i = 0; i < m_FrameCount; ++i) {
			Frame& frame = m_Frames[i];
			vkDestroyCommandPool(m_Device, frame.commandPool, nullptr);
			vkDestroyCommandPool(m_Device, frame.transientCommandPool, nullptr);
			vkDestroyFence(m_Device, frame.fence, nullptr);
		}

		// Destroy device and vulkan instance
		vkDestroyDevice(m_Device, nullptr);

#if SV_GFX_VK_VALIDATION_LAYERS
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_Debug);
#endif

		vkDestroyInstance(m_Instance, nullptr);
		return true;
	}


	void Graphics_vk::AdjustAllocator()
	{
		Graphics::_internal::GetPrimitiveAllocator().SetFunctions(VulkanConstructor, VulkanDestructor);
	}

	void Graphics_vk::SetProperties()
	{
		SV::GraphicsProperties props;

		props.transposedMatrices = false;

		Graphics::_internal::SetProperties(props);
	}

	bool Graphics_vk::CreateInstance()
	{
		// Check Extensions
		{
			ui32 count = 0u;
			vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
			std::vector<VkExtensionProperties> props(count);
			vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

			for (ui32 i = 0; i < m_Extensions.size(); ++i) {
				bool found = false;

				for (ui32 j = 0; j < props.size(); ++j) {
					if (strcmp(props[j].extensionName, m_Extensions[i]) == 0) {
						found = true;
						break;
					}
				}

				if (!found) {
					SV::LogE("InstanceExtension '%s' not found", m_Extensions[i]);
					return false;
				}
			}
		}

		// Check Validation Layers
		{
			ui32 count = 0u;
			vkEnumerateInstanceLayerProperties(&count, nullptr);
			std::vector<VkLayerProperties> props(count);
			vkEnumerateInstanceLayerProperties(&count, props.data());

			for (ui32 i = 0; i < m_ValidationLayers.size(); ++i) {
				bool found = false;

				for (ui32 j = 0; j < props.size(); ++j) {
					if (strcmp(props[j].layerName, m_ValidationLayers[i]) == 0) {
						found = true;
						break;
					}
				}

				if (!found) {
					SV::LogE("InstanceValidationLayer '%s' not found", m_ValidationLayers[i]);
					return false;
				}
			}
		}

		// Create Instance
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pEngineName = "SilverEngine";
		app_info.pApplicationName = "";
		//SV::Version engineVersion = GetEngine().GetVersion();
		//app_info.engineVersion = VK_MAKE_VERSION(engineVersion.major, engineVersion.minor, engineVersion.revision);
		app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
		app_info.apiVersion = VK_VERSION_1_2;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		create_info.enabledExtensionCount = ui32(m_Extensions.size());
		create_info.ppEnabledExtensionNames = m_Extensions.data();
		create_info.enabledLayerCount = ui32(m_ValidationLayers.size());
		create_info.ppEnabledLayerNames = m_ValidationLayers.data();

		create_info.flags = 0u;

		vkCheck(vkCreateInstance(&create_info, nullptr, &m_Instance));

		return true;
	}

#if SV_GFX_VK_VALIDATION_LAYERS
	bool Graphics_vk::CreateDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.flags = 0u;
		create_info.messageSeverity = 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		create_info.messageType = 
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		create_info.pfnUserCallback = DebugCallback;
		create_info.pUserData = nullptr;

		vkCheck(vkCreateDebugUtilsMessengerEXT(m_Instance, &create_info, nullptr, &m_Debug));

		return true;
	}
#endif

	bool Graphics_vk::CreateAdapters()
	{
		// Enumerate Vulkan Adapters
		ui32 count = 0u;
		vkCheck(vkEnumeratePhysicalDevices(m_Instance, &count, nullptr));

		std::vector<VkPhysicalDevice> devices(count);
		vkCheck(vkEnumeratePhysicalDevices(m_Instance, &count, devices.data()));

		// Allocate in Graphics member
		for (ui32 i = 0; i < devices.size(); ++i) {
			std::unique_ptr<SV::Adapter> adapter = std::make_unique<SV::Adapter_vk>(devices[i]);
			if(adapter->GetSuitability() > 0)
				Graphics::_internal::AddAdapter(std::move(adapter));
		}

		// Select the most suitable
		ui32 maxSuitability = 0;
		ui32 index = count;

		const auto& adapters = Graphics::GetAdapters();

		for (ui32 i = 0; i < adapters.size(); ++i) {
			SV::Adapter_vk& adapter = *reinterpret_cast<SV::Adapter_vk*>(adapters[i].get());
			if (adapter.GetSuitability() > maxSuitability) index = i;
		}

		// Adapter not found
		if (index == count) {
			SV::LogE("Can't find valid adapters for Vulkan");
			return false;
		}

		Graphics::SetAdapter(index);

		return true;
	}

	bool Graphics_vk::CreateLogicalDevice()
	{
		const SV::Adapter_vk& adapter = *reinterpret_cast<const SV::Adapter_vk*>(Graphics::GetAdapter());
		VkPhysicalDevice physicalDevice = adapter.GetPhysicalDevice();

		// Queue Create Info Structs
		constexpr ui32 queueCount = 1u;
		ui32 queueIndices[] = {
			adapter.GetFamilyIndex().graphics
		};

		const float priorities = 1.f;

		VkDeviceQueueCreateInfo queue_create_info[queueCount];
		for (ui32 i = 0; i < queueCount; ++i) {
			queue_create_info[i] = {};
			queue_create_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info[i].flags = 0u;
			queue_create_info[i].queueFamilyIndex = queueIndices[i];
			queue_create_info[i].queueCount = 1u;
			queue_create_info[i].pQueuePriorities = &priorities;
		}

		// TODO: Check Extensions And ValidationLayers
		
		// Create
		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.flags = 0u;
		create_info.queueCreateInfoCount = queueCount;
		create_info.pQueueCreateInfos = queue_create_info;
		
		create_info.enabledLayerCount = ui32(m_DeviceValidationLayers.size());
		create_info.ppEnabledLayerNames = m_DeviceValidationLayers.data();
		create_info.enabledExtensionCount = ui32(m_DeviceExtensions.size());
		create_info.ppEnabledExtensionNames = m_DeviceExtensions.data();

		create_info.pEnabledFeatures = &adapter.GetFeatures();

		vkCheck(vkCreateDevice(physicalDevice, &create_info, nullptr, &m_Device));

		// Get Queues
		vkGetDeviceQueue(m_Device, adapter.GetFamilyIndex().graphics, 0u, &m_QueueGraphics);

		return true;
	}

	bool Graphics_vk::CreateAllocator()
	{
		Adapter_vk& adapter = *reinterpret_cast<Adapter_vk*>(Graphics::GetAdapter());

		VmaAllocatorCreateInfo create_info{};
		create_info.device = m_Device;
		create_info.instance = m_Instance;
		create_info.physicalDevice = adapter.GetPhysicalDevice();

		vkCheck(vmaCreateAllocator(&create_info, &m_Allocator));

		return true;
	}

	bool Graphics_vk::CreateFrames()
	{
		m_FrameCount = 3u;
		m_Frames.resize(m_FrameCount);

		VkCommandPoolCreateInfo cmdPool_info{};
		cmdPool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdPool_info.queueFamilyIndex = reinterpret_cast<Adapter_vk*>(Graphics::GetAdapter())->GetFamilyIndex().graphics;

		for (ui32 i = 0; i < m_FrameCount; ++i) {
			Frame& frame = m_Frames[i];

			frame.fence = CreateFence(true);

			vkCheck(vkCreateCommandPool(m_Device, &cmdPool_info, nullptr, &frame.commandPool));

			cmdPool_info.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			vkCheck(vkCreateCommandPool(m_Device, &cmdPool_info, nullptr, &frame.transientCommandPool));

			// AllocateCommandBuffers
			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc_info.commandPool = frame.commandPool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = SV_GFX_COMMAND_LIST_COUNT;

			vkCheck(vkAllocateCommandBuffers(m_Device, &alloc_info, frame.commandBuffers));
		}

		return true;
	}

	bool Graphics_vk::CreateSwapChain(VkSwapchainKHR oldSwapchain)
	{
		const Adapter_vk& adapter = *reinterpret_cast<const Adapter_vk*>(Graphics::GetAdapter());
		VkPhysicalDevice physicalDevice = adapter.GetPhysicalDevice();

		WindowHandle hWnd = Window::GetWindowHandle();
		ui32 width = Window::GetWidth();
		ui32 height = Window::GetHeight();

		// Create Surface
		if (oldSwapchain == VK_NULL_HANDLE) 
		{
#ifdef SV_PLATFORM_WINDOWS
			VkWin32SurfaceCreateInfoKHR create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			create_info.hinstance = GetModuleHandle(nullptr);
			create_info.flags = 0u;
			create_info.hwnd = reinterpret_cast<HWND>(hWnd);

			vkCheck(vkCreateWin32SurfaceKHR(m_Instance, &create_info, nullptr, &m_SwapChain.surface));
#endif
		}

		// Check SwapChain Supported by this adapter
		{
			VkBool32 supported;
			vkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, adapter.GetFamilyIndex().graphics, m_SwapChain.surface, &supported));
			if (!supported) {
				SV::LogE("This adapter don't support vulkan swapChain");
				return false;
			}
		}

		// Get Support Details
		vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_SwapChain.surface, &m_SwapChain.capabilities));
		{
			ui32 count = 0;
			vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_SwapChain.surface, &count, nullptr));
			m_SwapChain.presentModes.resize(count);
			vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_SwapChain.surface, &count, m_SwapChain.presentModes.data()));
		}
		{
			ui32 count = 0;
			vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_SwapChain.surface, &count, nullptr));
			m_SwapChain.formats.resize(count);
			vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_SwapChain.surface, &count, m_SwapChain.formats.data()));
		}

		// Create SwapChain
		{
			// Choose image count
			ui32 imageCount = m_SwapChain.capabilities.minImageCount + 1u;
			if (imageCount > m_SwapChain.capabilities.maxImageCount && m_SwapChain.capabilities.maxImageCount > 0)
				imageCount = m_SwapChain.capabilities.maxImageCount;

			// Choose format
			VkSurfaceFormatKHR format = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			if (!m_SwapChain.formats.empty()) format = m_SwapChain.formats[0];

			for (ui32 i = 0; i < m_SwapChain.formats.size(); ++i) {
				VkSurfaceFormatKHR f = m_SwapChain.formats[i];
				if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
					format = f;
					break;
				}
			}

			// Choose Swapchain Extent
			VkExtent2D extent;
			if (m_SwapChain.capabilities.currentExtent.width != UINT32_MAX) extent = m_SwapChain.capabilities.currentExtent;
			else {

				extent.width = width;
				extent.height = height;
				VkExtent2D min = m_SwapChain.capabilities.minImageExtent;
				VkExtent2D max = m_SwapChain.capabilities.maxImageExtent;

				if (extent.width < min.width) extent.width = min.width;
				else if (extent.width > max.width) extent.width = max.width;
				if (extent.height < min.height) extent.height = min.height;
				else if (extent.height > max.height) extent.height = max.height;
			}

			VkSwapchainCreateInfoKHR create_info{};

			// Family Queue Indices
			auto fi = adapter.GetFamilyIndex();

			// TODO: if familyqueue not support that surface, create presentQueue with other index
			////////////// UNFINISHED ///////////////////////
			ui32 indices[] = {
					fi.graphics
			};

			//if (fi.graphics == sc.familyIndexPresent) {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0u;
			create_info.pQueueFamilyIndices = nullptr;
			//}
			//else {
			//	create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			//	create_info.queueFamilyIndexCount = 2u;
			//	create_info.pQueueFamilyIndices = indices;
			//}
			/////////////////////////

			// Choose Present Mode
			VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

			for (ui32 i = 0; i < m_SwapChain.presentModes.size(); ++i) {
				if (m_SwapChain.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
					presentMode = m_SwapChain.presentModes[i];
					break;
				}
			}

			create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			create_info.flags = 0u;
			create_info.surface = m_SwapChain.surface;
			create_info.minImageCount = imageCount;
			create_info.imageFormat = format.format;
			create_info.imageColorSpace = format.colorSpace;
			create_info.imageExtent = extent;
			create_info.imageArrayLayers = 1u;
			create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			create_info.preTransform = m_SwapChain.capabilities.currentTransform;
			create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			create_info.presentMode = presentMode;
			create_info.clipped = VK_TRUE;
			create_info.oldSwapchain = oldSwapchain;

			vkCheck(vkCreateSwapchainKHR(m_Device, &create_info, nullptr, &m_SwapChain.swapChain));

			m_SwapChain.currentFormat = format.format;
			m_SwapChain.currentColorSpace = format.colorSpace;
			m_SwapChain.currentPresentMode = presentMode;
			m_SwapChain.currentExtent = extent;
		}

		// Get Images Handles
		{
			std::vector<VkImage> images;
			ui32 count = 0u;
			vkCheck(vkGetSwapchainImagesKHR(m_Device, m_SwapChain.swapChain, &count, nullptr));
			images.resize(count);
			vkCheck(vkGetSwapchainImagesKHR(m_Device, m_SwapChain.swapChain, &count, images.data()));

			m_SwapChain.images.resize(images.size());
			for (ui32 i = 0; i < m_SwapChain.images.size(); ++i) {
				m_SwapChain.images[i].image = images[i];
			}
		}

		// Create Views and IDs
		{
			for (ui32 i = 0; i < m_SwapChain.images.size(); ++i) {
				VkImage image = m_SwapChain.images[i].image;

				m_SwapChain.images[i].ID = GetID();

				VkImageViewCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				create_info.flags = 0u;
				create_info.image = image;
				create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				create_info.format = m_SwapChain.currentFormat;
				create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				create_info.subresourceRange.baseArrayLayer = 0u;
				create_info.subresourceRange.baseMipLevel = 0u;
				create_info.subresourceRange.layerCount = 1u;
				create_info.subresourceRange.levelCount = 1u;

				vkCheck(vkCreateImageView(m_Device, &create_info, nullptr, &m_SwapChain.images[i].view));
			}
		}

		// Create Semaphores
		m_SwapChain.semAcquireImage = CreateSemaphore();
		m_SwapChain.semPresent = CreateSemaphore();

		m_ImageFences.clear();
		m_ImageFences.resize(m_SwapChain.images.size(), VK_NULL_HANDLE);

		return true;
	}

	bool Graphics_vk::DestroySwapChain(bool resizing)
	{
		vkDestroySemaphore(m_Device, m_SwapChain.semAcquireImage, nullptr);
		vkDestroySemaphore(m_Device, m_SwapChain.semPresent, nullptr);

		for (ui32 i = 0; i < m_SwapChain.images.size(); ++i) {

			vkDestroyImageView(m_Device, m_SwapChain.images[i].view, nullptr);
		}
		m_SwapChain.images.clear();

		if (resizing) {
			vkDestroySwapchainKHR(m_Device, m_SwapChain.swapChain, nullptr);
			vkDestroySurfaceKHR(m_Instance, m_SwapChain.surface, nullptr);
		}
		return true;
	}

	/////////////////////////////////////////////// STATE METHODS ////////////////////////////////////////////////////

	void Graphics_vk::UpdateGraphicsState(SV::CommandList cmd_)
	{
		SV::_internal::GraphicsPipelineState& state = SV::Graphics::_internal::GetPipelineState().graphics[cmd_];

		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);
		GraphicsPipeline_vk& pipeline = *reinterpret_cast<GraphicsPipeline_vk*>(state.pipeline);

		VkCommandBuffer cmd = GetCMD(cmd_);

		bool updatePipeline = state.flags & SV_GFX_GRAPHICS_PIPELINE_STATE_PIPELINE;

		// Bind Vertex Buffers
		if (state.flags & SV_GFX_GRAPHICS_PIPELINE_STATE_VERTEX_BUFFER) {
			
			VkBuffer buffers[SV_GFX_VERTEX_BUFFER_COUNT];
			VkDeviceSize offsets[SV_GFX_VERTEX_BUFFER_COUNT];
			for (ui32 i = 0; i < state.vertexBuffersCount; ++i) {
				if (state.vertexBuffers[i] == nullptr) {
					buffers[i] = VK_NULL_HANDLE;
					offsets[i] = 0u;
				}
				else {
					buffers[i] = reinterpret_cast<Buffer_vk*>(state.vertexBuffers[i])->buffer;
					offsets[i] = VkDeviceSize(state.vertexBufferOffsets[i]);
				}
			}
			
			// TODO: strides??
			vkCmdBindVertexBuffers(cmd, 0u, state.vertexBuffersCount, buffers, offsets);
		}
		// Bind Index Buffer
		if (state.flags & SV_GFX_GRAPHICS_PIPELINE_STATE_INDEX_BUFFER && state.indexBuffer != nullptr) {
			vkCmdBindIndexBuffer(cmd, reinterpret_cast<Buffer_vk*>(state.indexBuffer)->buffer, state.indexBufferOffset, ParseIndexType(state.indexBuffer->GetIndexType()));
		}

		// Bind RenderPass
		if (state.flags & (SV_GFX_GRAPHICS_PIPELINE_STATE_RENDER_PASS | SV_GFX_GRAPHICS_PIPELINE_STATE_CLEAR_COLOR | SV_GFX_GRAPHICS_PIPELINE_STATE_CLEAR_DEPTH_STENCIL)) {

			updatePipeline = true;

			// End
			if (m_ActiveRenderPass[cmd_]) {
				vkCmdEndRenderPass(cmd);
				m_ActiveRenderPass[cmd_] = false;
			}

			// Begin
			if (state.renderPass != nullptr) {

				m_ActiveRenderPass[cmd_] = true;

				// Color Attachments clear values
				VkClearValue clearValues[SV_GFX_ATTACHMENTS_COUNT];
				for (ui32 i = 0; i < renderPass.GetAttachmentsCount(); ++i) {

					if (renderPass.HasDepthStencilAttachment() && renderPass.GetDepthStencilAttachment() == i) {
						clearValues[i].depthStencil.depth = state.clearDepthStencil.first;
						clearValues[i].depthStencil.stencil = state.clearDepthStencil.second;

						clearValues[i].color = {};
					}
					else {
						clearValues[i].depthStencil.depth = 1.f;
						clearValues[i].depthStencil.stencil = 0u;

						clearValues[i].color = { state.clearColors[i].x, state.clearColors[i].y, state.clearColors[i].z, state.clearColors[i].w };
					}
				}

				// FrameBuffer
				VkFramebuffer fb;
				{
					// Calculate hash value for attachments
					size_t hash = 0u;
					SV::HashCombine(hash, renderPass.GetAttachmentsCount());
					for (ui32 i = 0; i < renderPass.GetAttachmentsCount(); ++i) {
						Image_vk& att = *reinterpret_cast<Image_vk*>(state.attachments[i]);
						SV::HashCombine(hash, att.ID);
					}

					// Find framebuffer
					auto it = renderPass.frameBuffers.find(hash);
					if (it == renderPass.frameBuffers.end()) {

						// Create attachments views list
						VkImageView views[SV_GFX_ATTACHMENTS_COUNT];
						ui32 width = 0, height = 0, layers = 0;

						for (ui32 i = 0; i < renderPass.GetAttachmentsCount(); ++i) {
							Image_vk& att = *reinterpret_cast<Image_vk*>(state.attachments[i]);

							if (renderPass.GetDepthStencilAttachment() == i) {
								views[i] = att.depthStencilView;
							}
							else {
								views[i] = att.renderTargetView;
							}

							if (i == 0) {
								width = att.GetWidth();
								height = att.GetHeight();
								layers = att.GetLayers();
							}
						}

						VkFramebufferCreateInfo create_info{};
						create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
						create_info.renderPass = renderPass.renderPass;
						create_info.attachmentCount = renderPass.GetAttachmentsCount();
						create_info.pAttachments = views;
						create_info.width = width;
						create_info.height = height;
						create_info.layers = layers;

						vkAssert(vkCreateFramebuffer(m_Device, &create_info, nullptr, &fb));

						renderPass.frameBuffers[hash] = fb;
					}
					else {
						fb = it->second;
					}
				}

				VkRenderPassBeginInfo begin_info{};
				begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				begin_info.renderPass = renderPass.renderPass;
				begin_info.framebuffer = fb;
				begin_info.renderArea.offset.x = 0;
				begin_info.renderArea.offset.y = 0;
				begin_info.renderArea.extent.width = state.attachments[0]->GetWidth();
				begin_info.renderArea.extent.height = state.attachments[0]->GetHeight();
				begin_info.clearValueCount = renderPass.GetAttachmentsCount();
				begin_info.pClearValues = clearValues;

				vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
			}

		}

		// Bind Pipeline
		if (updatePipeline) {

			// Compute hash
			size_t hash = 0u;
			SV::HashCombine(hash, pipeline.ID);
			SV::HashCombine(hash, renderPass.renderPass);
			
			// Find Pipeline
			VkPipeline vkPipeline = VK_NULL_HANDLE;

			pipeline.mutex.lock();
			auto it = pipeline.pipelines.find(hash);
			if (it == pipeline.pipelines.end()) {

				// Shader Stages
				VkPipelineShaderStageCreateInfo shaderStages[SV_GFX_SHADER_TYPE_GFX_COUNT] = {};
				ui32 shaderStagesCount = 0u;

				if (pipeline.GetVertexShader()) {
					VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
					stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					stage.flags = 0u;
					stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
					stage.module = reinterpret_cast<Shader_vk*>(pipeline.GetVertexShader())->module;
					stage.pName = "main";
					stage.pSpecializationInfo = nullptr;
				}
				if (pipeline.GetPixelShader()) {
					VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
					stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					stage.flags = 0u;
					stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
					stage.module = reinterpret_cast<Shader_vk*>(pipeline.GetPixelShader())->module;
					stage.pName = "main";
					stage.pSpecializationInfo = nullptr;
				}
				if (pipeline.GetGeometryShader()) {
					VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
					stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					stage.flags = 0u;
					stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
					stage.module = reinterpret_cast<Shader_vk*>(pipeline.GetGeometryShader())->module;
					stage.pName = "main";
					stage.pSpecializationInfo = nullptr;
				}

				// Input Layout
				VkPipelineVertexInputStateCreateInfo vertexInput{};
				vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				
				VkVertexInputBindingDescription bindings[SV_GFX_INPUT_SLOT_COUNT];
				VkVertexInputAttributeDescription attributes[SV_GFX_INPUT_ELEMENT_COUNT];
				{
					SV_GFX_INPUT_LAYOUT_DESC il = pipeline.GetInputLayout();
					for (ui32 i = 0; i < il.slots.size(); ++i) {
						bindings[i].binding = il.slots[i].slot;
						bindings[i].inputRate = il.slots[i].instanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
						bindings[i].stride = il.slots[i].stride;
					}
					for (ui32 i = 0; i < il.elements.size(); ++i) {
						attributes[i].binding = il.elements[i].inputSlot;
						attributes[i].format = ParseFormat(il.elements[i].format);
						attributes[i].location = pipeline.semanticNames[il.elements[i].name];
						attributes[i].offset = il.elements[i].offset;
					}
					vertexInput.vertexBindingDescriptionCount = ui32(il.slots.size());
					vertexInput.pVertexBindingDescriptions = bindings;
					vertexInput.pVertexAttributeDescriptions = attributes;
					vertexInput.vertexAttributeDescriptionCount = ui32(il.elements.size());
				}

				// Rasterizer State
				VkPipelineRasterizationStateCreateInfo rasterizerState{};
				{
					const SV_GFX_RASTERIZER_STATE_DESC& rDesc = pipeline.GetRasterizerState();

					rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
					rasterizerState.flags = 0u;
					rasterizerState.depthClampEnable = VK_FALSE;
					rasterizerState.rasterizerDiscardEnable = VK_FALSE;
					rasterizerState.polygonMode = rDesc.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
					rasterizerState.cullMode = ParseCullMode(rDesc.cullMode);
					rasterizerState.frontFace = rDesc.clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
					rasterizerState.depthBiasEnable = VK_FALSE;
					rasterizerState.depthBiasConstantFactor = 0;
					rasterizerState.depthBiasClamp = 0;
					rasterizerState.depthBiasSlopeFactor = 0;
					rasterizerState.lineWidth = rDesc.lineWidth;
				}

				// Blend State
				VkPipelineColorBlendStateCreateInfo blendState{};
				VkPipelineColorBlendAttachmentState attachments[SV_GFX_RENDER_TARGET_ATT_COUNT];
				{
					const SV_GFX_BLEND_STATE_DESC& bDesc = pipeline.GetBlendState();
					
					for (ui32 i = 0; i < bDesc.attachments.size(); ++i)
					{
						const SV_GFX_BLEND_ATTACHMENT_DESC& b = bDesc.attachments[i];

						attachments[i].blendEnable = b.blendEnabled ? VK_TRUE : VK_FALSE;
						attachments[i].srcColorBlendFactor = ParseBlendFactor(b.srcColorBlendFactor);
						attachments[i].dstColorBlendFactor = ParseBlendFactor(b.dstColorBlendFactor);;
						attachments[i].colorBlendOp = ParseBlendOp(b.colorBlendOp);
						attachments[i].srcAlphaBlendFactor = ParseBlendFactor(b.srcAlphaBlendFactor);;
						attachments[i].dstAlphaBlendFactor = ParseBlendFactor(b.dstAlphaBlendFactor);;
						attachments[i].alphaBlendOp = ParseBlendOp(b.alphaBlendOp);;
						attachments[i].colorWriteMask = ParseColorComponent(b.colorWriteMask);
					}

					blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
					blendState.flags = 0u;
					blendState.logicOpEnable = VK_FALSE;
					blendState.logicOp;
					blendState.attachmentCount = ui32(bDesc.attachments.size());
					blendState.pAttachments = attachments;
					blendState.blendConstants[0] = bDesc.blendConstants.x;
					blendState.blendConstants[1] = bDesc.blendConstants.y;
					blendState.blendConstants[2] = bDesc.blendConstants.z;
					blendState.blendConstants[3] = bDesc.blendConstants.w;
				}

				// DepthStencilState
				VkPipelineDepthStencilStateCreateInfo depthStencilState{};
				{
					const SV_GFX_DEPTHSTENCIL_STATE_DESC& dDesc = pipeline.GetDepthStencilState();
					depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
					depthStencilState.flags = 0u;
					depthStencilState.depthTestEnable = dDesc.depthTestEnabled;
					depthStencilState.depthWriteEnable = dDesc.depthWriteEnabled;
					depthStencilState.depthCompareOp = ParseCompareOp(dDesc.depthCompareOp);
					
					depthStencilState.stencilTestEnable = dDesc.stencilTestEnabled;
					depthStencilState.front.failOp = ParseStencilOp(dDesc.front.failOp);
					depthStencilState.front.passOp = ParseStencilOp(dDesc.front.passOp);
					depthStencilState.front.depthFailOp = ParseStencilOp(dDesc.front.depthFailOp);
					depthStencilState.front.compareOp = ParseCompareOp(dDesc.front.compareOp);
					depthStencilState.front.compareMask = dDesc.front.compareMask;
					depthStencilState.front.writeMask = dDesc.front.writeMask;
					depthStencilState.front.reference = dDesc.front.reference;
					depthStencilState.back.failOp = ParseStencilOp(dDesc.back.failOp);
					depthStencilState.back.passOp = ParseStencilOp(dDesc.back.passOp);
					depthStencilState.back.depthFailOp = ParseStencilOp(dDesc.back.depthFailOp);
					depthStencilState.back.compareOp = ParseCompareOp(dDesc.back.compareOp);
					depthStencilState.back.compareMask = dDesc.back.compareMask;
					depthStencilState.back.writeMask = dDesc.back.writeMask;
					depthStencilState.back.reference = dDesc.back.reference;

					depthStencilState.depthBoundsTestEnable = VK_FALSE;
					depthStencilState.minDepthBounds = 0.f;
					depthStencilState.maxDepthBounds = 1.f;
				}

				// Topology
				VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
				inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssembly.topology = ParseTopology(pipeline.GetTopology());

				// ViewportState
				VkPipelineViewportStateCreateInfo viewportState{};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.flags = 0u;
				viewportState.viewportCount = SV_GFX_VIEWPORT_COUNT;
				viewportState.pViewports = nullptr;
				viewportState.scissorCount = SV_GFX_SCISSOR_COUNT;
				viewportState.pScissors = nullptr;

				// MultisampleState
				VkPipelineMultisampleStateCreateInfo multisampleState{};
				multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampleState.flags = 0u;
				multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				multisampleState.sampleShadingEnable = VK_FALSE;
				multisampleState.minSampleShading = 1.f;
				multisampleState.pSampleMask = nullptr;
				multisampleState.alphaToCoverageEnable = VK_FALSE;
				multisampleState.alphaToOneEnable = VK_FALSE;

				// Dynamic States
				VkDynamicState dynamicStates[] = {
					VK_DYNAMIC_STATE_VIEWPORT,
					VK_DYNAMIC_STATE_SCISSOR,
				};

				VkPipelineDynamicStateCreateInfo dynamicStatesInfo{};
				dynamicStatesInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicStatesInfo.dynamicStateCount = 2u;
				dynamicStatesInfo.pDynamicStates = dynamicStates;

				// Creation
				VkGraphicsPipelineCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				create_info.flags = 0u;
				create_info.stageCount = shaderStagesCount;
				create_info.pStages = shaderStages;
				create_info.pVertexInputState = &vertexInput;
				create_info.pInputAssemblyState = &inputAssembly;
				create_info.pTessellationState = nullptr;
				create_info.pViewportState = &viewportState;
				create_info.pRasterizationState = &rasterizerState;
				create_info.pMultisampleState = &multisampleState;
				create_info.pDepthStencilState = &depthStencilState;
				create_info.pColorBlendState = &blendState;
				create_info.pDynamicState = &dynamicStatesInfo;
				create_info.layout = pipeline.layout;
				create_info.renderPass = renderPass.renderPass;
				create_info.subpass = 0u;

				vkAssert(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1u, &create_info, nullptr, &vkPipeline));
				pipeline.pipelines[hash] = vkPipeline;
			}
			else {
				vkPipeline = it->second;
			}
			pipeline.mutex.unlock();

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

		}

		// Bind Viewports
		if (state.flags & SV_GFX_GRAPHICS_PIPELINE_STATE_VIEWPORT) {
			
			VkViewport viewports[SV_GFX_VIEWPORT_COUNT];
			for (ui32 i = 0; i < state.viewportsCount; ++i) {
				const SV_GFX_VIEWPORT& vp = state.viewports[i];

				viewports[i].x			= vp.x;
				viewports[i].y			= vp.y;
				viewports[i].width		= vp.width;
				viewports[i].height		= vp.height;
				viewports[i].minDepth	= vp.minDepth;
				viewports[i].maxDepth	= vp.maxDepth;
			}
			for (ui32 i = state.viewportsCount; i < SV_GFX_VIEWPORT_COUNT; ++i) {
				viewports[i].x = 0.f;
				viewports[i].y = 0.f;
				viewports[i].width = 1.f;
				viewports[i].height = 1.f;
				viewports[i].minDepth = 0.f;
				viewports[i].maxDepth = 1.f;
			}

			vkCmdSetViewport(cmd, 0u, SV_GFX_VIEWPORT_COUNT, viewports);

		}

		// Bind Scissors
		if (state.flags & SV_GFX_GRAPHICS_PIPELINE_STATE_SCISSOR) {

			VkRect2D scissors[SV_GFX_SCISSOR_COUNT];
			for (ui32 i = 0; i < state.scissorsCount; ++i) {
				const SV_GFX_SCISSOR& sc = state.scissors[i];

				scissors[i].offset.x = sc.x;
				scissors[i].offset.y = sc.y;
				scissors[i].extent.width = sc.width;
				scissors[i].extent.height = sc.height;
			}
			for (ui32 i = state.scissorsCount; i < SV_GFX_SCISSOR_COUNT; ++i) {
				scissors[i].offset.x = 0.f;
				scissors[i].offset.y = 0.f;
				scissors[i].extent.width = 1.f;
				scissors[i].extent.height = 1.f;
			}

			vkCmdSetScissor(cmd, 0u, SV_GFX_SCISSOR_COUNT, scissors);

		}

		// Update Descriptors
		if (state.flags & (SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER | SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER | SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE)) {

			VkDescriptorSet descSet = pipeline.descriptorSets[cmd_];

			auto& writeDesc = pipeline.writeDescriptors;

			for (ui32 i = 0; i < pipeline.setLayoutBindings.size(); ++i) {

				auto& binding = pipeline.setLayoutBindings[i];

				writeDesc[i].dstSet = descSet;

				switch (binding.descriptorType)
				{
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				{
					Sampler_vk& sampler = *reinterpret_cast<Sampler_vk*>(state.sampers[ParseShaderType(binding.stageFlags)][pipeline.bindingsLocation[writeDesc[i].dstBinding]]);
					writeDesc[i].pImageInfo = &sampler.image_info;
				}
					break;

				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				{
					Image_vk& image = *reinterpret_cast<Image_vk*>(state.images[ParseShaderType(binding.stageFlags)][pipeline.bindingsLocation[writeDesc[i].dstBinding]]);
					writeDesc[i].pImageInfo = &image.image_info;
				}
					break;

				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(state.constantBuffers[ParseShaderType(binding.stageFlags)][pipeline.bindingsLocation[writeDesc[i].dstBinding]]);
					writeDesc[i].pBufferInfo = &buffer.buffer_info;
				}
				break;
				}

			}

			vkUpdateDescriptorSets(m_Device, writeDesc.size(), writeDesc.data(), 0u, nullptr);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0u, 1u, &descSet, 0u, nullptr);
		}

		state.flags = 0u;
	}

	/////////////////////////////////////////////// GETTERS ////////////////////////////////////////////////////
	
	VkPhysicalDevice Graphics_vk::GetPhysicalDevice() const noexcept
	{
		return reinterpret_cast<Adapter_vk*>(Graphics::GetAdapter())->GetPhysicalDevice();
	}

	/////////////////////////////////////////////// STATE METHODS ////////////////////////////////////////////////////

	CommandList Graphics_vk::BeginCommandList()
	{
		m_MutexCMD.lock();
		CommandList index = m_ActiveCMDCount++;
		VkCommandBuffer cmd = GetCMD(index);
		m_MutexCMD.unlock();

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = nullptr;

		vkAssert(vkBeginCommandBuffer(cmd, &begin_info));

		return index;
	}

	/////////////////////////////////////////////// COMMAND BUFFERS ////////////////////////////////////////////////////
	VkResult Graphics_vk::BeginSingleTimeCMD(VkCommandBuffer* pCmd)
	{
		// Allocate CommandBuffer
		{
			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc_info.commandPool = m_Frames[m_CurrentFrame].transientCommandPool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = 1u;

			VkResult res = vkAllocateCommandBuffers(m_Device, &alloc_info, pCmd);
			if (res != VK_SUCCESS) {
				SV::LogE("Can't allocate SingleTime CommandBuffer");
				return res;
			}
		}

		// Begin CommandBuffer
		{
			VkCommandBufferBeginInfo begin_info{};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			begin_info.pInheritanceInfo = nullptr;

			VkResult res = vkBeginCommandBuffer(*pCmd, &begin_info);
			if (res != VK_SUCCESS) {
				SV::LogE("Can't begin SingleTime CommandBuffer");
				vkFreeCommandBuffers(m_Device, m_Frames[m_CurrentFrame].transientCommandPool, 1u, pCmd);
				return res;
			}
		}

		return VK_SUCCESS;
	}
	VkResult Graphics_vk::EndSingleTimeCMD(VkCommandBuffer cmd)
	{
		// End CommandBuffer
		VkResult res = vkEndCommandBuffer(cmd);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 0u;
		submit_info.commandBufferCount = 1u;
		submit_info.pCommandBuffers = &cmd;
		submit_info.signalSemaphoreCount = 0u;
		
		vkExt(vkQueueSubmit(m_QueueGraphics, 1u, &submit_info, VK_NULL_HANDLE));
		vkQueueWaitIdle(m_QueueGraphics);

		// Free CommandBuffer
		if(res == VK_SUCCESS) vkFreeCommandBuffers(m_Device, m_Frames[m_CurrentFrame].transientCommandPool, 1u, &cmd);

		return res;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VmaMemoryUsage Graphics_vk::GetMemoryUsage(SV_GFX_USAGE usage, SV::CPUAccessFlags cpuAccess)
	{
		switch (usage)
		{
		case SV_GFX_USAGE_DEFAULT:
			if (cpuAccess & (SV_GFX_CPU_ACCESS_ALL) == SV_GFX_CPU_ACCESS_ALL)
				return VMA_MEMORY_USAGE_CPU_TO_GPU;
			else if (cpuAccess & SV_GFX_CPU_ACCESS_READ)
				return VMA_MEMORY_USAGE_GPU_TO_CPU;
			else return VMA_MEMORY_USAGE_GPU_ONLY;
		case SV_GFX_USAGE_STATIC:
			return VMA_MEMORY_USAGE_GPU_ONLY;
		case SV_GFX_USAGE_DYNAMIC:
			return VMA_MEMORY_USAGE_CPU_TO_GPU;
		case SV_GFX_USAGE_STAGING:
			SV::LogW("TODO: Staging resources");
			return VMA_MEMORY_USAGE_GPU_ONLY;
		}
	}

	VkResult  Graphics_vk::CreateStagingBuffer(VkBuffer& buffer, VmaAllocation& memory, void* data, VkDeviceSize size)
	{
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

		VmaAllocationInfo info;

		vkExt(vmaCreateBuffer(m_Allocator, &buffer_info, &alloc_info, &buffer, &memory, &info));

		memcpy(info.pMappedData, data, size);

		return VK_SUCCESS;
	}

	void Graphics_vk::CopyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
	{
		VkBufferCopy copy_info{};
		copy_info.dstOffset = dstOffset;
		copy_info.srcOffset = srcOffset;
		copy_info.size = size;
		vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1u, &copy_info);
	}

	VkResult Graphics_vk::MapMemoryUsingStagingBuffer(VkCommandBuffer cmd, VkImage image, VmaAllocation memory, void* data, VkDeviceSize size, ui32 width, ui32 height)
	{
		VkBuffer		stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation	stagingMemory = VK_NULL_HANDLE;
		
		CreateStagingBuffer(stagingBuffer, stagingMemory, data, size);

		// Copy StagingBuffer into image
		{
			VkBufferImageCopy region{};
			region.bufferOffset = stagingMemory->GetOffset();
			region.bufferRowLength = 0u;
			region.bufferImageHeight = 0u;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.baseArrayLayer = 0u;
			region.imageSubresource.layerCount = 1u;
			region.imageSubresource.mipLevel = 0u;
			region.imageOffset.x = 0u;
			region.imageOffset.y = 0u;
			region.imageOffset.z = 0u;
			region.imageExtent.width = width;
			region.imageExtent.height = height;
			region.imageExtent.depth = 1u;

			vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region);
		}

		// Remove StagingBuffer
		vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingMemory);
		return VK_SUCCESS;
	}

	VkResult Graphics_vk::CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, ui32 layerCount, VkImageView& view)
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.flags = 0u;
		create_info.image = image;
		create_info.viewType = viewType;
		create_info.format = format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = aspectFlags;
		create_info.subresourceRange.baseMipLevel = 0u;
		create_info.subresourceRange.levelCount = 1u;
		create_info.subresourceRange.baseArrayLayer = 0u;
		create_info.subresourceRange.layerCount = layerCount;

		return vkCreateImageView(m_Device, &create_info, nullptr, &view);
	}

	VkResult Graphics_vk::ImageMemoryBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspect, ui32 layers)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = aspect;
		barrier.subresourceRange.baseArrayLayer = 0u;
		barrier.subresourceRange.baseMipLevel = 0u;
		barrier.subresourceRange.layerCount = layers;
		barrier.subresourceRange.levelCount = 1u;

		barrier.srcAccessMask;
		barrier.dstAccessMask;
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
		VkPipelineStageFlags destStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0u;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

			srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			printf("Invalid Transfer Layout");
		}

		vkCmdPipelineBarrier(cmd, srcStageMask, destStageMask, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
		return VK_SUCCESS;
	}

	/////////////////////////////////////////////// SYNC METHODS ///////////////////////////////////////

	VkSemaphore Graphics_vk::CreateSemaphore()
	{
		VkSemaphore sem = VK_NULL_HANDLE;
		VkSemaphoreCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(m_Device, &create_info, nullptr, &sem) != VK_SUCCESS) 
			return VK_NULL_HANDLE;

		return sem;
	}
	VkFence Graphics_vk::CreateFence(bool sign)
	{
		VkFence fence = VK_NULL_HANDLE;
		VkFenceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = sign ? VK_FENCE_CREATE_SIGNALED_BIT : 0u;

		if (vkCreateFence(m_Device, &createInfo, nullptr, &fence) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		return fence;
	}

	/////////////////////////////////////////////// SHADER LAYOUT ///////////////////////////////////////

	void Graphics_vk::LoadSpirv_SemanticNames(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, std::map<std::string, ui32>& semanticNames)
	{
		// Semantic names
		for (ui32 i = 0; i < shaderResources.stage_inputs.size(); ++i) {
			auto& input = shaderResources.stage_inputs[i];
			semanticNames[input.name] = compiler.get_decoration(input.id, spv::Decoration::DecorationLocation);
		}
	}

	void Graphics_vk::LoadSpirv_Samplers(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, SV_GFX_SHADER_TYPE shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		auto& samplers = shaderResources.separate_samplers;

		size_t offset = bindings.size();
		bindings.resize(offset + samplers.size());

		for (ui64 i = 0; i < samplers.size(); ++i) {
			auto& sampler = samplers[i];

			VkDescriptorSetLayoutBinding& b = bindings[offset + i];
			b.binding = compiler.get_decoration(sampler.id, spv::Decoration::DecorationBinding);
			b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			b.descriptorCount = 1u;
			b.stageFlags = ParseShaderType(shaderType);
			b.pImmutableSamplers = nullptr;
		}
	}

	void Graphics_vk::LoadSpirv_Images(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, SV_GFX_SHADER_TYPE shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		auto& images = shaderResources.separate_images;

		size_t offset = bindings.size();
		bindings.resize(offset + images.size());

		for (ui64 i = 0; i < images.size(); ++i) {
			auto& image = images[i];

			VkDescriptorSetLayoutBinding& b = bindings[offset + i];
			b.binding = compiler.get_decoration(image.id, spv::Decoration::DecorationBinding);
			b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			b.descriptorCount = 1u;
			b.stageFlags = ParseShaderType(shaderType);
			b.pImmutableSamplers = nullptr;
		}
	}

	void Graphics_vk::LoadSpirv_Uniforms(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& shaderResources, SV_GFX_SHADER_TYPE shaderType, std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		auto& uniforms = shaderResources.uniform_buffers;

		size_t offset = bindings.size();
		bindings.resize(offset + uniforms.size());

		for (ui64 i = 0; i < uniforms.size(); ++i) {
			auto& uniform = uniforms[i];

			VkDescriptorSetLayoutBinding& b = bindings[offset + i];
			b.binding = compiler.get_decoration(uniform.id, spv::Decoration::DecorationBinding);
			b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			b.descriptorCount = 1u;
			b.stageFlags = ParseShaderType(shaderType);
			b.pImmutableSamplers = nullptr;
		}
	}

	/////////////////////////////////////////////////// CREATION ///////////////////////////////////////////////////

	bool Graphics_vk::CreateBuffer(Buffer_vk& buffer, const SV_GFX_BUFFER_DESC& desc)
	{
		VkBufferUsageFlags bufferUsage = 0u;
		VmaMemoryUsage memoryUsage = GetMemoryUsage(desc.usage, desc.CPUAccess);
		bool mustUseStagingBuffer = memoryUsage == VMA_MEMORY_USAGE_GPU_ONLY;
		bool mustConserveStagingBuffer = desc.usage == SV_GFX_USAGE_DEFAULT && desc.CPUAccess == SV_GFX_CPU_ACCESS_WRITE;

		// Usage Flags
		switch (desc.bufferType)
		{
		case SV_GFX_BUFFER_TYPE_VERTEX:
			bufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case SV_GFX_BUFFER_TYPE_INDEX:
			bufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case SV_GFX_BUFFER_TYPE_CONSTANT:
			bufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		}

		if (mustUseStagingBuffer) bufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		// Create Buffer
		{
			VkBufferCreateInfo buffer_info{};
			buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_info.flags = 0u;
			buffer_info.usage = bufferUsage;
			buffer_info.size = desc.size;

			buffer_info.pQueueFamilyIndices = nullptr;
			buffer_info.queueFamilyIndexCount = 0u;
			buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo alloc_info{};
			alloc_info.usage = memoryUsage;

			vkCheck(vmaCreateBuffer(m_Allocator, &buffer_info, &alloc_info, &buffer.buffer, &buffer.allocation, nullptr));
		}

		// Set data
		if (desc.pData) {
			if (mustUseStagingBuffer) {
				VkCommandBuffer cmd;
				vkCheck(BeginSingleTimeCMD(&cmd));

				vkCheck(CreateStagingBuffer(buffer.stagingBuffer, buffer.stagingAllocation, desc.pData, desc.size));

				CopyBuffer(cmd, buffer.stagingBuffer, buffer.buffer, 0u, 0u, desc.size);

				vkCheck(EndSingleTimeCMD(cmd));

				if (!mustConserveStagingBuffer) {
					vmaDestroyBuffer(m_Allocator, buffer.stagingBuffer, buffer.stagingAllocation);
					buffer.stagingBuffer = VK_NULL_HANDLE;
					buffer.stagingAllocation = VK_NULL_HANDLE;
				}
			}
			else {
				void* d;
				vmaMapMemory(m_Allocator, buffer.allocation, &d);
				memcpy(d, desc.pData, desc.size);
				vmaUnmapMemory(m_Allocator, buffer.allocation);
			}
		}

		if (mustUseStagingBuffer) {
			// TODO: memory barrier
		}

		// Desc
		buffer.buffer_info.buffer = buffer.buffer;
		buffer.buffer_info.offset = 0u;
		buffer.buffer_info.range = desc.size;

		return true;
	}

	bool Graphics_vk::CreateImage(Image_vk& image, const SV_GFX_IMAGE_DESC& desc)
	{
		VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;
		VkFormat format = ParseFormat(desc.format);
		VkImageUsageFlags imageUsage = 0u;
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VmaMemoryUsage memoryUsage = GetMemoryUsage(desc.usage, desc.CPUAccess);
		VmaAllocationInfo alloc;
		
		{
			switch (desc.dimension)
			{
			case 1:
				imageType = VK_IMAGE_TYPE_1D;
				break;
			case 2:
				imageType = VK_IMAGE_TYPE_2D;
				break;
			case 3:
				imageType = VK_IMAGE_TYPE_3D;
				break;
			}

			if (desc.type & SV_GFX_IMAGE_TYPE_RENDER_TARGET) {
				imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
			if (desc.type & SV_GFX_IMAGE_TYPE_SHADER_RESOURCE) {
				imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			if (desc.type & SV_GFX_IMAGE_TYPE_DEPTH_STENCIL) {
				imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}

			switch (desc.dimension)
			{
			case 1:
				if (desc.layers > 1) viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
				else viewType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			case 2:
				if (desc.layers > 1) viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				else viewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
			case 3:
				if (desc.layers > 1) viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
				else viewType = VK_IMAGE_VIEW_TYPE_CUBE;
				break;
			}
		}

		// Create Image
		{
			VkImageCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			create_info.flags = 0u;
			create_info.imageType = imageType;
			create_info.format = format;
			create_info.extent.width = desc.width;
			create_info.extent.height = desc.height;
			create_info.extent.depth = desc.depth;
			create_info.arrayLayers = desc.layers;
			create_info.samples = VK_SAMPLE_COUNT_1_BIT;
			create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			create_info.usage = imageUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0u;
			create_info.pQueueFamilyIndices = nullptr;
			create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			create_info.mipLevels = 1u;

			VmaAllocationCreateInfo alloc_info{};
			alloc_info.usage = memoryUsage;
			alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
			
			vkCheck(vmaCreateImage(m_Allocator, &create_info, &alloc_info, &image.image, &image.allocation, &alloc));
		}

		// Set Data
		VkCommandBuffer cmd;

		VkImageLayout finalLayout = ParseImageLayout(desc.layout);

		vkCheck(BeginSingleTimeCMD(&cmd));
		
		if (desc.pData) {
			// Mapping Data
			if (alloc.pUserData) {
				memcpy(alloc.pMappedData, desc.pData, desc.size);
				vkCheck(ImageMemoryBarrier(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, finalLayout, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers));
			}
			// Use staging buffer to map data in Device memory
			else {
				vkCheck(ImageMemoryBarrier(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers));
				vkCheck(MapMemoryUsingStagingBuffer(cmd, image.image, image.allocation, desc.pData, desc.size, desc.width, desc.height));
				vkCheck(ImageMemoryBarrier(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers));
			}
		}
		
		vkCheck(EndSingleTimeCMD(cmd));

		// TODO: MipMapping

		// Create Render Target View
		if (desc.type & SV_GFX_IMAGE_TYPE_RENDER_TARGET) {
			vkCheck(CreateImageView(image.image, format, viewType, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers, image.renderTargetView));
		}
		// Create Depth Stencil View
		if (desc.type & SV_GFX_IMAGE_TYPE_DEPTH_STENCIL) {
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			
			if (Graphics::FormatHasStencil(desc.format)) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

			vkCheck(CreateImageView(image.image, format, viewType, aspect, desc.layers, image.depthStencilView));
		}
		// Create Shader Resource View
		if (desc.type & SV_GFX_IMAGE_TYPE_SHADER_RESOURCE) {
			vkCheck(CreateImageView(image.image, format, viewType, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers, image.shaderResouceView));

			// Set image info
			image.image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image.image_info.imageView = image.shaderResouceView;
			image.image_info.sampler = VK_NULL_HANDLE;
		}
		
		// Set ID
		image.ID = GetID();

		return true;
	}

	bool Graphics_vk::CreateSampler(Sampler_vk& sampler, const SV_GFX_SAMPLER_DESC& desc)
	{
		VkSamplerCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		create_info.minFilter = ParseFilter(desc.minFilter);
		create_info.magFilter = ParseFilter(desc.magFilter);
		create_info.addressModeU = ParseAddressMode(desc.addressModeU);
		create_info.addressModeV = ParseAddressMode(desc.addressModeV);
		create_info.addressModeW = ParseAddressMode(desc.addressModeW);
		create_info.compareEnable = VK_FALSE;
		create_info.compareOp = VK_COMPARE_OP_NEVER;
		create_info.unnormalizedCoordinates = VK_FALSE;

		// TODO: Border Color
		create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		// TODO: MipMapping
		create_info.mipLodBias;
		create_info.mipmapMode;

		create_info.anisotropyEnable;
		create_info.maxAnisotropy;

		create_info.minLod;
		create_info.maxLod;

		vkCheck(vkCreateSampler(m_Device, &create_info, nullptr, &sampler.sampler));

		sampler.image_info.sampler = sampler.sampler;
		sampler.image_info.imageView = VK_NULL_HANDLE;
		sampler.image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return true;
	}

	bool Graphics_vk::CreateShader(Shader_vk& shader, const SV_GFX_SHADER_DESC& desc)
	{
		if (shader.module != VK_NULL_HANDLE) return true;

		// Get spv filePath
		std::string binPath;
		if (!SV::Graphics::GetShaderBinPath(desc.filePath, SV_GFX_API_VULKAN, binPath)) {
			SV::LogE("Shader not found '%s'", desc.filePath);
		}

		// Get spv bytes
		std::vector<ui8> data;
		{
			SV::BinFile file;
			if (!file.OpenR(binPath.c_str())) {
				SV::LogE("ShaderBin not found '%s'", binPath.c_str());
				return false;
			}

			data.resize(file.GetSize());
			file.Read(data.data(), data.size());
			file.Close();
		}

		// Create ShaderModule
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.flags;
		create_info.codeSize = data.size();
		create_info.pCode = reinterpret_cast<const ui32*>(data.data());

		vkCheck(vkCreateShaderModule(m_Device, &create_info, nullptr, &shader.module));

		shader.sprvCode = std::move(data);

		return true;
	}

	bool Graphics_vk::CreateRenderPass(RenderPass_vk& renderPass, const SV_GFX_RENDERPASS_DESC& desc)
	{
		VkAttachmentDescription attachments[SV_GFX_ATTACHMENTS_COUNT];

		VkAttachmentReference colorAttachments[SV_GFX_RENDER_TARGET_ATT_COUNT];
		VkAttachmentReference depthStencilAttachment;

		VkSubpassDescription subpass_info{};

		bool hasDepthStencil = false;
		ui32 colorIt = 0u;

		for (ui32 i = 0; i < desc.attachments.size(); ++i) {
			const SV_GFX_ATTACHMENT_DESC& attDesc = desc.attachments[i];

			attachments[i] = {};

			// Reference
			switch (attDesc.type)
			{
			case SV_GFX_ATTACHMENT_TYPE_RENDER_TARGET:
				colorAttachments[colorIt].layout = ParseImageLayout(attDesc.layout);
				colorAttachments[colorIt].attachment = i;
				colorIt++;
				break;
			case SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL:
				if (hasDepthStencil) return false;

				depthStencilAttachment.attachment = i;
				depthStencilAttachment.layout = ParseImageLayout(attDesc.layout, attDesc.format);

				hasDepthStencil = true;
				break;
			}

			// Description
			attachments[i].flags = 0u;
			attachments[i].format = ParseFormat(attDesc.format);
			attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[i].loadOp = ParseAttachmentLoadOp(attDesc.loadOp);
			attachments[i].storeOp = ParseAttachmentStoreOp(attDesc.storeOp);
			attachments[i].initialLayout = ParseImageLayout(attDesc.initialLayout);
			attachments[i].finalLayout = ParseImageLayout(attDesc.finalLayout);

			if (attDesc.type == SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL) {
				// TODO: stencil operations
				attachments[desc.attachments.size()].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachments[desc.attachments.size()].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}
		}

		subpass_info.pColorAttachments = colorAttachments;
		subpass_info.colorAttachmentCount = colorIt;
		subpass_info.pDepthStencilAttachment = hasDepthStencil ? &depthStencilAttachment : nullptr;
		subpass_info.flags = 0u;
		subpass_info.inputAttachmentCount = 0u;
		subpass_info.pInputAttachments = nullptr;
		subpass_info.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_info.pPreserveAttachments = nullptr;
		subpass_info.preserveAttachmentCount = 0u;
		subpass_info.pResolveAttachments = nullptr;

		VkRenderPassCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		create_info.pSubpasses = &subpass_info;
		create_info.subpassCount = 1u;
		create_info.attachmentCount = desc.attachments.size();
		create_info.pAttachments = attachments;

		vkCheck(vkCreateRenderPass(m_Device, &create_info, nullptr, &renderPass.renderPass));
		
		return true;
	}

	bool Graphics_vk::CreateGraphicsPipeline(GraphicsPipeline_vk& pipeline, const SV_GFX_GRAPHICS_PIPELINE_DESC& desc)
	{
		pipeline.ID = GetID();

		// Shader Layout
		spirv_cross::ShaderResources sr;

		ui32 samplersCount = 0u;
		ui32 imagesCount = 0u;
		ui32 uniformsCount = 0u;

		if (desc.pVertexShader) {
			Shader_vk& shader = *reinterpret_cast<Shader_vk*>(desc.pVertexShader->GetPtr());
			spirv_cross::Compiler comp(reinterpret_cast<const ui32*>(shader.sprvCode.data()), shader.sprvCode.size() / sizeof(ui32));
			sr = comp.get_shader_resources();

			LoadSpirv_SemanticNames(comp, sr, pipeline.semanticNames);
			LoadSpirv_Samplers(comp, sr, SV_GFX_SHADER_TYPE_VERTEX, pipeline.setLayoutBindings);
			LoadSpirv_Images(comp, sr, SV_GFX_SHADER_TYPE_VERTEX, pipeline.setLayoutBindings);
			LoadSpirv_Uniforms(comp, sr, SV_GFX_SHADER_TYPE_VERTEX, pipeline.setLayoutBindings);
		}

		if (desc.pPixelShader) {
			Shader_vk& shader = *reinterpret_cast<Shader_vk*>(desc.pPixelShader->GetPtr());
			spirv_cross::Compiler comp(reinterpret_cast<const ui32*>(shader.sprvCode.data()), shader.sprvCode.size() / sizeof(ui32));
			sr = comp.get_shader_resources();

			LoadSpirv_Samplers(comp, sr, SV_GFX_SHADER_TYPE_PIXEL, pipeline.setLayoutBindings);
			LoadSpirv_Images(comp, sr, SV_GFX_SHADER_TYPE_PIXEL, pipeline.setLayoutBindings);
			LoadSpirv_Uniforms(comp, sr, SV_GFX_SHADER_TYPE_PIXEL, pipeline.setLayoutBindings);
		}

		// Count and locations
		pipeline.bindingsLocation.resize(pipeline.setLayoutBindings.size());

		for (ui32 i = 0; i < pipeline.setLayoutBindings.size(); ++i) {

			switch (pipeline.setLayoutBindings[i].descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				pipeline.bindingsLocation[pipeline.setLayoutBindings[i].binding] = samplersCount;
				samplersCount++;
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				pipeline.bindingsLocation[pipeline.setLayoutBindings[i].binding] = imagesCount;
				imagesCount++;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				pipeline.bindingsLocation[pipeline.setLayoutBindings[i].binding] = uniformsCount;
				uniformsCount++;
				break;
			}
		}

		// Create Pipeline Layout
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.flags = 0u;
			layoutInfo.bindingCount = pipeline.setLayoutBindings.size();
			layoutInfo.pBindings = pipeline.setLayoutBindings.data();

			vkCheck(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &pipeline.setLayout));

			VkPipelineLayoutCreateInfo layout_info{};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1u;
			layout_info.pSetLayouts = &pipeline.setLayout;
			layout_info.pushConstantRangeCount = 0u;

			vkCheck(vkCreatePipelineLayout(m_Device, &layout_info, nullptr, &pipeline.layout));
		}

		if (pipeline.setLayoutBindings.size() == 0) return true;

		// Create Desc Pool
		{
			VkDescriptorPoolSize poolSizes[3];
			ui32 poolSizesCount = 0u;

			if (imagesCount > 0u) {
				poolSizes[poolSizesCount].descriptorCount = imagesCount * SV_GFX_COMMAND_LIST_COUNT;
				poolSizes[poolSizesCount].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				poolSizesCount++;
			}
			
			if (samplersCount > 0u) {
				poolSizes[poolSizesCount].descriptorCount = samplersCount * SV_GFX_COMMAND_LIST_COUNT;
				poolSizes[poolSizesCount].type = VK_DESCRIPTOR_TYPE_SAMPLER;
				poolSizesCount++;
			}

			if (uniformsCount > 0u) {
				poolSizes[poolSizesCount].descriptorCount = uniformsCount * SV_GFX_COMMAND_LIST_COUNT;
				poolSizes[poolSizesCount].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolSizesCount++;
			}

			VkDescriptorPoolCreateInfo descPool_info{};
			descPool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descPool_info.flags = 0u;
			descPool_info.maxSets = SV_GFX_COMMAND_LIST_COUNT;
			descPool_info.poolSizeCount = poolSizesCount;
			descPool_info.pPoolSizes = poolSizes;
			vkCheck(vkCreateDescriptorPool(m_Device, &descPool_info, nullptr, &pipeline.descriptorPool));
		}

		// Allocate Desc Sets
		{
			auto& descSets = pipeline.descriptorSets;

			descSets.resize(SV_GFX_COMMAND_LIST_COUNT);

			VkDescriptorSetLayout setLayouts[SV_GFX_COMMAND_LIST_COUNT];
			for (ui64 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i) setLayouts[i] = pipeline.setLayout;

			VkDescriptorSetAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = pipeline.descriptorPool;
			alloc_info.descriptorSetCount = SV_GFX_COMMAND_LIST_COUNT;
			alloc_info.pSetLayouts = setLayouts;

			vkCheck(vkAllocateDescriptorSets(m_Device, &alloc_info, descSets.data()));
		}

		// Fill Write Desc strucs
		{
			pipeline.writeDescriptors.resize(pipeline.setLayoutBindings.size());
			auto& writeDesc = pipeline.writeDescriptors;
			for (ui32 i = 0; i < writeDesc.size(); ++i) {
				auto& binding = pipeline.setLayoutBindings[i];

				writeDesc[i] = {};
				writeDesc[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDesc[i].dstBinding = binding.binding;
				writeDesc[i].dstArrayElement = 0u;
				writeDesc[i].descriptorCount = 1u;
				writeDesc[i].descriptorType = binding.descriptorType;
			}
		}

		return true;
	}

	///////////////////////////////////////////////////// DESTRUCTION ///////////////////////////////////////////////////////

	bool Graphics_vk::DestroyBuffer(Buffer_vk& buffer)
	{
		vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);

		if (buffer.stagingBuffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(m_Allocator, buffer.stagingBuffer, buffer.stagingAllocation);
		}

		return true;
	}
	bool Graphics_vk::DestroyImage(Image_vk& image)
	{
		vmaDestroyImage(m_Allocator, image.image, image.allocation);
		if (image.renderTargetView != VK_NULL_HANDLE)	vkDestroyImageView(m_Device, image.renderTargetView, nullptr);
		if (image.depthStencilView != VK_NULL_HANDLE)	vkDestroyImageView(m_Device, image.depthStencilView, nullptr);
		if (image.shaderResouceView != VK_NULL_HANDLE)	vkDestroyImageView(m_Device, image.shaderResouceView, nullptr);
		return true;
	}
	bool Graphics_vk::DestroySampler(Sampler_vk& sampler)
	{
		vkDestroySampler(m_Device, sampler.sampler, nullptr);
		return true;
	}
	bool Graphics_vk::DestroyShader(Shader_vk& shader)
	{
		vkDestroyShaderModule(m_Device, shader.module, nullptr);
		return true;
	}
	bool Graphics_vk::DestroyRenderPass(RenderPass_vk& renderPass)
	{
		vkDestroyRenderPass(m_Device, renderPass.renderPass, nullptr);
		for (auto& it : renderPass.frameBuffers) {
			vkDestroyFramebuffer(m_Device, it.second, nullptr);
		}
		return true;
	}
	bool Graphics_vk::DestroyGraphicsPipeline(GraphicsPipeline_vk& graphicsPipeline)
	{
		for (auto& it : graphicsPipeline.pipelines) {
			vkDestroyPipeline(m_Device, it.second, nullptr);
		}
		vkDestroyPipelineLayout(m_Device, graphicsPipeline.layout, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, graphicsPipeline.setLayout, nullptr);
		vkDestroyDescriptorPool(m_Device, graphicsPipeline.descriptorPool, nullptr);
		return true;
	}

	//////////////////////////////////////// DEVICE ////////////////////////////////////////
	void Graphics_vk::ResizeSwapChain()
	{
		vkAssert(vkDeviceWaitIdle(m_Device));
		VkSwapchainKHR old = m_SwapChain.swapChain;
		SV_ASSERT(DestroySwapChain(false));
		SV_ASSERT(CreateSwapChain(old));
		vkDestroySwapchainKHR(m_Device, old, nullptr);
	}
	SV::Image& Graphics_vk::GetSwapChainBackBuffer()
	{
		m_SwapChain.backBuffer.image = m_SwapChain.images[m_ImageIndex].image;
		m_SwapChain.backBuffer.renderTargetView = m_SwapChain.images[m_ImageIndex].view;
		m_SwapChain.backBuffer.ID = m_SwapChain.images[m_ImageIndex].ID;

		SV_GFX_IMAGE_DESC bbDesc{};
		bbDesc.width = m_SwapChain.currentExtent.width;
		bbDesc.height = m_SwapChain.currentExtent.height;
		bbDesc.depth = 1u;
		bbDesc.layers = 1u;
		bbDesc.dimension = 2u;
		bbDesc.format = ParseFormat(m_SwapChain.currentFormat);

		m_SwapChain.backBuffer.SetType(SV_GFX_PRIMITIVE_IMAGE);
		m_SwapChain.backBuffer.SetDescription(bbDesc);

		m_SwapChain.backBufferImage = SV::Primitive(&m_SwapChain.backBuffer);
		return *reinterpret_cast<SV::Image*>(&m_SwapChain.backBufferImage);
	}

	void Graphics_vk::BeginFrame()
	{
		Frame& frame = GetFrame();

		vkAssert(vkWaitForFences(m_Device, 1, &frame.fence, VK_TRUE, UINT64_MAX));

		vkAssert(vkResetCommandPool(m_Device, frame.commandPool, 0u));

		vkAssert(vkAcquireNextImageKHR(m_Device, m_SwapChain.swapChain, UINT64_MAX, m_SwapChain.semAcquireImage, VK_NULL_HANDLE, &m_ImageIndex));
	}
	void Graphics_vk::SubmitCommandLists()
	{
		Frame& frame = GetFrame();

		// End CommandBuffers & RenderPasses
		for (ui32 i = 0; i < m_ActiveCMDCount; ++i) {
			if(m_ActiveRenderPass[i]) vkCmdEndRenderPass(GetCMD(i));
			vkAssert(vkEndCommandBuffer(GetCMD(i)));
		}

		if (m_ImageFences[m_ImageIndex] != VK_NULL_HANDLE) {
			vkAssert(vkWaitForFences(m_Device, 1u, &m_ImageFences[m_ImageIndex], VK_TRUE, UINT64_MAX));
		}
		m_ImageFences[m_ImageIndex] = frame.fence;

		vkAssert(vkResetFences(m_Device, 1u, &frame.fence));

		VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1u;
		submit_info.pWaitSemaphores = &m_SwapChain.semAcquireImage;
		submit_info.pWaitDstStageMask = &waitDstStage;
		submit_info.commandBufferCount = m_ActiveCMDCount;
		submit_info.pCommandBuffers = frame.commandBuffers;
		submit_info.signalSemaphoreCount = 1u;
		submit_info.pSignalSemaphores = &m_SwapChain.semPresent;

		m_ActiveCMDCount = 0u;
		svZeroMemory(m_ActiveRenderPass, sizeof(bool) * SV_GFX_COMMAND_LIST_COUNT);

		vkAssert(vkQueueSubmit(m_QueueGraphics, 1u, &submit_info, frame.fence));
	}
	void Graphics_vk::Present()
	{
		VkCommandBuffer cmd;
		vkAssert(BeginSingleTimeCMD(&cmd));
		ImageMemoryBarrier(cmd, m_SwapChain.backBuffer.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 
			VK_IMAGE_ASPECT_COLOR_BIT, 1u);
		vkAssert(EndSingleTimeCMD(cmd));

		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1u;
		present_info.pWaitSemaphores = &m_SwapChain.semPresent;
		present_info.swapchainCount = 1u;
		present_info.pSwapchains = &m_SwapChain.swapChain;
		present_info.pImageIndices = &m_ImageIndex;
		present_info.pResults = nullptr;

		vkAssert(vkQueuePresentKHR(m_QueueGraphics, &present_info));

		m_CurrentFrame++;
		if (m_CurrentFrame == m_FrameCount) m_CurrentFrame = 0u;
	}

	void Graphics_vk::Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, SV::CommandList cmd)
	{
		UpdateGraphicsState(cmd);
		vkCmdDraw(GetCMD(cmd), vertexCount, instanceCount, startVertex, startInstance);
	}
	void Graphics_vk::DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, SV::CommandList cmd)
	{
		UpdateGraphicsState(cmd);
		vkCmdDrawIndexed(GetCMD(cmd), indexCount, instanceCount, startIndex, startVertex, startInstance);
	}

	void Graphics_vk::UpdateBuffer(SV::Buffer& buffer_, void* pData, ui32 size, ui32 offset, SV::CommandList cmd_)
	{
		Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(buffer_.GetPtr());

		if (buffer.GetUsage() == SV_GFX_USAGE_DEFAULT && buffer.GetCPUAccess() == SV_GFX_CPU_ACCESS_WRITE) {
			VkCommandBuffer cmd = GetCMD(cmd_);

			// Memory Barrier

			memcpy(buffer.stagingAllocation->GetMappedData(), pData, ui64(size));
			CopyBuffer(cmd, buffer.stagingBuffer, buffer.buffer, 0u, VkDeviceSize(offset), VkDeviceSize(size));

			// Memory Barrier
		}
		else {
			ui8* dst;
			vmaMapMemory(m_Allocator, buffer.allocation, (void**)&dst);
			memcpy(dst + ui64(offset), pData, ui64(size));
			vmaUnmapMemory(m_Allocator, buffer.allocation);
		}
	}

	//////////////////////////////////////// CONSTRUCTOR & DESTRUCTOR ////////////////////////////////////////
	void* VulkanConstructor(SV_GFX_PRIMITIVE type, const void* desc)
	{
		Graphics_vk& gfx = *reinterpret_cast<Graphics_vk*>(Graphics::_internal::GetDevice());
		void* ptr = nullptr;

		switch (type)
		{

		case SV_GFX_PRIMITIVE_BUFFER:
		{
			Buffer_vk* b = new Buffer_vk();
			if (!gfx.CreateBuffer(*b, *reinterpret_cast<const SV_GFX_BUFFER_DESC*>(desc))) {
				delete b;
				b = nullptr;
			}
			ptr = b;
		}
		break;

		case SV_GFX_PRIMITIVE_SHADER:
		{
			Shader_vk* s = new Shader_vk();
			if (!gfx.CreateShader(*s, *reinterpret_cast<const SV_GFX_SHADER_DESC*>(desc))) {
				delete s;
				s = nullptr;
			}
			ptr = s;
		}
		break;

		case SV_GFX_PRIMITIVE_IMAGE:
		{
			Image_vk* i = new Image_vk();
			if (!gfx.CreateImage(*i, *reinterpret_cast<const SV_GFX_IMAGE_DESC*>(desc))) {
				delete i;
				i = nullptr;
			}
			ptr = i;
		}
		break;

		case SV_GFX_PRIMITIVE_SAMPLER:
		{
			Sampler_vk* i = new Sampler_vk();
			if (!gfx.CreateSampler(*i, *reinterpret_cast<const SV_GFX_SAMPLER_DESC*>(desc))) {
				delete i;
				i = nullptr;
			}
			ptr = i;
		}
		break;

		case SV_GFX_PRIMITIVE_RENDERPASS:
		{
			RenderPass_vk* rp = new RenderPass_vk();
			if (!gfx.CreateRenderPass(*rp, *reinterpret_cast<const SV_GFX_RENDERPASS_DESC*>(desc))) {
				delete rp;
				rp = nullptr;
			}
			ptr = rp;
		}
		break;

		case SV_GFX_PRIMITIVE_GRAPHICS_PIPELINE:
		{
			GraphicsPipeline_vk* gp = new GraphicsPipeline_vk();
			if (!gfx.CreateGraphicsPipeline(*gp, *reinterpret_cast<const SV_GFX_GRAPHICS_PIPELINE_DESC*>(desc))) {
				delete gp;
				gp = nullptr;
			}
			ptr = gp;
		}
		break;

		}

		return ptr;
	}
	bool VulkanDestructor(Primitive& primitive)
	{
		Graphics_vk& gfx = *reinterpret_cast<Graphics_vk*>(Graphics::_internal::GetDevice());

		bool result = false;

		switch (primitive->GetType())
		{

		case SV_GFX_PRIMITIVE_BUFFER:
		{
			Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(primitive.GetPtr());
			result = gfx.DestroyBuffer(buffer);
			buffer.~Buffer_vk();
			break;
		}

		case SV_GFX_PRIMITIVE_SHADER:
		{
			Shader_vk& shader = *reinterpret_cast<Shader_vk*>(primitive.GetPtr());
			result = gfx.DestroyShader(shader);
			shader.~Shader_vk();
			break;
		}

		case SV_GFX_PRIMITIVE_IMAGE:
		{
			Image_vk& image = *reinterpret_cast<Image_vk*>(primitive.GetPtr());
			result = gfx.DestroyImage(image);
			image.~Image_vk();
			break;
		}

		case SV_GFX_PRIMITIVE_SAMPLER:
		{
			Sampler_vk& sampler = *reinterpret_cast<Sampler_vk*>(primitive.GetPtr());
			result = gfx.DestroySampler(sampler);
			sampler.~Sampler_vk();
			break;
		}

		case SV_GFX_PRIMITIVE_RENDERPASS:
		{
			RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(primitive.GetPtr());
			result = gfx.DestroyRenderPass(renderPass);
			renderPass.~RenderPass_vk();
			break;
		}

		case SV_GFX_PRIMITIVE_GRAPHICS_PIPELINE:
		{
			GraphicsPipeline_vk& pipeline = *reinterpret_cast<GraphicsPipeline_vk*>(primitive.GetPtr());
			result = gfx.DestroyGraphicsPipeline(pipeline);
			pipeline.~GraphicsPipeline_vk();
			break;
		}

		default:
			return true;

		}

		delete primitive.GetPtr();
		return result;
	}

}