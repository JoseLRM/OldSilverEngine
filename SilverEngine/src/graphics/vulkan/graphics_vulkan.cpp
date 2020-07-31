#include "core.h"

#include "Engine.h"
#include "graphics/graphics_state.h"

#include "graphics_vulkan.h"
#include "graphics/graphics_properties.h"

using namespace sv;

namespace _sv {

	Graphics_vk& graphics_vulkan_device_get()
	{
		return *reinterpret_cast<Graphics_vk*>(graphics_device_get());
	}

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
			sv::log("[VULKAN VERBOSE] %s\n", data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			sv::log("[VULKAN WARNING] %s\n", data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			sv::log("[VULKAN ERROR] %s\n", data->pMessage);
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

		// Destroy Pipelines
		for (auto& it : m_Pipelines) {
			auto& pipeline = it.second;

			vkDestroyPipelineLayout(m_Device, pipeline.layout, nullptr);
			
			pipeline.descriptors.Clear();

			for (auto& it0 : pipeline.pipelines) {
				vkDestroyPipeline(m_Device, it0.second, nullptr);
			}
		}
		m_Pipelines.clear();

		// Destroy frames
		for (ui32 i = 0; i < m_FrameCount; ++i) {
			Frame& frame = m_Frames[i];
			vkDestroyCommandPool(m_Device, frame.commandPool, nullptr);
			vkDestroyCommandPool(m_Device, frame.transientCommandPool, nullptr);
			vkDestroyFence(m_Device, frame.fence, nullptr);
			frame.memory.Clear();
		}

		// Destroy Allocator
		vmaDestroyAllocator(m_Allocator);

		// Destroy SwapChain
		svCheck(DestroySwapChain());

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
		_sv::graphics_allocator_get().SetFunctions(VulkanConstructor, VulkanDestructor);
	}

	void Graphics_vk::SetProperties()
	{
		sv::GraphicsProperties props;

		props.transposedMatrices = false;

		_sv::graphics_properties_set(props);
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
					sv::log_error("InstanceExtension '%s' not found", m_Extensions[i]);
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
					sv::log_error("InstanceValidationLayer '%s' not found", m_ValidationLayers[i]);
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
			std::unique_ptr<Adapter> adapter = std::make_unique<Adapter_vk>(devices[i]);
			if(adapter->GetSuitability() > 0)
				_sv::graphics_adapter_add(std::move(adapter));
		}

		// Select the most suitable
		ui32 maxSuitability = 0;
		ui32 index = count;

		const auto& adapters = graphics_adapter_get_list();

		for (ui32 i = 0; i < adapters.size(); ++i) {
			Adapter_vk& adapter = *reinterpret_cast<Adapter_vk*>(adapters[i].get());
			if (adapter.GetSuitability() > maxSuitability) index = i;
		}

		// Adapter not found
		if (index == count) {
			sv::log_error("Can't find valid adapters for Vulkan");
			return false;
		}

		graphics_adapter_set(index);

		return true;
	}

	bool Graphics_vk::CreateLogicalDevice()
	{
		const Adapter_vk& adapter = *reinterpret_cast<const Adapter_vk*>(graphics_adapter_get());
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
		Adapter_vk& adapter = *reinterpret_cast<Adapter_vk*>(graphics_adapter_get());

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
		cmdPool_info.queueFamilyIndex = reinterpret_cast<Adapter_vk*>(graphics_adapter_get())->GetFamilyIndex().graphics;

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
		const Adapter_vk& adapter = *reinterpret_cast<const Adapter_vk*>(graphics_adapter_get());
		VkPhysicalDevice physicalDevice = adapter.GetPhysicalDevice();

		WindowHandle hWnd = window_get_handle();
		ui32 width = window_get_width();
		ui32 height = window_get_height();

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
				sv::log_error("This adapter don't support vulkan swapChain");
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
				//if (m_SwapChain.presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
				//if (m_SwapChain.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
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

	void Graphics_vk::UpdateGraphicsState(CommandList cmd_)
	{
		_sv::GraphicsPipelineState& state = _sv::graphics_state_get().graphics[cmd_];

		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);
		GraphicsPipeline_vk& svPipeline = *reinterpret_cast<GraphicsPipeline_vk*>(state.pipeline);
		size_t pipelineHash = svPipeline.hash;
		VulkanPipeline& pipeline = m_Pipelines[pipelineHash];

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
			vkCmdBindIndexBuffer(cmd, reinterpret_cast<Buffer_vk*>(state.indexBuffer)->buffer, state.indexBufferOffset, graphics_vulkan_parse_indextype(state.indexBuffer->GetIndexType()));
		}

		// Bind RenderPass
		if (state.flags & SV_GFX_GRAPHICS_PIPELINE_STATE_RENDER_PASS && m_ActiveRenderPass[cmd_] != renderPass.renderPass) {
			updatePipeline = true;
		}

