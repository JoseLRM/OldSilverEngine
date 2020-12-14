#include "core.h"

#include "platform/window.h"

#define SV_VULKAN_IMPLEMENTATION
#include "graphics_vulkan.h"

namespace sv {

	static std::unique_ptr<Graphics_vk> g_API;

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
			SV_LOG_WARNING("[VULKAN] %s", data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			SV_LOG_WARNING("[VULKAN] %s", data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			SV_LOG_ERROR("[VULKAN] %s", data->pMessage);
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
	void __stdcall vkCmdInsertDebugUtilsLabelEXT(VkInstance instance, VkCommandBuffer cmd, const VkDebugUtilsLabelEXT* info)
	{
		auto fn = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
		if (fn) fn(cmd, info);
	}
	void __stdcall vkCmdBeginDebugUtilsLabelEXT(VkInstance instance, VkCommandBuffer cmd, const VkDebugUtilsLabelEXT* info)
	{
		auto fn = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
		if (fn) fn(cmd, info);
	}
	void __stdcall vkCmdEndDebugUtilsLabelEXT(VkInstance instance, VkCommandBuffer cmd)
	{
		auto fn = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
		if (fn) fn(cmd);
	}

	////////////////////////////////////////////ADAPTER/////////////////////////////////////////////////
	Adapter_vk::Adapter_vk(VkPhysicalDevice device) : m_PhysicalDevice(device)
	{
		vkGetPhysicalDeviceProperties(device, &m_Properties);
		vkGetPhysicalDeviceFeatures(device, &m_Features);
		vkGetPhysicalDeviceMemoryProperties(device, &m_MemoryProps);
		
		
		// Choose FamilyQueueIndices
		{
			u32 count = 0u;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
			std::vector<VkQueueFamilyProperties> props(count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

			for (u32 i = 0; i < props.size(); ++i) {
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
	
	//////////////////////////////////////////// DEVICE /////////////////////////////////////////////////
	
	void graphics_vulkan_device_prepare(GraphicsDevice& device)
	{
		device.initialize				= graphics_vulkan_initialize;
		device.close					= graphics_vulkan_close;
		device.get						= graphics_vulkan_get;
		device.create					= graphics_vulkan_create;
		device.destroy					= graphics_vulkan_destroy;
		device.commandlist_begin		= graphics_vulkan_commandlist_begin;
		device.commandlist_last			= graphics_vulkan_commandlist_last;
		device.commandlist_count		= graphics_vulkan_commandlist_count;
		device.renderpass_begin			= graphics_vulkan_renderpass_begin;
		device.renderpass_end			= graphics_vulkan_renderpass_end;
		device.swapchain_resize			= graphics_vulkan_swapchain_resize;
		device.gpu_wait					= graphics_vulkan_gpu_wait;
		device.frame_begin				= graphics_vulkan_frame_begin;
		device.frame_end				= graphics_vulkan_frame_end;
		device.present					= graphics_vulkan_present;
		device.draw						= graphics_vulkan_draw;
		device.draw_indexed				= graphics_vulkan_draw_indexed;
		device.image_clear				= graphics_vulkan_image_clear;
		device.image_blit				= graphics_vulkan_image_blit;
		device.buffer_update			= graphics_vulkan_buffer_update;
		device.barrier					= graphics_vulkan_barrier;
		device.event_begin				= graphics_vulkan_event_begin;
		device.event_mark				= graphics_vulkan_event_mark;
		device.event_end				= graphics_vulkan_event_end;

		device.bufferAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(Buffer_vk), 200u);
		device.imageAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(Image_vk), 200u);
		device.samplerAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(Sampler_vk), 200u);
		device.shaderAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(Shader_vk), 200u);
		device.renderPassAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(RenderPass_vk), 200u);
		device.inputLayoutStateAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(InputLayoutState_vk), 200u);
		device.blendStateAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(BlendState_vk), 200u);
		device.depthStencilStateAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(DepthStencilState_vk), 200u);
		device.rasterizerStateAllocator = std::make_unique<SizedInstanceAllocator>(sizeof(RasterizerState_vk), 200u);
		
		device.api = GraphicsAPI_Vulkan;
	}

	Result graphics_vulkan_initialize()
	{
		g_API = std::make_unique<Graphics_vk>();

		// Instance extensions and validation layers
#ifdef SV_ENABLE_VULKAN_VALIDATION_LAYERS
		g_API->validationLayers.push_back("VK_LAYER_KHRONOS_validation");
		g_API->extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		g_API->extensions.push_back("VK_KHR_surface");
		g_API->extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

		// Device extensions and validation layers
#ifdef SV_ENABLE_VULKAN_VALIDATION_LAYERS
		g_API->deviceValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
		g_API->deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		// Create instance
		{
			// Check Extensions
			{
				u32 count = 0u;
				vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
				std::vector<VkExtensionProperties> props(count);
				vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

				for (u32 i = 0; i < g_API->extensions.size(); ++i) {
					bool found = false;

					for (u32 j = 0; j < props.size(); ++j) {
						if (strcmp(props[j].extensionName, g_API->extensions[i]) == 0) {
							found = true;
							break;
						}
					}

					if (!found) {
						SV_LOG_ERROR("InstanceExtension '%s' not found", g_API->extensions[i]);
						return Result_GraphicsAPIError;
					}
				}
			}

			// Check Validation Layers
			{
				u32 count = 0u;
				vkEnumerateInstanceLayerProperties(&count, nullptr);
				std::vector<VkLayerProperties> props(count);
				vkEnumerateInstanceLayerProperties(&count, props.data());

				for (u32 i = 0; i < g_API->validationLayers.size(); ++i) {
					bool found = false;

					for (u32 j = 0; j < props.size(); ++j) {
						if (strcmp(props[j].layerName, g_API->validationLayers[i]) == 0) {
							found = true;
							break;
						}
					}

					if (!found) {
						SV_LOG_ERROR("InstanceValidationLayer '%s' not found", g_API->validationLayers[i]);
						return Result_GraphicsAPIError;
					}
				}
			}

			// Create Instance
			VkApplicationInfo app_info{};
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pEngineName = "SilverEngine";
			app_info.pApplicationName = "";
			//Version engineVersion = GetEngine().GetVersion();
			//app_info.engineVersion = VK_MAKE_VERSION(engineVersion.major, engineVersion.minor, engineVersion.revision);
			app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
			app_info.apiVersion = VK_VERSION_1_2;

			VkInstanceCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			create_info.pApplicationInfo = &app_info;

			create_info.enabledExtensionCount = u32(g_API->extensions.size());
			create_info.ppEnabledExtensionNames = g_API->extensions.data();
			create_info.enabledLayerCount = u32(g_API->validationLayers.size());
			create_info.ppEnabledLayerNames = g_API->validationLayers.data();

			create_info.flags = 0u;

			vkCheck(vkCreateInstance(&create_info, nullptr, &g_API->instance));
		}

		// Initialize validation layers
#ifdef SV_ENABLE_VULKAN_VALIDATION_LAYERS
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
			
			vkCheck(vkCreateDebugUtilsMessengerEXT(g_API->instance, &create_info, nullptr, &g_API->debug));
		}
