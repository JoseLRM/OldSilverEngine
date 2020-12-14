#include "core_editor.h"

#include "ImGuiDevice_vk.h"
#include "platform/window.h"

namespace sv {

	void ErrorHandler(VkResult res)
	{
		vkAssert(res);
	}

	u64 DeviceWindowProc(WindowHandle handle, u32 msg, u64 wParam, i64 lParam)
	{
		return ImGui_ImplWin32_WndProcHandler((HWND)window_handle_get(), msg, wParam, lParam);
	}

	Result ImGuiDevice_vk::Initialize()
	{
		// Input handling
		window_userproc_set(DeviceWindowProc);

		// Graphics
		ImGui_ImplWin32_EnableDpiAwareness();

		IMGUI_CHECKVERSION();
		m_Ctx = ImGui::CreateContext();

		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DpiEnableScaleViewports;;
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_DpiEnableScaleViewports;;

		{
			const char* filePath = "library/fonts/open_sans/OpenSans-Regular.ttf";
			const char* filePathBold = "library/fonts/open_sans/OpenSans-Bold.ttf";
#ifdef SV_RES_PATH
			std::string filePathStr = SV_RES_PATH;
			filePathStr += filePath;
			filePath = filePathStr.c_str();

			std::string filePathBoldStr = SV_RES_PATH;
			filePathBoldStr += filePathBold;
			filePathBold = filePathBoldStr.c_str();
#endif
			
			io.Fonts->AddFontFromFileTTF(filePathBold, 15.f);
			io.FontDefault = io.Fonts->AddFontFromFileTTF(filePath, 15.f);
			
		}

		if (!ImGui_ImplWin32_Init(window_handle_get())) return Result_PlatformError;

		Graphics_vk& gfx = graphics_vulkan_device_get();
		Adapter_vk& adapter = *reinterpret_cast<Adapter_vk*>(graphics_adapter_get());

		// Create Descriptor Pool
		{
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			create_info.maxSets = 100;
			create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
			create_info.pPoolSizes = pool_sizes;

			vkCheck(vkCreateDescriptorPool(gfx.device, &create_info, nullptr, &m_DescPool));
		}

		// Render Pass
		{
			VkAttachmentDescription att{};
			att.flags = 0u;
			att.format = gfx.swapChain.currentFormat;
			att.samples = VK_SAMPLE_COUNT_1_BIT;
			att.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			att.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference ref{};
			ref.attachment = 0u;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1u;
			subpass.pColorAttachments = &ref;

			VkRenderPassCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			create_info.attachmentCount = 1u;
			create_info.pAttachments = &att;
			create_info.subpassCount = 1u;
			create_info.pSubpasses = &subpass;
			create_info.dependencyCount = 0u;
			create_info.pDependencies = nullptr;

			vkCheck(vkCreateRenderPass(gfx.device, &create_info, nullptr, &m_RenderPass));
		}
		// Image sampler
		{
			VkSamplerCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			create_info.magFilter = VK_FILTER_LINEAR;
			create_info.minFilter = VK_FILTER_LINEAR;
			create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			vkCheck(vkCreateSampler(gfx.device, &create_info, nullptr, &m_ImageSampler));
		}


		vkCheck(CreateFrames());

		ImGui_ImplVulkan_InitInfo info{};
		info.QueueFamily = adapter.GetFamilyIndex().graphics;
		info.Instance = gfx.instance;
		info.PhysicalDevice = adapter.GetPhysicalDevice();
		info.Device = gfx.device;
		info.Queue = gfx.queueGraphics;
		info.PipelineCache = 0;
		info.DescriptorPool = m_DescPool;
		info.MinImageCount = gfx.swapChain.capabilities.minImageCount;
		info.ImageCount = u32(gfx.swapChain.images.size());
		info.CheckVkResultFn = ErrorHandler;

		ImGui_ImplVulkan_Init(&info, m_RenderPass);

		VkCommandBuffer cmd;
		vkCheck(graphics_vulkan_singletimecmb_begin(&cmd));
		if (!ImGui_ImplVulkan_CreateFontsTexture(cmd)) return Result_UnknownError;
		vkCheck(graphics_vulkan_singletimecmb_end(cmd));

		return Result_Success;
	}
	Result ImGuiDevice_vk::Close()
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		graphics_gpu_wait();