		// Bind Pipeline
		if (updatePipeline) {

			// Set active renderpass
			m_ActiveRenderPass[cmd_] = renderPass.renderPass;

			// Compute hash
			size_t hash = pipelineHash;
			sv::utils_hash_combine(hash, renderPass.renderPass);

			// Find Pipeline
			VkPipeline vkPipeline = VK_NULL_HANDLE;

			pipeline.mutex.lock();
			auto it = pipeline.pipelines.find(hash);
			if (it == pipeline.pipelines.end()) {

				// Shader Stages
				VkPipelineShaderStageCreateInfo shaderStages[SV_GFX_SHADER_TYPE_GFX_COUNT] = {};
				ui32 shaderStagesCount = 0u;

				if (svPipeline.GetVertexShader()) {
					VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
					stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					stage.flags = 0u;
					stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
					stage.module = reinterpret_cast<Shader_vk*>(svPipeline.GetVertexShader())->module;
					stage.pName = "main";
					stage.pSpecializationInfo = nullptr;
				}
				if (svPipeline.GetPixelShader()) {
					VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
					stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					stage.flags = 0u;
					stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
					stage.module = reinterpret_cast<Shader_vk*>(svPipeline.GetPixelShader())->module;
					stage.pName = "main";
					stage.pSpecializationInfo = nullptr;
				}
				if (svPipeline.GetGeometryShader()) {
					VkPipelineShaderStageCreateInfo& stage = shaderStages[shaderStagesCount++];
					stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					stage.flags = 0u;
					stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
					stage.module = reinterpret_cast<Shader_vk*>(svPipeline.GetGeometryShader())->module;
					stage.pName = "main";
					stage.pSpecializationInfo = nullptr;
				}

				// Input Layout
				VkPipelineVertexInputStateCreateInfo vertexInput{};
				vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				
				VkVertexInputBindingDescription bindings[SV_GFX_INPUT_SLOT_COUNT];
				VkVertexInputAttributeDescription attributes[SV_GFX_INPUT_ELEMENT_COUNT];
				{
					SV_GFX_INPUT_LAYOUT_DESC il = svPipeline.GetInputLayout();
					for (ui32 i = 0; i < il.slots.size(); ++i) {
						bindings[i].binding = il.slots[i].slot;
						bindings[i].inputRate = il.slots[i].instanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
						bindings[i].stride = il.slots[i].stride;
					}
					for (ui32 i = 0; i < il.elements.size(); ++i) {
						attributes[i].binding = il.elements[i].inputSlot;
						attributes[i].format = graphics_vulkan_parse_format(il.elements[i].format);
						attributes[i].location = pipeline.semanticNames[il.elements[i].name] + il.elements[i].index;
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
					const SV_GFX_RASTERIZER_STATE_DESC& rDesc = svPipeline.GetRasterizerState();

					rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
					rasterizerState.flags = 0u;
					rasterizerState.depthClampEnable = VK_FALSE;
					rasterizerState.rasterizerDiscardEnable = VK_FALSE;
					rasterizerState.polygonMode = rDesc.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
					rasterizerState.cullMode = graphics_vulkan_parse_cullmode(rDesc.cullMode);
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
					const SV_GFX_BLEND_STATE_DESC& bDesc = svPipeline.GetBlendState();
					
					for (ui32 i = 0; i < bDesc.attachments.size(); ++i)
					{
						const SV_GFX_BLEND_ATTACHMENT_DESC& b = bDesc.attachments[i];

						attachments[i].blendEnable = b.blendEnabled ? VK_TRUE : VK_FALSE;
						attachments[i].srcColorBlendFactor = graphics_vulkan_parse_blendfactor(b.srcColorBlendFactor);
						attachments[i].dstColorBlendFactor = graphics_vulkan_parse_blendfactor(b.dstColorBlendFactor);;
						attachments[i].colorBlendOp = graphics_vulkan_parse_blendop(b.colorBlendOp);
						attachments[i].srcAlphaBlendFactor = graphics_vulkan_parse_blendfactor(b.srcAlphaBlendFactor);;
						attachments[i].dstAlphaBlendFactor = graphics_vulkan_parse_blendfactor(b.dstAlphaBlendFactor);;
						attachments[i].alphaBlendOp = graphics_vulkan_parse_blendop(b.alphaBlendOp);;
						attachments[i].colorWriteMask = graphics_vulkan_parse_colorcomponent(b.colorWriteMask);
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
					const SV_GFX_DEPTHSTENCIL_STATE_DESC& dDesc = svPipeline.GetDepthStencilState();
					depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
					depthStencilState.flags = 0u;
					depthStencilState.depthTestEnable = dDesc.depthTestEnabled;
					depthStencilState.depthWriteEnable = dDesc.depthWriteEnabled;
					depthStencilState.depthCompareOp = graphics_vulkan_parse_compareop(dDesc.depthCompareOp);
					
					depthStencilState.stencilTestEnable = dDesc.stencilTestEnabled;
					depthStencilState.front.failOp = graphics_vulkan_parse_stencilop(dDesc.front.failOp);
					depthStencilState.front.passOp = graphics_vulkan_parse_stencilop(dDesc.front.passOp);
					depthStencilState.front.depthFailOp = graphics_vulkan_parse_stencilop(dDesc.front.depthFailOp);
					depthStencilState.front.compareOp = graphics_vulkan_parse_compareop(dDesc.front.compareOp);
					depthStencilState.front.compareMask = dDesc.front.compareMask;
					depthStencilState.front.writeMask = dDesc.front.writeMask;
					depthStencilState.front.reference = dDesc.front.reference;
					depthStencilState.back.failOp = graphics_vulkan_parse_stencilop(dDesc.back.failOp);
					depthStencilState.back.passOp = graphics_vulkan_parse_stencilop(dDesc.back.passOp);
					depthStencilState.back.depthFailOp = graphics_vulkan_parse_stencilop(dDesc.back.depthFailOp);
					depthStencilState.back.compareOp = graphics_vulkan_parse_compareop(dDesc.back.compareOp);
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
				inputAssembly.topology = graphics_vulkan_parse_topology(svPipeline.GetTopology());

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
				const Viewport& vp = state.viewports[i];

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
				const Scissor& sc = state.scissors[i];

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

			VkDescriptorSet descSet = pipeline.descriptors.GetDescriptorSet(m_CurrentFrame, cmd_);

			VkWriteDescriptorSet writeDesc[SV_GFX_CONSTANT_BUFFER_COUNT + SV_GFX_IMAGE_COUNT + SV_GFX_SAMPLER_COUNT];

			for (ui32 i = 0; i < pipeline.bindings.size(); ++i) {

				auto& binding = pipeline.bindings[i];

				writeDesc[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDesc[i].pNext = nullptr;
				writeDesc[i].dstSet = descSet;
				writeDesc[i].dstBinding = binding.binding;
				writeDesc[i].dstArrayElement = 0u;
				writeDesc[i].descriptorCount = 1u;

				switch (binding.descriptorType)
				{
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				{
					Sampler_vk& sampler = *reinterpret_cast<Sampler_vk*>(state.sampers[graphics_vulkan_parse_shadertype(binding.stageFlags)][pipeline.bindingsLocation[binding.binding]]);
					writeDesc[i].pImageInfo = &sampler.image_info;
					writeDesc[i].pBufferInfo = nullptr;
					writeDesc[i].pTexelBufferView = nullptr;
					writeDesc[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				}
					break;

				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				{
					Image_vk& image = *reinterpret_cast<Image_vk*>(state.images[graphics_vulkan_parse_shadertype(binding.stageFlags)][pipeline.bindingsLocation[binding.binding]]);
					writeDesc[i].pImageInfo = &image.image_info;
					writeDesc[i].pBufferInfo = nullptr;
					writeDesc[i].pTexelBufferView = nullptr;
					writeDesc[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				}
					break;

				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(state.constantBuffers[graphics_vulkan_parse_shadertype(binding.stageFlags)][pipeline.bindingsLocation[binding.binding]]);
					writeDesc[i].pImageInfo = nullptr;
					writeDesc[i].pBufferInfo = &buffer.buffer_info;
					writeDesc[i].pTexelBufferView = nullptr;
					writeDesc[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				}
				break;
				}

			}

			vkUpdateDescriptorSets(m_Device, pipeline.bindings.size(), writeDesc, 0u, nullptr);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0u, 1u, &descSet, 0u, nullptr);
		}

		state.flags = 0u;
	}

	/////////////////////////////////////////////// GETTERS ////////////////////////////////////////////////////
	
	VkPhysicalDevice Graphics_vk::GetPhysicalDevice() const noexcept
	{
		return reinterpret_cast<Adapter_vk*>(graphics_adapter_get())->GetPhysicalDevice();
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

	sv::CommandList Graphics_vk::GetLastCommandList()
	{
		std::lock_guard<std::mutex> lock(m_MutexCMD);
		SV_ASSERT(m_ActiveCMDCount != 0);
		return m_ActiveCMDCount - 1u;
	}

	void Graphics_vk::BeginRenderPass(sv::CommandList cmd_)
	{
		_sv::GraphicsPipelineState& state = _sv::graphics_state_get().graphics[cmd_];
		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);
		VkCommandBuffer cmd = GetCMD(cmd_);

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
			sv::utils_hash_combine(hash, renderPass.GetAttachmentsCount());
			for (ui32 i = 0; i < renderPass.GetAttachmentsCount(); ++i) {
				Image_vk& att = *reinterpret_cast<Image_vk*>(state.attachments[i]);
				sv::utils_hash_combine(hash, att.ID);
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
	void Graphics_vk::EndRenderPass(sv::CommandList cmd_)
	{
		_sv::GraphicsPipelineState& state = _sv::graphics_state_get().graphics[cmd_];
		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);
		VkCommandBuffer cmd = GetCMD(cmd_);

		vkCmdEndRenderPass(cmd);
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
				sv::log_error("Can't allocate SingleTime CommandBuffer");
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
				sv::log_error("Can't begin SingleTime CommandBuffer");
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

	void Graphics_vk::CopyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
	{
		VkBufferCopy copy_info{};
		copy_info.dstOffset = dstOffset;
		copy_info.srcOffset = srcOffset;
		copy_info.size = size;
		vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1u, &copy_info);
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
			b.stageFlags = graphics_vulkan_parse_shadertype(shaderType);
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
			b.stageFlags = graphics_vulkan_parse_shadertype(shaderType);
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
			b.stageFlags = graphics_vulkan_parse_shadertype(shaderType);
			b.pImmutableSamplers = nullptr;
		}
	}

	/////////////////////////////////////////////////// CREATION ///////////////////////////////////////////////////

	bool Graphics_vk::CreateBuffer(Buffer_vk& buffer, const SV_GFX_BUFFER_DESC& desc)
	{
		VkBufferUsageFlags bufferUsage = 0u;
		bool deviceMemory = true;
		VmaMemoryUsage memoryUsage = deviceMemory ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU;

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

		if (deviceMemory) bufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

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
			//alloc_info.usage = memoryUsage;
			alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			vkCheck(vmaCreateBuffer(m_Allocator, &buffer_info, &alloc_info, &buffer.buffer, &buffer.allocation, nullptr));
		}

		// Set data
		if (desc.pData) {
			if (deviceMemory) {
				VkCommandBuffer cmd;
				vkCheck(BeginSingleTimeCMD(&cmd));

				VkBuffer stagingBuffer;
				VmaAllocation stagingAllocation;
				void* mapData;

				vkCheck(graphics_vulkan_memory_create_stagingbuffer(stagingBuffer, stagingAllocation, &mapData, desc.size));
				memcpy(mapData, desc.pData, desc.size);

				VkBufferMemoryBarrier bufferBarrier{};

				bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				bufferBarrier.srcAccessMask = 0;
				bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				bufferBarrier.buffer = buffer.buffer;
				bufferBarrier.offset = 0u;
				bufferBarrier.size = VK_WHOLE_SIZE;

				vkCmdPipelineBarrier(cmd,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0u,
					0u,
					nullptr,
					1u,
					&bufferBarrier,
					0u,
					nullptr);

				CopyBuffer(cmd, stagingBuffer, buffer.buffer, 0u, 0u, desc.size);

				bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				bufferBarrier.dstAccessMask = 0u;

				switch (desc.bufferType)
				{
				case SV_GFX_BUFFER_TYPE_VERTEX:
					bufferBarrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
					break;
				case SV_GFX_BUFFER_TYPE_INDEX:
					bufferBarrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
					break;
				case SV_GFX_BUFFER_TYPE_CONSTANT:
					bufferBarrier.dstAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
					break;
				}

				vkCmdPipelineBarrier(cmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
					0u,
					0u,
					nullptr,
					1u,
					&bufferBarrier,
					0u,
					nullptr);

				vkCheck(EndSingleTimeCMD(cmd));

				vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingAllocation);
				
			}
			else {
				void* d;
				vmaMapMemory(m_Allocator, buffer.allocation, &d);
				memcpy(d, desc.pData, desc.size);
				vmaUnmapMemory(m_Allocator, buffer.allocation);
			}
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
		VkFormat format = graphics_vulkan_parse_format(desc.format);
		VkImageUsageFlags imageUsage = 0u;
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		
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
			alloc_info.requiredFlags = VMA_MEMORY_USAGE_GPU_ONLY;
			
			vkCheck(vmaCreateImage(m_Allocator, &create_info, &alloc_info, &image.image, &image.allocation, nullptr));
		}

		// Set Data
		VkCommandBuffer cmd;

		vkCheck(BeginSingleTimeCMD(&cmd));
		
		if (desc.pData) {

			VkImageAspectFlags aspect = graphics_vulkan_aspect_from_image_layout(desc.layout, desc.format);

			// Memory barrier to set the image transfer dst layout
			VkImageMemoryBarrier memBarrier{};
			memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memBarrier.srcAccessMask = graphics_vulkan_access_from_image_layout(SV_GFX_IMAGE_LAYOUT_UNDEFINED);
			memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memBarrier.oldLayout = graphics_vulkan_parse_image_layout(SV_GFX_IMAGE_LAYOUT_UNDEFINED);
			memBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.image = image.image;
			memBarrier.subresourceRange.aspectMask = aspect;
			memBarrier.subresourceRange.layerCount = desc.layers;
			memBarrier.subresourceRange.levelCount = 1u;

			vkCmdPipelineBarrier(cmd,
				graphics_vulkan_stage_from_image_layout(SV_GFX_IMAGE_LAYOUT_UNDEFINED),
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0u,
				0u,
				nullptr,
				0u,
				nullptr,
				1u,
				&memBarrier);

			// Create staging buffer and copy desc.pData
			VkBuffer stagingBuffer;
			VmaAllocation stagingAllocation;
			void* mapData;

			graphics_vulkan_memory_create_stagingbuffer(stagingBuffer, stagingAllocation, &mapData, desc.size);
			memcpy(mapData, desc.pData, desc.size);

			// Copy buffer to image
			VkBufferImageCopy copy_info{};
			copy_info.bufferOffset = 0u;
			copy_info.bufferRowLength = 0u;
			copy_info.bufferImageHeight = 0u;
			copy_info.imageSubresource.aspectMask = aspect;
			copy_info.imageSubresource.baseArrayLayer = 0u;
			copy_info.imageSubresource.layerCount = desc.layers;
			copy_info.imageSubresource.mipLevel = 0u;
			copy_info.imageOffset = { 0, 0, 0 };
			copy_info.imageExtent = { desc.width, desc.height, desc.depth };

			vkCmdCopyBufferToImage(cmd, stagingBuffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy_info);
			
			// Set the layout to desc.layout
			memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memBarrier.dstAccessMask = graphics_vulkan_access_from_image_layout(desc.layout);
			memBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memBarrier.newLayout = graphics_vulkan_parse_image_layout(desc.layout);

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				graphics_vulkan_stage_from_image_layout(desc.layout),
				0u,
				0u,
				nullptr,
				0u,
				nullptr,
				1u,
				&memBarrier);

			vkCheck(EndSingleTimeCMD(cmd));

			// Destroy staging buffer
			vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingAllocation);
		}
		else {

			VkImageMemoryBarrier memBarrier{};
			memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memBarrier.srcAccessMask = graphics_vulkan_access_from_image_layout(SV_GFX_IMAGE_LAYOUT_UNDEFINED);
			memBarrier.dstAccessMask = graphics_vulkan_access_from_image_layout(desc.layout);
			memBarrier.oldLayout = graphics_vulkan_parse_image_layout(SV_GFX_IMAGE_LAYOUT_UNDEFINED);
			memBarrier.newLayout = graphics_vulkan_parse_image_layout(desc.layout);
			memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.image = image.image;
			memBarrier.subresourceRange.aspectMask = graphics_vulkan_aspect_from_image_layout(desc.layout, desc.format);
			memBarrier.subresourceRange.layerCount = desc.layers;
			memBarrier.subresourceRange.levelCount = 1u;

			vkCmdPipelineBarrier(cmd,
				graphics_vulkan_stage_from_image_layout(SV_GFX_IMAGE_LAYOUT_UNDEFINED),
				graphics_vulkan_stage_from_image_layout(desc.layout),
				0u,
				0u,
				nullptr,
				0u,
				nullptr,
				1u,
				&memBarrier);

			vkCheck(EndSingleTimeCMD(cmd));
		}

		// TODO: MipMapping

		// Create Render Target View
		if (desc.type & SV_GFX_IMAGE_TYPE_RENDER_TARGET) {
			vkCheck(CreateImageView(image.image, format, viewType, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers, image.renderTargetView));
		}
		// Create Depth Stencil View
		if (desc.type & SV_GFX_IMAGE_TYPE_DEPTH_STENCIL) {
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			
			if (graphics_format_has_stencil(desc.format)) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

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

		create_info.minFilter = graphics_vulkan_parse_filter(desc.minFilter);
		create_info.magFilter = graphics_vulkan_parse_filter(desc.magFilter);
		create_info.addressModeU = graphics_vulkan_parse_addressmode(desc.addressModeU);
		create_info.addressModeV = graphics_vulkan_parse_addressmode(desc.addressModeV);
		create_info.addressModeW = graphics_vulkan_parse_addressmode(desc.addressModeW);
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
		svCheck(graphics_shader_binpath(desc.filePath, SV_GFX_API_VULKAN, binPath));

		// Get spv bytes
		std::vector<ui8> data;
		{
			sv::BinFile file;
			if (!file.OpenR(binPath.c_str())) {
				sv::log_error("ShaderBin not found '%s'", binPath.c_str());
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

		// Get Layout from sprv code
		spirv_cross::Compiler comp(reinterpret_cast<const ui32*>(data.data()), data.size() / sizeof(ui32));
		spirv_cross::ShaderResources sr = comp.get_shader_resources();

		// Semantic Names
		if (desc.shaderType == SV_GFX_SHADER_TYPE_VERTEX) {
			LoadSpirv_SemanticNames(comp, sr, shader.semanticNames);
		}

		// Bindings
		LoadSpirv_Samplers(comp, sr, desc.shaderType, shader.bindings);
		LoadSpirv_Images(comp, sr, desc.shaderType, shader.bindings);
		LoadSpirv_Uniforms(comp, sr, desc.shaderType, shader.bindings);

		// Bindings Locations
		ui32 samplersCount = 0u;
		ui32 imagesCount = 0u;
		ui32 uniformsCount = 0u;

		shader.bindingsLocation.resize(shader.bindings.size());

		for (ui32 i = 0; i < shader.bindings.size(); ++i) {

			switch (shader.bindings[i].descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				shader.bindingsLocation[shader.bindings[i].binding] = samplersCount;
				samplersCount++;
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				shader.bindingsLocation[shader.bindings[i].binding] = imagesCount;
				imagesCount++;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				shader.bindingsLocation[shader.bindings[i].binding] = uniformsCount;
				uniformsCount++;
				break;
			}
		}

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
				colorAttachments[colorIt].layout = graphics_vulkan_parse_image_layout(attDesc.layout);
				colorAttachments[colorIt].attachment = i;
				colorIt++;
				break;
			case SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL:
				if (hasDepthStencil) return false;

				depthStencilAttachment.attachment = i;
				depthStencilAttachment.layout = graphics_vulkan_parse_image_layout(attDesc.layout, attDesc.format);

				hasDepthStencil = true;
				break;
			}

			// Description
			attachments[i].flags = 0u;
			attachments[i].format = graphics_vulkan_parse_format(attDesc.format);
			attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[i].loadOp = graphics_vulkan_parse_attachment_loadop(attDesc.loadOp);
			attachments[i].storeOp = graphics_vulkan_parse_attachment_storeop(attDesc.storeOp);
			attachments[i].initialLayout = graphics_vulkan_parse_image_layout(attDesc.initialLayout);
			attachments[i].finalLayout = graphics_vulkan_parse_image_layout(attDesc.finalLayout);

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
		// Compute Hash
		pipeline.hash = 0u;

		if (desc.pVertexShader)		sv::utils_hash_combine(pipeline.hash, desc.pVertexShader->GetPtr());
		if (desc.pPixelShader)		sv::utils_hash_combine(pipeline.hash, desc.pPixelShader->GetPtr());
		if (desc.pGeometryShader)	sv::utils_hash_combine(pipeline.hash, desc.pGeometryShader->GetPtr());

		sv::utils_hash_combine(pipeline.hash, _sv::graphics_compute_hash_inputlayout(desc.pInputLayout));
		sv::utils_hash_combine(pipeline.hash, _sv::graphics_compute_hash_rasterizerstate(desc.pRasterizerState));
		sv::utils_hash_combine(pipeline.hash, _sv::graphics_compute_hash_blendstate(desc.pBlendState));
		sv::utils_hash_combine(pipeline.hash, _sv::graphics_compute_hash_depthstencilstate(desc.pDepthStencilState));

		// Get VulkanPipeline
		m_PipelinesMutex.lock();

		auto it = m_Pipelines.find(pipeline.hash);
		if (it == m_Pipelines.end()) {
			m_Pipelines[pipeline.hash] = VulkanPipeline();
		}

		VulkanPipeline& p = m_Pipelines[pipeline.hash];

		m_PipelinesMutex.unlock();

		// Create
		std::lock_guard<std::mutex> lock(p.creationMutex);

		// Check if it is created
		if (p.layout != VK_NULL_HANDLE) return true;

		if (desc.pVertexShader) {
			Shader_vk& shader = *reinterpret_cast<Shader_vk*>(desc.pVertexShader->GetPtr());
			p.semanticNames = shader.semanticNames;
			p.bindings.insert(p.bindings.end(), shader.bindings.begin(), shader.bindings.end());
			p.bindingsLocation.insert(p.bindingsLocation.end(), shader.bindingsLocation.begin(), shader.bindingsLocation.end());
		}
		if (desc.pPixelShader) {
			Shader_vk& shader = *reinterpret_cast<Shader_vk*>(desc.pPixelShader->GetPtr());
			p.bindings.insert(p.bindings.end(), shader.bindings.begin(), shader.bindings.end());
			p.bindingsLocation.insert(p.bindingsLocation.end(), shader.bindingsLocation.begin(), shader.bindingsLocation.end());
		}

		// Count
		ui32 samplersCount = 0u;
		ui32 imagesCount = 0u;
		ui32 uniformsCount = 0u;

		for (ui32 i = 0; i < p.bindings.size(); ++i) {

			switch (p.bindings[i].descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				samplersCount++;
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				imagesCount++;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				uniformsCount++;
				break;
			}
		}

		// Create Pipeline Layout
		VkDescriptorSetLayout setLayout;
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.flags = 0u;
			layoutInfo.bindingCount = p.bindings.size();
			layoutInfo.pBindings = p.bindings.data();

			vkCheck(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &setLayout));

			VkPipelineLayoutCreateInfo layout_info{};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1u;
			layout_info.pSetLayouts = &setLayout;
			layout_info.pushConstantRangeCount = 0u;

			vkCheck(vkCreatePipelineLayout(m_Device, &layout_info, nullptr, &p.layout));
		}

		if (p.bindings.size() == 0) return true;

		p.descriptors.Create(setLayout, m_FrameCount, imagesCount, samplersCount, uniformsCount);

		return true;
	}

	///////////////////////////////////////////////////// DESTRUCTION ///////////////////////////////////////////////////////

	bool Graphics_vk::DestroyBuffer(Buffer_vk& buffer)
	{
		vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
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
	Image& Graphics_vk::AcquireSwapChainImage()
	{
		vkAssert(vkAcquireNextImageKHR(m_Device, m_SwapChain.swapChain, UINT64_MAX, m_SwapChain.semAcquireImage, VK_NULL_HANDLE, &m_ImageIndex));

		m_SwapChain.backBuffer.image = m_SwapChain.images[m_ImageIndex].image;
		m_SwapChain.backBuffer.renderTargetView = m_SwapChain.images[m_ImageIndex].view;
		m_SwapChain.backBuffer.ID = m_SwapChain.images[m_ImageIndex].ID;

		SV_GFX_IMAGE_DESC bbDesc{};
		bbDesc.width = m_SwapChain.currentExtent.width;
		bbDesc.height = m_SwapChain.currentExtent.height;
		bbDesc.depth = 1u;
		bbDesc.layers = 1u;
		bbDesc.dimension = 2u;
		bbDesc.format = graphics_vulkan_parse_format(m_SwapChain.currentFormat);

		m_SwapChain.backBuffer.SetType(SV_GFX_PRIMITIVE_IMAGE);
		m_SwapChain.backBuffer.SetDescription(bbDesc);

		m_SwapChain.backBufferImage = Primitive(&m_SwapChain.backBuffer);
		return *reinterpret_cast<Image*>(&m_SwapChain.backBufferImage);
	}

	void Graphics_vk::WaitGPU()
	{
		vkAssert(vkDeviceWaitIdle(m_Device));
	}

	void Graphics_vk::BeginFrame()
	{
		Frame& frame = GetFrame();

		frame.memory.Reset();

		// Reset Pipeline Descriptors
		for (auto& it : m_Pipelines) {
			it.second.descriptors.Reset(m_CurrentFrame);
		}

		vkAssert(vkWaitForFences(m_Device, 1, &frame.fence, VK_TRUE, UINT64_MAX));

		vkAssert(vkResetCommandPool(m_Device, frame.commandPool, 0u));

	}
	void Graphics_vk::SubmitCommandLists()
	{
		Frame& frame = GetFrame();

		// End CommandBuffers & RenderPasses
		for (ui32 i = 0; i < m_ActiveCMDCount; ++i) {

			VkCommandBuffer cmd = GetCMD(i);

			if (i == m_ActiveCMDCount - 1) {
				
				VkImageMemoryBarrier imageBarrier{};
				imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
				imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier.image = m_SwapChain.backBuffer.image;
				imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBarrier.subresourceRange.layerCount = 1u;
				imageBarrier.subresourceRange.levelCount = 1u;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0u, 0u, 0u, 0u, 0u, 1u, &imageBarrier);

			}

			vkAssert(vkEndCommandBuffer(cmd));
		}

		if (m_ImageFences[m_ImageIndex] != VK_NULL_HANDLE) {
			vkAssert(vkWaitForFences(m_Device, 1u, &m_ImageFences[m_ImageIndex], VK_TRUE, UINT64_MAX));
		}
		m_ImageFences[m_ImageIndex] = frame.fence;

		vkAssert(vkResetFences(m_Device, 1u, &frame.fence));

		VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

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

		vkAssert(vkQueueSubmit(m_QueueGraphics, 1u, &submit_info, frame.fence));
	}
	void Graphics_vk::Present()
	{
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

	void Graphics_vk::Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		UpdateGraphicsState(cmd);
		vkCmdDraw(GetCMD(cmd), vertexCount, instanceCount, startVertex, startInstance);
	}
	void Graphics_vk::DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		UpdateGraphicsState(cmd);
		vkCmdDrawIndexed(GetCMD(cmd), indexCount, instanceCount, startIndex, startVertex, startInstance);
	}

