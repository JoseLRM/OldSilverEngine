#pragma once

#include "core_editor.h"
#include "graphics.h"

namespace sve {

	class ImGuiDevice {
	protected:
		ImGuiContext* m_Ctx = nullptr;

	public:
		virtual sv::Result Initialize() = 0;
		virtual sv::Result Close() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void ResizeSwapChain() = 0;

		virtual ImTextureID ParseImage(sv::GPUImage& image) = 0;

		inline ImGuiContext* GetCtx() const noexcept { return m_Ctx; }

	};

	std::unique_ptr<ImGuiDevice> device_create();

}