		DestroyFrames();

		vkDestroyRenderPass(gfx.device, m_RenderPass, nullptr);
		vkDestroyDescriptorPool(gfx.device, m_DescPool, nullptr);
		vkDestroySampler(gfx.device, m_ImageSampler, nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(m_Ctx);
		return Result_Success;
	}

	void ImGuiDevice_vk::BeginFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		m_CommandList = graphics_commandlist_begin();
	}
	void ImGuiDevice_vk::EndFrame()
	{
		graphics_vulkan_acquire_image();

		Graphics_vk& gfx = graphics_vulkan_device_get();
		ImGui::Render();

		u32 currentFrame = gfx.currentFrame;
		
		VkCommandBuffer cmd = gfx.GetCMD(m_CommandList);

		// From VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Frames[currentFrame].image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0u;
		barrier.subresourceRange.baseMipLevel = 0u;
		barrier.subresourceRange.layerCount = 1u;
		barrier.subresourceRange.levelCount = 1u;

		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);

		VkRenderPassBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		begin_info.renderPass = m_RenderPass;
		begin_info.framebuffer = m_Frames[currentFrame].frameBuffer;
		begin_info.renderArea.offset = { 0, 0 };
		begin_info.renderArea.extent = m_SwapChainSize;
		begin_info.clearValueCount = 0u;
		begin_info.pClearValues = nullptr;

		vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		vkCmdEndRenderPass(cmd);

		// From VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		std::swap(barrier.srcAccessMask, barrier.dstAccessMask);
		std::swap(barrier.oldLayout, barrier.newLayout);
		std::swap(srcStage, dstStage);

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);

		ImGui::EndFrame();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	void ImGuiDevice_vk::ResizeSwapChain()
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();
		VkExtent2D currentSize = gfx.swapChain.currentExtent;

		graphics_gpu_wait();

		if (currentSize.width != m_SwapChainSize.width || currentSize.height != m_SwapChainSize.height) {
			DestroyFrames();
			vkAssert(CreateFrames());
		}
	}

	ImTextureID ImGuiDevice_vk::ParseImage(GPUImage* image_)
	{
		Image_vk& image = *reinterpret_cast<Image_vk*>(image_);
		SV_ASSERT(image.image != VK_NULL_HANDLE);

		auto it = m_Images.find(&image);
		if (it == m_Images.end() || it->second.first != image.shaderResouceView) {

			if (it != m_Images.end()) {
				Graphics_vk& gfx = graphics_vulkan_device_get();
				vkFreeDescriptorSets(gfx.device, m_DescPool, 1u, (VkDescriptorSet*)(&it->second.second));
			}

			ImTextureID tex = ImGui_ImplVulkan_AddTexture(m_ImageSampler, image.shaderResouceView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_Images[&image] = std::make_pair(image.shaderResouceView, tex);
			return tex;
		}
		else {
			return it->second.second;
		}
	}

	VkResult ImGuiDevice_vk::CreateFrames()
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		// Create Framebuffers
		{
			auto& fb = gfx.swapChain;

			m_SwapChainSize = fb.currentExtent;
			m_Frames.clear();
			m_Frames.resize(fb.images.size());

			for (u32 i = 0; i < fb.images.size(); ++i) {
				Frame& frame = m_Frames[i];

				frame.view = fb.images[i].view;
				frame.image = fb.images[i].image;

				VkFramebufferCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				create_info.renderPass = m_RenderPass;
				create_info.attachmentCount = 1u;
				create_info.pAttachments = &frame.view;
				create_info.width = window_size_get().x;
				create_info.height = window_size_get().y;
				create_info.layers = 1u;

				vkExt(vkCreateFramebuffer(gfx.device, &create_info, nullptr, &frame.frameBuffer));
			}
		}
		return VK_SUCCESS;
	}

	void ImGuiDevice_vk::DestroyFrames()
	{
		Graphics_vk& gfx = graphics_vulkan_device_get();

		for (u32 i = 0; i < m_Frames.size(); ++i) {
			vkDestroyFramebuffer(gfx.device, m_Frames[i].frameBuffer, nullptr);
		}
	}

}