	void Graphics_vk::UpdateBuffer(Buffer& buffer_, void* pData, ui32 size, ui32 offset, CommandList cmd_)
	{
		Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(buffer_.GetPtr());

		VmaAllocationInfo info;
		vmaGetAllocationInfo(m_Allocator, buffer.allocation, &info);
		// TODO: For now
		if (true) {
			VkCommandBuffer cmd = GetCMD(cmd_);

			// Transfer dst Memory Barrier
			VkBufferMemoryBarrier bufferBarrier{};
			bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferBarrier.buffer = buffer.buffer;
			bufferBarrier.size = VK_WHOLE_SIZE;
			bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.srcAccessMask = 0u;

			VkPipelineStageFlags stages = 0u;

			switch (buffer.GetBufferType())
			{
			case SV_GFX_BUFFER_TYPE_VERTEX:
				bufferBarrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
				stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				break;
			case SV_GFX_BUFFER_TYPE_INDEX:
				bufferBarrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
				stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				break;
			case SV_GFX_BUFFER_TYPE_CONSTANT:
				bufferBarrier.srcAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
				stages |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				break;
			}

			vkCmdPipelineBarrier(cmd, stages, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 1u, &bufferBarrier, 0u, nullptr);

			// copy
			void* mapData;
			VkBuffer mapBuffer;
			ui32 mapOffset;
			GetFrame().memory.GetMappingData(size, mapBuffer, &mapData, mapOffset);
			memcpy(mapData, pData, ui64(size));
			CopyBuffer(cmd, mapBuffer, buffer.buffer, VkDeviceSize(mapOffset), VkDeviceSize(offset), VkDeviceSize(size));

			// Memory Barrier

			std::swap(bufferBarrier.srcAccessMask, bufferBarrier.dstAccessMask);
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, stages, 0u, 0u, nullptr, 1u, &bufferBarrier, 0u, nullptr);

		}
		else {
			ui8* dst;
			vmaMapMemory(m_Allocator, buffer.allocation, (void**)&dst);
			memcpy(dst + ui64(offset), pData, ui64(size));
			vmaUnmapMemory(m_Allocator, buffer.allocation);
		}
	}

	void Graphics_vk::Barrier(const sv::GPUBarrier* barriers, ui32 count, sv::CommandList cmd_)
	{
		VkCommandBuffer cmd = GetCMD(cmd_);

		VkPipelineStageFlags srcStage = 0u;
		VkPipelineStageFlags dstStage = 0u;

		ui32 memoryBarrierCount = 0u;
		ui32 bufferBarrierCount = 0u;
		ui32 imageBarrierCount = 0u;

		VkMemoryBarrier			memoryBarrier[SV_GFX_BARRIER_COUNT];
		VkBufferMemoryBarrier	bufferBarrier[SV_GFX_BARRIER_COUNT];
		VkImageMemoryBarrier	imageBarrier[SV_GFX_BARRIER_COUNT];

		for (ui32 i = 0; i < count; ++i) {

			const GPUBarrier& barrier = barriers[i];

			switch (barrier.type)
			{
			case SV_GFX_BARRIER_TYPE_IMAGE:

				Image_vk& image = *reinterpret_cast<Image_vk*>(barrier.image.pImage->GetPtr());

				imageBarrier[imageBarrierCount].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarrier[imageBarrierCount].pNext = nullptr;
				imageBarrier[imageBarrierCount].srcAccessMask = graphics_vulkan_access_from_image_layout(barrier.image.oldLayout);
				imageBarrier[imageBarrierCount].dstAccessMask = graphics_vulkan_access_from_image_layout(barrier.image.newLayout);
				imageBarrier[imageBarrierCount].oldLayout = graphics_vulkan_parse_image_layout(barrier.image.oldLayout);
				imageBarrier[imageBarrierCount].newLayout = graphics_vulkan_parse_image_layout(barrier.image.newLayout);
				imageBarrier[imageBarrierCount].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier[imageBarrierCount].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier[imageBarrierCount].image = image.image;
				imageBarrier[imageBarrierCount].subresourceRange.aspectMask = graphics_vulkan_aspect_from_image_layout(barrier.image.oldLayout, image.GetFormat());
				imageBarrier[imageBarrierCount].subresourceRange.layerCount = image.GetLayers();
				imageBarrier[imageBarrierCount].subresourceRange.levelCount = 1u;
				imageBarrier[imageBarrierCount].subresourceRange.baseArrayLayer = 0u;
				imageBarrier[imageBarrierCount].subresourceRange.baseMipLevel = 0u;

				srcStage |= graphics_vulkan_stage_from_image_layout(barrier.image.oldLayout);
				dstStage |= graphics_vulkan_stage_from_image_layout(barrier.image.newLayout);

				imageBarrierCount++;
				break;
			}

		}

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, memoryBarrierCount, memoryBarrier, bufferBarrierCount, bufferBarrier, imageBarrierCount, imageBarrier);
	}

	//////////////////////////////////////// CONSTRUCTOR & DESTRUCTOR ////////////////////////////////////////
	void* VulkanConstructor(SV_GFX_PRIMITIVE type, const void* desc)
	{
		Graphics_vk& gfx = *reinterpret_cast<Graphics_vk*>(_sv::graphics_device_get());
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
		Graphics_vk& gfx = *reinterpret_cast<Graphics_vk*>(_sv::graphics_device_get());

		vkDeviceWaitIdle(gfx.GetDevice());

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