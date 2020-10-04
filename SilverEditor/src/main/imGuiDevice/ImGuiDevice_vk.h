#pragma once

#include "ImGuiDevice.h"
#define SV_VULKAN_IMPLEMENTATION
#include "graphics/vulkan/graphics_vulkan.h"

#ifdef SV_PLATFORM_WINDOWS

#include "external/ImGui/imgui_impl_win32.h"

#endif

#include "external/ImGui/imgui_impl_vulkan.h"

namespace sve {

	class ImGuiDevice_vk : public ImGuiDevice {

		struct Frame {
			VkImage image;
			VkImageView view;
			VkFramebuffer frameBuffer;
		};

		VkRenderPass m_RenderPass;
		VkDescriptorPool m_DescPool;
		std::vector<Frame> m_Frames;
		VkExtent2D m_SwapChainSize;

		VkSampler m_ImageSampler;
		std::map<sv::GPUImage_internal*, std::pair<VkImageView, ImTextureID>> m_Images;

	public:
		sv::Result Initialize() override;
		sv::Result Close() override;

		void BeginFrame() override;
		void EndFrame() override;

		void ResizeSwapChain() override;

		ImTextureID ParseImage(sv::GPUImage* image) override;

	private:
		VkResult CreateFrames();
		void DestroyFrames();

	};

}