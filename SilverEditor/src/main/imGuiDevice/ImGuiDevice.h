#pragma once

#include "core_editor.h"
#include "platform/graphics.h"

namespace sv {

	class ImGuiDevice {
	protected:
		ImGuiContext* m_Ctx = nullptr;
		CommandList m_CommandList;

	public:
		virtual Result Initialize() = 0;
		virtual Result Close() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void ResizeSwapChain() = 0;

		virtual ImTextureID ParseImage(GPUImage* image) = 0;

		inline ImGuiContext* GetCtx() const noexcept { return m_Ctx; }
		inline CommandList GetCMD() const noexcept { return m_CommandList; }

	};

	std::unique_ptr<ImGuiDevice> device_create();

}