#endif

		// Create adapter
		{
			// Enumerate Vulkan Adapters
			u32 count = 0u;
			vkCheck(vkEnumeratePhysicalDevices(g_API->instance, &count, nullptr));

			std::vector<VkPhysicalDevice> devices(count);
			vkCheck(vkEnumeratePhysicalDevices(g_API->instance, &count, devices.data()));

			// Allocate in Graphics member
			for (u32 i = 0; i < devices.size(); ++i) {
				std::unique_ptr<Adapter> adapter = std::make_unique<Adapter_vk>(devices[i]);
				if (adapter->GetSuitability() > 0)
					graphics_adapter_add(std::move(adapter));
			}

			// Select the most suitable
			u32 maxSuitability = 0;
			u32 index = count;

			const auto& adapters = graphics_adapter_get_list();

			for (u32 i = 0; i < adapters.size(); ++i) {
				Adapter_vk& adapter = *reinterpret_cast<Adapter_vk*>(adapters[i].get());
				if (adapter.GetSuitability() > maxSuitability) index = i;
			}

			// Adapter not found
			if (index == count) {
				SV_LOG_ERROR("Can't find valid adapters for Vulkan");
				return Result_GraphicsAPIError;
			}

			graphics_adapter_set(index);
		}
		
		// Create logical device
		{
			const Adapter_vk& adapter = *reinterpret_cast<const Adapter_vk*>(graphics_adapter_get());
			VkPhysicalDevice physicalDevice = adapter.GetPhysicalDevice();

			// Queue Create Info Structs
			constexpr u32 queueCount = 1u;
			u32 queueIndices[] = {
				adapter.GetFamilyIndex().graphics
			};

			const float priorities = 1.f;

			VkDeviceQueueCreateInfo queue_create_info[queueCount];
			for (u32 i = 0; i < queueCount; ++i) {
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

			create_info.enabledLayerCount = u32(g_API->deviceValidationLayers.size());
			create_info.ppEnabledLayerNames = g_API->deviceValidationLayers.data();
			create_info.enabledExtensionCount = u32(g_API->deviceExtensions.size());
			create_info.ppEnabledExtensionNames = g_API->deviceExtensions.data();

			create_info.pEnabledFeatures = &adapter.GetFeatures();

			vkCheck(vkCreateDevice(physicalDevice, &create_info, nullptr, &g_API->device));

			// Get Queues
			vkGetDeviceQueue(g_API->device, adapter.GetFamilyIndex().graphics, 0u, &g_API->queueGraphics);
		}

		// Create allocator
		{
			Adapter_vk& adapter = *reinterpret_cast<Adapter_vk*>(graphics_adapter_get());

			VmaAllocatorCreateInfo create_info{};
			create_info.device = g_API->device;
			create_info.instance = g_API->instance;
			create_info.physicalDevice = adapter.GetPhysicalDevice();

			vkCheck(vmaCreateAllocator(&create_info, &g_API->allocator));
		}
		
		// Create frames
		{
			g_API->frameCount = 3u;
			g_API->frames.resize(g_API->frameCount);

			VkCommandPoolCreateInfo cmdPool_info{};
			cmdPool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cmdPool_info.queueFamilyIndex = reinterpret_cast<Adapter_vk*>(graphics_adapter_get())->GetFamilyIndex().graphics;

			for (u32 i = 0; i < g_API->frameCount; ++i) {
				Frame& frame = g_API->frames[i];

				frame.fence = graphics_vulkan_fence_create(true);

				vkCheck(vkCreateCommandPool(g_API->device, &cmdPool_info, nullptr, &frame.commandPool));

				cmdPool_info.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
				vkCheck(vkCreateCommandPool(g_API->device, &cmdPool_info, nullptr, &frame.transientCommandPool));

				// AllocateCommandBuffers
				VkCommandBufferAllocateInfo alloc_info{};
				alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				alloc_info.commandPool = frame.commandPool;
				alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				alloc_info.commandBufferCount = GraphicsLimit_CommandList;

				vkCheck(vkAllocateCommandBuffers(g_API->device, &alloc_info, frame.commandBuffers));
			}
		}
		
		// Create swapchain
		{
			Result res = graphics_vulkan_swapchain_create(0);
			if (res != Result_Success) {
				SV_LOG_ERROR("Can't create vulkan swapchain");
				return res;
			}
		}

		// Set properties
		{
			GraphicsProperties props;
			graphics_properties_set(props);
		}

		return Result_Success;
	}

	Result graphics_vulkan_close()
	{
		vkDeviceWaitIdle(g_API->device);

		// Destroy Pipelines
		for (auto& it : g_API->pipelines) {
			graphics_vulkan_pipeline_destroy(it.second);
		}
		g_API->pipelines.clear();

		// Destroy frames
		for (u32 i = 0; i < g_API->frameCount; ++i) {
			Frame& frame = g_API->frames[i];
			vkDestroyCommandPool(g_API->device, frame.commandPool, nullptr);
			vkDestroyCommandPool(g_API->device, frame.transientCommandPool, nullptr);
			vkDestroyFence(g_API->device, frame.fence, nullptr);

			for (u32 i = 0; i < GraphicsLimit_CommandList; ++i) {
				graphics_vulkan_descriptors_clear(frame.descPool[i]);
			}
		}

		// Destroy Allocator
		vmaDestroyAllocator(g_API->allocator);

		// Destroy SwapChain
		svCheck(graphics_vulkan_swapchain_destroy());

		// Destroy device and vulkan instance
		vkDestroyDevice(g_API->device, nullptr);

#ifdef SV_ENABLE_VULKAN_VALIDATION_LAYERS
		vkDestroyDebugUtilsMessengerEXT(g_API->instance, g_API->debug);
#endif

		vkDestroyInstance(g_API->instance, nullptr);
		return Result_Success;
	}

	void* graphics_vulkan_get()
	{
		return g_API.get();
	}

	Result graphics_vulkan_create(GraphicsPrimitiveType type, const void* desc, Primitive_internal* ptr)
	{
		switch (type)
		{

		case GraphicsPrimitiveType_Buffer:
		{
			Buffer_vk* b = new(ptr) Buffer_vk();
			return graphics_vulkan_buffer_create(*b, *reinterpret_cast<const GPUBufferDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_Shader:
		{
			Shader_vk* s = new(ptr) Shader_vk();
			return graphics_vulkan_shader_create(*s, *reinterpret_cast<const ShaderDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_Image:
		{
			Image_vk* i = new(ptr) Image_vk();
			return graphics_vulkan_image_create(*i, *reinterpret_cast<const GPUImageDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_Sampler:
		{
			Sampler_vk* i = new(ptr) Sampler_vk();
			return graphics_vulkan_sampler_create(*i, *reinterpret_cast<const SamplerDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_RenderPass:
		{
			RenderPass_vk* rp = new(ptr) RenderPass_vk();
			return graphics_vulkan_renderpass_create(*rp, *reinterpret_cast<const RenderPassDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_InputLayoutState:
		{
			InputLayoutState_vk* ils = new(ptr) InputLayoutState_vk();
			return graphics_vulkan_inputlayoutstate_create(*ils, *reinterpret_cast<const InputLayoutStateDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_BlendState:
		{
			BlendState_vk* bs = new(ptr) BlendState_vk();
			return graphics_vulkan_blendstate_create(*bs, *reinterpret_cast<const BlendStateDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_DepthStencilState:
		{
			DepthStencilState_vk* dss = new(ptr) DepthStencilState_vk();
			return graphics_vulkan_depthstencilstate_create(*dss, *reinterpret_cast<const DepthStencilStateDesc*>(desc));
		}
		break;

		case GraphicsPrimitiveType_RasterizerState:
		{
			RasterizerState_vk* rs = new(ptr) RasterizerState_vk();
			return graphics_vulkan_rasterizerstate_create(*rs, *reinterpret_cast<const RasterizerStateDesc*>(desc));
		}
		break;

		}

		return Result_InvalidUsage;
	}

	Result graphics_vulkan_destroy(Primitive_internal* primitive)
	{
		vkDeviceWaitIdle(g_API->device);

		Result result = Result_Success;

		switch (primitive->type)
		{

		case GraphicsPrimitiveType_Buffer:
		{
			Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(primitive);
			result = graphics_vulkan_buffer_destroy(buffer);
			buffer.~Buffer_vk();
			break;
		}

		case GraphicsPrimitiveType_Shader:
		{
			Shader_vk& shader = *reinterpret_cast<Shader_vk*>(primitive);
			result = graphics_vulkan_shader_destroy(shader);
			shader.~Shader_vk();
			break;
		}

		case GraphicsPrimitiveType_Image:
		{
			Image_vk& image = *reinterpret_cast<Image_vk*>(primitive);
			result = graphics_vulkan_image_destroy(image);
			image.~Image_vk();
			break;
		}

		case GraphicsPrimitiveType_Sampler:
		{
			Sampler_vk& sampler = *reinterpret_cast<Sampler_vk*>(primitive);
			result = graphics_vulkan_sampler_destroy(sampler);
			sampler.~Sampler_vk();
			break;
		}

		case GraphicsPrimitiveType_RenderPass:
		{
			RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(primitive);
			result = graphics_vulkan_renderpass_destroy(renderPass);
			renderPass.~RenderPass_vk();
			break;
		}

		case GraphicsPrimitiveType_InputLayoutState:
		{
			InputLayoutState_vk& inputLayoutState = *reinterpret_cast<InputLayoutState_vk*>(primitive);
			inputLayoutState.~InputLayoutState_vk();
			result = Result_Success;
			break;
		}

		case GraphicsPrimitiveType_BlendState:
		{
			BlendState_vk& blendState = *reinterpret_cast<BlendState_vk*>(primitive);
			blendState.~BlendState_vk();
			result = Result_Success;
			break;
		}

		case GraphicsPrimitiveType_DepthStencilState:
		{
			DepthStencilState_vk& depthStencilState = *reinterpret_cast<DepthStencilState_vk*>(primitive);
			depthStencilState.~DepthStencilState_vk();
			result = Result_Success;
			break;
		}

		case GraphicsPrimitiveType_RasterizerState:
		{
			RasterizerState_vk& rasterizerState = *reinterpret_cast<RasterizerState_vk*>(primitive);
			rasterizerState.~RasterizerState_vk();
			result = Result_Success;
			break;
		}

		default:
			return Result_Success;

		}

		return result;
	}

	CommandList graphics_vulkan_commandlist_begin()
	{
		g_API->mutexCMD.lock();
		CommandList index = g_API->activeCMDCount++;
		VkCommandBuffer cmd = g_API->GetCMD(index);
		g_API->mutexCMD.unlock();

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = nullptr;

		vkAssert(vkBeginCommandBuffer(cmd, &begin_info));

		return index;
	}

	CommandList graphics_vulkan_commandlist_last()
	{
		std::lock_guard<std::mutex> lock(g_API->mutexCMD);
		SV_ASSERT(g_API->activeCMDCount != 0);
		return g_API->activeCMDCount - 1u;
	}

	u32 graphics_vulkan_commandlist_count()
	{
		std::lock_guard<std::mutex> lock(g_API->mutexCMD);
		return g_API->activeCMDCount;
	}

	void graphics_vulkan_renderpass_begin(CommandList cmd_)
	{
		GraphicsState& state = graphics_state_get().graphics[cmd_];
		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);
		VkCommandBuffer cmd = g_API->GetCMD(cmd_);

		// Color Attachments clear values
		VkClearValue clearValues[GraphicsLimit_Attachment];
		for (u32 i = 0; i < u32(renderPass.attachments.size()); ++i) {

			if (renderPass.depthStencilAttachment == i) {
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
			hash_combine(hash, renderPass.attachments.size());
			for (u32 i = 0; i < u32(renderPass.attachments.size()); ++i) {
				Image_vk& att = *reinterpret_cast<Image_vk*>(state.attachments[i]);
				hash_combine(hash, att.ID);
			}

			// Find framebuffer
			std::lock_guard<std::mutex> lock(renderPass.mutex);
			auto it = renderPass.frameBuffers.find(hash);
			if (it == renderPass.frameBuffers.end()) {

				// Create attachments views list
				VkImageView views[GraphicsLimit_Attachment];
				u32 width = 0, height = 0, layers = 0;

				for (u32 i = 0; i < u32(renderPass.attachments.size()); ++i) {
					Image_vk& att = *reinterpret_cast<Image_vk*>(state.attachments[i]);

					if (renderPass.depthStencilAttachment == i) {
						views[i] = att.depthStencilView;
					}
					else {
						views[i] = att.renderTargetView;
					}

					if (i == 0) {
						width = att.width;
						height = att.height;
						layers = att.layers;
					}
				}
				
				VkFramebufferCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				create_info.renderPass = renderPass.renderPass;
				create_info.attachmentCount = u32(renderPass.attachments.size());
				create_info.pAttachments = views;
				create_info.width = width;
				create_info.height = height;
				create_info.layers = layers;

				vkAssert(vkCreateFramebuffer(g_API->device, &create_info, nullptr, &fb));

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
		begin_info.renderArea.extent.width = state.attachments[0]->width;
		begin_info.renderArea.extent.height = state.attachments[0]->height;
		begin_info.clearValueCount = u32(renderPass.attachments.size());
		begin_info.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void graphics_vulkan_renderpass_end(CommandList cmd_)
	{
		GraphicsState& state = graphics_state_get().graphics[cmd_];
		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);
		VkCommandBuffer cmd = g_API->GetCMD(cmd_);

		vkCmdEndRenderPass(cmd);
	}

	void graphics_vulkan_swapchain_resize()
	{
		vkAssert(vkDeviceWaitIdle(g_API->device));
		VkSwapchainKHR old = g_API->swapChain.swapChain;
		graphics_vulkan_swapchain_destroy(false);
		graphics_vulkan_swapchain_create(old);
		vkDestroySwapchainKHR(g_API->device, old, nullptr);
	}

	void graphics_vulkan_gpu_wait()
	{
		vkAssert(vkDeviceWaitIdle(g_API->device));
	}

	void graphics_vulkan_frame_begin()
	{
		Frame& frame = g_API->GetFrame();
		float now = timer_now();

		// Reset Descriptors
		for (u32 i = 0; i < GraphicsLimit_CommandList; ++i) {
			graphics_vulkan_descriptors_reset(frame.descPool[i]);
		}

		// Destroy unused objects
		if (now - g_API->lastTime >= VULKAN_UNUSED_OBJECTS_TIMECHECK) {

			vkAssert(vkDeviceWaitIdle(g_API->device));

			// Pipelines
			auto next_it = g_API->pipelines.begin();
			for (auto it = next_it; it != g_API->pipelines.end(); it = next_it) {
				++next_it;
				size_t hash = it->first;
				auto& pipeline = it->second;
				if (now - pipeline.lastUsage >= VULKAN_UNUSED_OBJECTS_LIFETIME) {
					graphics_vulkan_pipeline_destroy(pipeline);
					g_API->pipelines.erase(it);
				}
			}

			g_API->lastTime = now;
		}

		vkAssert(vkWaitForFences(g_API->device, 1, &frame.fence, VK_TRUE, UINT64_MAX));

		vkAssert(vkResetCommandPool(g_API->device, frame.commandPool, 0u));
	}

	void graphics_vulkan_frame_end()
	{
		graphics_vulkan_submit_commandbuffers();
		graphics_vulkan_present();
	}

	void graphics_vulkan_present(GPUImage* image, const GPUImageRegion& region, GPUImageLayout layout, CommandList cmd)
	{
		// Adquire swapchain image
		graphics_vulkan_acquire_image();
		SwapChain::Image& swapChainImage = g_API->swapChain.images[g_API->imageIndex];

		// Image blit
		Image_vk dstImage;
		dstImage.image = swapChainImage.image;
		dstImage.layers = 1u;

		GPUImageBlit blit;
		blit.srcRegion = region;
		blit.dstRegion.offset = { 0u, 0u, 0u };
		blit.dstRegion.size = { g_API->swapChain.currentExtent.width, g_API->swapChain.currentExtent.height, 1u };

		graphics_vulkan_image_blit(image, reinterpret_cast<GPUImage*>(&dstImage), layout, GPUImageLayout_Present, 1u, &blit, SamplerFilter_Linear, cmd);
	}

	void graphics_vulkan_draw(u32 vertexCount, u32 instanceCount, u32 startVertex, u32 startInstance, CommandList cmd)
	{
		graphics_vulkan_state_update_graphics(cmd);
		vkCmdDraw(g_API->GetCMD(cmd), vertexCount, instanceCount, startVertex, startInstance);
	}

	void graphics_vulkan_draw_indexed(u32 indexCount, u32 instanceCount, u32 startIndex, u32 startVertex, u32 startInstance, CommandList cmd)
	{
		graphics_vulkan_state_update_graphics(cmd);
		vkCmdDrawIndexed(g_API->GetCMD(cmd), indexCount, instanceCount, startIndex, startVertex, startInstance);
	}

	void graphics_vulkan_image_clear(GPUImage* image_, GPUImageLayout oldLayout, GPUImageLayout newLayout, const Color4f& clearColor, float depth, u32 stencil, CommandList cmd_)
	{
		Image_vk& image = *reinterpret_cast<Image_vk*>(image_);
		VkCommandBuffer cmd = g_API->GetCMD(cmd_);

		VkImageSubresourceRange range{};
		range.aspectMask = graphics_vulkan_aspect_from_image_layout(oldLayout, image.format);
		range.baseMipLevel = 0u;
		range.levelCount = 1u;
		range.baseArrayLayer = 0u;
		range.layerCount = image.layers;

		// Image Barrier: from oldLayout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		VkPipelineStageFlags srcStage = graphics_vulkan_stage_from_image_layout(oldLayout);
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = graphics_vulkan_access_from_image_layout(oldLayout);
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = graphics_vulkan_parse_image_layout(oldLayout);
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image.image;
		barrier.subresourceRange = range;

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);

		// Clear
		if (image.imageType & GPUImageType_DepthStencil) {

			VkClearDepthStencilValue clear = { depth, stencil };
			vkCmdClearDepthStencilImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1u, &range);

		}
		else {

			VkClearColorValue clear = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
			vkCmdClearColorImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1u, &range);

		}

		// Image Barrier: from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to newLayout
		srcStage = dstStage;
		dstStage = graphics_vulkan_stage_from_image_layout(newLayout);

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = graphics_vulkan_access_from_image_layout(newLayout);
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = graphics_vulkan_parse_image_layout(newLayout);
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image.image;

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
	}
	
	void graphics_vulkan_image_blit(GPUImage* src, GPUImage* dst, GPUImageLayout srcLayout, GPUImageLayout dstLayout, u32 count, const GPUImageBlit* imageBlit, SamplerFilter filter, CommandList cmd_)
	{
		VkCommandBuffer cmd = g_API->GetCMD(cmd_);

		SV_ASSERT(count <= 16u);

		Image_vk& srcImage = *reinterpret_cast<Image_vk*>(src);
		Image_vk& dstImage = *reinterpret_cast<Image_vk*>(dst);

		// Barriers

		VkImageMemoryBarrier imgBarrier[2];
		imgBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgBarrier[0].pNext = nullptr;
		imgBarrier[0].srcAccessMask = graphics_vulkan_access_from_image_layout(srcLayout);
		imgBarrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imgBarrier[0].oldLayout = graphics_vulkan_parse_image_layout(srcLayout);
		imgBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier[0].image = srcImage.image;
		imgBarrier[0].subresourceRange.aspectMask = graphics_vulkan_aspect_from_image_layout(srcLayout);
		imgBarrier[0].subresourceRange.baseArrayLayer = 0u;
		imgBarrier[0].subresourceRange.baseMipLevel = 0u;
		imgBarrier[0].subresourceRange.layerCount = srcImage.layers;
		imgBarrier[0].subresourceRange.levelCount = 1u;

		imgBarrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgBarrier[1].pNext = nullptr;
		imgBarrier[1].srcAccessMask = graphics_vulkan_access_from_image_layout(dstLayout);
		imgBarrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		imgBarrier[1].oldLayout = graphics_vulkan_parse_image_layout(dstLayout);
		imgBarrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgBarrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier[1].image = dstImage.image;
		imgBarrier[1].subresourceRange.aspectMask = graphics_vulkan_aspect_from_image_layout(dstLayout);
		imgBarrier[1].subresourceRange.baseArrayLayer = 0u;
		imgBarrier[1].subresourceRange.baseMipLevel = 0u;
		imgBarrier[1].subresourceRange.layerCount = dstImage.layers;
		imgBarrier[1].subresourceRange.levelCount = 1u;

		VkPipelineStageFlags srcStage = graphics_vulkan_stage_from_image_layout(srcLayout) | graphics_vulkan_stage_from_image_layout(dstLayout);
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, 0u, 0u, 0u, 0u, 2u, imgBarrier);

		VkImageBlit blits[16];

		for (u32 i = 0; i < count; ++i) {

			blits[i].srcOffsets[0].x = imageBlit[i].srcRegion.offset.x;
			blits[i].srcOffsets[0].y = imageBlit[i].srcRegion.offset.y;
			blits[i].srcOffsets[0].z = imageBlit[i].srcRegion.offset.z;

			blits[i].srcOffsets[1].x = imageBlit[i].srcRegion.offset.x + imageBlit[i].srcRegion.size.x;
			blits[i].srcOffsets[1].y = imageBlit[i].srcRegion.offset.y + imageBlit[i].srcRegion.size.y;
			blits[i].srcOffsets[1].z = imageBlit[i].srcRegion.offset.z + imageBlit[i].srcRegion.size.z;

			blits[i].dstOffsets[0].x = imageBlit[i].dstRegion.offset.x;
			blits[i].dstOffsets[0].y = imageBlit[i].dstRegion.offset.y;
			blits[i].dstOffsets[0].z = imageBlit[i].dstRegion.offset.z;

			blits[i].dstOffsets[1].x = imageBlit[i].dstRegion.offset.x + imageBlit[i].dstRegion.size.x;
			blits[i].dstOffsets[1].y = imageBlit[i].dstRegion.offset.y + imageBlit[i].dstRegion.size.y;
			blits[i].dstOffsets[1].z = imageBlit[i].dstRegion.offset.z + imageBlit[i].dstRegion.size.z;

			blits[i].srcSubresource.aspectMask = graphics_vulkan_aspect_from_image_layout(srcLayout);
			blits[i].srcSubresource.baseArrayLayer = 0u;
			blits[i].srcSubresource.layerCount = srcImage.layers;
			blits[i].srcSubresource.mipLevel = 0u;

			blits[i].dstSubresource.aspectMask = graphics_vulkan_aspect_from_image_layout(dstLayout);
			blits[i].dstSubresource.baseArrayLayer = 0u;
			blits[i].dstSubresource.layerCount = dstImage.layers;
			blits[i].dstSubresource.mipLevel = 0u;

		}

		vkCmdBlitImage(cmd, srcImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, count, blits, graphics_vulkan_parse_filter(filter));

		// Barrier
		std::swap(srcStage, dstStage);
		std::swap(imgBarrier[0].srcAccessMask, imgBarrier[0].dstAccessMask);
		std::swap(imgBarrier[0].oldLayout, imgBarrier[0].newLayout);
		std::swap(imgBarrier[1].srcAccessMask, imgBarrier[1].dstAccessMask);
		std::swap(imgBarrier[1].oldLayout, imgBarrier[1].newLayout);

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, 0u, 0u, 0u, 0u, 2u, imgBarrier);
	}

	void graphics_vulkan_buffer_update(GPUBuffer* buffer_, void* pData, u32 size, u32 offset, CommandList cmd_)
	{
		Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(buffer_);

		VmaAllocationInfo info;
		vmaGetAllocationInfo(g_API->allocator, buffer.allocation, &info);
		// TODO: For now
		if (true) {
			VkCommandBuffer cmd = g_API->GetCMD(cmd_);

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

			switch (buffer.bufferType)
			{
			case GPUBufferType_Vertex:
				bufferBarrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
				stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				break;
			case GPUBufferType_Index:
				bufferBarrier.srcAccessMask |= VK_ACCESS_INDEX_READ_BIT;
				stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				break;
			case GPUBufferType_Constant:
				bufferBarrier.srcAccessMask |= VK_ACCESS_UNIFORM_READ_BIT;
				stages |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				break;
			}

			vkCmdPipelineBarrier(cmd, stages, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 1u, &bufferBarrier, 0u, nullptr);

			// copy
			void* mapData;
			VkBuffer mapBuffer;
			u32 mapOffset;
			buffer.memory.GetMappingData(size, mapBuffer, &mapData, mapOffset);
			memcpy(mapData, pData, u64(size));
			graphics_vulkan_buffer_copy(cmd, mapBuffer, buffer.buffer, VkDeviceSize(mapOffset), VkDeviceSize(offset), VkDeviceSize(size));

			// Memory Barrier

			std::swap(bufferBarrier.srcAccessMask, bufferBarrier.dstAccessMask);
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, stages, 0u, 0u, nullptr, 1u, &bufferBarrier, 0u, nullptr);

		}
		else {
			u8* dst;
			vmaMapMemory(g_API->allocator, buffer.allocation, (void**)& dst);
			memcpy(dst + u64(offset), pData, u64(size));
			vmaUnmapMemory(g_API->allocator, buffer.allocation);
		}
	}

	void graphics_vulkan_barrier(const GPUBarrier* barriers, u32 count, CommandList cmd_)
	{
		VkCommandBuffer cmd = g_API->GetCMD(cmd_);

		VkPipelineStageFlags srcStage = 0u;
		VkPipelineStageFlags dstStage = 0u;

		u32 memoryBarrierCount = 0u;
		u32 bufferBarrierCount = 0u;
		u32 imageBarrierCount = 0u;

		VkMemoryBarrier			memoryBarrier[GraphicsLimit_GPUBarrier];
		VkBufferMemoryBarrier	bufferBarrier[GraphicsLimit_GPUBarrier];
		VkImageMemoryBarrier	imageBarrier[GraphicsLimit_GPUBarrier];

		for (u32 i = 0; i < count; ++i) {

			const GPUBarrier& barrier = barriers[i];

			switch (barrier.type)
			{
			case BarrierType_Image:

				Image_vk& image = *reinterpret_cast<Image_vk*>(barrier.image.pImage);

				imageBarrier[imageBarrierCount].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarrier[imageBarrierCount].pNext = nullptr;
				imageBarrier[imageBarrierCount].srcAccessMask = graphics_vulkan_access_from_image_layout(barrier.image.oldLayout);
				imageBarrier[imageBarrierCount].dstAccessMask = graphics_vulkan_access_from_image_layout(barrier.image.newLayout);
				imageBarrier[imageBarrierCount].oldLayout = graphics_vulkan_parse_image_layout(barrier.image.oldLayout);
				imageBarrier[imageBarrierCount].newLayout = graphics_vulkan_parse_image_layout(barrier.image.newLayout);
				imageBarrier[imageBarrierCount].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier[imageBarrierCount].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier[imageBarrierCount].image = image.image;
				imageBarrier[imageBarrierCount].subresourceRange.aspectMask = graphics_vulkan_aspect_from_image_layout(barrier.image.oldLayout, image.format);
				imageBarrier[imageBarrierCount].subresourceRange.layerCount = image.layers;
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

	void graphics_vulkan_event_begin(const char* name, CommandList cmd)
	{
		VkDebugUtilsLabelEXT label = {};
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pLabelName = name;
		label.color[0] = 1.f;
		label.color[1] = 1.f;
		label.color[2] = 1.f;
		label.color[3] = 1.f;

		vkCmdBeginDebugUtilsLabelEXT(g_API->instance, g_API->GetCMD(cmd), &label);
	}
	void graphics_vulkan_event_mark(const char* name, CommandList cmd)
	{
		VkDebugUtilsLabelEXT label = {};
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pLabelName = name;
		label.color[0] = 1.f;
		label.color[1] = 1.f;
		label.color[2] = 1.f;
		label.color[3] = 1.f;

		vkCmdInsertDebugUtilsLabelEXT(g_API->instance, g_API->GetCMD(cmd), &label);
	}
	void graphics_vulkan_event_end(CommandList cmd)
	{
		vkCmdEndDebugUtilsLabelEXT(g_API->instance, g_API->GetCMD(cmd));
	}

	//////////////////////////////////////////// API /////////////////////////////////////////////////

	Result graphics_vulkan_swapchain_create(VkSwapchainKHR oldSwapchain)
	{
		const Adapter_vk& adapter = *reinterpret_cast<const Adapter_vk*>(graphics_adapter_get());
		VkPhysicalDevice physicalDevice = adapter.GetPhysicalDevice();

		WindowHandle hWnd = window_handle_get();
		u32 width = window_size_get().x;
		u32 height = window_size_get().y;

		// Create Surface
		if (oldSwapchain == VK_NULL_HANDLE) 
		{
#ifdef SV_PLATFORM_WIN
			VkWin32SurfaceCreateInfoKHR create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			create_info.hinstance = GetModuleHandle(nullptr);
			create_info.flags = 0u;
			create_info.hwnd = reinterpret_cast<HWND>(hWnd);

			vkCheck(vkCreateWin32SurfaceKHR(g_API->instance, &create_info, nullptr, &g_API->swapChain.surface));
#endif
		}

		// Check SwapChain Supported by this adapter
		{
			VkBool32 supported;
			vkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, adapter.GetFamilyIndex().graphics, g_API->swapChain.surface, &supported));
			if (!supported) {
				SV_LOG_ERROR("This adapter don't support vulkan swapChain");
				return Result_GraphicsAPIError;
			}
		}

		// Get Support Details
		vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, g_API->swapChain.surface, &g_API->swapChain.capabilities));
		{
			u32 count = 0;
			vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, g_API->swapChain.surface, &count, nullptr));
			g_API->swapChain.presentModes.resize(count);
			vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, g_API->swapChain.surface, &count, g_API->swapChain.presentModes.data()));
		}
		{
			u32 count = 0;
			vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, g_API->swapChain.surface, &count, nullptr));
			g_API->swapChain.formats.resize(count);
			vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, g_API->swapChain.surface, &count, g_API->swapChain.formats.data()));
		}

		// Create SwapChain
		{
			// Choose image count
			u32 imageCount = g_API->swapChain.capabilities.minImageCount + 1u;
			if (imageCount > g_API->swapChain.capabilities.maxImageCount && g_API->swapChain.capabilities.maxImageCount > 0)
				imageCount = g_API->swapChain.capabilities.maxImageCount;

			// Choose format
			VkSurfaceFormatKHR format = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			if (!g_API->swapChain.formats.empty()) format = g_API->swapChain.formats[0];

			for (u32 i = 0; i < g_API->swapChain.formats.size(); ++i) {
				VkSurfaceFormatKHR f = g_API->swapChain.formats[i];
				if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
					format = f;
					break;
				}
			}

			// Choose Swapchain Extent
			VkExtent2D extent;
			if (g_API->swapChain.capabilities.currentExtent.width != UINT32_MAX) extent = g_API->swapChain.capabilities.currentExtent;
			else {

				extent.width = width;
				extent.height = height;
				VkExtent2D min = g_API->swapChain.capabilities.minImageExtent;
				VkExtent2D max = g_API->swapChain.capabilities.maxImageExtent;

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
			u32 indices[] = {
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

			for (u32 i = 0; i < g_API->swapChain.presentModes.size(); ++i) {
				if (g_API->swapChain.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				//if (g_API->swapChain.presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
				//if (g_API->swapChain.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					presentMode = g_API->swapChain.presentModes[i];
					break;
				}
			}

			create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			create_info.flags = 0u;
			create_info.surface = g_API->swapChain.surface;
			create_info.minImageCount = imageCount;
			create_info.imageFormat = format.format;
			create_info.imageColorSpace = format.colorSpace;
			create_info.imageExtent = extent;
			create_info.imageArrayLayers = 1u;
			create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			create_info.preTransform = g_API->swapChain.capabilities.currentTransform;
			create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			create_info.presentMode = presentMode;
			create_info.clipped = VK_TRUE;
			create_info.oldSwapchain = oldSwapchain;

			vkCheck(vkCreateSwapchainKHR(g_API->device, &create_info, nullptr, &g_API->swapChain.swapChain));

			g_API->swapChain.currentFormat = format.format;
			g_API->swapChain.currentColorSpace = format.colorSpace;
			g_API->swapChain.currentPresentMode = presentMode;
			g_API->swapChain.currentExtent = extent;
		}

		// Get Images Handles
		{
			std::vector<VkImage> images;
			u32 count = 0u;
			vkCheck(vkGetSwapchainImagesKHR(g_API->device, g_API->swapChain.swapChain, &count, nullptr));
			images.resize(count);
			vkCheck(vkGetSwapchainImagesKHR(g_API->device, g_API->swapChain.swapChain, &count, images.data()));

			g_API->swapChain.images.resize(images.size());
			for (u32 i = 0; i < g_API->swapChain.images.size(); ++i) {
				g_API->swapChain.images[i].image = images[i];
			}
		}

		// Create Views and IDs
		{
			for (u32 i = 0; i < g_API->swapChain.images.size(); ++i) {
				VkImage image = g_API->swapChain.images[i].image;

				g_API->swapChain.images[i].ID = g_API->GetID();

				VkImageViewCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				create_info.flags = 0u;
				create_info.image = image;
				create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				create_info.format = g_API->swapChain.currentFormat;
				create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				create_info.subresourceRange.baseArrayLayer = 0u;
				create_info.subresourceRange.baseMipLevel = 0u;
				create_info.subresourceRange.layerCount = 1u;
				create_info.subresourceRange.levelCount = 1u;

				vkCheck(vkCreateImageView(g_API->device, &create_info, nullptr, &g_API->swapChain.images[i].view));
			}
		}

		// Change Images layout
		{
			VkCommandBuffer cmd;
			vkCheck(graphics_vulkan_singletimecmb_begin(&cmd));

			for (u32 i = 0; i < g_API->swapChain.images.size(); ++i) {

				VkImageMemoryBarrier imageBarrier{};
				imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
				imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
				imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageBarrier.image = g_API->swapChain.images[i].image;
				imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBarrier.subresourceRange.layerCount = 1u;
				imageBarrier.subresourceRange.levelCount = 1u;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0u, 0u, 0u, 0u, 0u, 1u, &imageBarrier);

			}

			graphics_vulkan_singletimecmb_end(cmd);
		}

		// Create Semaphores
		g_API->swapChain.semAcquireImage = graphics_vulkan_semaphore_create();
		g_API->swapChain.semPresent = graphics_vulkan_semaphore_create();

		g_API->imageFences.clear();
		g_API->imageFences.resize(g_API->swapChain.images.size(), VK_NULL_HANDLE);

		return Result_Success;
	}

	Result graphics_vulkan_swapchain_destroy(bool resizing)
	{
		vkDestroySemaphore(g_API->device, g_API->swapChain.semAcquireImage, nullptr);
		vkDestroySemaphore(g_API->device, g_API->swapChain.semPresent, nullptr);

		for (u32 i = 0; i < g_API->swapChain.images.size(); ++i) {

			vkDestroyImageView(g_API->device, g_API->swapChain.images[i].view, nullptr);
		}
		g_API->swapChain.images.clear();

		if (resizing) {
			vkDestroySwapchainKHR(g_API->device, g_API->swapChain.swapChain, nullptr);
			vkDestroySurfaceKHR(g_API->instance, g_API->swapChain.surface, nullptr);
		}
		return Result_Success;
	}

	void graphics_vulkan_state_update_graphics(CommandList cmd_)
	{
		GraphicsState& state = graphics_state_get().graphics[cmd_];

		RenderPass_vk& renderPass = *reinterpret_cast<RenderPass_vk*>(state.renderPass);

		VkCommandBuffer cmd = g_API->GetCMD(cmd_);

		bool updatePipeline = state.flags & (
			GraphicsPipelineState_Shader |
			GraphicsPipelineState_InputLayoutState |
			GraphicsPipelineState_BlendState |
			GraphicsPipelineState_DepthStencilState |
			GraphicsPipelineState_RasterizerState |
			GraphicsPipelineState_Topology
				);

		// Bind Vertex Buffers
		if (state.flags & GraphicsPipelineState_VertexBuffer) {
			
			VkBuffer buffers[GraphicsLimit_VertexBuffer] = {};
			VkDeviceSize offsets[GraphicsLimit_VertexBuffer] = {};
			for (u32 i = 0; i < state.vertexBuffersCount; ++i) {
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
			if (state.vertexBuffersCount)
				vkCmdBindVertexBuffers(cmd, 0u, state.vertexBuffersCount, buffers, offsets);
		}
		// Bind Index Buffer
		if (state.flags & GraphicsPipelineState_IndexBuffer && state.indexBuffer != nullptr) {
			Buffer_vk& buffer = *reinterpret_cast<Buffer_vk*>(state.indexBuffer);
			vkCmdBindIndexBuffer(cmd, buffer.buffer, state.indexBufferOffset, graphics_vulkan_parse_indextype(buffer.indexType));
		}

		// Bind RenderPass
		if (state.flags & GraphicsPipelineState_RenderPass && g_API->activeRenderPass[cmd_] != renderPass.renderPass) {
			updatePipeline = true;
		}

		// Bind Pipeline
		size_t pipelineHash = 0u;

		if (updatePipeline) {

			// Set active renderpass
			g_API->activeRenderPass[cmd_] = renderPass.renderPass;

			// Compute hash
			pipelineHash = graphics_vulkan_pipeline_compute_hash(state);

			// Find Pipeline
			VulkanPipeline* pipelinePtr = nullptr;
			{
				// Critical Section
				std::lock_guard<std::mutex> lock(g_API->pipelinesMutex);
				auto it = g_API->pipelines.find(pipelineHash);
				if (it == g_API->pipelines.end()) {
					
					// Create New Pipeline Object
					VulkanPipeline& p = g_API->pipelines[pipelineHash];
					Shader_vk* vertexShader = reinterpret_cast<Shader_vk*>(state.vertexShader);
					Shader_vk* pixelShader = reinterpret_cast<Shader_vk*>(state.pixelShader);
					Shader_vk* geometryShader = reinterpret_cast<Shader_vk*>(state.geometryShader);
					bool res = graphics_vulkan_pipeline_create(p, vertexShader, pixelShader, geometryShader);
					SV_ASSERT(res);
					pipelinePtr = &p;

				}
				else {
					pipelinePtr = &it->second;
				}
			}
			VulkanPipeline& pipeline = *pipelinePtr;

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_vulkan_pipeline_get(pipeline, state, pipelineHash));

		}

		// Bind Viewports
		if (state.flags & GraphicsPipelineState_Viewport) {
			
			VkViewport viewports[GraphicsLimit_Viewport];
			for (u32 i = 0; i < state.viewportsCount; ++i) {
				const Viewport& vp = state.viewports[i];

				viewports[i].x			= vp.x;
				viewports[i].y			= vp.y;
				viewports[i].width		= vp.width;
				viewports[i].height		= vp.height;
				viewports[i].minDepth	= vp.minDepth;
				viewports[i].maxDepth	= vp.maxDepth;
			}
			for (u32 i = state.viewportsCount; i < GraphicsLimit_Viewport; ++i) {
				viewports[i].x = 0.f;
				viewports[i].y = 0.f;
				viewports[i].width = 1.f;
				viewports[i].height = 1.f;
				viewports[i].minDepth = 0.f;
				viewports[i].maxDepth = 1.f;
			}

			vkCmdSetViewport(cmd, 0u, GraphicsLimit_Viewport, viewports);

		}

		// Bind Scissors
		if (state.flags & GraphicsPipelineState_Scissor) {

			VkRect2D scissors[GraphicsLimit_Scissor];
			for (u32 i = 0; i < state.scissorsCount; ++i) {
				const Scissor& sc = state.scissors[i];

				scissors[i].offset.x = sc.x;
				scissors[i].offset.y = sc.y;
				scissors[i].extent.width = sc.width;
				scissors[i].extent.height = sc.height;
			}
			for (u32 i = state.scissorsCount; i < GraphicsLimit_Scissor; ++i) {
				scissors[i].offset.x = 0u;
				scissors[i].offset.y = 0u;
				scissors[i].extent.width = 1u;
				scissors[i].extent.height = 1u;
			}

			vkCmdSetScissor(cmd, 0u, GraphicsLimit_Scissor, scissors);

		}

		// Set stencil reference
		if (state.flags & GraphicsPipelineState_StencilRef) {
			vkCmdSetStencilReference(cmd, VK_STENCIL_FRONT_AND_BACK, state.stencilReference);
		}

		// Set line width
		if (state.flags & GraphicsPipelineState_LineWidth) {
			vkCmdSetLineWidth(cmd, state.lineWidth);
		}

		// Update Descriptors
		if (state.flags & (GraphicsPipelineState_ConstantBuffer | GraphicsPipelineState_Sampler | GraphicsPipelineState_Image)) {

			if (pipelineHash == 0u) {
				pipelineHash = graphics_vulkan_pipeline_compute_hash(state);
			}

			VulkanPipeline& pipeline = g_API->pipelines[pipelineHash];

			if (state.flags & (GraphicsPipelineState_ConstantBuffer_VS | GraphicsPipelineState_Sampler_VS | GraphicsPipelineState_Image_VS)) {

				pipeline.descriptorSets[ShaderType_Vertex] = graphics_vulkan_state_update_graphics_descriptors(pipeline, ShaderType_Vertex, state, state.flags & GraphicsPipelineState_Sampler_VS,
					state.flags& GraphicsPipelineState_Image_VS, state.flags& GraphicsPipelineState_ConstantBuffer_VS, cmd_);
				
			}
			if (state.flags & (GraphicsPipelineState_ConstantBuffer_PS | GraphicsPipelineState_Sampler_PS | GraphicsPipelineState_Image_PS)) {

				pipeline.descriptorSets[ShaderType_Pixel] = graphics_vulkan_state_update_graphics_descriptors(pipeline, ShaderType_Pixel, state, state.flags& GraphicsPipelineState_Sampler_PS,
					state.flags& GraphicsPipelineState_Image_PS, state.flags& GraphicsPipelineState_ConstantBuffer_PS, cmd_);

			}
			if (state.flags & (GraphicsPipelineState_ConstantBuffer_GS | GraphicsPipelineState_Sampler_GS | GraphicsPipelineState_Image_GS)) {

				pipeline.descriptorSets[ShaderType_Geometry] = graphics_vulkan_state_update_graphics_descriptors(pipeline, ShaderType_Geometry, state, state.flags& GraphicsPipelineState_Sampler_GS,
					state.flags& GraphicsPipelineState_Image_GS, state.flags& GraphicsPipelineState_ConstantBuffer_GS, cmd_);

			}

			u32 offset = 0u;
			for (u32 i = 0; i < ShaderType_GraphicsCount; ++i) {
				if (pipeline.descriptorSets[i] == VK_NULL_HANDLE) {
					if (i == offset) {
						offset++;
						continue;
					}
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, offset, i - offset, &pipeline.descriptorSets[offset], 0u, nullptr);
					offset = i + 1u;
				}
			}

			if (offset != ShaderType_GraphicsCount) {
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, offset, ShaderType_GraphicsCount - offset, &pipeline.descriptorSets[offset], 0u, nullptr);
			}
		}

		state.flags = 0u;
	}

	VkDescriptorSet graphics_vulkan_state_update_graphics_descriptors(VulkanPipeline& pipeline, ShaderType shaderType, GraphicsState& state, bool samplers, bool images, bool uniforms, CommandList cmd_)
	{
		VkCommandBuffer cmd = g_API->GetCMD(cmd_);

		samplers = true;
		images = true;
		uniforms = true;

		VkWriteDescriptorSet writeDesc[GraphicsLimit_ConstantBuffer + GraphicsLimit_GPUImage + GraphicsLimit_Sampler];
		u32 writeCount = 0u;
		const ShaderDescriptorSetLayout& layout = pipeline.setLayout.layouts[shaderType];

		VkDescriptorSet descSet = graphics_vulkan_descriptors_allocate_sets(g_API->GetFrame().descPool[cmd_], layout);

		// Write samplers
		if (samplers) {
			for (u32 i = 0; i < layout.count[0]; ++i) {

				const auto& binding = layout.bindings[i];

				writeDesc[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDesc[writeCount].pNext = nullptr;
				writeDesc[writeCount].dstSet = descSet;
				writeDesc[writeCount].dstBinding = binding.vulkanBinding;
				writeDesc[writeCount].dstArrayElement = 0u;
				writeDesc[writeCount].descriptorCount = 1u;
				writeDesc[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

				Sampler_vk* sampler = reinterpret_cast<Sampler_vk*>(state.samplers[shaderType][binding.userBinding]);
				if (sampler == nullptr) continue;
				writeDesc[writeCount].pImageInfo = &sampler->image_info;
				writeDesc[writeCount].pBufferInfo = nullptr;
				writeDesc[writeCount].pTexelBufferView = nullptr;

				writeCount++;
			}
		}

		// Write images
		if (images) {
			u32 end = layout.count[0] + layout.count[1];
			for (u32 i = layout.count[0]; i < end; ++i) {

				const auto& binding = layout.bindings[i];

				writeDesc[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDesc[writeCount].pNext = nullptr;
				writeDesc[writeCount].dstSet = descSet;
				writeDesc[writeCount].dstBinding = binding.vulkanBinding;
				writeDesc[writeCount].dstArrayElement = 0u;
				writeDesc[writeCount].descriptorCount = 1u;
				writeDesc[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

				Image_vk* image = reinterpret_cast<Image_vk*>(state.images[shaderType][binding.userBinding]);
				if (image == nullptr) continue;
				writeDesc[writeCount].pImageInfo = &image->image_info;
				writeDesc[writeCount].pBufferInfo = nullptr;
				writeDesc[writeCount].pTexelBufferView = nullptr;

				writeCount++;
			}
		}

		// Write uniforms
		if (uniforms) {
			u32 end = layout.count[0] + layout.count[1] + layout.count[2];
			for (u32 i = layout.count[0] + layout.count[1]; i < end; ++i) {

				const auto& binding = layout.bindings[i];

				writeDesc[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDesc[writeCount].pNext = nullptr;
				writeDesc[writeCount].dstSet = descSet;
				writeDesc[writeCount].dstBinding = binding.vulkanBinding;
				writeDesc[writeCount].dstArrayElement = 0u;
				writeDesc[writeCount].descriptorCount = 1u;
				writeDesc[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

				Buffer_vk* buffer = reinterpret_cast<Buffer_vk*>(state.constantBuffers[shaderType][binding.userBinding]);
				if (buffer == nullptr) continue;
				writeDesc[writeCount].pImageInfo = nullptr;
				writeDesc[writeCount].pBufferInfo = &buffer->buffer_info;
				writeDesc[writeCount].pTexelBufferView = nullptr;

				writeCount++;
			}
		}

		vkUpdateDescriptorSets(g_API->device, writeCount, writeDesc, 0u, nullptr);
		return descSet;
	}
	
	VkResult graphics_vulkan_singletimecmb_begin(VkCommandBuffer* pCmd)
	{
		// Allocate CommandBuffer
		{
			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc_info.commandPool = g_API->frames[g_API->currentFrame].transientCommandPool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = 1u;

			VkResult res = vkAllocateCommandBuffers(g_API->device, &alloc_info, pCmd);
			if (res != VK_SUCCESS) {
				SV_LOG_ERROR("Can't allocate SingleTime CommandBuffer");
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
				SV_LOG_ERROR("Can't begin SingleTime CommandBuffer");
				vkFreeCommandBuffers(g_API->device, g_API->frames[g_API->currentFrame].transientCommandPool, 1u, pCmd);
				return res;
			}
		}

		return VK_SUCCESS;
	}

	VkResult graphics_vulkan_singletimecmb_end(VkCommandBuffer cmd)
	{
		// End CommandBuffer
		VkResult res = vkEndCommandBuffer(cmd);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 0u;
		submit_info.commandBufferCount = 1u;
		submit_info.pCommandBuffers = &cmd;
		submit_info.signalSemaphoreCount = 0u;

		vkExt(vkQueueSubmit(g_API->queueGraphics, 1u, &submit_info, VK_NULL_HANDLE));
		vkQueueWaitIdle(g_API->queueGraphics);

		// Free CommandBuffer
		if (res == VK_SUCCESS) vkFreeCommandBuffers(g_API->device, g_API->frames[g_API->currentFrame].transientCommandPool, 1u, &cmd);

		return res;
	}

	void graphics_vulkan_buffer_copy(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
	{
		VkBufferCopy copy_info{};
		copy_info.dstOffset = dstOffset;
		copy_info.srcOffset = srcOffset;
		copy_info.size = size;
		vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1u, &copy_info);
	}

	VkResult graphics_vulkan_imageview_create(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags, u32 layerCount, VkImageView& view)
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

		return vkCreateImageView(g_API->device, &create_info, nullptr, &view);
	}

	VkSemaphore graphics_vulkan_semaphore_create()
	{
		VkSemaphore sem = VK_NULL_HANDLE;
		VkSemaphoreCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(g_API->device, &create_info, nullptr, &sem) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		return sem;
	}

	VkFence graphics_vulkan_fence_create(bool sign)
	{
		VkFence fence = VK_NULL_HANDLE;
		VkFenceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = sign ? VK_FENCE_CREATE_SIGNALED_BIT : 0u;

		if (vkCreateFence(g_API->device, &createInfo, nullptr, &fence) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		return fence;
	}

	void graphics_vulkan_acquire_image()
	{
		vkAssert(vkAcquireNextImageKHR(g_API->device, g_API->swapChain.swapChain, UINT64_MAX, g_API->swapChain.semAcquireImage, VK_NULL_HANDLE, &g_API->imageIndex));
	}

	void graphics_vulkan_submit_commandbuffers()
	{
		Frame& frame = g_API->GetFrame();

		// End CommandBuffers & RenderPasses
		for (u32 i = 0; i < g_API->activeCMDCount; ++i) {

			VkCommandBuffer cmd = g_API->GetCMD(i);
			vkAssert(vkEndCommandBuffer(cmd));
		}

		if (g_API->imageFences[g_API->imageIndex] != VK_NULL_HANDLE) {
			vkAssert(vkWaitForFences(g_API->device, 1u, &g_API->imageFences[g_API->imageIndex], VK_TRUE, UINT64_MAX));
		}
		g_API->imageFences[g_API->imageIndex] = frame.fence;

		vkAssert(vkResetFences(g_API->device, 1u, &frame.fence));

		VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1u;
		submit_info.pWaitSemaphores = &g_API->swapChain.semAcquireImage;
		submit_info.pWaitDstStageMask = &waitDstStage;
		submit_info.commandBufferCount = g_API->activeCMDCount;
		submit_info.pCommandBuffers = frame.commandBuffers;
		submit_info.signalSemaphoreCount = 1u;
		submit_info.pSignalSemaphores = &g_API->swapChain.semPresent;

		g_API->activeCMDCount = 0u;

		vkAssert(vkQueueSubmit(g_API->queueGraphics, 1u, &submit_info, frame.fence));
	}

	void graphics_vulkan_present()
	{
		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1u;
		present_info.pWaitSemaphores = &g_API->swapChain.semPresent;
		present_info.swapchainCount = 1u;
		present_info.pSwapchains = &g_API->swapChain.swapChain;
		present_info.pImageIndices = &g_API->imageIndex;
		present_info.pResults = nullptr;

		vkAssert(vkQueuePresentKHR(g_API->queueGraphics, &present_info));

		g_API->currentFrame++;
		if (g_API->currentFrame == g_API->frameCount) g_API->currentFrame = 0u;
	}

	Result graphics_vulkan_buffer_create(Buffer_vk& buffer, const GPUBufferDesc& desc)
	{
		VkBufferUsageFlags bufferUsage = 0u;
		bool deviceMemory = !(desc.usage == ResourceUsage_Dynamic && desc.bufferType == GPUBufferType_Constant);
		VmaMemoryUsage memoryUsage = deviceMemory ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU;

		// Usage Flags
		switch (desc.bufferType)
		{
		case GPUBufferType_Vertex:
			bufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case GPUBufferType_Index:
			bufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case GPUBufferType_Constant:
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

			vkCheck(vmaCreateBuffer(g_API->allocator, &buffer_info, &alloc_info, &buffer.buffer, &buffer.allocation, nullptr));
			buffer.memory.Create(desc.size);
		}

		// Set data
		if (desc.pData) {
			if (deviceMemory) {
				VkCommandBuffer cmd;
				vkCheck(graphics_vulkan_singletimecmb_begin(&cmd));

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

				graphics_vulkan_buffer_copy(cmd, stagingBuffer, buffer.buffer, 0u, 0u, desc.size);

				bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				bufferBarrier.dstAccessMask = 0u;

				switch (desc.bufferType)
				{
				case GPUBufferType_Vertex:
					bufferBarrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
					break;
				case GPUBufferType_Index:
					bufferBarrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
					break;
				case GPUBufferType_Constant:
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

				vkCheck(graphics_vulkan_singletimecmb_end(cmd));

				vmaDestroyBuffer(g_API->allocator, stagingBuffer, stagingAllocation);
				
			}
			else {
				void* d;
				vmaMapMemory(g_API->allocator, buffer.allocation, &d);
				memcpy(d, desc.pData, desc.size);
				vmaUnmapMemory(g_API->allocator, buffer.allocation);
			}
		}

		// Desc
		buffer.buffer_info.buffer = buffer.buffer;
		buffer.buffer_info.offset = 0u;
		buffer.buffer_info.range = desc.size;

		return Result_Success;
	}

	Result graphics_vulkan_image_create(Image_vk& image, const GPUImageDesc& desc)
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

			if (desc.type & GPUImageType_RenderTarget) {
				imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
			if (desc.type & GPUImageType_ShaderResource) {
				imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			if (desc.type & GPUImageType_DepthStencil) {
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
			create_info.usage = imageUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0u;
			create_info.pQueueFamilyIndices = nullptr;
			create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			create_info.mipLevels = 1u;

			VmaAllocationCreateInfo alloc_info{};
			alloc_info.requiredFlags = VMA_MEMORY_USAGE_GPU_ONLY;
			
			vkCheck(vmaCreateImage(g_API->allocator, &create_info, &alloc_info, &image.image, &image.allocation, nullptr));
		}

		// Set Data
		VkCommandBuffer cmd;

		vkCheck(graphics_vulkan_singletimecmb_begin(&cmd));
		
		if (desc.pData) {

			VkImageAspectFlags aspect = graphics_vulkan_aspect_from_image_layout(desc.layout, desc.format);

			// Memory barrier to set the image transfer dst layout
			VkImageMemoryBarrier memBarrier{};
			memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memBarrier.srcAccessMask = graphics_vulkan_access_from_image_layout(GPUImageLayout_Undefined);
			memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memBarrier.oldLayout = graphics_vulkan_parse_image_layout(GPUImageLayout_Undefined);
			memBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.image = image.image;
			memBarrier.subresourceRange.aspectMask = aspect;
			memBarrier.subresourceRange.layerCount = desc.layers;
			memBarrier.subresourceRange.levelCount = 1u;

			vkCmdPipelineBarrier(cmd,
				graphics_vulkan_stage_from_image_layout(GPUImageLayout_Undefined),
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

			vkCheck(graphics_vulkan_singletimecmb_end(cmd));

			// Destroy staging buffer
			vmaDestroyBuffer(g_API->allocator, stagingBuffer, stagingAllocation);
		}
		else {

			VkImageMemoryBarrier memBarrier{};
			memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memBarrier.srcAccessMask = graphics_vulkan_access_from_image_layout(GPUImageLayout_Undefined);
			memBarrier.dstAccessMask = graphics_vulkan_access_from_image_layout(desc.layout);
			memBarrier.oldLayout = graphics_vulkan_parse_image_layout(GPUImageLayout_Undefined);
			memBarrier.newLayout = graphics_vulkan_parse_image_layout(desc.layout);
			memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.image = image.image;
			memBarrier.subresourceRange.aspectMask = graphics_vulkan_aspect_from_image_layout(desc.layout, desc.format);
			memBarrier.subresourceRange.layerCount = desc.layers;
			memBarrier.subresourceRange.levelCount = 1u;

			vkCmdPipelineBarrier(cmd,
				graphics_vulkan_stage_from_image_layout(GPUImageLayout_Undefined),
				graphics_vulkan_stage_from_image_layout(desc.layout),
				0u,
				0u,
				nullptr,
				0u,
				nullptr,
				1u,
				&memBarrier);

			vkCheck(graphics_vulkan_singletimecmb_end(cmd));
		}

		// TODO: MipMapping

		// Create Render Target View
		if (desc.type & GPUImageType_RenderTarget) {
			vkCheck(graphics_vulkan_imageview_create(image.image, format, viewType, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers, image.renderTargetView));
		}
		// Create Depth Stencil View
		if (desc.type & GPUImageType_DepthStencil) {
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			
			if (graphics_format_has_stencil(desc.format)) aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

			vkCheck(graphics_vulkan_imageview_create(image.image, format, viewType, aspect, desc.layers, image.depthStencilView));
		}
		// Create Shader Resource View
		if (desc.type & GPUImageType_ShaderResource) {

			if (desc.type & GPUImageType_DepthStencil) {

				vkCheck(graphics_vulkan_imageview_create(image.image, format, viewType, VK_IMAGE_ASPECT_DEPTH_BIT, desc.layers, image.shaderResouceView));

				// Set image info
				image.image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				image.image_info.sampler = VK_NULL_HANDLE;
				image.image_info.imageView = image.shaderResouceView;
			}
			else {

				vkCheck(graphics_vulkan_imageview_create(image.image, format, viewType, VK_IMAGE_ASPECT_COLOR_BIT, desc.layers, image.shaderResouceView));
				
				// Set image info
				image.image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				image.image_info.sampler = VK_NULL_HANDLE;
				image.image_info.imageView = image.shaderResouceView;
			}
		}

		// Set ID
		image.ID = g_API->GetID();

		return Result_Success;
	}

	Result graphics_vulkan_sampler_create(Sampler_vk& sampler, const SamplerDesc& desc)
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

		vkCheck(vkCreateSampler(g_API->device, &create_info, nullptr, &sampler.sampler));

		sampler.image_info.sampler = sampler.sampler;
		sampler.image_info.imageView = VK_NULL_HANDLE;
		sampler.image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return Result_Success;
	}

	Result graphics_vulkan_shader_create(Shader_vk& shader, const ShaderDesc& desc)
	{
		// Create ShaderModule
		{
			VkShaderModuleCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			create_info.flags;
			create_info.codeSize = desc.binDataSize;
			create_info.pCode = reinterpret_cast<const u32*>(desc.pBinData);

			vkCheck(vkCreateShaderModule(g_API->device, &create_info, nullptr, &shader.module));
		}

		// Get Layout from sprv code
		spirv_cross::Compiler comp(reinterpret_cast<const u32*>(desc.pBinData), desc.binDataSize / sizeof(u32));
		spirv_cross::ShaderResources sr = comp.get_shader_resources();

		// Semantic Names
		if (desc.shaderType == ShaderType_Vertex) {
			
			for (u32 i = 0; i < sr.stage_inputs.size(); ++i) {
				auto& input = sr.stage_inputs[i];
				shader.semanticNames[input.name.c_str() + 7] = comp.get_decoration(input.id, spv::Decoration::DecorationLocation);

				ShaderAttribute attr;
				attr.name = std::move(input.name);
				attr.type = graphics_vulkan_parse_spirvtype(comp.get_type(input.base_type_id));

				if (attr.type != ShaderAttributeType_Unknown) shader.info.input.emplace_back() = attr;
			}

		}

		// Get Spirv bindings
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		// Samplers
		{
			auto& samplers = sr.separate_samplers;
			if (!samplers.empty()) {

				size_t initialIndex = bindings.size();
				bindings.resize(initialIndex + samplers.size());

				SV_ASSERT(samplers.size() <= GraphicsLimit_Sampler);

				for (u64 i = 0; i < samplers.size(); ++i) {
					auto& sampler = samplers[i];
					VkDescriptorSetLayoutBinding& binding = bindings[i + initialIndex];
					binding.binding = comp.get_decoration(sampler.id, spv::Decoration::DecorationBinding);
					binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					binding.descriptorCount = 1u;
					binding.stageFlags = graphics_vulkan_parse_shadertype(desc.shaderType);
					binding.pImmutableSamplers = nullptr;
				}
			}
			shader.layout.count[0] = u32(bindings.size());
		}
		
		// Images
		{
			auto& images = sr.separate_images;
			if (!images.empty()) {

				size_t initialIndex = bindings.size();
				bindings.resize(initialIndex + images.size());

				SV_ASSERT(images.size() <= GraphicsLimit_GPUImage);

				for (u64 i = 0; i < images.size(); ++i) {
					auto& image = images[i];
					VkDescriptorSetLayoutBinding& binding = bindings[i + initialIndex];
					binding.binding = comp.get_decoration(image.id, spv::Decoration::DecorationBinding);
					binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					binding.descriptorCount = 1u;
					binding.stageFlags = graphics_vulkan_parse_shadertype(desc.shaderType);
					binding.pImmutableSamplers = nullptr;

					// Add image to shader info
					ShaderInfo::ResourceImage& res = shader.info.images.emplace_back();
					res.name = std::move(image.name);
					res.bindingSlot = binding.binding - GraphicsLimit_Sampler;
				}
			}
			shader.layout.count[1] = u32(bindings.size()) - shader.layout.count[0];
		}

		{
			auto& uniforms = sr.uniform_buffers;
			if (!uniforms.empty()) {

				SV_ASSERT(uniforms.size() <= GraphicsLimit_ConstantBuffer);

				for (u64 i = 0; i < uniforms.size(); ++i) {
					auto& uniform = uniforms[i];
					
					if (strcmp(uniform.name.c_str(), "type__Globals") == 0) {
						continue;
					}

					VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
					binding.binding = comp.get_decoration(uniform.id, spv::Decoration::DecorationBinding);
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					binding.descriptorCount = 1u;
					binding.stageFlags = graphics_vulkan_parse_shadertype(desc.shaderType);
					binding.pImmutableSamplers = nullptr;

					// Add cbuffer to shader info
					ShaderInfo::ResourceBuffer& res = shader.info.constantBuffers.emplace_back();
					res.name = uniform.name.c_str() + 5;
					res.bindingSlot = binding.binding - (GraphicsLimit_Sampler + GraphicsLimit_GPUImage);
					graphics_vulkan_parse_spirvstruct(comp, uniform.base_type_id, res.attributes);
					res.size = res.attributes.empty() ? 0u : (res.attributes.back().offset + graphics_shader_attribute_size(res.attributes.back().type));
				}
			}
			shader.layout.count[2] = u32(bindings.size()) - shader.layout.count[0] - shader.layout.count[1];
		}

		// Calculate user bindings values
		for (u32 i = 0; i < bindings.size(); ++i) {
			const VkDescriptorSetLayoutBinding& binding = bindings[i];
			ShaderResourceBinding& srb = shader.layout.bindings.emplace_back();
			srb.vulkanBinding = binding.binding;
			srb.userBinding = binding.binding;

			switch (binding.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				srb.userBinding -= 0u;
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				srb.userBinding -= GraphicsLimit_Sampler;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				srb.userBinding -= GraphicsLimit_Sampler + GraphicsLimit_GPUImage;
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				srb.userBinding -= GraphicsLimit_Sampler + GraphicsLimit_GPUImage + GraphicsLimit_ConstantBuffer;
				break;
			}
		}

		// Create set layout
		{
			VkDescriptorSetLayoutCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			create_info.bindingCount = u32(bindings.size());
			create_info.pBindings = bindings.data();

			vkCheck(vkCreateDescriptorSetLayout(g_API->device, &create_info, nullptr, &shader.layout.setLayout));
		}

		// ID
		shader.ID = g_API->GetID();

		return Result_Success;
	}

	Result graphics_vulkan_renderpass_create(RenderPass_vk& renderPass, const RenderPassDesc& desc)
	{
		VkAttachmentDescription attachments[GraphicsLimit_Attachment];

		VkAttachmentReference colorAttachments[GraphicsLimit_AttachmentRT];
		VkAttachmentReference depthStencilAttachment;

		VkSubpassDescription subpass_info{};

		bool hasDepthStencil = false;
		u32 colorIt = 0u;

		for (u32 i = 0; i < desc.attachments.size(); ++i) {
			const AttachmentDesc& attDesc = desc.attachments[i];

			attachments[i] = {};

			// Reference
			switch (attDesc.type)
			{
			case AttachmentType_RenderTarget:
				colorAttachments[colorIt].layout = graphics_vulkan_parse_image_layout(attDesc.layout);
				colorAttachments[colorIt].attachment = i;
				colorIt++;
				break;
			case AttachmentType_DepthStencil:
				if (hasDepthStencil) return Result_InvalidUsage;

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

			if (attDesc.type == AttachmentType_DepthStencil) {
				attachments[desc.attachments.size()].stencilLoadOp = graphics_vulkan_parse_attachment_loadop(attDesc.stencilLoadOp);
				attachments[desc.attachments.size()].stencilStoreOp = graphics_vulkan_parse_attachment_storeop(attDesc.stencilStoreOp);
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
		create_info.attachmentCount = u32(desc.attachments.size());
		create_info.pAttachments = attachments;

		vkCheck(vkCreateRenderPass(g_API->device, &create_info, nullptr, &renderPass.renderPass));
		
		return Result_Success;
	}

	Result graphics_vulkan_inputlayoutstate_create(InputLayoutState_vk& inputLayoutState, const InputLayoutStateDesc& desc)
	{
		inputLayoutState.hash = graphics_compute_hash_inputlayoutstate(&desc);
		return Result_Success;
	}

	Result graphics_vulkan_blendstate_create(BlendState_vk& blendState, const BlendStateDesc& desc)
	{
		blendState.hash = graphics_compute_hash_blendstate(&desc);
		return Result_Success;
	}

	Result graphics_vulkan_depthstencilstate_create(DepthStencilState_vk& depthStencilState, const DepthStencilStateDesc& desc)
	{
		depthStencilState.hash = graphics_compute_hash_depthstencilstate(&desc);
		return Result_Success;
	}

	Result graphics_vulkan_rasterizerstate_create(RasterizerState_vk& rasterizerState, const RasterizerStateDesc& desc)
	{
		rasterizerState.hash = graphics_compute_hash_rasterizerstate(&desc);
		return Result_Success;
	}

	Result graphics_vulkan_buffer_destroy(Buffer_vk& buffer)
	{
		vmaDestroyBuffer(g_API->allocator, buffer.buffer, buffer.allocation);
		buffer.memory.Clear();
		return Result_Success;
	}

	Result graphics_vulkan_image_destroy(Image_vk& image)
	{
		vmaDestroyImage(g_API->allocator, image.image, image.allocation);
		if (image.renderTargetView != VK_NULL_HANDLE)	vkDestroyImageView(g_API->device, image.renderTargetView, nullptr);
		if (image.depthStencilView != VK_NULL_HANDLE)	vkDestroyImageView(g_API->device, image.depthStencilView, nullptr);
		if (image.shaderResouceView != VK_NULL_HANDLE)	vkDestroyImageView(g_API->device, image.shaderResouceView, nullptr);
		return Result_Success;
	}

	Result graphics_vulkan_sampler_destroy(Sampler_vk& sampler)
	{
		vkDestroySampler(g_API->device, sampler.sampler, nullptr);
		return Result_Success;
	}

	Result graphics_vulkan_shader_destroy(Shader_vk& shader)
	{
		vkDestroyShaderModule(g_API->device, shader.module, nullptr);
		vkDestroyDescriptorSetLayout(g_API->device, shader.layout.setLayout, nullptr);
		return Result_Success;
	}

	Result graphics_vulkan_renderpass_destroy(RenderPass_vk& renderPass)
	{
		vkDestroyRenderPass(g_API->device, renderPass.renderPass, nullptr);
		for (auto& it : renderPass.frameBuffers) {
			vkDestroyFramebuffer(g_API->device, it.second, nullptr);
		}
		return Result_Success;
	}

}