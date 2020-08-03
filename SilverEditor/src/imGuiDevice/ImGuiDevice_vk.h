#pragma once

#include "ImGuiDevice.h"
#include "src/graphics/vulkan/graphics_vulkan.h"

#ifdef SV_PLATFORM_WINDOWS

#include "..//external/ImGui/imgui_impl_win32.h"

#endif

#include "..//external/ImGui/imgui_impl_vulkan.h"

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

		VkSampler m_ImageSampler;
		std::map<_sv::Image_internal*, std::pair<VkImageView, ImTextureID>> m_Images;

	public:
		bool Initialize() override;
		bool Close() override;

		void BeginFrame() override;
		void EndFrame() override;

		ImTextureID ParseImage(sv::Image& image) override;

	};

}