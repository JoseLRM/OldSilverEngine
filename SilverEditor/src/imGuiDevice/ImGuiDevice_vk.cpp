#include "core_editor.h"

#include "ImGuiDevice_vk.h"

using namespace sv;

namespace sve {

	void ErrorHandler(VkResult res)
	{
		vkAssert(res);
	}

	void DeviceWindowProc(ui32& msg, ui64& wParam, i64& lParam)
	{
		ImGui_ImplWin32_WndProcHandler((HWND)window_get_handle(), msg, wParam, lParam);
	}

	bool ImGuiDevice_vk::Initialize()
	{
		// Input handling
		auto& platform = sv::window_get_platform();
		platform.AddWindowProc(DeviceWindowProc);

		// Graphics
		ImGui_ImplWin32_EnableDpiAwareness();

		IMGUI_CHECKVERSION();
		m_Ctx = ImGui::CreateContext();

		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DpiEnableScaleViewports;;
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_DpiEnableScaleViewports;;

		svCheck(ImGui_ImplWin32_Init(window_get_handle()));

		sv::Graphics_vk& gfx = sv::graphics_vulkan_device_get();
		sv::Adapter_vk& adapter = *reinterpret_cast<sv::Adapter_vk*>(sv::graphics_adapter_get());

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

			vkCheck(vkCreateDescriptorPool(gfx.GetDevice(), &create_info, nullptr, &m_DescPool));
		}

		// Render Pass
		{
			VkAttachmentDescription att{};
			att.flags = 0u;
			att.format = gfx.GetSwapChain().currentFormat;
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

			vkCheck(vkCreateRenderPass(gfx.GetDevice(), &create_info, nullptr, &m_RenderPass));
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

			vkCheck(vkCreateSampler(gfx.GetDevice(), &create_info, nullptr, &m_ImageSampler));
		}


		vkCheck(CreateFrames());

		ImGui_ImplVulkan_InitInfo info{};
		info.QueueFamily = adapter.GetFamilyIndex().graphics;
		info.Instance = gfx.GetInstance();
		info.PhysicalDevice = adapter.GetPhysicalDevice();
		info.Device = gfx.GetDevice();
		info.Queue = gfx.GetGraphicsQueue();
		info.PipelineCache = 0;
		info.DescriptorPool = m_DescPool;
		info.MinImageCount = gfx.GetSwapChain().capabilities.minImageCount;
		info.ImageCount = gfx.GetSwapChain().images.size();
		info.CheckVkResultFn = ErrorHandler;

		ImGui_ImplVulkan_Init(&info, m_RenderPass);

		VkCommandBuffer cmd;
		vkCheck(gfx.BeginSingleTimeCMD(&cmd));
		svCheck(ImGui_ImplVulkan_CreateFontsTexture(cmd));
		vkCheck(gfx.EndSingleTimeCMD(cmd));

		return true;
	}
	bool ImGuiDevice_vk::Close()
	{
		sv::Graphics_vk& gfx = sv::graphics_vulkan_device_get();

		graphics_gpu_wait();

		DestroyFrames();

		vkDestroyRenderPass(gfx.GetDevice(), m_RenderPass, nullptr);
		vkDestroyDescriptorPool(gfx.GetDevice(), m_DescPool, nullptr);
		vkDestroySampler(gfx.GetDevice(), m_ImageSampler, nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(m_Ctx);
		return true;
	}

	void ImGuiDevice_vk::BeginFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}
	void ImGuiDevice_vk::EndFrame()
	{
		sv::Graphics_vk& gfx = sv::graphics_vulkan_device_get();
		ImGui::Render();

		ui32 currentFrame = gfx.GetCurrentFrame();
		CommandList cmd_ = graphics_commandlist_last();
		VkCommandBuffer cmd = gfx.GetCMD(cmd_);

		// From VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Frames[currentFrame].image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0u;
		barrier.subresourceRange.baseMipLevel = 0u;
		barrier.subresourceRange.layerCount = 1u;
		barrier.subresourceRange.levelCount = 1u;

		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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

		ImGui::EndFrame();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	void ImGuiDevice_vk::ResizeSwapChain()
	{
		sv::Graphics_vk& gfx = sv::graphics_vulkan_device_get();
		VkExtent2D currentSize = gfx.GetSwapChain().currentExtent;

		sv::graphics_gpu_wait();

		if (currentSize.width != m_SwapChainSize.width || currentSize.height != m_SwapChainSize.height) {
			DestroyFrames();
			vkAssert(CreateFrames());
		}
	}

	ImTextureID ImGuiDevice_vk::ParseImage(sv::GPUImage& image_)
	{
		sv::Image_vk& image = *reinterpret_cast<sv::Image_vk*>(image_.GetPtr());
		SV_ASSERT(image.image != VK_NULL_HANDLE);

		auto it = m_Images.find(&image);
		if (it == m_Images.end() || it->second.first != image.shaderResouceView) {

			if (it != m_Images.end()) {
				sv::Graphics_vk& gfx = sv::graphics_vulkan_device_get();
				vkFreeDescriptorSets(gfx.GetDevice(), m_DescPool, 1u, (VkDescriptorSet*)(&it->second.second));
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
		sv::Graphics_vk& gfx = sv::graphics_vulkan_device_get();

		// Create Framebuffers
		{
			auto& fb = gfx.GetSwapChain();

			m_SwapChainSize = fb.currentExtent;
			m_Frames.clear();
			m_Frames.resize(fb.images.size());

			for (ui32 i = 0; i < fb.images.size(); ++i) {
				Frame& frame = m_Frames[i];

				frame.view = fb.images[i].view;
				frame.image = fb.images[i].image;

				VkFramebufferCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				create_info.renderPass = m_RenderPass;
				create_info.attachmentCount = 1u;
				create_info.pAttachments = &frame.view;
				create_info.width = window_get_width();
				create_info.height = window_get_height();
				create_info.layers = 1u;

				vkExt(vkCreateFramebuffer(gfx.GetDevice(), &create_info, nullptr, &frame.frameBuffer));
			}
		}
		return VK_SUCCESS;
	}

	void ImGuiDevice_vk::DestroyFrames()
	{
		sv::Graphics_vk& gfx = sv::graphics_vulkan_device_get();

		for (ui32 i = 0; i < m_Frames.size(); ++i) {
			vkDestroyFramebuffer(gfx.GetDevice(), m_Frames[i].frameBuffer, nullptr);
		}
	}

}